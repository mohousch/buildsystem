/**********************************文件头部注释************************************/
//
//
// Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名： d0376_qam.c
//
// 创建者： lwj
//
// 创建时间： 2010-12-28
//
// 文件描述：
//
// 修改记录： 日 期 作 者 版本 修定
//				 --------- --------- ----- -----
// 2010-12-28 lwj 1.0
/**********************************************************************************/

/* Includes ---------------------------------------------------------------- */

#define TUNER_ST
#define TUNER_USE_CAB_STI7167CAB

#include <linux/kernel.h> /* Kernel support */
#include <linux/string.h>

#include "D3501.h"

#include "ywtuner_ext.h"
#include "tuner_def.h"

#include "ioarch.h"
#include "ioreg.h"
#include "tuner_interface.h"

#include "stv0367qam_init.h"
#include "chip_0367qam.h"
#include "stv0367qam_drv.h"
#include "d0367_qam.h"

extern int debug_fe7162;
#define _DEBUG if (debug_fe7162)

STCHIP_Register_t Def367qamVal[STV0367qam_NBREGS] =
{
	{ R367qam_ID, 0x60 }, /* ID */
	{ R367qam_I2CRPT, 0x22 }, /* I2CRPT */
	{ R367qam_TOPCTRL, 0x10 }, /* TOPCTRL */
	{ R367qam_IOCFG0, 0x80 }, /* IOCFG0 */
	{ R367qam_DAC0R, 0x00 }, /* DAC0R */
	{ R367qam_IOCFG1, 0x00 }, /* IOCFG1 */
	{ R367qam_DAC1R, 0x00 }, /* DAC1R */
	{ R367qam_IOCFG2, 0x00 }, /* IOCFG2 */
	{ R367qam_SDFR, 0x00 }, /* SDFR */
	{ R367qam_AUX_CLK, 0x00 }, /* AUX_CLK */
	{ R367qam_FREESYS1, 0x00 }, /* FREESYS1 */
	{ R367qam_FREESYS2, 0x00 }, /* FREESYS2 */
	{ R367qam_FREESYS3, 0x00 }, /* FREESYS3 */
	{ R367qam_GPIO_CFG, 0x55 }, /* GPIO_CFG */
	{ R367qam_GPIO_CMD, 0x01 }, /* GPIO_CMD */
	{ R367qam_TSTRES, 0x00 }, /* TSTRES */
	{ R367qam_ANACTRL, 0x00 }, /* ANACTRL */
	{ R367qam_TSTBUS, 0x00 }, /* TSTBUS */
	{ R367qam_RF_AGC1, 0xea }, /* RF_AGC1 */
	{ R367qam_RF_AGC2, 0x82 }, /* RF_AGC2 */
	{ R367qam_ANADIGCTRL, 0x0b }, /* ANADIGCTRL */
	{ R367qam_PLLMDIV, 0x01 }, /* PLLMDIV */
	{ R367qam_PLLNDIV, 0x08 }, /* PLLNDIV */
	{ R367qam_PLLSETUP, 0x18 }, /* PLLSETUP */
	{ R367qam_DUAL_AD12, 0x04 }, /* DUAL_AD12 */
	{ R367qam_TSTBIST, 0x00 }, /* TSTBIST */
	{ R367qam_CTRL_1, 0x00 }, /* CTRL_1 */
	{ R367qam_CTRL_2, 0x03 }, /* CTRL_2 */
	{ R367qam_IT_STATUS1, 0x2b }, /* IT_STATUS1 */
	{ R367qam_IT_STATUS2, 0x08 }, /* IT_STATUS2 */
	{ R367qam_IT_EN1, 0x00 }, /* IT_EN1 */
	{ R367qam_IT_EN2, 0x00 }, /* IT_EN2 */
	{ R367qam_CTRL_STATUS, 0x04 }, /* CTRL_STATUS */
	{ R367qam_TEST_CTL, 0x00 }, /* TEST_CTL */
	{ R367qam_AGC_CTL, 0x73 }, /* AGC_CTL */
	{ R367qam_AGC_IF_CFG, 0x50 }, /* AGC_IF_CFG */
	{ R367qam_AGC_RF_CFG, 0x00 }, /* AGC_RF_CFG */
	{ R367qam_AGC_PWM_CFG, 0x03 }, /* AGC_PWM_CFG */
	{ R367qam_AGC_PWR_REF_L, 0x5a }, /* AGC_PWR_REF_L */
	{ R367qam_AGC_PWR_REF_H, 0x00 }, /* AGC_PWR_REF_H */
	{ R367qam_AGC_RF_TH_L, 0xff }, /* AGC_RF_TH_L */
	{ R367qam_AGC_RF_TH_H, 0x07 }, /* AGC_RF_TH_H */
	{ R367qam_AGC_IF_LTH_L, 0x00 }, /* AGC_IF_LTH_L */
	{ R367qam_AGC_IF_LTH_H, 0x08 }, /* AGC_IF_LTH_H */
	{ R367qam_AGC_IF_HTH_L, 0xff }, /* AGC_IF_HTH_L */
	{ R367qam_AGC_IF_HTH_H, 0x07 }, /* AGC_IF_HTH_H */
	{ R367qam_AGC_PWR_RD_L, 0xa0 }, /* AGC_PWR_RD_L */
	{ R367qam_AGC_PWR_RD_M, 0xe9 }, /* AGC_PWR_RD_M */
	{ R367qam_AGC_PWR_RD_H, 0x03 }, /* AGC_PWR_RD_H */
	{ R367qam_AGC_PWM_IFCMD_L, 0xe4 }, /* AGC_PWM_IFCMD_L */
	{ R367qam_AGC_PWM_IFCMD_H, 0x00 }, /* AGC_PWM_IFCMD_H */
	{ R367qam_AGC_PWM_RFCMD_L, 0xff }, /* AGC_PWM_RFCMD_L */
	{ R367qam_AGC_PWM_RFCMD_H, 0x07 }, /* AGC_PWM_RFCMD_H */
	{ R367qam_IQDEM_CFG, 0x01 }, /* IQDEM_CFG */
	{ R367qam_MIX_NCO_LL, 0x22 }, /* MIX_NCO_LL */
	{ R367qam_MIX_NCO_HL, 0x96 }, /* MIX_NCO_HL */
	{ R367qam_MIX_NCO_HH, 0x55 }, /* MIX_NCO_HH */
	{ R367qam_SRC_NCO_LL, 0xff }, /* SRC_NCO_LL */
	{ R367qam_SRC_NCO_LH, 0x0c }, /* SRC_NCO_LH */
	{ R367qam_SRC_NCO_HL, 0xf5 }, /* SRC_NCO_HL */
	{ R367qam_SRC_NCO_HH, 0x20 }, /* SRC_NCO_HH */
	{ R367qam_IQDEM_GAIN_SRC_L, 0x06 }, /* IQDEM_GAIN_SRC_L */
	{ R367qam_IQDEM_GAIN_SRC_H, 0x01 }, /* IQDEM_GAIN_SRC_H */
	{ R367qam_IQDEM_DCRM_CFG_LL, 0xfe }, /* IQDEM_DCRM_CFG_LL */
	{ R367qam_IQDEM_DCRM_CFG_LH, 0xff }, /* IQDEM_DCRM_CFG_LH */
	{ R367qam_IQDEM_DCRM_CFG_HL, 0x0f }, /* IQDEM_DCRM_CFG_HL */
	{ R367qam_IQDEM_DCRM_CFG_HH, 0x00 }, /* IQDEM_DCRM_CFG_HH */
	{ R367qam_IQDEM_ADJ_COEFF0, 0x34 }, /* IQDEM_ADJ_COEFF0 */
	{ R367qam_IQDEM_ADJ_COEFF1, 0xae }, /* IQDEM_ADJ_COEFF1 */
	{ R367qam_IQDEM_ADJ_COEFF2, 0x46 }, /* IQDEM_ADJ_COEFF2 */
	{ R367qam_IQDEM_ADJ_COEFF3, 0x77 }, /* IQDEM_ADJ_COEFF3 */
	{ R367qam_IQDEM_ADJ_COEFF4, 0x96 }, /* IQDEM_ADJ_COEFF4 */
	{ R367qam_IQDEM_ADJ_COEFF5, 0x69 }, /* IQDEM_ADJ_COEFF5 */
	{ R367qam_IQDEM_ADJ_COEFF6, 0xc7 }, /* IQDEM_ADJ_COEFF6 */
	{ R367qam_IQDEM_ADJ_COEFF7, 0x01 }, /* IQDEM_ADJ_COEFF7 */
	{ R367qam_IQDEM_ADJ_EN, 0x04 }, /* IQDEM_ADJ_EN */
	{ R367qam_IQDEM_ADJ_AGC_REF, 0x94 }, /* IQDEM_ADJ_AGC_REF */
	{ R367qam_ALLPASSFILT1, 0xc9 }, /* ALLPASSFILT1 */
	{ R367qam_ALLPASSFILT2, 0x2d }, /* ALLPASSFILT2 */
	{ R367qam_ALLPASSFILT3, 0xa3 }, /* ALLPASSFILT3 */
	{ R367qam_ALLPASSFILT4, 0xfb }, /* ALLPASSFILT4 */
	{ R367qam_ALLPASSFILT5, 0xf6 }, /* ALLPASSFILT5 */
	{ R367qam_ALLPASSFILT6, 0x45 }, /* ALLPASSFILT6 */
	{ R367qam_ALLPASSFILT7, 0x6f }, /* ALLPASSFILT7 */
	{ R367qam_ALLPASSFILT8, 0x7e }, /* ALLPASSFILT8 */
	{ R367qam_ALLPASSFILT9, 0x05 }, /* ALLPASSFILT9 */
	{ R367qam_ALLPASSFILT10, 0x0a }, /* ALLPASSFILT10 */
	{ R367qam_ALLPASSFILT11, 0x51 }, /* ALLPASSFILT11 */
	{ R367qam_TRL_AGC_CFG, 0x20 }, /* TRL_AGC_CFG */
	{ R367qam_TRL_LPF_CFG, 0x28 }, /* TRL_LPF_CFG */
	{ R367qam_TRL_LPF_ACQ_GAIN, 0x44 }, /* TRL_LPF_ACQ_GAIN */
	{ R367qam_TRL_LPF_TRK_GAIN, 0x22 }, /* TRL_LPF_TRK_GAIN */
	{ R367qam_TRL_LPF_OUT_GAIN, 0x03 }, /* TRL_LPF_OUT_GAIN */
	{ R367qam_TRL_LOCKDET_LTH, 0x04 }, /* TRL_LOCKDET_LTH */
	{ R367qam_TRL_LOCKDET_HTH, 0x11 }, /* TRL_LOCKDET_HTH */
	{ R367qam_TRL_LOCKDET_TRGVAL, 0x20 }, /* TRL_LOCKDET_TRGVAL */
	{ R367qam_IQ_QAM, 0x01}, /* IQ_QAM */
	{ R367qam_FSM_STATE, 0xa0 }, /* FSM_STATE */
	{ R367qam_FSM_CTL, 0x08 }, /* FSM_CTL */
	{ R367qam_FSM_STS, 0x0c }, /* FSM_STS */
	{ R367qam_FSM_SNR0_HTH, 0x00 }, /* FSM_SNR0_HTH */
	{ R367qam_FSM_SNR1_HTH, 0x00 }, /* FSM_SNR1_HTH */
	{ R367qam_FSM_SNR2_HTH, 0x00 }, /* FSM_SNR2_HTH */
	{ R367qam_FSM_SNR0_LTH, 0x00 }, /* FSM_SNR0_LTH */
	{ R367qam_FSM_SNR1_LTH, 0x00 }, /* FSM_SNR1_LTH */
	{ R367qam_FSM_EQA1_HTH, 0x00 }, /* FSM_EQA1_HTH */
	{ R367qam_FSM_TEMPO, 0x32 }, /* FSM_TEMPO */
	{ R367qam_FSM_CONFIG, 0x03 }, /* FSM_CONFIG */
	{ R367qam_EQU_I_TESTTAP_L, 0x11 }, /* EQU_I_TESTTAP_L */
	{ R367qam_EQU_I_TESTTAP_M, 0x00 }, /* EQU_I_TESTTAP_M */
	{ R367qam_EQU_I_TESTTAP_H, 0x00 }, /* EQU_I_TESTTAP_H */
	{ R367qam_EQU_TESTAP_CFG, 0x00 }, /* EQU_TESTAP_CFG */
	{ R367qam_EQU_Q_TESTTAP_L, 0xff }, /* EQU_Q_TESTTAP_L */
	{ R367qam_EQU_Q_TESTTAP_M, 0x00 }, /* EQU_Q_TESTTAP_M */
	{ R367qam_EQU_Q_TESTTAP_H, 0x00 }, /* EQU_Q_TESTTAP_H */
	{ R367qam_EQU_TAP_CTRL, 0x00 }, /* EQU_TAP_CTRL */
	{ R367qam_EQU_CTR_CRL_CONTROL_L, 0x11 }, /* EQU_CTR_CRL_CONTROL_L */
	{ R367qam_EQU_CTR_CRL_CONTROL_H, 0x05 }, /* EQU_CTR_CRL_CONTROL_H */
	{ R367qam_EQU_CTR_HIPOW_L, 0x00 }, /* EQU_CTR_HIPOW_L */
	{ R367qam_EQU_CTR_HIPOW_H, 0x00 }, /* EQU_CTR_HIPOW_H */
	{ R367qam_EQU_I_EQU_LO, 0xef }, /* EQU_I_EQU_LO */
	{ R367qam_EQU_I_EQU_HI, 0x00 }, /* EQU_I_EQU_HI */
	{ R367qam_EQU_Q_EQU_LO, 0xee }, /* EQU_Q_EQU_LO */
	{ R367qam_EQU_Q_EQU_HI, 0x00 }, /* EQU_Q_EQU_HI */
	{ R367qam_EQU_MAPPER, 0xc5 }, /* EQU_MAPPER */
	{ R367qam_EQU_SWEEP_RATE, 0x80 }, /* EQU_SWEEP_RATE */
	{ R367qam_EQU_SNR_LO, 0x64 }, /* EQU_SNR_LO */
	{ R367qam_EQU_SNR_HI, 0x03 }, /* EQU_SNR_HI */
	{ R367qam_EQU_GAMMA_LO, 0x00 }, /* EQU_GAMMA_LO */
	{ R367qam_EQU_GAMMA_HI, 0x00 }, /* EQU_GAMMA_HI */
	{ R367qam_EQU_ERR_GAIN, 0x36 }, /* EQU_ERR_GAIN */
	{ R367qam_EQU_RADIUS, 0xaa }, /* EQU_RADIUS */
	{ R367qam_EQU_FFE_MAINTAP, 0x00 }, /* EQU_FFE_MAINTAP */
	{ R367qam_EQU_FFE_LEAKAGE, 0x63 }, /* EQU_FFE_LEAKAGE */
	{ R367qam_EQU_FFE_MAINTAP_POS, 0xdf }, /* EQU_FFE_MAINTAP_POS */
	{ R367qam_EQU_GAIN_WIDE, 0x88 }, /* EQU_GAIN_WIDE */
	{ R367qam_EQU_GAIN_NARROW, 0x41 }, /* EQU_GAIN_NARROW */
	{ R367qam_EQU_CTR_LPF_GAIN, 0xd1 }, /* EQU_CTR_LPF_GAIN */
	{ R367qam_EQU_CRL_LPF_GAIN, 0xa7 }, /* EQU_CRL_LPF_GAIN */
	{ R367qam_EQU_GLOBAL_GAIN, 0x06 }, /* EQU_GLOBAL_GAIN */
	{ R367qam_EQU_CRL_LD_SEN, 0x85 }, /* EQU_CRL_LD_SEN */
	{ R367qam_EQU_CRL_LD_VAL, 0xe2 }, /* EQU_CRL_LD_VAL */
	{ R367qam_EQU_CRL_TFR, 0x20 }, /* EQU_CRL_TFR */
	{ R367qam_EQU_CRL_BISTH_LO, 0x00 }, /* EQU_CRL_BISTH_LO */
	{ R367qam_EQU_CRL_BISTH_HI, 0x00 }, /* EQU_CRL_BISTH_HI */
	{ R367qam_EQU_SWEEP_RANGE_LO, 0x00 }, /* EQU_SWEEP_RANGE_LO */
	{ R367qam_EQU_SWEEP_RANGE_HI, 0x00 }, /* EQU_SWEEP_RANGE_HI */
	{ R367qam_EQU_CRL_LIMITER, 0x40 }, /* EQU_CRL_LIMITER */
	{ R367qam_EQU_MODULUS_MAP, 0x90 }, /* EQU_MODULUS_MAP */
	{ R367qam_EQU_PNT_GAIN, 0xa7 }, /* EQU_PNT_GAIN */
	{ R367qam_FEC_AC_CTR_0, 0x16 }, /* FEC_AC_CTR_0 */
	{ R367qam_FEC_AC_CTR_1, 0x0b }, /* FEC_AC_CTR_1 */
	{ R367qam_FEC_AC_CTR_2, 0x88 }, /* FEC_AC_CTR_2 */
	{ R367qam_FEC_AC_CTR_3, 0x02 }, /* FEC_AC_CTR_3 */
	{ R367qam_FEC_STATUS, 0x12 }, /* FEC_STATUS */
	{ R367qam_RS_COUNTER_0, 0x7d }, /* RS_COUNTER_0 */
	{ R367qam_RS_COUNTER_1, 0xd0 }, /* RS_COUNTER_1 */
	{ R367qam_RS_COUNTER_2, 0x19 }, /* RS_COUNTER_2 */
	{ R367qam_RS_COUNTER_3, 0x0b }, /* RS_COUNTER_3 */
	{ R367qam_RS_COUNTER_4, 0xa3 }, /* RS_COUNTER_4 */
	{ R367qam_RS_COUNTER_5, 0x00 }, /* RS_COUNTER_5 */
	{ R367qam_BERT_0, 0x01 }, /* BERT_0 */
	{ R367qam_BERT_1, 0x25 }, /* BERT_1 */
	{ R367qam_BERT_2, 0x41 }, /* BERT_2 */
	{ R367qam_BERT_3, 0x39 }, /* BERT_3 */
	{ R367qam_OUTFORMAT_0, 0xc2 }, /* OUTFORMAT_0 */
	{ R367qam_OUTFORMAT_1, 0x22 }, /* OUTFORMAT_1 */
	{ R367qam_SMOOTHER_2, 0x28 }, /* SMOOTHER_2 */
	{ R367qam_TSMF_CTRL_0, 0x01 }, /* TSMF_CTRL_0 */
	{ R367qam_TSMF_CTRL_1, 0xc6 }, /* TSMF_CTRL_1 */
	{ R367qam_TSMF_CTRL_3, 0x43 }, /* TSMF_CTRL_3 */
	{ R367qam_TS_ON_ID_0, 0x00 }, /* TS_ON_ID_0 */
	{ R367qam_TS_ON_ID_1, 0x00 }, /* TS_ON_ID_1 */
	{ R367qam_TS_ON_ID_2, 0x00 }, /* TS_ON_ID_2 */
	{ R367qam_TS_ON_ID_3, 0x00 }, /* TS_ON_ID_3 */
	{ R367qam_RE_STATUS_0, 0x00 }, /* RE_STATUS_0 */
	{ R367qam_RE_STATUS_1, 0x00 }, /* RE_STATUS_1 */
	{ R367qam_RE_STATUS_2, 0x00 }, /* RE_STATUS_2 */
	{ R367qam_RE_STATUS_3, 0x00 }, /* RE_STATUS_3 */
	{ R367qam_TS_STATUS_0, 0x00 }, /* TS_STATUS_0 */
	{ R367qam_TS_STATUS_1, 0x00 }, /* TS_STATUS_1 */
	{ R367qam_TS_STATUS_2, 0xa0 }, /* TS_STATUS_2 */
	{ R367qam_TS_STATUS_3, 0x00 }, /* TS_STATUS_3 */
	{ R367qam_T_O_ID_0, 0x00 }, /* T_O_ID_0 */
	{ R367qam_T_O_ID_1, 0x00 }, /* T_O_ID_1 */
	{ R367qam_T_O_ID_2, 0x00 }, /* T_O_ID_2 */
	{ R367qam_T_O_ID_3, 0x00 }, /* T_O_ID_3 */
};

