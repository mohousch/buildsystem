/*****************************************************************************************
 *
 * FILE NAME : MxL603_OEM_Drv.c
 *
 * AUTHOR : Mahendra Kondur
 *
 * DATE CREATED : 12/23/2011
 *
 * DESCRIPTION : This file contains I2C driver and Sleep functins that
 * OEM should implement for MxL603 APIs
 *
 *****************************************************************************************
 * Copyright (c) 2011, MaxLinear, Inc.
 ****************************************************************************************/

#include <types.h>
#include <nim_dev.h>
#include <nim_tuner.h>

#include "MxL603_TunerApi.h"
#include "MxL603_TunerCfg.h"
#include "tun_mxl603.h"

#include "nim_panic6158.h"

#ifdef BOARD_RAINROW_02
#include "../avl6861/bsp/ibsp.h"
#include "../avl6861/avl/include/AVLEM61_FrontEnd_Config.h"

static AVL_Tuner *g_tuner_config = NULL;

INT32 nim_mxl603_init_ext(AVL_Tuner *tuner_config)
{
	g_tuner_config = tuner_config;
	return SUCCESS;
}

#endif

#if ((SYS_TUN_MODULE == MXL603) || (SYS_TUN_MODULE == ANY_TUNER))

#define TUN_MXL603_PRINTF(...)
#if 1
#define NIM_PRINTF(...)
#else
#if defined(MODULE)
#define NIM_PRINTF(x...) printk(x)
#else
#define NIM_PRINTF(x...) printf(x)
#endif
#endif

#define BURST_SZ 6

#if 1
static MxL_ERR_MSG MxL603_Tuner_RFTune(UINT32 tuner_id, UINT32 RF_Freq_Hz, MXL603_BW_E BWMHz);
#else
static MxL_ERR_MSG MxL603_Tuner_RFTune(UINT32 tuner_id, UINT32 RF_Freq_Hz, MXL603_BW_E BWMHz, MXL603_SIGNAL_MODE_E mode);
#endif
static MxL_ERR_MSG MxL603_Tuner_Init(UINT32 tuner_id);
static BOOL run_on_through_mode(UINT32 tuner_id);

static struct COFDM_TUNER_CONFIG_EXT MxL603_Config[MAX_TUNER_SUPPORT_NUM];
static DEM_WRITE_READ_TUNER m_ThroughMode[MAX_TUNER_SUPPORT_NUM];
static BOOL bMxl_Tuner_Inited[MAX_TUNER_SUPPORT_NUM];
static UINT32 tuner_cnt = MAX_TUNER_SUPPORT_NUM;

#define TUNER_MXL603_DEBUG_LEVEL 0
#if (TUNER_MXL603_DEBUG_LEVEL>0)
#define TUNER_MXL603_DEBUG_PRINTF libc_printf_direct
#else
#define TUNER_MXL603_DEBUG_PRINTF(...) do{}while(0)
#endif

/*----------------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_OEM_WriteRegister
--|
--| AUTHOR : Brenndon Lee
--|
--| DATE CREATED : 7/30/2009
--|
--| DESCRIPTION : This function does I2C write operation.
--|
--| RETURN VALUE : True or False
--|
--|-------------------------------------------------------------------------------------*/

#ifdef BOARD_RAINROW_02
MXL_STATUS MxLWare603_OEM_WriteRegister(UINT32 tuner_id, UINT8 RegAddr, UINT8 RegData)
{
	AVLEM61_Chip *pAVL_Chip = NULL;
	UINT32 status = ERR_FAILUE;
	UINT8 Cmd[2];
	UINT16 ucAddrSize = 2;
	Cmd[0] = RegAddr;
	Cmd[1] = RegData;
	if (NULL != g_tuner_config)
		pAVL_Chip = g_tuner_config->m_pAVL_Chip;
	if (run_on_through_mode(tuner_id))
		status = m_ThroughMode[tuner_id].Dem_Write_Read_Tuner(m_ThroughMode[tuner_id].nim_dev_priv, MxL603_Config[tuner_id].cTuner_Base_Addr, Cmd, 2, 0, 0);
	else
	{
		status = AVLEM61_I2CRepeater_WriteData(g_tuner_config->m_uiSlaveAddress, Cmd, ucAddrSize, pAVL_Chip);
		if (status != SUCCESS)
		{
			TUNER_MXL603_DEBUG_PRINTF("AVLEM61_I2CRepeater_WriteData fail err=%d\n", status);
		}
	}
	return (status == SUCCESS ? MXL_TRUE : MXL_FALSE);
}

