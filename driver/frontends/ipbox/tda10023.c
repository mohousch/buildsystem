/*
 TDA10023 - DVB-C decoder
 (as used in Philips CU1216-3 NIM and the Reelbox DVB-C tuner card)

 Copyright (C) 2005 Georg Acher, BayCom GmbH (acher at baycom dot de)
 Copyright (c) 2006 Hartmut Birr (e9hack at gmail dot com)

 Remotely based on tda10021.c
 Copyright (C) 1999 Convergence Integrated Media GmbH <ralph@convergence.de>
 Copyright (C) 2004 Markus Schulz <msc@antzsystem.de>
 Support for TDA10021

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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <asm/div64.h>

#include <linux/completion.h>
#include <linux/kthread.h>

#include "dvb_frontend.h"
#include "tda1002x.h"

#define REG0_INIT_VAL 0x23
#define MXL201_I2C_ADDR 0x60 // (0xC0 >> 1)
#define MHz 1000000

struct tda10023_state
{
	struct i2c_adapter *i2c;
	/* configuration settings */
	const struct tda10023_config *config;
	struct dvb_frontend frontend;

	/* clock settings */
	u32 xtal;
	u32 sysclk;

	u8 pwm;
	u8 reg0;

	u8 pll_m;
	u8 pll_p;
	u8 pll_n;
};

static int lock_tuner(struct tda10023_state *state);
static int unlock_tuner(struct tda10023_state *state);

#define dprintk(fmt, args...) printk(fmt, ##args)

static int verbose;

/* Talking to MxL201 tuner */
typedef struct
{
	u8 Num; /*Register number */
	u8 Val; /*Register value */
} IRVType, *PIRVType;

/* local functions called by Init and RFTune */
static u32 SetIRVBit(PIRVType pIRV, u8 Num, u8 Mask, u8 Val)
{
	while (pIRV->Num || pIRV->Val)
	{
		if (pIRV->Num == Num)
		{
			pIRV->Val &= ~Mask;
			pIRV->Val |= Val;
		}
		pIRV++;
	}
	return 0;
}

static int MxL_WriteReg(struct tda10023_state *state, u8 addr, u8 data)
{
	int ret;
	u8 buf[2] = { addr, data };
	struct i2c_msg msg = { .addr = MXL201_I2C_ADDR, .flags = 0, .buf = buf, .len = 2 };
	ret = i2c_transfer(state->i2c, &msg, 1);
	if (ret != 1)
	{
		dprintk("Mxl write reg failed, %d\n", ret);
	}
	return ret;
}

static u8 MxL_ReadReg(struct tda10023_state *state, u8 reg)
{
	u8 b1 [1] = { 0 };
	struct i2c_msg msg [] = { { .addr = MXL201_I2C_ADDR, .flags = I2C_M_RD, .buf = b1, .len = 1 } };
	int ret = 0;
	ret = MxL_WriteReg(state, 0xFB, reg); // Tell which reg we want to read, needs to be written to reg 0xFB
	ret += i2c_transfer(state->i2c, msg, 1); // Read the reg we want
	if (ret != 2)
	{
		int num = state->frontend.dvb ? state->frontend.dvb->num : -1;
		printk(KERN_ERR "DVB: MxL201RF(%d): %s: readreg error "
			   "(reg == 0x%02x, ret == %i)\n",
			   num, __func__, reg, ret);
	}
	return b1[0];
}

static void MxL_Soft_Reset(struct tda10023_state *state)
{
	int ret = MxL_WriteReg(state, 0xFF, 0xFF);
	if (ret != 1)
	{
		dprintk("Mxl Reset failed\n");
	}
}

static void MxL_Check_ChipVersion(struct tda10023_state *state)
{
	u8 ver = 0;
	ver = MxL_ReadReg(state, 0x15);
//	if ( (ver & 0x0F) == 0x06 ) // Chip Version ES4
	if (ver == 0)
	{
		dprintk("Mxl reading chip version failed\n");
	}
	else
		printk("MxL Version: %s (0x%x)\n", (ver & 0x0F) == 0x06 ? "ES4" : "ES3", (ver & 0x0F));
}