/*------------------------------------------------------------------------------*/
U32 FirstTimeBER[3] = {0};
FE_367qam_SIGNALTYPE_t FE_367qam_Algo(YWTUNER_Handle_T Handle, FE_367qam_InternalParams_t *pIntParams);

/*------------------------------------------------------------------------------*/
/***********************************************************************
 函数名称: demod_d0367qam_Identify

 函数说明: 检测硬件是否0367qam

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010-12-28 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_Identify(IOARCH_Handle_t IOHandle, U8 ucID, U8 *pucActualID)
{
#ifdef TUNER_USE_CAB_STI7167CAB
	//if (TUNER_IOARCH_ReadWrite(IOHandle, TUNER_IO_SA_READ, R367qam_ID, pucActualID, 1, 50) == YW_NO_ERROR)
	if (YW_NO_ERROR == YW_NO_ERROR)
	{
		_DEBUG
		printk("demod_d0367qam_Identify pucActualID = 0x%x\n", *pucActualID);//question
		return YW_NO_ERROR;
	}
	else
	{
		_DEBUG
		printk("demod_d0367qam_Identify YWHAL_ERROR_UNKNOWN_DEVICE \n");//question
		return YWHAL_ERROR_UNKNOWN_DEVICE;
	}
	return YW_NO_ERROR;
#else
	return YWHAL_ERROR_UNKNOWN_DEVICE;
#endif
}

/***********************************************************************
 函数名称: demod_d0367qam_Repeat

 函数说明: 0367qam的转发功能

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.12.28 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_Repeat(IOARCH_Handle_t DemodIOHandle,
									 IOARCH_Handle_t TunerIOHandle,
									 TUNER_IOARCH_Operation_t Operation,
									 unsigned short SubAddr,
									 U8 *Data,
									 U32 TransferSize,
									 U32 Timeout)
{
	return 0;
}

#ifdef TUNER_USE_CAB_STI7167CAB

YW_ErrorType_T D0367qam_Init(TUNER_IOREG_DeviceMap_t *DeviceMap,
							 IOARCH_Handle_t IOHandle,
							 TUNER_TunerType_T TunerType)
{
	U16 i, j = 1;
	for (i = 1; i <= DeviceMap->Registers; i++)
	{
		ChipUpdateDefaultValues_0367qam(DeviceMap, IOHandle, Def367qamVal[i - 1].Addr, Def367qamVal[i - 1].Value, i - 1);
		if (i != STV0367qam_NBREGS)
		{
			if (Def367qamVal[i].Addr == Def367qamVal[i - 1].Addr + 1)
			{
				j++;
			}
			else if (j == 1)
			{
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, Def367qamVal[i - 1].Addr, Def367qamVal[i - 1].Value);
			}
			else
			{
				ChipSetRegisters_0367qam(DeviceMap, IOHandle, Def367qamVal[i - j].Addr, j);
				j = 1;
			}
		}
		else
		{
			if (j == 1)
			{
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, Def367qamVal[i - 1].Addr, Def367qamVal[i - 1].Value);
			}
			else
			{
				ChipSetRegisters_0367qam(DeviceMap, IOHandle, Def367qamVal[i - j].Addr, j);
				j = 1;
			}
		}
	}
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_BERT_ON, 0); /* restart a new BER count */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_BERT_ON, 1); /* restart a new BER count */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_OUTFORMAT, 0x00); //FE_TS_PARALLEL_PUNCT_CLOCK
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_CLK_POLARITY, 0x01); //FE_TS_RISINGEDGE_CLOCK
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_SYNC_STRIP, 0X00); //STFRONTEND_TS_SYNCBYTE_ON
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_CT_NBST, 0x00); //STFRONTEND_TS_PARITYBYTES_OFF
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_TS_SWAP, 0x00); //STFRONTEND_TS_SWAP_OFF
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_FIFO_BYPASS, 0x01); //FE_TS_SMOOTHER_DEFAULT, ?? question
	/* Here we make the necessary changes to the demod's registers depending on the tuner */
	if (DeviceMap != NULL)
	{
		/*----------------------------------------------------------------------------------------*/
		switch (TunerType) //important
		{
			case TUNER_TUNER_SHARP5469C:
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_ANACTRL, 0x0D); /* PLL bypassed and disabled */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_PLLMDIV, 0x01); /* IC runs at 54MHz with a 27MHz crystal */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_PLLNDIV, 0x08);
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_PLLSETUP, 0x18); /* ADC clock is equal to system clock */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_ANACTRL, 0x00); /* PLL enabled and used */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_ANADIGCTRL, 0x0b); /* Buffer Q disabled */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_DUAL_AD12, 0x04); /* ADCQ disabled */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_FSM_SNR2_HTH, 0x23); /* Improves the C/N lock limit */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_IQ_QAM, 0x01); /* ZIF/IF Automatic mode */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_I2CRPT, 0x22); /* I2C repeater configuration, value changes with I2C master clock */
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_EQU_FFE_LEAKAGE, 0x63);
				ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_IQDEM_ADJ_EN, 0x04); //lwj change 0x05 to 0x04
				break;
			default:
				break;
		}
	}
	return YW_NO_ERROR;
}

