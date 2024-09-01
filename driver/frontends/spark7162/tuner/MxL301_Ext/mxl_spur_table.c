/******************************************************
mxl_spur_table.c
----------------------------------------------------
Rf IC control functions

<Revision History>
'11/10/07 : OKAMOTO Modify SHF_DVBT_TAB for SH.
'11/10/03 : OKAMOTO Update to "MxL301RF_API_ Files_V9.4.5.0".
----------------------------------------------------
Copyright(C) 2011 SHARP CORPORATION
******************************************************/

/*
 Analog/Cable spur channel tables for each application mode : V9.2.7.0 - 06/10/2010

 Copyright, Maxlinear, Inc.
 All Rights Reserved

 File Name: mxl_spur_table.c

 Description: The file is for MxL30xRF user to quickly modify the spur frequency table for their own purpose.
 There are 5 spur shifting tables for NTSC, BG, DK, I and SECAM seperately. In genral, MxL provides the
 required spur shifing table based on MxL30xRF EVM. But,if customer wnats to add more channels to improve
 the spur performance based on cusotmer evaluation board, customer will add this table using below table.

 Freq : Channel center frequency.
 Freq_TH : Frequency range from low to high to be adopted for spur shifing.
 Val : Spur shifting value.
 Dir : Spur shifing direction.
 Type : Depend on IF split type
*/

/* #include "StdAfx.h" */
#include <types.h>
#include "mxl_common.h"

/* Spur shifting table number for analog mode : V9.2.7.0 */
UINT8 Shfnum_NTSC = 32;
UINT8 Shfnum_BG = 29;
UINT8 Shfnum_DK = 10;
UINT8 Shfnum_I = 12;
#if 0
UINT8 Shfnum_SECAM = 1;
#else
/* '11/10/03 : OKAMOTO Update to "MxL301RF_API_ Files_V9.4.5.0". */
UINT8 Shfnum_SECAM = 2;
#endif
UINT8 Shfnum_CABLE = 25;

#if 0
UINT8 Shfnum_DVBT = 14;
#else
/* '11/10/07 : OKAMOTO Modify SHF_DVBT_TAB for SH. */
UINT8 Shfnum_DVBT = 17;
#endif

UINT8 Shfnum_ATSC = 1;

SHFType SHF_NTSC_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */
	{ 75000, 200, 0xD4, 0x07, 0}, /*1*/
	{ 81000, 500, 0x74, 0x07, 0},
	{ 99000, 150, 0x88, 0x07, 0},
	{ 117000, 150, 0x88, 0x07, 0},
	{ 135000, 150, 0x98, 0x07, 0},
	{ 153000, 150, 0x98, 0x07, 0},
	{ 165000, 500, 0xCE, 0x07, 0},
	{ 171000, 500, 0xBA, 0x07, 0},
	{ 189000, 150, 0x46, 0x04, 0},
	{ 219000, 150, 0xD4, 0x07, 0},
	{ 261000, 500, 0xCE, 0x07, 0}, /*11*/
	{ 303000, 250, 0x1E, 0x04, 0}, /* V9.4.2.0-added */
	{ 332950, 49, 0x2C, 0x04, 0},
	{ 333050, 50, 0xD4, 0x07, 0},
	{ 339000, 150, 0x00, 0x04, 2},
	{ 345000, 150, 0x3E, 0x04, 0},
	{ 357000, 150, 0x70, 0x07, 0},
	{ 375000, 150, 0x00, 0x04, 0},
	{ 381000, 150, 0x00, 0x04, 0},
	{ 387000, 150, 0x2C, 0x04, 0},
	{ 393000, 150, 0x74, 0x07, 0}, /*21*/
	{ 411000, 150, 0x74, 0x07, 0},
	{ 429000, 150, 0x4C, 0x04, 0},
	{ 447000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 519000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 537000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 555000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 591000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 627000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 699000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 807000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */ /*31*/
	{ 843000, 250, 0x2C, 0x04, 0}, /* V9.4.2.0-added */ /*32*/
};

