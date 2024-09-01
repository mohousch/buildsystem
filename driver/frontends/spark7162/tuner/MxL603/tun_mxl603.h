/*******************************************************************************
 *
 * FILE NAME : MxL601_OEM_Drv.h
 *
 * AUTHOR : Dong Liu
 *
 * DATE CREATED : 11/23/2011
 *
 * DESCRIPTION : Header file for MxL601_OEM_Drv.c
 *
 *******************************************************************************
 * Copyright (c) 2010, MaxLinear, Inc.
 ******************************************************************************/

#ifndef __TUN_MXL603_H__
#define __TUN_MXL603_H__
/******************************************************************************
 Include Header Files
 (No absolute paths - paths handled by make file)
******************************************************************************/

#include "MaxLinearDataTypes.h"
//#include "MxL_Debug.h"

/******************************************************************************
 Macros
******************************************************************************/

/******************************************************************************
 User-Defined Types (Typedefs)
******************************************************************************/
//typedef struct
//{
//	MxL5007_I2CAddr I2C_Addr;
//	MxL5007_Mode Mode;
//	SINT32 IF_Diff_Out_Level;
//	MxL5007_Xtal_Freq Xtal_Freq;
//	MxL5007_IF_Freq IF_Freq;
//	MxL5007_IF_Spectrum IF_Spectrum;
//	MxL5007_ClkOut ClkOut_Setting;
// MxL5007_ClkOut_Amp ClkOut_Amp;
//	MxL5007_BW_MHz BW_MHz;
//	MxL5007_LoopThru LoopThru;
//	UINT32 RF_Freq_Hz;
//	UINT32 tuner_id;
//} MXL603_PARAMETER;

/******************************************************************************
 Global Variable Declarations
******************************************************************************/
extern void *MxL603_OEM_DataPtr[];
/*****************************************************************************
 Prototypes
******************************************************************************/

MXL_STATUS MxLWare603_OEM_WriteRegister(UINT32 tuner_id, UINT8 regAddr, UINT8 regData);
MXL_STATUS MxLWare603_OEM_ReadRegister(UINT32 tuner_id, UINT8 regAddr, UINT8 *regDataPtr);
void MxLWare603_OEM_Sleep(UINT16 delayTimeInMs);

#endif /* __TUN_MXL603_H__*/

