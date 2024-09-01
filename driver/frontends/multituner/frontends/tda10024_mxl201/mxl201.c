/*
  mxl201.c  - MaxLinear MXL201 RF IC DVB-C Tuner

  Copyright (C) 2011 duckbox

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include "dvb_frontend.h"

#include "tda10024_platform.h"
#include "mxl201.h"

extern short paramDebug;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[mxl201] "

/* ************************************************** */

struct mxl201_config
{
	u32 if_freq;
	u32 xtal;
	u8  xtal_cap;
	u8  clk_out;
	u8  clk_amp;
	u8  si;
	u8  mode;
	u8  power;
	u8  lt;
};

struct mxl201_state
{
	struct i2c_adapter   *i2c;
	u8                   i2c_address;
	u32                  bw;
	u32                  freq;
	u32                  if_freq;
	u32                  power;
	u8                   initDone;
	struct mxl201_config *config;
};

/* ************************************************** */
static const u8 mxl201_lt_on[] =
{
	0x0C,0x00, 0xFF,0xFF
};

static const u8 mxl201_lt_off[] =
{
	0x0C,0x01, 0xFF,0xFF
};

static const u8 mxl201_power_d0[] =
{
		0x01,0x01, 0xFF,0xFF
};

static const u8 mxl201_power_d3[] =
{
		0x01,0x00, 0x10,0x00, 0xFF,0xFF
};

/* ************************************************** */

static int mxl201_i2c_read(struct dvb_frontend *fe, u32 index, int num, u8 *buf)
{
	struct mxl201_state *state = (struct mxl201_state*) fe->tuner_priv;
	int    res = 0;
	u8     addr_data[2];
	struct i2c_msg msg[2];
	int    old_num = num;
	u8*    old_buf = buf;
	u32    old_index = index;

	dprintk(200, "%s: index 0x%02x %d\n", __func__, index, num);

	for ( ; num-- ; ++index , ++buf)
	{
		addr_data[0] = 0xFB;
		addr_data[1] = (u8)(index);
		
		/* write */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = 0;
		msg[0].buf   = addr_data;
		msg[0].len   = 2;

		if ((res = i2c_transfer(state->i2c, msg, 1)) != 1)
		{
			printk("%s: 1. error on i2c_transfer (%d)\n", __func__, res);
			return res;
		}

		/* read */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = I2C_M_RD;
		msg[0].buf   = buf;
		msg[0].len   = 1;

		if (( res = i2c_transfer(state->i2c, msg, 1)) != 1)
		{
			printk("%s: 2. error on i2c_transfer (%d)\n", __func__, res);
			return res;
		}
	}
	{
		u8 i;
		u8 dstr[1024];

		dstr[0] = '\0';
		for (i = 0; i < old_num; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, old_buf[i]);
		}
		dprintk(200, "%s(): n: %u r: 0x%02x b: %s\n", __func__, old_num, old_index, dstr);
	}
	return 0;
}
	
static int mxl201_i2c_write(struct dvb_frontend *fe, u32 index, u32 num, u8 *buf)
{
	struct mxl201_state *state = (struct mxl201_state*) fe->tuner_priv;
	int    res = 0;
	u8     bytes[4];
	struct i2c_msg msg[2];

	if (num == 1)
	{
		bytes[0] = (u8)index;
		bytes[1] = *buf;
		{
			u8 i;
			u8 dstr[1024];

			dstr[0] = '\0';
			for (i = 0; i < 2; i++)
			{
				sprintf(dstr, "%s 0x%02x", dstr, bytes[i]);
			}
			dprintk(200, "%s(): n: %u b: %s\n", __func__, 2, dstr);
		}
		/* write */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = 0;
		msg[0].buf   = bytes;
		msg[0].len   = 2;

		if ((res = i2c_transfer(state->i2c, msg, 1)) != 1)
		{
			printk("%s: 1. error on i2c_transfer (%d)\n", __func__, res);
			return res;
		}
	}
	else
	{
		/* write */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = 0;
		msg[0].buf   = buf;
		msg[0].len   = num;
		{
			u8 i;
			u8 dstr[1024];
			dstr[0] = '\0';
			for (i = 0; i < num; i++)
			{
				sprintf(dstr, "%s 0x%02x", dstr, buf[i]);
			}
			dprintk(200, "%s(): n: %u b: %s\n", __func__, num, dstr);
		}
		if ((res = i2c_transfer(state->i2c, msg, 1)) != 1)
		{
			printk("%s: 2. error on i2c_transfer (%d)\n", __func__, res);
			return res;
		}
	}
	return 0;
}
	