static void MxL_Stand_By(struct tda10023_state *state)
{
	int ret;
	ret = MxL_WriteReg(state, 0x01, 0x0);
	ret = MxL_WriteReg(state, 0x10, 0x0);
	if (ret != 1)
		dprintk("MxL Stand By failed\n");
}

static void MxL_Wake_Up(struct tda10023_state *state)
{
	int ret = MxL_WriteReg(state, 0x01, 0x01);
	if (ret != 1)
		dprintk("MxL Wake Up failed\n");
}

static u32 MxL201RF_ES4_Init(struct tda10023_state *state)
{
	int ret = 0;
	u32 Reg_Index = 0;
	IRVType IRV_Init_Cable[] =
	{
		/*{ Addr, Data} */
		{ 0x02, 0x06},
		{ 0x03, 0x1A},
		{ 0x04, 0x14},
		{ 0x05, 0x0E},
		{ 0x07, 0x14},
		{ 0x29, 0x03},
		{ 0x45, 0x01},
		{ 0x7A, 0xCF},
		{ 0x7C, 0x7C},
		{ 0x7E, 0x27},
		{ 0x93, 0xD7},
		{ 0x2F, 0x00},
		{ 0x60, 0x60},
		{ 0x70, 0x00},
		{ 0xB9, 0x00},
		{ 0x01, 0x01}, /*TOP_MASTER_ENABLE=1 */
		{ 0, 0}
	};
	/*edit Init setting here */
	PIRVType myIRV = IRV_Init_Cable;
	/*Cable Standard Mode */
	SetIRVBit(myIRV, 0x45, 0xFF, 0x01);
	SetIRVBit(myIRV, 0x7A, 0xFF, 0x6F);
	SetIRVBit(myIRV, 0x7C, 0xFF, 0x1C);
	SetIRVBit(myIRV, 0x7E, 0xFF, 0x7C);
	SetIRVBit(myIRV, 0x02, 0x0F, 0x06); // MxL_IF_6_MHZ
	SetIRVBit(myIRV, 0x02, 0x10, 0x00); // Normal IF
	SetIRVBit(myIRV, 0x04, 0x0F, 0x00); // MxL_XTAL_16_MHZ
	/* Clk_Out_Amp */
	SetIRVBit(myIRV, 0x03, 0x0F, 7); // MxL_CLKOUT_AMP_7
	/* Xtal Capacitor */
	SetIRVBit(myIRV, 0x05, 0xFF, 25); // MxL_XTAL_CAP_25_PF
	/* Generate one Array that Contain Data, Address */
	while (myIRV[Reg_Index].Num || myIRV[Reg_Index].Val)
	{
		ret = MxL_WriteReg(state, myIRV[Reg_Index].Num, myIRV[Reg_Index].Val);
		if (ret != 1)
		{
			dprintk("Mxl201 ES4 Init failed\n");
			break;
		}
		Reg_Index++;
	}
	return 0;
}

static void MxL_Init(struct tda10023_state *state)
{
	lock_tuner(state);
	MxL_Soft_Reset(state);
	//MxL_Check_ChipVersion(state);
	MxL201RF_ES4_Init(state);
	MxL_Stand_By(state);
	MxL_Wake_Up(state);
	unlock_tuner(state);
}

