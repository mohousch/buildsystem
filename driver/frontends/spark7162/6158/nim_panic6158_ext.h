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
// 创建时间： 2013.12.16
//
// 最后更新： $Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述： nim panic6158 ext
//
/******************************************************************************/

#ifndef __NIM_PANIC6158_EXT_H
#define __NIM_PANIC6158_EXT_H

/******************************** 文件包含************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************** 常量定义************************************/

/******************************** 数据结构************************************/

/******************************** 宏 定 义************************************/

/******************************** 变量定义************************************/

/******************************** 变量引用************************************/

/******************************** 函数声明************************************/

INT32 nim_panic6158_channel_change(struct nim_device *dev, struct NIM_Channel_Change *param);
INT32 nim_panic6158_open(struct nim_device *dev);
INT32 nim_panic6158_close(struct nim_device *dev);
INT32 nim_panic6158_get_lock(struct nim_device *dev, UINT8 *lock);
INT32 nim_panic6158_get_SNR(struct nim_device *dev, UINT8 *snr);
INT32 nim_panic6158_get_BER(struct nim_device *dev, UINT32 *ber);
INT32 nim_panic6158_get_AGC(struct nim_device *dev, UINT8 *agc);
INT32 nim_panic6158_get_AGC_301(struct nim_device *dev, UINT8 *agc);
INT32 nim_panic6158_get_AGC_603(struct nim_device *dev, UINT8 *agc);
INT32 nim_panic6158_attach(UINT8 Handle, PCOFDM_TUNER_CONFIG_API pConfig, TUNER_OpenParams_T *OpenParams);
YW_ErrorType_T tuner_mxl301_Identify(IOARCH_Handle_t *i2c_adap);
INT32 nim_panic6158_channel_change_earda(struct nim_device *dev, struct NIM_Channel_Change *param);
INT32 nim_panic6158_ioctl_earda(struct nim_device *dev, INT32 cmd, UINT32 param);
INT32 nim_panic6158_attach_earda(UINT8 Handle, PCOFDM_TUNER_CONFIG_API pConfig, TUNER_OpenParams_T *OpenParams);

INT32 DMD_TCB_WriteRead(void *nim_dev_priv, UINT8 tuner_address, UINT8 *wdata, int wlen, UINT8 *rdata, int rlen);

/******************************** 函数定义************************************/

#ifdef __cplusplus
}
#endif

#endif /* __NIM_PANIC6158_EXT_H */
/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

