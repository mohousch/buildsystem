/*$Source$*/
/*****************************文件头部注释*************************************/
//
//			Copyright (C), 2013-2018, AV Frontier Tech. Co., Ltd.
//
//
// 文 件 名： $RCSfile$
//
// 创 建 者： D26LF
//
// 创建时间： 2013.12.11
//
// 最后更新： $Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述： ter 6158
//
/******************************************************************************/

#ifndef __D6158_H
#define __D6158_H

/******************************** 文件包含************************************/

#include "nim_panic6158.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************** 常量定义************************************/

/******************************** 数据结构************************************/

/******************************** 宏 定 义************************************/

#if defined(MODULE)
#define printf printk
#endif

/******************************** 变量定义************************************/

/******************************** 变量引用************************************/

/******************************** 函数声明************************************/

YW_ErrorType_T demod_d6158_Open(U8 Handle);
YW_ErrorType_T demod_d6158_Close(U8 Index);
YW_ErrorType_T demod_d6158_IsLocked(U8 Handle, BOOL *IsLocked);
#if !defined(MODULE)
YW_ErrorType_T demod_d6158_Identify(IOARCH_Handle_t IOHandle, U8 ucID, U8 *
									pucActualID);
#endif

YW_ErrorType_T demod_d6158_Repeat(IOARCH_Handle_t DemodIOHandle, /*demod
io ??柄*/
								  IOARCH_Handle_t TunerIOHandle, /*??? io ??柄*/
								  TUNER_IOARCH_Operation_t Operation,
								  unsigned short SubAddr,
								  unsigned char *Data,
								  unsigned int TransferSize,
								  unsigned int Timeout);
YW_ErrorType_T demod_d6158_GetSignalInfo(U8 Handle,
										 unsigned int *Quality,
										 unsigned int *Intensity,
										 unsigned int *Ber);

YW_ErrorType_T demod_d6158_ScanFreq(struct dvb_frontend_parameters *p,
									   struct nim_device *dev, UINT8 System, UINT8 plp_id);
YW_ErrorType_T demod_d6158earda_ScanFreq(struct dvb_frontend_parameters *p,
										 struct nim_device *dev, UINT8 System, UINT8 plp_id);
void nim_config_EARDATEK11658(struct COFDM_TUNER_CONFIG_API *Tuner_API_T, UINT32 i2c_id, UINT8 idx);
/******************************** 函数定义************************************/

#ifdef __cplusplus
}
#endif

#endif /* __D6158_H */
/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