static u32 MxL201RF_ES4_RFTune(struct tda10023_state *state, struct dvb_frontend_parameters *p)
{
	int ret;
	u32 RF_Freq = p->frequency;
	//u32 BWMHz = 8000000/1000000;
	IRVType IRV_RFTune[] =
	{
		/*{ Addr, Data} */
		{ 0x10, 0x00}, /*abort tune*/
		{ 0x0D, 0x15},
		{ 0x0E, 0x40},
		{ 0x0F, 0x0E},
		{ 0xAB, 0x10},
		{ 0x91, 0x00},
		{ 0x10, 0x01}, /* start tune */
		{ 0, 0}
	};
	u32 dig_rf_freq = 0;
	u32 rf_freq_MHz = 0;
	u32 temp = 0 ;
	u32 Reg_Index = 0;
	u32 i = 0;
	u32 frac_divider = 1000000;
	rf_freq_MHz = RF_Freq / MHz;
	//dprintk("tda10023: Freq %dMHz\n", rf_freq_MHz);
	/*CABLE */
	// MxL_MODE_CAB_STD:
	//MxL_MODE_CAB_OPT1:
	SetIRVBit(IRV_RFTune, 0x0D, 0xFF, 0x6F); // MxL_BW_8MHz
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
	SetIRVBit(IRV_RFTune, 0x0E, 0xFF, (u8)dig_rf_freq);
	SetIRVBit(IRV_RFTune, 0x0F, 0xFF, (u8)(dig_rf_freq >> 8));
	if (rf_freq_MHz < 444)
		SetIRVBit(IRV_RFTune, 0xAB, 0xFF, 0x70);
	else if (rf_freq_MHz < 528)
		SetIRVBit(IRV_RFTune, 0xAB, 0xFF, 0x30);
	else if (rf_freq_MHz < 667)
		SetIRVBit(IRV_RFTune, 0xAB, 0xFF, 0x20);
	else
		SetIRVBit(IRV_RFTune, 0xAB, 0xFF, 0x10);
	if (rf_freq_MHz <= 334)
		SetIRVBit(IRV_RFTune, 0x91, 0x40, 0x40);
	else
		SetIRVBit(IRV_RFTune, 0x91, 0x40, 0x00);
	lock_tuner(state);
	/*Generate one Array that Contain Data, Address */
	while (IRV_RFTune[Reg_Index].Num || IRV_RFTune[Reg_Index].Val)
	{
		ret = MxL_WriteReg(state, IRV_RFTune[Reg_Index].Num, IRV_RFTune[Reg_Index].Val);
		if (ret != 1)
		{
			dprintk("Mxl201 ES4 tuning failed\n");
			break;
		}
		/*
		 {
		 u8 val = MxL_ReadReg(state, IRV_RFTune[Reg_Index].Num);
		 dprintk("RFTune: %x: %x=%x\n", IRV_RFTune[Reg_Index].Num, IRV_RFTune[Reg_Index].Val, val);
		 }
		*/
		Reg_Index++;
	}
	msleep(3);
	unlock_tuner(state);
	return 0;
}

static void MxL201RF_ES4_Lock_Status(struct tda10023_state *state)
{
	u8 Data = 0;
	lock_tuner(state);
	Data = MxL_ReadReg(state, 0x14);
	unlock_tuner(state);
	dprintk("%s: RFSynth 0x%x, REFSynth 0x%x\n", __FUNCTION__, (Data & 0x0C), (Data & 0x03));
}

/* End MxL section */