MXL_STATUS MxLWare603_OEM_ReadRegister(UINT32 tuner_id, UINT8 RegAddr, UINT8 *DataPtr)
{
	AVLEM61_Chip *pAVL_Chip = NULL;
	UINT32 status = ERR_FAILUE;
	UINT8 Read_Cmd[2];
	UINT8 ucAddrSize = 2;
	UINT16 uiSize = 1;
	/* read step 1. accroding to mxl301 driver API user guide. */
	Read_Cmd[0] = 0xFB;
	Read_Cmd[1] = RegAddr;
	if (NULL != g_tuner_config)
		pAVL_Chip = g_tuner_config->m_pAVL_Chip;
	if (run_on_through_mode(tuner_id))
	{
		status = m_ThroughMode[tuner_id].Dem_Write_Read_Tuner(m_ThroughMode[tuner_id].nim_dev_priv, MxL603_Config[tuner_id].cTuner_Base_Addr, Read_Cmd, 2, 0, 0);
		if (status != SUCCESS)
			return MXL_FALSE;
		status = m_ThroughMode[tuner_id].Dem_Write_Read_Tuner(m_ThroughMode[tuner_id].nim_dev_priv, MxL603_Config[tuner_id].cTuner_Base_Addr, 0, 0, DataPtr, 1);
		if (status != SUCCESS)
			return MXL_FALSE;
	}
	else
	{
		status = AVLEM61_I2CRepeater_ReadData(g_tuner_config->m_uiSlaveAddress, Read_Cmd, ucAddrSize, DataPtr, uiSize, pAVL_Chip);
		if (status != SUCCESS)
		{
			TUNER_MXL603_DEBUG_PRINTF("AVLEM61_I2CRepeater_ReadData fail err=%d", status);
			return MXL_FALSE;
		}
	}
	return MXL_TRUE;
	return status;
}

