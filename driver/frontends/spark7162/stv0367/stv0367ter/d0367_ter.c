/**********************************文件头部注释************************************/
//
//
// Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名： d0376.c
//
// 创建者： lwj
//
// 创建时间： 2010-11-11
//
// 文件描述：
//
// 修改记录： 日 期 作 者 版本 修定
//				 --------- --------- ----- -----
// 2010-11-11 lwj 1.0
/**********************************************************************************/

/* Includes ---------------------------------------------------------------- */

#define TUNER_ST
#define TUNER_USE_TER_STI7167TER

#include <linux/kernel.h> /* Kernel support */
#include <linux/string.h>

#include "D3501.h"

#include "ywtuner_ext.h"
#include "tuner_def.h"

#include "ioarch.h"
#include "ioreg.h"
#include "tuner_interface.h"

#include "stv0367ofdm_init.h"
#include "chip_0367ter.h"
#include "stv0367ofdm_drv.h"
#include "d0367_ter.h"

//IOARCH_HandleData_t IOARCH_Handle[TUNER_IOARCH_MAX_HANDLES];

STCHIP_Register_t Def367Val[STV0367ofdm_NBREGS] =
{
	{R367_ID, 0x60},
	{R367_I2CRPT, 0x22},
	{R367_TOPCTRL, 0x02},
	{R367_IOCFG0, 0x40},
	{R367_DAC0R, 0x00},
	{R367_IOCFG1, 0x00},
	{R367_DAC1R, 0x00},
	{R367_IOCFG2, 0x62},
	{R367_SDFR, 0x00},
	{R367ofdm_STATUS, 0xf8},
	{R367_AUX_CLK, 0x0a},
	{R367_FREESYS1, 0x00},
	{R367_FREESYS2, 0x00},
	{R367_FREESYS3, 0x00},
	{R367_GPIO_CFG, 0x55},
	{R367_GPIO_CMD, 0x00},
	{R367ofdm_AGC2MAX, 0xff},
	{R367ofdm_AGC2MIN, 0x00},
	{R367ofdm_AGC1MAX, 0xff},
	{R367ofdm_AGC1MIN, 0x00},
	{R367ofdm_AGCR, 0xbc},
	{R367ofdm_AGC2TH, 0x00},
	{R367ofdm_AGC12C, 0x00},
	{R367ofdm_AGCCTRL1, 0x85},
	{R367ofdm_AGCCTRL2, 0x1f},
	{R367ofdm_AGC1VAL1, 0x00},
	{R367ofdm_AGC1VAL2, 0x00},
	{R367ofdm_AGC2VAL1, 0x6f},
	{R367ofdm_AGC2VAL2, 0x05},
	{R367ofdm_AGC2PGA, 0x00},
	{R367ofdm_OVF_RATE1, 0x00},
	{R367ofdm_OVF_RATE2, 0x00},
	{R367ofdm_GAIN_SRC1, 0x2b},
	{R367ofdm_GAIN_SRC2, 0x04},
	{R367ofdm_INC_DEROT1, 0x55},
	{R367ofdm_INC_DEROT2, 0x55},
	{R367ofdm_PPM_CPAMP_DIR, 0x2c},
	{R367ofdm_PPM_CPAMP_INV, 0x00},
	{R367ofdm_FREESTFE_1, 0x00},
	{R367ofdm_FREESTFE_2, 0x1c},
	{R367ofdm_DCOFFSET, 0x00},
	{R367ofdm_EN_PROCESS, 0x05},
	{R367ofdm_SDI_SMOOTHER, 0x80},
	{R367ofdm_FE_LOOP_OPEN, 0x1c},
	{R367ofdm_FREQOFF1, 0x00},
	{R367ofdm_FREQOFF2, 0x00},
	{R367ofdm_FREQOFF3, 0x00},
	{R367ofdm_TIMOFF1, 0x00},
	{R367ofdm_TIMOFF2, 0x00},
	{R367ofdm_EPQ, 0x02},
	{R367ofdm_EPQAUTO, 0x01},
	{R367ofdm_SYR_UPDATE, 0xf5},
	{R367ofdm_CHPFREE, 0x00},
	{R367ofdm_PPM_STATE_MAC, 0x23},
	{R367ofdm_INR_THRESHOLD, 0xff},
	{R367ofdm_EPQ_TPS_ID_CELL, 0xf9},
	{R367ofdm_EPQ_CFG, 0x00},
	{R367ofdm_EPQ_STATUS, 0x01},
	{R367ofdm_AUTORELOCK, 0x81},
	{R367ofdm_BER_THR_VMSB, 0x00},
	{R367ofdm_BER_THR_MSB, 0x00},
	{R367ofdm_BER_THR_LSB, 0x00},
	{R367ofdm_CCD, 0x83},
	{R367ofdm_SPECTR_CFG, 0x00},
	{R367ofdm_CHC_DUMMY, 0x18},
	{R367ofdm_INC_CTL, 0x88},
	{R367ofdm_INCTHRES_COR1, 0xb4},
	{R367ofdm_INCTHRES_COR2, 0x96},
	{R367ofdm_INCTHRES_DET1, 0x0e},
	{R367ofdm_INCTHRES_DET2, 0x11},
	{R367ofdm_IIR_CELLNB, 0x8d},
	{R367ofdm_IIRCX_COEFF1_MSB, 0x00},
	{R367ofdm_IIRCX_COEFF1_LSB, 0x00},
	{R367ofdm_IIRCX_COEFF2_MSB, 0x09},
	{R367ofdm_IIRCX_COEFF2_LSB, 0x18},
	{R367ofdm_IIRCX_COEFF3_MSB, 0x14},
	{R367ofdm_IIRCX_COEFF3_LSB, 0x9c},
	{R367ofdm_IIRCX_COEFF4_MSB, 0x00},
	{R367ofdm_IIRCX_COEFF4_LSB, 0x00},
	{R367ofdm_IIRCX_COEFF5_MSB, 0x36},
	{R367ofdm_IIRCX_COEFF5_LSB, 0x42},
	{R367ofdm_FEPATH_CFG, 0x00},
	{R367ofdm_PMC1_FUNC, 0x65},
	{R367ofdm_PMC1_FOR, 0x00},
	{R367ofdm_PMC2_FUNC, 0x00},
	{R367ofdm_STATUS_ERR_DA, 0xe0},
	{R367ofdm_DIG_AGC_R, 0xfe},
	{R367ofdm_COMAGC_TARMSB, 0x0b},
	{R367ofdm_COM_AGC_TAR_ENMODE, 0x41},
	{R367ofdm_COM_AGC_CFG, 0x3e},
	{R367ofdm_COM_AGC_GAIN1, 0x39},
	{R367ofdm_AUT_AGC_TARGETMSB, 0x0b},
	{R367ofdm_LOCK_DET_MSB, 0x01},
	{R367ofdm_AGCTAR_LOCK_LSBS, 0x40},
	{R367ofdm_AUT_GAIN_EN, 0xf4},
	{R367ofdm_AUT_CFG, 0xf0},
	{R367ofdm_LOCKN, 0x23},
	{R367ofdm_INT_X_3, 0x00},
	{R367ofdm_INT_X_2, 0x03},
	{R367ofdm_INT_X_1, 0x8d},
	{R367ofdm_INT_X_0, 0xa0},
	{R367ofdm_MIN_ERRX_MSB, 0x00},
	{R367ofdm_COR_CTL, 0x23},
	{R367ofdm_COR_STAT, 0xf6},
	{R367ofdm_COR_INTEN, 0x00},
	{R367ofdm_COR_INTSTAT, 0x3f},
	{R367ofdm_COR_MODEGUARD, 0x03},
	{R367ofdm_AGC_CTL, 0x08},
	{R367ofdm_AGC_MANUAL1, 0x00},
	{R367ofdm_AGC_MANUAL2, 0x00},
	{R367ofdm_AGC_TARG, 0x16},
	{R367ofdm_AGC_GAIN1, 0x53},
	{R367ofdm_AGC_GAIN2, 0x1d},
	{R367ofdm_RESERVED_1, 0x00},
	{R367ofdm_RESERVED_2, 0x00},
	{R367ofdm_RESERVED_3, 0x00},
	{R367ofdm_CAS_CTL, 0x44},
	{R367ofdm_CAS_FREQ, 0xb3},
	{R367ofdm_CAS_DAGCGAIN, 0x12},
	{R367ofdm_SYR_CTL, 0x04},
	{R367ofdm_SYR_STAT, 0x10},
	{R367ofdm_SYR_NCO1, 0x00},
	{R367ofdm_SYR_NCO2, 0x00},
	{R367ofdm_SYR_OFFSET1, 0x00},
	{R367ofdm_SYR_OFFSET2, 0x00},
	{R367ofdm_FFT_CTL, 0x00},
	{R367ofdm_SCR_CTL, 0x70},
	{R367ofdm_PPM_CTL1, 0xf8},
	{R367ofdm_TRL_CTL, 0xac},
	{R367ofdm_TRL_NOMRATE1, 0x1e},
	{R367ofdm_TRL_NOMRATE2, 0x58},
	{R367ofdm_TRL_TIME1, 0x1d},
	{R367ofdm_TRL_TIME2, 0xfc},
	{R367ofdm_CRL_CTL, 0x24},
	{R367ofdm_CRL_FREQ1, 0xad},
	{R367ofdm_CRL_FREQ2, 0x9d},
	{R367ofdm_CRL_FREQ3, 0xff},
	{R367ofdm_CHC_CTL, 0x01},
	{R367ofdm_CHC_SNR, 0xf0},
	{R367ofdm_BDI_CTL, 0x00},
	{R367ofdm_DMP_CTL, 0x00},
	{R367ofdm_TPS_RCVD1, 0x30},
	{R367ofdm_TPS_RCVD2, 0x02},
	{R367ofdm_TPS_RCVD3, 0x01},
	{R367ofdm_TPS_RCVD4, 0x00},
	{R367ofdm_TPS_ID_CELL1, 0x00},
	{R367ofdm_TPS_ID_CELL2, 0x00},
	{R367ofdm_TPS_RCVD5_SET1, 0x02},
	{R367ofdm_TPS_SET2, 0x02},
	{R367ofdm_TPS_SET3, 0x01},
	{R367ofdm_TPS_CTL, 0x00},
	{R367ofdm_CTL_FFTOSNUM, 0x34},
	{R367ofdm_TESTSELECT, 0x09},
	{R367ofdm_MSC_REV, 0x0a},
	{R367ofdm_PIR_CTL, 0x00},
	{R367ofdm_SNR_CARRIER1, 0xa1},
	{R367ofdm_SNR_CARRIER2, 0x9a},
	{R367ofdm_PPM_CPAMP, 0x2c},
	{R367ofdm_TSM_AP0, 0x00},
	{R367ofdm_TSM_AP1, 0x00},
	{R367ofdm_TSM_AP2, 0x00},
	{R367ofdm_TSM_AP3, 0x00},
	{R367ofdm_TSM_AP4, 0x00},
	{R367ofdm_TSM_AP5, 0x00},
	{R367ofdm_TSM_AP6, 0x00},
	{R367ofdm_TSM_AP7, 0x00},
	{R367_TSTRES, 0x00},
	{R367_ANACTRL, 0x0D}, /*caution PLL stopped, to be restarted at init!!!*/
	{R367_TSTBUS, 0x00},
	{R367ofdm_TSTRATE, 0x00},
	{R367ofdm_CONSTMODE, 0x01},
	{R367ofdm_CONSTCARR1, 0x00},
	{R367ofdm_CONSTCARR2, 0x00},
	{R367ofdm_ICONSTEL, 0x0a},
	{R367ofdm_QCONSTEL, 0x15},
	{R367ofdm_TSTBISTRES0, 0x00},
	{R367ofdm_TSTBISTRES1, 0x00},
	{R367ofdm_TSTBISTRES2, 0x28},
	{R367ofdm_TSTBISTRES3, 0x00},
	{R367_RF_AGC1, 0xff},
	{R367_RF_AGC2, 0x83},
	{R367_ANADIGCTRL, 0x19},
	{R367_PLLMDIV, 0x0c},
	{R367_PLLNDIV, 0x55},
	{R367_PLLSETUP, 0x18},
	{R367_DUAL_AD12, 0x00},
	{R367_TSTBIST, 0x00},
	{R367ofdm_PAD_COMP_CTRL, 0x00},
	{R367ofdm_PAD_COMP_WR, 0x00},
	{R367ofdm_PAD_COMP_RD, 0xe0},
	{R367ofdm_SYR_TARGET_FFTADJT_MSB, 0x00},
	{R367ofdm_SYR_TARGET_FFTADJT_LSB, 0x00},
	{R367ofdm_SYR_TARGET_CHCADJT_MSB, 0x00},
	{R367ofdm_SYR_TARGET_CHCADJT_LSB, 0x00},
	{R367ofdm_SYR_FLAG, 0x00},
	{R367ofdm_CRL_TARGET1, 0x00},
	{R367ofdm_CRL_TARGET2, 0x00},
	{R367ofdm_CRL_TARGET3, 0x00},
	{R367ofdm_CRL_TARGET4, 0x00},
	{R367ofdm_CRL_FLAG, 0x00},
	{R367ofdm_TRL_TARGET1, 0x00},
	{R367ofdm_TRL_TARGET2, 0x00},
	{R367ofdm_TRL_CHC, 0x00},
	{R367ofdm_CHC_SNR_TARG, 0x00},
	{R367ofdm_TOP_TRACK, 0x00},
	{R367ofdm_TRACKER_FREE1, 0x00},
	{R367ofdm_ERROR_CRL1, 0x00},
	{R367ofdm_ERROR_CRL2, 0x00},
	{R367ofdm_ERROR_CRL3, 0x00},
	{R367ofdm_ERROR_CRL4, 0x00},
	{R367ofdm_DEC_NCO1, 0x2c},
	{R367ofdm_DEC_NCO2, 0x0f},
	{R367ofdm_DEC_NCO3, 0x20},
	{R367ofdm_SNR, 0xf1},
	{R367ofdm_SYR_FFTADJ1, 0x00},
	{R367ofdm_SYR_FFTADJ2, 0x00},
	{R367ofdm_SYR_CHCADJ1, 0x00},
	{R367ofdm_SYR_CHCADJ2, 0x00},
	{R367ofdm_SYR_OFF, 0x00},
	{R367ofdm_PPM_OFFSET1, 0x00},
	{R367ofdm_PPM_OFFSET2, 0x03},
	{R367ofdm_TRACKER_FREE2, 0x00},
	{R367ofdm_DEBG_LT10, 0x00},
	{R367ofdm_DEBG_LT11, 0x00},
	{R367ofdm_DEBG_LT12, 0x00},
	{R367ofdm_DEBG_LT13, 0x00},
	{R367ofdm_DEBG_LT14, 0x00},
	{R367ofdm_DEBG_LT15, 0x00},
	{R367ofdm_DEBG_LT16, 0x00},
	{R367ofdm_DEBG_LT17, 0x00},
	{R367ofdm_DEBG_LT18, 0x00},
	{R367ofdm_DEBG_LT19, 0x00},
	{R367ofdm_DEBG_LT1A, 0x00},
	{R367ofdm_DEBG_LT1B, 0x00},
	{R367ofdm_DEBG_LT1C, 0x00},
	{R367ofdm_DEBG_LT1D, 0x00},
	{R367ofdm_DEBG_LT1E, 0x00},
	{R367ofdm_DEBG_LT1F, 0x00},
	{R367ofdm_RCCFGH, 0x00},
	{R367ofdm_RCCFGM, 0x00},
	{R367ofdm_RCCFGL, 0x00},
	{R367ofdm_RCINSDELH, 0x00},
	{R367ofdm_RCINSDELM, 0x00},
	{R367ofdm_RCINSDELL, 0x00},
	{R367ofdm_RCSTATUS, 0x00},
	{R367ofdm_RCSPEED, 0x6f},
	{R367ofdm_RCDEBUGM, 0xe7},
	{R367ofdm_RCDEBUGL, 0x9b},
	{R367ofdm_RCOBSCFG, 0x00},
	{R367ofdm_RCOBSM, 0x00},
	{R367ofdm_RCOBSL, 0x00},
	{R367ofdm_RCFECSPY, 0x00},
	{R367ofdm_RCFSPYCFG, 0x00},
	{R367ofdm_RCFSPYDATA, 0x00},
	{R367ofdm_RCFSPYOUT, 0x00},
	{R367ofdm_RCFSTATUS, 0x00},
	{R367ofdm_RCFGOODPACK, 0x00},
	{R367ofdm_RCFPACKCNT, 0x00},
	{R367ofdm_RCFSPYMISC, 0x00},
	{R367ofdm_RCFBERCPT4, 0x00},
	{R367ofdm_RCFBERCPT3, 0x00},
	{R367ofdm_RCFBERCPT2, 0x00},
	{R367ofdm_RCFBERCPT1, 0x00},
	{R367ofdm_RCFBERCPT0, 0x00},
	{R367ofdm_RCFBERERR2, 0x00},
	{R367ofdm_RCFBERERR1, 0x00},
	{R367ofdm_RCFBERERR0, 0x00},
	{R367ofdm_RCFSTATESM, 0x00},
	{R367ofdm_RCFSTATESL, 0x00},
	{R367ofdm_RCFSPYBER, 0x00},
	{R367ofdm_RCFSPYDISTM, 0x00},
	{R367ofdm_RCFSPYDISTL, 0x00},
	{R367ofdm_RCFSPYOBS7, 0x00},
	{R367ofdm_RCFSPYOBS6, 0x00},
	{R367ofdm_RCFSPYOBS5, 0x00},
	{R367ofdm_RCFSPYOBS4, 0x00},
	{R367ofdm_RCFSPYOBS3, 0x00},
	{R367ofdm_RCFSPYOBS2, 0x00},
	{R367ofdm_RCFSPYOBS1, 0x00},
	{R367ofdm_RCFSPYOBS0, 0x00},
	{R367ofdm_TSGENERAL, 0x00},
	{R367ofdm_RC1SPEED, 0x6f},
	{R367ofdm_TSGSTATUS, 0x18},
	{R367ofdm_FECM, 0x01},
	{R367ofdm_VTH12, 0xff},
	{R367ofdm_VTH23, 0xa1},
	{R367ofdm_VTH34, 0x64},
	{R367ofdm_VTH56, 0x40},
	{R367ofdm_VTH67, 0x00},
	{R367ofdm_VTH78, 0x2c},
	{R367ofdm_VITCURPUN, 0x12},
	{R367ofdm_VERROR, 0x01},
	{R367ofdm_PRVIT, 0x3f},
	{R367ofdm_VAVSRVIT, 0x00},
	{R367ofdm_VSTATUSVIT, 0xbd},
	{R367ofdm_VTHINUSE, 0xa1},
	{R367ofdm_KDIV12, 0x20},
	{R367ofdm_KDIV23, 0x40},
	{R367ofdm_KDIV34, 0x20},
	{R367ofdm_KDIV56, 0x30},
	{R367ofdm_KDIV67, 0x00},
	{R367ofdm_KDIV78, 0x30},
	{R367ofdm_SIGPOWER, 0x54},
	{R367ofdm_DEMAPVIT, 0x40},
	{R367ofdm_VITSCALE, 0x00},
	{R367ofdm_FFEC1PRG, 0x00},
	{R367ofdm_FVITCURPUN, 0x12},
	{R367ofdm_FVERROR, 0x01},
	{R367ofdm_FVSTATUSVIT, 0xbd},
	{R367ofdm_DEBUG_LT1, 0x00},
	{R367ofdm_DEBUG_LT2, 0x00},
	{R367ofdm_DEBUG_LT3, 0x00},
	{R367ofdm_TSTSFMET, 0x00},
	{R367ofdm_SELOUT, 0x00},
	{R367ofdm_TSYNC, 0x00},
	{R367ofdm_TSTERR, 0x00},
	{R367ofdm_TSFSYNC, 0x00},
	{R367ofdm_TSTSFERR, 0x00},
	{R367ofdm_TSTTSSF1, 0x01},
	{R367ofdm_TSTTSSF2, 0x1f},
	{R367ofdm_TSTTSSF3, 0x00},
	{R367ofdm_TSTTS1, 0x00},
	{R367ofdm_TSTTS2, 0x1f},
	{R367ofdm_TSTTS3, 0x01},
	{R367ofdm_TSTTS4, 0x00},
	{R367ofdm_TSTTSRC, 0x00},
	{R367ofdm_TSTTSRS, 0x00},
	{R367ofdm_TSSTATEM, 0xb0},
	{R367ofdm_TSSTATEL, 0x40},
	{R367ofdm_TSCFGH, 0x80},
	{R367ofdm_TSCFGM, 0x00},
	{R367ofdm_TSCFGL, 0x20},
	{R367ofdm_TSSYNC, 0x00},
	{R367ofdm_TSINSDELH, 0x00},
	{R367ofdm_TSINSDELM, 0x00},
	{R367ofdm_TSINSDELL, 0x00},
	{R367ofdm_TSDIVN, 0x03},
	{R367ofdm_TSDIVPM, 0x00},
	{R367ofdm_TSDIVPL, 0x00},
	{R367ofdm_TSDIVQM, 0x00},
	{R367ofdm_TSDIVQL, 0x00},
	{R367ofdm_TSDILSTKM, 0x00},
	{R367ofdm_TSDILSTKL, 0x00},
	{R367ofdm_TSSPEED, 0x6f},
	{R367ofdm_TSSTATUS, 0x81},
	{R367ofdm_TSSTATUS2, 0x6a},
	{R367ofdm_TSBITRATEM, 0x0f},
	{R367ofdm_TSBITRATEL, 0xc6},
	{R367ofdm_TSPACKLENM, 0x00},
	{R367ofdm_TSPACKLENL, 0xfc},
	{R367ofdm_TSBLOCLENM, 0x0a},
	{R367ofdm_TSBLOCLENL, 0x80},
	{R367ofdm_TSDLYH, 0x90},
	{R367ofdm_TSDLYM, 0x68},
	{R367ofdm_TSDLYL, 0x01},
	{R367ofdm_TSNPDAV, 0x00},
	{R367ofdm_TSBUFSTATH, 0x00},
	{R367ofdm_TSBUFSTATM, 0x00},
	{R367ofdm_TSBUFSTATL, 0x00},
	{R367ofdm_TSDEBUGM, 0xcf},
	{R367ofdm_TSDEBUGL, 0x1e},
	{R367ofdm_TSDLYSETH, 0x00},
	{R367ofdm_TSDLYSETM, 0x68},
	{R367ofdm_TSDLYSETL, 0x00},
	{R367ofdm_TSOBSCFG, 0x00},
	{R367ofdm_TSOBSM, 0x47},
	{R367ofdm_TSOBSL, 0x1f},
	{R367ofdm_ERRCTRL1, 0x95},
	{R367ofdm_ERRCNT1H, 0x80},
	{R367ofdm_ERRCNT1M, 0x00},
	{R367ofdm_ERRCNT1L, 0x00},
	{R367ofdm_ERRCTRL2, 0x95},
	{R367ofdm_ERRCNT2H, 0x00},
	{R367ofdm_ERRCNT2M, 0x00},
	{R367ofdm_ERRCNT2L, 0x00},
	{R367ofdm_FECSPY, 0x88},
	{R367ofdm_FSPYCFG, 0x2c},
	{R367ofdm_FSPYDATA, 0x3a},
	{R367ofdm_FSPYOUT, 0x06},
	{R367ofdm_FSTATUS, 0x61},
	{R367ofdm_FGOODPACK, 0xff},
	{R367ofdm_FPACKCNT, 0xff},
	{R367ofdm_FSPYMISC, 0x66},
	{R367ofdm_FBERCPT4, 0x00},
	{R367ofdm_FBERCPT3, 0x00},
	{R367ofdm_FBERCPT2, 0x36},
	{R367ofdm_FBERCPT1, 0x36},
	{R367ofdm_FBERCPT0, 0x14},
	{R367ofdm_FBERERR2, 0x00},
	{R367ofdm_FBERERR1, 0x03},
	{R367ofdm_FBERERR0, 0x28},
	{R367ofdm_FSTATESM, 0x00},
	{R367ofdm_FSTATESL, 0x02},
	{R367ofdm_FSPYBER, 0x00},
	{R367ofdm_FSPYDISTM, 0x01},
	{R367ofdm_FSPYDISTL, 0x9f},
	{R367ofdm_FSPYOBS7, 0xc9},
	{R367ofdm_FSPYOBS6, 0x99},
	{R367ofdm_FSPYOBS5, 0x08},
	{R367ofdm_FSPYOBS4, 0xec},
	{R367ofdm_FSPYOBS3, 0x01},
	{R367ofdm_FSPYOBS2, 0x0f},
	{R367ofdm_FSPYOBS1, 0xf5},
	{R367ofdm_FSPYOBS0, 0x08},
	{R367ofdm_SFDEMAP, 0x40},
	{R367ofdm_SFERROR, 0x00},
	{R367ofdm_SFAVSR, 0x30},
	{R367ofdm_SFECSTATUS, 0xcc},
	{R367ofdm_SFKDIV12, 0x20},
	{R367ofdm_SFKDIV23, 0x40},
	{R367ofdm_SFKDIV34, 0x20},
	{R367ofdm_SFKDIV56, 0x20},
	{R367ofdm_SFKDIV67, 0x00},
	{R367ofdm_SFKDIV78, 0x20},
	{R367ofdm_SFDILSTKM, 0x00},
	{R367ofdm_SFDILSTKL, 0x00},
	{R367ofdm_SFSTATUS, 0xb5},
	{R367ofdm_SFDLYH, 0x90},
	{R367ofdm_SFDLYM, 0x60},
	{R367ofdm_SFDLYL, 0x01},
	{R367ofdm_SFDLYSETH, 0xc0},
	{R367ofdm_SFDLYSETM, 0x60},
	{R367ofdm_SFDLYSETL, 0x00},
	{R367ofdm_SFOBSCFG, 0x00},
	{R367ofdm_SFOBSM, 0x47},
	{R367ofdm_SFOBSL, 0x05},
	{R367ofdm_SFECINFO, 0x40},
	{R367ofdm_SFERRCTRL, 0x74},
	{R367ofdm_SFERRCNTH, 0x80},
	{R367ofdm_SFERRCNTM, 0x00},
	{R367ofdm_SFERRCNTL, 0x00},
	{R367ofdm_SYMBRATEM, 0x2f},
	{R367ofdm_SYMBRATEL, 0x50},
	{R367ofdm_SYMBSTATUS, 0x7f},
	{R367ofdm_SYMBCFG, 0x00},
	{R367ofdm_SYMBFIFOM, 0xf4},
	{R367ofdm_SYMBFIFOL, 0x0d},
	{R367ofdm_SYMBOFFSM, 0xf0},
	{R367ofdm_SYMBOFFSL, 0x2d},
	{R367ofdm_DEBUG_LT4, 0x00},
	{R367ofdm_DEBUG_LT5, 0x00},
	{R367ofdm_DEBUG_LT6, 0x00},
	{R367ofdm_DEBUG_LT7, 0x00},
	{R367ofdm_DEBUG_LT8, 0x00},
	{R367ofdm_DEBUG_LT9, 0x00},
};

