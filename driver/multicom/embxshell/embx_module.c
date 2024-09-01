/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_module.c                                             */
/*                                                                 */
/* Description:                                                    */
/*         Linux kernel module functionality                       */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"
#include "embx_debug.h"

#include "embxshell.h"

#if defined(__LINUX__) && defined(__KERNEL__) && defined(MODULE)

/* Public API exports */
EXPORT_SYMBOL( EMBX_Init );
EXPORT_SYMBOL( EMBX_Deinit );
EXPORT_SYMBOL( EMBX_ModifyTuneable );

EXPORT_SYMBOL( EMBX_FindTransport );
EXPORT_SYMBOL( EMBX_GetFirstTransport );
EXPORT_SYMBOL( EMBX_GetNextTransport );
EXPORT_SYMBOL( EMBX_OpenTransport );
EXPORT_SYMBOL( EMBX_CloseTransport );
EXPORT_SYMBOL( EMBX_GetTransportInfo );

EXPORT_SYMBOL( EMBX_RemoveProcessor );

EXPORT_SYMBOL( EMBX_Alloc );
EXPORT_SYMBOL( EMBX_Free );
EXPORT_SYMBOL( EMBX_Release );
EXPORT_SYMBOL( EMBX_GetBufferSize );

EXPORT_SYMBOL( EMBX_RegisterObject );
EXPORT_SYMBOL( EMBX_DeregisterObject );
EXPORT_SYMBOL( EMBX_GetObject );

EXPORT_SYMBOL( EMBX_CreatePort );
EXPORT_SYMBOL( EMBX_Connect );
EXPORT_SYMBOL( EMBX_ConnectBlock );
EXPORT_SYMBOL( EMBX_ConnectBlockTimeout );
EXPORT_SYMBOL( EMBX_ClosePort );
EXPORT_SYMBOL( EMBX_InvalidatePort );

EXPORT_SYMBOL( EMBX_Receive );
EXPORT_SYMBOL( EMBX_ReceiveBlock );
EXPORT_SYMBOL( EMBX_ReceiveBlockTimeout );
EXPORT_SYMBOL( EMBX_SendMessage );
EXPORT_SYMBOL( EMBX_SendObject );
EXPORT_SYMBOL( EMBX_UpdateObject );

EXPORT_SYMBOL( EMBX_Offset );
EXPORT_SYMBOL( EMBX_Address );

/* Transport module helper exports */
EXPORT_SYMBOL( EMBX_RegisterTransport );
EXPORT_SYMBOL( EMBX_UnregisterTransport );

EXPORT_SYMBOL( EMBX_GetTuneable );

EXPORT_SYMBOL( EMBX_EventListAdd );
EXPORT_SYMBOL( EMBX_EventListRemove );
EXPORT_SYMBOL( EMBX_EventListFront );
EXPORT_SYMBOL( EMBX_EventListSignal );

EXPORT_SYMBOL( EMBX_OS_EventCreate );
EXPORT_SYMBOL( EMBX_OS_EventWaitTimeout );
EXPORT_SYMBOL( EMBX_OS_EventSignal );

EXPORT_SYMBOL( EMBX_OS_ThreadCreate );
EXPORT_SYMBOL( EMBX_OS_ThreadDelete );

EXPORT_SYMBOL( EMBX_OS_ContigMemAlloc );
EXPORT_SYMBOL( EMBX_OS_ContigMemFree );
EXPORT_SYMBOL( EMBX_OS_MemAlloc );
EXPORT_SYMBOL( EMBX_OS_MemFree );

EXPORT_SYMBOL( EMBX_OS_PhysMemMap );
EXPORT_SYMBOL( EMBX_OS_PhysMemUnMap );
EXPORT_SYMBOL( EMBX_OS_VirtToPhys );

EXPORT_SYMBOL( EMBX_HANDLE_CREATE );
EXPORT_SYMBOL( EMBX_HandleManager_SetSize );
EXPORT_SYMBOL( EMBX_HandleManager_Init );
EXPORT_SYMBOL( EMBX_HandleManager_Destroy );

EXPORT_SYMBOL( EMBX_debug_enabled );
EXPORT_SYMBOL( EMBX_debug_enable );