/*****************************************************************************
* INT32 nim_mxl603_control(UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd)
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
INT32 tun_mxl603_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth, UINT8 AGC_Time_Const, UINT8 *data, UINT8 DvbMode)
{
	MXL603_BW_E BW;
	MxL_ERR_MSG Status;
	MXL603_SIGNAL_MODE_E mode = 0;
	if (tuner_id >= tuner_cnt)
		return ERR_FAILUE;
	//add by yanbinL
	if (DvbMode == 0)
	{
		mode = MXL603_DIG_DVB_T;
		switch (bandwidth)
		{
			case 6:
				BW = MXL603_TERR_BW_6MHz;
				break;
			case 7:
				BW = MXL603_TERR_BW_7MHz;
				break;
			case 8:
				BW = MXL603_TERR_BW_8MHz;
				break;
			default:
				return ERR_FAILUE;
		}
	}
	else if (DvbMode == 1)
	{
		mode = MXL603_DIG_DVB_C;
		switch (bandwidth)
		{
			case 6:
				BW = MXL603_CABLE_BW_6MHz;
				break;
			case 7:
				BW = MXL603_CABLE_BW_7MHz;
				break;
			case 8:
				BW = MXL603_CABLE_BW_8MHz;
				break;
			default:
				return ERR_FAILUE;
		}
	}
	else
	{
		return ERR_FAILUE;
	}
//	switch(BW)
//	{
//		case MXL603_TERR_BW_6MHz:
// Mxl301_TunerConfig[tuner_id].IF_Freq = MxL_IF_4_5_MHZ;
// break;
//		case MXL603_TERR_BW_7MHz:
// Mxl301_TunerConfig[tuner_id].IF_Freq = MxL_IF_4_5_MHZ;
// break;
//		case MXL603_TERR_BW_8MHz:
// Mxl301_TunerConfig[tuner_id].IF_Freq = MxL_IF_5_MHZ;
// break;
//		default:
//			return ERR_FAILUE;
//	}
	if (bMxl_Tuner_Inited[tuner_id] != TRUE)
	{
		if (MxL603_Tuner_Init(tuner_id) != MxL_OK)
		{
			TUNER_MXL603_DEBUG_PRINTF("MxL603_Tuner_Init error\n");
			return ERR_FAILUE;
		}
		bMxl_Tuner_Inited[tuner_id] = TRUE;
	}
	if ((Status = MxL603_Tuner_RFTune(tuner_id, freq, BW, mode)) != MxL_OK)
	{
		TUNER_MXL603_DEBUG_PRINTF("MxL603_Tuner_RFTune error\n");
		return ERR_FAILUE;
	}
	return SUCCESS;
}

static MxL_ERR_MSG MxL603_Tuner_RFTune(UINT32 tuner_id, UINT32 RF_Freq_Hz, MXL603_BW_E BWMHz, MXL603_SIGNAL_MODE_E mode)
{
	MXL_STATUS status;
	MXL603_CHAN_TUNE_CFG_T chanTuneCfg;
	TUNER_MXL603_DEBUG_PRINTF("[MxL603_Tuner_RFTune] freq:%d Hz,BWMHz:%d,mode:%d,Crystal:%d\n", RF_Freq_Hz, BWMHz, mode, MxL603_Config[tuner_id].cTuner_Crystal);
	chanTuneCfg.bandWidth = BWMHz;
	chanTuneCfg.freqInHz = RF_Freq_Hz;
	chanTuneCfg.signalMode = mode;
	chanTuneCfg.xtalFreqSel = MxL603_Config[tuner_id].cTuner_Crystal;;
	chanTuneCfg.startTune = MXL_START_TUNE;
	status = MxLWare603_API_CfgTunerChanTune(tuner_id, chanTuneCfg);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgTunerChanTune\n");
		return status;
	}
	// Wait 15 ms
	MxLWare603_OEM_Sleep(15);
	return MxL_OK;
}

#else

#if defined(MODULE)
U32 *tun_getExtDeviceHandle(UINT32 tuner_id)
{
	struct COFDM_TUNER_CONFIG_EXT *mxl603_ptr = NULL;
	NIM_PRINTF("MxL603_Config[%d].i2c_adap = 0x%08x\n", tuner_id, (int)MxL603_Config[tuner_id].i2c_adap);
	mxl603_ptr = &MxL603_Config[tuner_id];
	return mxl603_ptr->i2c_adap;
}
#else
U32 *tun_getExtDeviceHandle(UINT32 tuner_id)
{
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle = 0;
	Inst = TUNER_GetScanInfo(tuner_id);
	if (Inst->Device == YWTUNER_DELIVER_TER)
		IOHandle = Inst->DriverParam.Ter.TunerIOHandle;
	else if (Inst->Device == YWTUNER_DELIVER_CAB)
		IOHandle = Inst->DriverParam.Cab.TunerIOHandle;
	NIM_PRINTF("IOHandle = %d\n", IOHandle);
	NIM_PRINTF("IOARCH_Handle[%d].ExtDeviceHandle = %d\n", IOHandle,
			   IOARCH_Handle[IOHandle].ExtDeviceHandle);
	if (0 == IOHandle)
	{
#if defined(MODULE)
		printk("[%s:%d]warning IOHahdle maybe error\n", __FUNCTION__, __LINE__);
#else
		printf("[%s:%d]warning IOHahdle maybe error\n", __FUNCTION__, __LINE__);
#endif
	}
	return &IOARCH_Handle[IOHandle].ExtDeviceHandle;
}
#endif

INT32 tun_mxl603_mask_write(UINT32 tuner_id, UINT8 addr, UINT8 reg, UINT8 mask, UINT8 data)
{
	INT32 ret;
	UINT8 value[2];
	U32 *pExtDeviceHandle = tun_getExtDeviceHandle(tuner_id);
	(void)addr;
	//value[0] = reg;
	//ret = i2c_write(i2c_id, addr, value, 1);
	//ret = i2c_read(i2c_id, addr, value, 1);
	ret = I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_READ, reg, value, 1, 100);
	NIM_PRINTF("%s value[0] = %d\n", __FUNCTION__, value[0]);
	value[0] |= mask & data;
	value[0] &= (mask ^ 0xff) | data;
	//value[1] = value[0];
	//value[0] = reg;
	//ret |= i2c_write(i2c_id, addr, value, 2);
	ret |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, reg, value, 1, 100);
	if (SUCCESS != ret)
	{
		NIM_PRINTF("%s write i2c failed\n", __FUNCTION__);
	}
	return ret;
}

INT32 tun_mxl603_i2c_write(UINT32 tuner_id, UINT8 *pArray, UINT32 count)
{
	INT32 result = SUCCESS;
	INT32 i, j, cycle;
	UINT8 tmp[BURST_SZ + 2];
	struct COFDM_TUNER_CONFIG_EXT *mxl603_ptr = NULL;
	U32 *pExtDeviceHandle = tun_getExtDeviceHandle(tuner_id);
	NIM_PRINTF("tun_mxl603_i2c_write tuner_id = %d, count = %d\n", tuner_id, count);
	//if(tuner_id >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	cycle = count / BURST_SZ;
	mxl603_ptr = &MxL603_Config[tuner_id];
	osal_mutex_lock(mxl603_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	tun_mxl603_mask_write(tuner_id, PANIC6158_T2_ADDR, DMD_TCBSET, 0x7f, 0x53);
	//tmp[0] = DMD_TCBADR;
	//tmp[1] = 0;
	//result |= i2c_write(MxL603_Config[tuner_id].i2c_type_id, PANIC6158_T2_ADDR, tmp, 2);
	tmp[0] = 0;
	result |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, DMD_TCBADR, tmp, 1, 100);
	NIM_PRINTF("result = %d\n", result);
	for (i = 0; i < cycle; i++)
	{
		//MEMCPY(&tmp[2], &pArray[BURST_SZ*i], BURST_SZ);
		//tmp[0] = DMD_TCBCOM;
		//tmp[1] = 0xc0;//mxl301_ptr->cTuner_Base_Addr;
		//result += i2c_write(MxL603_Config[tuner_id].i2c_type_id, PANIC6158_T2_ADDR, tmp, BURST_SZ+2);
		YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ * i], BURST_SZ);
		tmp[0] = 0xc0;
		result = I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, DMD_TCBCOM, tmp, BURST_SZ + 1, 100);
		for (j = 0; j < (BURST_SZ / 2); j++)
		{
			NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[1 + 2 * j], tmp[1 + 2 * j + 1]);
		}
	}
	if (BURST_SZ * i < (INT32)count)
	{
		//MEMCPY(&tmp[2], &pArray[BURST_SZ*i], count - BURST_SZ*i);
		//tmp[0] = DMD_TCBCOM;
		//tmp[1] = 0xc0;//mxl301_ptr->cTuner_Base_Addr;
		//result += i2c_write(MxL603_Config[tuner_id].i2c_type_id, PANIC6158_T2_ADDR, tmp, count-BURST_SZ*i+2);
		YWLIB_Memcpy(&tmp[1], &pArray[BURST_SZ * i], count - BURST_SZ * i);
		tmp[0] = 0xc0;
		result += I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, DMD_TCBCOM, tmp, count - BURST_SZ * i + 1, 100);
		for (j = 0; j < (INT32)((count - BURST_SZ * i) / 2); j++)
		{
			NIM_PRINTF("reg[0x%2x], value[0x%02x]\n", tmp[1 + 2 * j], tmp[1 + 2 * j + 1]);
		}
	}
	osal_mutex_unlock(mxl603_ptr->i2c_mutex_id);
	if (SUCCESS != result)
	{
		NIM_PRINTF("%s write i2c failed\n", __FUNCTION__);
	}
	return result;
}

INT32 tun_mxl603_i2c_read(UINT32 tuner_id, UINT8 Addr, UINT8 *mData)
{
	INT32 ret = 0;
	UINT8 cmd[4];
	struct COFDM_TUNER_CONFIG_EXT *mxl603_ptr = NULL;
	U32 *pExtDeviceHandle = tun_getExtDeviceHandle(tuner_id);
	//if(tuner_id >= mxl301_tuner_cnt)
	// return ERR_FAILUE;
	mxl603_ptr = &MxL603_Config[tuner_id];
	osal_mutex_lock(mxl603_ptr->i2c_mutex_id, OSAL_WAIT_FOREVER_TIME);
	tun_mxl603_mask_write(tuner_id, PANIC6158_T2_ADDR, DMD_TCBSET, 0x7f, 0x53);
	//cmd[0] = DMD_TCBADR;
	//cmd[1] = 0;
	//ret |= i2c_write(MxL603_Config[tuner_id].i2c_type_id, PANIC6158_T2_ADDR, cmd, 2);
	cmd[0] = 0;
	ret |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, DMD_TCBADR, cmd, 1, 100);
	//cmd[0] = DMD_TCBCOM;
	//cmd[1] = 0xc0;
	//cmd[2] = 0xFB;
	//cmd[3] = Addr;
	//ret |= i2c_write(MxL603_Config[tuner_id].i2c_type_id, PANIC6158_T2_ADDR, cmd, 4);
	cmd[0] = 0xc0;
	cmd[1] = 0xFB;
	cmd[2] = Addr;
	ret |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, DMD_TCBCOM, cmd, 3, 100);
	//tun_mxl603_mask_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, DMD_TCBSET, 0x7f, 0x53);
	//cmd[0] = DMD_TCBADR;
	//cmd[1] = 0;
	//ret |= i2c_write(mxl301_ptr->i2c_type_id, mxl301_ptr->demo_addr, cmd, 2);
	//cmd[0] = DMD_TCBCOM;
	//cmd[1] = 0xc0 | 0x1;
	//ret |= i2c_write_read(MxL603_Config[tuner_id].i2c_type_id, PANIC6158_T2_ADDR, cmd, 2, 1);
	cmd[0] = 0xc0 | 0x1;
	ret |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, DMD_TCBCOM, cmd, 1, 100);
	ret |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_READ, DMD_TCBCOM, cmd, 1, 100);
	*mData = cmd[0];
	NIM_PRINTF("cmd[0] = %d\n", cmd[0]);
	osal_mutex_unlock(mxl603_ptr->i2c_mutex_id);
	if (SUCCESS != ret)
	{
		NIM_PRINTF("%s read i2c failed\n", __FUNCTION__);
	}
	return ret;
}

MXL_STATUS MxLWare603_OEM_WriteRegister(UINT32 tuner_id, UINT8 RegAddr, UINT8 RegData)
{
	UINT32 status = ERR_FAILUE;
	UINT8 Cmd[2];
	U32 *pExtDeviceHandle = tun_getExtDeviceHandle(tuner_id);
	Cmd[0] = RegAddr;
	Cmd[1] = RegData;
	if (run_on_through_mode(tuner_id))
	{
		// status = m_ThroughMode[tuner_id].Dem_Write_Read_Tuner(m_ThroughMode[tuner_id].nim_dev_priv, MxL603_Config[tuner_id].cTuner_Base_Addr, Cmd, 2, 0, 0);
		status = tun_mxl603_i2c_write(tuner_id, Cmd, 2);
	}
	else
	{
		NIM_PRINTF("I2C_ReadWrite\n");
		//status = i2c_write(MxL603_Config[tuner_id].i2c_type_id, MxL603_Config[tuner_id].cTuner_Base_Addr, Cmd, 2);
		status = I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, Cmd[0],
							   &Cmd[1], 1, 100);
	}
	NIM_PRINTF("MxLWare603_OEM_WriteRegister status = %d\n", status);
	return (status == SUCCESS ? MXL_TRUE : MXL_FALSE);
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_OEM_ReadRegister
--|
--| AUTHOR : Brenndon Lee
--|
--| DATE CREATED : 7/30/2009
--|
--| DESCRIPTION : This function does I2C read operation.
--|
--| RETURN VALUE : True or False
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_OEM_ReadRegister(UINT32 tuner_id, UINT8 RegAddr, UINT8 *DataPtr)
{
	UINT32 status = ERR_FAILUE;
	UINT8 Read_Cmd[2];
	U32 *pExtDeviceHandle = tun_getExtDeviceHandle(tuner_id);
	/* read step 1. accroding to mxl301 driver API user guide. */
	Read_Cmd[0] = 0xFB;
	Read_Cmd[1] = RegAddr;
	if (run_on_through_mode(tuner_id))
	{
		//status = m_ThroughMode[tuner_id].Dem_Write_Read_Tuner(m_ThroughMode[tuner_id].nim_dev_priv, MxL603_Config[tuner_id].cTuner_Base_Addr, Read_Cmd, 2, 0, 0);
		status = tun_mxl603_i2c_write(tuner_id, Read_Cmd, 2);
		if (status != SUCCESS)
			return MXL_FALSE;
		//status = m_ThroughMode[tuner_id].Dem_Write_Read_Tuner(m_ThroughMode[tuner_id].nim_dev_priv, MxL603_Config[tuner_id].cTuner_Base_Addr, 0, 0, DataPtr, 1);
		status = tun_mxl603_i2c_read(tuner_id, RegAddr, DataPtr);
		if (status != SUCCESS)
			return MXL_FALSE;
	}
	else
	{
		//status = i2c_write(MxL603_Config[tuner_id].i2c_type_id, MxL603_Config[tuner_id].cTuner_Base_Addr, Read_Cmd, 2);
		//if(status != SUCCESS)
		// return MXL_FALSE;
		//status = i2c_read(MxL603_Config[tuner_id].i2c_type_id, MxL603_Config[tuner_id].cTuner_Base_Addr, DataPtr, 1);
		//if(status != SUCCESS)
		// return MXL_FALSE;
		status |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, 0xFB, &Read_Cmd[1], 1, 100);
		Read_Cmd[0] = 0xc0 | 0x1;
		status |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_SA_WRITE, DMD_TCBCOM, &Read_Cmd[0], 1, 100);
		status |= I2C_ReadWrite(pExtDeviceHandle, TUNER_IO_READ, DMD_TCBCOM, DataPtr, 1, 100);
	}
	return MXL_TRUE;
	return status;
}

