/**********************************文件头部注释************************************/
//
//
// Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名： stv0367ofdm_drv.c
//
// 创建者： lwj
//
// 创建时间： 2010-11-12
//
// 文件描述：
//
// 修改记录： 日 期 作 者 版本 修定
//				 --------- --------- ----- -----
//
/**********************************************************************************/

/* Includes ---------------------------------------------------------------- */

#define TUNER_ST
#include <linux/kernel.h> /* Kernel support */
#include <linux/delay.h>

#include "D3501.h"

#include "ywtuner_ext.h"
#include "tuner_def.h"
#include "ioarch.h"
#include "ioreg.h"
#include "tuner_interface.h"
#include "chip_0367ter.h"

/* 367 includes */

#include "stv0367ofdm_init.h"
#include "stv0367ofdm_drv.h"

/* dcdc #define CPQ_LIMIT 23*/
#define CPQ_LIMIT 23

/* Current LLA revision */
static const char *Revision367ofdm = "STV367ofdm-LLA_REL_3.0";
/*global variables */
/*U32 PreviousBitErrorRate=0;
U32 PreviousPacketErrorRate=0;*/

/*local functions definition*/
int FE_367TER_FilterCoeffInit(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
							  IOARCH_Handle_t DemodIOHandle,
							  U16 CellsCoeffs[2][6][5], U32 DemodXtal);
void FE_367TER_AGC_IIR_LOCK_DETECTOR_SET(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
										 IOARCH_Handle_t DemodIOHandle);
void FE_367TER_AGC_IIR_RESET(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
							 IOARCH_Handle_t DemodIOHandle);
FE_TER_SignalStatus_t
FE_367TER_CheckSYR(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
				   IOARCH_Handle_t DemodIOHandle);
FE_TER_SignalStatus_t
FE_367TER_CheckCPAMP(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
					 IOARCH_Handle_t DemodIOHandle, S32 FFTmode);
void FE_STV0367ofdm_SetTS_Parallel_Serial(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
										  IOARCH_Handle_t DemodIOHandle,
										  FE_TS_OutputMode_t PathTS);
void FE_STV0367ofdm_SetCLK_Polarity(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
									IOARCH_Handle_t DemodIOHandle,
									FE_TS_ClockPolarity_t clock);
////FE_LLA_Error_t FE_367ofdm_Algo(FE_367ofdm_Handle_t Handle, FE_TER_SearchParams_t *pSearch, FE_TER_SearchResult_t *pResult);
U32 FE_367ofdm_GetMclkFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
						   IOARCH_Handle_t DemodIOHandle, U32 ExtClk_Hz);

U16 CellsCoeffs_8MHz_367cofdm[3][6][5] =
{
	{
		{0x10EF, 0xE205, 0x10EF, 0xCE49, 0x6DA7}, /* CELL 1 COEFFS 27M */
		{0x2151, 0xc557, 0x2151, 0xc705, 0x6f93}, /* CELL 2 COEFFS */
		{0x2503, 0xc000, 0x2503, 0xc375, 0x7194}, /* CELL 3 COEFFS */
		{0x20E9, 0xca94, 0x20e9, 0xc153, 0x7194}, /* CELL 4 COEFFS */
		{0x06EF, 0xF852, 0x06EF, 0xC057, 0x7207}, /* CELL 5 COEFFS */
		{0x0000, 0x0ECC, 0x0ECC, 0x0000, 0x3647} /* CELL 6 COEFFS */
	},
	{
		{0x10A0, 0xE2AF, 0x10A1, 0xCE76, 0x6D6D}, /* CELL 1 COEFFS 25M */
		{0x20DC, 0xC676, 0x20D9, 0xC80A, 0x6F29},
		{0x2532, 0xC000, 0x251D, 0xC391, 0x706F},
		{0x1F7A, 0xCD2B, 0x2032, 0xC15E, 0x711F},
		{0x0698, 0xFA5E, 0x0568, 0xC059, 0x7193},
		{0x0000, 0x0918, 0x149C, 0x0000, 0x3642} /* CELL 6 COEFFS */
	},
	{
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, /* 30M */
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000}
	}

} ;

U16 CellsCoeffs_7MHz_367cofdm[3][6][5] =
{
	{
		{0x12CA, 0xDDAF, 0x12CA, 0xCCEB, 0x6FB1}, /* CELL 1 COEFFS 27M*/
		{0x2329, 0xC000, 0x2329, 0xC6B0, 0x725F}, /* CELL 2 COEFFS */
		{0x2394, 0xC000, 0x2394, 0xC2C7, 0x7410}, /* CELL 3 COEFFS */
		{0x251C, 0xC000, 0x251C, 0xC103, 0x74D9}, /* CELL 4 COEFFS */
		{0x0804, 0xF546, 0x0804, 0xC040, 0x7544}, /* CELL 5 COEFFS */
		{0x0000, 0x0CD9, 0x0CD9, 0x0000, 0x370A} /* CELL 6 COEFFS */
	},
	{
		{0x1285, 0xDE47, 0x1285, 0xCD17, 0x6F76}, /*25M*/
		{0x234C, 0xC000, 0x2348, 0xC6DA, 0x7206},
		{0x23B4, 0xC000, 0x23AC, 0xC2DB, 0x73B3},
		{0x253D, 0xC000, 0x25B6, 0xC10B, 0x747F},
		{0x0721, 0xF79C, 0x065F, 0xC041, 0x74EB},
		{0x0000, 0x08FA, 0x1162, 0x0000, 0x36FF}
	},
	{
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, /* 30M */
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000}
	}

} ;

U16 CellsCoeffs_6MHz_367cofdm[3][6][5] =
{
	{
		{0x1699, 0xD5B8, 0x1699, 0xCBC3, 0x713B}, /* CELL 1 COEFFS 27M*/
		{0x2245, 0xC000, 0x2245, 0xC568, 0x74D5}, /* CELL 2 COEFFS */
		{0x227F, 0xC000, 0x227F, 0xC1FC, 0x76C6}, /* CELL 3 COEFFS */
		{0x235E, 0xC000, 0x235E, 0xC0A7, 0x778A}, /* CELL 4 COEFFS */
		{0x0ECB, 0xEA0B, 0x0ECB, 0xC027, 0x77DD}, /* CELL 5 COEFFS */
		{0x0000, 0x0B68, 0x0B68, 0x0000, 0xC89A}, /* CELL 6 COEFFS */
	},
	{
		{0x1655, 0xD64E, 0x1658, 0xCBEF, 0x70FE}, /*25M*/
		{0x225E, 0xC000, 0x2256, 0xC589, 0x7489},
		{0x2293, 0xC000, 0x2295, 0xC209, 0x767E},
		{0x2377, 0xC000, 0x23AA, 0xC0AB, 0x7746},
		{0x0DC7, 0xEBC8, 0x0D07, 0xC027, 0x7799},
		{0x0000, 0x0888, 0x0E9C, 0x0000, 0x3757}

	},
	{
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, /* 30M */
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0000, 0x0000, 0x0000, 0x0000, 0x0000}
	}

} ;

#define RF_LOOKUP_TABLE_SIZE_0 19
#define RF_LOOKUP_TABLE_SIZE_1 7
#define RF_LOOKUP_TABLE_SIZE_2 7
#define RF_LOOKUP_TABLE_SIZE_3 11
#define IF_LOOKUP_TABLE_SIZE_4 43

U32 FE_TUNER_RF_LookUp3[RF_LOOKUP_TABLE_SIZE_3] =
{ 66, 79, 87, 97, 108, 119, 132, 144, 160, 182, 217};

/*-44dBm to -86dBm lnagain=3 IF AGC*/
U32 FE_TUNER_IF_LookUp4[IF_LOOKUP_TABLE_SIZE_4] =
{
	990, 1020, 1070, 1160, 1220, 1275, 1350, 1410, 1475, 1530, 1600, 1690, 1785, 1805, 1825, 1885, 1910,
	1950, 2000, 2040, 2090, 2130, 2200, 2295, 2335, 2400, 2450, 2550, 2600, 2660, 2730, 2805, 2840, 2905,
	2955, 3060, 3080, 3150, 3180, 3315, 3405, 4065, 4080
};

/*********************************************************
--FUNCTION :: FE_367TER_FilterCoeffInit
--ACTION :: Apply filter coeffs values

--PARAMS IN :: Handle to the Chip
 CellsCoeffs[2][6][5]
--PARAMS OUT:: NONE
--RETURN :: 0 error , 1 no error
--********************************************************/
int FE_367TER_FilterCoeffInit(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
							  IOARCH_Handle_t DemodIOHandle,
							  U16 CellsCoeffs[2][6][5], U32 DemodXtal)
{
	int i, j, k, InternalFreq;
	if (DemodDeviceMap)
	{
		InternalFreq = FE_367ofdm_GetMclkFreq(DemodDeviceMap, DemodIOHandle, DemodXtal);
		if (InternalFreq == 53125000)
			k = 1; /* equivalent to Xtal 25M on 362*/
		else if (InternalFreq == 54000000)
			k = 0; /* equivalent to Xtal 27M on 362*/
		else if (InternalFreq == 52500000)
			k = 2; /* equivalent to Xtal 30M on 362*/
		else
		{
			return 0;
		}
		for (i = 1; i <= 6; i++)
		{
			ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_IIR_CELL_NB, i - 1);
			for (j = 1; j <= 5; j++)
			{
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, (R367ofdm_IIRCX_COEFF1_MSB + 2 * (j - 1)), MSB(CellsCoeffs[k][i - 1][j - 1]));
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, (R367ofdm_IIRCX_COEFF1_LSB + 2 * (j - 1)), LSB(CellsCoeffs[k][i - 1][j - 1]));
			}
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

/**********************************************************/
/*********************************************************
--FUNCTION :: FE_367TER_SpeedInit
--ACTION :: calculate I2C speed (for SystemWait ...)

--PARAMS IN :: Handle to the Chip
--PARAMS OUT:: NONE
--RETURN :: #ms for an I2C reading access ..
--********************************************************/
#if 0
int FE_367TER_SpeedInit(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle)
{
	unsigned int i, tempo;
	clock_t time1, time2, time_diff;
	time1 = time_now();
	for (i = 0; i < 16; i++) ChipGetField(hChip, F367ofdm_EPQ1);
	time2 = time_now();
	time_diff = time_minus(time2, time1);
	tempo = (time_diff * 1000) / ST_GetClocksPerSecond() + 4; /* Duration in milliseconds, + 4 for rounded value */
	tempo = tempo << 4;
	return tempo;
}
#endif //lwj remove

/*****************************************************
--FUNCTION :: FE_367TER_AGC_IIR_LOCK_DETECTOR_SET
--ACTION :: Sets Good values for AGC IIR lock detector
--PARAMS IN :: Handle to the Chip
--PARAMS OUT :: None
--***************************************************/
void FE_367TER_AGC_IIR_LOCK_DETECTOR_SET(TUNER_IOREG_DeviceMap_t *DemodDeviceMap,
										 IOARCH_Handle_t DemodIOHandle)

{
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_LSB, 0x00);
	/* Lock detect 1 */
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_CHOICE, 0x00);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_MSB, 0x06);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_AUT_AGC_TARGET_LSB, 0x04);
	/* Lock detect 2 */
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_CHOICE, 0x01);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_MSB, 0x06);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_AUT_AGC_TARGET_LSB, 0x04);
	/* Lock detect 3 */
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_CHOICE, 0x02);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_MSB, 0x01);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_AUT_AGC_TARGET_LSB, 0x00);
	/* Lock detect 4 */
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_CHOICE, 0x03);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LOCK_DETECT_MSB, 0x01);
	ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_AUT_AGC_TARGET_LSB, 0x00);
}

/*****************************************************
--FUNCTION :: FE_367TER_IIR_FILTER_INIT
--ACTION :: Sets Good IIR Filters coefficients
--PARAMS IN :: Handle to the Chip
 selected bandwidth
--PARAMS OUT :: None
--***************************************************/
int FE_367TER_IIR_FILTER_INIT(TUNER_IOREG_DeviceMap_t *DeviceMap,
							  IOARCH_Handle_t IOHandle, U8 Bandwidth,
							  U32 DemodXtalValue)

