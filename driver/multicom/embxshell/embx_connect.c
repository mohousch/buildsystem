/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_connect.c                                            */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Connection API entrypoints                         */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"

#include "debug_ctrl.h"
#include "embxshell.h"

#ifdef EMBX_VERBOSE
static char *_driver_uninit_error = "Failed 'EMBX Driver is not initialized'\n";
#endif /* EMBX_VERBOSE */


static EMBX_ERROR connect_validate_args(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port)
{
    if(portName == 0 || port == 0)
    {
        return EMBX_INVALID_ARGUMENT;
    }

    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
        EMBX_DebugMessage(("Failed 'transport handle parameter is not a transport handle'\n"));
        return EMBX_INVALID_TRANSPORT;
    }

    if(strlen(portName) > EMBX_MAX_PORT_NAME)
    {
        EMBX_DebugMessage(("Failed 'port name too long'\n"));
        return EMBX_INVALID_ARGUMENT;
    }

    return EMBX_SUCCESS;
}



static EMBX_ERROR connect_allocate_resources(EMBX_RemotePortList_t **node, EMBX_PORT *port)
{
    /*
     * Try and allocate a port list node
     */
    *node = (EMBX_RemotePortList_t *)EMBX_OS_MemAlloc(sizeof(EMBX_RemotePortList_t));
    if(*node != 0)
    {
        /*
         * Try and get a new public HANDLE
         */
        *port = EMBX_HANDLE_CREATE(&_embx_handle_manager, 0, EMBX_HANDLE_TYPE_REMOTEPORT);
        if(*port != EMBX_INVALID_HANDLE_VALUE)
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



static void connect_destroy_resources(EMBX_RemotePortList_t *node, EMBX_PORT *port)
{
    EMBX_OS_MemFree(node);

    EMBX_HANDLE_FREE(&_embx_handle_manager, *port);
    *port = EMBX_INVALID_HANDLE_VALUE;
}



static void register_connection(EMBX_TransportHandle_t *tph, EMBX_RemotePortHandle_t *ph, EMBX_RemotePortList_t *node, EMBX_PORT port)
{
    /* Attach the handle structure to the public HANDLE */
    EMBX_HANDLE_ATTACHOBJ(&_embx_handle_manager, port, ph);

    /* Add to the transport handle's local port list */
    node->port = ph;
    node->prev = 0;
    node->next = tph->remotePorts;

    if(node->next != 0)
        node->next->prev = node;


    tph->remotePorts = node;

    /* This does not cause the port use count to increase,
     * that is for locally created ports only
     */

    /* Finally mark the handle state valid */
    ph->transportHandle = tph;
    ph->state = EMBX_HANDLE_VALID;

    EMBX_Info(EMBX_INFO_PORT, ("Connected to port, structure address = 0x%08lx HANDLE = 0x%08lx\n",(unsigned long)ph,(unsigned long)port));
}


/****************************************************************************/

static EMBX_ERROR do_connect(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;
EMBX_TransportHandle_t *tph;

    tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
    if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
    {
    EMBX_RemotePortHandle_t *ph;
    EMBX_RemotePortList_t   *node;

        res = connect_allocate_resources(&node, port);
        if(res == EMBX_SUCCESS)
        {
            res = tph->methods->connect(tph, portName, &ph);
            if(res != EMBX_SUCCESS)
            {
                connect_destroy_resources(node, port);
            }
            else
            {
                register_connection(tph, ph, node, *port);
            }
        }
    }

    return res;
}



EMBX_ERROR EMBX_Connect(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_Connect(htp=0x%08lx, %s)\n",(unsigned long)htp,(portName==0?"(NULL)":portName)));
    EMBX_DebugOn(EMBX_INFO_PORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    res = connect_validate_args(htp, portName, port);
    if(res != EMBX_SUCCESS)
    {
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_INVALID_TRANSPORT;
    }
    else
    {
        res = do_connect(htp, portName, port);
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_Connect(htp=0x%08lx, %s)=%d\n",(unsigned long)htp,(portName==0?"(NULL)":portName),res));

    return res;
}


/****************************************************************************/

static EMBX_ERROR connectblock_wait(EMBX_TransportHandle_t *tph, EMBX_EventState_t *pEventState, EMBX_UINT timeout)
{
EMBX_ERROR res;
EMBX_ERROR bInterrupt;

    EMBX_Info(EMBX_INFO_PORT, ("Waiting for connection\n"));

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
    /* TIMEOUT SUPPORT */
    bInterrupt = EMBX_OS_EVENT_WAIT_TIMEOUT(&pEventState->event, timeout);
    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(bInterrupt)
    {
        tph->methods->connect_interrupt(tph,pEventState);
    }

    /* Cannot read the result back until we have informed
     * the transport of any OS interrupt, the transport's ISR/T
     * might actually have made the connection before we've have
     * been able to get the driver lock back and calling
     * connect_interrupt. We need to know if that has happened.
     */
    res = pEventState->result;

    if((res == EMBX_SYSTEM_INTERRUPT) && (bInterrupt == EMBX_SYSTEM_TIMEOUT))
    {
	/* TIMEOUT SUPPORT: Distinguish between an interrupted blocking wait
	 * and a timeout
	 */
	res = EMBX_SYSTEM_TIMEOUT;
    }

#if defined(EMBX_VERBOSE)

    if(res == EMBX_SYSTEM_INTERRUPT)
    {
	/* Got interrupted and the connection hadn't succeeded */
	EMBX_Info(EMBX_INFO_PORT, ("Failed 'interrupted by OS'\n"));
    }
    else if(res == EMBX_SYSTEM_TIMEOUT)
    {
	EMBX_Info(EMBX_INFO_PORT, ("Failed 'receive timed out by OS'\n"));
    }
    else if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_INFO_PORT, ("Failed 'interrupted by driver'\n"));
    }

#endif /* EMBX_VERBOSE */

    return res;
}



static EMBX_ERROR do_connect_block(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port, EMBX_UINT timeout)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;
EMBX_TransportHandle_t *tph;

    tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
    if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
    {
    EMBX_RemotePortHandle_t *ph;
    EMBX_RemotePortList_t   *node;
    EMBX_EventState_t        eventState;

        res = connect_allocate_resources(&node, port);
        if(res == EMBX_SUCCESS)
        {
            if(EMBX_OS_EVENT_INIT(&eventState.event))
            {
                /* Set the default result when blocking to catch OS interrupts.
                 * This must be done before we pass the structure to the transport
                 * connect_block function; after that we can not safely access the
                 * structure until either the event has been signalled or we have
                 * called connect_interrupt.
                 */
                eventState.result = EMBX_SYSTEM_INTERRUPT;

                res = tph->methods->connect_block(tph, portName, &eventState, &ph);
                if(res == EMBX_BLOCK)
                {
                    res = connectblock_wait(tph,&eventState, timeout);
                }

                if(res != EMBX_SUCCESS)
                {
                    /* Whatever has happened it isn't a successful connection
                     * so clean up allocated resources.
                     */
                    connect_destroy_resources(node, port);
                }
                else
                {
                    register_connection(tph, ph, node, *port);
                }

                EMBX_OS_EVENT_DESTROY(&eventState.event);
            }
            else
            {
                /* Could not initialise the wait event */
                res = EMBX_NOMEM;
            }
        }
    }

    return res;
}


/*
 * TIMEOUT SUPPORT
 * New API function that is exactly like EMBX_ConnectBlock() except that it will abort after
 * waiting for 'timeout' ms and that case return EMBX_SYSTEM_TIMEOUT
 */
EMBX_ERROR EMBX_ConnectBlockTimeout(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port, EMBX_UINT timeout)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_ConnectBlockTimeout(htp=0x%08lx, %s, %u)\n",
			       (unsigned long)htp,(portName==0?"(NULL)":portName),timeout));
    EMBX_DebugOn(EMBX_INFO_PORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    res = connect_validate_args(htp, portName, port);
    if(res != EMBX_SUCCESS)
    {
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_INVALID_TRANSPORT;
    }
    else
    {
	res = do_connect_block(htp, portName, port, timeout);
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_ConnectBlockTimeout(htp=0x%08lx, %s, %u) = %d\n",
			       (unsigned long)htp,(portName==0?"(NULL)":portName),timeout, res));

    return res;
}

EMBX_ERROR EMBX_ConnectBlock(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port)
{
    /* Simple wrapper which uses an infinite timeout */
    return EMBX_ConnectBlockTimeout(htp, portName, port, EMBX_TIMEOUT_INFINITE);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