/*****************************************************************************
* INT32 nim_mxl603_control(UINT32 freq, UINT8 bandwidth,UINT8 AGC_Time_Const,UINT8 *data,UINT8 _i2c_cmd)
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
INT32 tun_mxl603_control(UINT32 tuner_id, UINT32 freq, UINT8 bandwidth, UINT8 AGC_Time_Const, UINT8 *data, UINT8 _i2c_cmd)
{
	MXL603_BW_E BW;
	MxL_ERR_MSG Status;
	(void)AGC_Time_Const;
	(void)data;
	(void)_i2c_cmd;
	//if(tuner_id >= tuner_cnt)
	// return ERR_FAILUE;
	switch (bandwidth)
	{
		case 6:
			BW = MXL603_TERR_BW_6MHz;
			break;
		case 7:
			BW = MXL603_TERR_BW_7MHz;
			break;
		case 8:
			BW = MXL603_TERR_BW_8MHz;
			break;
		default:
			return ERR_FAILUE;
	}
//	switch(BW)
//	{
//		case MXL603_TERR_BW_6MHz:
// Mxl301_TunerConfig[tuner_id].IF_Freq = MxL_IF_4_5_MHZ;
// break;
//		case MXL603_TERR_BW_7MHz:
// Mxl301_TunerConfig[tuner_id].IF_Freq = MxL_IF_4_5_MHZ;
// break;
//		case MXL603_TERR_BW_8MHz:
// Mxl301_TunerConfig[tuner_id].IF_Freq = MxL_IF_5_MHZ;
// break;
//		default:
//			return ERR_FAILUE;
//	}
	if (bMxl_Tuner_Inited[tuner_id] != TRUE)
	{
		if (MxL603_Tuner_Init(tuner_id) != MxL_OK)
		{
			return ERR_FAILUE;
		}
		bMxl_Tuner_Inited[tuner_id] = TRUE;
	}
	if ((Status = MxL603_Tuner_RFTune(tuner_id, freq * 1000, BW)) != MxL_OK)
	{
		return ERR_FAILUE;
	}
	return SUCCESS;
}

static MxL_ERR_MSG MxL603_Tuner_RFTune(UINT32 tuner_id, UINT32 RF_Freq_Hz, MXL603_BW_E BWMHz)
{
	MXL_STATUS status;
	MXL603_CHAN_TUNE_CFG_T chanTuneCfg;
	chanTuneCfg.bandWidth = BWMHz;
	chanTuneCfg.freqInHz = RF_Freq_Hz;
	chanTuneCfg.signalMode = MXL603_DIG_DVB_T;
	chanTuneCfg.xtalFreqSel = MxL603_Config[tuner_id].cTuner_Crystal;;
	chanTuneCfg.startTune = MXL_START_TUNE;
	status = MxLWare603_API_CfgTunerChanTune(tuner_id, chanTuneCfg);
	if (status != MXL_SUCCESS)
	{
		TUN_MXL603_PRINTF("Error! MxLWare603_API_CfgTunerChanTune\n");
		return status;
	}
	// Wait 15 ms
	MxLWare603_OEM_Sleep(15);
	return MxL_OK;
}

INT32 tun_mxl603_set_addr(UINT32 tuner_idx, UINT8 addr, UINT32 i2c_mutex_id)
{
	struct COFDM_TUNER_CONFIG_EXT *mxl603_ptr = NULL;
	if (tuner_idx >= tuner_cnt)
		return ERR_FAILUE;
	mxl603_ptr = &MxL603_Config[tuner_idx];
	mxl603_ptr->demo_addr = addr;
	mxl603_ptr->i2c_mutex_id = i2c_mutex_id;
	return SUCCESS;
}
#endif

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_OEM_Sleep
--|
--| AUTHOR : Dong Liu
--|
--| DATE CREATED : 01/10/2010
--|
--| DESCRIPTION : This function complete sleep operation. WaitTime is in ms unit
--|
--| RETURN VALUE : None
--|
--|-------------------------------------------------------------------------------------*/
void MxLWare603_OEM_Sleep(UINT16 DelayTimeInMs)
{
	osal_task_sleep(DelayTimeInMs);
}