{
	if (DeviceMap != NULL)
	{
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_NRST_IIR, 0);
		switch (Bandwidth)
		{
			case 6:
				if (!FE_367TER_FilterCoeffInit(DeviceMap, IOHandle, CellsCoeffs_6MHz_367cofdm, DemodXtalValue))
					return 0;
				break;
			case 7:
				if (!FE_367TER_FilterCoeffInit(DeviceMap, IOHandle, CellsCoeffs_7MHz_367cofdm, DemodXtalValue))
					return 0;
				break;
			case 8:
				if (!FE_367TER_FilterCoeffInit(DeviceMap, IOHandle, CellsCoeffs_8MHz_367cofdm, DemodXtalValue))
					return 0;
				break;
			default:
				return 0;
		}
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_NRST_IIR, 1);
		return 1;
	}
	else
	{
		return 0;
	}
}

/*****************************************************
--FUNCTION :: FE_367TER_AGC_IIR_RESET
--ACTION :: AGC reset procedure
--PARAMS IN :: Handle to the Chip
--PARAMS OUT :: None
--***************************************************/
void FE_367TER_AGC_IIR_RESET(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
	U8 com_n;
	com_n = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_N);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_N, 0x07);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_SOFT_RSTN, 0x00);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_AGC_ON, 0x00);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_SOFT_RSTN, 0x01);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_AGC_ON, 0x01);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_N, com_n);
}
/*********************************************************
--FUNCTION :: FE_367ofdm_duration
--ACTION :: return a duration regarding mode
--PARAMS IN :: mode, tempo1,tempo2,tempo3
--PARAMS OUT:: none
--********************************************************/
int FE_367ofdm_duration(S32 mode, int tempo1, int tempo2, int tempo3)
{
	int local_tempo = 0;
	switch (mode)
	{
		case 0:
			local_tempo = tempo1;
			break;
		case 1:
			local_tempo = tempo2;
			break ;
		case 2:
			local_tempo = tempo3;
			break;
		default:
			break;
	}
	/* WAIT_N_MS(local_tempo); */
	return local_tempo;
}

/*********************************************************
--FUNCTION :: FE_367TER_CheckSYR
--ACTION :: Get SYR status
--PARAMS IN :: Handle to the Chip
--PARAMS OUT:: CPAMP status
--********************************************************/
FE_TER_SignalStatus_t FE_367TER_CheckSYR(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle)
{
	int wd = 100;
	unsigned short int SYR_var;
	S32 SYRStatus;
	SYR_var = ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SYR_LOCK);
	while ((!SYR_var) && (wd > 0))
	{
		msleep(2);
		wd -= 2;
		SYR_var = ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SYR_LOCK);
	}
	if (!SYR_var)
	{
		SYRStatus = FE_TER_NOSYMBOL;
	}
	else
	{
		SYRStatus = FE_TER_SYMBOLOK;
	}
	return SYRStatus;
}

/*********************************************************
--FUNCTION :: FE_367TER_CheckCPAMP
--ACTION :: Get CPAMP status
--PARAMS IN :: Handle to the Chip
--PARAMS OUT:: CPAMP status
--********************************************************/
FE_TER_SignalStatus_t FE_367TER_CheckCPAMP(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, S32 FFTmode)
{
	S32 CPAMPvalue = 0, CPAMPStatus, CPAMPMin;
	int wd = 0;
	switch (FFTmode)
	{
		case 0: /*2k mode*/
			CPAMPMin = 20;
			wd = 10;
			break;
		case 1: /*8k mode*/
			CPAMPMin = 80;
			wd = 55;
			break;
		case 2: /*4k mode*/
			CPAMPMin = 40;
			wd = 30;
			break;
		default:
			CPAMPMin = 0xffff; /*drives to NOCPAMP */
			break;
	}
	CPAMPvalue = ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_PPM_CPAMP_DIRECT);
	while ((CPAMPvalue < CPAMPMin) && (wd > 0))
	{
		msleep(1);
		wd -= 1;
		CPAMPvalue = ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_PPM_CPAMP_DIRECT);
		/*printf("CPAMPvalue= %d at wd=%d\n",CPAMPvalue,wd); */
	}
	/*printf("******last CPAMPvalue= %d at wd=%d\n",CPAMPvalue,wd); */
	if (CPAMPvalue < CPAMPMin)
	{
		CPAMPStatus = FE_TER_NOCPAMP;
	}
	else
		CPAMPStatus = FE_TER_CPAMPOK;
	return CPAMPStatus;
}

/*****************************************************
--FUNCTION :: FE_367ofdm_LockAlgo
--ACTION :: Search for Signal, Timing, Carrier and then data at a given Frequency,
-- in a given range:

--PARAMS IN :: NONE
--PARAMS OUT:: NONE
--RETURN :: Type of the founded signal (if any)

--REMARKS :: This function is supposed to replace FE_367ofdm_Algo according
-- to last findings on SYR block
--***************************************************/
FE_TER_SignalStatus_t
FE_367ofdm_LockAlgo(TUNER_IOREG_DeviceMap_t *pDemod_DeviceMap,
					IOARCH_Handle_t DemodIOHandle,
					FE_367ofdm_InternalParams_t *pParams)
{
	FE_TER_SignalStatus_t ret_flag;
	short int wd, tempo;
	unsigned short int try, u_var1 = 0, u_var2 = 0, u_var3 = 0, u_var4 = 0, mode, guard;
	// YW_ErrorType_T Error = YW_NO_ERROR;
	//TUNER_ScanTaskParam_T *Inst;
	IOARCH_Handle_t IOHandle = DemodIOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap = pDemod_DeviceMap;
	//Inst = TUNER_GetScanInfo(Handle);
	//IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	//DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	try = 0;
	do
	{
		ret_flag = FE_TER_LOCKOK;
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_CORE_ACTIVE, 0);
		if (pParams->IF_IQ_Mode != 0)
		{
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_COM_N, 0x07);
		}
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_GUARD, 3); /* suggested mode is 2k 1/4 */
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_MODE, 0);
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_TR_DIS, 0);
		ChipWaitOrAbort_0367ter(FALSE, 5);
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_CORE_ACTIVE, 1);
		if (FE_367TER_CheckSYR(DeviceMap, IOHandle) == FE_TER_NOSYMBOL)
		{
			return FE_TER_NOSYMBOL;
		}
		else
		{
			/* if chip locked on wrong mode first try, it must lock correctly second try *db*/
			mode = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_MODE);
			if (FE_367TER_CheckCPAMP(DeviceMap, IOHandle, mode) == FE_TER_NOCPAMP)
			{
				if (try == 0)
					{
						ret_flag = FE_TER_NOCPAMP;
					}
			}
		}
		try++;
	}
	while ((try < 2) && (ret_flag != FE_TER_LOCKOK));
	if ((mode != 0) && (mode != 1) && (mode != 2))
	{
		return FE_TER_SWNOK;
	}
	/*guard=ChipGetField(hChip,F367ofdm_SYR_GUARD); */
	/*supress EPQ auto for SYR_GARD 1/16 or 1/32 and set channel predictor in automatic */
	/* switch (guard)
	 {

	 case 0:
	 case 1:
	 ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_AUTO_LE_EN,0);
	 ChipSetOneRegister(hChip,R367ofdm_CHC_CTL, 0x01);
	 break;
	 case 2:
	 case 3:
	 ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_AUTO_LE_EN,1);
	 ChipSetOneRegister(hChip,R367ofdm_CHC_CTL, 0x11);
	 break;

	 default:
	 #if defined(HOST_PC) && !defined(NO_GUI)
	 ReportInsertMessage("SYR_GUARD uncorrectly set");
	 #endif
	 return FE_TER_SWNOK;
	 }*/
	/*reset fec an reedsolo FOR 367 only*/
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_RST_SFEC, 1);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_RST_REEDSOLO, 1);
	msleep(1);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_RST_SFEC, 0);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_RST_REEDSOLO, 0);
	u_var1 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_LK);
	u_var2 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_PRF);
	u_var3 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_LOCK);
	/* u_var4=ChipGetField(hChip,F367ofdm_TSFIFO_LINEOK); */
	wd = FE_367ofdm_duration(mode, 150, 600, 300);
	tempo = FE_367ofdm_duration(mode, 4, 16, 8);
	/*while ( ((!u_var1)||(!u_var2)||(!u_var3)||(!u_var4)) && (wd>=0)) */
	while (((!u_var1) || (!u_var2) || (!u_var3)) && (wd >= 0))
	{
		ChipWaitOrAbort_0367ter(FALSE, tempo);
		wd -= tempo;
		u_var1 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_LK);
		u_var2 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_PRF);
		u_var3 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_LOCK);
		/*u_var4=ChipGetField(hChip,F367ofdm_TSFIFO_LINEOK); */
	}
	if (!u_var1)
	{
		//printf("FE_TER_NOLOCK ================= \n");
		return FE_TER_NOLOCK;
	}
	if (!u_var2)
	{
		//printf("FE_TER_NOPRFOUND ================= \n");
		return FE_TER_NOPRFOUND;
	}
	if (!u_var3)
	{
		//printf("FE_TER_NOTPS ==============================\n");
		return FE_TER_NOTPS;
	}
	guard = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_GUARD);
	ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_CHC_CTL, 0x11);
	switch (guard)
	{
		case 0:
		case 1:
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_AUTO_LE_EN, 0);
			/*ChipSetOneRegister(hChip,R367ofdm_CHC_CTL, 0x1);*/
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_FILTER, 0);
			break;
		case 2:
		case 3:
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_AUTO_LE_EN, 1);
			/*ChipSetOneRegister(hChip,R367ofdm_CHC_CTL, 0x11);*/
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_FILTER, 1);
			break;
		default:
			return FE_TER_SWNOK;
	}
	/* apply Sfec workaround if 8K 64QAM CR!=1/2*/
	if ((ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_CONST) == 2) &&
			(mode == 1) &&
			(ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_HPCODE) != 0))
	{
		ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_SFDLYSETH, 0xc0);
		ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_SFDLYSETM, 0x60);
		ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_SFDLYSETL, 0x0);
	}
	else
	{
		ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_SFDLYSETH, 0x0);
	}
	wd = FE_367ofdm_duration(mode, 125, 500, 250);
	u_var4 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TSFIFO_LINEOK);
	while ((!u_var4) && (wd >= 0))
	{
		ChipWaitOrAbort_0367ter(FALSE, tempo);
		wd -= tempo;
		u_var4 = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TSFIFO_LINEOK);
	}
	if (!u_var4)
	{
		return FE_TER_NOLOCK;
	}
	/* for 367 leave COM_N at 0x7 for IQ_mode*/
	/*if(pParams->IF_IQ_Mode!=FE_TER_NORMAL_IF_TUNER)
	{
	 tempo=0;
	 while ( (ChipGetField(hChip,F367ofdm_COM_USEGAINTRK)!=1)&&(ChipGetField(hChip,F367ofdm_COM_AGCLOCK)!=1)&&(tempo<100))
	 {
	 ChipWaitOrAbort(hChip,1);
	 tempo+=1;
	 }
	 #if defined(HOST_PC) && !defined(NO_GUI)
	 Fmt(str_tmp,"%s<%s%i%s","AGC digital locked after ",tempo," ms");
	 ReportInsertMessage(str_tmp);
	 #endif
	 ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_COM_N,0x17);
	}*/
	/*ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_SYR_TR_DIS,1); */
	return FE_TER_LOCKOK;
}

/*****************************************************
--FUNCTION :: FE_STV0367TER_SetTS_Parallel_Serial
--ACTION :: TSOutput setting
--PARAMS IN :: hChip, PathTS
--PARAMS OUT:: NONE
--RETURN ::

--***************************************************/

void FE_STV0367ofdm_SetTS_Parallel_Serial(TUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_TS_OutputMode_t PathTS)
{
	if (DeviceMap != NULL)
	{
		ChipSetField_0367ter(DeviceMap, IOHandle, F367_TS_DIS, 0);
		switch (PathTS)
		{
			default:
			/*for removing warning :default we can assume in parallel mode*/
			case FE_TS_PARALLEL_PUNCT_CLOCK:
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_TSFIFO_SERIAL, 0);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_TSFIFO_DVBCI, 0);
				break;
			case FE_TS_SERIAL_PUNCT_CLOCK:
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_TSFIFO_SERIAL, 1);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_TSFIFO_DVBCI, 1);
				break;
		}
	}
}