static int mxl201_i2c_writeN(struct dvb_frontend *fe, const u8 *buf)
{
	int res = 0;
	u32 n   = 0;

	for ( ; (buf[n] & buf[n+1]) != 0xFF; n += 2);
	{
		if (n)
		{
			res |= mxl201_i2c_write(fe, 0, n, (u8*)buf);
		}
		else
		{
			res = -1;
		}
	}
	return res;
}

static int mxl201_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	int res = 0;
	
	dprintk(10, "%s >\n", __func__);

	if (fe->ops.i2c_gate_ctrl)
	{
		res |= fe->ops.i2c_gate_ctrl(fe, enable);
	}
	dprintk(10, "%s < %d\n", __func__, res);
	return res;
	}

/* ***************************************** irv functions ********************** */

typedef struct
{
	u8 Num;	/*Register number */
	u8 Val;	/*Register value */
} IRVType, *PIRVType;
	
static void SetIRVBit(PIRVType pIRV, u8 Num, u8 Mask, u8 Val)
{
	while (pIRV->Num || pIRV->Val)
	{
		if (pIRV->Num == Num)
		{
			pIRV->Val&=~Mask;
			pIRV->Val|=Val;
		}
		pIRV++;
	}	
}


static int rf_tune(u8 *pArray, u32 *Array_Size, u32 RF_Freq, u32 BWMHz)
{
	IRVType IRV_RFTune[]=
	{
		/*{ Addr, Data} */
		{ 0x10, 0x00 },  /*abort tune*/
		{ 0x0D, 0x15 },	
		{ 0x0E, 0x40 },	
		{ 0x0F, 0x0E },
		{ 0xAB, 0x10 },
		{ 0x91, 0x00 },
		/*{ 0x10, 0x01 },*/	/* start tune */
		{ 0, 0}
	};

	u32 dig_rf_freq = 0;
	u32 temp = 0;
	u32 Reg_Index = 0;
	u32 Array_Index = 0;
	u32 i = 0;
	u32 frac_divider = 1000000;

	dprintk(10, "%s >\n", __func__);

	switch (BWMHz)
	{
		case MxL_BW_6MHz:
		{
			SetIRVBit(IRV_RFTune, 0x0D, 0xFF, 0x49);
			break;
		}
		case MxL_BW_7MHz:
		{
			SetIRVBit(IRV_RFTune, 0x0D, 0xFF, 0x5A);
			break;
		}
		case MxL_BW_8MHz:
		{
			SetIRVBit(IRV_RFTune, 0x0D, 0xFF, 0x6F);
			break;
		}
		default:
		{
			printk("%s: wrong bw MHz %d\n", __func__, BWMHz);
			return -1;
		}
	}
	/*Convert RF frequency into 16 bits => 10 bit integer (MHz) + 6 bit fraction */
	dig_rf_freq = RF_Freq / MHz; /*Whole number portion of RF freq (in MHz) */
	temp = RF_Freq % MHz; /*Decimal portion of RF freq (in MHz) */
	for (i = 0; i < 6; i++)
	{
		dig_rf_freq <<= 1;
		frac_divider /=2;
		if (temp > frac_divider) /* Carryover from decimal */
		{
			temp -= frac_divider;
			dig_rf_freq++;
		}
	}

	/*add to have shift center point by 7.8124 kHz */
	if (temp > 7812)
	{
		dig_rf_freq ++;
	}
	SetIRVBit(IRV_RFTune, 0x0E, 0xFF, (u8)dig_rf_freq);
	SetIRVBit(IRV_RFTune, 0x0F, 0xFF, (u8)(dig_rf_freq >> 8));

	if (RF_Freq < 444000000)
	{
		SetIRVBit(IRV_RFTune, 0xAB, 0xFF, 0x70);
	}
	else if (RF_Freq < 667000000)
	{
		SetIRVBit(IRV_RFTune, 0xAB, 0xFF, 0x20);
	}
	else
	{
		SetIRVBit(IRV_RFTune, 0xAB, 0xFF, 0x10);
	}
	if (RF_Freq <= 334000000)
	{
		SetIRVBit(IRV_RFTune, 0x91, 0x40, 0x40);
	}
	else
	{
		SetIRVBit(IRV_RFTune, 0x91, 0x40, 0x00);
	}
	/*Generate one Array that Contain Data, Address  */
	while (IRV_RFTune[Reg_Index].Num || IRV_RFTune[Reg_Index].Val)
	{
		pArray[Array_Index++] = IRV_RFTune[Reg_Index].Num;
		pArray[Array_Index++] = IRV_RFTune[Reg_Index].Val;
		Reg_Index++;
	}
	*Array_Size=Array_Index;
	dprintk(10, "%s <\n", __func__);
	return 0;
}