static BOOL run_on_through_mode(UINT32 tuner_id)
{
	return (m_ThroughMode[tuner_id].nim_dev_priv && m_ThroughMode[tuner_id].Dem_Write_Read_Tuner);
}

static INT32 set_through_mode(UINT32 tuner_id, DEM_WRITE_READ_TUNER *ThroughMode)
{
	if (tuner_id >= tuner_cnt)
		return ERR_FAILUE;
	MEMCPY((UINT8 *)(&m_ThroughMode[tuner_id]), (UINT8 *)ThroughMode, sizeof(DEM_WRITE_READ_TUNER));
	return SUCCESS;
}

/*****************************************************************************
* INT32 tun_mxl603_init(struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config)
*
* Tuner mxl603 Initialization
*
* Arguments:
* Parameter1: struct COFDM_TUNER_CONFIG_EXT * ptrTuner_Config : pointer for Tuner configuration structure
*
* Return Value: INT32 : Result
*****************************************************************************/
INT32 tun_mxl603_init(UINT32 *tuner_id, struct COFDM_TUNER_CONFIG_EXT *ptrTuner_Config)
{
	UINT16 Xtal_Freq;
	/* check Tuner Configuration structure is available or not */
	if ((ptrTuner_Config == NULL))
		return ERR_FAILUE;
	if (*tuner_id >= MAX_TUNER_SUPPORT_NUM)
		return ERR_FAILUE;
	MEMCPY(&MxL603_Config[*tuner_id], ptrTuner_Config, sizeof(struct COFDM_TUNER_CONFIG_EXT));
	m_ThroughMode[*tuner_id].Dem_Write_Read_Tuner = NULL;
	Xtal_Freq = MxL603_Config[*tuner_id].cTuner_Crystal;
	if (Xtal_Freq >= 1000) //If it's in Khz, then trans it into Mhz.
		Xtal_Freq /= 1000;
	switch (Xtal_Freq)
	{
		case 16:
			MxL603_Config[*tuner_id].cTuner_Crystal = (MXL603_XTAL_FREQ_E)MXL603_XTAL_16MHz;
			break;
		case 24:
			MxL603_Config[*tuner_id].cTuner_Crystal = (MXL603_XTAL_FREQ_E)MXL603_XTAL_24MHz;
			break;
		default:
			return ERR_FAILUE;
	}
	switch (MxL603_Config[*tuner_id].wTuner_IF_Freq)
	{
		case 5000:
			MxL603_Config[*tuner_id].wTuner_IF_Freq = MXL603_IF_5MHz;
			break;
		case 4750:
			MxL603_Config[*tuner_id].wTuner_IF_Freq = MXL603_IF_4_57MHz;
			break;
		case 36000:
			MxL603_Config[*tuner_id].wTuner_IF_Freq = MXL603_IF_36MHz;
			break;
		default:
			return ERR_FAILUE;
	}
	bMxl_Tuner_Inited[*tuner_id] = FALSE;
//	if( MxL603_Tuner_Init(tuner_cnt) != MxL_OK )
//		return ERR_FAILUE;
//	bMxl_Tuner_Inited[tuner_cnt] = TRUE;
	//if(tuner_id)
	// *tuner_id = tuner_cnt; //I2C_TYPE_SCB1
	tuner_cnt = MAX_TUNER_SUPPORT_NUM;
	return SUCCESS;
}

