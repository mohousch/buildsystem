/*$Source$*/
/*****************************文件头部注释*************************************/
//
//			Copyright (C), 2012-2017, AV Frontier Tech. Co., Ltd.
//
//
// 文 件 名： $RCSfile$
//
// 创 建 者： D26LF
//
// 创建时间： 2012.09.07
//
// 最后更新： $Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述： ioarch
//
/******************************************************************************/

/******************************** 文件包含************************************/

#include "dvb_frontend.h"
#include "d6158.h"

#define tuner_i2c_addr 0x1c

/******************************** 常量定义************************************/

/******************************** 数据结构************************************/

/******************************** 宏 定 义************************************/

/******************************** 变量定义************************************/

/******************************** 变量引用************************************/

/******************************** 函数声明************************************/

/******************************** 函数定义************************************/

YW_ErrorType_T TUNER_IOARCH_Open(IOARCH_Handle_t *Handle,
								 TUNER_IOARCH_OpenParams_t *OpenParams)
{
	//todo
	return YW_NO_ERROR;
}

YW_ErrorType_T TUNER_IOARCH_Close(IOARCH_Handle_t Handle,
								  TUNER_IOARCH_CloseParams_t *CloseParams)
{
	//todo
	return YW_NO_ERROR;
}

YW_ErrorType_T TUNER_IOARCH_ReadWrite(IOARCH_Handle_t Handle,
									  TUNER_IOARCH_Operation_t Operation,
									  unsigned short SubAddr,
									  U8 *Data, U32 TransferSize, U32 Timeout)
{
	//todo
	return YW_NO_ERROR;
}

INT32 I2C_Read(struct i2c_adapter *i2c_adap, UINT8 I2cAddr, UINT8 *pData, UINT8 bLen)
{
	//int i;
	int ret;
	UINT8 RegAddr[] = {pData[0]};
	struct i2c_msg msg[] =
	{
		{ .addr = I2cAddr, .flags = I2C_M_RD, .buf = pData, .len = bLen}
	};
	// printk("[information]len:%d MemAddr:%x addr%x\n",bLen,pData[0],I2cAddr);
	ret = i2c_transfer(i2c_adap, msg, 1);
#if 0
	for (i = 0; i < bLen; i++)
	{
		printk("[DATA]%x ", pData[i]);
	}
	printk("\n");
#endif
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			printk("Read error, Reg=[0x%02x], Status=%d\n", RegAddr[0], ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return 0;
}

INT32 I2C_Write(struct i2c_adapter *i2c_adap, UINT8 I2CAddr, UINT8 *pData, UINT8 bLen)
{
	int ret;
	//struct nim_panic6158_private *priv_mem;
	struct i2c_msg i2c_msg = {.addr = I2CAddr, .flags = 0, .buf = pData, .len = bLen };
#if 0
	int i;
	for (i = 0; i < bLen; i++)
	{
		printk("%02x ", pData[i]);
	}
	printk(" write Data,Addr: %x\n", I2CAddr);
#endif
	ret = i2c_transfer(i2c_adap, &i2c_msg, 1);
	if (ret != 1)
	{
		if (ret != -ERESTARTSYS)
			printk("[write error]Reg=[0x%04x], Data=[0x%02x ...], Count=%u, Status=%d\n",
				   pData[0], pData[1], bLen, ret);
		return ret < 0 ? ret : -EREMOTEIO;
	}
	return 0;
}

int I2C_ReadWrite(void *I2CHandle,
				  TUNER_IOARCH_Operation_t Operation,
				  unsigned short SubAddr,
				  U8 *Data, U32 TransferSize, U32 Timeout)
{
	int Ret = 0;
	U8 SubAddress, SubAddress16bit[2] = {0};
	U8 nsbuffer[50];
	BOOL ADR16_FLAG = FALSE;
	struct i2c_adapter *i2c_adap = (struct i2c_adapter *)I2CHandle;
	if (SubAddr & 0xFF00)
		ADR16_FLAG = TRUE;
	else
		ADR16_FLAG = FALSE;
	if (ADR16_FLAG == FALSE)
	{
		SubAddress = (U8)(SubAddr & 0xFF);
	}
	else
	{
		SubAddress16bit[0] = (U8)((SubAddr & 0xFF00) >> 8);
		SubAddress16bit[1] = (U8)(SubAddr & 0xFF);
	}
	switch (Operation)
	{
		/* ---------- Read ---------- */
		case TUNER_IO_SA_READ:
			if (ADR16_FLAG == FALSE)
			{
				Ret = I2C_Write(i2c_adap, tuner_i2c_addr, &SubAddress, 1); /* fix for cable (297 chip) */
			}
			else
			{
				Ret = I2C_Write(i2c_adap, tuner_i2c_addr, SubAddress16bit, 2);
			}
		/* fallthrough (no break;) */
		case TUNER_IO_READ:
			Ret = I2C_Read(i2c_adap, tuner_i2c_addr, Data, TransferSize);
			break;
		case TUNER_IO_SA_WRITE:
			if (TransferSize >= 50)
			{
				return (YWHAL_ERROR_NO_MEMORY);
			}
			if (ADR16_FLAG == FALSE)
			{
				nsbuffer[0] = SubAddress;
				YWLIB_Memcpy((nsbuffer + 1), Data, TransferSize);
				Ret = I2C_Write(i2c_adap, tuner_i2c_addr, nsbuffer, TransferSize + 1);
			}
			else
			{
				nsbuffer[0] = SubAddress16bit[0];
				nsbuffer[1] = SubAddress16bit[1];
				YWLIB_Memcpy((nsbuffer + 2), Data, TransferSize);
				Ret = I2C_Write(i2c_adap, tuner_i2c_addr, nsbuffer, TransferSize + 2);
			}
			break;
		/* ---------- Error ---------- */
		default:
			break;
	}
	// STI2C_Unlock(I2C_HANDLE(I2CHandle)); //lwj remove
	return Ret;
}

/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