YW_ErrorType_T D0367qam_Sleep(TUNER_IOREG_DeviceMap_t *DeviceMap,
							  IOARCH_Handle_t IOHandle)
{
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_BYPASS_PLLXN, 0x03);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_STDBY_PLLXN, 0x01);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_STDBY, 1);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_STDBY_CORE, 1);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_EN_BUFFER_I, 0);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_EN_BUFFER_Q, 0);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_POFFQ, 1);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_POFFI, 1);
	return YW_NO_ERROR;
}

YW_ErrorType_T D0367qam_Wake(TUNER_IOREG_DeviceMap_t *DeviceMap,
							 IOARCH_Handle_t IOHandle)
{
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_STDBY_PLLXN, 0x00);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_BYPASS_PLLXN, 0x00);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_STDBY, 0);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_STDBY_CORE, 0);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_EN_BUFFER_I, 1);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_EN_BUFFER_Q, 1);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_POFFQ, 0);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_POFFI, 0);
	return YW_NO_ERROR;
}

YW_ErrorType_T D0367qam_I2ctOn(TUNER_IOREG_DeviceMap_t *DeviceMap,
							   IOARCH_Handle_t IOHandle)
{
	return ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_I2CT_ON, 1);
}

YW_ErrorType_T D0367qam_I2ctOff(TUNER_IOREG_DeviceMap_t *DeviceMap,
								IOARCH_Handle_t IOHandle)
{
	return ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_I2CT_ON, 0);
}