static u8 tda10023_readreg(struct tda10023_state *state, u8 reg)
{
	u8 b0 [1] = { reg };
	u8 b1 [1] = { 0 };
	struct i2c_msg msg [] = { { .addr = state->config->demod_address, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = state->config->demod_address, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	};
	int ret;
	ret = i2c_transfer(state->i2c, msg, 2);
	if (ret != 2)
	{
		int num = state->frontend.dvb ? state->frontend.dvb->num : -1;
		printk(KERN_ERR "DVB: TDA10023(%d): %s: readreg error "
			   "(reg == 0x%02x, ret == %i)\n",
			   num, __func__, reg, ret);
	}
	return b1[0];
}

static int tda10023_writereg(struct tda10023_state *state, u8 reg, u8 data)
{
	u8 buf[2] = { reg, data };
	struct i2c_msg msg = { .addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2 };
	int ret;
	//printk("before: i2c_transfer, demod_address = %d\n", state->config->demod_address);
	//printk("before: i2c_transfer, state->i2c = %x\n", state->i2c);
	ret = i2c_transfer(state->i2c, &msg, 1);
	//printk("after: i2c_transfer\n");
	if (ret != 1)
	{
		int num = state->frontend.dvb ? state->frontend.dvb->num : -1;
		printk(KERN_ERR "DVB: TDA10023(%d): %s, writereg error "
			   "(reg == 0x%02x, val == 0x%02x, ret == %i)\n",
			   num, __func__, reg, data, ret);
	}
	return (ret != 1) ? -EREMOTEIO : 0;
}

static int tda10023_writebit(struct tda10023_state *state, u8 reg, u8 mask, u8 data)
{
	if (mask == 0xff)
		return tda10023_writereg(state, reg, data);
	else
	{
		u8 val;
		val = tda10023_readreg(state, reg);
		val &= ~mask; // force the bits which will be set to 0
		val |= (data & mask); // apply the new values
		return tda10023_writereg(state, reg, val);
	}
}

//get access to tuner
static int lock_tuner(struct tda10023_state *state)
{
	u8 buf[2] = { 0x0f, 0xc0 };
	struct i2c_msg msg = {.addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2};
	if (i2c_transfer(state->i2c, &msg, 1) != 1)
	{
		printk("tda10023: lock tuner fails\n");
		return -EREMOTEIO;
	}
	return 0;
}

//release access from tuner
static int unlock_tuner(struct tda10023_state *state)
{
	u8 buf[2] = { 0x0f, 0x40 };
	struct i2c_msg msg_post = {.addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2};
	if (i2c_transfer(state->i2c, &msg_post, 1) != 1)
	{
		printk("tda10023: unlock tuner fails\n");
		return -EREMOTEIO;
	}
	return 0;
}

static int tda10023_set_symbolrate(struct tda10023_state *state, u32 sr)
{
	s32 BDR;
	s32 BDRI;
	s16 SFIL = 0;
	u16 NDEC = 0;
	/* avoid floating point operations multiplying syscloc and divider
	 by 10 */
	u32 sysclk_x_10 = state->sysclk * 10;
	if (sr < (u32)(sysclk_x_10 / 984))
	{
		NDEC = 3;
		SFIL = 1;
	}
	else if (sr < (u32)(sysclk_x_10 / 640))
	{
		NDEC = 3;
		SFIL = 0;
	}
	else if (sr < (u32)(sysclk_x_10 / 492))
	{
		NDEC = 2;
		SFIL = 1;
	}
	else if (sr < (u32)(sysclk_x_10 / 320))
	{
		NDEC = 2;
		SFIL = 0;
	}
	else if (sr < (u32)(sysclk_x_10 / 246))
	{
		NDEC = 1;
		SFIL = 1;
	}
	else if (sr < (u32)(sysclk_x_10 / 160))
	{
		NDEC = 1;
		SFIL = 0;
	}
	else if (sr < (u32)(sysclk_x_10 / 123))
	{
		NDEC = 0;
		SFIL = 1;
	}
	BDRI = (state->sysclk) * 16;
	BDRI >>= NDEC;
	BDRI += sr / 2;
	BDRI /= sr;
	if (BDRI > 255)
		BDRI = 255;
	{
		u64 BDRX;
		BDRX = 1 << (24 + NDEC);
		BDRX *= sr;
		do_div(BDRX, state->sysclk); /* BDRX/=SYSCLK; */
		BDR = (s32)BDRX;
	}
	//dprintk("@@@@@@Symbolrate %d, BDR %d BDRI %d, NDEC %d\n", sr, BDR, BDRI, NDEC);
	tda10023_writebit(state, 0x03, 0xc0, NDEC << 6);
	tda10023_writereg(state, 0x0a, BDR & 255);
	tda10023_writereg(state, 0x0b, (BDR >> 8) & 255);
	tda10023_writereg(state, 0x0c, (BDR >> 16) & 31);
	tda10023_writereg(state, 0x0d, BDRI);
	tda10023_writereg(state, 0x3d, (SFIL << 7));
	return 0;
}

/**
 * function write_init_tda10023
*/
static void write_init_tda10023(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
#define TRUE 1
#define FALSE 0
#define TUNER_AGC_MAX 150
	struct tda10023_state *state = fe->demodulator_priv;
	u32 system_clk_local = state->sysclk;
	u32 tuner_if = 6000000;
	u32 uSACLK = 0;
	uint8_t puByte[3], uByte;
	long lDeltaF;
	uint8_t high_sampling = TRUE;
	//dprintk("DVB: TDA10023(): init chip, xtal %d, sys_clk_local %d\n", state->xtal, state->sysclk);
	uSACLK = state->sysclk;
// disable the PLL
	tda10023_writebit(state, 0x2A, 0x0F, 0x0F);
	msleep(100); //wait 100ms
// write the PLL registers with PLL bypassed
	uByte = (u32)state->pll_n;
	uByte |= (u32)(state->pll_p << 6);
	tda10023_writereg(state, 0x28, state->pll_m);
	tda10023_writereg(state, 0x29, uByte);
	//dprintk("0x28 => 0x%x; 0x29 => 0x%x\n", tda10023_readreg(state, 0x28), tda10023_readreg(state, 0x29));
// Enable FSAMPLING
	if (high_sampling == TRUE)
		tda10023_writebit(state, 0x00, 0x23, 0x23); //0x23 -> QAM256=0x33 (not 0x07)
	else
		tda10023_writebit(state, 0x00, 0x03, 0x03); //0x03 -> QAM256=0x13
// enable the PLL
	tda10023_writebit(state, 0x2a, 0x03, 0);
	msleep(100); //wait 100ms
// DVB mode
	tda10023_writebit(state, 0x1f, 0x80, 0);
// set the BER depth
	tda10023_writebit(state, 0xe6, 0x0c, 0x80);
// set GAIN1 bit to 0
	tda10023_writereg(state, 0x0e, 0x82);
// set the acquisition to +/-480ppm
	tda10023_writebit(state, 0x03, 0x08, 0x08);
// TRIAGC, POSAGC, enable AGCIF
	tda10023_writereg(state, 0x2e, 0x20); // POSAGC=1
// set the AGCREF
	tda10023_writereg(state, 0x01, 0x50);
// set SACLK_ON
	tda10023_writereg(state, 0x1e, 0xa3);
// program CS depending on SACLK and set GAINADC
	tda10023_writereg(state, 0x1b, 0xc8);
// set the polarity of the PWM for the AGC
	tda10023_writebit(state, 0x2e, 0x08, 0);
	tda10023_writebit(state, 0x2e, 0x02, 0);
// set the threshold for the IF AGC
	tda10023_writereg(state, 0x3b, 0xff);
	tda10023_writereg(state, 0x3c, 0xff);
// set the threshold for the TUN AGC
	tda10023_writereg(state, 0x35, 0xff);
	tda10023_writereg(state, 0x36, 0x00);
// configure the equalizer
// enable the equalizer and set the DFE bit
	uByte = 0x70 | 0x04 | 0x02 | 0x01;
	tda10023_writereg(state, 0x06, uByte); //lim_test
	tda10023_writebit(state, 0x1c, 0x30, 0x30);
// set ALGOD and deltaF
	if (high_sampling == TRUE)
	{
// FSAMPLING = 1 - high sampling clock
		lDeltaF = (uint32_t)(tuner_if / 1000);
		lDeltaF *= 32768; // 32768 = 2^20/32
		lDeltaF += (uint32_t)(system_clk_local / 500);
		lDeltaF /= (uint32_t)(system_clk_local / 1000);
		lDeltaF -= 53248; // 53248 = (2^20/32) * 13/8
	}
	else
	{
// FSAMPLING = 0 - low sampling clock
		lDeltaF = (uint32_t)(tuner_if / 1000);
		lDeltaF *= 32768; // 32768 = 2^20/32
		lDeltaF += (uint32_t)(system_clk_local / 1000);
		lDeltaF /= (uint32_t)(system_clk_local / 2000);
		lDeltaF -= 40960; // 53248 = (2^20/32) * 5/4
	}
	puByte[0] = (uint8_t)lDeltaF;
	puByte[1] = (uint8_t)(((lDeltaF >> 8) & 0x7F) | 0x80);
	tda10023_writereg(state, 0x37, puByte[0]);
	tda10023_writereg(state, 0x38, puByte[1]);
// set the KAGCIF and KAGCTUN to acquisition mode
	tda10023_writereg(state, 0x02, 0x93);
// set carrier algorithm parameters
	uByte = 0x82;
	uByte |= 1 << 4; // 1-> 2 06.12.18 carrier offset range extend
	uByte |= 1 << 2; // 1-> 2 06.12.18 swstep parameter change
	tda10023_writereg(state, 0x2D, uByte);
// set the MPEG output clock polarity
	tda10023_writebit(state, 0x12, 0x01, 0x01);
// TS interface 1
	uByte = 0;
// PARALLEL
// set to 1 MSB if parallel
	uByte |= 0x04;
// PARALLEL mode A
	uByte |= 0x00;
	tda10023_writereg(state, 0x20, uByte);
// disable the tri state of the outputs
	tda10023_writereg(state, 0x2c, 0);
}

static int tda10023_set_parameters(struct dvb_frontend *fe,
								   struct dvb_frontend_parameters *p)
{
	int qam;
	static u8 qamvals[6][6] =
	{
		// QAM LOCKTHR MSETH AREF AGCREFNYQ ERAGCNYQ_THD
		{ /*(5<<2)*/0x14, 0x78, 0x8c, 0x96, 0x78, 0x4c }, // 4 QAM
		{ /*(0<<2)*/0x00, 0x87, 0xa2, 0x91, 0x8c, 0x57 }, // 16 QAM
		{ /*(1<<2)*/0x04, 0x64, 0x74, 0x96, 0x8c, 0x57 }, // 32 QAM
		{ /*(2<<2)*/0x08, 0x46, 0x43, 0x6a, 0x6a, 0x44 }, // 64 QAM
		{ /*(3<<2)*/0x0C, 0x36, 0x34, 0x7e, 0x70, 0x4c }, // 128 QAM
		{ /*(4<<2)*/0x10, 0x26, 0x23, 0x6c, 0x5c, 0x3c }, // 256 QAM
	};
	struct tda10023_state *state = fe->demodulator_priv;
	if (state == NULL)
	{
		return 0;
	}
	qam = p->u.qam.modulation;
	if (qam < 0 || qam > 5)
		return -EINVAL;
	/* Set MxL201 params */
	MxL_Init(state);
	MxL201RF_ES4_RFTune(state, p);
	msleep(50);
	write_init_tda10023(fe, p);
	//printk("p->u.qam.symbol_rate === %d\n",p->u.qam.symbol_rate);
	tda10023_writebit(state, 0x04, 0x40, 0x40); // this is set_si! and correct, AUTOSI
	tda10023_writereg(state, 0x05, qamvals[qam][1]);
	tda10023_writereg(state, 0x08, qamvals[qam][2]);
	tda10023_writereg(state, 0x09, qamvals[qam][3]);
	tda10023_writereg(state, 0xb4, qamvals[qam][4]);
	tda10023_writereg(state, 0xb6, qamvals[qam][5]);
	tda10023_writebit(state, 0x00, 0x1c, qamvals[qam][0]);
	tda10023_set_symbolrate(state, p->u.qam.symbol_rate);
// Soft reset
	tda10023_writebit(state, 0x00, 0x03, 0x02);
	msleep(100);
	/* dprintk("%x=%x | %x=%x | %x=%x | %x=%x | %x=%x, ==> reg 0x0: 0x%x\n",
	 qamvals[qam][1], tda10023_readreg(state, 0x05),
	 qamvals[qam][2], tda10023_readreg(state, 0x08),
	 qamvals[qam][3], tda10023_readreg(state, 0x09),
	 qamvals[qam][4], tda10023_readreg(state, 0xb4),
	 qamvals[qam][5], tda10023_readreg(state, 0xb6),
	 tda10023_readreg(state,0x00));
	*/
	return 0;
}

static int tda10023_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8 ifgain = tda10023_readreg(state, 0x2f);
	u16 gain = ((255 - tda10023_readreg(state, 0x17))) + (255 - ifgain) / 16;
	// Max raw value is about 0xb0 -> Normalize to >0xf0 after 0x90
	if (gain > 0x90)
		gain = gain + 2 * (gain - 0x90);
	if (gain > 255)
		gain = 255;
	*strength = (gain << 8) | gain;
	return 0;
}