static int Init(u8 *pArray, u32 *Array_Size, u32 Xtal_Freq_Hz, u32 IF_Freq_Hz,
		u8 Invert_IF, u8 Clk_Out_Enable, u8 Clk_Out_Amp, u8 Xtal_Cap)
{
	u32 Reg_Index = 0;
	u32 Array_Index = 0;

	IRVType IRV_Init_Cable[]=
	{
		/*{ Addr, Data}	 */
		{ 0x02, 0x06 },
		{ 0x03, 0x1A },
		{ 0x04, 0x14 },
		{ 0x05, 0x0E },
		{ 0x0C, 0x00 },
		{ 0x07, 0x14 },
		{ 0x29, 0x03 },
		{ 0x45, 0x01 },
		{ 0x7A, 0xCF },
		{ 0x7C, 0x7C },
		{ 0x7E, 0x27 },
		{ 0x93, 0xD7 },
		{ 0x99, 0x40 },
		{ 0x2F, 0x00 },
		{ 0x60, 0x60 },
		{ 0x70, 0x00 },
		{ 0xB9, 0x10 },
		{ 0x8E, 0x57 },
		{ 0x58, 0x08 },
		{ 0x5C, 0x00 },
		{ 0x01, 0x01 }, /*TOP_MASTER_ENABLE=1 */
		{ 0,    0    }
	};
	/*edit Init setting here */

	PIRVType myIRV = NULL;
	dprintk(10, "%s >\n", __func__);
	myIRV = IRV_Init_Cable;
	SetIRVBit(myIRV, 0x45, 0xFF, 0x01);
	SetIRVBit(myIRV, 0x7A, 0xFF, 0x6F);
	SetIRVBit(myIRV, 0x7C, 0xFF, 0x1C);
	SetIRVBit(myIRV, 0x7E, 0xFF, 0x7C);
	SetIRVBit(myIRV, 0x93, 0xFF, 0xE7);

	switch(IF_Freq_Hz)
	{
		case MxL_IF_4_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x01);
			break;
		}
		case MxL_IF_4_5_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x02);
			break;
		}
		case MxL_IF_4_57_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x03);
			break;
		}
		case MxL_IF_5_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x04);
			break;
		}
		case MxL_IF_5_38_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x05);
			break;
		}
		case MxL_IF_6_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x06);
			break;
		}
		case MxL_IF_6_28_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x07);
			break;
		}
		case MxL_IF_7_2_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x08);
			break;
		}
		case MxL_IF_35_25_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x09);
			break;
		}
		case MxL_IF_36_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x0A);
			break;
		}
		case MxL_IF_36_15_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x0B);
			break;
		}
		case MxL_IF_44_MHZ:
		{
			SetIRVBit(myIRV, 0x02, 0x0F, 0x0C);
			break;
		}
		default:
		{
			printk("%s wrong IF_Freq_Hz %d\n", __func__, IF_Freq_Hz);
			return -1;
		}
	}
	if (Invert_IF)
	{
		SetIRVBit(myIRV, 0x02, 0x10, 0x10);  /*Invert IF*/
	}
	else
	{
		SetIRVBit(myIRV, 0x02, 0x10, 0x00);  /*Normal IF*/
	}
	switch (Xtal_Freq_Hz)
	{
		case MxL_XTAL_16_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x00);
			SetIRVBit(myIRV, 0x58, 0x03, 0x03);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x36);
			break;
		}
		case MxL_XTAL_20_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x01);
			SetIRVBit(myIRV, 0x58, 0x03, 0x03);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x2B);
			break;
		}
		case MxL_XTAL_20_25_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x02);
			SetIRVBit(myIRV, 0x58, 0x03, 0x03);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x2A);
			break;
		}
		case MxL_XTAL_20_48_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x03);
			SetIRVBit(myIRV, 0x58, 0x03, 0x03);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x2A);
			break;
		}
		case MxL_XTAL_24_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x04);
			SetIRVBit(myIRV, 0x58, 0x03, 0x00);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x48);
			break;
		}
		case MxL_XTAL_25_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x05);
			SetIRVBit(myIRV, 0x58, 0x03, 0x00);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x45);
			break;
		}
		case MxL_XTAL_25_14_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x06);
			SetIRVBit(myIRV, 0x58, 0x03, 0x00);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x44);
			break;
		}
		case MxL_XTAL_28_8_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x08);
			SetIRVBit(myIRV, 0x58, 0x03, 0x00);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x3C);
			break;
		}
		case MxL_XTAL_32_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x09);
			SetIRVBit(myIRV, 0x58, 0x03, 0x00);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x36);
			break;
		}
		case MxL_XTAL_40_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x0A);
			SetIRVBit(myIRV, 0x58, 0x03, 0x00);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x2B);
			break;
		}
		case MxL_XTAL_44_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x0B);
			SetIRVBit(myIRV, 0x58, 0x03, 0x02);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x4E);
			break;
		}
		case MxL_XTAL_48_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x0C);
			SetIRVBit(myIRV, 0x58, 0x03, 0x02);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x48);
			break;
		}
		case MxL_XTAL_49_3811_MHZ:
		{
			SetIRVBit(myIRV, 0x04, 0x0F, 0x0D);
			SetIRVBit(myIRV, 0x58, 0x03, 0x02);
			SetIRVBit(myIRV, 0x5C, 0xFF, 0x45);
			break;
		}
		default:
		{
			printk("%s wrong Xtal_Freq_Hz %d\n", __func__, Xtal_Freq_Hz);
			return -1;
 	}
	}
	if (!Clk_Out_Enable) /*default is enable  */
	{
		SetIRVBit(myIRV, 0x03, 0x10, 0x00);
	}
	/* Clk_Out_Amp */
	SetIRVBit(myIRV, 0x03, 0x0F, Clk_Out_Amp);

	/* Xtal Capacitor */
	if (Xtal_Cap > 0 && Xtal_Cap <= 25)
	{
		SetIRVBit(myIRV, 0x05, 0xFF, Xtal_Cap);
	}
	else if (Xtal_Cap == 0)
	{
		SetIRVBit(myIRV, 0x05, 0xFF, 0x3F);
	}
	else
	{
		printk("%s wrong Xtal_Cap %d\n", __func__, Xtal_Cap);
		return -1;
	}
	/* Generate one Array that contains Data, Address */
	while (myIRV[Reg_Index].Num || myIRV[Reg_Index].Val)
	{
		pArray[Array_Index++] = myIRV[Reg_Index].Num;
		pArray[Array_Index++] = myIRV[Reg_Index].Val;
		Reg_Index++;
	}
	*Array_Size = Array_Index;
	dprintk(10, "%s <\n", __func__);
	return 0;
}

