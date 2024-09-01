/*****************************************************************************
* Copyright (C)2009 fulan Corporation. All Rights Reserved.
*
* File: tun_mxl301.c
*
* Description: This file contains mxl301 basic function in LLD.
* History:
* Date Athor Version Reason
* ============ ============= ========= =================
* 1. 2011/08/26 dmq Ver 0.1 Create file.
* 2. 2011/12/12 dmq Ver 0.2 Add to support Panasonic demodulator
*
*****************************************************************************/

#include <types.h>

#include "ywdefs.h"
#include "ywos.h"
#include "ywlib.h"
#include "ywhal_assert.h"
#include "ywtuner_ext.h"
#include "tuner_def.h"
#include "ioarch.h"

#include "ioreg.h"
#include "tuner_interface.h"
#include "ywtuner_def.h"

#include "d6158_inner.h"

#include "mxL_user_define.h"
#include "tun_mxl301.h"
#include "mxl_common.h"
#include "mxl301rf.h"

#define BURST_SZ 6

static UINT32 mxl301_tuner_cnt = 0;
static struct COFDM_TUNER_CONFIG_EXT *mxl301_cfg[YWTUNERi_MAX_TUNER_NUM] = {NULL};

#if 0
INT32 tun_mxl301_mask_write(UINT32 tuner_id, UINT8 addr, UINT8 reg, UINT8 mask, UINT8 data)
{
	INT32 ret;
	UINT8 value[2];
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle;
	Inst = TUNER_GetScanInfo(tuner_id);
	IOHandle = Inst->DriverParam.Ter.TunerIOHandle;
	value[0] = reg;
	//ret = i2c_write(i2c_id, addr, value, 1);
	//ret = i2c_read(i2c_id, addr, value, 1);
	//ret = I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE,addr, value, 1, 100);
	ret = I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_READ, addr, value, 1, 100);
	value[0] |= mask & data;
	value[0] &= (mask ^ 0xff) | data;
	value[1] = value[0];
	value[0] = reg;
	//ret |= i2c_write(i2c_id, addr, value, 2);
	ret |= I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, addr, value, 2, 100);
	return ret;
}
#else
INT32 tun_mxl301_mask_write(UINT32 tuner_id, UINT8 reg, UINT8 mask, UINT8 data)
{
	INT32 ret;
	UINT8 value[2];
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle = 0;
	Inst = TUNER_GetScanInfo(tuner_id);
	if (Inst->Device == YWTUNER_DELIVER_TER)
		IOHandle = Inst->DriverParam.Ter.TunerIOHandle;
	else if (Inst->Device == YWTUNER_DELIVER_CAB)
		IOHandle = Inst->DriverParam.Cab.TunerIOHandle;
	//value[0] = reg;
	//ret = i2c_write(i2c_id, addr, value, 1);
	//ret = i2c_read(i2c_id, addr, value, 1);
	ret = I2C_ReadWrite(mxl301_cfg[tuner_id]->i2c_adap, TUNER_IO_SA_READ, reg, value, 1, 100);
	value[0] |= mask & data;
	value[0] &= (mask ^ 0xff) | data;
	//value[1] = value[0];
	//value[0] = reg;
	//ret |= i2c_write(i2c_id, addr, value, 2);
	ret |= I2C_ReadWrite(mxl301_cfg[tuner_id]->i2c_adap, TUNER_IO_SA_WRITE, reg, value, 1, 100);
	return ret;
}

