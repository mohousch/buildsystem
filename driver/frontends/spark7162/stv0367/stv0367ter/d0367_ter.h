/**********************************文件头部注释************************************/
//
//
// Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名： d353.h
//
// 创建者： chm
//
// 创建时间： 12-Oct-2005
//
// 文件描述：
//
// 修改记录： 日 期 作 者 版本 修定
//				 --------- --------- ----- -----
//
/**********************************************************************************/

#ifndef _D0367_TER_H
#define _D0367_TER_H

#define NINV 0
#define INV 1

extern STCHIP_Register_t Def367Val[STV0367ofdm_NBREGS];

/* functions --------------------------------------------------------------- */
void D0367ter_Init(TUNER_IOREG_DeviceMap_t *DeviceMap,
				   IOARCH_Handle_t IOHandle,
				   TUNER_TunerType_T TunerType);

YW_ErrorType_T demod_d0367ter_Repeat(IOARCH_Handle_t DemodIOHandle,
									 IOARCH_Handle_t TunerIOHandle,
									 TUNER_IOARCH_Operation_t Operation,
									 unsigned short SubAddr,
									 U8 *Data,
									 U32 TransferSize,
									 U32 Timeout);
YW_ErrorType_T demod_d0367ter_Open(U8 Index);
YW_ErrorType_T demod_d0367ter_GetSignalInfo(U8 Index, U32 *Quality, U32 *Intensity, U32 *Ber);
YW_ErrorType_T demod_d0367ter_IsLocked(U8 Handle, BOOL *IsLocked);
YW_ErrorType_T demod_d0367ter_Close(U8 Index);
YW_ErrorType_T demod_d0367ter_SetStandby(U8 Handle);
YW_ErrorType_T demod_d0367ter_ScanFreq(U8 Handle);
YW_ErrorType_T demod_d0367ter_Reset(U8 Handle);
YW_ErrorType_T demod_d0367ter_Open_test(IOARCH_Handle_t IOHandle);
#endif
/*eof-------------------------------------------------------------- */

