/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_deinit.c                                             */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Driver Closedown API entrypoints                   */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"

#include "embxshell.h"
#include "debug_ctrl.h"

/*
 * Manage multiple tasks/processes waiting on EMBX_Deinit
 * to complete.
 */
static EMBX_ERROR _deinit_wait(void)
{
EMBX_ERROR res;
EMBX_EventState_t ev;
EMBX_ERROR bInterrupt;
   
    /* Another call to shut the driver down is in progress so
     * we need to wait for that to complete.
     */   
    if(!EMBX_OS_EVENT_INIT(&ev.event))
    {
        return EMBX_SYSTEM_ERROR;
    }

    ev.result = EMBX_SYSTEM_INTERRUPT;

    EMBX_EventListAdd(&_embx_dvr_ctx.deinitWaitList, &ev);

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
    bInterrupt = EMBX_OS_EVENT_WAIT(&ev.event);
    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    EMBX_OS_EVENT_DESTROY(&ev.event);
    res = ev.result;

    if(bInterrupt && res == EMBX_SYSTEM_INTERRUPT)
    {
        /* This was an OS interrupt so we must remove
         * outselves from the wait list.
         */
        EMBX_EventListRemove(&_embx_dvr_ctx.deinitWaitList, &ev);
    }

    return res;
}


/*
 * Helper function for _closedown_transports and _closedown_factory_transports
 * to invalidate all transport and port handles on a transport,
 * wake up and abort blocked initialization and to wake up
 * blocked connect and receive calls.
 */
static EMBX_VOID _invalidate_transport(EMBX_Transport_t *tp)
{
    if(tp->state == EMBX_TP_IN_INITIALIZATION)
    {
        /* If the transport is waiting for initialization
         * to complete then inform the transport that
         * we want to interrupt the process
         */
        tp->methods->init_interrupt(tp);

        /* This is nasty, although we tried to interrupt
         * the initialization it may just have succeeded
         * so we have to be able to cleanup the transport's
         * resources in closedown. The first step is to
         * mark the transport state as "failed" 
         */
        tp->state = EMBX_TP_FAILED;

        /* Wake up any processes that were waiting for this
         * transport to initialize, including the task that
         * initiated the initialization in the first place.
         * If the transport succeeded in signalling the task
         * it had completed, while we have had the driver lock
         * then this will OVERWRITE the result that the
         * waiting task will eventually get when we finally
         * release the driver lock and it gets scheduled.
         */
        EMBX_EventListSignal(&tp->initWaitList, EMBX_TRANSPORT_CLOSED);

        /* Once we have singalled everything with the error, forget
         * about the list so we don't accidently do it again. The
         * structures composing the list are on the stacks of the
         * tasks we have just woken up and should no longer be valid.
         */
        tp->initWaitList = 0;
    }
    else if(tp->state == EMBX_TP_INITIALIZED)
    {
        /* Ask the transport to invalidate all transport handles
         * and port handles and wake up any blocked processes
         * waiting for connections or to receive events.
         */
        tp->methods->invalidate(tp);
    }
}



/*
 * Helper function to close down a transport. This is also used by
 * EMBX_OpenTransport as part of retrying the initialization of
 * a failed transport.
 */
EMBX_ERROR EMBX_close_transport(EMBX_Transport_t *tp, EMBX_ERROR *bInterrupted)
{
EMBX_ERROR res = EMBX_SUCCESS;
EMBX_EventState_t ev;

    /* Set return parameter to default */
    *bInterrupted = EMBX_SUCCESS;

    ev.result = EMBX_SYSTEM_INTERRUPT;
    if(!EMBX_OS_EVENT_INIT(&ev.event))
    {
        return EMBX_SYSTEM_ERROR;
    }

    /* Change the transport state to prevent any more handles
     * being issued.
     */
    tp->state = EMBX_TP_IN_CLOSEDOWN;

    /* If there are outstanding transport handles we must block
     * until they are closed or the user gets fed up and presses
     * Ctrl-C
     */
    if(tp->transportHandles != 0)
    {
    EMBX_ERROR bInterrupt;

        tp->closeEvent = &ev;

        EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
        bInterrupt = EMBX_OS_EVENT_WAIT(&ev.event);
        EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

        tp->closeEvent = 0;

        if(bInterrupt != EMBX_SUCCESS)
        {
            tp->state = EMBX_TP_FAILED;    
            return res;
        }
    }

    /* Note: we may be calling closedown on a transport
     * that never fully initialized or previously got
     * interrupted during closedown; it must be able to
     * cope with those situations.
     */
    res = tp->methods->closedown(tp, &ev);

    if(res == EMBX_BLOCK)
    {
    EMBX_ERROR bInterrupt;

        /* Release the driver lock and wait for the closedown
         * to complete, then retake the driver lock.
         */
        EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
        bInterrupt = EMBX_OS_EVENT_WAIT(&ev.event);
        EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

        if(bInterrupt)
        {
            tp->methods->close_interrupt(tp);

            /* We have no idea what state the transport is in now so
             * assume it is completely unusable.
             */
            tp->state = EMBX_TP_FAILED;

            /* Propogate interrupt up to caller. */
            *bInterrupted = bInterrupt;
        }

        res = ev.result;
    }


    EMBX_OS_EVENT_DESTROY(&ev.event);

    return res;
}