/* ***************************************************************** */

static int mxl201_reset(struct dvb_frontend *fe)
{
	int res = 0;
	u8  bytes[4] = {0xFF,0xFF,0xFF,0xFF};

	dprintk(10, "%s >\n", __func__);
	res |= mxl201_i2c_write(fe, 0, 2, bytes);
	dprintk(10, "%s < res %d\n", __func__, res);
	return res;
}
	
static int mxl201_get_id(struct dvb_frontend *fe)
{
	int    res = 0;
	u8     bytes[4];
	
	dprintk(10, "%s >\n", __func__);
	res |= mxl201_i2c_gate_ctrl(fe,1);
	if (res != 0)
	{
		printk("%s: 1. i2c-gate-error (%d)\n", __func__, res);
		return res;
	}
	res |= mxl201_i2c_read(fe, 0x15, 0x01, bytes);
	res |= mxl201_i2c_gate_ctrl(fe, 0);
	if (res != 0)
	{
		printk("%s: 2. i2c-gate-error (%d)\n", __func__, res);
		return res;
	}
	if ((bytes[0] & 0x0F) != 0x06)
	{
		res = -1;
		printk("%s: mxl201 not found/responding\n", __func__);
	}
	else
	{
		printk("%s: mxl201 detected\n", __func__);
	}
	dprintk(10, "%s < res %d\n", __func__, res);
	return res;
}
	
