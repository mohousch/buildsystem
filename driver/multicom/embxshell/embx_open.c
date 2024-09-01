/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_transport.c                                          */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Transport open and initialize                      */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"

#include "embxshell.h"
#include "debug_ctrl.h"

#if defined(EMBX_VERBOSE)
static char *_driver_uninit_error = "Failed 'EMBX Driver is not initialized'\n";
#endif /* EMBX_VERBOSE */


/**************************************************************
 * Helper function which initializes a transport for the first
 * time. The complexity is in having to block if the transport
 * cannot complete the initialization immediately. In that
 * case we may get woken up in a number of ways:
 *
 * 1. The initialization succeeds
 * 2. EMBX_Deinit has been called while we have relinquished
 *    the driver lock
 * 3. The OS has interrupted the wait (e.g. a signal is delivered
 *    to the blocked process on Linux) and the initialization
 *    didn't complete by the time we were able to notify the
 *    transport that this happened
 * 4. As 3. but the initialization did finish before we could
 *    tell the transport we no longer wanted it to complete.
 */
static EMBX_ERROR _initialise_transport(EMBX_Transport_t *tp)
{
EMBX_EventState_t evNode;
EMBX_ERROR res;

    if(!EMBX_OS_EVENT_INIT(&evNode.event))
    {
        return EMBX_NOMEM;
    }

    /*
     * Preset the result if blocking so we can catch OS interrupts
     */
    evNode.result = EMBX_SYSTEM_INTERRUPT;

    EMBX_EventListAdd(&tp->initWaitList,&evNode);

    tp->state = EMBX_TP_IN_INITIALIZATION;
    
    res = tp->methods->initialize(tp, &evNode);

    if(res == EMBX_BLOCK)
    {
    EMBX_ERROR bInterrupt;

        EMBX_Info(EMBX_INFO_TRANSPORT, ("Waiting for transport initialization\n"));

        /*
         * Release the driver lock and wait
         */
        EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
        bInterrupt = EMBX_OS_EVENT_WAIT(&evNode.event);
        EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

        if(bInterrupt)
        {
            tp->methods->init_interrupt(tp);
        }

        res = evNode.result;
    }


    /*  Clean up the event */
    EMBX_OS_EVENT_DESTROY(&evNode.event);

    switch (res)
    {
        case EMBX_SUCCESS:
        {
            tp->state = EMBX_TP_INITIALIZED;
            tp->transportInfo.isInitialized = EMBX_TRUE;

            /* Transport is alive so remove ourselves from the wait list,
             * before we signal everyone else in the list to wake up.
             */
            EMBX_EventListRemove(&tp->initWaitList,&evNode);

            /* The initialization succeded so signal any other
             * blocked tasks to continue. We are under the
             * driver lock at this point so the tasks will block
             * immediately waiting for the lock. This means access
             * to the whole list is safe.
             */
            EMBX_EventListSignal(&tp->initWaitList, EMBX_SUCCESS);

            /* Clear the wait list now we have signalled everyone */
            tp->initWaitList = 0;

            EMBX_Info(EMBX_INFO_TRANSPORT, ("Transport initialized\n"));

            return EMBX_SUCCESS;
        } /* EMBX_SUCCESS */

        case EMBX_SYSTEM_INTERRUPT:
        {
            /* We got interrupted by the OS and the initialise hasn't completed
             * so we have to roll things back so the initialisation
             * process can start again
             */

            EMBX_EventListRemove(&tp->initWaitList,&evNode);
            EMBX_EventListSignal(&tp->initWaitList, EMBX_SYSTEM_ERROR);

            /* Clear the wait list now we have signalled everyone */
            tp->initWaitList = 0;

            /* We set the transport state to failed so that any outstanding
             * resources or threads contexts can be cleaned up by EMBX_Deinit,
             * EMBX_UnregisterTransport or by EMBX_OpenTransport retrying
             * the initialization.
             */
            tp->state = EMBX_TP_FAILED; 
            tp->transportInfo.isInitialized = EMBX_FALSE;

            EMBX_Info(EMBX_INFO_TRANSPORT, ("Transport initialization interrupted by OS\n"));

            return EMBX_SYSTEM_INTERRUPT;
        } /* EMBX_SYSTEM_INTERRUPT */

        case EMBX_TRANSPORT_CLOSED:
        {
            /*
             * IMPORTANT: at this point the transport structure may already be
             *            freed and a driver closedown may have already completed.
             *            do not access tp or anything in the driver context other than
             *            the driver lock (which is guarenteed to persist even after
             *            EMBX_Deinit). All other waiters will have been signalled too.
             */
            EMBX_Info(EMBX_INFO_TRANSPORT, ("Transport initialization interrupted by driver\n"));

            return EMBX_TRANSPORT_CLOSED;
        } /* EMBX_TRANSPORT_CLOSED */

        default:
        {
            EMBX_Info(EMBX_INFO_TRANSPORT, ("Transport initialization failed\n"));

            tp->state = EMBX_TP_FAILED;
            tp->transportInfo.isInitialized = EMBX_FALSE;

            return res;
        } /* default */
    }
}