/*****************************************************
--FUNCTION :: FE_STV0367TER_SetCLK_Polarity
--ACTION :: clock polarity setting
--PARAMS IN :: hChip, PathTS
--PARAMS OUT:: NONE
--RETURN ::

--***************************************************/
void FE_STV0367ofdm_SetCLK_Polarity(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, FE_TS_ClockPolarity_t clock)
{
	if (DemodDeviceMap != NULL)
	{
		switch (clock)
		{
			case FE_TS_RISINGEDGE_CLOCK:
				ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367_TS_BYTE_CLK_INV, 0);
				break;
			case FE_TS_FALLINGEDGE_CLOCK:
				ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367_TS_BYTE_CLK_INV, 1);
				break;
			/*case FE_TER_CLOCK_POLARITY_DEFAULT:*/
			default:
				ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367_TS_BYTE_CLK_INV, 0);
				break;
		}
	}
}

/*****************************************************
--FUNCTION :: FE_STV0367_SetCLKgen
--ACTION :: PLL divider setting
--PARAMS IN :: hChip, PathTS
--PARAMS OUT:: NONE
--RETURN ::

--***************************************************/
void FE_STV0367TER_SetCLKgen(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 DemodXtalFreq)
{
	if (DemodDeviceMap != NULL)
	{
		switch (DemodXtalFreq)
		{
			/*set internal freq to 53.125MHz */
			case 25000000:
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLMDIV, 0xA);
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLNDIV, 0x55);
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLSETUP, 0x18);
				break;
			case 27000000:
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLMDIV, 0x1);
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLNDIV, 0x8);
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLSETUP, 0x18);
				break;
			case 30000000:
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLMDIV, 0xc);
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLNDIV, 0x55);
				ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLSETUP, 0x18);
				break;
			default:
				break;
		}
		/*ChipSetOneRegister_0367ter(DemodDeviceMap, DemodIOHandle, R367ofdm_ANACTRL, 0); */
		/*error=hChip->Error;*/
	}
}

#if 0 //lwj remove
/*****************************************************
--FUNCTION :: FE_367TER_Init
--ACTION :: Initialisation of the STV0367 chip
--PARAMS IN :: pInit ==> pointer to FE_TER_InitParams_t structure
--PARAMS OUT:: NONE
--RETURN :: Handle to STV0367
--***************************************************/

FE_LLA_Error_t FE_367ofdm_Init(FE_TER_InitParams_t *pInit, FE_367ofdm_Handle_t *Handle)

{
	FE_367ofdm_InternalParams_t *pParams = NULL;
	Demod_InitParams_t DemodInit;
	TER_TUNER_InitParams_t TunerInitParams;
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	STCHIP_Error_t StChipError = CHIPERR_NO_ERROR ;
	TUNER_Error_t Tuner2Error = TUNER_NO_ERR;
	/* FE_OFDMEchoParams_t EchoParams; */
	/* POUR STAPI passer un multiinstance */
	/* Internal params structure allocation */
#ifdef CHIP_STAPI
	pParams = (FE_367ofdm_InternalParams_t *)(*Handle);
#endif
	/* pParams->Echo = EchoParams;*/ /* POUR STAPI passer un multiinstance */
	if (pParams != NULL)
	{
		/* Chip initialisation */
		/* in internal struct chip is stored */
		/*#ifdef HOST_PC*/
#ifndef NO_GUI
		pParams->hDemod = DEMOD;
		pParams->hTuner = TUNER;
		pParams->hTuner2 = TUNER_B;
		pParams->Crystal_Hz = EXTCLK;
		pParams->PreviousBER = 0;
#else
		STCHIP_Error_t DemodError = CHIPERR_NO_ERROR;
		TUNER_Error_t TunerError = TUNER_NO_ERR;
#ifdef CHIP_STAPI
		DemodInit.Chip = (pParams->hDemod); /*Change */
		DemodInit.NbDefVal = STV0367ofdm_NBREGS;
#else
		DemodInit.Chip = &DemodChip;
#endif
		DemodInit.Chip->RepeaterFn = NULL;
		DemodInit.Chip->RepeaterHost = NULL;
		DemodInit.Chip->Repeater = FALSE;
		DemodInit.Chip->pData = NULL;
		DemodInit.Chip->I2cAddr = pInit->DemodI2cAddr;
		strcpy((char *)DemodInit.Chip->Name, (char *)(pInit->DemodName));
		pParams->Crystal_Hz = pInit->Demod_Crystal_Hz;
		/*DemodInit.Chip->XtalFreq= pInit->Demod_Crystal_Hz; */
		/*DemodInit.Chip->IFmode= pInit->IFmode;*/
		pParams->PreviousBER = 0;
		DemodError = STV0367ofdm_Init(&DemodInit, (STCHIP_Handle_t *) &pParams->hDemod);
		if ((pParams->hDemod != NULL) && (!pParams->hDemod->Error))
		{
			/* Here we make the necessary changes to the demod's registers depending on the tuner */
			switch (pInit->TunerModel)
			{
				case TUNER_DTT7546:
					/*ChipSetOneRegister(pParams->hDemod,R367_I2CRPT,0x42);*/
					ChipSetOneRegister(pParams->hDemod, R367_ANACTRL, 0x0D); /* PLL bypassed and disabled */
					ChipSetOneRegister(pParams->hDemod, R367_PLLMDIV, 0x01); /* IC runs at 54MHz with a 27MHz crystal */
					ChipSetOneRegister(pParams->hDemod, R367_PLLNDIV, 0x08);
					ChipSetOneRegister(pParams->hDemod, R367_PLLSETUP, 0x18); /* ADC clock is equal to system clock */
					ChipSetOneRegister(pParams->hDemod, R367_TOPCTRL, 0x0); /* Buffer Q disabled */
					ChipSetOneRegister(pParams->hDemod, R367_DUAL_AD12, 0x04); /* ADCQ disabled */
					ChipSetOneRegister(pParams->hDemod, R367ofdm_GAIN_SRC1, 0x2A);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_GAIN_SRC2, 0xD6);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_INC_DEROT1, 0x55);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_INC_DEROT2, 0x55);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_TRL_CTL, 0x14);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_TRL_NOMRATE1, 0xAE);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_TRL_NOMRATE2, 0x56);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_FEPATH_CFG, 0x0);
					ChipSetOneRegister(pParams->hDemod, R367_ANACTRL, 0x00);
					break;
				case TUNER_STV4100:
					ChipSetOneRegister(pParams->hDemod, R367_TOPCTRL, 0x2); /* Buffer Q enabled */
					ChipSetOneRegister(pParams->hDemod, R367_DUAL_AD12, 0x00); /* ADCQ enabled */
					ChipSetOneRegister(pParams->hDemod, R367_ANACTRL, 0x00);
					break;
				default:
					break;
			}
			FE_STV0367TER_SetCLKgen(pParams->hDemod, pInit->Demod_Crystal_Hz);
			TunerInitParams.Model = pInit->TunerModel;
			TunerInitParams.TunerName = pInit->TunerName;
			strcpy((char *)TunerInitParams.TunerName, pInit->TunerName);
			TunerInitParams.TunerI2cAddress = pInit->TunerI2cAddr;
			TunerInitParams.Fxtal_Hz = 0;
			TunerInitParams.IF = pInit->TunerIF_kHz;
			TunerInitParams.Fxtal_Hz = pInit->Tuner_Crystal_Hz;
			TunerInitParams.Repeater = TRUE;
			TunerInitParams.RepeaterHost = pParams->hDemod;
			TunerInitParams.RepeaterFn = (STCHIP_RepeaterFn_t)STV367ofdm_RepeaterFn;
			TunerInitParams.TunerTechno = TUNER_DVB_T;
			pParams->DemodModel = DEMOD_STV0367DVBT;
			TunerError = FE_TunerInit(&TunerInitParams, &(pParams->hTuner));
			if ((pInit->TunerModel == TUNER_FQD1116) || (pInit->TunerModel == TUNER_DNOS40AS))
			{
				TunerInitParams.Model = TUNER_TDA9898;
				TunerInitParams.TunerName = "TDA9898";
				strcpy((char *)TunerInitParams.TunerName, "TDA9898");
				TunerInitParams.TunerI2cAddress = pInit->TunerI2cAddr2;
				TunerInitParams.RepeaterHost = pParams->hDemod;
				TunerInitParams.RepeaterFn = (STCHIP_RepeaterFn_t)STV367ofdm_RepeaterFn;
				pParams->DemodModel = DEMOD_STV0367DVBT;
				Tuner2Error = FE_TunerInit(&TunerInitParams, &(pParams->hTuner2));
			}
			if (DemodError == CHIPERR_NO_ERROR)
			{
				/* If no Error on the demod then check the Tuners */
				if (TunerError == TUNER_INVALID_HANDLE)
					return FE_LLA_INVALID_HANDLE; /* if tuner type error */
				else if (TunerError == TUNER_TYPE_ERR)
					return FE_LLA_BAD_PARAMETER; /* if tuner NULL Handle error = FE_LLA_INVALIDE_HANDLE */
				else if (TunerError != TUNER_NO_ERR)
					return FE_LLA_I2C_ERROR;
			}
			else
			{
				if (DemodError == CHIPERR_INVALID_HANDLE)
					return FE_LLA_INVALID_HANDLE;
				else
					return FE_LLA_I2C_ERROR;
			}
			/* pParams->hTuner = FE_TunerInit(pInit->TunerModel,pInit->TunerName,pInit->TunerI2cAddr,pInit->TunerIF_kHz,pParams->hDemod,(STCHIP_RepeaterFn_t)STV362_RepeaterFn); */
		} /*if((pParams->hDemod != NULL) && (!pParams->hDemod->Error))*/
		else
		{
			return FE_367ofdm_Term(Handle);
		}
#endif
		/*#endif*/
#if defined(CHIP_STAPI) || defined(NO_GUI)
		FE_STV0367ofdm_SetTS_Parallel_Serial(pParams->hDemod, pInit->PathTSClock); /*Set TS1 and TS2 to serial or parallel mode */
		FE_STV0367ofdm_SetCLK_Polarity(pParams->hDemod, pInit->ClockPolarity); /*Set TS1 and TS2 to serial or parallel mode */
#endif
		if (pParams->hDemod != NULL)
		{
			/*check I2c errors*/
			StChipError = ChipGetRegisters(pParams->hDemod, R367_ID, 1);
			if (StChipError == CHIPERR_NO_ERROR)
				error = FE_LLA_NO_ERROR ;
			else if (StChipError == CHIPERR_I2C_NO_ACK)
				error = FE_LLA_I2C_ERROR;
			else if (StChipError == CHIPERR_I2C_BURST)
				error = FE_LLA_I2C_ERROR;
			else if (StChipError == CHIPERR_INVALID_HANDLE)
				error = FE_LLA_INVALID_HANDLE;
			else error = FE_LLA_BAD_PARAMETER;
			if (error == FE_LLA_NO_ERROR)
			{
				/*Read IC cut ID*/
				pParams->hDemod->ChipID = ChipGetOneRegister(pParams->hDemod, R367_ID);
				pParams->I2Cspeed = FE_367TER_SpeedInit(pParams->hDemod);
				pParams->first_lock = 0;
				pParams->Unlockcounter = 2;
			}
			else
			{
#ifdef HOST_PC
				free(pParams);
#endif
				pParams = NULL;
			}
		} /*if(pParams->hDemod != NULL)*/
	}
	ChipSetField(pParams->hDemod, F367_STDBY_ADCGP, 1);
	ChipSetField(pParams->hDemod, F367_STDBY_ADCGP, 0);
	return error;
}
#endif
#if 0 //lwj remove
/*****************************************************
--FUNCTION :: FE_367ofdm_CableToTerrestrial
--ACTION :: switch from Cable config to terr. config
--PARAMS IN :: Handle ==> Front End Handle
--PARAMS OUT:: NONE
--RETURN :: Error (if any)
--***************************************************/
FE_LLA_Error_t FE_367ofdm_SwitchDemodToDVBT(FE_367ofdm_Handle_t Handle)