static int mxl201_init (struct dvb_frontend *fe)
{
	struct mxl201_state *state = (struct mxl201_state*) fe->tuner_priv;
	int    res = 0;
	u32    num = 0;
	u8     buf[MAX_ARRAY_SIZE];
	
	dprintk(10, "%s >\n", __func__);

	if (state->initDone == 1)
	{
		 printk("%s: init already done. ignoring ... <\n", __func__);
		 return 0;
	}
	/* default values */
	state->bw = MxL_BW_8MHz;
	state->if_freq = 6000000;

	if (state->config->if_freq)
	{
		state->if_freq = state->config->if_freq;
	}

	res |= mxl201_i2c_gate_ctrl(fe,1);
								
	if (res != 0)
	{
		printk("%s: i2c-gate-error (%d)\n", __func__, res);
		return res;
	}
	res |= mxl201_reset(fe);
	
	if (res != 0)
	{
		printk("%s: reset-error (%d)\n", __func__, res);
		res |= mxl201_i2c_gate_ctrl(fe,0);
		return res;
	}
	res |= Init(buf, &num, state->config->xtal, state->if_freq, state->config->si,
		state->config->clk_out, state->config->clk_amp, state->config->xtal_cap);

	if (res != 0)
	{
		printk("%s: init-error (%d)\n", __func__, res);
		res |= mxl201_i2c_gate_ctrl(fe,0);
		return res;
	}
	res |= mxl201_i2c_write(fe, 0, num, buf);
	res |= mxl201_i2c_writeN(fe, state->config->lt ? mxl201_lt_on : mxl201_lt_off);
	if (res != 0)
	{
		printk("%s: error (%d)\n", __func__, res);
		res |= mxl201_i2c_gate_ctrl(fe,0);
		return res;
	}
	state->initDone  = 1;
	res |= mxl201_i2c_gate_ctrl(fe,0);
	dprintk(10, "%s < res %d\n", __func__, res);
	return res;
}

