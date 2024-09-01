/******************************************************
mxL_user_define.c
----------------------------------------------------
Rf IC control functions

<Revision History>
'11/09/30 : OKAMOTO Implement "MxL_agc_gain_fix".
'11/02/22 : OKAMOTO Read/Write data on specified bit.
'11/02/21 : OKAMOTO Correct MxL_I2C_Read.
'11/02/10 : OKAMOTO Mount MxL_I2C_Write and MxL_I2C_Read.
'11/02/09 : OKAMOTO Correct build error.
----------------------------------------------------
Copyright(C) 2011 SHARP CORPORATION
******************************************************/

/*

 Driver APIs for MxL Tuner

 Copyright, Maxlinear, Inc.
 All Rights Reserved

 File Name: MxL_User_Define.c

 */
#include <types.h>

#ifdef SHARP_DEBUG_FOR_CYGWIN
#include <stdio.h>
#else
//#include <windows.h>
#endif

#include "mxL_user_define.h"

/* '11/02/09 : Correct build error. */
#include "tun_mxl301.h"
#include "mxl_common.h"

void MxL_Delay(UINT32 mSec)
{
	/* '11/02/10 : Mount MxL_I2C_Write, MxL_I2C_Read, and MxL_Delay. */
	//osal_task_sleep(mSec);
	usleep(mSec);
}

/* '11/02/22 : OKAMOTO Read/Write data on specified bit. */
/*====================================================*
 MXL301_register_read_reg_struct
 --------------------------------------------------
 Description read data on specified bit
 Argument DeviceAddr - MxL Tuner Device address
 in_reg Register address
 pRegData Register data
 Return Value UINT32(MxL_OK:Success, Others:Fail)
 *====================================================*/
UINT32 MXL301_register_read_reg_struct(UINT32 tuner_idx, MXL301_REG_STRUCT in_reg, UINT8 *pRegData)
{
	UINT8 nFilter = 0;
	UINT8 nReadData = 0;
	UINT8 i;
	UINT32 Status = 0;
	/* Check input value */
	if (in_reg.BottomBit > in_reg.TopBit)
		return MxL_ERR_OTHERS;
	if (in_reg.TopBit > 7)
		return MxL_ERR_OTHERS;
	/* Read register */
	Status = tun_mxl301_i2c_read(tuner_idx, in_reg.RegAddr, &nReadData);
	if (Status != MxL_OK)
	{
		return Status;
	}
	/* Shift Data */
	nReadData = nReadData >> in_reg.BottomBit;
	/* Make Filter */
	nFilter = 0x00;
	for (i = 0; i < (in_reg.TopBit - in_reg.BottomBit + 1); i++)
	{
		nFilter |= 1 << i;
	}
	/* Filter read data */
	*pRegData = nFilter & nReadData;
	return MxL_OK;
}

/* '11/02/22 : OKAMOTO Read/Write data on specified bit. */
/*====================================================*
 MXL301_register_write_reg_struct
 --------------------------------------------------
 Description write data on specified bit
 Argument DeviceAddr - MxL Tuner Device address
 in_reg Register address
 RegData Register data
 Return Value UINT32(MxL_OK:Success, Others:Fail)
 *====================================================*/
UINT32 MXL301_register_write_reg_struct(UINT32 tuner_idx, MXL301_REG_STRUCT in_reg, UINT8 RegData)
{
	UINT8 nFilter = 0;
	UINT8 nWriteData = 0;
	UINT8 nReadData = 0;
	UINT8 i;
	UINT32 Status = 0;
	/* Check input value */
	if (in_reg.BottomBit > in_reg.TopBit)
		return MxL_ERR_OTHERS;
	if (in_reg.TopBit > 7)
		return MxL_ERR_OTHERS;
	if (RegData >= (1 << (in_reg.TopBit - in_reg.BottomBit + 1)))
		return MxL_ERR_OTHERS;
	if ((in_reg.TopBit == 7) && (in_reg.BottomBit == 0))
	{
		//Not use filering
		nWriteData = RegData;
	}
	else
	{
		/* Read register */
		Status = tun_mxl301_i2c_read(tuner_idx, in_reg.RegAddr, &nReadData);
		if (Status != MxL_OK)
		{
			return Status;
		}
		/* Make Filter */
		nFilter = 0x00;
		for (i = 0; i < in_reg.BottomBit; i++)
		{
			nFilter |= 1 << i;
		}
		for (i = in_reg.TopBit + 1; i < 8; i++)
		{
			nFilter |= 1 << i;
		}
		/* Filtering */
		nFilter = nFilter & nReadData;
		/* Shift write data*/
		nWriteData = RegData << in_reg.BottomBit;
		/* Make write data */
		nWriteData = nFilter | nWriteData;
	}
	/* Write register */
	{
		UINT8 Array[2];
		Array[0] = in_reg.RegAddr;
		Array[1] = nWriteData;
		Status = tun_mxl301_i2c_write(tuner_idx, Array, 2);
	}
	return Status;
}

