/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_object.c                                             */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Distributed object API entrypoints                 */
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


EMBX_ERROR EMBX_RegisterObject(EMBX_TRANSPORT htp, EMBX_VOID *object, EMBX_UINT size, EMBX_HANDLE *handle)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_BUFFER, (">>>>EMBX_RegisterObject(htp=0x%08lx, object=0x%08lx, size=%u)\n",(unsigned long)htp,(unsigned long)object,size));
    EMBX_DebugOn(EMBX_INFO_BUFFER);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(handle == 0)
    {
        res = EMBX_INVALID_ARGUMENT;
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

            res = tp->methods->register_object(tp, object, size, handle);
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_BUFFER);
    EMBX_Info(EMBX_INFO_BUFFER, ("<<<<EMBX_RegisterObject(htp=0x%08lx, object=0x%08lx, size=%u)\n",(unsigned long)htp,(unsigned long)object,size));

    return res;
}


/****************************************************************************/

EMBX_ERROR EMBX_DeregisterObject(EMBX_TRANSPORT htp, EMBX_HANDLE handle)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_BUFFER, (">>>>EMBX_DeregisterObject(htp=0x%08lx, handle=0x%08lx)\n",(unsigned long)htp,(unsigned long)handle));
    EMBX_DebugOn(EMBX_INFO_BUFFER);

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

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED &&
       _embx_dvr_ctx.state != EMBX_DVR_IN_SHUTDOWN)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
    EMBX_TransportHandle_t *tph;

        tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
        /* The fact the user handle was valid is a sufficient test here as we allow
         * objects to be deregistered when the transport is invalidated but not yet closed.
         */
        if(tph != 0)
        {
        EMBX_Transport_t *tp = tph->transport;

            res = tp->methods->deregister_object(tp, handle);
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_BUFFER);
    EMBX_Info(EMBX_INFO_BUFFER, ("<<<<EMBX_DeregisterObject(htp=0x%08lx, handle=0x%08lx)\n",(unsigned long)htp,(unsigned long)handle));

    return res;
}


/****************************************************************************/

EMBX_ERROR EMBX_GetObject(EMBX_TRANSPORT htp, EMBX_HANDLE handle, EMBX_VOID **object, EMBX_UINT *size)
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_BUFFER, (">>>>EMBX_GetObject(htp=0x%08lx, handle=0x%08lx)\n",(unsigned long)htp,(unsigned long)handle));
    EMBX_DebugOn(EMBX_INFO_BUFFER);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(object == 0 || size == 0)
    {
        res = EMBX_INVALID_ARGUMENT;
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

            res = tp->methods->get_object(tp, handle, object, size);
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_BUFFER);
    EMBX_Info(EMBX_INFO_BUFFER, ("<<<<EMBX_GetObject(htp=0x%08lx, handle=0x%08lx)\n",(unsigned long)htp,(unsigned long)handle));

    return res;
}