/* Spur shifting table for analg NTSC mode : V9.2.7.0 */
SHFType SHF_BG_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */

	{ 64500, 500, 0x8C, 0x07, 0}, /*1*/
	{ 99500, 500, 0x92, 0x07, 0}, /* V9.4.2.0-added */
	{ 114500, 500, 0x32, 0x04, 0},
	{ 135500, 500, 0xA6, 0x07, 0},
	{ 149500, 500, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 170500, 500, 0x46, 0x04, 0},
	{ 191500, 500, 0xD4, 0x07, 0},
	{ 205500, 500, 0x2C, 0x04, 0},
	{ 212500, 500, 0xAA, 0x04, 0}, /* V9.4.2.0-added */
	{ 226500, 500, 0xCE, 0x07, 0}, /*10*/
	{ 240500, 500, 0x24, 0x04, 0},
	{ 261500, 500, 0x46, 0x04, 0}, /* V9.4.2.0-added */
	{ 275500, 500, 0x78, 0x04, 0}, /* V9.4.2.0-added */
	{ 296500, 500, 0xC4, 0x07, 0},
	{ 306000, 500, 0xCE, 0x07, 0},
	{ 314000, 500, 0xC4, 0x07, 0},
	{ 330000, 500, 0xB0, 0x07, 0}, /* V9.4.2.0-chaged 0xB4 t0 0xB0 */
	{ 394000, 500, 0x2C, 0x04, 0},
	{ 402000, 500, 0xC0, 0x07, 0},
	{ 410000, 500, 0x2C, 0x04, 0}, /* V9.4.2.0-added */ /*20*/
	{ 450000, 500, 0x6A, 0x04, 0},
	{ 522000, 500, 0x5A, 0x04, 0},
	{ 562000, 500, 0x36, 0x04, 0},
	{ 586000, 500, 0x6E, 0x04, 0}, /* V9.4.2.0-added */
	{ 594000, 500, 0x50, 0x04, 0},
	{ 618000, 500, 0x00, 0x04, 0},
	{ 706000, 500, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 778000, 500, 0x2C, 0x04, 0}, /* V9.4.2.0-added */
	{ 810000, 500, 0xD4, 0x07, 0}, /* V9.4.2.0-chaged 0x3C t0 0xD8 */ /*29*/
};

/* Spur shifting table for analg NTSC mode : V9.2.7.0 */
SHFType SHF_DK_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */
	{ 155000, 500, 0xD4, 0x07, 0}, /*1*/
	{ 171000, 500, 0x5A, 0x04, 0},
	{ 187000, 500, 0x2C, 0x04, 0},
	{ 227000, 500, 0xD8, 0x07, 0},
	{ 243000, 500, 0x3C, 0x04, 0},
	{ 259000, 500, 0xAC, 0x07, 0},
	{ 299000, 500, 0x4A, 0x04, 0},
	{ 522000, 500, 0x5A, 0x04, 0},
	{ 562000, 500, 0x36, 0x04, 0},
	{ 594000, 500, 0x50, 0x04, 0}, /*10*/
};

/* Spur shifting table for analg NTSC mode : V9.2.7.0 */
SHFType SHF_I_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */
	{ 46000, 500, 0x74, 0x07, 0}, /* V9.4.2.0-added */
	{ 63000, 500, 0x9A, 0x04, 0}, /* V9.4.2.0-added */
	{ 64500, 500, 0x7E, 0x07, 0}, /* V9.4.2.0-added */
	{ 690000, 500, 0x50, 0x04, 1}, /* V9.4.2.0-added */
	{ 714000, 500, 0x50, 0x04, 1}, /* V9.4.2.0-added */
	{ 738000, 500, 0x50, 0x04, 1}, /* V9.4.2.0-added */
	{ 762000, 500, 0x8C, 0x04, 1}, /* V9.4.2.0-added */
	{ 786000, 500, 0x8C, 0x04, 1}, /* V9.4.2.0-added */
	{ 810000, 500, 0x8C, 0x04, 1}, /* V9.4.2.0-added */
	{ 834000, 500, 0x8C, 0x04, 1}, /* V9.4.4.0-added */
	{ 858000, 500, 0x8C, 0x04, 1}, /* V9.4.4.0-added */
	{ 882000, 500, 0x8C, 0x04, 1}, /* V9.4.4.0-added */ /*12*/
};