/*
 * Helper function for _close_transports and _close_factory_transports
 * which closes a transport, blocking if necessary, then releases the
 * transport structure.
 */
static EMBX_ERROR _destroy_transport(EMBX_Transport_t *tp, EMBX_ERROR *bInterrupted)
{
EMBX_ERROR res = EMBX_SUCCESS;


    if(tp->state != EMBX_TP_UNINITIALIZED)
    {
        res = EMBX_close_transport(tp, bInterrupted);
    }
    else
    {
        *bInterrupted = EMBX_SUCCESS;
    }

    if(res != EMBX_SYSTEM_INTERRUPT)
    {
        /* Even if we were really interrupted, the close succeeded
         * so we must clean up the memory.
         */
        EMBX_OS_MemFree(tp);

        if(res != EMBX_SUCCESS)
        {
            /* Convert to a single error code for the API */
            res = EMBX_SYSTEM_ERROR;
        }
    }

    return res;
}



/*
 * Helper function for EMBX_Deinit that closes
 * down all transports.
 */
static EMBX_ERROR _closedown_transports(void)
{
EMBX_ERROR res = EMBX_SUCCESS;
EMBX_TransportList_t *pTransportList;

    /* First invalidate all transports, transport handles and
     * port handles and wake up any blocked processes. This
     * should be enough to get the application's attention
     * so it can close everything down for us.
     */
    pTransportList = _embx_dvr_ctx.transports;
    while(pTransportList)
    {
        _invalidate_transport(pTransportList->transport);

        pTransportList = pTransportList->next;
    }

    /* Now do transport closedown, waiting for each transport
     * to close if necessary
     */
    while(_embx_dvr_ctx.transports)
    {
    EMBX_ERROR bInterrupt;
    EMBX_ERROR tmpres;

        tmpres = _destroy_transport(_embx_dvr_ctx.transports->transport, &bInterrupt);

        if(tmpres != EMBX_SYSTEM_INTERRUPT)
        {         
        EMBX_TransportList_t *next;

            next = _embx_dvr_ctx.transports->next;
            EMBX_OS_MemFree(_embx_dvr_ctx.transports);
            _embx_dvr_ctx.transports = next;

            if(tmpres != EMBX_SUCCESS)
                res = tmpres;
        }

        if(bInterrupt && (_embx_dvr_ctx.transports != 0) )
        {
            /* The close transport got interrupted by the OS
             * even though the close may have succeeded hence
             * res != EMBX_SYSTEM_INTERRUPT. But we haven't got
             * to the end of the transport list we must honor
             * the interrupt and stop the process.
             */
            return EMBX_SYSTEM_INTERRUPT;
        }
    }

    return res;
}



