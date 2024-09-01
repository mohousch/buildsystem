/******************************************************
mxl_common.h
----------------------------------------------------
Rf IC control functions

<Revision History>
'11/10/06 : OKAMOTO Select AGC external or internal.
'11/10/06 : OKAMOTO Control AGC set point.
'11/10/06 : OKAMOTO Change "commdef.h" to "dcommdef.h" in sample code.
'11/03/16 : OKAMOTO Select IDAC setting in "MxL_Tuner_RFTune".
'11/02/14 : Add new MxL_ERR_MSG "MxL_ERR_UNKNOWN_ID".
'11/02/10 : Move "User-Defined Types" to "commdef.h".
----------------------------------------------------
Copyright(C) 2011 SHARP CORPORATION
******************************************************/

/*******************************************************************************
 *
 * FILE NAME : MxL_Common.h
 *
 * AUTHOR : hchan
 * DATE CREATED : Nov. 9, 2009
 *
 * DESCRIPTION : MxL Common Header file
 *
 *******************************************************************************
 * Copyright (c) 2009, MaxLinear, Inc.
 ******************************************************************************/

#ifndef __MxL_COMMON_H
#define __MxL_COMMON_H

#if 0 /* '11/02/10 : Move "User-Defined Types" to "commdef.h". */
/******************************************************************************
* User-Defined Types (Typedefs)
******************************************************************************/

/****************************************************************************
* Imports and definitions for WIN32
****************************************************************************/
/*#include <windows.h> */
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef char SINT8;
typedef short SINT16;
typedef int SINT32;
typedef float REAL32;

/****************************************************************************\
* Imports and definitions for non WIN32 platforms *
\****************************************************************************/
/*
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef char SINT8;
typedef short SINT16;
typedef int SINT32;
typedef float REAL32;

// create a boolean
#ifndef __boolean__
#define __boolean__
typedef enum {FALSE=0,TRUE} BOOL;
#endif
*/

/****************************************************************************\
* Definitions for all platforms *
\****************************************************************************/
#ifndef NULL
#define NULL (void*)0
#endif

#else
#ifdef SHARP_DEBUG_FOR_CYGWIN
#include "../_include/dcommdef.h"
#else
//#include "commdef.h"
#endif
#endif

/******************************/
/* MxL Err message */
/******************************/
typedef enum
{
	MxL_OK = 0,
	MxL_ERR_INIT = 1,
	MxL_ERR_RFTUNE = 2,
	MxL_ERR_SET_REG = 3,
	MxL_ERR_GET_REG = 4,
	MxL_ERR_OTHERS = 10,
	MxL_ERR_UNKNOWN_ID,
	MxL_GET_ID_FAIL = 0xFF
} MxL_ERR_MSG;

/******************************/
/* MxLF Chip verstion */
/******************************/
typedef enum
{
	MxL_UNKNOWN_ID = 0x00,
	MxL_30xRF_V8 = 0x08,
	MxL_30xRF_V9 = 0x09
} MxLxxxRF_ChipVersion;

/******************************************************************************
 CONSTANTS
******************************************************************************/

#ifndef MHz
#define MHz 1000000
#endif

#define MAX_ARRAY_SIZE 100

/* Enumeration of Tuner Types supported */
typedef enum
{
	MxL_TunerID_MxL202RF = 0,
	MxL_TunerID_MxL301RF = 1,
	MxL_TunerID_MxL302RF = 2
} MxLxxxRF_TunerID;

typedef enum
{
	MxL_I2C_ADDR_96 = 96,
	MxL_I2C_ADDR_97 = 97,
	MxL_I2C_ADDR_98 = 98,
	MxL_I2C_ADDR_99 = 99
} MxLxxxRF_I2CAddr ;

/* Enumeration of Mode */
typedef enum
{
	MxL_MODE_DVBT = 1,
	MxL_MODE_ATSC = 2,
	MxL_MODE_CAB_STD = 0x10,
	MxL_MODE_ANA_MN = 0x20,
	MxL_MODE_ANA_BG = 0x21,
	MxL_MODE_ANA_I = 0x22,
	MxL_MODE_ANA_DKL = 0x23,
	MxL_MODE_ANA_SECAM = 0x24,
	MxL_MODE_ANA_SECAM_ACC = 0x25
} MxLxxxRF_Mode ;

typedef enum
{
	MxL_IF_3_65_MHZ = 3650000,
	MxL_IF_4_MHZ = 4000000,
	MxL_IF_4_1_MHZ = 4100000,
	MxL_IF_4_15_MHZ = 4150000,
	MxL_IF_4_5_MHZ = 4500000,
	MxL_IF_4_57_MHZ = 4570000,
	MxL_IF_5_MHZ = 5000000,
	MxL_IF_5_38_MHZ = 5380000,
	MxL_IF_6_MHZ = 6000000,
	MxL_IF_6_28_MHZ = 6280000,
	MxL_IF_7_2_MHZ = 7200000,
	MxL_IF_8_25_MHZ = 8250000,
	MxL_IF_35_25_MHZ = 35250000,
	MxL_IF_36_MHZ = 36000000,
	MxL_IF_36_15_MHZ = 36150000,
	MxL_IF_36_65_MHZ = 36650000,
	MxL_IF_44_MHZ = 44000000
} MxLxxxRF_IF_Freq ;