/*****************************************************************************
* INT32 tun_mxl603_status(UINT8 *lock)
*
* Tuner read operation
*
* Arguments:
* Parameter1: UINT8 *lock : Phase lock status
*
* Return Value: INT32 : Result
*****************************************************************************/

INT32 tun_mxl603_status(UINT32 tuner_id, UINT8 *lock)
{
	MXL_STATUS status;
	MXL_BOOL refLockPtr = MXL_UNLOCKED;
	MXL_BOOL rfLockPtr = MXL_UNLOCKED;
	if (tuner_id >= tuner_cnt)
		return ERR_FAILUE;
	*lock = FALSE;
	status = MxLWare603_API_ReqTunerLockStatus(tuner_id, &rfLockPtr, &refLockPtr);
	if (status == MXL_TRUE)
	{
		if (MXL_LOCKED == rfLockPtr && MXL_LOCKED == refLockPtr)
			*lock = TRUE;
	}
	return SUCCESS;
}

INT32 tun_mxl603_rfpower(UINT32 tuner_id, INT16 *rf_power_dbm)
{
	MXL_STATUS status;
	short int RSSI = 0;
	status = MxLWare603_API_ReqTunerRxPower(tuner_id, &RSSI);
	*rf_power_dbm = (INT16)RSSI / 100;
	return SUCCESS;//AVLEM61_EC_OK;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//																		 //
//							Tuner Functions //
//																		 //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
static MxL_ERR_MSG MxL603_Tuner_Init(UINT32 tuner_id)
{
	MXL_STATUS status;
	MXL_BOOL singleSupply_3_3V;
	MXL603_XTAL_SET_CFG_T xtalCfg;
	MXL603_IF_OUT_CFG_T ifOutCfg;
	MXL603_AGC_CFG_T agcCfg;
	MXL603_TUNER_MODE_CFG_T tunerModeCfg;
	MXL603_CHAN_TUNE_CFG_T chanTuneCfg;
	//MXL603_IF_FREQ_E ifOutFreq;
	//Step 1 : Soft Reset MxL603
	status = MxLWare603_API_CfgDevSoftReset(tuner_id);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgDevSoftReset\n");
		return status;
	}
	//Step 2 : Overwrite Default. It should be called after MxLWare603_API_CfgDevSoftReset and MxLWare603_API_CfgDevXtal.
// singleSupply_3_3V = MXL_DISABLE;
	singleSupply_3_3V = MXL_ENABLE;
	status = MxLWare603_API_CfgDevOverwriteDefaults(tuner_id, singleSupply_3_3V);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgDevOverwriteDefaults\n");
		return status;
	}
	//Step 3 : XTAL Setting
	xtalCfg.xtalFreqSel = MxL603_Config[tuner_id].cTuner_Crystal;
	xtalCfg.xtalCap = 25;//20; //12 //匹配电容和晶振型号相关
