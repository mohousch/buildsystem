/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_transport.c                                          */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Transport API entrypoints                          */
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


EMBX_ERROR EMBX_FindTransport(const EMBX_CHAR *name, EMBX_TPINFO *tpinfo)
{
EMBX_ERROR res;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_FindTransport(%s)\n",(name==0?"(NULL)":name)));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_DRIVER_NOT_INITIALIZED;
        goto exit;
    }

    if((name == 0) || (tpinfo == 0))
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
    EMBX_TransportList_t *tpl;

        tpl = EMBX_find_transport_entry(name);
        if(tpl != 0)
        {
            EMBX_Info(EMBX_INFO_TRANSPORT,("Found transport\n"));

            *tpinfo = tpl->transport->transportInfo;
            res = EMBX_SUCCESS;
        }
        else
        {
            EMBX_Info(EMBX_INFO_TRANSPORT,("Could not find transport\n"));
            res = EMBX_INVALID_TRANSPORT;
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_FindTransport(%s) = %d\n",(name==0?"(NULL)":name), res));

    return res;
}



EMBX_ERROR EMBX_GetFirstTransport(EMBX_TPINFO *tpinfo)
{
EMBX_ERROR res;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_GetFirstTransport\n"));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_DRIVER_NOT_INITIALIZED;
        goto exit;
    }

    if(tpinfo == 0)
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
        if(_embx_dvr_ctx.transports == 0)
        {
            res = EMBX_INVALID_STATUS;
        }
        else
        {
            *tpinfo = _embx_dvr_ctx.transports->transport->transportInfo;
            EMBX_Info(EMBX_INFO_TRANSPORT, ("Returning transport '%s'\n",tpinfo->name));

            res = EMBX_SUCCESS;
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_GetFirstTransport = %d\n", res));

    return res;
}



EMBX_ERROR EMBX_GetNextTransport(EMBX_TPINFO *tpinfo)
{
EMBX_ERROR res;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_GetNextTransport\n"));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_DRIVER_NOT_INITIALIZED;
        goto exit;
    }

    if(tpinfo == 0)
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
    EMBX_TransportList_t *tpl;

        tpl = EMBX_find_transport_entry(tpinfo->name);
        if(tpl != 0)
        {
            if(tpl->next != 0)
            {
                *tpinfo = tpl->next->transport->transportInfo;
                EMBX_Info(EMBX_INFO_TRANSPORT, ("Returning transport '%s'\n",tpinfo->name));

                res = EMBX_SUCCESS;
            }
            else
            {
                res = EMBX_INVALID_STATUS;
            }
        }
        else
        {
            res = EMBX_INVALID_ARGUMENT;
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_GetNextTransport = %d\n", res));

    return res;
}



EMBX_ERROR EMBX_GetTransportInfo(EMBX_TRANSPORT htp, EMBX_TPINFO *tpinfo)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_GetTransportInfo(htp=0x%08lx)\n",(unsigned long)htp));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(tpinfo == 0)
    {
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
    EMBX_TransportHandle_t *tph;

        tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
        if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
        {
            *tpinfo = tph->transport->transportInfo;
            EMBX_Info(EMBX_INFO_TRANSPORT, ("Returning transport '%s'\n",tpinfo->name));

            res = EMBX_SUCCESS;
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_GetTransportInfo(htp=0x%08lx) = %d\n",(unsigned long)htp, res));

    return res;
}



/****************************************************************************/

static EMBX_ERROR do_close_transport(EMBX_TRANSPORT htp)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;
EMBX_TransportHandle_t *tph;

    tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
    if(tph != 0)
    {
    EMBX_Transport_t           *tp;
    EMBX_TransportHandleList_t *handleList;

        if( (tph->state != EMBX_HANDLE_VALID) && (tph->state != EMBX_HANDLE_INVALID) )
        {
            EMBX_DebugMessage(("Failed 'transport handle state is corrupted'\n"));
            return EMBX_INVALID_TRANSPORT;
        }

        if( (tph->localPorts != 0) || (tph->remotePorts != 0) )
        {
            EMBX_DebugMessage(("Failed 'transport still has open ports'\n"));
            return EMBX_PORTS_STILL_OPEN;
        }

        tp = tph->transport;
        handleList = tp->transportHandles;

        while(handleList)
        {
            if(handleList->transportHandle == tph)
                break;

            handleList = handleList->next;
        }

#if defined(EMBX_VERBOSE)
        if(handleList == 0)
        {
            EMBX_DebugMessage(("Unable to find transport handle (0x%08lx) on transport (0x%08lx) handle list!\n",(unsigned long)tph,(unsigned long)tp));
        }
#endif /* EMBX_VERBOSE */

        res = tp->methods->close_handle(tp, tph);
        if(res == EMBX_SUCCESS)
        {
            /* At this point tph has been freed by the transport
             * so we must not use it again.
             */
            EMBX_HANDLE_FREE(&_embx_handle_manager, htp);
            tp->transportInfo.nrOpenHandles--;

            /* Unhook the transport handle list entry */
            if(handleList != 0)
            {
                if(handleList->prev != 0)
                    handleList->prev->next = handleList->next;
                else
                    tp->transportHandles = handleList->next;

                if(handleList->next != 0)
                    handleList->next->prev = handleList->prev;

                EMBX_OS_MemFree(handleList);
            }


            if(tp->closeEvent != 0 && tp->transportHandles == 0)
            {
                /* Another thread is waiting for the last handle to
                 * close on this transport. Signal it now this has
                 * happened.
                 */
                EMBX_OS_EVENT_POST(&tp->closeEvent->event);
            }
        }
    }

    return res;
}