YW_ErrorType_T demod_d0367qam_Open_test(IOARCH_Handle_t IOHandle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_IOREG_DeviceMap_t DeviceMap;
	/*-----------------寄存器结构分配--------------*/
	DeviceMap.Timeout = IOREG_DEFAULT_TIMEOUT;
	DeviceMap.Registers = STV0367qam_NBREGS;
	DeviceMap.Fields = STV0367qam_NBFIELDS;
	DeviceMap.Mode = IOREG_MODE_SUBADR_16;
	DeviceMap.RegExtClk = 27000000; //Demod External Crystal_HZ
	//Error = TUNER_IOREG_Open(&DeviceMap);
	DeviceMap.DefVal = NULL;
	DeviceMap.Error = 0;
	D0367qam_Init(&DeviceMap, IOHandle, TUNER_TUNER_SHARP5469C);
	//pParams->MasterClock_Hz = FE_367qam_GetMclkFreq(pParams->hDemod,pParams->Crystal_Hz);
	//pParams->AdcClock_Hz = FE_367qam_GetADCFreq(pParams->hDemod,pParams->Crystal_Hz); //question
	//Error = TUNER_IOREG_Close(&DeviceMap);
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367qam_Open

 函数说明: 打开0367qam，初始化寄存器

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.12.28 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_Open(U8 Handle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	//printk("demod_d0367qam_Open ===== \n");
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Cab.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Cab.Demod_DeviceMap;
	/*------------------驱动函数指针----------------*/
	Inst->DriverParam.Cab.DemodDriver.Demod_GetSignalInfo = demod_d0367qam_GetSignalInfo;
	Inst->DriverParam.Cab.DemodDriver.Demod_IsLocked = demod_d0367qam_IsLocked;
	Inst->DriverParam.Cab.DemodDriver.Demod_repeat = demod_d0367qam_Repeat;
	Inst->DriverParam.Cab.DemodDriver.Demod_reset = demod_d0367qam_Reset;
	Inst->DriverParam.Cab.DemodDriver.Demod_ScanFreq = demod_d0367qam_ScanFreq;
	Inst->DriverParam.Cab.DemodDriver.Demod_standy = demod_d0367qam_SetStandby;
	/*-----------------寄存器结构分配--------------*/
	DeviceMap->Timeout = IOREG_DEFAULT_TIMEOUT;
	DeviceMap->Registers = STV0367qam_NBREGS;
	DeviceMap->Fields = STV0367qam_NBFIELDS;
	DeviceMap->Mode = IOREG_MODE_SUBADR_16;
	DeviceMap->RegExtClk = Inst->ExternalClock; //Demod External Crystal_HZ
	//Error = TUNER_IOREG_Open(DeviceMap);
	DeviceMap->DefVal = NULL;
	DeviceMap->Error = 0;
	D0367qam_Init(DeviceMap, IOHandle, Inst->DriverParam.Cab.TunerType);
	//pParams->MasterClock_Hz = FE_367qam_GetMclkFreq(pParams->hDemod,pParams->Crystal_Hz);
	//pParams->AdcClock_Hz = FE_367qam_GetADCFreq(pParams->hDemod,pParams->Crystal_Hz); //question
	return (Error);
}
/***********************************************************************
 函数名称: demod_d0367qam_Close

 函数说明: 关闭0367qam

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.16 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_Close(U8 Handle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	Inst = TUNER_GetScanInfo(Handle);
	Inst->DriverParam.Cab.DemodDriver.Demod_GetSignalInfo = NULL;
	Inst->DriverParam.Cab.DemodDriver.Demod_IsLocked = NULL;
	Inst->DriverParam.Cab.DemodDriver.Demod_repeat = NULL;
	Inst->DriverParam.Cab.DemodDriver.Demod_reset = NULL;
	Inst->DriverParam.Cab.DemodDriver.Demod_ScanFreq = NULL;
	//Error = TUNER_IOREG_Close(&Inst->DriverParam.Cab.Demod_DeviceMap);
	Error |= Inst->DriverParam.Cab.Demod_DeviceMap.Error;
	Inst->DriverParam.Cab.Demod_DeviceMap.Error = YW_NO_ERROR;
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367qam_Reset

 函数说明: 复位0367 寄存器

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.12.28 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_Reset(U8 Index)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	Inst = TUNER_GetScanInfo(Index);
	IOHandle = Inst->DriverParam.Cab.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Cab.Demod_DeviceMap;
	D0367qam_Init(DeviceMap, IOHandle, Inst->DriverParam.Cab.TunerType);
	//pParams->MasterClock_Hz = FE_367qam_GetMclkFreq(pParams->hDemod,pParams->Crystal_Hz);
	//pParams->AdcClock_Hz = FE_367qam_GetADCFreq(pParams->hDemod,pParams->Crystal_Hz); //question
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367qam_GetSignalInfo

 函数说明: 读取信号质量与强度

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.12.28 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_GetSignalInfo(U8 Index, U32 *Quality, U32 *Intensity, U32 *Ber)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	*Quality = 0;
	*Intensity = 0;
	*Ber = 0;
	FE_STV0367QAM_GetSignalInfo(Index, Quality, Intensity, Ber, &(FirstTimeBER[Index]));
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367qam_IsLocked

 函数说明: 读取信号是否锁定

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_IsLocked(U8 Handle, BOOL *IsLocked)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Cab.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Cab.Demod_DeviceMap;
	*IsLocked = FE_367qam_Status(DeviceMap, IOHandle);
	return (Error);
}

/*****************************************************
--FUNCTION :: FE_367qam_Algo
--ACTION :: Driver functio of the STV0367QAM chip
--PARAMS IN :: *pIntParams ==> Demod handle
-- Tuner handle
 Search frequency
 Symbolrate
 QAM Size
 DerotOffset
 SweepRate
 Search Range
 Master clock
 ADC clock
 Structure for result storage
--PARAMS OUT:: NONE
--RETURN :: Handle to STV0367QAM
--***************************************************/
FE_367qam_SIGNALTYPE_t FE_367qam_Algo(YWTUNER_Handle_T Handle, FE_367qam_InternalParams_t *pIntParams)
{
	FE_367qam_SIGNALTYPE_t signalType = FE_367qam_NOAGC; /* Signal search status initialization */
	U32 QAMFEC_Lock, QAM_Lock, u32_tmp ;
	BOOL TunerLock = FALSE;
	U32 LockTime, TRLTimeOut, AGCTimeOut, CRLSymbols, CRLTimeOut, EQLTimeOut, DemodTimeOut, FECTimeOut;
	U8 TrackAGCAccum;
	TUNER_ScanTaskParam_T *Inst = NULL;
	TUNER_IOREG_DeviceMap_t *DeviceMap = NULL;
	IOARCH_Handle_t IOHandle;
	U32 FreqResult = 0;
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Cab.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Cab.Demod_DeviceMap;
	if (Inst->ForceSearchTerm)
	{
		return (FE_LLA_INVALID_HANDLE);
	}
	/* Timeouts calculation */
	/* A max lock time of 25 ms is allowed for delayed AGC */
	AGCTimeOut = 25;
	/* 100000 symbols needed by the TRL as a maximum value */
	TRLTimeOut = 100000000 / pIntParams->SymbolRate_Bds;
	/* CRLSymbols is the needed number of symbols to achieve a lock within [-4%, +4%] of the symbol rate.
	 CRL timeout is calculated for a lock within [-SearchRange_Hz, +SearchRange_Hz].
	 EQL timeout can be changed depending on the micro-reflections we want to handle.
	 A characterization must be performed with these echoes to get new timeout values.
	*/
	switch (pIntParams->Modulation)
	{
		case FE_CAB_MOD_QAM16:
			CRLSymbols = 150000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM32:
			CRLSymbols = 250000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM64:
			CRLSymbols = 200000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM128:
			CRLSymbols = 250000;
			EQLTimeOut = 100;
			break;
		case FE_CAB_MOD_QAM256:
			CRLSymbols = 250000;
			EQLTimeOut = 100;
			break;
		default:
			CRLSymbols = 200000;
			EQLTimeOut = 100;
			break;
	}
	if (pIntParams->SearchRange_Hz < 0)
	{
		CRLTimeOut = (25 * CRLSymbols * (-pIntParams->SearchRange_Hz / 1000)) / (pIntParams->SymbolRate_Bds / 1000);
	}
	else
	{
		CRLTimeOut = (25 * CRLSymbols * (pIntParams->SearchRange_Hz / 1000)) / (pIntParams->SymbolRate_Bds / 1000);
	}
	CRLTimeOut = (1000 * CRLTimeOut) / pIntParams->SymbolRate_Bds;
	/* Timeouts below 50ms are coerced */
	if (CRLTimeOut < 50) CRLTimeOut = 50;
	/* A maximum of 100 TS packets is needed to get FEC lock even in case the spectrum inversion needs to be changed.
	 This is equal to 20 ms in case of the lowest symbol rate of 0.87Msps
	*/
	FECTimeOut = 20;
	DemodTimeOut = AGCTimeOut + TRLTimeOut + CRLTimeOut + EQLTimeOut;
	/* Reset the TRL to ensure nothing starts until the
	 AGC is stable which ensures a better lock time
	*/
	ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_CTRL_1, 0x04);
	/* Set AGC accumulation time to minimum and lock threshold to maximum in order to speed up the AGC lock */
	TrackAGCAccum = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_ACCUMRSTSEL);
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_ACCUMRSTSEL, 0x0);
	/* Modulus Mapper is disabled */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_MODULUSMAP_EN, 0);
	/* Disable the sweep function */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_SWEEP_EN, 0);
	/* The sweep function is never used, Sweep rate must be set to 0 */
	/* Set the derotator frequency in Hz */
	//FE_367qam_SetDerotFreq(DeviceMap,IOHandle,pIntParams->AdcClock_Hz,(1000*(S32)FE_TunerGetIF_Freq(pIntParams->hTuner)+pIntParams->DerotOffset_Hz));
	if (Inst->DriverParam.Cab.TunerType == TUNER_TUNER_SHARP5469C)
	{
		FE_367qam_SetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz, (1000 * (36125000 / 1000) + pIntParams->DerotOffset_Hz)); //question if freq
	}
	/* Disable the Allpass Filter when the symbol rate is out of range */
	if ((pIntParams->SymbolRate_Bds > 10800000) || (pIntParams->SymbolRate_Bds < 1800000))
	{
		ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_ADJ_EN, 0);
		ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_ALLPASSFILT_EN, 0);
	}
	/* Check if the tuner is locked */
	if (Inst->DriverParam.Cab.TunerDriver.tuner_IsLocked != NULL)
	{
		Inst->DriverParam.Cab.TunerDriver.tuner_IsLocked(Handle, &TunerLock);
	}
	if (TunerLock == 0)
	{
		return FE_367qam_NOTUNER;
	}
	/* Relase the TRL to start demodulator acquisition */
	/* Wait for QAM lock */
	LockTime = 0;
	ChipSetOneRegister_0367qam(DeviceMap, IOHandle, R367qam_CTRL_1, 0x00);
	do
	{
		QAM_Lock = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_FSM_STATUS);
		if ((LockTime >= (DemodTimeOut - EQLTimeOut)) && (QAM_Lock == 0x04))
			/* We don't wait longer, the frequency/phase offset must be too big */
			LockTime = DemodTimeOut;
		else if ((LockTime >= (AGCTimeOut + TRLTimeOut)) && (QAM_Lock == 0x02))
			/* We don't wait longer, either there is no signal or it is not the right symbol rate or it is an analog carrier */
		{
			LockTime = DemodTimeOut;
			ChipGetRegisters_0367qam(DeviceMap, IOHandle, R367qam_AGC_PWR_RD_L, 3);
			u32_tmp = ChipGetFieldImage_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWR_WORD_LO)
					  + (ChipGetFieldImage_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWR_WORD_ME) << 8)
					  + (ChipGetFieldImage_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWR_WORD_HI) << 16);
			if (u32_tmp >= 131072)
				u32_tmp = 262144 - u32_tmp;
			u32_tmp = u32_tmp / (PowOf2(11 - ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_IF_BWSEL)));
			if (u32_tmp < ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWRREF_LO) + 256 * ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_PWRREF_HI) - 10)
				QAM_Lock = 0x0f;
		}
		else
		{
			ChipWaitOrAbort_0367qam(Inst->ForceSearchTerm, 10); /* wait 10 ms */
			LockTime += 10;
		}
	}
	while (((QAM_Lock != 0x0c) && (QAM_Lock != 0x0b)) && (LockTime < DemodTimeOut));
	if ((QAM_Lock == 0x0c) || (QAM_Lock == 0x0b))
	{
		/* Wait for FEC lock */
		LockTime = 0;
		do
		{
			ChipWaitOrAbort_0367qam(Inst->ForceSearchTerm, 5); /* wait 5 ms */
			LockTime += 5;
			QAMFEC_Lock = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_QAMFEC_LOCK);
		}
		while (!QAMFEC_Lock && (LockTime < FECTimeOut));
	}
	else
		QAMFEC_Lock = 0;
	if (QAMFEC_Lock)
	{
		signalType = FE_367qam_DATAOK;
		pIntParams->DemodResult.Modulation = pIntParams->Modulation;
		pIntParams->DemodResult.SpectInv = ChipGetField_0367qam(DeviceMap, IOHandle, F367qam_QUAD_INV);
		//lwj add begin
		if (Inst->DriverParam.Cab.TunerDriver.tuner_GetFreq != NULL) //lwj add
		{
			Inst->DriverParam.Cab.TunerDriver.tuner_GetFreq(Handle, &FreqResult);
		}
		//lwj add end
		//if (FE_TunerGetIF_Freq(pIntParams->hTuner) != 0)
#if 0 //lwj remove
		if (0) //IF 为０
		{
			if (FE_TunerGetIF_Freq(pIntParams->hTuner) > pIntParams->AdcClock_Hz / 1000)
			{
				pIntParams->DemodResult.Frequency_kHz = FreqResult - FE_367qam_GetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz) - pIntParams->AdcClock_Hz / 1000 + FE_TunerGetIF_Freq(pIntParams->hTuner);
			}
			else
			{
				pIntParams->DemodResult.Frequency_kHz = FreqResult - FE_367qam_GetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz) + FE_TunerGetIF_Freq(pIntParams->hTuner);
			}
		}
		else
