/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlb.h                                                  */
/*                                                                 */
/* Description:                                                    */
/*    EMBX Inprocess loopback transport public interface           */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBXLB_H
#define _EMBXLB_H

#include "embx_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct {
    EMBX_CHAR name[EMBX_MAX_TRANSPORT_NAME+1];
    EMBX_UINT maxPorts;
    EMBX_UINT maxAllocations;
    EMBX_UINT maxObjects;
    EMBX_BOOL multiConnect;
    EMBX_BOOL blockInit;
} EMBXLB_Config_t;


EMBX_Transport_t *EMBXLB_loopback_factory(EMBX_VOID *param);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EMBXLB_H */