/*------------------------------------------------------------------------------*/

FE_LLA_Error_t FE_367ofdm_Algo(YWTUNER_Handle_T Handle, FE_367ofdm_InternalParams_t *pParams, FE_TER_SearchResult_t *pResult);
/*****************************************************
**FUNCTION :: PowOf2
**ACTION :: Compute 2^n (where n is an integer)
**PARAMS IN :: number -> n
**PARAMS OUT:: NONE
**RETURN :: 2^n
*****************************************************/
S32 PowOf2(S32 number)
{
	S32 i;
	S32 result = 1;
	for (i = 0; i < number; i++)
		result *= 2;
	return result;
}

/***********************************************************************
 函数名称: demod_d0367ter_Repeat

 函数说明: 0367ter的转发功能

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367ter_Repeat(IOARCH_Handle_t DemodIOHandle,
									 IOARCH_Handle_t TunerIOHandle,
									 TUNER_IOARCH_Operation_t Operation,
									 unsigned short SubAddr,
									 U8 *Data,
									 U32 TransferSize,
									 U32 Timeout)
{
	return 0;
}

#ifdef TUNER_USE_TER_STI7167TER

void D0367ter_Init(TUNER_IOREG_DeviceMap_t *DeviceMap,
				   IOARCH_Handle_t IOHandle,
				   TUNER_TunerType_T TunerType)
{
	U16 i;
	for (i = 0; i < DeviceMap->Registers; i++)
	{
		ChipUpdateDefaultValues_0367ter(DeviceMap, IOHandle, Def367Val[i].Addr, Def367Val[i].Value, i);
		ChipSetOneRegister_0367ter(DeviceMap, IOHandle, Def367Val[i].Addr, Def367Val[i].Value);
	}
	if (DeviceMap != NULL)
	{
		/*----------------------------------------------------------------------------------------*/
		switch (TunerType)
		{
			case TUNER_TUNER_STV4100:
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_TOPCTRL, 0x2); /* Buffer Q enabled */
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_DUAL_AD12, 0x00); /* ADCQ enabled */
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_ANACTRL, 0x00);
				break;
			case TUNER_TUNER_SHARP6465:
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_ANACTRL, 0x0D); /* PLL bypassed and disabled */
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_PLLMDIV, 0x01); /* IC runs at 54MHz with a 27MHz crystal */
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_PLLNDIV, 0x08);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_PLLSETUP, 0x18); /* ADC clock is equal to system clock */
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_TOPCTRL, 0x0); /* Buffer Q disabled */
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_DUAL_AD12, 0x04); /* ADCQ disabled */
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC1, 0x2A);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC2, 0xD6);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_INC_DEROT1, 0x55);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_INC_DEROT2, 0x55);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_CTL, 0x14);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE1, 0xAE);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE2, 0x56);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_FEPATH_CFG, 0x0);
				ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367_ANACTRL, 0x00);
				break;
			default:
				break;
		}
	}
	FE_STV0367TER_SetCLKgen(DeviceMap, IOHandle, DeviceMap->RegExtClk);
	FE_STV0367ofdm_SetTS_Parallel_Serial(DeviceMap, IOHandle, FE_TS_PARALLEL_PUNCT_CLOCK);
	FE_STV0367ofdm_SetCLK_Polarity(DeviceMap, IOHandle, FE_TS_RISINGEDGE_CLOCK);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_ADCGP, 1);
	ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_ADCGP, 0);
}