#endif
		{
			pIntParams->DemodResult.Frequency_kHz = FreqResult + FE_367qam_GetDerotFreq(DeviceMap, IOHandle, pIntParams->AdcClock_Hz) - pIntParams->AdcClock_Hz / 4000;
		}
		pIntParams->DemodResult.SymbolRate_Bds = FE_367qam_GetSymbolRate(DeviceMap, IOHandle, pIntParams->MasterClock_Hz);
		pIntParams->DemodResult.Locked = 1;
		/* ChipSetField(pIntParams->hDemod,F367qam_AGC_ACCUMRSTSEL,7);*/
	}
	else
	{
		switch (QAM_Lock)
		{
			case 1:
				signalType = FE_367qam_NOAGC;
				break;
			case 2:
				signalType = FE_367qam_NOTIMING;
				break;
			case 3:
				signalType = FE_367qam_TIMINGOK;
				break;
			case 4:
				signalType = FE_367qam_NOCARRIER;
				break;
			case 5:
				signalType = FE_367qam_CARRIEROK;
				break;
			case 7:
				signalType = FE_367qam_NOBLIND;
				break;
			case 8:
				signalType = FE_367qam_BLINDOK;
				break;
			case 10:
				signalType = FE_367qam_NODEMOD;
				break;
			case 11:
				signalType = FE_367qam_DEMODOK;
				break;
			case 12:
				signalType = FE_367qam_DEMODOK;
				break;
			case 13:
				signalType = FE_367qam_NODEMOD;
				break;
			case 14:
				signalType = FE_367qam_NOBLIND;
				break;
			case 15:
				signalType = FE_367qam_NOSIGNAL;
				break;
			default:
				break;
		}
	}
	/* Set the AGC control values to tracking values */
	ChipSetField_0367qam(DeviceMap, IOHandle, F367qam_AGC_ACCUMRSTSEL, TrackAGCAccum);
	return signalType;
}