typedef enum
{
	MxL_XTAL_16_MHZ = 16000000,
	MxL_XTAL_20_MHZ = 20000000,
	MxL_XTAL_20_25_MHZ = 20250000,
	MxL_XTAL_20_48_MHZ = 20480000,
	MxL_XTAL_24_MHZ = 24000000,
	MxL_XTAL_25_MHZ = 25000000,
	MxL_XTAL_25_14_MHZ = 25140000,
	MxL_XTAL_27_MHZ = 27000000,
	MxL_XTAL_28_8_MHZ = 28800000,
	MxL_XTAL_32_MHZ = 32000000,
	MxL_XTAL_40_MHZ = 40000000,
	MxL_XTAL_44_MHZ = 44000000,
	MxL_XTAL_48_MHZ = 48000000,
	MxL_XTAL_49_3811_MHZ = 49381100
} MxLxxxRF_Xtal_Freq ;

typedef enum
{
	MxL_BW_6MHz = 6,
	MxL_BW_7MHz = 7,
	MxL_BW_8MHz = 8
} MxLxxxRF_BW_MHz;

typedef enum
{
	MxL_NORMAL_IF = 0,
	MxL_INVERT_IF

} MxLxxxRF_IF_Spectrum ;

typedef enum
{
	MxL_CLKOUT_DISABLE = 0,
	MxL_CLKOUT_ENABLE

} MxLxxxRF_ClkOut;

typedef enum
{
	MxL_CLKOUT_AMP_0 = 0,
	MxL_CLKOUT_AMP_1,
	MxL_CLKOUT_AMP_2,
	MxL_CLKOUT_AMP_3,
	MxL_CLKOUT_AMP_4,
	MxL_CLKOUT_AMP_5,
	MxL_CLKOUT_AMP_6,
	MxL_CLKOUT_AMP_7,
	MxL_CLKOUT_AMP_8,
	MxL_CLKOUT_AMP_9,
	MxL_CLKOUT_AMP_10,
	MxL_CLKOUT_AMP_11,
	MxL_CLKOUT_AMP_12,
	MxL_CLKOUT_AMP_13,
	MxL_CLKOUT_AMP_14,
	MxL_CLKOUT_AMP_15
} MxLxxxRF_ClkOut_Amp;

typedef enum
{
	MxL_AGC_SEL1 = 0,
	MxL_AGC_SEL2
} MxLxxxRF_AGC_Sel;

/* Enumeration of Acceptable Crystal Capacitor values */
typedef enum
{
	MxL_XTAL_CAP_0_PF = 0,
	MxL_XTAL_CAP_1_PF = 1,
	MxL_XTAL_CAP_2_PF = 2,
	MxL_XTAL_CAP_3_PF = 3,
	MxL_XTAL_CAP_4_PF = 4,
	MxL_XTAL_CAP_5_PF = 5,
	MxL_XTAL_CAP_6_PF = 6,
	MxL_XTAL_CAP_7_PF = 7,
	MxL_XTAL_CAP_8_PF = 8,
	MxL_XTAL_CAP_9_PF = 9,
	MxL_XTAL_CAP_10_PF = 10,
	MxL_XTAL_CAP_11_PF = 11,
	MxL_XTAL_CAP_12_PF = 12,
	MxL_XTAL_CAP_13_PF = 13,
	MxL_XTAL_CAP_14_PF = 14,
	MxL_XTAL_CAP_15_PF = 15,
	MxL_XTAL_CAP_16_PF = 16,
	MxL_XTAL_CAP_17_PF = 17,
	MxL_XTAL_CAP_18_PF = 18,
	MxL_XTAL_CAP_19_PF = 19,
	MxL_XTAL_CAP_20_PF = 20,
	MxL_XTAL_CAP_21_PF = 21,
	MxL_XTAL_CAP_22_PF = 22,
	MxL_XTAL_CAP_23_PF = 23,
	MxL_XTAL_CAP_24_PF = 24,
	MxL_XTAL_CAP_25_PF = 25
} MxLxxxRF_Xtal_Cap;

typedef enum
{
	MxL_IF_SPLIT_DISABLE = 0,
	MxL_IF_SPLIT_ENABLE
} MxLxxxRF_Analog_IF_Split;

typedef enum
{
	MxL_IF_PATH1 = 0,
	MxL_IF_PATH2
} MxLxxxRF_IF_Path;

/* '11/03/16 : OKAMOTO Select IDAC setting. */
/******************************/
/* MxL IDAC setting */
/******************************/
typedef enum _MxL_IDAC_SETTING
{
	MxL_IDAC_SETTING_AUTO = 0,
	MxL_IDAC_SETTING_MANUAL,
	MxL_IDAC_SETTING_OFF,
	MxL_IDAC_SETTING_MAX,
} MxL_IDAC_SETTING, *PMxL_IDAC_SETTING;

