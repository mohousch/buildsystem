#ifndef mxl201_123
#define mxl201_123

/*
  mxl201  - DVB-C Tuner

  Copyright (C) 2011 duckbox

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define dprintk(level, x...) do \
{ \
	if ((paramDebug) && (paramDebug >= level) || level == 0) \
	{ \
		printk(TAGDEBUG x); \
	} \
} while (0)

#define MXL201_LT_OFF    0 
#define MXL201_LT_ON     1

#define MXL201_POWER_D0  0
#define MXL201_POWER_D1  1
#define MXL201_POWER_D2  2
#define MXL201_POWER_D3  3

/* Enumeration of Acceptable Crystal Frequencies */ 
typedef enum 
{ 
	MxL_XTAL_16_MHZ      = 16000000, 
	MxL_XTAL_20_MHZ      = 20000000, 
	MxL_XTAL_20_25_MHZ   = 20250000, 
	MxL_XTAL_20_48_MHZ   = 20480000, 
	MxL_XTAL_24_MHZ      = 24000000, 
	MxL_XTAL_25_MHZ      = 25000000, 
	MxL_XTAL_25_14_MHZ   = 25140000, 
	MxL_XTAL_27_MHZ      = 27000000, 
	MxL_XTAL_28_8_MHZ    = 28800000, 
	MxL_XTAL_32_MHZ      = 32000000, 
	MxL_XTAL_40_MHZ      = 40000000, 
	MxL_XTAL_44_MHZ      = 44000000, 
	MxL_XTAL_48_MHZ      = 48000000, 
	MxL_XTAL_49_3811_MHZ = 49381100	 
} MxL201RF_Xtal_Freq; 

/* Enumeration of Acceptable IF Frequencies */ 
typedef enum 
{ 
	MxL_IF_4_MHZ         = 4000000, 
	MxL_IF_4_5_MHZ       = 4500000, 
	MxL_IF_4_57_MHZ      = 4570000, 
	MxL_IF_5_MHZ         = 5000000, 
	MxL_IF_5_38_MHZ      = 5380000, 
	MxL_IF_6_MHZ         = 6000000, 
	MxL_IF_6_28_MHZ      = 6280000, 
	MxL_IF_7_2_MHZ       = 7200000, 
	MxL_IF_35_25_MHZ     = 35250000, 
	MxL_IF_36_MHZ        = 36000000, 
	MxL_IF_36_15_MHZ     = 36150000, 
	MxL_IF_44_MHZ        = 44000000 
} MxL201RF_IF_Freq; 

typedef enum
{
	MxL_BW_6MHz = 6,
	MxL_BW_7MHz = 7,
	MxL_BW_8MHz = 8
} MxL201RF_BW_MHz;

#ifndef MHz
	#define MHz 1000000
#endif

#define MAX_ARRAY_SIZE 100

extern int mxl201_attach(struct dvb_frontend *fe, struct mxl201_private_data_s *mxl201, struct i2c_adapter *i2c, u8 i2c_address);

#endif
// vim:ts=4