{
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	TUNER_Params_t hTunerParams = NULL;
	FE_367ofdm_InternalParams_t *pParams;
	if ((void *)Handle != NULL)
	{
		pParams = (FE_367ofdm_InternalParams_t *) Handle;
		if (pParams != NULL)
		{
			//if ( ChipGetField(pParams->hDemod,F367_QAM_COFDM))
			//{
			if ((pParams->hTuner->pData != NULL) && (pParams->hDemod != NULL) && (pParams->hTuner != NULL))
			{
#if defined(HOST_PC) && !defined(NO_GUI)
				pParams->hDemod = DEMOD; /* use current selected 367qam */
				pParams->Crystal_Hz = EXTCLK;
#endif
				/*set the values from demod init inside generic tuners*/
				ChipSetOneRegister(pParams->hDemod, R367_I2CRPT, 0x22);
				ChipSetOneRegister(pParams->hDemod, R367_TOPCTRL, 0x02);
				ChipSetOneRegister(pParams->hDemod, R367_IOCFG0, 0x40);
				ChipSetOneRegister(pParams->hDemod, R367_DAC0R, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_IOCFG1, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_DAC1R, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_IOCFG2, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_SDFR, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_AUX_CLK, 0x0a);
				ChipSetOneRegister(pParams->hDemod, R367_FREESYS1, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_FREESYS2, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_FREESYS3, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_GPIO_CFG, 0x55);
				ChipSetOneRegister(pParams->hDemod, R367_GPIO_CMD, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_TSTRES, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_ANACTRL, 0x00); /*was d caution PLL stopped, to be restarted at init!!!*/
				ChipSetOneRegister(pParams->hDemod, R367_TSTBUS, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_RF_AGC1, 0xff);
				ChipSetOneRegister(pParams->hDemod, R367_RF_AGC2, 0x83);
				ChipSetOneRegister(pParams->hDemod, R367_ANADIGCTRL, 0x19);
				ChipSetOneRegister(pParams->hDemod, R367_PLLMDIV, 0x0c);
				ChipSetOneRegister(pParams->hDemod, R367_PLLNDIV, 0x55);
				ChipSetOneRegister(pParams->hDemod, R367_PLLSETUP, 0x18);
				ChipSetOneRegister(pParams->hDemod, R367_DUAL_AD12, 0x00);
				ChipSetOneRegister(pParams->hDemod, R367_TSTBIST, 0x00);
				hTunerParams = (TUNER_Params_t) pParams->hTuner->pData;
				switch (hTunerParams->Model)
				{
					case TUNER_DTT7546:
						pParams->DemodModel = DEMOD_STV0367DVBT;
						/*set here the necessary registers to switch from cable to ter*/
						ChipSetOneRegister(pParams->hDemod, R367_I2CRPT, 0x42);
						ChipSetOneRegister(pParams->hDemod, R367_ANADIGCTRL, 0x09); /* Buffer Q disabled */
						ChipSetOneRegister(pParams->hDemod, R367_PLLMDIV, 0x01); /* IC runs at 54MHz with a 27MHz crystal */
						ChipSetOneRegister(pParams->hDemod, R367_PLLNDIV, 0x08);
						ChipSetOneRegister(pParams->hDemod, R367_PLLSETUP, 0x18); /* ADC clock is equal to system clock */
						ChipSetOneRegister(pParams->hDemod, R367_TOPCTRL, 0x0); /* Buffer Q disabled */
						ChipSetOneRegister(pParams->hDemod, R367_DUAL_AD12, 0x04); /* ADCQ disabled */
						ChipSetOneRegister(pParams->hDemod, R367ofdm_GAIN_SRC1, 0x2A);
						ChipSetOneRegister(pParams->hDemod, R367ofdm_GAIN_SRC2, 0xD6);
						ChipSetOneRegister(pParams->hDemod, R367ofdm_INC_DEROT1, 0x55);
						ChipSetOneRegister(pParams->hDemod, R367ofdm_INC_DEROT2, 0x55);
						ChipSetOneRegister(pParams->hDemod, R367ofdm_TRL_CTL, 0x2c);
						ChipSetOneRegister(pParams->hDemod, R367ofdm_TRL_NOMRATE1, 0xAE);
						ChipSetOneRegister(pParams->hDemod, R367ofdm_TRL_NOMRATE2, 0x56);
						ChipSetOneRegister(pParams->hDemod, R367ofdm_FEPATH_CFG, 0x0);
						ChipSetOneRegister(pParams->hDemod, R367_ANACTRL, 0x00);
						break;
					case TUNER_FQD1116:
						pParams->DemodModel = DEMOD_STV0367DVBT;
						/*set here the necessary registers to switch from cable to ter*/
						break;
					case TUNER_STV4100:
						pParams->DemodModel = DEMOD_STV0367DVBT;
						/*set here the necessary registers to switch from cable to ter*/
						break;
					default:
						break;
				}
				/*set tuner to terrestrial conf*/
				error = FE_SwitchTunerToDVBT(pParams->hTuner);
			}
			else
				error = FE_LLA_BAD_PARAMETER; /* tuner not allowed in ter mode*/
		}
		else
			error = FE_LLA_INVALID_HANDLE;
		// }/* if ( ChipGetField(pParams->hDemod,F367_QAM_COFDM)) */
	}
	else
		error = FE_LLA_INVALID_HANDLE;
	return error;
}
/*****************************************************
--FUNCTION :: FE_367TER_Algo
--ACTION :: Search for a valid channel
--PARAMS IN :: Handle ==> Front End Handle
 pSearch ==> Search parameters
 pResult ==> Result of the search
--PARAMS OUT:: NONE
--RETURN :: Error (if any)
--***************************************************/
FE_LLA_Error_t FE_367ofdm_Algo(FE_367ofdm_Handle_t Handle, FE_TER_SearchParams_t *pSearch, FE_TER_SearchResult_t *pResult)/*,STTUNER_tuner_instance_t *TunerInstance) */