/***********************************************************************
 函数名称: demod_d0367ter_Open

 函数说明: 打开0367ter，初始化寄存器

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367ter_Open(U8 Handle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	//printk("demod_d0367ter_Open ===== \n");
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	/*------------------驱动函数指针----------------*/
	Inst->DriverParam.Ter.DemodDriver.Demod_GetSignalInfo = demod_d0367ter_GetSignalInfo;
	Inst->DriverParam.Ter.DemodDriver.Demod_IsLocked = demod_d0367ter_IsLocked;
	Inst->DriverParam.Ter.DemodDriver.Demod_repeat = demod_d0367ter_Repeat;
	Inst->DriverParam.Ter.DemodDriver.Demod_reset = demod_d0367ter_Reset;
	Inst->DriverParam.Ter.DemodDriver.Demod_ScanFreq = demod_d0367ter_ScanFreq;
	Inst->DriverParam.Ter.DemodDriver.Demod_standy = demod_d0367ter_SetStandby;
	/*-----------------寄存器结构分配--------------*/
	DeviceMap->Timeout = IOREG_DEFAULT_TIMEOUT;
	DeviceMap->Registers = STV0367ofdm_NBREGS;
	DeviceMap->Fields = STV0367ofdm_NBFIELDS;
	DeviceMap->Mode = IOREG_MODE_SUBADR_16;
	DeviceMap->RegExtClk = Inst->ExternalClock; //Demod External Crystal_HZ
	//Error = TUNER_IOREG_Open(DeviceMap);
	DeviceMap->DefVal = NULL;
	DeviceMap->Error = 0;
	D0367ter_Init(DeviceMap, IOHandle, TUNER_TUNER_SHARP6465);
	return (Error);
}