/*
 * This helper function blocks a second or subsequent call to open
 * the transport until the first call, blocked waiting 
 * for the transport to initialize, signals that the transport
 * has either been initialized successfully or that an error
 * occurred.
 */
static EMBX_ERROR _initialise_wait(EMBX_Transport_t *tp)
{
EMBX_EventList_t evNode;
EMBX_ERROR bInterrupt;
EMBX_ERROR res;


    if(!EMBX_OS_EVENT_INIT(&evNode.event))
    {
        return EMBX_NOMEM;
    }

    EMBX_Info(EMBX_INFO_TRANSPORT, ("Waiting for transport initialization\n"));

    /*
     * Preset the result if blocking so we can catch OS interrupts
     */
    evNode.result = EMBX_SYSTEM_INTERRUPT;

    EMBX_EventListAdd(&tp->initWaitList, &evNode);

    /*
     * Release the driver lock and wait
     */
    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
    bInterrupt = EMBX_OS_EVENT_WAIT(&evNode.event);
    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    res = evNode.result;

    if(bInterrupt && res == EMBX_SYSTEM_INTERRUPT)
    {
        /*
         * We got interrupted by the OS and the initialization hadn't finished
         * by the time we managed to re-take the driver lock
         */
        EMBX_Info(EMBX_INFO_TRANSPORT, ("Transport initialization interrupted by OS\n"));

        /*
         * We need to remove ourselves from the list as the task
         * doing the initialization may still complete and signal
         * the list. We don't want to be on the list when that
         * happens as we are about to exit. The list is only accessed
         * under the driver lock so we can't be signalled as we are
         * removing ourselves.
         */
        EMBX_EventListRemove(&tp->initWaitList, &evNode);
    }
    else
    {
        if(res != EMBX_SUCCESS)
        {
            /*
             * IMPORTANT: at this point the transport structure may already be
             *            freed and a driver closedown may have already have completed.
             *            do not access tp or anything in the driver context other than
             *            the driver lock (which is guaranteed to persist even after
             *            EMBX_Deinit)
             */
            EMBX_Info(EMBX_INFO_TRANSPORT, ("Transport initialization interrupted by Driver\n"));
        }
        else
        {
            EMBX_Info(EMBX_INFO_TRANSPORT, ("Transport initialized\n"));
        }
    }

    /*
     * Now we know nothing references it anymore we can
     * destroy the event.
     */
    EMBX_OS_EVENT_DESTROY(&evNode.event);

    return res;
}



/*
 * This helper function manages the first use initialization
 * of a transport. 
 */
static EMBX_ERROR _initialise(EMBX_Transport_t *tp)
{
    switch(tp->state)
    {
        case EMBX_TP_UNINITIALIZED:
        {
            return _initialise_transport(tp);
        } /* EMBX_TP_UNINITIALIZED */

        case EMBX_TP_IN_INITIALIZATION:
        {
            return _initialise_wait(tp);
        } /* EMBX_TP_IN_INITIALIZATION */

        case EMBX_TP_FAILED:
        {
        EMBX_ERROR bInterrupted;
        EMBX_ERROR res;
            /*
             * A previous attempt to initialize the transport failed
             * or was interrupted by a system interrupt so lets try
             * again.
             */
            res = EMBX_close_transport(tp, &bInterrupted);

            tp->state = (res != EMBX_SUCCESS) ? EMBX_TP_FAILED : EMBX_TP_UNINITIALIZED;
            tp->transportInfo.isInitialized = EMBX_FALSE;

            return (bInterrupted || res != EMBX_SUCCESS) ? res : _initialise_transport(tp);
        } /* EMBX_TP_FAILED */

        case EMBX_TP_IN_CLOSEDOWN:
        {
            /*
             * The transport is being destroyed by unregister transport
             * (it can't be due to EMBX_Deinit as we would have failed
             *  an earlier test.) So pretend we never found the transport
             * at all.
             */
            EMBX_DebugMessage(("Failed 'Transport in the process of closing down'\n"));

            return EMBX_INVALID_ARGUMENT;
        } /* EMBX_TP_IN_CLOSEDOWN */

        case EMBX_TP_INITIALIZED:
        {
            return EMBX_SUCCESS;
        } /* EMBX_TP_INITIALIZED */

        default:
        {
            EMBX_DebugMessage(("Failed 'Transport State is corrupted'\n"));

            return EMBX_SYSTEM_ERROR;
            break;
        }
    }

    return EMBX_SYSTEM_ERROR;
}