{
	FE_367ofdm_InternalParams_t *pIntParams;
	TER_TUNER_Params_t hTunerParams = NULL;
	int offset = 0, tempo = 0, AgcIF = 0;
	unsigned short int u_var; /*added for HM *db*/
	U8 constell, counter, tps_rcvd[2];
	S8 step;
	/*BOOL SpectrumInversion=FALSE;*/
	S32 timing_offset = 0;
	U32 trl_nomrate = 0, intX, InternalFreq = 0, temp = 0;
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	if ((void *)Handle != NULL)
	{
		pIntParams = (FE_367ofdm_InternalParams_t *) Handle;
		if (pIntParams != NULL)
		{
			hTunerParams = (TUNER_Params_t) pIntParams->hTuner->pData;
			/* Fill pParams structure with search parameters */
			pIntParams->Frequency = pSearch->Frequency;
			pIntParams->Mode = pSearch->Mode;
			pIntParams->Guard = pSearch->Guard;
			pIntParams->Inv = pSearch->Inv;
			pIntParams->Force = pSearch->Force + ChipGetField(pIntParams->hDemod, F367ofdm_FORCE) * 2;
			pIntParams->ChannelBW = pSearch->ChannelBW;
			pIntParams->IF_IQ_Mode = pSearch->IF_IQ_Mode;
			ChipSetField(pIntParams->hDemod, F367ofdm_CCS_ENABLE, 0);
			switch (pIntParams->IF_IQ_Mode)
			{
				case FE_TER_NORMAL_IF_TUNER: /* Normal IF mode */
					ChipSetField(pIntParams->hDemod, F367_TUNER_BB, 0);
					ChipSetField(pIntParams->hDemod, F367ofdm_LONGPATH_IF, 0);
					ChipSetField(pIntParams->hDemod, F367ofdm_DEMUX_SWAP, 0);
					break;
				case FE_TER_LONGPATH_IF_TUNER: /* Long IF mode */
					ChipSetField(pIntParams->hDemod, F367_TUNER_BB, 0);
					ChipSetField(pIntParams->hDemod, F367ofdm_LONGPATH_IF, 1);
					ChipSetField(pIntParams->hDemod, F367ofdm_DEMUX_SWAP, 1);
					break;
				case FE_TER_IQ_TUNER: /* IQ mode */
					ChipSetField(pIntParams->hDemod, F367_TUNER_BB, 1);
					ChipSetField(pIntParams->hDemod, F367ofdm_PPM_INVSEL, 0); /*spectrum inversion hw detection off *db*/
					break;
				default:
					return FE_LLA_SEARCH_FAILED;
			}
			if ((pIntParams->Inv == FE_TER_INVERSION_NONE))
			{
				if (pIntParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
				{
					ChipSetField(pIntParams->hDemod, F367ofdm_IQ_INVERT, 0);
					pIntParams->SpectrumInversion = FALSE;
				}
				else
				{
					ChipSetField(pIntParams->hDemod, F367ofdm_INV_SPECTR, 0);
					pIntParams->SpectrumInversion = FALSE;
				}
			}
			else if (pIntParams->Inv == FE_TER_INVERSION)
			{
				if (pIntParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
				{
					ChipSetField(pIntParams->hDemod, F367ofdm_IQ_INVERT, 1);
					pIntParams->SpectrumInversion = TRUE;
				}
				else
				{
					ChipSetField(pIntParams->hDemod, F367ofdm_INV_SPECTR, 1);
					pIntParams->SpectrumInversion = TRUE;
				}
			}
			else if ((pIntParams->Inv == FE_TER_INVERSION_AUTO) || ((pIntParams->Inv == FE_TER_INVERSION_UNK)/*&& (!pParams->first_lock)*/))
			{
				if (pIntParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
				{
					if (pIntParams->Sense == 1)
					{
						ChipSetField(pIntParams->hDemod, F367ofdm_IQ_INVERT, 1);
						pIntParams->SpectrumInversion = TRUE;
					}
					else
					{
						ChipSetField(pIntParams->hDemod, F367ofdm_IQ_INVERT, 0);
						pIntParams->SpectrumInversion = FALSE;
					}
				}
				else
				{
					if (pIntParams->Sense == 1)
					{
						ChipSetField(pIntParams->hDemod, F367ofdm_INV_SPECTR, 1);
						pIntParams->SpectrumInversion = TRUE;
					}
					else
					{
						ChipSetField(pIntParams->hDemod, F367ofdm_INV_SPECTR, 0);
						pIntParams->SpectrumInversion = FALSE;
					}
				}
			}
			if ((pIntParams->IF_IQ_Mode != FE_TER_NORMAL_IF_TUNER) && (pIntParams->PreviousChannelBW != pIntParams->ChannelBW))
			{
				FE_367TER_AGC_IIR_LOCK_DETECTOR_SET(pIntParams->hDemod);
				/*set fine agc target to 180 for LPIF or IQ mode*/
				/* set Q_AGCTarget */
				ChipSetField(pIntParams->hDemod, F367ofdm_SEL_IQNTAR, 1);
				ChipSetField(pIntParams->hDemod, F367ofdm_AUT_AGC_TARGET_MSB, 0xB);
				/*ChipSetField(pIntParams->hDemod,AUT_AGC_TARGET_LSB,0x04); */
				/* set Q_AGCTarget */
				ChipSetField(pIntParams->hDemod, F367ofdm_SEL_IQNTAR, 0);
				ChipSetField(pIntParams->hDemod, F367ofdm_AUT_AGC_TARGET_MSB, 0xB);
				/*ChipSetField(pIntParams->hDemod,AUT_AGC_TARGET_LSB,0x04); */
				if (!FE_367TER_IIR_FILTER_INIT(pIntParams->hDemod, pIntParams->ChannelBW, pIntParams->Crystal_Hz))
					return FE_LLA_BAD_PARAMETER;
				/*set IIR filter once for 6,7 or 8MHz BW*/
				pIntParams->PreviousChannelBW = pIntParams->ChannelBW;
				FE_367TER_AGC_IIR_RESET(pIntParams->hDemod);
			}
			/*********Code Added For Hierarchical Modulation****************/
			if (pIntParams->Hierarchy == FE_TER_HIER_LOW_PRIO)
			{
				/*printf(" I am in Hierarchy low priority\n");*/
				ChipSetField(pIntParams->hDemod, F367ofdm_BDI_LPSEL, 0x01);
			}
			else
			{
				/*printf(" I am in Hierarchy high priority\n");*/
				ChipSetField(pIntParams->hDemod, F367ofdm_BDI_LPSEL, 0x00);
			}
			/*************************/
			InternalFreq = FE_367ofdm_GetMclkFreq(pIntParams->hDemod, pIntParams->Crystal_Hz) / 1000;
			temp = (int)((((pIntParams->ChannelBW * 64 * PowOf2(15) * 100) / (InternalFreq)) * 10) / 7);
			ChipSetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LSB, temp % 2);
			temp = temp / 2;
			ChipSetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_HI, temp / 256);
			ChipSetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LO, temp % 256);
			ChipSetRegisters(pIntParams->hDemod, R367ofdm_TRL_NOMRATE1, 2);
			ChipGetRegisters(pIntParams->hDemod, R367ofdm_TRL_CTL, 3);
			temp = ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_HI) * 512 + ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LO) * 2 +
				   ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LSB) ;
			temp = (int)((PowOf2(17) * pIntParams->ChannelBW * 1000) / (7 * (InternalFreq)));
			ChipSetFieldImage(pIntParams->hDemod, F367ofdm_GAIN_SRC_HI, temp / 256);
			ChipSetFieldImage(pIntParams->hDemod, F367ofdm_GAIN_SRC_LO, temp % 256);
			ChipSetRegisters(pIntParams->hDemod, R367ofdm_GAIN_SRC1, 2);
			ChipGetRegisters(pIntParams->hDemod, R367ofdm_GAIN_SRC1, 2);
			temp = ChipGetFieldImage(pIntParams->hDemod, F367ofdm_GAIN_SRC_HI) * 256 + ChipGetFieldImage(pIntParams->hDemod, F367ofdm_GAIN_SRC_LO);
			temp = (int)((InternalFreq - hTunerParams->IF) * PowOf2(16) / (InternalFreq));
			ChipSetFieldImage(pIntParams->hDemod, F367ofdm_INC_DEROT_HI, temp / 256);
			ChipSetFieldImage(pIntParams->hDemod, F367ofdm_INC_DEROT_LO, temp % 256);
			ChipSetRegisters(pIntParams->hDemod, R367ofdm_INC_DEROT1, 2);
			pIntParams->EchoPos = pSearch->EchoPos;
			ChipSetField(pIntParams->hDemod, F367ofdm_LONG_ECHO, pIntParams->EchoPos);
			if (hTunerParams->Model == TUNER_STV4100)
			{
				ChipSetOneRegister(pIntParams->hDemod, R367ofdm_AGC_TARG, 0x16);
				error = FE_TunerStartAndCalibrate(pIntParams->hTuner);
				if (error != ST_NO_ERROR)
				{
					return error;
				}
				error = FE_TunerSetBandWidth(pIntParams->hTuner, pIntParams->ChannelBW);
				if (error != ST_NO_ERROR)
				{
					return error;
				}
				error = FE_TunerSetFrequency(pIntParams->hTuner, pIntParams->Frequency + 100);
				if (error != ST_NO_ERROR)
				{
					return error;
				}
				pIntParams->Frequency = FE_TunerGetFrequency(pIntParams->hTuner);
				ChipWaitOrAbort(pIntParams->hDemod, 66);
				AgcIF = ChipGetField(pIntParams->hDemod, F367ofdm_AGC2_VAL_LO) +
						(ChipGetField(pIntParams->hDemod, F367ofdm_AGC2_VAL_HI) << 8);
				intX = (ChipGetField(pIntParams->hDemod, F367ofdm_INT_X3) << 24) +
					   (ChipGetField(pIntParams->hDemod, F367ofdm_INT_X2) << 16) +
					   (ChipGetField(pIntParams->hDemod, F367ofdm_INT_X1) << 8) +
					   ChipGetField(pIntParams->hDemod, F367ofdm_INT_X0) ;
				if ((AgcIF > 0x500) && (intX > 0x50000) && (AgcIF < 0xc00))
				{
					error = FE_TunerSetLna(pIntParams->hTuner);
					if (error != ST_NO_ERROR)
					{
						return error;
					}
				}
				else
				{
					error = FE_TunerAdjustRfPower(pIntParams->hTuner, 0);
					if (error != ST_NO_ERROR)
					{
						return error;
					}
				}
				/* printf("estimated power=%03d\n",
				 FE_TunerEstimateRfPower(pIntParams->hTuner,ChipGetField(pIntParams->hDemod,F362_RF_AGC1_LEVEL_HI),AgcIF) ); */
				/* core active */
				ChipSetField(pIntParams->hDemod, F367ofdm_NRST_IIR, 0); /*reset filter to avoid saturation*/
				/* ChipSetField(pIntParams->hDemod,F367ofdm_CORE_ACTIVE,0);*/
				/* ChipWaitOrAbort(pIntParams->hDemod,20);*/
				ChipSetField(pIntParams->hDemod, F367ofdm_NRST_IIR, 1);
				/* ChipSetField(pIntParams->hDemod,F367ofdm_CORE_ACTIVE,1);*/
			}
			else
			{
				error = FE_TunerSetFrequency(pIntParams->hTuner, pIntParams->Frequency);
				if (error != ST_NO_ERROR)
				{
					return error;
				}
				pIntParams->Frequency = FE_TunerGetFrequency(pIntParams->hTuner);
			}
			/*********************************/
			if (FE_367ofdm_LockAlgo(pIntParams) == FE_TER_LOCKOK)
			{
				pResult->Locked = TRUE;
				pResult->SignalStatus = FE_TER_LOCKOK;
				/* update results */
				/*********** dans search term auparavant **********/
				tps_rcvd[0] = ChipGetOneRegister(pIntParams->hDemod, R367ofdm_TPS_RCVD2);
				tps_rcvd[1] = ChipGetOneRegister(pIntParams->hDemod, R367ofdm_TPS_RCVD3);
				ChipGetRegisters(pIntParams->hDemod, R367ofdm_SYR_STAT, 1);
				pResult->Mode = ChipGetFieldImage(pIntParams->hDemod, F367ofdm_SYR_MODE);
				pResult->Guard = ChipGetFieldImage(pIntParams->hDemod, F367ofdm_SYR_GUARD);
				constell = ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TPS_CONST);
				if (constell == 0) pResult->Modulation = FE_TER_MOD_QPSK;
				else if (constell == 1) pResult->Modulation = FE_TER_MOD_16QAM;
				else pResult->Modulation = FE_TER_MOD_64QAM;
				/***Code replced and changed for HM**/
				pResult->hier = pIntParams->Hierarchy;
				pResult->Hierarchy_Alpha = (tps_rcvd[0] & 0x70) >> 4;
				/****/
				pResult->HPRate = tps_rcvd[1] & 0x07;
				pResult->LPRate = (tps_rcvd[1] & 0x70) >> 4;
				/****/
				constell = ChipGetField(pIntParams->hDemod, F367ofdm_PR);
				if (constell == 5) constell = 4;
				pResult->pr = (FE_TER_Rate_t) constell;
				if (pIntParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
				{
					pResult->Sense = ChipGetField(pIntParams->hDemod, F367ofdm_IQ_INVERT);
				}
				else
				{
					pResult->Sense = ChipGetField(pIntParams->hDemod, F367ofdm_INV_SPECTR);
				}
				/* dcdc modifs per Gilles*/
				pIntParams->first_lock = 1;
				/* modifs Tuner */
				ChipGetRegisters(pIntParams->hDemod, R367ofdm_AGC2MAX, 13);
				pResult->Agc_val = (ChipGetFieldImage(pIntParams->hDemod, F367ofdm_AGC1_VAL_LO) << 16) +
								   (ChipGetFieldImage(pIntParams->hDemod, F367ofdm_AGC1_VAL_HI) << 24) +
								   ChipGetFieldImage(pIntParams->hDemod, F367ofdm_AGC2_VAL_LO) +
								   (ChipGetFieldImage(pIntParams->hDemod, F367ofdm_AGC2_VAL_HI) << 8);
				/* Carrier offset calculation */
				ChipSetField(pIntParams->hDemod, F367ofdm_FREEZE, 1);
				ChipGetRegisters(pIntParams->hDemod, R367ofdm_CRL_FREQ1, 3);
				ChipSetField(pIntParams->hDemod, F367ofdm_FREEZE, 0);
				offset = (ChipGetFieldImage(pIntParams->hDemod, F367ofdm_CRL_FOFFSET_VHI) << 16) ;
				offset += (ChipGetFieldImage(pIntParams->hDemod, F367ofdm_CRL_FOFFSET_HI) << 8);
				offset += (ChipGetFieldImage(pIntParams->hDemod, F367ofdm_CRL_FOFFSET_LO));
				if (offset > 8388607)
					offset -= 16777216;
				offset = offset * 2 / 16384;
				if (pResult->Mode == FE_TER_MODE_2K)
				{
					offset = (offset * 4464) / 1000; /*** 1 FFT BIN=4.464khz***/
				}
				else if (pResult->Mode == FE_TER_MODE_4K)
				{
					offset = (offset * 223) / 100; /*** 1 FFT BIN=2.23khz***/
				}
				else if (pResult->Mode == FE_TER_MODE_8K)
				{
					offset = (offset * 111) / 100; /*** 1 FFT BIN=1.1khz***/
				}
				if (ChipGetField(pIntParams->hDemod, F367ofdm_PPM_INVSEL) == 1) /* inversion hard auto */
				{
					if (((ChipGetField(pIntParams->hDemod, F367ofdm_INV_SPECTR) != ChipGetField(pIntParams->hDemod, F367ofdm_STATUS_INV_SPECRUM)) == 1))
					{
						/* no inversion nothing to do*/
					}
					else
					{
						offset = offset * -1;
					}
				}
				else /* manual inversion*/
				{
					if (((!pIntParams->SpectrumInversion) && (hTunerParams->SpectrInvert == TUNER_IQ_NORMAL))
							|| ((pIntParams->SpectrumInversion) && (hTunerParams->SpectrInvert == TUNER_IQ_INVERT)))
						offset = offset * -1;
				}
				if (pIntParams->ChannelBW == 6)
					offset = (offset * 6) / 8;
				else if (pIntParams->ChannelBW == 7)
					offset = (offset * 7) / 8;
				pIntParams->Frequency += offset;
				pResult->Frequency = pIntParams->Frequency;
				pResult->ResidualOffset = offset;
				pResult->Echo_pos = ChipGetField(pIntParams->hDemod, F367ofdm_LONG_ECHO);
				/*For FEC rate return to application*/
				/* Get the FEC Rate */
				if (pResult->hier == FE_TER_HIER_LOW_PRIO)
				{
					pResult->FECRates = ChipGetField(pIntParams->hDemod, F367ofdm_TPS_LPCODE);
				}
				else
				{
					pResult->FECRates = ChipGetField(pIntParams->hDemod, F367ofdm_TPS_HPCODE);
				}
				switch (pResult->FECRates)
				{
					case 0:
						pResult->FECRates = FE_TER_FEC_1_2;
						break;
					case 1:
						pResult->FECRates = FE_TER_FEC_2_3;
						break;
					case 2:
						pResult->FECRates = FE_TER_FEC_3_4;
						break;
					case 3:
						pResult->FECRates = FE_TER_FEC_5_6;
						break;
					case 4:
						pResult->FECRates = FE_TER_FEC_7_8;
						break;
					default:
						break; /* error */
				}
				/*WAIT_N_MS(200) ; */
				tempo = 10; /* exit even if timing_offset stays null *db* */
				while ((timing_offset == 0) && (tempo > 0))
				{
					ChipWaitOrAbort(pIntParams->hDemod, 10); /*was 20ms */
					/* fine tuning of timing offset if required */
					ChipGetRegisters(pIntParams->hDemod, R367ofdm_TRL_CTL, 5);
					timing_offset = ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_TOFFSET_LO) + 256 * ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_TOFFSET_HI);
					if (timing_offset >= 32768) timing_offset -= 65536;
					trl_nomrate = (512 * ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_HI) + ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LO) * 2 + ChipGetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LSB));
					timing_offset = ((signed)(1000000 / trl_nomrate) * timing_offset) / 2048;
					tempo--;
				}
				if (timing_offset <= 0)
				{
					timing_offset = (timing_offset - 11) / 22;
					step = -1;
				}
				else
				{
					timing_offset = (timing_offset + 11) / 22;
					step = 1;
				}
				for (counter = 0; counter < abs(timing_offset); counter++)
				{
					trl_nomrate += step;
					ChipSetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LSB, trl_nomrate % 2);
					ChipSetFieldImage(pIntParams->hDemod, F367ofdm_TRL_NOMRATE_LO, trl_nomrate / 2);
					ChipSetRegisters(pIntParams->hDemod, R367ofdm_TRL_CTL, 2);
					ChipWaitOrAbort(pIntParams->hDemod, 1);
				}
				ChipWaitOrAbort(pIntParams->hDemod, 5);
				/* unlocks could happen in case of trl centring big step, then a core off/on restarts demod */
				u_var = ChipGetField(pIntParams->hDemod, F367ofdm_LK);
				if (!u_var)
				{
					ChipSetField(pIntParams->hDemod, F367ofdm_CORE_ACTIVE, 0);
					ChipWaitOrAbort(pIntParams->hDemod, 20);
					ChipSetField(pIntParams->hDemod, F367ofdm_CORE_ACTIVE, 1);
					ChipWaitOrAbort(pIntParams->hDemod, 350);
				}
			} /* if(FE_367_Algo(pParams) == FE_TER_LOCKOK) */
			else
			{
				pResult->Locked = FALSE;
				error = FE_LLA_SEARCH_FAILED;
			}
		}
		else
		{
			error = FE_LLA_BAD_PARAMETER;
		}
	} /*if((void *)Handle != NULL)*/
	else
	{
		error = FE_LLA_BAD_PARAMETER;
	}
	return error;
}
/*****************************************************
--FUNCTION :: FE_367TER_Search
--ACTION :: Intermediate layer before launching Search
--PARAMS IN :: Handle ==> Front End Handle
 pSearch ==> Search parameters
 pResult ==> Result of the search
--PARAMS OUT:: NONE
--RETURN :: Error (if any)
--***************************************************/
FE_LLA_Error_t FE_367ofdm_Search(FE_367ofdm_Handle_t Handle, FE_TER_SearchParams_t *pSearch, FE_TER_SearchResult_t *pResult)/*,STTUNER_tuner_instance_t *TunerInstance)*/
{
	FE_TER_SearchParams_t pLook;
	/*U8 trials[2]; */
	S8 num_trials, index;
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	U8 flag_spec_inv;
	U8 flag;
	U8 SenseTrials[2];
	U8 SenseTrialsAuto[2];
	FE_367ofdm_InternalParams_t *pParams;
	/*to remove warning on OS20*/
	SenseTrials[0] = INV;
	SenseTrials[1] = NINV;
	SenseTrialsAuto[0] = INV;
	SenseTrialsAuto[1] = NINV;
	if ((void *) Handle != NULL)
	{
		pParams = (FE_367ofdm_InternalParams_t *) Handle;
		/* frequency = pSearch->Frequency;*/
		pLook.Frequency = pSearch->Frequency;
		pLook.Mode = pSearch->Mode;
		pLook.Guard = pSearch->Guard;
		pLook.Inv = pSearch->Inv;
		pLook.Force = pSearch->Force;
		pLook.ChannelBW = pSearch->ChannelBW;
		pLook.EchoPos = pSearch->EchoPos;
		pLook.IF_IQ_Mode = pSearch->IF_IQ_Mode;
		pParams->Inv = pSearch->Inv;
		pLook.Hierarchy = pParams->Hierarchy = pSearch->Hierarchy; /*added for hierarchical*/
		/*printf(" in FE_367TER_Search () value of pParams.Hierarchy %d\n",pParams.Hierarchy );*/
		flag_spec_inv = 0;
		flag = ((pSearch->Inv >> 1) & 1);
		switch (flag) /* sw spectrum inversion for LP_IF &IQ &normal IF *db* */
		{
			case 0: /*INV+ INV_NONE*/
				if ((pParams->Inv == FE_TER_INVERSION_NONE) || (pParams->Inv == FE_TER_INVERSION))
				{
					num_trials = 1;
				}
				else /*UNK */
					num_trials = 2;
				break;
			case 1:/*AUTO*/
				num_trials = 2;
				if ((pParams->first_lock) && (pParams->Inv == FE_TER_INVERSION_AUTO))
				{
					num_trials = 1;
				}
				break;
			default:
				return FE_TER_NOLOCK;
				break;
		}
		pResult->SignalStatus = FE_TER_NOLOCK;
		index = 0;
		while (((index) < num_trials) && (pResult->SignalStatus != FE_TER_LOCKOK))
		{
			if (!pParams->first_lock)
			{
				if (pParams->Inv == FE_TER_INVERSION_UNK)
				{
					pParams->Sense = SenseTrials[index];
				}
				if (pParams->Inv == FE_TER_INVERSION_AUTO)
				{
					pParams->Sense = SenseTrialsAuto[index];
				}
			}
			error = FE_367ofdm_Algo(Handle, &pLook, pResult);
			if ((pResult->SignalStatus == FE_TER_LOCKOK) && (pParams->Inv == FE_TER_INVERSION_AUTO) && (index == 1))
			{
				SenseTrialsAuto[index] = SenseTrialsAuto[0]; /* invert spectrum sense */
				SenseTrialsAuto[(index + 1) % 2] = (SenseTrialsAuto[1] + 1) % 2;
			}
			index++;
		}
	}
	return error;
}