static int tda10023_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8 a, b, c;
	a = tda10023_readreg(state, 0x14);
	b = tda10023_readreg(state, 0x15);
	c = tda10023_readreg(state, 0x16) & 0xf;
	tda10023_writebit(state, 0x10, 0xc0, 0x00);
	*ber = a | (b << 8) | (c << 16);
	return 0;
}

static int tda10023_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8 sync = 0;
	*status = 0;
	//0x11[1] == CARLOCK -> Carrier locked
	//0x11[2] == FSYNC -> Frame synchronisation
	//0x11[3] == FEL -> Front End locked
	//0x11[6] == NODVB -> DVB Mode Information
	sync = tda10023_readreg(state, 0x11);
	//printk(">>> HAS_SIGNAL %d, HAS_SYNC %d, HAS_LOCK %d\n", (sync & 2), (sync & 4), (sync & 8));
	if (sync & 2)
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER;
	if (sync & 4)
		*status |= FE_HAS_SYNC | FE_HAS_VITERBI;
	if (sync & 8)
		*status |= FE_HAS_LOCK;
	return 0;
}

static int tda10023_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8 quality = ~tda10023_readreg(state, 0x18);
	*snr = (quality << 8) | quality;
	return 0;
}

static int tda10023_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8 a, b, c, d;
	a = tda10023_readreg(state, 0x74);
	b = tda10023_readreg(state, 0x75);
	c = tda10023_readreg(state, 0x76);
	d = tda10023_readreg(state, 0x77);
	*ucblocks = a | (b << 8) | (c << 16) | (d << 24);
	tda10023_writebit(state, 0x10, 0x20, 0x00);
	tda10023_writebit(state, 0x10, 0x20, 0x20);
	tda10023_writebit(state, 0x13, 0x01, 0x00);
	return 0;
}