static EMBX_ERROR _allocate_handle_resources(EMBX_TransportHandleList_t **node, EMBX_TRANSPORT *htp)
{
    /*
     * Try to allocate a transport handle list node, if we can't allocate
     * one then fail.
     */
    *node = (EMBX_TransportHandleList_t *)EMBX_OS_MemAlloc(sizeof(EMBX_TransportHandleList_t));
    if(*node != 0)
    {
        /*
         * Try and get a new public HANDLE, if we can't allocate one then fail.
         */
        *htp = EMBX_HANDLE_CREATE(&_embx_handle_manager, 0, EMBX_HANDLE_TYPE_TRANSPORT);
        if(*htp != EMBX_INVALID_HANDLE_VALUE)
        {
            return EMBX_SUCCESS;
        }
        else
        {
            EMBX_DebugMessage(("Failed 'no more free handles'\n"));

            EMBX_OS_MemFree(*node);
            *node = 0;
        }
    }

    return EMBX_NOMEM;
}



static void _register_handle(EMBX_Transport_t *tp,        EMBX_TransportHandleList_t *node,
                             EMBX_TransportHandle_t *tph, EMBX_TRANSPORT htp)
{
    /* Attach the handle structure to the public HANDLE */
    EMBX_HANDLE_ATTACHOBJ(&_embx_handle_manager, htp, tph);

    /* Setup handle state */
    tph->state        = EMBX_HANDLE_VALID;
    tph->publicHandle = htp;
    tph->transport    = tp;

    /* Add to transport's transport handle list */
    node->transportHandle = tph;
    node->prev = 0;
    node->next = tp->transportHandles;

    if(node->next != 0)
        node->next->prev = node;


    tp->transportHandles = node;

    /* Keep count of number of handles opened */
    tp->transportInfo.nrOpenHandles++;

    EMBX_Info(EMBX_INFO_TRANSPORT, ("Opened transport, structure address = 0x%lx HANDLE = 0x%lx\n",(unsigned long)tph,(unsigned long)htp));
}



EMBX_ERROR EMBX_OpenTransport(const EMBX_CHAR *name, EMBX_TRANSPORT *htp)
{
EMBX_ERROR res;
EMBX_TransportList_t *tpl;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_OpenTransport(%s)\n",(name==0?"(NULL)":name)));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_DRIVER_NOT_INITIALIZED;
        goto exit;
    }

    if((name == 0) || (htp == 0))
    {
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_DRIVER_NOT_INITIALIZED;
    }
    else
    {
        tpl = EMBX_find_transport_entry(name);
        if(tpl != 0)
        {
        EMBX_Transport_t *tp = tpl->transport;
        EMBX_TransportHandle_t *tph;
        EMBX_TransportHandleList_t *node;

            EMBX_Info(EMBX_INFO_TRANSPORT, ("Found transport\n"));

            /*
             * Initialise the transport, if necessary
             */
            res = _initialise(tp);
            if(res == EMBX_SUCCESS)
            {
                /*
                 * Allocate the resources needed by the shell
                 * to manage the transport handle.
                 */
                res = _allocate_handle_resources(&node,htp);
                if(res == EMBX_SUCCESS)
                {
                    /*
                     * Now try and get a new handle structure from the transport
                     */
                    res = tp->methods->open_handle(tp, &tph);
                    if(res != EMBX_SUCCESS)
                    {
                        EMBX_OS_MemFree(node);
                        EMBX_HANDLE_FREE(&_embx_handle_manager, *htp);
                        *htp = 0;
                    }
                    else
                    {
                        /*
                         * fix up the shell data structures to
                         * register the handle for use.
                         */
                        _register_handle(tp,node,tph,*htp);
                    }
                }
            }
        }
        else
        {
            EMBX_DebugMessage(("Failed 'Could not find transport'\n"));
            res = EMBX_INVALID_ARGUMENT;
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_OpenTransport(%s) = %d\n",(name==0?"(NULL)":name), res));

    return res;
}