/*****************************************************
--FUNCTION :: FE_STV0367TER_SetStandby
--ACTION :: Set demod STANDBY mode On/Off
--PARAMS IN :: Handle ==> Front End Handle

-PARAMS OUT:: NONE.
--RETURN :: Error (if any)
--***************************************************/
FE_LLA_Error_t FE_STV0367ofdm_SetStandby(FE_367ofdm_Handle_t Handle, U8 StandbyOn)
{
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	FE_367ofdm_InternalParams_t *pParams = (FE_367ofdm_InternalParams_t *)Handle;
	TER_TUNER_Params_t hTunerParams = NULL;
	if (pParams != NULL)
	{
		hTunerParams = (TER_TUNER_Params_t) pParams->hTuner->pData;
		if ((pParams->hDemod->Error) || (pParams->hTuner->Error))
		{
			error = FE_LLA_I2C_ERROR;
		}
		else
		{
			if (StandbyOn)
			{
				if (hTunerParams->Model == TUNER_STV4100)
				{
					error = FE_TunerSetStandbyMode(pParams->hTuner, StandbyOn);
				}
				ChipSetField(pParams->hDemod, F367_STDBY, 1);
				ChipSetField(pParams->hDemod, F367_STDBY_FEC, 1);
				ChipSetField(pParams->hDemod, F367_STDBY_CORE, 1);
			}
			else
			{
				ChipSetField(pParams->hDemod, F367_STDBY, 0);
				ChipSetField(pParams->hDemod, F367_STDBY_FEC, 0);
				ChipSetField(pParams->hDemod, F367_STDBY_CORE, 0);
				if (hTunerParams->Model == TUNER_STV4100)
				{
					error = FE_TunerSetStandbyMode(pParams->hTuner, StandbyOn);
				}
			}
			if ((pParams->hDemod->Error) || (pParams->hTuner->Error))
			{
				error = FE_LLA_I2C_ERROR;
			}
		}
	}
	else
		error = FE_LLA_INVALID_HANDLE;
	return (error);
}
#endif //lwj remove

int FE_STV0367TER_GetPower(TUNER_IOREG_DeviceMap_t *DeviceMap,
						   IOARCH_Handle_t IOHandle)
{
	S32 power = 0;
	//S32 snr = 0;
	U16 if_agc = 0;
	U16 rf_agc = 0;
	{
		U8 i = 0;
		if_agc = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_LO) +
				 (ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_HI) << 8);
		rf_agc = ChipGetField_0367ter(DeviceMap, IOHandle, F367_RF_AGC1_LEVEL_HI);
		if (rf_agc < 255)
		{
			for (i = 0; i < RF_LOOKUP_TABLE_SIZE_3; i++)
			{
				if (rf_agc <= FE_TUNER_RF_LookUp3[i])
				{
					/*-33dBm to -43dBm*/
					power = -33 + (-1) * i;
					break;
				}
			}
			if (i == RF_LOOKUP_TABLE_SIZE_3) power = -44;
		}
		else if (rf_agc == 255)
		{
			for (i = 0; i < IF_LOOKUP_TABLE_SIZE_4; i++)
			{
				if (if_agc <= FE_TUNER_IF_LookUp4[i])
				{
					/*-44dBm to -86dBm*/
					power = -44 + (-1) * i;
					break;
				}
			}
			if (i == IF_LOOKUP_TABLE_SIZE_4) power = -88;
		}
		else
			/*should never pass here*/
			power = -90 ;
	}
	power = power + 100;
	//power = 100-power;
	if (power > 100)
	{
		power = 100;
	}
	if (power < 0)
	{
		power = 0;
	}
	return power * 255 * 255 / 100;
}

int FE_STV0367TER_GetSnr(TUNER_IOREG_DeviceMap_t *DeviceMap,
						 IOARCH_Handle_t IOHandle)
{
	S32 snr = 0;
	/*For quality return 1000*snr to application SS*/
	/* /4 for STv0367, /8 for STv0362 */
	snr = (1000 * ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_CHCSNR)) / 8;
	/**pNoise = (snr*10) >> 3;*/
	/** fix done here for the bug GNBvd20972 where pNoise is calculated in right percentage **/
	/*pInfo->CN_dBx10=(((snr*1000)/8)/32)/10; */
	snr = (snr / 32) / 10 ;
	if (snr > 100)
	{
		snr = 100;
	}
	return snr * 255 * 255 / 100;
}

#if 1

int FE_STV0367TER_GetSignalInfo(TUNER_IOREG_DeviceMap_t *pDemod_DeviceMap,
								IOARCH_Handle_t DemodIOHandle,
								U32 *CN_dBx10, U32 *Power_dBmx10, U32 *Ber)
{
	//TUNER_ScanTaskParam_T *Inst;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	//S32 snr = 0;
	*Power_dBmx10 = 0;
	*CN_dBx10 = 0;
	*Ber = 0;
	//Inst = TUNER_GetScanInfo(Handle);
	IOHandle = DemodIOHandle;
	DeviceMap = pDemod_DeviceMap;
	/*FE_367TER_GetErrors(pParams, &Errors, &bits, &PackErrRate,&BitErrRate); */
	/*MeasureBER(pParams,0,1,&BitErrRate,&PackErrRate) ;*/
	*Ber = FE_367ofdm_GetErrors(DeviceMap, IOHandle);
	*CN_dBx10 = FE_STV0367TER_GetSnr(DeviceMap, IOHandle);
#if 0
	if (Inst->DriverParam.Ter.TunerType == TUNER_TUNER_STV4100)
	{
		if_agc = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_LO) +
				 (ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_HI) << 8);
		if (Inst->DriverParam.Ter.TunerDriver.tuner_GetRF_Level != NULL)
		{
			power = Inst->DriverParam.Ter.TunerDriver.tuner_GetRF_Level(Handle, ChipGetField_0367ter(DeviceMap, IOHandle, F367_RF_AGC1_LEVEL_HI), if_agc);
		}
	} //YWDRIVER_MODI lwj remove this has some problem
	else //lwj add for other tuner not stv4100 but not sure if this is right
#endif
		*Power_dBmx10 = FE_STV0367TER_GetPower(DeviceMap, IOHandle);
	return 0;
}

/*****************************************************
--FUNCTION :: FE_367TER_GetSignalInfo
--ACTION :: Return informations on the locked channel
--PARAMS IN :: Handle ==> Front End Handle
--PARAMS OUT:: pInfo ==> Informations (BER,C/N,power ...)
--RETURN :: Error (if any)
--***************************************************/
typedef enum
{
	TUNER_IQ_NORMAL = 1,
	TUNER_IQ_INVERT = -1
} TUNER_TER_IQ_t;