static int tda10023_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct tda10023_state *state = fe->demodulator_priv;
	int sync, inv;
	s8 afc = 0;
	sync = tda10023_readreg(state, 0x11);
	afc = tda10023_readreg(state, 0x19);
	inv = tda10023_readreg(state, 0x04);
	if (verbose)
	{
		/* AFC only valid when carrier has been recovered */
		printk(sync & 2 ? "DVB: TDA10023(%d): AFC (%d) %dHz\n" :
			   "DVB: TDA10023(%d): [AFC (%d) %dHz]\n",
			   state->frontend.dvb->num, afc,
			   -((s32)p->u.qam.symbol_rate * afc) >> 10);
	}
	p->inversion = (inv & 0x20 ? 0 : 1);
	p->u.qam.modulation = ((state->reg0 >> 2) & 7) + QAM_16;
	p->u.qam.fec_inner = FEC_NONE;
	p->frequency = ((p->frequency + 31250) / 62500) * 62500;
	if (sync & 2)
		p->frequency -= ((s32)p->u.qam.symbol_rate * afc) >> 10;
	return 0;
}

static int tda10023_sleep(struct dvb_frontend *fe)
{
#if 0
	struct tda10023_state *state = fe->demodulator_priv;
	tda10023_writereg(state, 0x1b, 0x02); /* pdown ADC */
	tda10023_writereg(state, 0x00, 0x80); /* standby */
#endif
	return 0;
}

