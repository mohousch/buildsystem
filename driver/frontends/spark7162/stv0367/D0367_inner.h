/*$Source$*/
/*****************************文件头部注释*************************************/
//
//			Copyright (C), 2011-2016, AV Frontier Tech. Co., Ltd.
//
//
// 文 件 名： $RCSfile$
//
// 创 建 者： Administrator
//
// 创建时间： 2011.05.09
//
// 最后更新： $Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述： d0367内部头文件
//
/******************************************************************************/

#ifndef __D0367_INNER_H
#define __D0367_INNER_H

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

U16 ChipGetRegAddress(U32 FieldId);
int ChipGetFieldMask(U32 FieldId);
int ChipGetFieldSign(U32 FieldId);
int ChipGetFieldPosition(U8 Mask);
int ChipGetFieldBits(int mask, int Position);
S32 ChipGetRegisterIndex(TUNER_IOREG_DeviceMap_t *DeviceMap,
						 IOARCH_Handle_t IOHandle, U16 RegId);

void D0367_write(TUNER_IOREG_DeviceMap_t *DeviceMap,
				 IOARCH_Handle_t IOHandle,
				 unsigned char *pcData, int nbdata);
void D0367_read(TUNER_IOREG_DeviceMap_t *DeviceMap,
				IOARCH_Handle_t IOHandle,
				unsigned char *pcData, int NbRegs);

/******************************** 函数定义************************************/

#ifdef __cplusplus
}
#endif

#endif /* __D0367_INNER_H */
/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