/***********************************************************************
 函数名称: demod_d0367qam_ScanFreq

 函数说明: 搜索频率

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367qam_ScanFreq(U8 Index)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	FE_367qam_InternalParams_t pParams;
	U32 ZigzagScan = 0;
	S32 SearchRange_Hz_Tmp;
	Inst = TUNER_GetScanInfo(Index);
	IOHandle = Inst->DriverParam.Cab.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Cab.Demod_DeviceMap;
	FirstTimeBER[Index] = 1;
	YWLIB_Memset(&pParams, 0, sizeof(FE_367qam_InternalParams_t));
	pParams.State = FE_367qam_NOTUNER;
	/*
	--- Modulation type is set to
	*/
	switch (Inst->DriverParam.Cab.Param.Modulation) ////
	{
		case YWTUNER_MOD_QAM_16 :
			pParams.Modulation = FE_CAB_MOD_QAM16;
			break;
		case YWTUNER_MOD_QAM_32 :
			pParams.Modulation = FE_CAB_MOD_QAM32;
			break;
		case YWTUNER_MOD_QAM_64 :
			pParams.Modulation = FE_CAB_MOD_QAM64;
			break;
		case YWTUNER_MOD_QAM_128 :
			pParams.Modulation = FE_CAB_MOD_QAM128;
			break;
		case YWTUNER_MOD_QAM_256 :
			pParams.Modulation = FE_CAB_MOD_QAM256;
			break;
		default:
			pParams.Modulation = FE_CAB_MOD_QAM64;
			break;
	}
	pParams.Crystal_Hz = DeviceMap->RegExtClk; //30M 还是27M，由硬件决定
	pParams.SearchRange_Hz = 280000;/*280 kHz*/ //question
	pParams.SymbolRate_Bds = Inst->DriverParam.Cab.Param.SymbolRateB;////
	pParams.Frequency_kHz = Inst->DriverParam.Cab.Param.FreqKHz;////
	pParams.AdcClock_Hz = FE_367qam_GetADCFreq(DeviceMap, IOHandle, pParams.Crystal_Hz);
	pParams.MasterClock_Hz = FE_367qam_GetMclkFreq(DeviceMap, IOHandle, pParams.Crystal_Hz);
