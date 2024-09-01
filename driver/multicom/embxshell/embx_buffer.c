/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_buffer.c                                             */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Buffer API entrypoints                             */
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


EMBX_ERROR EMBX_Alloc(EMBX_TRANSPORT htp, EMBX_UINT size, EMBX_VOID **buffer)
{
    EMBX_ERROR res = EMBX_INVALID_TRANSPORT;

    EMBX_Info(EMBX_INFO_BUFFER, (">>>>EMBX_Alloc(htp=%p, size=%u)\n",(void *)htp,(unsigned)size));
    EMBX_DebugOn(EMBX_INFO_BUFFER);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(buffer == 0)
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
        EMBX_Buffer_t    *buf;
        EMBX_Transport_t *tp = tph->transport;

            res = tp->methods->alloc_buffer(tp, size, &buf);
            if(res == EMBX_SUCCESS)
            {
                /*
                 * Fix up the local transport handle field in the buffer
                 */
                buf->htp   = htp;

                *buffer = (EMBX_VOID *)buf->data;
            }
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_BUFFER);
    EMBX_Info(EMBX_INFO_BUFFER, ("<<<<EMBX_Alloc(htp=%p, size=%u, buf=%p)=%d\n", (void *)htp, size, *buffer, res));

    return res;
}


/****************************************************************************/

static EMBX_ERROR _do_free(EMBX_TransportHandle_t *tph, EMBX_Buffer_t *buf)
{
    EMBX_Transport_t *tp = tph->transport;

    /* The fact the user handle was valid is a sufficient test here as we allow
     * memory to be released when the transport is invalidated but not yet closed.
     * If another CPU in a shared memory transport is erroneously freeing the buffer
     * at the same time, this should be caught by the transport's free_buffer implementation
     * once it has got hold of a private lock for the memory pool. In a correctly written
     * application this is illegal as only one application or a driver can have
     * current ownership of a buffer at a time. 
     */
    return tp->methods->free_buffer(tp, buf);
}



EMBX_ERROR EMBX_Free(EMBX_VOID *buffer)
{
    EMBX_ERROR res = EMBX_INVALID_ARGUMENT;
    EMBX_Buffer_t *buf;

    EMBX_Info(EMBX_INFO_BUFFER, (">>>>EMBX_Free(0x%08lx)\n",(unsigned long)buffer));
    EMBX_DebugOn(EMBX_INFO_BUFFER);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(buffer == 0)
    {
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */

    buf = EMBX_CAST_TO_BUFFER_T(buffer);

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED &&
       _embx_dvr_ctx.state != EMBX_DVR_IN_SHUTDOWN)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
        if(buf->state != EMBX_BUFFER_ALLOCATED)
        {
	    EMBX_DebugMessage(("Failed 'Already released, Buffer header corrupted or pointer not an EMBX buffer'\n"));
        }
        else
        {
	    EMBX_TransportHandle_t *tph;

#if !defined(EMBX_LEAN_AND_MEAN)
	    if(!EMBX_HANDLE_ISTYPEOF(buf->htp, EMBX_HANDLE_TYPE_TRANSPORT))
	    {
		EMBX_DebugMessage(("Failed 'Buffer header corrupted, contained wrong handle type'\n"));
		res = EMBX_INVALID_ARGUMENT;
		goto exit_release;
	    }
#endif /* EMBX_LEAN_AND_MEAN */

	    tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, buf->htp);

	    if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
	    {
		res = _do_free(tph, buf);
	    }
	    else
		res = EMBX_INVALID_ARGUMENT;
        }
    }

exit_release:
    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_BUFFER);
    EMBX_Info(EMBX_INFO_BUFFER, ("<<<<EMBX_Free(0x%08lx)=%d\n",(unsigned long)buffer, res));

    return res;
}


/* This is like EMBX_Free() except that it accepts a transport handle
 * It is required by the MME recovery code which may have to free off buffers
 * which have been sent to the remote before it crashed. The remote cpu may
 * have already received these buffer and changed their buf->htp thus preventing
 * them from being freed via the normal call
 */