EMBX_ERROR EMBX_Deinit(void)
{
EMBX_ERROR res = EMBX_SUCCESS;

    EMBX_Info(EMBX_INFO_INIT, (">>>>EMBX_Deinit\n"));
    EMBX_DebugOn(EMBX_INFO_INIT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        res = EMBX_DRIVER_NOT_INITIALIZED;
        goto exit;
    }

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    switch (_embx_dvr_ctx.state)
    {
        case EMBX_DVR_IN_SHUTDOWN:
        {
            res = _deinit_wait();
            break;
        } /* EMBX_DVR_IN_SHUTDOWN */
        case EMBX_DVR_INITIALIZED:
        {
            _embx_dvr_ctx.state = EMBX_DVR_IN_SHUTDOWN;

            res = _closedown_transports();
            if(res == EMBX_SYSTEM_INTERRUPT)
            {
                /* We got an OS interrupt before we could complete
                 * the transport closedowns so put the driver state
                 * back to initialized as we haven't managed to close
                 * it all down yet.
                 */
                _embx_dvr_ctx.state = EMBX_DVR_INITIALIZED;

                /* Signal any other waiting tasks about the
                 * interrupt, this is the best we can do at
                 * this point, hopefully someone will try
                 * and call EMBX_Deinit again to complete
                 * the closedown. We cannot actually signal 
                 * EMBX_SYSTEM_INTERRUPT as that conflicts with a
                 * real signal being delivered to the blocked
                 * tasks, so use EMBX_SYSTEM_ERROR instead.
                 */
                EMBX_EventListSignal(&_embx_dvr_ctx.deinitWaitList, EMBX_SYSTEM_ERROR);
            }
            else
            {
                /* We cleaned up all the transports, although possibly with
                 * errors along the way. Finish off the closedown and propogate
                 * the result just in case the caller and waiters care.
                 */
                _embx_dvr_ctx.state = EMBX_DVR_UNINITIALIZED;
                EMBX_EventListSignal(&_embx_dvr_ctx.deinitWaitList, res);
            }

            _embx_dvr_ctx.deinitWaitList = 0;
            break;
        } /* EMBX_DVR_INITIALIZED */
        case EMBX_DVR_UNINITIALIZED:
        case EMBX_DVR_FAILED:
        {
            res = EMBX_DRIVER_NOT_INITIALIZED;
            break;
        } /* EMBX_DVR_UNINITIALIZED */
        default:
        {
            res = EMBX_SYSTEM_ERROR;
            EMBX_DebugMessage(("Driver in unknown state %d, system is corrupt\n",_embx_dvr_ctx.state));
            break;
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_INIT);
    EMBX_Info(EMBX_INFO_INIT, ("<<<<EMBX_Deinit\n"));

    return res;
}



/*
 * Helper function for EMBX_UnregisterTransport that closes
 * down transports that were created using the given factory.
 */
static EMBX_ERROR _closedown_factory_transports(EMBX_TransportFactory_t *pFactory)
{
EMBX_ERROR res = EMBX_SUCCESS;
EMBX_TransportList_t *pTransportList,*prev;

    pTransportList = _embx_dvr_ctx.transports;

    /* First invalidate all transports associated with this
     * factory. This wakes up any blocked tasks and should
     * put the application into a position to close all the
     * transports down.
     */
    while(pTransportList)
    {
    EMBX_Transport_t *tp = pTransportList->transport;

        if(tp->parent == pFactory)
        {
            _invalidate_transport(tp);
        }

        pTransportList = pTransportList->next;
    }


    /* Now go through again and close those transports */
    prev = 0;
    pTransportList = _embx_dvr_ctx.transports;

    while(pTransportList)
    {
    EMBX_Transport_t *tp = pTransportList->transport;

        if(tp->parent == pFactory)
        {
        EMBX_TransportList_t *next;
        EMBX_ERROR bInterrupt;
        EMBX_ERROR tmpres;

            next = pTransportList->next;
            tmpres  = _destroy_transport(tp, &bInterrupt);

            if(tmpres != EMBX_SYSTEM_INTERRUPT)
            {
                /* the transport's structure has now been freed so
                 * we must remove it from the transport list
                 */
                if(prev)
                    prev->next = next;
                else
                    _embx_dvr_ctx.transports = next;

                EMBX_OS_MemFree(pTransportList);

                if(tmpres != EMBX_SUCCESS)
                    res = tmpres;
            }

            if( bInterrupt && (next != 0) )
            {
                /* The last _destroy_transport got interrupted, even though it
                 * might actually have finished before we could respond to 
                 * the interrupt. If we haven't yet got to the end of the
                 * transport list then propogate the interrupt up to our
                 * caller
                 */
                return EMBX_SYSTEM_INTERRUPT;
            }

            pTransportList = next;
        }
        else
        {
            prev = pTransportList;
            pTransportList = pTransportList->next;
        }
    }

    return res;
}



/*
 * Unregister a transport previously registered using EMBX_RegisterTransport.
 * This is used by Linux kernel modules to clean up before unloading from
 * the kernel.
 */
EMBX_ERROR EMBX_UnregisterTransport(EMBX_FACTORY hFactory)
{
EMBX_ERROR res = EMBX_SUCCESS;
EMBX_TransportFactory_t *pFactory;

    EMBX_Info(EMBX_INFO_INIT, (">>>>EMBX_UnregisterTransport\n"));
    EMBX_DebugOn(EMBX_INFO_INIT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        res = EMBX_DRIVER_NOT_INITIALIZED;
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(hFactory, EMBX_HANDLE_TYPE_FACTORY))
    {
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    pFactory = (EMBX_TransportFactory_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, hFactory);
    if(pFactory != 0)
    {
        /* We have found a matching factory entry, first we need to
         * closedown transports created using this entry.
         */
        res = _closedown_factory_transports(pFactory);
        switch(res)
        {
            case EMBX_SUCCESS:
            {
                if(pFactory->prev != 0)
                    pFactory->prev->next = pFactory->next;
                else
                    _embx_dvr_ctx.transportFactories = pFactory->next;

                if(pFactory->next != 0)
                    pFactory->next->prev = pFactory->prev;

                EMBX_OS_MemFree(pFactory->argument);
                EMBX_OS_MemFree(pFactory);

                EMBX_HANDLE_FREE(&_embx_handle_manager, hFactory);

                EMBX_OS_MODULE_UNREF();
                break;
            } /* EMBX_SUCCESS */
            case EMBX_SYSTEM_INTERRUPT:
            {
                break;
            } /* EMBX_SYSTEM_INTERRUPT */
            default:
            {
                res = EMBX_SYSTEM_ERROR;
                break;
            }
        }
    }
    else
    {
        res = EMBX_INVALID_ARGUMENT;
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_INIT);
    EMBX_Info(EMBX_INFO_INIT, ("<<<<EMBX_UnregisterTransport\n"));

    return res;
}