// printk("demod_d0367qam_ScanFreq Frequency === %d\n", pParams.Frequency_kHz);
// printk("SymbolRate_Bds =========== %d\n", pParams.SymbolRate_Bds);
// printk("pParams.AdcClock_Hz ====== %d\n",pParams.AdcClock_Hz);
// printk("pParams.MasterClock_Hz ===== %d\n",pParams.MasterClock_Hz);
	if (Inst->DriverParam.Cab.TunerDriver.tuner_SetFreq != NULL)
	{
		Error = (Inst->DriverParam.Cab.TunerDriver.tuner_SetFreq)(Index, pParams.Frequency_kHz, NULL);
	}
	/* Sets the QAM size and all the related parameters */
	FE_367qam_SetQamSize(Inst, DeviceMap, IOHandle, pParams.Frequency_kHz, pParams.SymbolRate_Bds, pParams.Modulation);
	/* Sets the symbol and all the related parameters */
	FE_367qam_SetSymbolRate(DeviceMap, IOHandle, pParams.AdcClock_Hz, pParams.MasterClock_Hz, pParams.SymbolRate_Bds, pParams.Modulation);
	/* Zigzag Algorithm test */
	if (25 * pParams.SearchRange_Hz > pParams.SymbolRate_Bds)
	{
		pParams.SearchRange_Hz = -(S32)(pParams.SymbolRate_Bds) / 25;
		ZigzagScan = 1;
	}
	/* Search algorithm launch, [-1.1*RangeOffset, +1.1*RangeOffset] scan */
	pParams.State = FE_367qam_Algo(Index, &pParams);
	SearchRange_Hz_Tmp = pParams.SearchRange_Hz; //lwj add
	if (ZigzagScan && (pParams.State != FE_367qam_DATAOK))
	{
		do
		{
#if 1 //lwj modify
			pParams.SearchRange_Hz = -pParams.SearchRange_Hz;
			if (pParams.SearchRange_Hz > 0)
				pParams.DerotOffset_Hz = -pParams.DerotOffset_Hz + pParams.SearchRange_Hz;
			else
				pParams.DerotOffset_Hz = -pParams.DerotOffset_Hz;
#endif
			/* Search algorithm launch, [-1.1*RangeOffset, +1.1*RangeOffset] scan */
			pParams.State = FE_367qam_Algo(Index, &pParams);
		}
		while (((pParams.DerotOffset_Hz + pParams.SearchRange_Hz) >= -(S32)SearchRange_Hz_Tmp) && (pParams.State != FE_367qam_DATAOK));
	}
	/* check results */
	if ((pParams.State == FE_367qam_DATAOK) && (!Error))
	{
		/* update results */
		//printk("TUNER_STATUS_LOCKED #######################\n");
		Inst->Status = TUNER_INNER_STATUS_LOCKED;
		//pResult->Frequency_kHz = pIntParams->DemodResult.Frequency_kHz;
		//pResult->SymbolRate_Bds = pIntParams->DemodResult.SymbolRate_Bds;
		//pResult->SpectInv = pIntParams->DemodResult.SpectInv;
		//pResult->Modulation = pIntParams->DemodResult.Modulation;
	}
	else
	{
		Inst->Status = TUNER_INNER_STATUS_UNLOCKED;
	}
	return (Error);
}