static int mxl201_sleep(struct dvb_frontend *fe)
{
	struct mxl201_state *state = (struct mxl201_state*) fe->tuner_priv;
	int                 res = 0;
	
	dprintk(10, "%s >\n", __func__);

	res |= mxl201_i2c_gate_ctrl(fe,1);
	
	if (res != 0)
	{
		printk("%s: i2c-gate-error (%d)\n", __func__, res);
		return res;
	}

/* fixme: in orig driver this is configurable */
	res |= mxl201_i2c_writeN(fe, mxl201_power_d3);

	if (res != 0)
	{
		printk("%s: writeN error (%d)\n", __func__, res);
		res |= mxl201_i2c_gate_ctrl(fe,0);
		return res;
	}
	state->power = 1;

	res |= mxl201_i2c_gate_ctrl(fe,0);
	
	if (res != 0)
	{
		printk("%s: i2c-gate-error (%d)\n", __func__, res);
		return res;
	}
	dprintk(10, "%s < res %d\n", __func__, res);
	return res;
}

static int mxl201_wakeup(struct dvb_frontend *fe)
{
	struct mxl201_state *state = (struct mxl201_state*)fe->tuner_priv;
	int                 res = 0;
	
	dprintk(10, "%s >\n", __func__);
	res |= mxl201_i2c_writeN(fe, mxl201_power_d0);
	state->power = 0;
	dprintk(10, "%s < res %d\n", __func__, res);
	return res;
}
	
static int mxl201_set_params(struct dvb_frontend *fe, struct dvb_frontend_parameters *params)
{
	struct mxl201_state *state = (struct mxl201_state*) fe->tuner_priv;
#if DVB_API_VERSION >= 5
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
#endif
	int res = 0;
	u32 nbytes = 0;
	u8  bytes[MAX_ARRAY_SIZE];
	u8  val8;

	dprintk(10, "%s >\n", __func__);
	res |= mxl201_i2c_gate_ctrl(fe,1);
	if (res != 0)
	{
		printk("%s: i2c-gate-error (%d)\n", __func__, res);
		return res;
	}
	if (state->power)
	{
		res |= mxl201_wakeup(fe);
	
		if (res != 0)
		{
			res |= mxl201_i2c_gate_ctrl(fe,0);
			printk("%s: wakeup-error (%d)\n", __func__, res);
			return res;
		}
	}
#if DVB_API_VERSION >= 5
	if (c->symbol_rate >= 5900000)
#else
	if (params->u.qam.symbol_rate >= 5900000)
#endif
	{
		state->bw = MxL_BW_8MHz;
	}
	else
	{
		state->bw = MxL_BW_6MHz;
	}
#if DVB_API_VERSION >= 5
	printk("%s freq %d\n", __func__, c->frequency);

	res |= rf_tune(bytes, &nbytes, c->frequency, state->bw);
#else
	printk("%s freq %d\n", __func__, params->frequency);

	res |= rf_tune(bytes, &nbytes, params->frequency, state->bw);
#endif
	if (res != 0)
	{
		printk("%s: tune error (%d)\n", __func__, res);
		res |= mxl201_i2c_gate_ctrl(fe,0);
		return res;
	}
	res |= mxl201_i2c_read(fe, 0x3E, 1, &val8);
	val8 &= 0xDF;
	res |= mxl201_i2c_write(fe, 0x3E, 1, &val8);
	res |= mxl201_i2c_write(fe, 0, nbytes, bytes);
	
	if (res != 0)
	{
		printk("%s: i2c-write-error (%d)\n", __func__, res);
		res |= mxl201_i2c_gate_ctrl(fe,0);
		return res;
	}
	mdelay(1);
	val8 |= 0x20;
	res |= mxl201_i2c_write(fe, 0x3E, 1, &val8);
	mdelay(2);
	bytes[0] = 0x10;
	bytes[1] = 0x01;
	res |= mxl201_i2c_write(fe, 0, 2, bytes);
#if DVB_API_VERSION >= 5
	state->freq = c->frequency;
#else
	state->freq = params->frequency;
#endif
	res |= mxl201_i2c_gate_ctrl(fe,0);
								
	if (res != 0)
	{
		printk("%s: i2c-gate-error (%d)\n", __func__, res);
		return res;
	}
	dprintk(10, "%s < res %d\n", __func__, res);
	return res;
}
	
