/**********************************文件头部注释************************************/
//
//
// Copyright (C), 2010-2015, AV Frontier Tech. Co., Ltd.
//
//
// 文件名： d0367_qam.h
//
// 创建者： chm
//
// 创建时间： 28-Dec-2010
//
// 文件描述：
//
// 修改记录： 日 期 作 者 版本 修定
//				 --------- --------- ----- -----
//
/**********************************************************************************/

#ifndef _D0367_QAM_H
#define _D0367_QAM_H

extern U32 FirstTimeBER[3];

/* functions --------------------------------------------------------------- */

YW_ErrorType_T D0367qam_Init(TUNER_IOREG_DeviceMap_t *DeviceMap,
							 IOARCH_Handle_t IOHandle,
							 TUNER_TunerType_T TunerType);
YW_ErrorType_T D0367qam_Sleep(TUNER_IOREG_DeviceMap_t *DeviceMap,
							  IOARCH_Handle_t IOHandle);
YW_ErrorType_T D0367qam_Wake(TUNER_IOREG_DeviceMap_t *DeviceMap,
							 IOARCH_Handle_t IOHandle);
YW_ErrorType_T D0367qam_I2ctOn(TUNER_IOREG_DeviceMap_t *DeviceMap,
							   IOARCH_Handle_t IOHandle);
YW_ErrorType_T D0367qam_I2ctOff(TUNER_IOREG_DeviceMap_t *DeviceMap,
								IOARCH_Handle_t IOHandle);

YW_ErrorType_T demod_d0367qam_Identify(IOARCH_Handle_t IOHandle, U8 ucID, U8 *pucActualID);
YW_ErrorType_T demod_d0367qam_Repeat(IOARCH_Handle_t DemodIOHandle,
									 IOARCH_Handle_t TunerIOHandle,
									 TUNER_IOARCH_Operation_t Operation,
									 unsigned short SubAddr,
									 U8 *Data,
									 U32 TransferSize,
									 U32 Timeout);
YW_ErrorType_T demod_d0367qam_Open(U8 Index);
YW_ErrorType_T demod_d0367qam_GetSignalInfo(U8 Index, U32 *Quality, U32 *Intensity, U32 *Ber);
YW_ErrorType_T demod_d0367qam_IsLocked(U8 Handle, BOOL *IsLocked);
YW_ErrorType_T demod_d0367qam_Close(U8 Index);
YW_ErrorType_T demod_d0367qam_SetStandby(U8 Handle);
YW_ErrorType_T demod_d0367qam_ScanFreq(U8 Handle);
YW_ErrorType_T demod_d0367qam_Reset(U8 Handle);

YW_ErrorType_T demod_d0367qam_Open_test(IOARCH_Handle_t IOHandle);

#endif

/*eof-------------------------------------------------------------- */
