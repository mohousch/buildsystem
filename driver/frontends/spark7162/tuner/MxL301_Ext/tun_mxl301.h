/*****************************************************************************
* Copyright (C)2011 FULAN Corporation. All Rights Reserved.
*
* File: tun_mxl301.h
*
* Description: MXL301 TUNER.
* History:
* Date Athor Version Reason
* ============ ============= ========= =================
* 1. 2011/08/26 dmq Ver 0.1 Create file.
*****************************************************************************/

#ifndef __TUN_MXL301_H__
#define __TUN_MXL301_H__

#include <types.h>
#include "nim_tuner.h"

#ifdef __cplusplus
extern "C"
{
#endif

INT32 tun_mxl301_i2c_write(UINT32 tuner_id, UINT8 *pArray, UINT32 count);
INT32 tun_mxl301_i2c_read(UINT32 tuner_id, UINT8 Addr, UINT8 *mData);

INT32 tun_mxl301_init(UINT32 *tuner_idx, struct COFDM_TUNER_CONFIG_EXT *ptrTuner_Config);
INT32 tun_mxl301_control(UINT32 tuner_idx, UINT32 freq, UINT8 bandwidth, UINT8 AGC_Time_Const, UINT8 *data, UINT8 _i2c_cmd);
INT32 tun_mxl301_status(UINT32 tuner_idx, UINT8 *lock);

#ifdef __cplusplus
}
#endif

#endif /* __TUN_MXL301_H__ */