static int mxl201_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct mxl201_state *state = (struct mxl201_state*)fe->tuner_priv;
	int    res = 0;
	
	dprintk(10, "%s >\n", __func__);
	*frequency = state->freq;
	dprintk(10, "%s < res %d, freq %d\n", __func__, res, *frequency);
	return res;
}

static int mxl201_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct mxl201_state *state = (struct mxl201_state*)fe->tuner_priv;
	int                 res = 0;

	dprintk(10, "%s >\n", __func__);
	*bandwidth = state->bw;
	dprintk(10, "%s < res %d, bw %d\n", __func__, res, *bandwidth);
	return res;
}
	
static int mxl201_get_status(struct dvb_frontend* fe, u32 *status)
{
	int res = 0;
	u8  byte = 0x00;
	
	dprintk(10, "%s >\n", __func__);
	*status = 0;
	res |= mxl201_i2c_gate_ctrl(fe,1);
	res |= mxl201_i2c_read(fe, 0x14, 0x01, &byte);
	res |= mxl201_i2c_gate_ctrl(fe,0);
	
	if (res != 0)
	{
		printk("%s: i2c-error (%d)\n", __func__, res);
		return res;
	}

	if ((byte & 0x0F) == 0x0F)
	{
		 dprintk(10, "%s TUNER_STATUS_LOCKED\n", __func__);
		 *status |= TUNER_STATUS_LOCKED;
	}
	/* fixme:
	if ((byte & 0x03) == 0x03)
	{
	*status|= TUNER_STATUS_LO1;
	}
	if ((byte & 0x0C) == 0x0C)
	{
		*status|= TUNER_STATUS_LO2;
	}
	*/
	dprintk(10, "%s < res %d, status %d\n", __func__, res, *status);
	return res;
	}
	
static int mxl201_release(struct dvb_frontend *fe)
{
	int res = 0;
	
	/* noop ? */
	return res;
}
	
static struct dvb_tuner_ops mxl201_tuner_ops =
{
	.info =
	{
		.name           = "MxL201 ES4",
		.frequency_min  = 44000000,
		.frequency_max  = 1002000000,
		.frequency_step = 100000,
		.bandwidth_min  = 6000000,
		.bandwidth_max  = 7000000,
		.bandwidth_step = 1000000
	},
	.release       = mxl201_release,
	.init          = mxl201_init,
	.sleep         = mxl201_sleep,
	.set_params    = mxl201_set_params,
	.get_frequency = mxl201_get_frequency,
	.get_bandwidth = mxl201_get_bandwidth,
	.get_status    = mxl201_get_status,
};
	
int mxl201_attach(struct dvb_frontend *fe, struct mxl201_private_data_s *mxl201, struct i2c_adapter *i2c, u8 i2c_address)
{
	struct mxl201_state  *state = kmalloc(sizeof(struct mxl201_state), GFP_KERNEL);
	struct mxl201_config *cfg = kmalloc(sizeof(struct mxl201_config), GFP_KERNEL);

	dprintk(10, "%s >\n", __func__);

	state->i2c = i2c;
	state->i2c_address = i2c_address;

	dprintk(10, "%s 0x%p 0x%x\n", __func__, i2c, i2c_address);

	memcpy(&fe->ops.tuner_ops, &mxl201_tuner_ops, sizeof(struct dvb_tuner_ops));
	fe->tuner_priv = state;

	cfg->if_freq  = mxl201->if_freq;
	cfg->xtal     = mxl201->xtal;
	cfg->xtal_cap = mxl201->xtal_cap;
	cfg->clk_out  = mxl201->clk_out;
	cfg->clk_amp  = mxl201->clk_amp;
	cfg->si       = mxl201->si;
	cfg->mode     = mxl201->mode;
	cfg->power    = mxl201->power;
	cfg->lt       = mxl201->lt;

	state->config = cfg;

	state->power     = 1;
	state->initDone  = 0;

	if (mxl201_get_id(fe) < 0)
	{
		printk("%s: failed\n",__func__);
		return -1;
	}
	dprintk(10, "%s <\n", __func__);
	return 0;
}
// vim:ts=4