EMBX_ERROR EMBX_Release(EMBX_TRANSPORT htp, EMBX_VOID *buffer)
{
    EMBX_ERROR res = EMBX_INVALID_ARGUMENT;
    EMBX_Buffer_t *buf;
    
    EMBX_Info(EMBX_INFO_BUFFER, (">>>>EMBX_Release(0x%x,0x%08lx)\n", htp, (unsigned long)buffer));
    EMBX_DebugOn(EMBX_INFO_BUFFER);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(buffer == 0)
    {
        goto exit;
    }

    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */

    buf = EMBX_CAST_TO_BUFFER_T(buffer);

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED &&
       _embx_dvr_ctx.state != EMBX_DVR_IN_SHUTDOWN)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
        if(buf->state != EMBX_BUFFER_ALLOCATED)
        {
	    EMBX_DebugMessage(("Failed 'Already released, Buffer header corrupted or pointer not an EMBX buffer'\n"));
        }
        else
        {
	    EMBX_TransportHandle_t *tph;

	    tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
	    if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
	    {
		res = _do_free(tph, buf);
	    }
	    else
		res = EMBX_INVALID_ARGUMENT;
	}
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_BUFFER);
    EMBX_Info(EMBX_INFO_BUFFER, ("<<<<EMBX_Release(0x%08lx)=%d\n",(unsigned long)buffer, res));

    return res;
}


/****************************************************************************/

static EMBX_ERROR _do_get_buffer_size(EMBX_Buffer_t *buf, EMBX_UINT *size)
{
#if !defined(EMBX_LEAN_AND_MEAN)

    if(!EMBX_HANDLE_ISTYPEOF(buf->htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
        EMBX_DebugMessage(("Failed 'Buffer header corrupted, contained wrong handle type'\n"));
        return EMBX_INVALID_ARGUMENT;
    }

    /* Recheck the buffer state in case it got changed underneath us, which
     * would be very bad news indeed.
     */
    if(buf->state != EMBX_BUFFER_ALLOCATED)
    {
	EMBX_DebugMessage(("Failed 'Buffer got released(?) as we read the size'\n"));
	return EMBX_INVALID_ARGUMENT;
    }

#endif /* EMBX_LEAN_AND_MEAN */

    *size = buf->size;

    return EMBX_SUCCESS;
}



EMBX_ERROR EMBX_GetBufferSize(EMBX_VOID *buffer, EMBX_UINT *size)
{
EMBX_ERROR res = EMBX_INVALID_ARGUMENT;
EMBX_Buffer_t *buf = NULL;

    EMBX_Info(EMBX_INFO_BUFFER, (">>>>EMBX_GetBufferSize(%p)\n",buffer));
    EMBX_DebugOn(EMBX_INFO_BUFFER);

#if !defined(EMBX_LEAN_AND_MEAN)

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }

    if(buffer == 0 || size == 0)
    {
        goto exit;
    }

#endif /* EMBX_LEAN_AND_MEAN */

    buf = EMBX_CAST_TO_BUFFER_T(buffer);

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
        /* This is subject to a race condition in a shared memory transport as
         * another CPU could decide (erroneously as it should not have
         * ownership) to delete the buffer at the same time we access
         * its header.  
         */ 
        if(buf->state != EMBX_BUFFER_ALLOCATED)
        {
            EMBX_DebugMessage(("Failed 'Already released, Buffer header corrupted or pointer not an EMBX buffer'\n"));
        }
        else
        {
            res = _do_get_buffer_size(buf,size);
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_BUFFER);
    EMBX_Info(EMBX_INFO_BUFFER, ("<<<<EMBX_GetBufferSize(%p, sz=%d state=%x)=%d\n", buffer, *size, 
				 (buf ? buf->state : 0), res));

    return res;
}

/*
 * Local Variables:
 *  tab-width: 0
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