/********************************/
/* MxL Hysterisis in IDAC auto */
/********************************/
typedef enum _MxL_IDAC_HYSTERISIS
{
	MxL_IDAC_HYSTERISIS_0_00DB = 0, // 0dB
	MxL_IDAC_HYSTERISIS_0_25DB, // 0.25dB
	MxL_IDAC_HYSTERISIS_0_50DB, // 0.5dB
	MxL_IDAC_HYSTERISIS_0_75DB, // 0.75dB
	MxL_IDAC_HYSTERISIS_1_00DB, // 1dB
	MxL_IDAC_HYSTERISIS_1_25DB, // 1.25dB
	MxL_IDAC_HYSTERISIS_1_50DB, // 1.5dB
	MxL_IDAC_HYSTERISIS_1_75DB, // 1.75dB
	MxL_IDAC_HYSTERISIS_2_00DB, // 2dB
	MxL_IDAC_HYSTERISIS_2_25DB, // 2.25dB
	MxL_IDAC_HYSTERISIS_2_50DB, // 2.5dB
	MxL_IDAC_HYSTERISIS_2_75DB, // 2.75dB
	MxL_IDAC_HYSTERISIS_3_00DB, // 3dB
	MxL_IDAC_HYSTERISIS_3_25DB, // 3.25dB
	MxL_IDAC_HYSTERISIS_3_50DB, // 3.5dB
	MxL_IDAC_HYSTERISIS_3_75DB, // 3.75dB
	MxL_IDAC_HYSTERISIS_MAX
} MxL_IDAC_HYSTERISIS, *PMxL_IDAC_HYSTERISIS;

/* MxLxxxRF TunerConfig Struct */
typedef struct _MxLxxxRF_TunerConfigS
{
	MxLxxxRF_I2CAddr I2C_Addr;
	MxLxxxRF_Mode Mode;
	MxLxxxRF_Xtal_Freq Xtal_Freq;
	MxLxxxRF_IF_Freq IF_Freq;
	MxLxxxRF_IF_Spectrum IF_Spectrum;
	MxLxxxRF_ClkOut ClkOut_Setting;
	MxLxxxRF_ClkOut_Amp ClkOut_Amp;
	MxLxxxRF_Xtal_Cap Xtal_Cap;
	MxLxxxRF_BW_MHz BW_MHz;
	MxLxxxRF_TunerID TunerID;
	MxLxxxRF_AGC_Sel AGC;
	MxLxxxRF_Analog_IF_Split IF_Split; /* Valid for MxL302RF only */
	MxLxxxRF_IF_Path IF_Path;
	UINT32 RF_Freq_Hz;

	/* '11/03/16 : OKAMOTO Select IDAC setting in "MxL_Tuner_RFTune". */
	MxL_IDAC_SETTING idac_setting;
	MxL_IDAC_HYSTERISIS idac_hysterisis;
	UINT8 dig_idac_code;

	/* '11/10/06 : OKAMOTO Control AGC set point. */
	UINT8 AGC_set_point;

	/* '11/10/06 : OKAMOTO Select AGC external or internal. */
	BOOL bInternalAgcEnable;
} MxLxxxRF_TunerConfigS;

/* typedef *MxLxxxRF_TunerConfigS pMxLxxxRF_TunerConfigS; */

typedef struct
{
	UINT8 Num; /*Register number */
	UINT8 Val; /*Register value */
} IRVType, *PIRVType;

/* typedef for spur shifting table at V9.2.7.0 */
typedef struct
{
	UINT32 Freq; /*Channel center frequency */
	UINT32 Freq_TH; /*Offset frequency threshold */
	UINT8 SHF_Val; /*Spur shift value */
	UINT8 SHF_Dir; /*Spur shift direction */
	UINT8 SHF_type; /*SHF_type 0 : All, 1 : Non-split, 2 : Split */
} SHFType, *PSHFType;

/* extern definition for analg spur shift mode : V9.2.7.0 */
extern UINT8 Shfnum_NTSC;
extern UINT8 Shfnum_BG;
extern UINT8 Shfnum_DK;
extern UINT8 Shfnum_I;
extern UINT8 Shfnum_SECAM;
extern UINT8 Shfnum_CABLE;
extern UINT8 Shfnum_DVBT;
extern UINT8 Shfnum_ATSC;

extern SHFType SHF_NTSC_TAB[];
extern SHFType SHF_BG_TAB[];
extern SHFType SHF_DK_TAB[];
extern SHFType SHF_I_TAB[];
extern SHFType SHF_SECAM_TAB[];
extern SHFType SHF_CABLE_TAB[];
extern SHFType SHF_DVBT_TAB[];
extern SHFType SHF_ATSC_TAB[];

/* Common functions */
UINT32 SetIRVBit(PIRVType pIRV, UINT8 Num, UINT8 Mask, UINT8 Val);
UINT32 SetIRVBit_Ext(PIRVType pIRV, UINT8 Num1, UINT8 Mask, UINT8 Val1, UINT8 Val2);
/* SINT32 round_d(double inVal); */
UINT32 div_rnd_uint32(UINT32 numerator, UINT32 denominator);

#endif /* __MxLxxxRF_COMMON_H__*/