#endif
#if 0
INT32 tun_mxl301_i2c_write(UINT32 tuner_id, UINT8 *pArray, UINT32 count)
{
	INT32 result = SUCCESS;
	INT32 i/*, j*/, cycle;
	UINT8 tmp[BURST_SZ + 2];
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle;
	//if(tuner_id >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	Inst = TUNER_GetScanInfo(tuner_id);
	IOHandle = Inst->DriverParam.Ter.TunerIOHandle;
	cycle = count / BURST_SZ;
	mxl301_ptr = mxl301_cfg[tuner_id];
	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{
		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		tun_mxl301_mask_write(tuner_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		tmp[0] = DMD_TCBADR;
		tmp[1] = 0;
		//result |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, 2);
		result |= I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp, 2, 100);
		for (i = 0; i < cycle; i++)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ * i], BURST_SZ);
			tmp[0] = DMD_TCBCOM;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			result = I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp, BURST_SZ + 2, 100);
			//for (j = 0; j < (BURST_SZ/2); j++)
			/// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
		if (BURST_SZ * i < count)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ * i], count - BURST_SZ * i);
			tmp[0] = DMD_TCBCOM;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			result += I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp, count - BURST_SZ * i + 2, 100);
			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	else
	{
		for (i = 0; i < cycle; i++)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ * i], BURST_SZ);
			tmp[0] = 0x09;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			result += I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp, BURST_SZ + 2, 100);
			//for (j = 0; j < (BURST_SZ/2); j++)
			// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
		if (BURST_SZ * i < count)
		{
			YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ * i], count - BURST_SZ * i);
			tmp[0] = 0x09;
			tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			result += I2C_ReadWrite(&IOARCH_Handle[IOHandle].ExtDeviceHandle, TUNER_IO_WRITE, mxl301_ptr->demo_addr, tmp, count - BURST_SZ * i + 2, 100);
			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);
	//if (SUCCESS != result)
	// NIM_PRINTF("%s write i2c failed\n");
	return result;
}
#else
INT32 tun_mxl301_i2c_write(UINT32 tuner_id, UINT8 *pArray, UINT32 count)
{
	INT32 result = SUCCESS;
	INT32 i/*, j*/, cycle;
	UINT8 tmp[BURST_SZ + 2];
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	//if(tuner_id >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	cycle = count / BURST_SZ;
	mxl301_ptr = mxl301_cfg[tuner_id];
	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{
		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		tun_mxl301_mask_write(tuner_id, DMD_TCBSET, 0x7f, 0x53);
		//tmp[0] = DMD_TCBADR;
		//tmp[1] = 0;
		//result |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, 2);
		tmp[0] = 0;
		result |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, DMD_TCBADR, tmp, 1, 100);
		for (i = 0; i < cycle; i++)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], BURST_SZ);
			//tmp[0] = DMD_TCBCOM;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ * i], BURST_SZ);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			result = I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, DMD_TCBCOM, tmp, BURST_SZ + 1, 100);
			//for (j = 0; j < (BURST_SZ/2); j++)
			/// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
		if (BURST_SZ * i < count)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			//tmp[0] = DMD_TCBCOM;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ * i], count - BURST_SZ * i);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			result += I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, DMD_TCBCOM, tmp, count - BURST_SZ * i + 1, 100);
			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	else
	{
		for (i = 0; i < cycle; i++)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], BURST_SZ);
			//tmp[0] = 0x09;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, BURST_SZ+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ * i], BURST_SZ + 1);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			printk("loop %p\n", mxl301_ptr->i2c_adap);
			result += I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, 0x09, tmp, BURST_SZ, 100);
			//for (j = 0; j < (BURST_SZ/2); j++)
			// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
		if (BURST_SZ * i < count)
		{
			//YWLIB_Memcpy(&tmp[2], &pArray[BURST_SZ*i], count - BURST_SZ*i);
			//tmp[0] = 0x09;
			//tmp[1] = mxl301_ptr->cTuner_Base_Addr;
			//result += i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, tmp, count-BURST_SZ*i+2);
			YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ * i], count - BURST_SZ * i);
			tmp[0] = mxl301_ptr->cTuner_Base_Addr;
			result += I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, 0x09, tmp, count - BURST_SZ * i + 1, 100);
			//for (j = 0; j < ((count-BURST_SZ*i)/2); j++)
			// NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[2+2*j], tmp[2+2*j+1]);
		}
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);
	//if (SUCCESS != result)
	// NIM_PRINTF("%s write i2c failed\n");
	return result;
}

