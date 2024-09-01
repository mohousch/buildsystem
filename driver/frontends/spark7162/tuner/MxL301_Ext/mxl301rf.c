/******************************************************
MxL301RF.c
----------------------------------------------------
Rf IC control functions

<Revision History>
'11/10/06 : OKAMOTO Select AGC external or internal.
----------------------------------------------------
Copyright(C) 2011 SHARP CORPORATION
******************************************************/

/*
 Description: The source code is for MxL301RF user to quickly integrate MxL301RF into their software.
 There are two functions the user can call to generate a array of I2C command that's require to
 program the MxL301RF tuner. The user should pass an array pointer and an integer pointer in to the
 function. The funciton will fill up the array with format like follow:

 addr1
 data1
 addr2
 data2
 ...

 The user can then pass this array to their I2C function to perform programming the tuner.
*/
/* #include "StdAfx.h" */
#include <types.h>
#include "mxl301rf.h"

UINT32 MxL301RF_Init(UINT8 *pArray, /* a array pointer that store the addr and data pairs for I2C write */
					 UINT32 *Array_Size, /* a integer pointer that store the number of element in above array */
					 UINT8 Mode,
					 UINT32 Xtal_Freq_Hz,
					 UINT32 IF_Freq_Hz,
					 UINT8 Invert_IF,
					 UINT8 Clk_Out_Enable,
					 UINT8 Clk_Out_Amp,
					 UINT8 Xtal_Cap,
					 UINT8 AGC_sel,
					 UINT8 IF_Out

					 /* '11/10/06 : OKAMOTO Select AGC external or internal. */
					 , BOOL bInternalAgcEnable
					)
{
	UINT32 Reg_Index = 0;
	UINT32 Array_Index = 0;
	/* Terrestial register settings */
	IRVType IRV_Init[] =
	{
		/* { Addr, Data} */
		{ 0x00, 0x02},
		{ 0x02, 0xA6},
		{ 0x05, 0x4A},
		{ 0x06, 0x14},
		{ 0x07, 0x0A},
		{ 0x0E, 0x00},
		{ 0x0F, 0x00},
		{ 0x21, 0xA8},

		{ 0xC8, 0x00}, /*Common Override */
		{ 0xAF, 0xA2}, /*Common Override */
		{ 0x1D, 0x4A}, /*Common Override */
		{ 0xCA, 0x05}, /*Common Override */
		{ 0x2E, 0x7E}, /*Common Override - changed V9.2.1.0*/
		{ 0x2D, 0x44}, /*Common Override - changed V9.2.1.0*/
		{ 0x1B, 0xAC}, /*Common Override */
		{ 0xAC, 0x00}, /*Common Override */
		{ 0x2C, 0x01}, /*Common Override - changed V9.2.1.0*/
		{ 0xAE, 0x03}, /*Common Override - Added V9.2.1.0 */
		{ 0x54, 0xE3}, /*Common Override - Added V9.2.6.0 */
		{ 0x56, 0x47}, /*Common Override - Added V9.2.6.0 */
		{ 0x55, 0x12}, /*Common Override - Added V9.2.7.0 */

		{ 0x09, 0x01}, /*Common Terrestial Override */
		{ 0xA4, 0x51}, /*Common Terrestial Override */
		{ 0xA0, 0x0C}, /*Common Terrestial Override */
		{ 0xB0, 0x00}, /*Common Terrestial Override */
		{ 0x57, 0x17}, /*Common Terrestial Override */
		{ 0x6F, 0x79}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x70, 0x0A}, /*Common Terrestial Override : changed V9.4.3*/
		{ 0x6F, 0x78}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x70, 0x0A}, /*Common Terrestial Override : changed V9.4.3*/
		{ 0x6F, 0x7B}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x70, 0x00}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x6F, 0x7C}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x70, 0x00}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x6F, 0x7A}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x70, 0x00}, /*Common Terrestial Override : added V9.2.3*/
		{ 0x1A, 0x0D}, /*Common Terrestial Override - Added V9.2.6.0*/

		{ 0x01, 0x01}, /*TOP_MASTER_ENABLE=1 */
		{0xFF, 0xFF}
	};
	/* Cable register settings */
	IRVType IRV_Init_Cable[] =
	{
		/*{ Addr, Data} */
		{ 0x00, 0x02},
		{ 0x02, 0xA6},
		{ 0x05, 0x4A},
		{ 0x06, 0x14},
		{ 0x07, 0x0A},
		{ 0x0E, 0x00},
		{ 0x0F, 0x00},
		{ 0x21, 0xA8},

		{ 0xC8, 0x00}, /*Common Override */
		{ 0xAF, 0xA2}, /*Common Override */
		{ 0x1D, 0x4A}, /*Common Override */
		{ 0xCA, 0x05}, /*Common Override */
		{ 0x2E, 0x7E}, /*Common Override - changed V9.2.1.0*/
		{ 0x2D, 0x44}, /*Common Override - changed V9.2.1.0*/
		{ 0x1B, 0xAC}, /*Common Override */
		{ 0xAC, 0x00}, /*Common Override */
		{ 0x2C, 0x01}, /*Common Override - changed V9.2.1.0*/
		{ 0xAE, 0x03}, /*Common Override - Added V9.2.1.0 */
		{ 0x54, 0xE1}, /*Common Override - Added V9.2.6.0 */
		{ 0x56, 0x47}, /*Common Override - Added V9.2.6.0 */
		{ 0x55, 0x12}, /*Common Override - Added V9.2.7.0 */

		{ 0x09, 0x04}, /*Common Cable Override */
		{ 0xA0, 0x8C}, /*Common Cable Override */
		{ 0xB0, 0xC0}, /*Common Cable Override */
		{ 0x4E, 0x37}, /*Common Cable Override */
		{ 0x57, 0x67}, /*Common Cable Override */
		{ 0x3E, 0x6B}, /*Common Cable Override - Added V9.2.1.0 */
		{ 0x6F, 0x79}, /*Common Cable Override : added V9.2.3*/
		{ 0x70, 0x0A}, /*Common Cable Override : changed V9.4.3*/
		{ 0x6F, 0x78}, /*Common Cable Override : added V9.2.3*/
		{ 0x70, 0x0A}, /*Common Cable Override : changed V9.4.3*/
		{ 0x6F, 0x7B}, /*Common Cable Override : added V9.2.3*/
		{ 0x70, 0x00}, /*Common Cable Override : added V9.2.3*/
		{ 0x6F, 0x7C}, /*Common Cable Override : added V9.2.3*/
		{ 0x70, 0x00}, /*Common Cable Override : added V9.2.3*/
		{ 0x6F, 0x7A}, /*Common Cable Override : added V9.2.3*/
		{ 0x70, 0x00}, /*Common Cable Override : added V9.2.3*/
		{ 0x6F, 0x8F}, /*Common Cable Override - Added V9.4.3.0 */
		{ 0x70, 0x02}, /*Common Cable Override - Added V9.4.3.0 */

		{ 0x01, 0x01}, /*TOP_MASTER_ENABLE=1 */
		{0xFF, 0xFF}
	};
	/* Analog register settings */
	IRVType IRV_Init_Analog[] =
	{
		/*{ Addr, Data} */
		{ 0x00, 0x02},
		{ 0x02, 0xA6},
		{ 0x05, 0x4A},
		{ 0x06, 0x14},
		{ 0x07, 0x0A},
		{ 0x0E, 0x00},
		{ 0x0F, 0x00},
		{ 0x21, 0xA8},

		{ 0xC8, 0x00}, /*Common Override */
		{ 0xAF, 0xA2}, /*Common Override */
		{ 0x1D, 0x4A}, /*Common Override */
		{ 0xCA, 0x05}, /*Common Override */
		{ 0x2E, 0x7E}, /*Common Override - changed V9.2.1.0*/
		{ 0x2D, 0x44}, /*Common/Analog Override - changed V9.2.1.0*/
		{ 0x1B, 0xAC}, /*Common Override */
		{ 0xAC, 0x00}, /*Common Override */
		{ 0x2C, 0x01}, /*Common Override - changed V9.2.1.0*/
		{ 0xAE, 0x03}, /*Common Override - Added V9.2.1.0 */
		{ 0x54, 0xE3}, /*Common Override - Added V9.2.6.0 */
		{ 0x56, 0x47}, /*Common Override - Added V9.2.6.0 */
		{ 0x55, 0x12}, /*Common Override - Added V9.2.7.0 */

		{ 0x29, 0x23}, /* Common Analog Override */
		{ 0x09, 0x15}, /* Common Analog Override */
		{ 0x1A, 0x0D}, /* Common Analog Override */
		{ 0x4E, 0x37}, /* Common Analog Override */
		{ 0x53, 0xD9}, /* Common Analog Override */
		{ 0xA0, 0x88}, /* Common Analog Override */
		{ 0xA3, 0x3A}, /* Common Analog Override */
		{ 0x57, 0x61}, /* Common Analog Override */
		{ 0x9D, 0x52}, /* Common Analog Override - Added V9.2.8.0 */
		{ 0x5A, 0x64}, /* Common Analog Override - Changed V9.2.1.0 */
		{ 0x7B, 0x00}, /* Common Analog Override - Changed V9.2.1.0 */
		{ 0x7E, 0x2A}, /* Common Analog Override - Changed V9.2.1.0 */
		{ 0x90, 0x10}, /* Common Analog Override - Added V9.4.2.0 */
		{ 0x6F, 0x79}, /* Common Analog Override - Added V9.2.1.0 */
		{ 0x70, 0x38}, /* Common Analog Override - Added V9.2.1.0 */
		{ 0x6F, 0x78}, /* Common Analog Override - Added V9.2.1.0 */
		{ 0x70, 0x38}, /* Common Analog Override - Added V9.2.1.0 */
		{ 0x6F, 0x7B}, /* Common Analog Override - chagned V9.2.2.0 */
		{ 0x70, 0x3C}, /* Common Analog Override - changed V9.2.2.0 */
		{ 0x6F, 0x7C}, /* Common Analog Override - Added V9.2.1.0 */
		{ 0x70, 0x40}, /* Common Analog Override - Added V9.2.1.0 */
		{ 0x6F, 0x7A}, /* Common Analog Override - Added V9.2.2.0 */
		{ 0x70, 0x24}, /* Common Analog Override - Added V9.2.2.0 */

		{ 0x01, 0x01}, /*TOP_MASTER_ENABLE=1 */
		{0xFF, 0xFF}
	};
	/*edit Init setting here */
	PIRVType myIRV = NULL;
	switch (Mode)
	{
		case MxL_MODE_DVBT: /*DVBT Mode */
			myIRV = IRV_Init;
			SetIRVBit(myIRV, 0x09, 0xFF, 0x01);
			SetIRVBit(myIRV, 0xB0, 0xFF, 0xC2);
			SetIRVBit(myIRV, 0x1A, 0xFF, 0x0D);
			break;
		case MxL_MODE_ATSC: /*ATSC Mode */
			myIRV = IRV_Init;
			SetIRVBit(myIRV, 0x09, 0xFF, 0x02);
			SetIRVBit(myIRV, 0xB0, 0xC0, 0xC0);
			SetIRVBit(myIRV, 0x1A, 0xFF, 0x05);
			break;
		case MxL_MODE_CAB_STD:
			myIRV = IRV_Init_Cable;
			break;
		case MxL_MODE_ANA_MN:
			myIRV = IRV_Init_Analog;
			SetIRVBit(myIRV, 0x29, 0xC0, 0xC0);
			SetIRVBit(myIRV, 0x57, 0xFF, 0x60); /* V9.2.8.0 */
			SetIRVBit(myIRV, 0x55, 0xFF, 0x11); /* V9.2.8.0 */
			SetIRVBit(myIRV, 0x56, 0xFF, 0x37); /* V9.2.8.0 */
			break;
		case MxL_MODE_ANA_BG:
			myIRV = IRV_Init_Analog;
			SetIRVBit(myIRV, 0x29, 0xC0, 0xC0);
			SetIRVBit(myIRV, 0x57, 0xFF, 0x60); /* V9.2.7.0 */
			break;
		case MxL_MODE_ANA_I:
			myIRV = IRV_Init_Analog;
			SetIRVBit(myIRV, 0x29, 0xC0, 0xC0);
			SetIRVBit(myIRV, 0x55, 0xFF, 0x11); /* V9.2.8.0 */
			SetIRVBit(myIRV, 0x56, 0xFF, 0x37); /* V9.2.8.0 */
			break;
		case MxL_MODE_ANA_DKL:
			myIRV = IRV_Init_Analog;
			SetIRVBit(myIRV, 0x29, 0xC0, 0xC0);
			SetIRVBit(myIRV, 0x2D, 0x08, 0x08);
			SetIRVBit(myIRV, 0x55, 0xFF, 0x11); /* V9.2.8.0 */
			SetIRVBit(myIRV, 0x56, 0xFF, 0x37); /* V9.2.8.0 */
			break;
		case MxL_MODE_ANA_SECAM:
			myIRV = IRV_Init_Analog;
			SetIRVBit(myIRV, 0x29, 0xC0, 0xC0);
			break;
		case MxL_MODE_ANA_SECAM_ACC:
			myIRV = IRV_Init_Analog;
			SetIRVBit(myIRV, 0x29, 0xC0, 0x00);
			SetIRVBit(myIRV, 0x90, 0xFF, 0x18); /* V9.4.2.0 */
			break;
		default:
			return MxL_ERR_INIT;
	} /* switch(Mode) */
	switch (IF_Freq_Hz)
	{
		case MxL_IF_3_65_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x00);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			SetIRVBit(myIRV, 0x0E, 0xFF, 0x8A);
			SetIRVBit(myIRV, 0x0F, 0xFF, 0x08);
			break;
		case MxL_IF_4_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x01);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_4_1_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x00);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			SetIRVBit(myIRV, 0x0E, 0xFF, 0x9B);
			SetIRVBit(myIRV, 0x0F, 0xFF, 0x08);
			break;
		case MxL_IF_4_15_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x00);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			SetIRVBit(myIRV, 0x0E, 0xFF, 0x9D);
			SetIRVBit(myIRV, 0x0F, 0xFF, 0x08);
			break;
		case MxL_IF_4_5_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x02);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_4_57_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x03);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_5_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x04);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_5_38_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x05);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_6_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x06);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_6_28_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x07);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_7_2_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x08);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			break;
		case MxL_IF_8_25_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x00);
			SetIRVBit(myIRV, 0xAF, 0xFF, 0xA2);
			SetIRVBit(myIRV, 0x0E, 0xFF, 0x39);
			SetIRVBit(myIRV, 0x0F, 0xFF, 0x09);
			break;
		case MxL_IF_35_25_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x09);
			break;
		case MxL_IF_36_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x0A);
			break;
		case MxL_IF_36_15_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x0B);
			break;
		case MxL_IF_36_65_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x0C);
			break;
		case MxL_IF_44_MHZ:
			SetIRVBit(myIRV, 0x02, 0x0F, 0x0D);
			break;
		default:
			return MxL_ERR_INIT;
	} /* switch(IF_Freq_Hz) */
	if (IF_Freq_Hz <= 11 * MHz)
	{
		SetIRVBit(myIRV, 0x02, 0xC0, 0x00);
		SetIRVBit(myIRV, 0xAC, 0xFF, 0x00);
		if (IF_Out == MxL_IF_PATH1)
			SetIRVBit(myIRV, 0x1B, 0x18, 0x08);
		else if (IF_Out == MxL_IF_PATH2)
			SetIRVBit(myIRV, 0x1B, 0x04, 0x04);
	}
	else
	{
		if (IF_Out == MxL_IF_PATH1)
		{
			SetIRVBit(myIRV, 0x02, 0xC0, 0xC0);
			SetIRVBit(myIRV, 0x1B, 0x18, 0x08);
			SetIRVBit(myIRV, 0xAC, 0xFF, 0x30);
		}
		else if (IF_Out == MxL_IF_PATH2)
		{
			SetIRVBit(myIRV, 0x02, 0xC0, 0x80);
			SetIRVBit(myIRV, 0x1B, 0x04, 0x00);
			SetIRVBit(myIRV, 0xAC, 0xFF, 0x00);
		}
	}
	/* IF Inversion */
	if (Mode == MxL_MODE_DVBT || Mode == MxL_MODE_ATSC
			|| Mode == MxL_MODE_CAB_STD)
	{
		if (Invert_IF)
			SetIRVBit(myIRV, 0x02, 0x10, 0x10); /*Invert IF*/
		else
			SetIRVBit(myIRV, 0x02, 0x10, 0x00); /*Normal IF*/
	}
	else if (Mode == MxL_MODE_ANA_MN || Mode == MxL_MODE_ANA_BG || Mode == MxL_MODE_ANA_I
			 || Mode == MxL_MODE_ANA_DKL || Mode == MxL_MODE_ANA_SECAM)
	{
		if (Invert_IF)
			SetIRVBit(myIRV, 0x02, 0x30, 0x20); /*Invert IF*/
		else
			SetIRVBit(myIRV, 0x02, 0x30, 0x10); /*Normal IF*/
	}
	else /* SECAM L' */
	{
		if (Invert_IF)
			SetIRVBit(myIRV, 0x02, 0x30, 0x30); /*Invert IF*/
		else
			SetIRVBit(myIRV, 0x02, 0x30, 0x00); /*Normal IF*/
	}
	switch (Xtal_Freq_Hz)
	{
		case MxL_XTAL_16_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x00);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_20_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x01);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_20_25_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x02);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_20_48_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x03);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_24_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x04);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_25_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x05);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_25_14_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x06);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_27_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x07);
			SetIRVBit(myIRV, 0xC8, 0xFF, 0x00);
			break;
		case MxL_XTAL_28_8_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x08);
			break;
		case MxL_XTAL_32_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x09);
			break;
		case MxL_XTAL_40_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x0A);
			break;
		case MxL_XTAL_44_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x0B);
			break;
		case MxL_XTAL_48_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x0C);
			break;
		case MxL_XTAL_49_3811_MHZ:
			SetIRVBit(myIRV, 0x06, 0x0F, 0x0D);
			break;
		default:
			return MxL_ERR_INIT;
	}
	if (!Clk_Out_Enable) /*default is enable */
		SetIRVBit(myIRV, 0x05, 0x40, 0x00);
	/* Clk_Out_Amp */
	SetIRVBit(myIRV, 0x05, 0x0F, Clk_Out_Amp);
	/* Xtal Capacitor */
	if (Xtal_Cap > 0 && Xtal_Cap <= MxL_XTAL_CAP_25_PF)
		SetIRVBit(myIRV, 0x07, 0xFF, Xtal_Cap);
	else if (Xtal_Cap == 0)
		SetIRVBit(myIRV, 0x07, 0xFF, 0x3F);
	else
		return MxL_ERR_INIT;
	if (IF_Out == MxL_IF_PATH1)
		SetIRVBit(myIRV, 0x00, 0x13, 0x02);
	else if (IF_Out == MxL_IF_PATH2)
		SetIRVBit(myIRV, 0x00, 0x13, 0x11);
	/* AGC Select */
	if (AGC_sel == MxL_AGC_SEL1)
		SetIRVBit(myIRV, 0x21, 0x01, 0x00);
	else if (AGC_sel == MxL_AGC_SEL2)
		SetIRVBit(myIRV, 0x21, 0x01, 0x01);
	else
		return MxL_ERR_INIT;
	/* '11/10/06 : OKAMOTO Select AGC external or internal. */
	if (bInternalAgcEnable)
		SetIRVBit(myIRV, 0x2E, 0xFF, 0x5E);
	/* Generate one Array that Contain Data, Address */
	while (myIRV[Reg_Index].Num != 0xFF || myIRV[Reg_Index].Val != 0xFF)
	{
		pArray[Array_Index++] = myIRV[Reg_Index].Num;
		pArray[Array_Index++] = myIRV[Reg_Index].Val;
		Reg_Index++;
	}
	*Array_Size = Array_Index;
	return MxL_OK;
}