static int tda10023_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct tda10023_state *state = fe->demodulator_priv;
	if (enable)
	{
		lock_tuner(state);
	}
	else
	{
		unlock_tuner(state);
	}
	return 0;
}

static void tda10023_release(struct dvb_frontend *fe)
{
	struct tda10023_state *state = fe->demodulator_priv;
	kfree(state);
}

static int tda10023_set_property(struct dvb_frontend *fe,
								 struct dtv_property *tvp)
{
	return 0;
}

static int tda10023_get_property(struct dvb_frontend *fe,
								 struct dtv_property *tvp)
{
	dprintk("%s(..)\n", __func__);
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBC_ANNEX_AC:
				break;
			default:
				return -EINVAL;
		}
	}
	return 0;
}

static struct dvb_frontend_ops tda10023_ops;

struct dvb_frontend *tda10023_attach(const struct tda10023_config *config,
									 struct i2c_adapter *i2c,
									 u8 pwm)
{
	struct tda10023_state *state = NULL;
	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct tda10023_state), GFP_KERNEL);
	if (state == NULL) goto error;
	dprintk("DVB: state 0x%x\n", (unsigned int)state);
	/* setup the state */
	state->config = config;
	state->i2c = i2c;
	dprintk("DVB: tda10023_writereg\n");
	/* wakeup if in standby */
	tda10023_writereg(state, 0x00, 0x33);
	dprintk("DVB: tda10023_readreg\n");
	/* check if the demod is there */
	if ((tda10023_readreg(state, 0x1a) & 0xf0) != 0x70) goto error;
	dprintk("DVB: state 0x%x\n", (unsigned int)state);
	/* create dvb_frontend */
	memcpy(&state->frontend.ops, &tda10023_ops, sizeof(struct dvb_frontend_ops));
	state->pwm = pwm;
	state->reg0 = REG0_INIT_VAL;
	/*if (state->config->xtal) {
	 state->xtal = state->config->xtal;
	 state->pll_m = state->config->pll_m;
	 state->pll_p = state->config->pll_p;
	 state->pll_n = state->config->pll_n;
	} else {*/
	/* set default values if not defined in config */
	state->xtal = 16000000;
	state->pll_m = 0x0b;
	state->pll_p = 0x02;
	state->pll_n = 0x00;
	//}
	/* calc sysclk */
	state->sysclk = (state->xtal * (state->pll_m + 1) / \
					 (state->pll_n + 1 * state->pll_p + 1));
	state->frontend.ops.info.symbol_rate_min = (state->sysclk / 2) / 64;
	state->frontend.ops.info.symbol_rate_max = (state->sysclk / 2) / 4;
	//dprintk("DVB: TDA10023 %s: xtal:%d pll_m:%d pll_p:%d pll_n:%d\n",
	// __func__, state->xtal, state->pll_m, state->pll_p,
	// state->pll_n);
	state->frontend.demodulator_priv = state;
	return &state->frontend;
