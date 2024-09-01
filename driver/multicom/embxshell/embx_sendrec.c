/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_sendrec.c                                            */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Send and Receive API entrypoints                   */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"

#include "debug_ctrl.h"
#include "embxshell.h"


#if defined(EMBX_VERBOSE)
static char *_driver_uninit_error = "Failed 'EMBX Driver is not initialized'\n";
#endif /* EMBX_VERBOSE */


EMBX_ERROR EMBX_Receive(EMBX_PORT port, EMBX_RECEIVE_EVENT *recev)
{
EMBX_ERROR res = EMBX_INVALID_PORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_Receive(port=0x%08lx)\n",(unsigned long)port));
    EMBX_DebugOn(EMBX_INFO_PORT);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(recev == 0)
    {
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_LOCALPORT))
    {
        EMBX_DebugMessage(("Failed 'handle is of the wrong type'\n"));
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
    EMBX_LocalPortHandle_t *ph;

        ph = (EMBX_LocalPortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
        if( (ph != 0) && (ph->state == EMBX_HANDLE_VALID) )
        {
            res = ph->methods->receive(ph, recev);
            if( (res == EMBX_SUCCESS) && (recev->type == EMBX_REC_MESSAGE) )
            {
            EMBX_Buffer_t *buf;

                /*
                 * We need to fix up the transport handle in the buffer
                 * with the one the port belongs to, so that we can 
                 * safely send the message buffer somewhere else.
                 */
                buf = EMBX_CAST_TO_BUFFER_T(recev->data);
                buf->htp = ph->transportHandle->publicHandle;
            }
        }
        else
        {
            EMBX_DebugMessage(("Failed 'port handle is invalid'\n"));
        }
    }


    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_Receive(port=0x%08lx)\n",(unsigned long)port));

    return res;
}


/****************************************************************************/

static EMBX_ERROR receiveblock_wait(EMBX_LocalPortHandle_t *ph,EMBX_EventState_t *pEventState, EMBX_UINT timeout)
{
EMBX_ERROR res;
EMBX_ERROR bInterrupt;

    EMBX_Info(EMBX_INFO_PORT, ("Receive is blocking\n"));

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
    /* TIMEOUT SUPPORT */
    bInterrupt = EMBX_OS_EVENT_WAIT_TIMEOUT(&pEventState->event, timeout);
    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(bInterrupt)
    {
        ph->methods->receive_interrupt(ph,pEventState);
    }
     
    /*
     * Cannot read the result back until we have informed
     * the transport of any OS interrupt, the transport's ISR/T
     * might actually have delivered an event before we've have
     * been able to get the driver lock back and call
     * receive_interrupt. We need to know if that has happened.
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
	EMBX_Info(EMBX_INFO_PORT, ("Failed 'receive interrupted by OS'\n"));
    }
    else if(res == EMBX_SYSTEM_TIMEOUT)
    {
	EMBX_Info(EMBX_INFO_PORT, ("Failed 'receive timed out by OS'\n"));
    }
    else if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_INFO_PORT, ("Failed 'receive interrupted by driver'\n"));
    }
    else
    {
        EMBX_Info(EMBX_INFO_PORT, ("Receive has completed\n"));
    }

#endif /* EMBX_VERBOSE */

    return res;
}


/*
 * TIMEOUT SUPPORT
 * New API function that is exactly like EMBX_ReceiveBlock() except that it will abort after
 * waiting for 'timeout' ms and that case return EMBX_SYSTEM_TIMEOUT
 */
EMBX_ERROR EMBX_ReceiveBlockTimeout(EMBX_PORT port, EMBX_RECEIVE_EVENT *recev, EMBX_UINT timeout)
{
EMBX_ERROR res = EMBX_INVALID_PORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_ReceiveBlock(port=0x%08lx)\n",(unsigned long)port));
    EMBX_DebugOn(EMBX_INFO_PORT);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(recev == 0)
    {
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_LOCALPORT))
    {
        EMBX_DebugMessage(("Failed 'handle is of the wrong type'\n"));
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
    EMBX_LocalPortHandle_t *ph;

        ph = (EMBX_LocalPortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
        if( (ph != 0) && (ph->state == EMBX_HANDLE_VALID) )
        {
        EMBX_EventState_t event;

            /*
             * Set the default result when blocking to catch OS interrupts
             */
            event.result = EMBX_SYSTEM_INTERRUPT;

            /*
             * Note creating the event here may cause a slight
             * delay on OS's that take time to initialize the
             * synchronization primitive used to implement the event;
             * if there is a message already waiting to be received,
             * which is the critical path, this creation will have been wasted.
             */
            if(EMBX_OS_EVENT_INIT(&event.event))
            {
                res = ph->methods->receive_block(ph, &event, recev);
                if(res == EMBX_BLOCK)
                {
		    res = receiveblock_wait(ph, &event, timeout);
                }

                if( (res == EMBX_SUCCESS) && (recev->type == EMBX_REC_MESSAGE) )
                {
                EMBX_Buffer_t *buf;

                    /*
                     * We need to fix up the transport handle in the buffer
                     * with the one the port belongs to, so that we can 
                     * safely send the message buffer somewhere else.
                     */
                    buf = EMBX_CAST_TO_BUFFER_T(recev->data);
                    buf->htp = ph->transportHandle->publicHandle;
                }

                EMBX_OS_EVENT_DESTROY(&event.event);
            }
        }
        else
        {
            EMBX_DebugMessage(("Failed 'port handle 0x%x is invalid'\n", port));
        }
    }


    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_ReceiveBlock(port=0x%08lx) = %d\n",(unsigned long)port, res));

    return res;
}