UINT32 MxL301RF_RFTune(UINT8 *pArray,
					   UINT32 *Array_Size,
					   UINT32 RF_Freq,
					   UINT8 BWMHz,
					   UINT8 Mode
					  )

{
	IRVType IRV_RFTune_Digital[] =
	{
		/*{ Addr, Data} */
		{ 0x13, 0x00}, /*abort tune */
		{ 0x3B, 0xC0}, /* Added V9.1.6.0 */
		{ 0x3B, 0x80}, /* Added V9.1.6.0 */
		{ 0x10, 0x15}, /* BW */
		{ 0x1A, 0x05}, /* Added V9.2.1.0 */
		{ 0x61, 0x00}, /* Added V9.2.8.0 */
		{ 0x62, 0xA0}, /* Added V9.2.8.0 */
		{ 0x11, 0x40}, /* 2 bytes to store RF frequency */
		{ 0x12, 0x0E}, /* 2 bytes to store RF frequency */
		{ 0x13, 0x01}, /* start tune */
		{ 0xFF, 0xFF}
	};
	IRVType IRV_RFTune_Analog[] =
	{
		/*{ Addr, Data} */
		{ 0x13, 0x00}, /*abort tune */
		{ 0x3B, 0xC0}, /* Added V9.1.6.0 */
		{ 0x3B, 0x80}, /* Added V9.1.6.0 */
		{ 0x0A, 0x0A}, /* Analog only Override -changed V9.1.3.0 from 4.2.3.2.5*/
		{ 0x10, 0x15}, /* BW */
		{ 0xA8, 0x46},
		{ 0xA2, 0xD3},
		{ 0xB0, 0x00},
		{ 0xA6, 0x04},
		{ 0x38, 0x01},
		{ 0x39, 0x0D}, /* Added V9.2.1.0 */
		{ 0x4B, 0x06}, /* Added V9.2.7.0 - From Init to RFTune */
		{ 0x7D, 0xAB}, /* Added V9.2.7.0 - From Init to RFTune */
		{ 0x7F, 0x78}, /* Added V9.2.7.0 - From Init to RFTune */
		{ 0x61, 0x00}, /* Added V9.1.7.0 */
		{ 0x62, 0xA0}, /* Added V9.1.7.0 */
		{ 0x85, 0x00}, /* Added V9.2.6.0 */
		{ 0x86, 0x00}, /* Added V9.2.6.0 */
		{ 0x1A, 0x05}, /* Added V9.2.1.0 */
		{ 0x11, 0x40}, /* 2 bytes to store RF frequency */
		{ 0x12, 0x0E}, /* 2 bytes to store RF frequency */
		{ 0x13, 0x01}, /* start tune */
		{ 0xFF, 0xFF}
	};
	UINT32 dig_rf_freq = 0;
	UINT32 temp = 0 ;
	UINT32 Reg_Index = 0;
	UINT32 Array_Index = 0;
	UINT32 i = 0;
	UINT32 frac_divider = 1000000;
	UINT32 kHz = 1000;
	UINT8 isBreak = 0; /* Add at V9.2.7.0 */
	PIRVType IRV_RFTune = NULL;
	switch (Mode)
	{
		case MxL_MODE_DVBT: /* DVB-T */
		case MxL_MODE_ATSC:
		case MxL_MODE_CAB_STD:
			IRV_RFTune = IRV_RFTune_Digital;
			break;
		case MxL_MODE_ANA_MN:
		case MxL_MODE_ANA_BG:
		case MxL_MODE_ANA_I:
		case MxL_MODE_ANA_DKL:
		case MxL_MODE_ANA_SECAM:
		case MxL_MODE_ANA_SECAM_ACC:
			IRV_RFTune = IRV_RFTune_Analog;
			break;
		default:
			return MxL_ERR_RFTUNE;
	}
	/* Set Mode setting for analog only*/
	switch (Mode)
	{
		case MxL_MODE_ANA_MN:
			SetIRVBit(IRV_RFTune, 0x0A, 0x71, 0x01);
			SetIRVBit(IRV_RFTune, 0x85, 0xFF, 0xCA);
			SetIRVBit(IRV_RFTune, 0x86, 0x0F, 0x08);
			break;
		case MxL_MODE_ANA_BG:
			if (BWMHz == MxL_BW_7MHz || BWMHz == MxL_BW_6MHz)
			{
				SetIRVBit(IRV_RFTune, 0x0A, 0x71, 0x11);
				SetIRVBit(IRV_RFTune, 0x85, 0xFF, 0x32); /* Added V9.2.6.0 */
				SetIRVBit(IRV_RFTune, 0x86, 0x0F, 0x08); /* Added V9.2.6.0 */
			}
			else if (BWMHz == MxL_BW_8MHz)
			{
				SetIRVBit(IRV_RFTune, 0x0A, 0x71, 0x21);
				SetIRVBit(IRV_RFTune, 0x85, 0xFF, 0x6B); /* Added V9.2.7.0 */
				SetIRVBit(IRV_RFTune, 0x86, 0x0F, 0x0E); /* Added V9.2.7.0 */
			}
			else
				return MxL_ERR_RFTUNE;
			break;
		case MxL_MODE_ANA_I:
			SetIRVBit(IRV_RFTune, 0x0A, 0x71, 0x31);
			break;
		case MxL_MODE_ANA_DKL:
		case MxL_MODE_ANA_SECAM:
		case MxL_MODE_ANA_SECAM_ACC: /* SECAM L/L' */
			SetIRVBit(IRV_RFTune, 0x0A, 0x71, 0x41);
			break;
	}
	/* Set BW setting */
	switch (Mode)
	{
		case MxL_MODE_DVBT: /* DVB-T */
			switch (BWMHz)
			{
				case MxL_BW_6MHz:
					SetIRVBit(IRV_RFTune, 0x10, 0xFF, 0x95);
					break;
				case MxL_BW_7MHz:
					SetIRVBit(IRV_RFTune, 0x10, 0xFF, 0xAA);
					break;
				case MxL_BW_8MHz:
					SetIRVBit(IRV_RFTune, 0x10, 0xFF, 0xBF);
					break;
				default:
					return MxL_ERR_RFTUNE;
			}
			break;
		case MxL_MODE_ATSC: /*ATSC */
			SetIRVBit(IRV_RFTune, 0x10, 0xFF, 0x99);
			break;
		case MxL_MODE_CAB_STD:
		case MxL_MODE_ANA_MN: /*Analog (same as cable) */
		case MxL_MODE_ANA_BG:
		case MxL_MODE_ANA_I:
		case MxL_MODE_ANA_DKL:
		case MxL_MODE_ANA_SECAM:
		case MxL_MODE_ANA_SECAM_ACC:
			switch (BWMHz)
			{
				case MxL_BW_6MHz:
					SetIRVBit(IRV_RFTune, 0x10, 0xFF, 0x49);
					break;
				case MxL_BW_7MHz:
					SetIRVBit(IRV_RFTune, 0x10, 0xFF, 0x5A);
					break;
				case MxL_BW_8MHz:
					SetIRVBit(IRV_RFTune, 0x10, 0xFF, 0x6F);
					break;
				default:
					return MxL_ERR_RFTUNE;
			}
			break;
		default:
			return MxL_ERR_RFTUNE;
	}
	/*Convert RF frequency into 16 bits => 10 bit integer (MHz) + 6 bit fraction */
	dig_rf_freq = RF_Freq / MHz; /*Whole number portion of RF freq (in MHz) */
	temp = RF_Freq % MHz; /*Decimal portion of RF freq (in MHz) */
	for (i = 0; i < 6; i++)
	{
		dig_rf_freq <<= 1;
		frac_divider /= 2;
		if (temp > frac_divider) /* Carryover from decimal */
		{
			temp -= frac_divider;
			dig_rf_freq++;
		}
	}
	/*add to have shift center point by 7.8124 kHz */
	if (temp > 7812)
		dig_rf_freq ++;
	SetIRVBit(IRV_RFTune, 0x11, 0xFF, (UINT8)dig_rf_freq);
	SetIRVBit(IRV_RFTune, 0x12, 0xFF, (UINT8)(dig_rf_freq >> 8));
	/* Frequency Dependent Settings for Analog Only */
	if (Mode >= MxL_MODE_ANA_MN)
	{
		if (RF_Freq < 333 * MHz)
		{
			/* Added V9.1.7.0 */
			if (RF_Freq < 131 * MHz || (RF_Freq >= 231 * MHz && RF_Freq < 333 * MHz))
				SetIRVBit(IRV_RFTune, 0x38, 0xFF, 0x03);
			else
				SetIRVBit(IRV_RFTune, 0x38, 0xFF, 0x01);
			SetIRVBit(IRV_RFTune, 0x39, 0xFF, 0x0D); /* Added V9.2.1.0 */
			SetIRVBit(IRV_RFTune, 0xA2, 0xFF, 0xC3);
			SetIRVBit(IRV_RFTune, 0xA6, 0xFF, 0x04);
			SetIRVBit(IRV_RFTune, 0xA8, 0xFF, 0x46);
			SetIRVBit(IRV_RFTune, 0xB0, 0xC0, 0x80);
			SetIRVBit(IRV_RFTune, 0x4B, 0xFF, 0x06); /* Added V9.2.7.0 */
			SetIRVBit(IRV_RFTune, 0x7D, 0xF0, 0xA0); /* Added V9.2.7.0 */
			SetIRVBit(IRV_RFTune, 0x7F, 0xF0, 0x70); /* Added V9.2.7.0 */
			if (RF_Freq >= 231 * MHz)
			{
				SetIRVBit(IRV_RFTune, 0x39, 0xFF, 0x17);/* Added V9.2.7.0 */
				SetIRVBit(IRV_RFTune, 0x4B, 0xFF, 0x01);/* Added V9.2.7.0 */
				SetIRVBit(IRV_RFTune, 0x7D, 0xF0, 0x60);/* Added V9.2.7.0 */
				SetIRVBit(IRV_RFTune, 0x7F, 0xF0, 0x30);/* Added V9.2.7.0 */
			}
		}
		else
		{
			SetIRVBit(IRV_RFTune, 0x38, 0xFF, 0x04);
			SetIRVBit(IRV_RFTune, 0x39, 0xFF, 0x17); /* Added V9.2.1.0 */
			SetIRVBit(IRV_RFTune, 0xA2, 0xFF, 0xD3);
			SetIRVBit(IRV_RFTune, 0xA6, 0xFF, 0x14);
			SetIRVBit(IRV_RFTune, 0xA8, 0xFF, 0x40);
			SetIRVBit(IRV_RFTune, 0xB0, 0xC0, 0x00);
			SetIRVBit(IRV_RFTune, 0x4B, 0xFF, 0x06); /* Added V9.2.7.0 */
			SetIRVBit(IRV_RFTune, 0x7D, 0xF0, 0xA0); /* Added V9.2.7.0 */
			SetIRVBit(IRV_RFTune, 0x7F, 0xF0, 0x70); /* Added V9.2.7.0 */
		}
	}
	/* Spur shift function for analog mode V9.2.7.0 */
	switch (Mode)
	{
		case MxL_MODE_DVBT:
			for (i = 0; i < Shfnum_DVBT; i++)
			{
				if ((RF_Freq >= (SHF_DVBT_TAB[i].Freq - SHF_DVBT_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_DVBT_TAB[i].Freq + SHF_DVBT_TAB[i].Freq_TH)*kHz))
				{
					if (SHF_DVBT_TAB[i].SHF_type == 2)
					{
						isBreak = 1;
						break;
					}
					SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_DVBT_TAB[i].SHF_Val);
					SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_DVBT_TAB[i].SHF_Dir);
					isBreak = 1;
				}
				if (isBreak) break;
			}
			break;
		case MxL_MODE_ATSC:
			for (i = 0; i < Shfnum_ATSC; i++)
			{
				if ((RF_Freq >= (SHF_ATSC_TAB[i].Freq - SHF_ATSC_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_ATSC_TAB[i].Freq + SHF_ATSC_TAB[i].Freq_TH)*kHz))
				{
					if (SHF_ATSC_TAB[i].SHF_type == 2)
					{
						isBreak = 1;
						break;
					}
					SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_ATSC_TAB[i].SHF_Val);
					SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_ATSC_TAB[i].SHF_Dir);
					isBreak = 1;
				}
				if (isBreak) break;
			}
			break;
		case MxL_MODE_CAB_STD:
			for (i = 0; i < Shfnum_CABLE; i++)
			{
				if ((RF_Freq >= (SHF_CABLE_TAB[i].Freq - SHF_CABLE_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_CABLE_TAB[i].Freq + SHF_CABLE_TAB[i].Freq_TH)*kHz))
				{
					if (SHF_CABLE_TAB[i].SHF_type == 2)
					{
						isBreak = 1;
						break;
					}
					SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_CABLE_TAB[i].SHF_Val);
					SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_CABLE_TAB[i].SHF_Dir);
					isBreak = 1;
				}
				if (isBreak) break;
			}
			break;
		case MxL_MODE_ANA_MN:
			for (i = 0; i < Shfnum_NTSC; i++)
			{
				if ((RF_Freq >= (SHF_NTSC_TAB[i].Freq - SHF_NTSC_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_NTSC_TAB[i].Freq + SHF_NTSC_TAB[i].Freq_TH)*kHz))
				{
					if (SHF_NTSC_TAB[i].SHF_type == 2)
					{
						isBreak = 1;
						break;
					}
					SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_NTSC_TAB[i].SHF_Val);
					SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_NTSC_TAB[i].SHF_Dir);
					isBreak = 1;
				}
				if (isBreak) break;
			}
			break;
		case MxL_MODE_ANA_BG:
			for (i = 0; i < Shfnum_BG; i++)
			{
				if ((RF_Freq >= (SHF_BG_TAB[i].Freq - SHF_BG_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_BG_TAB[i].Freq + SHF_BG_TAB[i].Freq_TH)*kHz))
				{
					if (SHF_BG_TAB[i].SHF_type == 2)
					{
						isBreak = 1;
						break;
					}
					SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_BG_TAB[i].SHF_Val);
					SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_BG_TAB[i].SHF_Dir);
					isBreak = 1;
				}
				if (isBreak) break;
			}
			break;
		case MxL_MODE_ANA_I:
		{
			/* Added V9.4.2.0 */
#if defined(MODULE)
			long long llRF_Freq = (long long)RF_Freq * 10;
#endif
			if (RF_Freq <= 682 * MHz)
			{
				SetIRVBit(IRV_RFTune, 0x85, 0xFF, 0x4E);
				SetIRVBit(IRV_RFTune, 0x86, 0x0F, 0x08);
			}
			/***** 2012-09-07 *****/
			//YWDRIVER_MODI modify by lf for change to int start
#if defined(MODULE)
			else if (((llRF_Freq >= (long long)6895 * MHz) && (llRF_Freq <= (long long)6905 * MHz)) ||
					 ((llRF_Freq >= (long long)7135 * MHz) && (llRF_Freq <= (long long)7145 * MHz)) ||
					 ((llRF_Freq >= (long long)7375 * MHz) && (llRF_Freq <= (long long)7385 * MHz)))
#else
			else if (((RF_Freq >= 689.5 * MHz) && (RF_Freq <= 690.5 * MHz)) || ((RF_Freq >= 713.5 * MHz) && (RF_Freq <= 714.5 * MHz)) || ((RF_Freq >= 737.5 * MHz) && (RF_Freq <= 738.5 * MHz)))
#endif
				//YWDRIVER_MODI modify by lf end
			{
				SetIRVBit(IRV_RFTune, 0x85, 0xFF, 0x40);
				SetIRVBit(IRV_RFTune, 0x86, 0x0F, 0x0F);
			}
			/***** 2012-09-07 *****/
			//YWDRIVER_MODI modify by lf for change to int start
#if defined(MODULE)
			else if (((llRF_Freq >= (long long)7615 * MHz) && (llRF_Freq <= (long long)7625 * MHz)) ||
					 ((llRF_Freq >= (long long)7855 * MHz) && (llRF_Freq <= (long long)7865 * MHz)) ||
					 ((llRF_Freq >= (long long)8095 * MHz) && (llRF_Freq <= (long long)8105 * MHz))
					 || ((llRF_Freq >= (long long)8335 * MHz) && (llRF_Freq <= (long long)8345 * MHz)) ||
					 ((llRF_Freq >= (long long)8575 * MHz) && (llRF_Freq <= (long long)8585 * MHz)) ||
					 ((llRF_Freq >= (long long)8815 * MHz) && (llRF_Freq <= (long long)8825 * MHz)))
#else
			else if (((RF_Freq >= 761.5 * MHz) && (RF_Freq <= 762.5 * MHz)) ||
					 ((RF_Freq >= 785.5 * MHz) && (RF_Freq <= 786.5 * MHz)) ||
					 ((RF_Freq >= 809.5 * MHz) && (RF_Freq <= 810.5 * MHz))
					 || ((RF_Freq >= 833.5 * MHz) && (RF_Freq <= 834.5 * MHz)) ||
					 ((RF_Freq >= 857.5 * MHz) && (RF_Freq <= 858.5 * MHz)) ||
					 ((RF_Freq >= 881.5 * MHz) && (RF_Freq <= 882.5 * MHz)))
#endif
				//YWDRIVER_MODI modify by lf end
			{
				SetIRVBit(IRV_RFTune, 0x85, 0xFF, 0x08);
				SetIRVBit(IRV_RFTune, 0x86, 0x0F, 0x0F);
			}
		}
		for (i = 0; i < Shfnum_I; i++)
		{
			if ((RF_Freq >= (SHF_I_TAB[i].Freq - SHF_I_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_I_TAB[i].Freq + SHF_I_TAB[i].Freq_TH)*kHz))
			{
				if (SHF_I_TAB[i].SHF_type == 2)
				{
					isBreak = 1;
					break;
				}
				SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_I_TAB[i].SHF_Val);
				SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_I_TAB[i].SHF_Dir);
				isBreak = 1;
			}
			if (isBreak) break;
		}
		break;
		case MxL_MODE_ANA_DKL:
			for (i = 0; i < Shfnum_DK; i++)
			{
				if ((RF_Freq >= (SHF_DK_TAB[i].Freq - SHF_DK_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_DK_TAB[i].Freq + SHF_DK_TAB[i].Freq_TH)*kHz))
				{
					if (SHF_DK_TAB[i].SHF_type == 2)
					{
						isBreak = 1;
						break;
					}
					SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_DK_TAB[i].SHF_Val);
					SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_DK_TAB[i].SHF_Dir);
					isBreak = 1;
				}
				if (isBreak) break;
			}
			break;
		case MxL_MODE_ANA_SECAM:
		case MxL_MODE_ANA_SECAM_ACC:
			for (i = 0; i < Shfnum_SECAM; i++)
			{
				if ((RF_Freq >= (SHF_SECAM_TAB[i].Freq - SHF_SECAM_TAB[i].Freq_TH)*kHz) && (RF_Freq <= (SHF_SECAM_TAB[i].Freq + SHF_SECAM_TAB[i].Freq_TH)*kHz))
				{
					if (SHF_SECAM_TAB[i].SHF_type == 2)
					{
						isBreak = 1;
						break;
					}
					SetIRVBit(IRV_RFTune, 0x61, 0xFF, SHF_SECAM_TAB[i].SHF_Val);
					SetIRVBit(IRV_RFTune, 0x62, 0x0F, SHF_SECAM_TAB[i].SHF_Dir);
					isBreak = 1;
				}
				if (isBreak) break;
			}
			break;
		default:
			break;
	}
	/*Generate one Array that Contain Data, Address */
	/*while (IRV_RFTune[Reg_Index].Num || IRV_RFTune[Reg_Index].Val) - Updated to have 0xFF as the limit - hchan 18/02/2010*/
	while (IRV_RFTune[Reg_Index].Num != 0xFF || IRV_RFTune[Reg_Index].Val != 0xFF)
	{
		pArray[Array_Index++] = IRV_RFTune[Reg_Index].Num;
		pArray[Array_Index++] = IRV_RFTune[Reg_Index].Val;
		Reg_Index++;
	}
	*Array_Size = Array_Index;
	return MxL_OK;
}