#endif
#if 0
INT32 tun_mxl301_i2c_read(UINT32 tuner_id, UINT8 Addr, UINT8 *mData)
{
	INT32 ret = 0;
	UINT8 cmd[4];
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle;
	//if(tuner_id >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	Inst = TUNER_GetScanInfo(tuner_id);
	IOHandle = Inst->DriverParam.Ter.TunerIOHandle;
	mxl301_ptr = mxl301_cfg[tuner_id];
	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{
		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		tun_mxl301_mask_write(tuner_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		cmd[0] = DMD_TCBADR;
		cmd[1] = 0;
		//ret |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2);
		cmd[0] = DMD_TCBCOM;
		cmd[1] = mxl301_ptr->cTuner_Base_Addr;
		cmd[2] = 0xFB;
		cmd[3] = Addr;
		//ret |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 4);
		//tun_mxl301_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
		//cmd[0] = DMD_TCBADR;
		//cmd[1] = 0;
		//ret |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2);
		cmd[0] = DMD_TCBCOM;
		cmd[1] = mxl301_ptr->cTuner_Base_Addr | 0x1;
		//ret |= i2c_write_read(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2, 1);
		*mData = cmd[0];
	}
	else
	{
		cmd[0] = 0x09;
		cmd[1] = mxl301_ptr->cTuner_Base_Addr;
		cmd[2] = 0xFB;
		cmd[3] = Addr;
		//ret = i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 4);
		cmd[1] |= 0x1;
		//ret = i2c_write_read(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2, 1);
		*mData = cmd[0];
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);
	//if (SUCCESS != ret)
	// NIM_PRINTF("%s read i2c failed\n");
	return ret;
}
#else
INT32 tun_mxl301_i2c_read(UINT32 tuner_id, UINT8 Addr, UINT8 *mData)
{
	INT32 ret = 0;
	UINT8 cmd[4];
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	//if(tuner_id >= mxl301_tuner_cnt)
	// return ERR_FAILUE
	mxl301_ptr = mxl301_cfg[tuner_id];
	//osal_mutex_lock(mxl301_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	if (PANASONIC_DEMODULATOR == mxl301_ptr->demo_type)
	{
		tun_mxl301_mask_write(tuner_id, DMD_TCBSET, 0x7f, 0x53);
		cmd[0] = 0;
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, DMD_TCBADR, cmd, 1, 100);
		cmd[0] = mxl301_ptr->cTuner_Base_Addr;
		cmd[1] = 0xFB;
		cmd[2] = Addr;
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, DMD_TCBCOM, cmd, 3, 100);
		cmd[0] = mxl301_ptr->cTuner_Base_Addr | 0x1;
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, DMD_TCBCOM, cmd, 1, 100);
		ret |= I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_READ, DMD_TCBCOM, cmd, 1, 100);
		//printk("[%s]%d,cTuner_Base_Addr=0x%x,Addr=0x%x,ret=%d,cmd[0]=0x%x \n",__FUNCTION__,__LINE__,mxl301_ptr->cTuner_Base_Addr,Addr,ret,cmd[0]);
		*mData = cmd[0];
	}
	else
	{
		//cmd[0] = 0x09;
		//cmd[1] = mxl301_ptr->cTuner_Base_Addr;
		//cmd[2] = 0xFB;
		//cmd[3] = Addr;
		//ret = i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 4);
		cmd[0] = mxl301_ptr->cTuner_Base_Addr;
		cmd[1] = 0xFB;
		cmd[2] = Addr;
		ret = I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, 0x09, cmd, 3, 100);
		cmd[0] |= 0x1;
		//ret = i2c_write_read(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2, 1);
		ret = I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_SA_WRITE, 0x09, cmd, 1, 100);
		ret = I2C_ReadWrite(mxl301_ptr->i2c_adap, TUNER_IO_READ, 0x09, cmd, 1, 100);
		*mData = cmd[0];
	}
	//osal_mutex_unlock(mxl301_ptr->i2c_mutex_id);
	//if (SUCCESS != ret)
	// NIM_PRINTF("%s read i2c failed\n");
	return ret;
}

#endif

INT32 MxL_Set_Register(UINT32 tuner_idx, UINT8 RegAddr, UINT8 RegData)
{
	INT32 ret = 0;
	UINT8 pArray[2];
	pArray[0] = RegAddr;
	pArray[1] = RegData;
	ret = tun_mxl301_i2c_write(tuner_idx, pArray, 2);
	return ret;
}

INT32 MxL_Get_Register(UINT32 tuner_idx, UINT8 RegAddr, UINT8 *RegData)
{
	INT32 ret = 0;
	ret = tun_mxl301_i2c_read(tuner_idx, RegAddr, RegData);
	return ret;
}

INT32 MxL_Soft_Reset(UINT32 tuner_idx)
{
	UINT8 reg_reset = 0xFF;
	INT32 ret = 0;
	ret = tun_mxl301_i2c_write(tuner_idx, &reg_reset, 1);
	return ret;
}

INT32 MxL_Stand_By(UINT32 tuner_idx)
{
	UINT8 pArray[4]; /* a array pointer that store the addr and data pairs for I2C write */
	INT32 ret = 0;
	pArray[0] = 0x01;
	pArray[1] = 0x0;
	pArray[2] = 0x13;
	pArray[3] = 0x0;
	ret = tun_mxl301_i2c_write(tuner_idx, &pArray[0], 4);
	return ret;
}