YW_ErrorType_T demod_d0367ter_Open_test(IOARCH_Handle_t IOHandle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_IOREG_DeviceMap_t DeviceMap;
	//printk("demod_d0367ter_Open ===== \n");
	/*-----------------寄存器结构分配--------------*/
	DeviceMap.Timeout = IOREG_DEFAULT_TIMEOUT;
	DeviceMap.Registers = STV0367ofdm_NBREGS;
	DeviceMap.Fields = STV0367ofdm_NBFIELDS;
	DeviceMap.Mode = IOREG_MODE_SUBADR_16;
	DeviceMap.RegExtClk = 27000000; //Demod External Crystal_HZ
	//Error = TUNER_IOREG_Open(&DeviceMap);
	DeviceMap.DefVal = NULL;
	DeviceMap.Error = 0;
	D0367ter_Init(&DeviceMap, IOHandle, TUNER_TUNER_SHARP6465);
	//Error = TUNER_IOREG_Close(&DeviceMap);
	return (Error);
}
/***********************************************************************
 函数名称: demod_d0367ter_Close

 函数说明: 关闭0367ter

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.16 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367ter_Close(U8 Handle)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	Inst = TUNER_GetScanInfo(Handle);
	Inst->DriverParam.Ter.DemodDriver.Demod_GetSignalInfo = NULL;
	Inst->DriverParam.Ter.DemodDriver.Demod_IsLocked = NULL;
	Inst->DriverParam.Ter.DemodDriver.Demod_repeat = NULL;
	Inst->DriverParam.Ter.DemodDriver.Demod_reset = NULL;
	Inst->DriverParam.Ter.DemodDriver.Demod_ScanFreq = NULL;
	//Error = TUNER_IOREG_Close(&Inst->DriverParam.Ter.Demod_DeviceMap);
	Error |= Inst->DriverParam.Ter.Demod_DeviceMap.Error;
	Inst->DriverParam.Ter.Demod_DeviceMap.Error = YW_NO_ERROR;
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367_ter_Reset

 函数说明: 复位353 寄存器

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367ter_Reset(U8 Index)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	Inst = TUNER_GetScanInfo(Index);
	IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	D0367ter_Init(DeviceMap, IOHandle, Inst->DriverParam.Ter.TunerType);
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367ter_GetSignalInfo

 函数说明: 读取信号质量与强度

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367ter_GetSignalInfo(U8 Index, U32 *Quality, U32 *Intensity, U32 *Ber)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	*Quality = 0;
	*Intensity = 0;
	*Ber = 0;
	//FE_STV0367TER_GetSignalInfo(Index, Quality, Intensity, Ber);
	// printk("demod_d0367ter_GetSignalInfo %d, Quality = %d, Intensity = %d\n",Index,*Quality,*Intensity);
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367ter_IsLocked

 函数说明: 读取信号是否锁定

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367ter_IsLocked(U8 Handle, BOOL *IsLocked)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	*IsLocked = FE_367ofdm_lock(DeviceMap, IOHandle);
	return (Error);
}

/***********************************************************************
 函数名称: demod_d0367ter_ScanFreq

 函数说明: 搜索频率

 修改记录: 日 期 作 者 修定
 --------- --------- -----
 2010.11.11 lwj 创建
************************************************************************/
YW_ErrorType_T demod_d0367ter_ScanFreq(U8 Index)
{
	YW_ErrorType_T Error = YW_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst;
	IOARCH_Handle_t IOHandle;
	TUNER_IOREG_DeviceMap_t *DeviceMap;
	/*U8 trials[2]; */
	S8 num_trials = 1;
	U8 index;
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	U8 flag_spec_inv;
	U8 flag;
	U8 SenseTrials[2];
	U8 SenseTrialsAuto[2];
	FE_367ofdm_InternalParams_t pParams;
	FE_TER_SearchResult_t pResult;
	//question pParams->Inv 多少
	Inst = TUNER_GetScanInfo(Index);
	IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	YWLIB_Memset(&pParams, 0, sizeof(FE_367ofdm_InternalParams_t));
	YWLIB_Memset(&pResult, 0, sizeof(FE_TER_SearchResult_t));
#if 0 //question
	SenseTrials[0] = INV;
	SenseTrials[1] = NINV;
	SenseTrialsAuto[0] = INV;
	SenseTrialsAuto[1] = NINV;
#else
	SenseTrials[1] = INV;
	SenseTrials[0] = NINV;
	SenseTrialsAuto[1] = INV;
	SenseTrialsAuto[0] = NINV;
#endif
	switch (Inst->DriverParam.Ter.Param.ChannelBW)
	{
		case YWTUNER_TER_BANDWIDTH_8_MHZ:
			pParams.ChannelBW = 8;
			break;
		case YWTUNER_TER_BANDWIDTH_7_MHZ:
			pParams.ChannelBW = 7;
			break;
		case YWTUNER_TER_BANDWIDTH_6_MHZ:
			pParams.ChannelBW = 6;
			break;
	}
	// printk("demod_d0367ter_ScanFreq #########Index = %d\n", Index);
	pParams.Frequency = Inst->DriverParam.Ter.Param.FreqKHz;
	pParams.Crystal_Hz = DeviceMap->RegExtClk;
	pParams.IF_IQ_Mode = FE_TER_NORMAL_IF_TUNER;//FE_TER_IQ_TUNER; //most tuner is IF mode, stv4100 is I/Q mode
	pParams.Inv = FE_TER_INVERSION_AUTO; //FE_TER_INVERSION_AUTO
	pParams.Hierarchy = FE_TER_HIER_NONE;
	pParams.EchoPos = 0;
	pParams.first_lock = 0;
	//pLook.Frequency = pSearch->Frequency;
	//pLook.Mode = pSearch->Mode;
	//pLook.Guard = pSearch->Guard;
	//pLook.Force = pSearch->Force;
	//pLook.ChannelBW = pSearch->ChannelBW;
	//pLook.EchoPos = pSearch->EchoPos;
	//pLook.IF_IQ_Mode= pSearch->IF_IQ_Mode;
	// pParams->Inv = pSearch->Inv;
	//pLook.Hierarchy=pParams->Hierarchy = pSearch->Hierarchy; /*added for hierarchical*/
	/*printk(" in FE_367TER_Search () value of pParams.Hierarchy %d\n",pParams.Hierarchy );*/
	flag_spec_inv = 0;
	flag = ((pParams.Inv >> 1) & 1);
	switch (flag) /* sw spectrum inversion for LP_IF &IQ &normal IF *db* */
	{
		case 0: /*INV+ INV_NONE*/
			if ((pParams.Inv == FE_TER_INVERSION_NONE) || (pParams.Inv == FE_TER_INVERSION))
			{
				num_trials = 1;
			}
			else /*UNK */
			{
				num_trials = 2;
			}
			break;
		case 1:/*AUTO*/
			num_trials = 2;
			if ((pParams.first_lock) && (pParams.Inv == FE_TER_INVERSION_AUTO))
			{
				num_trials = 1;
			}
			break;
		default:
			return FE_TER_NOLOCK;
			break;
	}
	pResult.SignalStatus = FE_TER_NOLOCK;
	index = 0;
	while (((index) < num_trials) && (pResult.SignalStatus != FE_TER_LOCKOK))
	{
		if (!pParams.first_lock)
		{
			if (pParams.Inv == FE_TER_INVERSION_UNK)
			{
				pParams.Sense = SenseTrials[index];
			}
			if (pParams.Inv == FE_TER_INVERSION_AUTO)
			{
				pParams.Sense = SenseTrialsAuto[index];
			}
		}
		error = FE_367ofdm_Algo(Index, &pParams, &pResult);
		if ((pResult.SignalStatus == FE_TER_LOCKOK) && (pParams.Inv == FE_TER_INVERSION_AUTO) && (index == 1))
		{
			SenseTrialsAuto[index] = SenseTrialsAuto[0]; /* invert spectrum sense */
			SenseTrialsAuto[(index + 1) % 2] = (SenseTrialsAuto[1] + 1) % 2;
		}
		index++;
	}
	if (!Error)
	{
		//Inst->DriverParam.Ter.Result = Inst->DriverParam.Ter.Param;
		if (pResult.Locked)
		{
			//printk("TUNER_STATUS_LOCKED #######################\n");
			Inst->Status = TUNER_INNER_STATUS_LOCKED;
		}
		else
		{
			//printk("TUNER_STATUS_UNLOCKED #######################\n");
			Inst->Status = TUNER_INNER_STATUS_UNLOCKED;
		}
	}
	else
	{
		Inst->Status = TUNER_INNER_STATUS_UNLOCKED;
	}
	return (Error);
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
FE_LLA_Error_t FE_367ofdm_Algo(YWTUNER_Handle_T Handle, FE_367ofdm_InternalParams_t *pParams, FE_TER_SearchResult_t *pResult)
{
	U32 InternalFreq = 0, temp = 0, intX = 0;
	int AgcIF = 0;
#if 0
	int offset = 0, tempo = 0;
	unsigned short int u_var; /*added for HM *db*/
	U8 constell, counter, tps_rcvd[2];
	S8 step;
	/*BOOL SpectrumInversion=FALSE;*/
	S32 timing_offset = 0;
	U32 trl_nomrate = 0;
#endif
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	BOOL TunerLocked;
	TUNER_ScanTaskParam_T *Inst = NULL;
	TUNER_IOREG_DeviceMap_t *DeviceMap = NULL;
	IOARCH_Handle_t IOHandle;
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	if (Inst->ForceSearchTerm)
	{
		return (FE_LLA_INVALID_HANDLE);
	}
	if (pParams != NULL)
	{
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_CCS_ENABLE, 0);
		//printk("pIntParams->IF_IQ_Mode == %d\n",pParams->IF_IQ_Mode);
		switch (pParams->IF_IQ_Mode)
		{
			case FE_TER_NORMAL_IF_TUNER: /* Normal IF mode */
				ChipSetField_0367ter(DeviceMap, IOHandle, F367_TUNER_BB, 0);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_LONGPATH_IF, 0);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_DEMUX_SWAP, 0);
				break;
			case FE_TER_LONGPATH_IF_TUNER: /* Long IF mode */
				ChipSetField_0367ter(DeviceMap, IOHandle, F367_TUNER_BB, 0);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_LONGPATH_IF, 1);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_DEMUX_SWAP, 1);
				break;
			case FE_TER_IQ_TUNER: /* IQ mode */
				ChipSetField_0367ter(DeviceMap, IOHandle, F367_TUNER_BB, 1);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_PPM_INVSEL, 0); /*spectrum inversion hw detection off *db*/
				break;
			default:
				return FE_LLA_SEARCH_FAILED;
		}
		//printk("pIntParams->Inv == %d\n",pParams->Inv);
		//printk("pIntParams->Sense == %d\n",pParams->Sense);
		if ((pParams->Inv == FE_TER_INVERSION_NONE))
		{
			if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 0);
				pParams->SpectrumInversion = FALSE;
			}
			else
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 0);
				pParams->SpectrumInversion = FALSE;
			}
		}
		else if (pParams->Inv == FE_TER_INVERSION)
		{
			if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 1);
				pParams->SpectrumInversion = TRUE;
			}
			else
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 1);
				pParams->SpectrumInversion = TRUE;
			}
		}
		else if ((pParams->Inv == FE_TER_INVERSION_AUTO) || ((pParams->Inv == FE_TER_INVERSION_UNK)/*&& (!pParams->first_lock)*/))
		{
			if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
			{
				if (pParams->Sense == 1)
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 1);
					pParams->SpectrumInversion = TRUE;
				}
				else
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_IQ_INVERT, 0);
					pParams->SpectrumInversion = FALSE;
				}
			}
			else
			{
				if (pParams->Sense == 1)
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 1);
					pParams->SpectrumInversion = TRUE;
				}
				else
				{
					ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_INV_SPECTR, 0);
					pParams->SpectrumInversion = FALSE;
				}
			}
		}
		// printk("pIntParams->PreviousChannelBW == %d\n",pParams->PreviousChannelBW);
		// printk("pIntParams->Crystal_Hz == %d\n",pParams->Crystal_Hz);
		if ((pParams->IF_IQ_Mode != FE_TER_NORMAL_IF_TUNER) && (pParams->PreviousChannelBW != pParams->ChannelBW))
		{
			FE_367TER_AGC_IIR_LOCK_DETECTOR_SET(DeviceMap, IOHandle);
			/*set fine agc target to 180 for LPIF or IQ mode*/
			/* set Q_AGCTarget */
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_SEL_IQNTAR, 1);
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_AUT_AGC_TARGET_MSB, 0xB);
			/*ChipSetField_0367ter(DeviceMap, IOHandle,AUT_AGC_TARGET_LSB,0x04); */
			/* set Q_AGCTarget */
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_SEL_IQNTAR, 0);
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_AUT_AGC_TARGET_MSB, 0xB);
			/*ChipSetField_0367ter(DeviceMap, IOHandle,AUT_AGC_TARGET_LSB,0x04); */
			if (!FE_367TER_IIR_FILTER_INIT(DeviceMap, IOHandle, pParams->ChannelBW, pParams->Crystal_Hz))
				return FE_LLA_BAD_PARAMETER;
			/*set IIR filter once for 6,7 or 8MHz BW*/
			pParams->PreviousChannelBW = pParams->ChannelBW;
			FE_367TER_AGC_IIR_RESET(DeviceMap, IOHandle);
		}
		//printk("pIntParams->Hierarchy == %d\n",pParams->Hierarchy);
		/*********Code Added For Hierarchical Modulation****************/
		if (pParams->Hierarchy == FE_TER_HIER_LOW_PRIO)
		{
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_BDI_LPSEL, 0x01);
		}
		else
		{
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_BDI_LPSEL, 0x00);
		}
		/*************************/
		InternalFreq = FE_367ofdm_GetMclkFreq(DeviceMap, IOHandle, pParams->Crystal_Hz) / 1000;
		//printk("InternalFreq = %d\n", InternalFreq);
		temp = (int)((((pParams->ChannelBW * 64 * PowOf2(15) * 100) / (InternalFreq)) * 10) / 7);
		//printk("pParams->ChannelBW = %d\n",pParams->ChannelBW);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LSB, temp % 2);
		temp = temp / 2;
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_HI, temp / 256);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LO, temp % 256);
		//ChipSetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_TRL_NOMRATE1,2);//lwj
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE1, 1);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE2, 1);
		//ChipGetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_TRL_CTL,3);//lwj
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_CTL, 1);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE1, 1);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_TRL_NOMRATE2, 1);
		temp = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_HI) * 512 + ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LO) * 2 +
			   ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LSB);
		temp = (int)((PowOf2(17) * pParams->ChannelBW * 1000) / (7 * (InternalFreq)));
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_HI, temp / 256);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_LO, temp % 256);
		//ChipSetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_GAIN_SRC1,2);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC1, 1);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC2, 1);
		//ChipGetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_GAIN_SRC1,2);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC1, 1);
		ChipGetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_GAIN_SRC2, 1);
		temp = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_HI) * 256 + ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_GAIN_SRC_LO);
		if (Inst->DriverParam.Ter.TunerType == TUNER_TUNER_SHARP6465)
		{
			temp = (int)((InternalFreq - 36166667 / 1000) * PowOf2(16) / (InternalFreq)); //IF freq
		}
		else if (Inst->DriverParam.Ter.TunerType == TUNER_TUNER_STV4100)
		{
			temp = (int)((InternalFreq - 0) * PowOf2(16) / (InternalFreq));
		}
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_INC_DEROT_HI, temp / 256);
		ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_INC_DEROT_LO, temp % 256);
		//ChipSetRegisters_0367ter(DeviceMap, IOHandle,R367ofdm_INC_DEROT1,2);//lwj change
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_INC_DEROT1, 1);
		ChipSetRegisters_0367ter(DeviceMap, IOHandle, R367ofdm_INC_DEROT2, 1);
		//pParams->EchoPos = pSearch->EchoPos;
		ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_LONG_ECHO, pParams->EchoPos);
		if (Inst->DriverParam.Ter.TunerType == TUNER_TUNER_STV4100)
		{
			ChipSetOneRegister_0367ter(DeviceMap, IOHandle, R367ofdm_AGC_TARG, 0x16);
			if (Inst->DriverParam.Ter.TunerDriver.tuner_StartAndCalibrate != NULL)
			{
				error = Inst->DriverParam.Ter.TunerDriver.tuner_StartAndCalibrate(Handle);
				if (error != YW_NO_ERROR)
				{
					return error;
				}
			}
			if (Inst->DriverParam.Ter.TunerDriver.tuner_SetBandWith != NULL)
			{
				error = Inst->DriverParam.Ter.TunerDriver.tuner_SetBandWith(Handle, pParams->ChannelBW);
				if (error != YW_NO_ERROR)
				{
					return error;
				}
			}
			if (Inst->DriverParam.Ter.TunerDriver.tuner_SetFreq != NULL)
			{
				error = (Inst->DriverParam.Ter.TunerDriver.tuner_SetFreq)(Handle, pParams->Frequency + 100, pParams->ChannelBW, NULL);
				if (error != YW_NO_ERROR)
				{
					return error;
				}
			}
			if (Inst->DriverParam.Ter.TunerDriver.tuner_GetFreq != NULL)
			{
				pParams->Frequency = (Inst->DriverParam.Ter.TunerDriver.tuner_GetFreq)(Handle, NULL);
			}
			ChipWaitOrAbort_0367ter(FALSE, 66);
			AgcIF = ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_LO) +
					(ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_HI) << 8);
			intX = (ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_INT_X3) << 24) +
				   (ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_INT_X2) << 16) +
				   (ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_INT_X1) << 8) +
				   ChipGetField_0367ter(DeviceMap, IOHandle, F367ofdm_INT_X0) ;
			if ((AgcIF > 0x500) && (intX > 0x50000) && (AgcIF < 0xc00))
			{
				if (Inst->DriverParam.Ter.TunerDriver.tuner_SetLna != NULL)
				{
					error = Inst->DriverParam.Ter.TunerDriver.tuner_SetLna(Handle);
					if (error != YW_NO_ERROR)
					{
						return error;
					}
				}
			}
			else
			{
				if (Inst->DriverParam.Ter.TunerDriver.tuner_AdjustRfPower != NULL)
				{
					error = Inst->DriverParam.Ter.TunerDriver.tuner_AdjustRfPower(Handle, 0);
					if (error != YW_NO_ERROR)
					{
						return error;
					}
				}
			}