#if 1
	xtalCfg.clkOutEnable = MXL_ENABLE;
#else
	xtalCfg.clkOutEnable = MXL_DISABLE;//MXL_ENABLE
#endif
	xtalCfg.clkOutDiv = MXL_DISABLE;
	xtalCfg.clkOutExt = MXL_DISABLE;
// xtalCfg.singleSupply_3_3V = MXL_DISABLE;
	xtalCfg.singleSupply_3_3V = singleSupply_3_3V;
	xtalCfg.XtalSharingMode = MXL_DISABLE;
	status = MxLWare603_API_CfgDevXtal(tuner_id, xtalCfg);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgDevXtal\n");
		return status;
	}
	//Step 4 : IF Out setting
// ifOutCfg.ifOutFreq = MXL603_IF_4_57MHz; //MXL603_IF_4_1MHz
// ifOutCfg.ifOutFreq = Mxl301_TunerConfig[tuner_id].IF_Freq;
#if 1
	ifOutCfg.ifOutFreq = MXL603_IF_5MHz; //For match to DEM mn88472 IF: DMD_E_IF_5000KHZ.
#else
	ifOutCfg.ifOutFreq = MXL603_IF_36MHz;
#endif
	ifOutCfg.ifInversion = MXL_DISABLE;
	ifOutCfg.gainLevel = 11;
	ifOutCfg.manualFreqSet = MXL_DISABLE;
	ifOutCfg.manualIFOutFreqInKHz = 0;
	status = MxLWare603_API_CfgTunerIFOutParam(tuner_id, ifOutCfg);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgTunerIFOutParam\n");
		return status;
	}
	//Step 5 : AGC Setting
	agcCfg.agcType = MXL603_AGC_EXTERNAL;