INT32 MxL_Wake_Up(UINT32 tuner_idx, MxLxxxRF_TunerConfigS *myTuner)
{
	UINT8 pArray[2]; /* a array pointer that store the addr and data pairs for I2C write */
	(void)myTuner;
	pArray[0] = 0x01;
	pArray[1] = 0x01;
	if (tun_mxl301_i2c_write(tuner_idx, pArray, 2))
		return MxL_ERR_OTHERS;
	//if(MxL_Tuner_RFTune(myTuner))
	// return MxL_ERR_RFTUNE;
	return MxL_OK;
}

/*****************************************************************************
* INT32 tun_mxl301_init(struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)
*
* Tuner mxl301 Initialization
*
* Arguments:
* Parameter1: struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config : pointer for Tuner configuration structure
*
* Return Value: INT32 : Result
*****************************************************************************/
INT32 tun_mxl301_init(UINT32 *tuner_idx, struct COFDM_TUNER_CONFIG_EXT *ptrTuner_Config)
{
	INT32 result;
	UINT8 data = 1;
	UINT8 pArray[MAX_ARRAY_SIZE]; /* a array pointer that store the addr and data pairs for I2C write */
	UINT32 Array_Size = 0; /* a integer pointer that store the number of element in above array */
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	MxLxxxRF_TunerConfigS *priv_ptr;
	/* check Tuner Configuration structure is available or not */
	if ((ptrTuner_Config == NULL) || mxl301_tuner_cnt >= YWTUNERi_MAX_TUNER_NUM)
		return ERR_FAILUE;
	mxl301_ptr = (struct COFDM_TUNER_CONFIG_EXT *)YWOS_Malloc(sizeof(struct COFDM_TUNER_CONFIG_EXT));
	if (NULL == mxl301_ptr)
		return ERR_FAILUE;
	YWLIB_Memcpy(mxl301_ptr, ptrTuner_Config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
	mxl301_ptr->priv = (MxLxxxRF_TunerConfigS *)YWOS_Malloc(sizeof(MxLxxxRF_TunerConfigS));
	if (NULL == mxl301_ptr->priv)
	{
		YWOS_Free(mxl301_ptr);
		return ERR_FAILUE;
	}
	//mxl301_cfg[mxl301_tuner_cnt] = mxl301_ptr;
	//*tuner_idx = mxl301_tuner_cnt;
	mxl301_cfg[*tuner_idx] = mxl301_ptr; //jhy modified
	mxl301_tuner_cnt++;
	YWLIB_Memset(mxl301_ptr->priv, 0, sizeof(MxLxxxRF_TunerConfigS));
	priv_ptr = (MxLxxxRF_TunerConfigS *)mxl301_ptr->priv;
	priv_ptr->I2C_Addr = MxL_I2C_ADDR_97;
	priv_ptr->TunerID = MxL_TunerID_MxL301RF;
	priv_ptr->Mode = MxL_MODE_DVBT;
	priv_ptr->Xtal_Freq = MxL_XTAL_24_MHZ;
	priv_ptr->IF_Freq = MxL_IF_5_MHZ;
	priv_ptr->IF_Spectrum = MxL_NORMAL_IF;
	priv_ptr->ClkOut_Amp = MxL_CLKOUT_AMP_0;
	priv_ptr->Xtal_Cap = MxL_XTAL_CAP_8_PF;
	priv_ptr->AGC = MxL_AGC_SEL1;
	priv_ptr->IF_Path = MxL_IF_PATH1;
	priv_ptr->idac_setting = MxL_IDAC_SETTING_OFF;
	/* '11/10/06 : OKAMOTO Control AGC set point. */
	priv_ptr->AGC_set_point = 0x93;
	/* '11/10/06 : OKAMOTO Select AGC external or internal. */
	priv_ptr->bInternalAgcEnable = TRUE;
	/*Soft reset tuner */
	MxL_Soft_Reset(*tuner_idx);
	//printf("[%s]%d \n",__FUNCTION__,__LINE__);
	result = tun_mxl301_i2c_read(*tuner_idx, 0x17, &data);
	//printk("[%s]%d,result=%d,data=0x%x \n",__FUNCTION__,__LINE__,result,data);
	//NIM_PRINTF("result = %x, 0x%x %s!\n", result, data, __FUNCTION__);
	if (SUCCESS != result || 0x09 != (UINT8)(data & 0x0F))
	{
		YWOS_Free(mxl301_ptr->priv);
		YWOS_Free(mxl301_ptr);
		return !SUCCESS;
	}
	/*perform initialization calculation */
	MxL301RF_Init(pArray, &Array_Size, (UINT8)priv_ptr->Mode, (UINT32)priv_ptr->Xtal_Freq,
				  (UINT32)priv_ptr->IF_Freq, (UINT8)priv_ptr->IF_Spectrum, (UINT8)priv_ptr->ClkOut_Setting, (UINT8)priv_ptr->ClkOut_Amp,
				  (UINT8)priv_ptr->Xtal_Cap, (UINT8)priv_ptr->AGC, (UINT8)priv_ptr->IF_Path, priv_ptr->bInternalAgcEnable);
	/* perform I2C write here */
	if (SUCCESS != tun_mxl301_i2c_write(*tuner_idx, pArray, Array_Size))
	{
		//libc_printf("MxL301RF_Init failed\n");
		//printf("[%s]%d,MxL301RF_Init failed! \n",__FUNCTION__,__LINE__);
		YWOS_Free(mxl301_ptr->priv);
		YWOS_Free(mxl301_ptr);
		return !SUCCESS;
	}
	osal_task_sleep(1); /* 1ms delay*/
	return SUCCESS;
}

/*****************************************************************************
* INT32 tun_mxl301_status(UINT8 *lock)
*
* Tuner read operation
*
* Arguments:
* Parameter1: UINT8 *lock : Phase lock status
*
* Return Value: INT32 : Result
*****************************************************************************/

INT32 tun_mxl301_status(UINT32 tuner_idx, UINT8 *lock)
{
	INT32 result;
	UINT8 data;
	//if(tuner_idx >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	result = MxL_Get_Register(tuner_idx, 0x16, &data);
	*lock = (0x03 == (data & 0x03));
	return result;
}

/*****************************************************************************
* INT32 nim_mxl301_control(UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd)
*
* Tuner write operation
*
* Arguments:
* Parameter1: UINT32 freq : Synthesiser programmable divider
* Parameter2: UINT8 bandwidth : channel bandwidth
* Parameter3: UINT8 AGC_Time_Const : AGC time constant
* Parameter4: UINT8 *data :
*
* Return Value: INT32 : Result
*****************************************************************************/
INT32 tun_mxl301_control(UINT32 tuner_idx, UINT32 freq, UINT8 bandwidth, UINT8 AGC_Time_Const, UINT8 *data, UINT8 _i2c_cmd)
{
	INT32 ret;
	INT16 Data;
	UINT8 i, Data1, Data2;
	MxLxxxRF_IF_Freq if_freq;
	MxLxxxRF_TunerConfigS *myTuner;
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	UINT8 pArray[MAX_ARRAY_SIZE]; /* a array pointer that store the addr and data pairs for I2C write */
	UINT32 Array_Size = 0; /* a integer pointer that store the number of element in above array */
	(void)AGC_Time_Const;
	(void)data;
	(void)_i2c_cmd;
	//printf("[%s]%d,tuner_idx=%d\n",__FUNCTION__,__LINE__,tuner_idx);
	if (/*tuner_idx >= mxl301_tuner_cnt ||*/ tuner_idx >= YWTUNERi_MAX_TUNER_NUM)
		return ERR_FAILUE;
	mxl301_ptr = mxl301_cfg[tuner_idx];
	myTuner = mxl301_ptr->priv;
	// do tuner init first.
	//printf("[%s]%d,frq =%d, bw= %d\n",__FUNCTION__,__LINE__,freq, bandwidth);
	if (bandwidth == MxL_BW_6MHz)
	{
		if_freq = MxL_IF_4_MHZ;
	}
	else if (bandwidth == MxL_BW_7MHz)
	{
		if_freq = MxL_IF_4_5_MHZ;
	}
	else// if( bandwidth == MxL_BW_8MHz )
	{
		if_freq = MxL_IF_5_MHZ;
	}
	if (if_freq != myTuner->IF_Freq)
	{
		/*Soft reset tuner */
		MxL_Soft_Reset(tuner_idx);
		/*perform initialization calculation */
		MxL301RF_Init(pArray, &Array_Size, (UINT8)myTuner->Mode, (UINT32)myTuner->Xtal_Freq,
					  (UINT32)myTuner->IF_Freq, (UINT8)myTuner->IF_Spectrum, (UINT8)myTuner->ClkOut_Setting, (UINT8)myTuner->ClkOut_Amp,
					  (UINT8)myTuner->Xtal_Cap, (UINT8)myTuner->AGC, (UINT8)myTuner->IF_Path, myTuner->bInternalAgcEnable);
		/* perform I2C write here */
		if (SUCCESS != tun_mxl301_i2c_write(tuner_idx, pArray, Array_Size))
		{
			//NIM_PRINTF("MxL301RF_Init failed\n");
		}
		osal_task_sleep(10); /* 1ms delay*/
		myTuner->IF_Freq = if_freq;
	}
	// then do tuner tune
	myTuner->RF_Freq_Hz = freq; //modify by yanbinL
	myTuner->BW_MHz = bandwidth;
	/* perform Channel Change calculation */
	ret = MxL301RF_RFTune(pArray, &Array_Size, myTuner->RF_Freq_Hz, myTuner->BW_MHz, myTuner->Mode);
	if (SUCCESS != ret)
		return MxL_ERR_RFTUNE;
	/* perform I2C write here */
	if (SUCCESS != tun_mxl301_i2c_write(tuner_idx, pArray, Array_Size))
		return !SUCCESS;
	MxL_Delay(1); /* Added V9.2.1.0 */
	/* Register read-back based setting for Analog M/N split mode only */
	if (myTuner->TunerID == MxL_TunerID_MxL302RF && myTuner->Mode == MxL_MODE_ANA_MN && myTuner->IF_Split == MxL_IF_SPLIT_ENABLE)
	{
		MxL_Get_Register(tuner_idx, 0xE3, &Data1);
		MxL_Get_Register(tuner_idx, 0xE4, &Data2);
		Data = ((Data2 & 0x03) << 8) + Data1;
		if (Data >= 512)
			Data = Data - 1024;
		if (Data < 20)
		{
			MxL_Set_Register(tuner_idx, 0x85, 0x43);
			MxL_Set_Register(tuner_idx, 0x86, 0x08);
		}
		else if (Data >= 20)
		{
			MxL_Set_Register(tuner_idx, 0x85, 0x9E);
			MxL_Set_Register(tuner_idx, 0x86, 0x0F);
		}
		for (i = 0; i < Array_Size; i += 2)
		{
			if (pArray[i] == 0x11)
				Data1 = pArray[i + 1];
			if (pArray[i] == 0x12)
				Data2 = pArray[i + 1];
		}
		MxL_Set_Register(tuner_idx, 0x11, Data1);
		MxL_Set_Register(tuner_idx, 0x12, Data2);
	}
	if (myTuner->TunerID == MxL_TunerID_MxL302RF)
		MxL_Set_Register(tuner_idx, 0x13, 0x01);
	if (myTuner->TunerID == MxL_TunerID_MxL302RF && myTuner->Mode >= MxL_MODE_ANA_MN && myTuner->IF_Split == MxL_IF_SPLIT_ENABLE)
	{
		if (MxL_Set_Register(tuner_idx, 0x00, 0x01))
			return MxL_ERR_RFTUNE;
	}
	MxL_Delay(30);
	if ((myTuner->Mode == MxL_MODE_DVBT) || (myTuner->Mode >= MxL_MODE_ANA_MN))
	{
		if (MxL_Set_Register(tuner_idx, 0x1A, 0x0D))
			return MxL_ERR_SET_REG;
	}
	if (myTuner->TunerID == MxL_TunerID_MxL302RF && myTuner->Mode >= MxL_MODE_ANA_MN && myTuner->IF_Split == MxL_IF_SPLIT_ENABLE)
	{
		if (MxL_Set_Register(tuner_idx, 0x00, 0x00))
			return MxL_ERR_RFTUNE;
	}
	/* '11/03/16 : OKAMOTO Select IDAC setting in "MxL_Tuner_RFTune". */
	switch (myTuner->idac_setting)
	{
		case MxL_IDAC_SETTING_AUTO:
		{
			UINT8 Array[] =
			{
				0x0D, 0x00,
				0x0C, 0x67,
				0x6F, 0x89,
				0x70, 0x0C,
				0x6F, 0x8A,
				0x70, 0x0E,
				0x6F, 0x8B,
				0x70, 0x10,
			};
			if ((INT32)myTuner->idac_hysterisis < 0 || myTuner->idac_hysterisis >= MxL_IDAC_HYSTERISIS_MAX)
			{
				ret = MxL_ERR_OTHERS;
				break;
			}
			else
			{
				UINT8 ui8_idac_hysterisis;
				ui8_idac_hysterisis = (UINT8)myTuner->idac_hysterisis;
				Array[15] = Array[15] + ui8_idac_hysterisis;
			}
			ret = tun_mxl301_i2c_write(tuner_idx, Array, sizeof(Array));
		}
		break;
		case MxL_IDAC_SETTING_MANUAL:
			if ((INT8)myTuner->dig_idac_code < 0 || myTuner->dig_idac_code >= 63)
			{
				ret = MxL_ERR_OTHERS;
				break;
			}
			else
			{
				UINT8 Array[] = {0x0D, 0x0};
				Array[1] = 0xc0 + myTuner->dig_idac_code; //DIG_ENIDAC_BYP(0x0D[7])=1, DIG_ENIDAC(0x0D[6])=1
				ret = tun_mxl301_i2c_write(tuner_idx, Array, sizeof(Array));
			}
			break;
		case MxL_IDAC_SETTING_OFF:
			ret = MXL301_register_write_bit_name(tuner_idx, MxL_BIT_NAME_DIG_ENIDAC, 0);//0x0D[6] 0
			break;
		default:
			ret = MxL_ERR_OTHERS;
			break;
	}
	return ret;
}

INT32 tun_mxl301_set_addr(UINT32 tuner_idx, UINT8 addr, UINT32 i2c_mutex_id)
{
	struct COFDM_TUNER_CONFIG_EXT *mxl301_ptr = NULL;
	//if(tuner_idx >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	mxl301_ptr = mxl301_cfg[tuner_idx];
	mxl301_ptr->demo_addr = addr;
	mxl301_ptr->i2c_mutex_id = i2c_mutex_id;
	return SUCCESS;
}

INT32 MxL_Check_RF_Input_Power(UINT32 tuner_idx, U32 *RF_Input_Level)
{
	UINT8 RFin1, RFin2, RFOff1, RFOff2;
	//float RFin, RFoff;
	//float cal_factor;
	U32 RFin, RFoff;
	U32 cal_factor;
	MxLxxxRF_TunerConfigS *myTuner;
	//if(tuner_idx >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	myTuner = mxl301_cfg[tuner_idx]->priv;
	if (MxL_Set_Register(tuner_idx, 0x14, 0x01))
		return MxL_ERR_SET_REG;
	MxL_Delay(1);
	if (MxL_Get_Register(tuner_idx, 0x18, &RFin1)) /* LSBs */
		return MxL_ERR_SET_REG;
	if (MxL_Get_Register(tuner_idx, 0x19, &RFin2)) /* MSBs */
		return MxL_ERR_SET_REG;
	if (MxL_Get_Register(tuner_idx, 0xD6, &RFOff1)) /* LSBs */
		return MxL_ERR_SET_REG;
	if (MxL_Get_Register(tuner_idx, 0xD7, &RFOff2)) /* MSBs */
		return MxL_ERR_SET_REG;
	//RFin = (float)(((RFin2 & 0x07) << 5) + ((RFin1 & 0xF8) >> 3) + ((RFin1 & 0x07) * 0.125));
	//RFoff = (float)(((RFOff2 & 0x0F) << 2) + ((RFOff1 & 0xC0) >> 6) + (((RFOff1 & 0x38)>>3) * 0.125));
	RFin = (((RFin2 & 0x07) << 5) + ((RFin1 & 0xF8) >> 3) + ((RFin1 & 0x07) << 3));
	RFoff = (((RFOff2 & 0x0F) << 2) + ((RFOff1 & 0xC0) >> 6) + (((RFOff1 & 0x38) >> 3) << 3));
	if (myTuner->Mode == MxL_MODE_DVBT)
		cal_factor = 113;
	else if (myTuner->Mode == MxL_MODE_ATSC)
		cal_factor = 109;
	else if (myTuner->Mode == MxL_MODE_CAB_STD)
		cal_factor = 110;
	else
		cal_factor = 107;
	*RF_Input_Level = (RFin - RFoff - cal_factor);
	return MxL_OK;
}

#if 0
MxL_ERR_MSG MxL_RFSynth_Lock_Status(MxLxxxRF_TunerConfigS *myTuner, BOOL *isLock)
{
	UINT8 Data;
	*isLock = FALSE;
	if (MxL_Get_Register(myTuner, 0x16, &Data))
		return MxL_ERR_OTHERS;
	Data &= 0x0C;
	if (Data == 0x0C)
		*isLock = TRUE; /* RF Synthesizer is Lock */
	return MxL_OK;
}

MxL_ERR_MSG MxL_REFSynth_Lock_Status(MxLxxxRF_TunerConfigS *myTuner, BOOL *isLock)
{
	UINT8 Data;
	*isLock = FALSE;
	if (MxL_Get_Register(myTuner, 0x16, &Data))
		return MxL_ERR_OTHERS;
	Data &= 0x03;
	if (Data == 0x03)
		*isLock = TRUE; /*REF Synthesizer is Lock */
	return MxL_OK;
}

MxL_ERR_MSG MxL_Check_RF_Input_Power(MxLxxxRF_TunerConfigS *myTuner, REAL32 *RF_Input_Level)
{
	UINT8 RFin1, RFin2, RFOff1, RFOff2;
	REAL32 RFin, RFoff;
	REAL32 cal_factor;
	if (MxL_Set_Register(myTuner, 0x14, 0x01))
		return MxL_ERR_SET_REG;
	MxL_Delay(1);
	if (MxL_Get_Register(myTuner, 0x18, &RFin1)) /* LSBs */
		return MxL_ERR_SET_REG;
	if (MxL_Get_Register(myTuner, 0x19, &RFin2)) /* MSBs */
		return MxL_ERR_SET_REG;
	if (MxL_Get_Register(myTuner, 0xD6, &RFOff1)) /* LSBs */
		return MxL_ERR_SET_REG;
	if (MxL_Get_Register(myTuner, 0xD7, &RFOff2)) /* MSBs */
		return MxL_ERR_SET_REG;
	RFin = (REAL32)(((RFin2 & 0x07) << 5) + ((RFin1 & 0xF8) >> 3) + ((RFin1 & 0x07) * 0.125));
	RFoff = (REAL32)(((RFOff2 & 0x0F) << 2) + ((RFOff1 & 0xC0) >> 6) + (((RFOff1 & 0x38) >> 3) * 0.125));
	if (myTuner->Mode == MxL_MODE_DVBT)
		cal_factor = 113.;
	else if (myTuner->Mode == MxL_MODE_ATSC)
		cal_factor = 109.;
	else if (myTuner->Mode == MxL_MODE_CAB_STD)
		cal_factor = 110.;
	else
		cal_factor = 107.;
	*RF_Input_Level = RFin - RFoff - cal_factor;
	return MxL_OK;
}

/* '11/02/22 : OKAMOTO IF out selection. */
/*====================================================*
 MxL_if_out_select
 --------------------------------------------------
 Description write data on specified bit
 Argument DeviceAddr - MxL Tuner Device address
 if_out IF output
 Return Value UINT32(MxL_OK:Success, Others:Fail)
 *====================================================*/
MxL_ERR_MSG MxL_if_out_select(UINT8 DeviceAddr, MxL_IF_OUT if_out)
{
	UINT8 if1_off_RegData = 0;
	UINT8 if2_off_RegData = 0;
	UINT8 main_to_if2_RegData = 0;
	UINT8 agc_mode_RegData = 0;
	UINT32 Status;
	switch (if_out)
	{
		case MxL_IF_OUT_1:
			if1_off_RegData = 0;
			if2_off_RegData = 1;
			main_to_if2_RegData = 0;
			agc_mode_RegData = 0;
			break;
		case MxL_IF_OUT_2:
			if1_off_RegData = 1;
			if2_off_RegData = 0;
			main_to_if2_RegData = 1;
			agc_mode_RegData = 1;
			break;
		default:
			return MxL_ERR_OTHERS;
	}
	Status = MXL301_register_write_bit_name(DeviceAddr, MxL_BIT_NAME_IF1_OFF, if1_off_RegData);
	if (Status != MxL_OK)
	{
		return Status;
	}
	Status = MXL301_register_write_bit_name(DeviceAddr, MxL_BIT_NAME_IF2_OFF, if2_off_RegData);
	if (Status != MxL_OK)
	{
		return Status;
	}
	Status = MXL301_register_write_bit_name(DeviceAddr, MxL_BIT_NAME_MAIN_TO_IF2, main_to_if2_RegData);
	if (Status != MxL_OK)
	{
		return Status;
	}
	Status = MXL301_register_write_bit_name(DeviceAddr, MxL_BIT_NAME_AGC_MODE, agc_mode_RegData);
	if (Status != MxL_OK)
	{
		return Status;
	}
	return MxL_OK;
}
#endif