// printk("%d,%s\n", __LINE__,__FUNCTION__);
			/* printk("estimated power=%03d\n",
			 FE_TunerEstimateRfPower(pParams->hTuner,ChipGetField(pParams->hDemod,F362_RF_AGC1_LEVEL_HI),AgcIF) ); */
			/* core active */
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_NRST_IIR, 0); /*reset filter to avoid saturation*/
			/* ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_CORE_ACTIVE,0);*/
			/* ChipWaitOrAbort(pParams->hDemod,20);*/
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_NRST_IIR, 1);
			/* ChipSetField_0367ter(DeviceMap, IOHandle,F367ofdm_CORE_ACTIVE,1);*/
// printk("%d,%s\n", __LINE__,__FUNCTION__);
		}
		else
		{
			if (Inst->DriverParam.Ter.TunerDriver.tuner_SetFreq != NULL)
			{
				error = (Inst->DriverParam.Ter.TunerDriver.tuner_SetFreq)(Handle, pParams->Frequency, pParams->ChannelBW, NULL);
				if (error != YW_NO_ERROR)
				{
					return error;
				}
			}
			if (Inst->DriverParam.Ter.TunerDriver.tuner_GetFreq != NULL)
			{
				(Inst->DriverParam.Ter.TunerDriver.tuner_GetFreq)(Handle, &(pParams->Frequency));
			}
		}
		if (Inst->DriverParam.Ter.TunerDriver.tuner_IsLocked != NULL)
		{
			Inst->DriverParam.Ter.TunerDriver.tuner_IsLocked(Handle, &TunerLocked);
		}
		/*********************************/
		if (FE_367ofdm_LockAlgo(DeviceMap, IOHandle, pParams) == FE_TER_LOCKOK)
		{
			pResult->Locked = TRUE;
			pResult->SignalStatus = FE_TER_LOCKOK;
#if 0
			/* update results */
			/*********** dans search term auparavant **********/
			tps_rcvd[0] = ChipGetOneRegister(pParams->hDemod, R367ofdm_TPS_RCVD2);
			tps_rcvd[1] = ChipGetOneRegister(pParams->hDemod, R367ofdm_TPS_RCVD3);
			ChipGetRegisters(pParams->hDemod, R367ofdm_SYR_STAT, 1);
			pResult->Mode = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_MODE);
			pResult->Guard = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_SYR_GUARD);
			constell = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TPS_CONST);
			if (constell == 0) pResult->Modulation = FE_TER_MOD_QPSK;
			else if (constell == 1) pResult->Modulation = FE_TER_MOD_16QAM;
			else pResult->Modulation = FE_TER_MOD_64QAM;
			/***Code replced and changed for HM**/
			pResult->hier = pParams->Hierarchy;
			pResult->Hierarchy_Alpha = (tps_rcvd[0] & 0x70) >> 4;
			/****/
			pResult->HPRate = tps_rcvd[1] & 0x07;
			pResult->LPRate = (tps_rcvd[1] & 0x70) >> 4;
			/****/
			constell = ChipGetField(pParams->hDemod, F367ofdm_PR);
			if (constell == 5) constell = 4;
			pResult->pr = (FE_TER_Rate_t) constell;
			if (pParams->IF_IQ_Mode == FE_TER_IQ_TUNER)
			{
				pResult->Sense = ChipGetField(pParams->hDemod, F367ofdm_IQ_INVERT);
			}
			else
			{
				pResult->Sense = ChipGetField(pParams->hDemod, F367ofdm_INV_SPECTR);
			}
			/* dcdc modifs per Gilles*/
			pParams->first_lock = 1;
			/* modifs Tuner */
			ChipGetRegisters(pParams->hDemod, R367ofdm_AGC2MAX, 13);
			pResult->Agc_val = (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_AGC1_VAL_LO) << 16) +
							   (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_AGC1_VAL_HI) << 24) +
							   ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_LO) +
							   (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_AGC2_VAL_HI) << 8);
			/* Carrier offset calculation */
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_FREEZE, 1);
			ChipGetRegisters(pParams->hDemod, R367ofdm_CRL_FREQ1, 3);
			ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_FREEZE, 0);
			offset = (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_CRL_FOFFSET_VHI) << 16) ;
			offset += (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_CRL_FOFFSET_HI) << 8);
			offset += (ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_CRL_FOFFSET_LO));
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
			if (ChipGetField(pParams->hDemod, F367ofdm_PPM_INVSEL) == 1) /* inversion hard auto */
			{
				if (((ChipGetField(pParams->hDemod, F367ofdm_INV_SPECTR) != ChipGetField(pParams->hDemod, F367ofdm_STATUS_INV_SPECRUM)) == 1))
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
				if (((!pParams->SpectrumInversion) && (hTunerParams->SpectrInvert == TUNER_IQ_NORMAL))
						|| ((pParams->SpectrumInversion) && (hTunerParams->SpectrInvert == TUNER_IQ_INVERT)))
					offset = offset * -1;
			}
			if (pParams->ChannelBW == 6)
				offset = (offset * 6) / 8;
			else if (pParams->ChannelBW == 7)
				offset = (offset * 7) / 8;
			pParams->Frequency += offset;
			pResult->Frequency = pParams->Frequency;
			pResult->ResidualOffset = offset;
			pResult->Echo_pos = ChipGetField(pParams->hDemod, F367ofdm_LONG_ECHO);
			/*For FEC rate return to application*/
			/* Get the FEC Rate */
			if (pResult->hier == FE_TER_HIER_LOW_PRIO)
			{
				pResult->FECRates = ChipGetField(pParams->hDemod, F367ofdm_TPS_LPCODE);
			}
			else
			{
				pResult->FECRates = ChipGetField(pParams->hDemod, F367ofdm_TPS_HPCODE);
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
				ChipWaitOrAbort(pParams->hDemod, 10); /*was 20ms */
				/* fine tuning of timing offset if required */
				ChipGetRegisters(pParams->hDemod, R367ofdm_TRL_CTL, 5);
				timing_offset = ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_TOFFSET_LO) + 256 * ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_TOFFSET_HI);
				if (timing_offset >= 32768) timing_offset -= 65536;
				trl_nomrate = (512 * ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_HI) + ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LO) * 2 + ChipGetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LSB));
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
				ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LSB, trl_nomrate % 2);
				ChipSetFieldImage_0367ter(DeviceMap, IOHandle, F367ofdm_TRL_NOMRATE_LO, trl_nomrate / 2);
				ChipSetRegisters(pParams->hDemod, R367ofdm_TRL_CTL, 2);
				ChipWaitOrAbort(pParams->hDemod, 1);
			}
			ChipWaitOrAbort(pParams->hDemod, 5);
			/* unlocks could happen in case of trl centring big step, then a core off/on restarts demod */
			u_var = ChipGetField(pParams->hDemod, F367ofdm_LK);
			if (!u_var)
			{
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_CORE_ACTIVE, 0);
				ChipWaitOrAbort(pParams->hDemod, 20);
				ChipSetField_0367ter(DeviceMap, IOHandle, F367ofdm_CORE_ACTIVE, 1);
				ChipWaitOrAbort(pParams->hDemod, 350);
			}
