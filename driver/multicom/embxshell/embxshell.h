/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxshell.h                                               */
/*                                                                 */
/* Description:                                                    */
/*         Private declarations for the EMBX shell framework       */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBXSHELL_H
#define _EMBXSHELL_H

#include "embxP.h"

extern EMBX_Context_t _embx_dvr_ctx;

extern EMBX_HandleManager_t _embx_handle_manager;

#define EMBX_HANDLE_TYPE_TRANSPORT  0x01000000
#define EMBX_HANDLE_TYPE_LOCALPORT  0x02000000
#define EMBX_HANDLE_TYPE_REMOTEPORT 0x03000000
#define EMBX_HANDLE_TYPE_FACTORY    0x04000000

extern EMBX_TransportList_t *EMBX_find_transport_entry(const EMBX_CHAR *name);
extern EMBX_ERROR EMBX_close_transport(EMBX_Transport_t *tp, EMBX_ERROR *bInterrupted);

#endif /* _EMBXSHELL_H */