EXPORT_SYMBOL( _EMBX_DriverMutexTake );
EXPORT_SYMBOL( _EMBX_DriverMutexRelease );
EXPORT_SYMBOL( _embx_handle_manager );

MODULE_DESCRIPTION("Extended EMBX Shell");
MODULE_AUTHOR("STMicroelectronics Ltd");
#ifdef MULTICOM_GPL
MODULE_LICENSE("GPL");
#else
MODULE_LICENSE("Copyright 2002 STMicroelectronics, All rights reserved");
#endif /* MULTICOM_GPL */

static int embx_read_procmem(char *buf, char **start, off_t offset,
                             int count, int *eof, void *data)
{
char tmp[935];
EMBX_TPINFO tpinfo;
EMBX_ERROR res;

    int i, len = 0;

    i = sprintf(tmp,"EMBX Driver\n-----------\n");

    if(len+i >= count)
        goto exit;

    len += sprintf(buf,tmp);

    res = EMBX_GetFirstTransport(&tpinfo);
    switch (res)
    {
        case EMBX_DRIVER_NOT_INITIALIZED:
        {
            i = sprintf(tmp,"Driver Not Initialized\n");
            if(len+i >= count)
                goto exit;

            len += sprintf(buf+len,tmp);
            goto exit;
        } /* EMBX_DRIVER_NOT_INITIALIZED */
        case EMBX_INVALID_STATUS:
        {
            i = sprintf(tmp,"No registered transports\n");
            if(len+i >= count)
                goto exit;

            len += sprintf(buf+len,tmp);
            goto exit;
        } /* EMBX_INVALID_STATUS */
        case EMBX_SUCCESS:
        {
            break;  /* Fall through to work loop */ 
        } /* EMBX_SUCCESS */
        default:
        {
            i = sprintf(tmp,"No registered transports\n");
            if(len+i >= count)
                goto exit;

            len += sprintf(buf+len,tmp);
            goto exit;
        } /* default */
    }

    do
    {
        if (tpinfo.isInitialized)
        {
            i = sprintf(tmp, "\n%s:\n Zero copy : %s\n Allows pointer translations : %s\n",
                    tpinfo.name, (tpinfo.usesZeroCopy ? "yes" : "no"), (tpinfo.allowsPointerTranslation ? "yes" : "no"));

            if (len+i >= count)
                goto exit;

            len += sprintf(buf+len,tmp);

            i = sprintf(tmp," Allows multiple connections : %s\n Max Ports : %d\n Open handles : %d\n",
                    (tpinfo.allowsMultipleConnections ? "yes" : "no"), tpinfo.maxPorts, tpinfo.nrOpenHandles);

            if (len+i >= count)
                goto exit;

            len += sprintf(buf+len,tmp);

            i = sprintf(tmp, " Ports in use : %d\n Mem start : 0x%08x\n Mem end   : 0x%08x\n\n",
                    tpinfo.nrPortsInUse, (EMBX_UINT)tpinfo.memStart, (EMBX_UINT)tpinfo.memEnd);
        }
        else
        {
            i = sprintf(tmp,"\n%s: Uninitialized\n\n", tpinfo.name);
        }

        if (len+i >= count)
            goto exit;

        len += sprintf(buf+len,tmp);

        res = EMBX_GetNextTransport(&tpinfo);

    } while (res == EMBX_SUCCESS);

exit:
    *eof = 1;
    return len;
}



int __init embx_init( void )
{
EMBX_ERROR res;

    res = EMBX_Init();

    if(res == EMBX_SUCCESS)
    {
        create_proc_read_entry("embx",
                               0      /* default mode */,
                               NULL   /* parent dir */,
                               embx_read_procmem,
                               NULL /* client data */);
        return 0;
    }

    return -ENODEV;
}



void __exit embx_deinit( void )
{
    remove_proc_entry("embx", NULL /* parent dir */);

    EMBX_Deinit();

    EMBX_OS_MUTEX_DESTROY(&_embx_dvr_ctx.lock);
    _embx_dvr_ctx.isLockInitialized = EMBX_FALSE;

    EMBX_HandleManager_Destroy(&_embx_handle_manager);

    return;
}

module_init(embx_init);
module_exit(embx_deinit);

#endif /* __LINUX__ */