EMBX_ERROR EMBX_CloseTransport(EMBX_TRANSPORT htp)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_CloseTransport(htp=0x%08lx)\n",(unsigned long)htp));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED &&
       _embx_dvr_ctx.state != EMBX_DVR_IN_SHUTDOWN )
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
        res = do_close_transport(htp);
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_CloseTransport(htp=0x%08lx) = %d\n",(unsigned long)htp, res));

    return res;
}

EMBX_ERROR EMBX_RemoveProcessor(EMBX_TRANSPORT htp, EMBX_UINT cpuID)
{
    EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_RemoveProcessor(htp=0x%08lx,cpu=%d)\n",(unsigned long)htp, cpuID));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }
    
    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
	res = EMBX_INVALID_TRANSPORT;
        goto exit;
    }

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED &&
       _embx_dvr_ctx.state != EMBX_DVR_IN_SHUTDOWN )
    {
        EMBX_DebugMessage((_driver_uninit_error));
	res = EMBX_INVALID_TRANSPORT;
    }
    else
    {
	EMBX_TransportHandle_t *tph;

	tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
	if( (tph == 0) || (tph->state != EMBX_HANDLE_VALID) ) 
	{
	    EMBX_DebugMessage(("Failed 'transport handle is not valid'\n"));
	    res = EMBX_INVALID_TRANSPORT;
	}
	else
	{
	    EMBX_Transport_t *tp = tph->transport;

	    EMBX_Assert(tp);
	    EMBX_Assert(tp->methods->remove_processor);
	    
	    /* Call the transport specific removal function */
	    res = tp->methods->remove_processor(tp, cpuID);
	}
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_RemoveProcessor(htp=0x%08lx,cpu=%d) = %d\n",
				    (unsigned long)htp, cpuID, res));

    return res;
}

/****************************************************************************/

EMBX_ERROR EMBX_Offset(EMBX_TRANSPORT htp, EMBX_VOID *address, EMBX_INT *offset)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_Offset(address = %p)\n", address));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }
 
    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
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
    EMBX_TransportHandle_t *tph;

        tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
        if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
        {
	    EMBX_Transport_t *tp = tph->transport;

	    if (tp->state == EMBX_TP_INITIALIZED)
	    {
		if (tp->transportInfo.usesZeroCopy)
		{
		    /* MULTICOM_32BIT_SUPPORT: The Warp range is now a physical address range */
#ifdef __TDT__
/*stm 24 prob with virt_to_phys */
		    res = tp->methods->virt_to_phys_alt(tp, address, (EMBX_UINT *)offset);
#else
		    res = tp->methods->virt_to_phys(tp, address, (EMBX_UINT *)offset);
#endif
		}
		else if (tp->transportInfo.allowsPointerTranslation)
		{
		    if(tp->transportInfo.memStart <= address &&
		       tp->transportInfo.memEnd   >= address)
		    {
			*offset = (EMBX_INT) ( (unsigned long)address - 
					       (unsigned long)tp->transportInfo.memStart );
			res = (0 == tp->methods->test_state ? 
			       EMBX_SUCCESS : 
			       tp->methods->test_state(tp, address)); 
			
			EMBX_Info(EMBX_INFO_TRANSPORT, ("    Warp: address %p warp range %p-%p : 0x%08x\n",
							address,
							tp->transportInfo.memStart, tp->transportInfo.memEnd,
							*offset));
		    }
		    else
		    {
			res = EMBX_INVALID_ARGUMENT;
		    }
		}
	    }
	}
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_Offset(address=%p) = %d\n", address, res));

    return res;
}



EMBX_ERROR EMBX_Address(EMBX_TRANSPORT htp, EMBX_INT offset, EMBX_VOID **address)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_TRANSPORT, (">>>>EMBX_Address(offset=0x%08x)\n", offset));
    EMBX_DebugOn(EMBX_INFO_TRANSPORT);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
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
    EMBX_TransportHandle_t *tph;

        tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
        if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
        {
	    EMBX_Transport_t *tp = tph->transport;

            if( tp->state == EMBX_TP_INITIALIZED)
	    {
		if (tp->transportInfo.usesZeroCopy)
		{
		    /* MULTICOM_32BIT_SUPPORT: The Warp range is now a physical address range */
#ifdef __TDT__
/* stm24 prob */
		    res = tp->methods->phys_to_virt_alt(tp, offset, address);
#else
		    res = tp->methods->phys_to_virt(tp, offset, address);
#endif
		}
		else if (tp->transportInfo.allowsPointerTranslation)
		{
		    EMBX_VOID *addr = (EMBX_VOID *)((unsigned long)(tp->transportInfo.memStart)+offset);
		    
		    if(tp->transportInfo.memStart <= addr &&
		       tp->transportInfo.memEnd   >= addr)
		    {
			*address = addr;
			res = (0 == tp->methods->test_state ?
			       EMBX_SUCCESS :
			       tp->methods->test_state(tp, addr));
		    }
		    else
		    {
			res = EMBX_INVALID_ARGUMENT;
		    }
		}
	    }
	}
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_TRANSPORT);
    EMBX_Info(EMBX_INFO_TRANSPORT, ("<<<<EMBX_Address(offset=0x%08x) = %d\n", offset, res));

    return res;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