/* Spur shifting table for analg NTSC mode : V9.2.7.0 */
SHFType SHF_SECAM_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */
#if 0
	{ 64500, 500, 0x74, 0x07, 0}, /*1*/
#else
	/* '11/10/03 : OKAMOTO Update to "MxL301RF_API_ Files_V9.4.5.0". */
	{ 63500, 500, 0xC0, 0x04, 0}, /*1*/
	{ 66500, 500, 0xCE, 0x07, 0}, /*2*/
#endif
};

/* Spur shifting table for Cable mode : V9.2.7.0 */
SHFType SHF_CABLE_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */
	{ 61750, 500, 0x96, 0x04, 0},
	{ 63000, 500, 0xC0, 0x04, 0},
	{ 79000, 500, 0x3C, 0x04, 0},
	{ 79750, 500, 0x6E, 0x04, 0},
	{ 151750, 500, 0x3C, 0x04, 0},
	{ 169750, 500, 0x3C, 0x04, 0},
	{ 187750, 500, 0x3C, 0x04, 0},
	{ 205750, 500, 0x3C, 0x04, 0},
	{ 223750, 500, 0x3C, 0x04, 0},
	{ 259750, 500, 0x3C, 0x04, 0},
	{ 307750, 500, 0xE2, 0x07, 0},
	{ 379750, 500, 0xE2, 0x07, 0},
	{ 415750, 500, 0xE2, 0x07, 0},
	{ 439750, 500, 0x1E, 0x04, 0},
	{ 523750, 500, 0x3C, 0x04, 0},
	{ 565750, 500, 0x50, 0x04, 0},
	{ 594750, 500, 0xE2, 0x07, 0},
	{ 631750, 500, 0xE2, 0x07, 0},
	{ 637750, 500, 0x50, 0x04, 0},
	{ 643750, 500, 0x32, 0x04, 0},
	{ 703750, 500, 0xC4, 0x07, 0},
	{ 775750, 500, 0xE2, 0x07, 0},
	{ 811750, 500, 0xE2, 0x07, 0},
	{ 835750, 500, 0x5A, 0x04, 0},
	{ 847750, 500, 0xCE, 0x07, 0},
};

/* Spur shifting table for DVB-T mode : V9.2.7.0 */
SHFType SHF_DVBT_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */
	{ 64500, 500, 0x92, 0x07, 0}, /* V9.4.2.0-added DVB-T */ /*1*/
	{ 191500, 300, 0xE2, 0x07, 0}, /* V9.4.2.0-added DVB-T */
	{ 205500, 500, 0x2C, 0x04, 0}, /* V9.4.2.0-added DVB-T */
	{ 212500, 500, 0x1E, 0x04, 0}, /* V9.4.2.0-added DVB-T */
	{ 226500, 500, 0xD4, 0x07, 0}, /* V9.4.2.0-added DVB-T */ /*5*/
	{ 99143, 500, 0x9C, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 173143, 500, 0xD4, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 191143, 300, 0xD4, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 207143, 500, 0xCE, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 225143, 500, 0xCE, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 243143, 500, 0xD4, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 261143, 500, 0xD4, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 291143, 500, 0xD4, 0x07, 0}, /* V9.4.2.0-added ISDB-T */
	{ 339143, 500, 0x2C, 0x04, 0}, /* V9.4.2.0-added ISDB-T *//*14*/

	/* '11/10/07 : OKAMOTO Modify SHF_DVBT_TAB for SH. */
	{ 117143, 500, 0x7A, 0x07, 0}, /* Added by Yamaguchi ISDB-T */
	{ 135143, 300, 0x7A, 0x07, 0}, /* Added by Yamaguchi ISDB-T */
	{ 153143, 500, 0x01, 0x07, 0}, /* Added by Yamaguchi ISDB-T *//*17*/
};

/* Spur shifting table for ATSC mode : V9.2.7.0 */
SHFType SHF_ATSC_TAB[] =
{
	/*{ Freq(kHz),Offset(kHz), Val, Dir, type}, */
	{ 79000, 500, 0x3C, 0x04, 0}, /*1*/
};