FE_LLA_Error_t FE_367ofdm_GetSignalInfo(TUNER_IOREG_DeviceMap_t *pDemod_DeviceMap,
										IOARCH_Handle_t DemodIOHandle,
										FE_367ofdm_InternalParams_t *pParams,
										FE_TER_SignalInfo_t *pInfo)
{
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	int offset = 0;
	FE_TER_FECRate_t CurFECRates; /*Added for HM*/
	FE_TER_Hierarchy_Alpha_t CurHierMode;/*Added for HM*/
	/*U32 BitErrRate, PackErrRate, Errors, bits; */
	int constell = 0, snr = 0, Data = 0;
	//U16 if_agc=0;
	//TUNER_ScanTaskParam_T *Inst;
	IOARCH_Handle_t IOHandle = DemodIOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap = pDemod_DeviceMap;
	TUNER_TER_IQ_t SpectrInvert = TUNER_IQ_NORMAL; //lwj add question
	//Inst = TUNER_GetScanInfo(Handle);
	//IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	//DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	/*There is no need to check LOCK status again, already checked in FE_362_Status*/
	/*pInfo->Locked=FE_362_Status(Handle);*/
#if 0
	if (Inst->DriverParam.Ter.TunerDriver.tuner_GetFreq != NULL)
	{
		pInfo->Frequency = Inst->DriverParam.Ter.TunerDriver.tuner_GetFreq(Handle, NULL);
	}
#endif
	constell = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_CONST);
	if (constell == 0) pInfo->Modulation = FE_TER_MOD_QPSK;
	else if (constell == 1) pInfo->Modulation = FE_TER_MOD_16QAM;
	else pInfo->Modulation = FE_TER_MOD_64QAM;
	if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
	{
		pInfo->SpectInversion = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT);
	}
	else
	{
		pInfo->SpectInversion = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR);
	}
	/* Get the Hierarchical Mode */
	Data = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_HIERMODE);
	switch (Data)
	{
		case 0 :
			CurHierMode = FE_TER_HIER_ALPHA_NONE;
			break;
		case 1 :
			CurHierMode = FE_TER_HIER_ALPHA_1;
			break;
		case 2 :
			CurHierMode = FE_TER_HIER_ALPHA_2;
			break;
		case 3 :
			CurHierMode = FE_TER_HIER_ALPHA_4;
			break;
		default :
			CurHierMode = Data;
			error = YWHAL_ERROR_BAD_PARAMETER;
			break; /* error */
	}
	/* Get the FEC Rate */
	if (pParams->Hierarchy == FE_TER_HIER_LOW_PRIO)
	{
		Data = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_LPCODE);
	}
	else
	{
		Data = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_HPCODE);
	}
	switch (Data)
	{
		case 0:
			CurFECRates = FE_TER_FEC_1_2;
			break;
		case 1:
			CurFECRates = FE_TER_FEC_2_3;
			break;
		case 2:
			CurFECRates = FE_TER_FEC_3_4;
			break;
		case 3:
			CurFECRates = FE_TER_FEC_5_6;
			break;
		case 4:
			CurFECRates = FE_TER_FEC_7_8;
			break;
		default:
			CurFECRates = Data;
			error = YWHAL_ERROR_BAD_PARAMETER;
			break; /* error */
	}
	/**** end of HM addition ******/
	pInfo->pr = CurFECRates;
	pInfo->Hierarchy_Alpha = CurHierMode;
	pInfo->Hierarchy = pParams->Hierarchy;
	pInfo->Mode = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_MODE);
	pInfo->Guard = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_GUARD);
	/* Carrier offset calculation */
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_FREEZE, 1);
// ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_CRL_FREQ1, 3);
	ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_CRL_FREQ1, 1);
	ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_CRL_FREQ2, 1);
	ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_CRL_FREQ3, 1);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_FREEZE, 0);
	offset = (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_CRL_FOFFSET_VHI) << 16) ;
	offset += (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_CRL_FOFFSET_HI) << 8);
	offset += (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_CRL_FOFFSET_LO));
	if (offset > 8388607)
		offset -= 16777216;
	offset = offset * 2 / 16384;
	if (pInfo->Mode == FE_TER_MODE_2K)
	{
		offset = (offset * 4464) / 1000; /*** 1 FFT BIN=4.464khz***/
	}
	else if (pInfo->Mode == FE_TER_MODE_4K)
	{
		offset = (offset * 223) / 100; /*** 1 FFT BIN=2.23khz***/
	}
	else if (pInfo->Mode == FE_TER_MODE_8K)
	{
		offset = (offset * 111) / 100; /*** 1 FFT BIN=1.1khz***/
	}
	if (ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_PPM_INVSEL) == 1) /* inversion hard auto */
	{
		if (((ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR) != ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_STATUS_INV_SPECRUM)) == 1))
		{
			/* no inversion nothing to do*/
		}
		else
		{
			offset = offset * -1;
		}
	}
	else /* manual inversion*/
	{
		if (((!pParams->SpectrumInversion) && (SpectrInvert == TUNER_IQ_NORMAL))
				|| ((pParams->SpectrumInversion) && (SpectrInvert == TUNER_IQ_INVERT)))
			offset = offset * -1;
	}
	if (pParams->ChannelBW == 6)
		offset = (offset * 6) / 8;
	else if (pParams->ChannelBW == 7)
		offset = (offset * 7) / 8;
	pInfo->ResidualOffset = offset;
	/*FE_367TER_GetErrors(pParams, &Errors, &bits, &PackErrRate,&BitErrRate); */
	/*MeasureBER(pParams,0,1,&BitErrRate,&PackErrRate) ;*/
	pInfo->BER = FE_367ofdm_GetErrors(DeviceMap, IOHandle);
	/*For quality return 1000*snr to application SS*/
	/* /4 for STv0367, /8 for STv0362 */
	snr = (1000 * ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_CHCSNR)) / 8;
	/**pNoise = (snr*10) >> 3;*/
	/** fix done here for the bug GNBvd20972 where pNoise is calculated in right percentage **/
	/*pInfo->CN_dBx10=(((snr*1000)/8)/32)/10; */
	pInfo->CN_dBx10 = (snr / 32) / 10 ;
#if 0
	if (Inst->DriverParam.Ter.TunerType == TUNER_TUNER_STV4100)
	{
		if_agc = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_LO) +
				 (ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_HI) << 8);
		if (Inst->DriverParam.Ter.TunerDriver.tuner_GetRF_Level != NULL)
		{
			pInfo->Power_dBmx10 = Inst->DriverParam.Ter.TunerDriver.tuner_GetRF_Level(Handle, ChipGetField_0367ter(DeviceMap, IOHandle, F367_RF_AGC1_LEVEL_HI), if_agc);
		}
	}
#endif
	return error;
}
#endif
#if 0
/*****************************************************
--FUNCTION :: FE_367TER_Term
--ACTION :: Terminate STV0367 chip connection
--PARAMS IN :: Handle ==> Front End Handle
--PARAMS OUT:: NONE
--RETURN :: Error (if any)
--***************************************************/
FE_LLA_Error_t FE_367ofdm_Term(FE_367ofdm_Handle_t Handle)
{
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	FE_367ofdm_InternalParams_t *pParams = NULL;
	pParams = (FE_367ofdm_InternalParams_t *) Handle;
	if (pParams != NULL)
	{
#ifdef HOST_PC
#ifdef NO_GUI
		if (pParams->hTuner2 != NULL)
			FE_TunerTerm(pParams->hTuner2);
		if (pParams->hTuner != NULL)
			FE_TunerTerm(pParams->hTuner);
		if (pParams->hDemod != NULL)
			ChipClose(pParams->hDemod);
		FE_TunerTerm(pParams->hTuner);
#endif
		if (Handle)
			free(pParams);
#endif
	}
	else
		error = FE_LLA_INVALID_HANDLE;
	return error;
}
#endif
/*****************************************************

**FUNCTION :: FE_367qam_GetMclkFreq

**ACTION :: Set the STV0367QAM master clock frequency

**PARAMS IN :: hChip ==> handle to the chip

** ExtClk_Hz ==> External clock frequency (Hz)

**PARAMS OUT:: NONE

**RETURN :: MasterClock frequency (Hz)

*****************************************************/

U32 FE_367ofdm_GetMclkFreq(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, U32 ExtClk_Hz)

{
	U32 mclk_Hz = 0; /* master clock frequency (Hz) */
	U32 M, N, P;
	if (ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367_BYPASS_PLLXN) == 0)
	{
		//ChipGetRegisters_0367ter(DemodDeviceMap,DemodIOHandle,R367_PLLMDIV,3); //lwj change
		ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLMDIV, 1);
		ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLNDIV, 1);
		ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367_PLLSETUP, 1);
		N = (U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367_PLL_NDIV);
		if (N == 0)
		{
			N = N + 1;
		}
		M = (U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367_PLL_MDIV);
		if (M == 0)
		{
			M = M + 1;
		}
		P = (U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367_PLL_PDIV);
		if (P > 5)
		{
			P = 5;
		}
		mclk_Hz = ((ExtClk_Hz / 2) * N) / (M * PowOf2(P));
	}
	else
		mclk_Hz = ExtClk_Hz;
	return mclk_Hz;
}

#if 0
/*****************************************************
**FUNCTION :: STV367TER_RepeaterFn
**ACTION :: Set the I2C repeater
**PARAMS IN :: handle to the chip
** state of the repeater
**PARAMS OUT:: Error
**RETURN :: NONE
*****************************************************/
#ifndef STFRONTEND_FORCE_STI2C_DEPRECATED
STCHIP_Error_t STV367ofdm_RepeaterFn(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, BOOL State, U8 *Buffer)
#else
STCHIP_Error_t STV367ofdm_RepeaterFn(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle, BOOL State)
#endif
{
	STCHIP_Error_t error = CHIPERR_NO_ERROR;
	if (hChip != NULL)
	{
		/*ChipSetField_0367ter(DeviceMap, IOHandle,F367_STOP_ENABLE,1);*/
		if (State == TRUE)
		{
#ifndef STFRONTEND_FORCE_STI2C_DEPRECATED
			ChipFillRepeaterMessage(hChip, F367_I2CT_ON, 1, Buffer);
#else
			ChipSetField_0367ter(DeviceMap, IOHandle, F367_I2CT_ON, 1);
#endif
		}
		else
		{
#ifndef STFRONTEND_FORCE_STI2C_DEPRECATED
			ChipFillRepeaterMessage(hChip, F367_I2CT_ON, 0, Buffer);
#else
			ChipSetField_0367ter(DeviceMap, IOHandle, F367_I2CT_ON, 0);
#endif
		}
	}
	return error;
}
#endif
/*STCHIP_Error_t STV367ofdm_RepeaterFn(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle,BOOL State)
{
 STCHIP_Error_t error = CHIPERR_NO_ERROR;

 if(hChip != NULL)
 {
 ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_STOP_ENABLE,1);
 if(State == TRUE)
 {
 ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_I2CT_ON,1);
 }
 else
 {
 ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_I2CT_ON,0);
 }
 }

 return error; */
BOOL FE_367ofdm_lock(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle)
{
	BOOL Locked = FALSE;
	Locked = ((ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_TPS_LOCK) |
			   ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_PRF) |
			   ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_LK) |
			   ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_TSFIFO_LINEOK)));
	return Locked;
}
#if 0 //question
/*****************************************************
**FUNCTION :: FE_367TER_Status
**ACTION :: checking demod lock in tracking
**PARAMS IN :: handle to the chip
**PARAMS OUT:: NONE
**RETURN :: Locked
*****************************************************/
BOOL FE_367ofdm_Status(FE_367ofdm_Handle_t Handle)
{
	FE_367ofdm_InternalParams_t *pParams = NULL;
	FE_TER_SearchParams_t Search;
	FE_TER_SearchResult_t SearchResult;
	FE_LLA_Error_t error = FE_LLA_SEARCH_FAILED;
	BOOL Locked = FALSE;
	pParams = (FE_367ofdm_InternalParams_t *) Handle;
	if (pParams != NULL)
	{
		Locked = ((ChipGetField(pParams->hDemod, F367ofdm_TPS_LOCK) && ChipGetField(pParams->hDemod, F367ofdm_PRF) &&
				   ChipGetField(pParams->hDemod, F367ofdm_LK) && ChipGetField(pParams->hDemod, F367ofdm_TSFIFO_LINEOK)));
		if (!Locked)
		{
			/*check if AGC is clamped ie antenna disconnected*/
			if (ChipGetField(pParams->hDemod, F367ofdm_AGC2_VAL_HI) != 0xf)
			{
				if (pParams->Unlockcounter > 2)
				{
					/*ind = FE_TunerGetFrequency(pParams->hTuner);*/
					Search.Frequency = pParams->Frequency;
					Search.IF_IQ_Mode = pParams->IF_IQ_Mode;
					Search.Mode = pParams->Mode;
					Search.Guard = pParams->Guard;
					Search.Force = pParams->Force;
					Search.Hierarchy = pParams->Hierarchy;
					Search.Inv = pParams->Inv;
					Search.Force = pParams->Force;
					Search.ChannelBW = pParams->ChannelBW;
					Search.EchoPos = pParams->EchoPos;
					/*FE_367ofdm_Init(&Init,&Handle);*/
					error = FE_367ofdm_Search(Handle, &Search, &SearchResult); /* Launch the search algorithm */
					if (error == FE_LLA_NO_ERROR)
						Locked = TRUE;
					else
						Locked = FALSE;
					/*FE_367ofdm_Term(Handle); */
					pParams->Unlockcounter = 0;
				}
				else
				{
					pParams->Unlockcounter++;
				}
			}
			else
			{
				pParams->Unlockcounter ++;
			}
		}
		else
		{
			pParams->Unlockcounter = 0;
			/* apply Sfec workaround if 8K 64QAM CR!=1/2*/
			if ((ChipGetField(pParams->hDemod, F367ofdm_TPS_CONST) == 2) && (ChipGetField(pParams->hDemod, F367ofdm_SYR_MODE) == 1) &&
					(ChipGetField(pParams->hDemod, F367ofdm_TPS_HPCODE) != 0))
			{
				if (ChipGetOneRegister(pParams->hDemod, R367ofdm_SFDLYSETH) != 0xc0)
				{
					ChipSetOneRegister(pParams->hDemod, R367ofdm_SFDLYSETH, 0xc0);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_SFDLYSETM, 0x60);
					ChipSetOneRegister(pParams->hDemod, R367ofdm_SFDLYSETL, 0x0);
				}
			}
			else
			{
				ChipSetOneRegister(pParams->hDemod, R367ofdm_SFDLYSETH, 0x0);
			}
		}
	}
	else
		Locked = FALSE;
	return Locked;
}
#endif //lwj remove
/*****************************************************
**FUNCTION :: FE_367TER_GetLLARevision
**ACTION :: return LLARevision
**PARAMS IN :: NONE
**PARAMS OUT:: NONE
**RETURN :: Revision367
*****************************************************/