INT32 MXL301_register_write_bit_name(UINT32 tuner_idx, MxL_BIT_NAME bit_name, UINT8 RegData)
{
	const MXL301_REG_STRUCT DIG_IDAC_CODE = {0x0D, 5, 0};
	const MXL301_REG_STRUCT DIG_ENIDAC_BYP = {0x0D, 7, 7};
	const MXL301_REG_STRUCT DIG_ENIDAC = {0x0D, 6, 6};
	const MXL301_REG_STRUCT DIG_ATTOFF = {0x0B, 5, 0};
	const MXL301_REG_STRUCT DIG_ATTON = {0x0C, 5, 0};
	const MXL301_REG_STRUCT DIG_IDAC_MODE = {0x0C, 6, 6};
	const MXL301_REG_STRUCT FRONT_BO = {0x78, 5, 0};
	const MXL301_REG_STRUCT IF1_OFF = {0x00, 0, 0};
	const MXL301_REG_STRUCT IF2_OFF = {0x00, 1, 1};
	const MXL301_REG_STRUCT MAIN_TO_IF2 = {0x00, 4, 4};
	const MXL301_REG_STRUCT AGC_MODE = {0x21, 0, 0};
	/* '11/09/30 : OKAMOTO Implement "MxL_agc_gain_fix". */
	const MXL301_REG_STRUCT AGC_GAIN_FIX = {0x15, 3, 2};
	MxL_ERR_MSG Status;
	switch (bit_name)
	{
		case MxL_BIT_NAME_DIG_IDAC_CODE:
			Status = MXL301_register_write_reg_struct(tuner_idx, DIG_IDAC_CODE, RegData);
			break;
		case MxL_BIT_NAME_DIG_ENIDAC_BYP:
			Status = MXL301_register_write_reg_struct(tuner_idx, DIG_ENIDAC_BYP, RegData);
			break;
		case MxL_BIT_NAME_DIG_ENIDAC:
			Status = MXL301_register_write_reg_struct(tuner_idx, DIG_ENIDAC, RegData);
			break;
		case MxL_BIT_NAME_DIG_ATTOFF:
			Status = MXL301_register_write_reg_struct(tuner_idx, DIG_ATTOFF, RegData);
			break;
		case MxL_BIT_NAME_DIG_ATTON:
			Status = MXL301_register_write_reg_struct(tuner_idx, DIG_ATTON, RegData);
			break;
		case MxL_BIT_NAME_DIG_IDAC_MODE:
			Status = MXL301_register_write_reg_struct(tuner_idx, DIG_IDAC_MODE, RegData);
			break;
		case MxL_BIT_NAME_FRONT_BO:
			Status = MXL301_register_write_reg_struct(tuner_idx, FRONT_BO, RegData);
			break;
		case MxL_BIT_NAME_IF1_OFF:
			Status = MXL301_register_write_reg_struct(tuner_idx, IF1_OFF, RegData);
			break;
		case MxL_BIT_NAME_IF2_OFF:
			Status = MXL301_register_write_reg_struct(tuner_idx, IF2_OFF, RegData);
			break;
		case MxL_BIT_NAME_MAIN_TO_IF2:
			Status = MXL301_register_write_reg_struct(tuner_idx, MAIN_TO_IF2, RegData);
			break;
		case MxL_BIT_NAME_AGC_MODE:
			Status = MXL301_register_write_reg_struct(tuner_idx, AGC_MODE, RegData);
			break;
		/* '11/09/30 : OKAMOTO Implement "MxL_agc_gain_fix". */
		case MxL_BIT_NAME_AGC_GAIN_FIX:
			Status = MXL301_register_write_reg_struct(tuner_idx, AGC_GAIN_FIX, RegData);
			break;
		default:
			Status = MxL_ERR_OTHERS;
			break;
	}
	return Status;
}
