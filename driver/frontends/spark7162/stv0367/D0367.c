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
// 文件描述： d0367
//
/******************************************************************************/

/******************************** 文件包含************************************/

#include <linux/kernel.h> /* Kernel support */
#include <linux/delay.h>
#include <linux/i2c.h>

#include "D3501.h"
#include "ywtuner_ext.h"
#include "tuner_def.h"
#include "ioarch.h"
#include "ioreg.h"
#include "tuner_interface.h"
#include "chip_0367ter.h"
#include "stv0367ofdm_init.h"
#include "D0367_inner.h"

/******************************** 常量定义********************************/

/******************************** 数据结构********************************/

/******************************** 宏 定 义**********************************/

/******************************** 变量定义********************************/

/******************************** 变量引用********************************/

/******************************** 函数声明********************************/

/******************************** 函数定义********************************/

/*****************************************************
**FUNCTION :: IOREG_GetRegAddress
**ACTION ::
**PARAMS IN ::
**PARAMS OUT:: mask
**RETURN :: mask
*****************************************************/
U16 ChipGetRegAddress(U32 FieldId)
{
	U16 RegAddress;
	RegAddress = (FieldId >> 16) & 0xFFFF; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */
	return RegAddress;
}

/*****************************************************
**FUNCTION :: ChipGetFieldMask
**ACTION ::
**PARAMS IN ::
**PARAMS OUT:: mask
**RETURN :: mask
*****************************************************/
int ChipGetFieldMask(U32 FieldId)
{
	int mask;
	mask = FieldId & 0xFF; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */
	return mask;
}

/*****************************************************
**FUNCTION :: ChipGetFieldSign
**ACTION ::
**PARAMS IN ::
**PARAMS OUT:: sign
**RETURN :: sign
*****************************************************/
int ChipGetFieldSign(U32 FieldId)
{
	int sign;
	sign = (FieldId >> 8) & 0x01; /*FieldId is [reg address][reg address][sign][mask] --- 4 bytes */
	return sign;
}

/*****************************************************
**FUNCTION :: ChipGetFieldPosition
**ACTION ::
**PARAMS IN ::
**PARAMS OUT:: position
**RETURN :: position
*****************************************************/
int ChipGetFieldPosition(U8 Mask)
{
	int position = 0, i = 0;
	while ((position == 0) && (i < 8))
	{
		position = (Mask >> i) & 0x01;
		i++;
	}
	return (i - 1);
}
/*****************************************************
**FUNCTION :: ChipGetFieldBits
**ACTION ::
**PARAMS IN ::
**PARAMS OUT:: number of bits
**RETURN :: number of bits
*****************************************************/
int ChipGetFieldBits(int mask, int Position)
{
	int bits, bit;
	int i = 0;
	bits = mask >> Position;
	bit = bits ;
	while ((bit > 0) && (i < 8))
	{
		i++;
		bit = bits >> i;
	}
	return i;
}

/* ----------------------------------------------------------------------------
Name: ChipGetRegisterIndex

Description:Get the index of a register from the pRegMapImage table

Parameters:hChip ==> Handle to the chip
 RegId ==> Id of the register (adress)

Return Value:Index of the register in the register map image
---------------------------------------------------------------------------- */
S32 ChipGetRegisterIndex(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 RegId)
{
	S32 regIndex = -1, reg = 0;
	/* S32 top, bottom,mid; to be used for binary search*/
	if (DeviceMap)
	{
		while (reg < DeviceMap->Registers)
		{
			if (DeviceMap->RegMap[reg].Address == RegId)
			{
				regIndex = reg;
				break;
			}
			reg++;
		}
	}
	return regIndex;
}

void D0367_write(TUNER_IOREG_DeviceMap_t *DeviceMap,
				 IOARCH_Handle_t IOHandle,
				 unsigned char *pcData, int nbdata)
{
	struct i2c_adapter *i2c = (struct i2c_adapter *)IOHandle;
	int ret;
	struct i2c_msg msg[] =
	{
		{ .addr = 0x38 >> 1, .flags = 0, .buf = pcData, .len = nbdata },
	};
	ret = i2c_transfer(i2c, &msg[0], 1);
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			printk("write error, pcData=[0x%x], Status=%d\n", (int)pcData, ret);
	}
}

void D0367_read(TUNER_IOREG_DeviceMap_t *DeviceMap,
				IOARCH_Handle_t IOHandle,
				unsigned char *pcData, int NbRegs)
{
	int ret;
	struct i2c_adapter *i2c = (struct i2c_adapter *)IOHandle;
	struct i2c_msg msg[] =
	{
		{ .addr = 0x38 >> 1, .flags = I2C_M_RD, .buf = pcData, .len = NbRegs }
	};
	ret = i2c_transfer(i2c, msg, 1);
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			printk("Read error, Status=%d\n", ret);
	}
}

/*------------------------------------------------------------------------------*/
/***********************************************************************
 函数名称: demod_d0367ter_Identify

 函数说明: 检测硬件是否0367ter

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010-11-12 lwj 创建
************************************************************************/
int demod_d0367ter_Identify(struct i2c_adapter *i2c, U8 ucID)
{
	int ret;
	U8 pucActualID = 0;
	u8 b0[] = { R367_ID };
	struct i2c_msg msg[] =
	{
		{ .addr = 0x38 >> 1, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = 0x38 >> 1, .flags = I2C_M_RD, .buf = &pucActualID, .len = 1 }
	};
	ret = i2c_transfer(i2c, msg, 2);
	if (ret == 2)
	{
		if (pucActualID == ucID)
		{
			printk("demod_d0367ter_Identify pucActualID = 0x%x\n", pucActualID); //question
			return YW_NO_ERROR;
		}
		else
		{
			printk("demod_d0367ter_Identify YWHAL_ERROR_UNKNOWN_DEVICE \n");//question
			return YWHAL_ERROR_UNKNOWN_DEVICE;
		}
	}
	return YWHAL_ERROR_UNKNOWN_DEVICE;
}

/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