/*****************************************************
--FUNCTION :: FE_STV0367qam_SetStandby
--ACTION :: Set demod STANDBY mode On/Off
--PARAMS IN :: Handle ==> Front End Handle

-PARAMS OUT:: NONE.
--RETURN :: Error (if any)
--***************************************************/
YW_ErrorType_T demod_d0367qam_SetStandby(U8 Handle)
{
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	TUNER_IOREG_DeviceMap_t *DeviceMap = NULL;
	IOARCH_Handle_t IOHandle;
	U8 StandbyOn = 1;
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Cab.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Cab.Demod_DeviceMap;
	if (StandbyOn)
	{
		if (Inst->DriverParam.Cab.TunerType == TUNER_TUNER_STV4100)
		{
			if (Inst->DriverParam.Cab.TunerDriver.tuner_SetStandby != NULL)
			{
				error = Inst->DriverParam.Cab.TunerDriver.tuner_SetStandby(Handle, StandbyOn);
			}
		}
		D0367qam_Sleep(DeviceMap, IOHandle);
	}
	else
	{
		D0367qam_Wake(DeviceMap, IOHandle);
		if (Inst->DriverParam.Cab.TunerType == TUNER_TUNER_STV4100)
		{
			if (Inst->DriverParam.Cab.TunerDriver.tuner_SetStandby != NULL)
			{
				error = Inst->DriverParam.Cab.TunerDriver.tuner_SetStandby(Handle, StandbyOn);
			}
		}
	}
	return (error);
}
#endif

/*eof----------------------------------------------------------------------------------*/
