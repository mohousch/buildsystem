/******************************************************
MxL301RF.h
----------------------------------------------------
Rf IC control functions

<Revision History>
'11/10/06 : OKAMOTO Select AGC external or internal.
----------------------------------------------------
Copyright(C) 2011 SHARP CORPORATION
******************************************************/

#include "mxl_common.h"

/* Initializes the registers of the chip */
UINT32 MxL301RF_Init(UINT8 *pArray, /* a array pointer that store the addr and data pairs for I2C write */
					 UINT32 *Array_Size, /* a integer pointer that store the number of element in above array */
					 UINT8 Mode, /* Standard */
					 UINT32 Xtal_Freq_Hz, /* Crystal Frequency in Hz */
					 UINT32 IF_Freq_Hz, /* IF Frequency in Hz */
					 UINT8 Invert_IF, /* Inverted IF Spectrum: 1, or Normal IF: 0 */
					 UINT8 Clk_Out_Enable, /* Enable Crystal Clock out */
					 UINT8 Clk_Out_Amp, /* Clock out amplitude: 0 min, 15 max */
					 UINT8 Xtal_Cap, /* Internal Crystal Capacitance */
					 UINT8 AGC_Sel, /* AGC Selection */
					 UINT8 IF_Out /* IF1 or IF2 output path */

					 /* '11/10/06 : OKAMOTO Select AGC external or internal. */
					 , BOOL bInternalAgcEnable
					);
/* Sets registers of the tuner based on RF freq, BW, etc. */
UINT32 MxL301RF_RFTune(UINT8 *pArray, UINT32 *Array_Size,
					   UINT32 RF_Freq, /* RF Frequency in Hz */
					   UINT8 BWMHz, /* Bandwidth in MHz */
					   UINT8 Mode
					  );