EMBX_ERROR EMBX_ReceiveBlock(EMBX_PORT port, EMBX_RECEIVE_EVENT *recev)
{
  /* Simple wrapper which uses an infinite timeout */
  return EMBX_ReceiveBlockTimeout(port, recev, EMBX_TIMEOUT_INFINITE);
}

/****************************************************************************/


EMBX_ERROR EMBX_SendMessage(EMBX_PORT port, EMBX_VOID *buffer, EMBX_UINT size)
{
EMBX_ERROR     res;
EMBX_Buffer_t *buf;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_SendMessage(port=0x%08lx, buffer=0x%08lx, size=%u)\n",(unsigned long)port,(unsigned long)buffer,size));
    EMBX_DebugOn(EMBX_INFO_PORT);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_INVALID_PORT;
        goto exit;
    }

    if(buffer == 0)
    {
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_REMOTEPORT))
    {
        EMBX_DebugMessage(("Failed 'handle is of the wrong type'\n"));
        res = EMBX_INVALID_PORT;
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */


    buf = EMBX_CAST_TO_BUFFER_T(buffer);


#if !defined(EMBX_LEAN_AND_MEAN)

    /*
     * Accessing the buffer state is not locked against multi-cpu access,
     * but as only a single process, or a single driver has ownership of
     * a buffer at any one time this is ok for a correctly written system.
     */
    if(buf->state != EMBX_BUFFER_ALLOCATED)
    {
        EMBX_DebugMessage(("Failed 'Buffer not allocated or header corrupted'\n"));
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(buf->htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
        EMBX_DebugMessage(("Failed 'Buffer header corrupted, contained wrong handle type'\n"));
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */

    if(buf->size < size)
    {
        EMBX_DebugMessage(("Failed 'valid data size larger than buffer'\n"));
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(_embx_dvr_ctx.state == EMBX_DVR_INITIALIZED)
    {
    EMBX_RemotePortHandle_t *ph;

        ph = (EMBX_RemotePortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
        if( (ph != 0) && (ph->state == EMBX_HANDLE_VALID) )
        {
        EMBX_TransportHandle_t *tph;

            tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, buf->htp);
            if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
            {
                if( tph->transport == ph->transportHandle->transport )
                {
                    res = ph->methods->send_message(ph, buf, size);
                }
                else
                {
                    EMBX_DebugMessage(("Failed 'Buffer's transport does not match the port's transport\n"));
                    EMBX_DebugMessage(("        buffer tp name = %s  port tp name = %s'\n",tph->transport->transportInfo.name,ph->transportHandle->transport->transportInfo.name));
                    res = EMBX_INVALID_ARGUMENT;
                }
            }
            else
            {
                EMBX_DebugMessage(("Failed 'Buffer's transport handle is invalid'\n"));
                res = EMBX_INVALID_ARGUMENT;
            }
        }
        else
        {
            EMBX_DebugMessage(("Failed 'port handle is invalid'\n"));
            res = EMBX_INVALID_PORT;
        }
    }
    else
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_INVALID_PORT;
    }


    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_SendMessage(port=0x%08lx, buffer=0x%08lx, size=%u) = %d\n",(unsigned long)port,(unsigned long)buffer,size,res));

    return res;
}


/****************************************************************************/

EMBX_ERROR EMBX_SendObject(EMBX_PORT port, EMBX_HANDLE handle, EMBX_UINT offset, EMBX_UINT size)
{
EMBX_ERROR res = EMBX_INVALID_PORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_SendObject(port=0x%08lx, handle=0x%08lx, offset=%u, size=%u)\n",(unsigned long)port,(unsigned long)handle,offset,size));
    EMBX_DebugOn(EMBX_INFO_PORT);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_REMOTEPORT))
    {
        EMBX_DebugMessage(("Failed 'port handle is of the wrong type'\n"));
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
    EMBX_RemotePortHandle_t *ph;

        ph = (EMBX_RemotePortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
        if( (ph != 0) && (ph->state == EMBX_HANDLE_VALID) )
        {
            res = ph->methods->send_object(ph, handle, offset, size);
        }
        else
        {
            EMBX_DebugMessage(("Failed 'port handle is invalid'\n"));
        }
    }


    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_SendObject(port=0x%08lx, handle=0x%08lx, offset=%u, size=%u)\n",(unsigned long)port,(unsigned long)handle,offset,size));

    return res;
}


/****************************************************************************/

EMBX_ERROR EMBX_UpdateObject(EMBX_PORT port, EMBX_HANDLE handle, EMBX_UINT offset, EMBX_UINT size)
{
EMBX_ERROR res = EMBX_INVALID_PORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_UpdateObject(port=0x%08lx, handle=0x%08lx, offset=%u, size=%u)\n",(unsigned long)port,(unsigned long)handle,offset,size));
    EMBX_DebugOn(EMBX_INFO_PORT);

    /* very fast exit path for objects with zero copy type */
    if (EMBX_HANDLE_ISTYPEOF(handle, EMBX_HANDLE_TYPE_ZEROCOPY_OBJECT)) {
	EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_UpdateObject() [short circuit]\n"));
	return EMBX_SUCCESS;
    }

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_REMOTEPORT))
    {
        EMBX_DebugMessage(("Failed 'port handle is of the wrong type'\n"));
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
    EMBX_RemotePortHandle_t *ph;

        ph = (EMBX_RemotePortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
        if( (ph != 0) && (ph->state == EMBX_HANDLE_VALID) )
        {
            res = ph->methods->update_object(ph, handle, offset, size);
        }
        else
        {
            EMBX_DebugMessage(("Failed 'port handle is invalid'\n"));
        }
    }


    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_UpdateObject(port=0x%08lx, handle=0x%08lx, offset=%u, size=%u)\n",(unsigned long)port,(unsigned long)handle,offset,size));

    return res;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