const char *FE_367TER_GetLLARevision(void)
{
	return (Revision367ofdm);
}

/*****************************************************
**FUNCTION :: GetBerErrors
**ACTION :: calculating ber using fec1
**PARAMS IN :: pParams ==> pointer to FE_TER_InternalParams_t structure
**PARAMS OUT:: NONE
**RETURN :: 10e7*Ber
*****************************************************/
U32 FE_367ofdm_GetBerErrors(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle) /*FEC1*/
{
	U32 Errors = 0, Ber = 0, temporary = 0;
	int abc = 0, def = 0, cpt = 0, max_count = 0;
	//if(pParams != NULL)
	{
		ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SFEC_ERR_SOURCE, 0x07);
		abc = ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SFEC_ERR_SOURCE);
		/* set max timeout regarding num events*/
		def = ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SFEC_NUM_EVENT);
		if (def == 2)
			max_count = 15;
		else if (def == 3)
			max_count = 100;
		else max_count = 500;
		/*wait for counting completion*/
		while (((ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SFERRC_OLDVALUE) == 1) && (cpt < max_count)) ||
				((Errors == 0) && (cpt < max_count)))
		{
			msleep(1);
			ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367ofdm_SFERRCTRL, 4);
			Errors =
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SFEC_ERR_CNT) * PowOf2(16)) +
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SFEC_ERR_CNT_HI) * PowOf2(8)) +
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_SFEC_ERR_CNT_LO));
			cpt++;
		}
		if (Errors == 0)
			Ber = 0;
		else if (abc == 0x7)
		{
			if (Errors <= 4)
			{
				temporary = (Errors * 1000000000) / (8 * PowOf2(14));
				temporary = temporary;
			}
			else if (Errors <= 42)
			{
				temporary = (Errors * 100000000) / (8 * PowOf2(14));
				temporary = temporary * 10;
			}
			else if (Errors <= 429)
			{
				temporary = (Errors * 10000000) / (8 * PowOf2(14));
				temporary = temporary * 100;
			}
			else if (Errors <= 4294)
			{
				temporary = (Errors * 1000000) / (8 * PowOf2(14));
				temporary = temporary * 1000;
			}
			else if (Errors <= 42949)
			{
				temporary = (Errors * 100000) / (8 * PowOf2(14));
				temporary = temporary * 10000;
			}
			else if (Errors <= 429496)
			{
				temporary = (Errors * 10000) / (8 * PowOf2(14));
				temporary = temporary * 100000;
			}
			else /*if (Errors<4294967) 2^22 max error*/
			{
				temporary = (Errors * 1000) / (8 * PowOf2(14));
				temporary = temporary * 100000; /* still to *10 */
			}
			/* Byte error*/
			if (def == 2)
				/*Ber=Errors/(8*pow(2,14));;*/
				Ber = temporary;
			else if (def == 3)
				/*Ber=Errors/(8*pow(2,16));*/
				Ber = temporary / 4;
			else if (def == 4)
				/*Ber=Errors/(8*pow(2,18));*/
				Ber = temporary / 16;
			else if (def == 5)
				/*Ber=Errors/(8*pow(2,20));*/
				Ber = temporary / 64;
			else if (def == 6)
				/*Ber=Errors/(8*pow(2,22));*/
				Ber = temporary / 256;
			else
				/* should not pass here*/
				Ber = 0;
			if ((Errors < 4294967) && (Errors > 429496))
				Ber /= 10;
			else
				Ber /= 100;
		}
		/* save actual value*/
		/*pParams->PreviousBER=Ber;*/
		/*measurement not completed, load previous value
		else Ber= pParams->PreviousBER;*/
	}
	return Ber;
}

/*****************************************************
**FUNCTION :: GetPerErrors
**ACTION :: counting packet errors using fec1
**PARAMS IN :: pParams ==> pointer to FE_TER_InternalParams_t structure
**PARAMS OUT:: NONE
**RETURN :: 10e9*Per
*****************************************************/

U32 FE_367ofdm_GetPerErrors(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle) /*Fec2*/
{
	U32 Errors = 0, Per = 0, temporary = 0;
	int abc = 0, def = 0, cpt = 0;
	if (DemodDeviceMap != NULL)
	{
		ChipSetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_SRC1, 0x09);
		abc = ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_SRC1);
		def = ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_NUM_EVT1);
		while (((ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERRCNT1_OLDVALUE) == 1) && (cpt < 400)) ||
				((Errors == 0) && (cpt < 400)))
		{
			ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367ofdm_ERRCTRL1, 4);
			msleep(1);
			Errors =
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_CNT1) * PowOf2(16)) +
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_CNT1_HI) * PowOf2(8)) +
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_CNT1_LO));
			cpt++;
		}
		if (Errors == 0)
			Per = 0;
		else if (abc == 0x9)
		{
			if (Errors <= 4)
			{
				temporary = (Errors * 1000000000) / (8 * PowOf2(8));
				temporary = temporary;
			}
			else if (Errors <= 42)
			{
				temporary = (Errors * 100000000) / (8 * PowOf2(8));
				temporary = temporary * 10;
			}
			else if (Errors <= 429)
			{
				temporary = (Errors * 10000000) / (8 * PowOf2(8));
				temporary = temporary * 100;
			}
			else if (Errors <= 4294)
			{
				temporary = (Errors * 1000000) / (8 * PowOf2(8));
				temporary = temporary * 1000;
			}
			else if (Errors <= 42949)
			{
				temporary = (Errors * 100000) / (8 * PowOf2(8));
				temporary = temporary * 10000;
			}
			else /*if(Errors<=429496) 2^16 errors max*/
			{
				temporary = (Errors * 10000) / (8 * PowOf2(8));
				temporary = temporary * 100000;
			}
			/* pkt error*/
			if (def == 2)
				/*Per=Errors/PowOf2(8);*/
				Per = temporary;
			else if (def == 3)
				/*Per=Errors/PowOf2(10);*/
				Per = temporary / 4;
			else if (def == 4)
				/*Per=Errors/PowOf2(12);*/
				Per = temporary / 16;
			else if (def == 5)
				/*Per=Errors/PowOf2(14);*/
				Per = temporary / 64;
			else if (def == 6)
				/*Per=Errors/PowOf2(16);*/
				Per = temporary / 256;
			else
				Per = 0;
			/* divide by 100 to get PER*10^7 */
			Per /= 100;
		}
		/* save actual value */
		//pParams->PreviousPER=Per;
#if 0
	}
	/*measurement not completed, load previous value*/
	else Ber = pParams->PreviousBER;
#endif
}
return Per;
}

/*****************************************************
**FUNCTION :: GetErrors
**ACTION :: counting packet errors using fec1
**PARAMS IN :: pParams ==> pointer to FE_TER_InternalParams_t structure
**PARAMS OUT:: NONE
**RETURN :: error nb
*****************************************************/

U32 FE_367ofdm_GetErrors(TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle) /*Fec2*/
{
	U32 Errors = 0;
	int cpt = 0;
	if (DemodDeviceMap != NULL)
	{
		/*wait for counting completion*/
		while (((ChipGetField_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERRCNT1_OLDVALUE) == 1) && (cpt < 400)) ||
				((Errors == 0) && (cpt < 400)))
		{
			//ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle,R367ofdm_ERRCTRL1,4); //lwj change 否则容易超时，当CPU负担重的时候
			ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367ofdm_ERRCTRL1, 1);
			ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367ofdm_ERRCNT1H, 1);
			ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367ofdm_ERRCNT1M, 1);
			ChipGetRegisters_0367ter(DemodDeviceMap, DemodIOHandle, R367ofdm_ERRCNT1L, 1);
			msleep(1);
			Errors =
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_CNT1) * PowOf2(16)) +
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_CNT1_HI) * PowOf2(8)) +
				((U32)ChipGetFieldImage_0367ter(DemodDeviceMap, DemodIOHandle, F367ofdm_ERR_CNT1_LO));
			cpt++;
		}
	}
	return Errors;
}

#if 0 //lwj remove
/*****************************************************
--FUNCTION :: FE_367TER_SetAbortFlag
--ACTION :: Set Abort flag On/Off
--PARAMS IN :: Handle ==> Front End Handle

-PARAMS OUT:: NONE.
--RETURN :: Error (if any)

--***************************************************/
FE_LLA_Error_t FE_367ofdm_SetAbortFlag(FE_367ofdm_Handle_t Handle, BOOL Abort)
{
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	FE_367ofdm_InternalParams_t *pParams = (FE_367ofdm_InternalParams_t *)Handle;
	if (pParams != NULL)
	{
		if ((pParams->hDemod->Error) || (pParams->hTuner->Error))
			error = FE_LLA_I2C_ERROR;
		else
		{
			ChipAbort(pParams->hTuner, Abort);
			ChipAbort(pParams->hDemod, Abort);
			if ((pParams->hDemod->Error) || (pParams->hTuner->Error)) /*Check the error at the end of the function*/
				error = FE_LLA_I2C_ERROR;
		}
	}
	else
		error = FE_LLA_INVALID_HANDLE;
	return (error);
}

void demod_get_pd(void *dummy_handle, unsigned short *level, TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle)
{
	*level = ChipGetField(hTuner, F367_RF_AGC1_LEVEL_HI);
	*level = *level << 2;
	*level |= (ChipGetField(hTuner, F367_RF_AGC1_LEVEL_LO) & 0x03);
}

void demod_get_agc(void *dummy_handle, U16 *rf_agc, U16 *bb_agc, TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle)
{
	U16 rf_low, rf_high, bb_low, bb_high;
	rf_low = ChipGetOneRegister(hTuner, R367ofdm_AGC1VAL1);
	rf_high = ChipGetOneRegister(hTuner, R367ofdm_AGC1VAL2);
	rf_high <<= 8;
	rf_low &= 0x00ff;
	*rf_agc = (rf_high + rf_low) << 4;
	bb_low = ChipGetOneRegister(hTuner, R367ofdm_AGC2VAL1);
	bb_high = ChipGetOneRegister(hTuner, R367ofdm_AGC2VAL2);
	bb_high <<= 8;
	bb_low &= 0x00ff;
	*bb_agc = (bb_high + bb_low) << 4;
}

U16 bbagc_min_start = 0xffff;

void demod_set_agclim(void *dummy_handle, U16 dir_up, TUNER_IOREG_DeviceMap_t *DemodDeviceMap, IOARCH_Handle_t DemodIOHandle)
{
	U8 agc_min = 0;
	agc_min = ChipGetOneRegister(hTuner, R367ofdm_AGC2MIN);
	if (bbagc_min_start == 0xffff)
		bbagc_min_start = agc_min;
	if (dir_up)
	{
		if ((agc_min >= bbagc_min_start) && (agc_min <= (0xa4 - 0x04)))
			agc_min += 0x04;
	}
	else
	{
		if ((agc_min >= (bbagc_min_start + 0x04)) && (agc_min <= 0xa4))
			agc_min -= 0x04;
	}
	ChipSetOneRegister(hTuner, R367ofdm_AGC2MIN, agc_min);
}
#endif

/*eof---------------------------------------------------------------------------------*/