// agcCfg.agcType = MXL603_AGC_SELF;
	agcCfg.setPoint = 66;
	agcCfg.agcPolarityInverstion = MXL_DISABLE;
	status = MxLWare603_API_CfgTunerAGC(tuner_id, agcCfg);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgTunerAGC\n");
		return status;
	}
	//Step 6 : Application Mode setting
	tunerModeCfg.signalMode = MXL603_DIG_DVB_T;
#if 1
	tunerModeCfg.ifOutFreqinKHz = 4100;
#else
	tunerModeCfg.ifOutFreqinKHz = 36000;
#endif
	tunerModeCfg.xtalFreqSel = MxL603_Config[tuner_id].cTuner_Crystal;
	tunerModeCfg.ifOutGainLevel = 11;
	status = MxLWare603_API_CfgTunerMode(tuner_id, tunerModeCfg);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgTunerMode\n");
		return status;
	}
	//Step 7 : Channel frequency & bandwidth setting
	chanTuneCfg.bandWidth = MXL603_TERR_BW_8MHz;
	chanTuneCfg.freqInHz = 474000000;
	chanTuneCfg.signalMode = MXL603_DIG_DVB_T;
	chanTuneCfg.xtalFreqSel = MxL603_Config[tuner_id].cTuner_Crystal;
	chanTuneCfg.startTune = MXL_START_TUNE;
	status = MxLWare603_API_CfgTunerChanTune(tuner_id, chanTuneCfg);
	if (status != MXL_SUCCESS)
	{
		TUNER_MXL603_DEBUG_PRINTF("Error! MxLWare603_API_CfgTunerChanTune\n");
		return status;
	}
	// MxLWare603_API_CfgTunerChanTune():MxLWare603_OEM_WriteRegister(devId, START_TUNE_REG, 0x01) will make singleSupply_3_3V mode become MXL_DISABLE,
	// It cause the power becomes 1.2V.
	//So it must be reset singleSupply_3_3V mode.
// if (MXL_ENABLE == singleSupply_3_3V)
// status |= MxLWare603_OEM_WriteRegister(tuner_id, MAIN_REG_AMP, 0x14);
	// Wait 15 ms
	MxLWare603_OEM_Sleep(15);
	status = MxLWare603_API_CfgTunerLoopThrough(tuner_id, MXL_ENABLE);
	if (status != MXL_SUCCESS)
		return status;
	return MxL_OK;
}

INT32 tun_mxl603_powcontrol(UINT32 tuner_id, UINT8 stdby)
{
	MXL_STATUS status;
	if (tuner_id >= tuner_cnt)
		return ERR_FAILUE;
	if (stdby)
	{
		//TUN_MXL603_PRINTF("start standby mode!\n");
		status = MxLWare603_API_CfgDevPowerMode(tuner_id, MXL603_PWR_MODE_STANDBY);
		if (status != MXL_SUCCESS)
		{
			//TUN_MXL603_PRINTF("standby mode setting fail!\n");
			return ERR_FAILUE;
		}
	}
	else
	{
		//TUN_MXL603_PRINTF("start wakeup mode!\n");
		status = MxLWare603_API_CfgDevPowerMode(tuner_id, MXL603_PWR_MODE_ACTIVE);
		if (status != MXL_SUCCESS)
		{
			//TUN_MXL603_PRINTF("wakeup mode setting fail!\n");
			return ERR_FAILUE;
		}
// if( MxL603_Tuner_Init(tuner_id) != MxL_OK )
// return ERR_FAILUE;
	}
	return SUCCESS;
}

INT32 tun_mxl603_command(UINT32 tuner_id, INT32 cmd, UINT32 param)
{
	INT32 ret = SUCCESS;
#if 0
	if (tuner_id >= tuner_cnt)
		return ERR_FAILUE;
	if (bMxl_Tuner_Inited[tuner_id] == FALSE)
	{
		if (MxL603_Tuner_Init(tuner_id) != MxL_OK)
			return ERR_FAILUE;
		bMxl_Tuner_Inited[tuner_id] = TRUE;
	}
#endif
	switch (cmd)
	{
		case NIM_TUNER_SET_THROUGH_MODE:
			ret = set_through_mode(tuner_id, (DEM_WRITE_READ_TUNER *)param);
			break;
		case NIM_TUNER_POWER_CONTROL:
			tun_mxl603_powcontrol(tuner_id, param);
			break;
		default:
			ret = ERR_FAILUE;
			break;
	}
	return ret;
}

#endif