#endif
		} /* if(FE_367_Algo(pParams) == FE_TER_LOCKOK) */
		else
		{
			pResult->Locked = FALSE;
			error = FE_LLA_SEARCH_FAILED;
		}
	} /*if((void *)Handle != NULL)*/
	else
	{
		error = FE_LLA_BAD_PARAMETER;
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
//FE_LLA_Error_t FE_STV0367ofdm_SetStandby(YWTUNER_Handle_T Handle, U8 StandbyOn)
YW_ErrorType_T demod_d0367ter_SetStandby(U8 Handle)
{
	FE_LLA_Error_t error = FE_LLA_NO_ERROR;
	TUNER_ScanTaskParam_T *Inst = NULL;
	TUNER_IOREG_DeviceMap_t *DeviceMap = NULL;
	IOARCH_Handle_t IOHandle;
	U8 StandbyOn = 1;
	Inst = TUNER_GetScanInfo(Handle);
	IOHandle = Inst->DriverParam.Ter.DemodIOHandle;
	DeviceMap = &Inst->DriverParam.Ter.Demod_DeviceMap;
	if (StandbyOn)
	{
		if (Inst->DriverParam.Ter.TunerType == TUNER_TUNER_STV4100)
		{
			if (Inst->DriverParam.Ter.TunerDriver.tuner_SetStandby != NULL)
			{
				error = Inst->DriverParam.Ter.TunerDriver.tuner_SetStandby(Handle, StandbyOn);
			}
		}
		ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY, 1);
		ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_FEC, 1);
		ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_CORE, 1);
	}
	else
	{
		ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY, 0);
		ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_FEC, 0);
		ChipSetField_0367ter(DeviceMap, IOHandle, F367_STDBY_CORE, 0);
		if (Inst->DriverParam.Ter.TunerType == TUNER_TUNER_STV4100)
		{
			if (Inst->DriverParam.Ter.TunerDriver.tuner_SetStandby != NULL)
			{
				error = Inst->DriverParam.Ter.TunerDriver.tuner_SetStandby(Handle, StandbyOn);
			}
		}
	}
	return (error);
}
#endif

#define MAX_TUNER_NUM 4
static TUNER_ScanTaskParam_T TUNER_DbaseInst[MAX_TUNER_NUM];
TUNER_ScanTaskParam_T *TUNER_GetScanInfo(U8 Index)
{
	if (Index >= MAX_TUNER_NUM)
	{
		return NULL;
	}
	return &TUNER_DbaseInst[Index];
}

/*eof----------------------------------------------------------------------------------*/