error:
	kfree(state);
	return NULL;
}

static struct dvb_frontend_ops tda10023_ops =
{

	.info = {
		.name = "NXP TDA10023 DVB-C",
		.type = FE_QAM,
		.frequency_stepsize = 62500,
		.frequency_min = 47000000,
		.frequency_max = 862000000,
		.symbol_rate_min = 0, /* set in tda10023_attach */
		.symbol_rate_max = 0, /* set in tda10023_attach */
		.caps = 0x400 | //FE_CAN_QAM_4
		FE_CAN_QAM_16 | FE_CAN_QAM_32 | FE_CAN_QAM_64 |
		FE_CAN_QAM_128 | FE_CAN_QAM_256 |
		FE_CAN_FEC_AUTO
	},

	.release = tda10023_release,

	.sleep = tda10023_sleep,
	.i2c_gate_ctrl = tda10023_i2c_gate_ctrl,

	.set_frontend = tda10023_set_parameters,
	.get_frontend = tda10023_get_frontend,

	.read_status = tda10023_read_status,
	.read_ber = tda10023_read_ber,
	.read_signal_strength = tda10023_read_signal_strength,
	.read_snr = tda10023_read_snr,
	.read_ucblocks = tda10023_read_ucblocks,

	.set_property = tda10023_set_property,
	.get_property = tda10023_get_property,
};

EXPORT_SYMBOL(tda10023_attach);
