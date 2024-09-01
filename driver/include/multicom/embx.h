/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx.h                                                    */
/*                                                                 */
/* Description:                                                    */
/*         Public export header for the EMBX2 API.                 */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBX_H
#define _EMBX_H

#ifdef __WINCE__
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#include <embx_types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Function prototypes
 */

/*
 * - Generic
 */
DLLEXPORT EMBX_ERROR  EMBX_Init   (void);
DLLEXPORT EMBX_ERROR  EMBX_Deinit (void);
DLLEXPORT EMBX_ERROR  EMBX_ModifyTuneable (EMBX_Tuneable_t key, EMBX_UINT value);

#if defined __WINCE__
DLLEXPORT BOOL        EMBX_IOControl(DWORD openContext, DWORD cookie,
                                     PBYTE arg, DWORD argLen,
                                     PBYTE argOut, DWORD argOutLen, PDWORD out);
#endif

/*
 * - Transports
 */
DLLEXPORT EMBX_ERROR EMBX_RegisterTransport   (EMBX_TransportFactory_fn *fn, EMBX_VOID *arg, EMBX_UINT argSize, EMBX_FACTORY *hFactory);
DLLEXPORT EMBX_ERROR EMBX_UnregisterTransport (EMBX_FACTORY hFactory);

DLLEXPORT EMBX_ERROR EMBX_FindTransport       (const EMBX_CHAR *name, EMBX_TPINFO *tpinfo);
DLLEXPORT EMBX_ERROR EMBX_GetFirstTransport   (EMBX_TPINFO *tpinfo);
DLLEXPORT EMBX_ERROR EMBX_GetNextTransport    (EMBX_TPINFO *tpinfo);

DLLEXPORT EMBX_ERROR EMBX_OpenTransport       (const EMBX_CHAR *name, EMBX_TRANSPORT *tp);
DLLEXPORT EMBX_ERROR EMBX_CloseTransport      (EMBX_TRANSPORT tp);
DLLEXPORT EMBX_ERROR EMBX_GetTransportInfo    (EMBX_TRANSPORT tp, EMBX_TPINFO *tpinfo);

DLLEXPORT EMBX_ERROR EMBX_RemoveProcessor     (EMBX_TRANSPORT tp, EMBX_UINT cpuID);

/*
 * - Buffer Management
 */

DLLEXPORT EMBX_ERROR EMBX_Alloc (EMBX_TRANSPORT tp, EMBX_UINT size, EMBX_VOID **buffer);
DLLEXPORT EMBX_ERROR EMBX_Free  (EMBX_VOID *buffer);
DLLEXPORT EMBX_ERROR EMBX_Release (EMBX_TRANSPORT tp, EMBX_VOID *buffer);

DLLEXPORT EMBX_ERROR EMBX_GetBufferSize(EMBX_VOID *buffer, EMBX_UINT *size);

/*
 * - Object Management
 */

DLLEXPORT EMBX_ERROR EMBX_RegisterObject   (EMBX_TRANSPORT tp, EMBX_VOID  *object, EMBX_UINT size, EMBX_HANDLE *handle);
DLLEXPORT EMBX_ERROR EMBX_DeregisterObject (EMBX_TRANSPORT tp, EMBX_HANDLE handle);
DLLEXPORT EMBX_ERROR EMBX_GetObject        (EMBX_TRANSPORT tp, EMBX_HANDLE handle, EMBX_VOID **object, EMBX_UINT *size);

/*
 * - Ports
 */

/* EMBX LocalPort CallBack funtion */
DLLEXPORT EMBX_ERROR EMBX_CreatePortCallback (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port,
					      EMBX_Callback_t callback, EMBX_HANDLE handle);

DLLEXPORT EMBX_ERROR EMBX_CreatePort     (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port);
DLLEXPORT EMBX_ERROR EMBX_Connect        (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port);
DLLEXPORT EMBX_ERROR EMBX_ConnectBlock   (EMBX_TRANSPORT tp, const EMBX_CHAR *portName, EMBX_PORT *port);
DLLEXPORT EMBX_ERROR EMBX_ConnectBlockTimeout (EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port, EMBX_UINT timeout);

DLLEXPORT EMBX_ERROR EMBX_ClosePort      (EMBX_PORT port);
DLLEXPORT EMBX_ERROR EMBX_InvalidatePort (EMBX_PORT port);

/*
 * - Send and Receive
 */
DLLEXPORT EMBX_ERROR EMBX_Receive      (EMBX_PORT port, EMBX_RECEIVE_EVENT *);
DLLEXPORT EMBX_ERROR EMBX_ReceiveBlock (EMBX_PORT port, EMBX_RECEIVE_EVENT *);
DLLEXPORT EMBX_ERROR EMBX_ReceiveBlockTimeout (EMBX_PORT port, EMBX_RECEIVE_EVENT *recev, EMBX_UINT timeout);

DLLEXPORT EMBX_ERROR EMBX_SendMessage  (EMBX_PORT port, EMBX_VOID *buffer,  EMBX_UINT size);

DLLEXPORT EMBX_ERROR EMBX_SendObject   (EMBX_PORT port, EMBX_HANDLE handle, EMBX_UINT offset, EMBX_UINT size);
DLLEXPORT EMBX_ERROR EMBX_UpdateObject (EMBX_PORT port, EMBX_HANDLE handle, EMBX_UINT offset, EMBX_UINT size);

/*
 * - Pointer manipulation
 */
DLLEXPORT EMBX_ERROR EMBX_Offset  (EMBX_TRANSPORT tp, EMBX_VOID *address, EMBX_INT   *offset);
DLLEXPORT EMBX_ERROR EMBX_Address (EMBX_TRANSPORT tp, EMBX_INT   offset,  EMBX_VOID **address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EMBX_H */
