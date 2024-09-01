/*
  TDA10023  - DVB-C decoder
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

#include <linux/dvb/version.h>

#include "dvb_frontend.h"

#include "tda1002x.h"

extern short paramDebug;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[tda10023] "

#define dprintk(level, x...) do \
{ \
	if ((paramDebug) && (paramDebug >= level) || level == 0) \
	{ \
		printk(TAGDEBUG x); \
	} \
} while (0)

#define REG0_INIT_VAL 0x23

struct tda10023_state
{
	struct i2c_adapter* i2c;
	/* configuration settings */
	const struct tda10023_config *config;
	struct dvb_frontend frontend;

	u8  pwm;
	u8  reg0;

	/* clock settings */
	u32 xtal;
	u8  pll_m;
	u8  pll_p;
	u8  pll_n;
	u32 sysclk;
};

static int verbose = 1;

static int tda10023_read_status(struct dvb_frontend *fe, fe_status_t *status);

static u8 tda10023_readreg(struct tda10023_state *state, u8 reg)
{
	u8 b0 [] = { reg };
	u8 b1 [] = { 0 };
	struct i2c_msg msg [] =
	{
		{ .addr = state->config->demod_address, .flags = 0,        .buf = b0, .len = 1 },
		{ .addr = state->config->demod_address, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	};
	int ret;

	dprintk(20, "%s config->address = 0x%x \n",__func__, state->config->demod_address);

	ret = i2c_transfer (state->i2c, msg, 2);
	if (ret != 2)
	{
		int num = state->frontend.dvb ? state->frontend.dvb->num : -1;
		dprintk(1, "%s: TDA10023(%d) readreg error (reg == 0x%02x, ret == %i)\n", __func__, num, reg, ret);
	}
	dprintk(20, "%s: reg=0x%02x , result=0x%02x\n",__func__, reg, b1[0]);
	return b1[0];
}

static int tda10023_writereg(struct tda10023_state *state, u8 reg, u8 data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2 };
	int ret;

	dprintk(20, "addr = 0x%02x : reg = 0x%02x val = 0x%02x\n", state->config->demod_address, reg, data);

	ret = i2c_transfer (state->i2c, &msg, 1);
	if (ret != 1)
	{
		int num = state->frontend.dvb ? state->frontend.dvb->num : -1;
		dprintk(1, "%s: TDA10023(%d) writereg error (reg == 0x%02x, val == 0x%02x, ret == %i)\n", __func__, num, reg, data, ret);
	}
	return (ret != 1) ? -EREMOTEIO : 0;
}

static int tda10023_writebit(struct tda10023_state *state, u8 reg, u8 mask,u8 data)
{
	if (mask == 0xff)
	{
		return tda10023_writereg(state, reg, data);
	}
	else
	{
		u8 val;

		val = tda10023_readreg(state,reg);
		val &= ~mask;
		val |= (data & mask);
		return tda10023_writereg(state, reg, val);
	}
}

static void tda10023_writetab(struct tda10023_state* state, u8* tab)
{
	u8 r, m, v;

	while (1)
	{
		r = *tab++;
		m = *tab++;
		v = *tab++;
		if (r == 0xff)
		{
			if (m == 0xff)
			{
				break;
			}
			else
			{
				msleep(m);
			}
		}
		else
		{
			tda10023_writebit(state, r, m, v);
		}
	}
}

// get access to tuner
static int lock_tuner(struct tda10023_state *state)
{
	u8 buf[2] = { 0x0f, 0xc0 };
	struct i2c_msg msg = {.addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2};

	if (i2c_transfer(state->i2c, &msg, 1) != 1)
	{
		dprintk(1, "%s: Access lock tuner failed\n", __func__);
		return -EREMOTEIO;
	}
	return 0;
}

// release access from tuner
static int unlock_tuner(struct tda10023_state *state)
{
	u8 buf[2] = { 0x0f, 0x40 };
	struct i2c_msg msg_post = {.addr = state->config->demod_address, .flags = 0, .buf = buf, .len = 2};

	if (i2c_transfer(state->i2c, &msg_post, 1) != 1)
	{
		dprintk(1, "%s: Access unlock tuner failed\n", __func__);
		return -EREMOTEIO;
	}
	return 0;
}

/* ****************************** tuner/pll functions ****************************** */
/* fixme: move those functions to other module, or better use orign tda665x driver   */

static int tda10023_tuner_write(struct tda10023_state *state, u8 addr, char *buff, int size)
{
	int ret = 0;
	struct i2c_msg msg = { .addr = addr, .buf =buff, .len = size };

/* fixme: use gate on here */
	if (tda10023_writebit(state, 0x0f, 0x80, 0x80) < 0)  // gate on
	{
		dprintk(1, "%s: Error opening i2c gate\n", __func__);
		return -1;
	}
	if (paramDebug >= 100)
	{
		int i;

		for (i = 0; i < size; i++)
		{
			printk(" 0x%02x", buff[i]);
		}
		printk("\n");
	}
	msleep(50);  // wait 50ms

	if (i2c_transfer (state->i2c, &msg, 1) != 1)
	{
		dprintk(1, "%s: failed to write to pll on 0x%0x\n", __func__, addr);
	}
	msleep(50);  // wait 50ms
/* fixme: use gate off here */
	if (tda10023_writebit(state, 0x0f, 0x80, 0x00) < 0)
	{
		dprintk(1, "%s: error closing i2c gate\n", __func__);
		return -1;
	}
	return ret;
}

static int tda10023_tuner_read(struct tda10023_state* state,  u8 addr,char *buff)
{
	struct i2c_msg msg = { .addr = addr, .flags = I2C_M_RD, .buf =buff, .len = 1 };

	if (tda10023_writebit(state, 0x0f, 0x80, 0x80) < 0)
	{
		return -1;
	}
	msleep(50);  // wait 50ms

	if (i2c_transfer(state->i2c, &msg, 1) < 0)
	{
		return -1;
	}
	msleep(50);  // wait 50ms

	if (tda10023_writebit(state, 0x0f, 0x80, 0x00) < 0)
	{
		return -1;
	}
	return 0;
}

//got from STV297 Step size 166,67 if 36166
static int tda10023_pll_set_freq(struct tda10023_state *state, unsigned int freq_khz)
{
	//unsigned int div = (freq_khz + 36166 + 83) * 24 / 4 / 1000;
	unsigned int div = ((freq_khz * 10) + 361250) / 625;  // IF 36,125 step 62,5

	int a;
	unsigned char buf[5];
	const int Vhflowend = 160000;
	const int Vhfhighend = 445000;
	unsigned char cp = 0, bs = 0;
	const struct
	{
		int min, max;
		unsigned char cp, bs;
	} cptlb[] =
	{
		/* VHF-L */
		{  51000, 100000, 2 << 5, 1 },
		{ 100000, 122000, 3 << 5, 1 },
		{ 122000, 129000, 4 << 5, 1 },
		{ 129000, 136000, 5 << 5, 1 },
		{ 136000, 143000, 6 << 5, 1 },

		/* VHF-H */
		{ 150000, 240000, 1 << 5, 2 },
		{ 240000, 310000, 2 << 5, 2 },
		{ 310000, 380000, 3 << 5, 2 },
		{ 380000, 434000, 4 << 5, 2 },

		/* UHF */
		{ 434000, 578000, 4 << 5, 4 },
		{ 578000, 650000, 5 << 5, 4 },
		{ 650000, 746000, 6 << 5, 4 },
		{ 746000, 858000, 7 << 5, 4 },
	};

	if (freq_khz == 658000)
	{
		div += 2;
	}
	else
	{
		div += 1;
	}
	/* get charge pump */
#if 0
	for (a = 0; a < (sizeof(cptlb) / sizeof(cptlb[0])); a++)
	{
		if (cptlb[a].min <= freq_khz && freq_khz < cptlb[a].max)
		{
			cp = cptlb[a].cp;
			bs = cptlb[a].bs;
			break;
		}
	}
	if (a == (sizeof(cptlb)/sizeof(cptlb[0])))
	{
		dprintk(1, "unknown frequency.(%d)\n", freq_khz);
		return -1;
	}
#endif
	if (freq_khz < Vhflowend)
	{
		buf[3] = 0x81;
	}
	else if (freq_khz > Vhfhighend)
	{
		if (freq_khz == 658000)
		{
			buf[3] = 0x24;
		}
		else
		{
			buf[3] = 0x84;
		}
	}
	else
	{
		buf[3] = 0x82;
	}
	buf[0] = (div >> 8) & 0x7f;
	buf[1] = div & 0xff;
	buf[2] = 0xc0; //| 0x08 ;//| 0x02;//step 62.5
	//buf[3] = cp | bs;
	buf[4] = 0x80 | 0x00 | 0x02;

	if (tda10023_tuner_write(state,state->config->tuner_address,buf, 5) < 0)
	{
		dprintk(1, "%s: tda10023_tuner_write failed\n", __func__);
		return -1;
	}
	return 0;
}

static int tda10023_reset_all(struct tda10023_state *state)
{
	// reset the chip
	tda10023_writebit(state, 0x00, 0x02 | 0x01, 0x02 | 0);
	return 0;
}

static int tda10023_write_init(struct tda10023_state* state, unsigned int f_RF_freq)
{
	unsigned char p_byte[3], byte;
	unsigned int sampling_clk;
	long delta_freq;
	const unsigned int tuner_IF = 36125000;  /** lock??? ?? freq */
	int b_high_sampling;

	//dprintk(100, "%s >\n", __func__);

	if (f_RF_freq == 282000)
	{
		b_high_sampling = 1;
	}
	else
	{
		b_high_sampling = 0;
	}
	// Calculate the sampling clock
	if (b_high_sampling == 1)
	{
		sampling_clk = state->sysclk;// high sampling clock
	}
	else
	{
		sampling_clk = state->sysclk / 2;// low sampling clock
	}
	// disable the PLL
	tda10023_writebit(state, 0x2A, 0x02 | 0x01, 0x02 | 0x01);
	//crystal?? ??? clock? ?? bypass??. pll? power? down??.

	msleep(100);  //wait 100ms

	// write the PLL registers with PLL bypassed
	tda10023_writereg(state, 0x28, 0x0b);
	tda10023_writereg(state, 0x29, 0x42);  // 28.92MHz

	// Set FSAMPLING
	if (f_RF_freq == 282000)
	{
		b_high_sampling = 1;
	}
	else
	{
		b_high_sampling = 0;
	}
	if (b_high_sampling == 1)
	{
		tda10023_writebit(state, 0x00, 0x20 | 0x02 | 0x01, 0x20 | 0x02 | 0x01);
	}
	else
	{
		tda10023_writebit(state, 0x00, 0x20 | 0x02 | 0x01, 0 | 0x02 | 0x01);
	}
	// enable the PLL
	tda10023_writebit(state, 0x2a, 0x02 | 0x01, 0);
	msleep(100);  //wait 100ms

	// DVB mode
	tda10023_writebit(state, 0x1f, 0x80, 0);
	// set the BER depth
	tda10023_writebit(state, 0xe6, 0x0c, 2 << 6);

	// set the acquisition to +/-480ppm
	tda10023_writebit(state, 0x03, 0x08, 0x08);

	// TRIAGC, POSAGC, enable AGCIF
	tda10023_writebit(state, 0x2e, 0x80 | 0x20 |0x10, 0 | 0x20 | 0x10);

	// set the AGCREF
	tda10023_writereg(state, 0x01, 0x35); //0x48  //0x50

	// set SACLK_OFF
	// Remove SACLK since it is not used and generates interferences
	tda10023_writebit(state, 0x1e, 0x80, 0x80);

	// program CS depending on SACLK and set GAINADC
	tda10023_writereg(state, 0x1b,0xc8);

	// set the polarity of the PWM for the AGC
	tda10023_writebit(state, 0x2e, 0x08, 0);
	tda10023_writebit(state, 0x2e, 0x02, 0);

	// set the threshold for the IF AGC
	tda10023_writereg(state, 0x3b, 0);
	tda10023_writereg(state, 0x3c, 0);

	// set the threshold for the TUN AGC
	tda10023_writereg(state, 0x35, 150);  //AGCTUNER MAX
	tda10023_writereg(state, 0x36, 60);  //AGCTUNER MIN

	// configure the equalizer
	// enable the equalizer and set the DFE bit
	byte = 0x70 | 0x04 | 0x02 | 1;  //DFE
	tda10023_writereg(state, 0x06, byte);  //lim_test

	tda10023_writebit(state, 0x1c, 0x20 | 0x10, 0x20 | 0x10);

	// set ALGOD and deltaF
	if (b_high_sampling == 1)
	{
		// FSAMPLING = 1 - high sampling clock
		// SACLK = Sysclk  (SACLK max = 72MHz)
		delta_freq  = (unsigned int)(tuner_IF/1000);
		delta_freq *= 32768;  // 32768 = 2^20/32
		delta_freq += (unsigned int)(state->sysclk / 500);
		delta_freq /= (unsigned int)(state->sysclk / 1000);
		delta_freq -= 53248;  // 53248 = (2^20/32) * 13/8
	}
	else
	{
		// FSAMPLING = 0 - low sampling clock
		// SACLK = Sysclk/2 (SACLK max = 36MHz)
		delta_freq  = (unsigned int)(tuner_IF / 1000);
		delta_freq *= 32768;  // 32768 = 2^20/32
		delta_freq += (unsigned int)(state->sysclk / 1000);
		delta_freq /= (unsigned int)(state->sysclk / 2000);
		delta_freq -= 40960;  // 40960 = (2^20/32) * 5/4
	}
	p_byte[0] = (unsigned char)delta_freq;
	p_byte[1] = (unsigned char)(((delta_freq >> 8) & 0x7F) | 0x80);
	tda10023_writereg(state, 0x37, p_byte[0]);
	tda10023_writereg(state, 0x38, p_byte[1]);

	// set the KAGCIF and KAGCTUN to acquisition mode
	tda10023_writereg(state, 0x02, 0x93);

	// set carrier algorithm parameters
	byte  = 0x82;
	byte |= 2 << 4; // 1-> 2 06.12.18 carrier offset range extend
	byte |= 2 << 2; // 1-> 2 06.12.18 swstep parameter change

	tda10023_writereg(state, 0x2D, byte);

	// set the MPEG output clock polarity
	tda10023_writebit(state, 0x12, 0x01, 0x01);
	tda10023_writereg(state, 0x12, 0xa1);  // khg_temp_chindao
	tda10023_writebit(state, 0x2b, 0x01, 0x01);

	// TS interface 1
	byte = 0;
	// PARALLEL
	// set to 1 MSB if parallel
	byte |= 0x04;

	// PARALLEL mode A
	byte |= 0x00;

	tda10023_writereg(state, 0x20, byte);

	// disable the tri state of the outputs
	tda10023_writebit(state, 0x2c, 0x02 | 0x01, 0);

	// Soft reset
	tda10023_reset_all(state);

	// set the KAGCIF and KAGCTUN to acquisition mode
	tda10023_writereg(state, 0x02, 0x93);

	msleep(100);  //wait 100ms
	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ****************************** end tuner/pll functions ****************************** */

static int tda10023_setup_reg0(struct tda10023_state * state, u8 reg0)
{
	reg0 |= state->reg0 & 0x63;

	tda10023_writereg(state, 0x00, reg0 & 0xfe);
	tda10023_writereg(state, 0x00, reg0 | 0x01);

	state->reg0 = reg0;
	return 0;
}

static int tda10023_set_symbolrate(struct tda10023_state* state, u32 sr)
{
	s32 BDR;
	s32 BDRI;
	s16 SFIL = 0;
	u16 NDEC = 0;

	u32 sysclk_x_10;
	dprintk(100, "%s > sr = %d\n", __func__, sr);

	/* avoid floating point operations multiplying syscloc and divider
	   by 10 */
	sysclk_x_10 = state->sysclk * 10;

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
	{
		BDRI = 255;
	}
	{
		u64 BDRX;

		BDRX = 1 << (24 + NDEC);
		BDRX *= sr;
		do_div(BDRX, state->sysclk);  /* BDRX/=SYSCLK; */

		BDR=(s32)BDRX;
	}
	dprintk(20, "Symbolrate %i, BDR %i BDRI %i, NDEC %i\n", sr, BDR, BDRI, NDEC);
/* see below
	tda10023_writebit(state, 0x03, 0xc0, NDEC << 6);
	tda10023_writereg(state, 0x0a, BDR & 255);
	tda10023_writereg(state, 0x0b, (BDR >> 8) & 255);
	tda10023_writereg(state, 0x0c, (BDR >> 16) & 31);
	tda10023_writereg(state, 0x0d, BDRI);
	tda10023_writereg(state, 0x3d, (SFIL << 7));
*/
/* fixme: orig app path */
	tda10023_writebit(state, 0x3d, 0x80, (SFIL << 7));
	tda10023_writebit(state, 0x03, 0xc0, NDEC << 6);
	tda10023_writereg(state, 0x0a, BDR & 255);
	tda10023_writereg(state, 0x0b, (BDR >> 8) & 255);
	tda10023_writereg(state, 0x0c, (BDR >> 16) & 31);
	tda10023_writereg(state, 0x0d, BDRI);
/* *** orig app path */

	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int tda10023_init (struct dvb_frontend *fe)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8 tda10023_inittab[] =
	{
		/*        reg  mask val */
		/* 000 */ 0x2a, 0xff, 0x02,  /* PLL3, Bypass, Power Down */
		/* 003 */ 0xff, 0x64, 0x00,  /* Sleep 100ms */
		/* 006 */ 0x2a, 0xff, 0x03,  /* PLL3, Bypass, Power Down */
		/* 009 */ 0xff, 0x64, 0x00,  /* Sleep 100ms */
				/* PLL1 */
		/* 012 */ 0x28, 0xff, (state->pll_m-1),
				/* PLL2 */
		/* 015 */ 0x29, 0xff, ((state->pll_p-1)<<6)|(state->pll_n-1),
				/* GPR FSAMPLING=1 */
		/* 018 */ 0x00, 0xff, REG0_INIT_VAL,
		/* 021 */ 0x2a, 0x03, 0x00,  /* PLL3 PSACLK=1 */
		/* 024 */ 0xff, 0x64, 0x00,  /* Sleep 100ms */
		/* 027 */ 0x1f, 0x80, 0x00,  /* RESET */
		/* 030 */ 0xff, 0x64, 0x00,  /* Sleep 100ms */
		/* 033 */ 0xe6, 0x0c, 0x80,  //0x04,  /* RSCFG_IND */
		/* 036 */ 0x10, 0xc0, 0x80,  /* DECDVBCFG1 PBER=1 */
		
		/* 039 */ 0x0e, 0xff, 0x82,  /* GAIN1 */
		/* 042 */ 0x03, 0x08, 0x08,  /* CLKCONF DYN=1 */
		/* 045 */ 0x2e, 0xb0, 0x30,  /* AGCCONF2 TRIAGC=0,POSAGC=ENAGCIF=1
				PPWMTUN=0 PPWMIF=0 */
		/* 048 */ 0x01, 0xff, 0x35,  /* AGCREF */
		/* 051 */ 0x1e, 0x80, 0x80,  /* CONTROL SACLK_ON=1 */
		/* 054 */ 0x1b, 0xff, 0xc8,  /* ADC TWOS=1 */
		/* 057 */ 0x3b, 0xff, 0x00,  /* IFMAX */
		/* 060 */ 0x3c, 0xff, 0x00,  /* IFMIN */
		/* 063 */ 0x34, 0xff, 0x00,  /* PWMREF */
		/* 066 */ 0x35, 0xff, 150,   /* TUNMAX */
		/* 069 */ 0x36, 0xff, 60,    /* TUNMIN */
		/* 072 */ 0x06, 0xff, 0x77,  /* EQCONF1 POSI=7 ENADAPT=ENEQUAL=DFE=1 */
		/* 075 */ 0x1c, 0x30, 0x30,  /* EQCONF2 STEPALGO=SGNALGO=1 */
		/* 078 */ 0x37, 0xff, 0xf6,  /* DELTAF_LSB */
		/* 081 */ 0x38, 0xff, 0xff,  /* DELTAF_MSB */
		/* 084 */ 0x02, 0xff, 0x93,  /* AGCCONF1  IFS=1 KAGCIF=2 KAGCTUN=3 */
		/* 087 */ 0x2d, 0xff, 0xfa,  /* SWEEP SWPOS=1 SWDYN=7 SWSTEP=1 SWLEN=2 */
		/* 090 */ 0x04, 0x10, 0x00,  /* SWRAMP=1 */
		/* 093 */ 0x12, 0xff, TDA10023_OUTPUT_MODE_PARALLEL_B, /*
				INTP1 POCLKP=1 FEL=1 MFS=0 */
		/* 096 */ 0x2b, 0x01, 0x01,  /* INTS1 */
		/* 099 */ 0x20, 0xff, 0x04,  /* INTP2 SWAPP=? MSBFIRSTP=? INTPSEL=? */
		/* 102 */ 0x2c, 0x03, 0x00,  /* INTP/S TRIP=0 TRIS=0 */
		/* 105 */ 0xc4, 0xff, 0x00,
		/* 108 */ 0xc3, 0x30, 0x00,
		/* 111 */ 0xb5, 0xff, 0x19,  /* ERAGC_THD */
		/* 114 */ 0x00, 0x03, 0x01,  /* GPR, CLBS soft reset */
		/* 117 */ 0x00, 0x03, 0x03,  /* GPR, CLBS soft reset */
		/* 120 */ 0xff, 0x64, 0x00,  /* Sleep 100ms */
		/* 123 */ 0xff, 0xff, 0xff
	};
	dprintk(100, "%s > TDA10023(%d)\n", __func__, fe->dvb->num);

	/* override default values if set in config */
	if (state->config->deltaf)
	{
		tda10023_inittab[80] = (state->config->deltaf & 0xff);
		tda10023_inittab[83] = (state->config->deltaf >> 8);
	}
	if (state->config->output_mode)
	{
		tda10023_inittab[95] = state->config->output_mode;
	}
	tda10023_writetab(state, tda10023_inittab);
	return 0;
}

static int tda10023_set_parameters (struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct tda10023_state* state = fe->demodulator_priv;
	int cnt;

	static int qamvals[6][6] =
	{
		//  QAM      LOCKTHR  MSETH   AREF AGCREFNYQ  ERAGCNYQ_THD
		{ (5 << 2),  0x78,    0x8c,   0x96,   0x78,   0x4c  },  // 4 QAM
		{ (0 << 2),  0x87,    0xa2,   0x91,   0x8c,   0x57  },  // 16 QAM
		{ (1 << 2),  0x64,    0x74,   0x96,   0x8c,   0x57  },  // 32 QAM
		{ (2 << 2),  0x46,    0x43,   0x6a,   0x6a,   0x44  },  // 64 QAM
		{ (3 << 2),  0x36,    0x34,   0x7e,   0x78,   0x4c  },  // 128 QAM
		{ (4 << 2),  0x26,    0x23,   0x6c,   0x5c,   0x3c  },  // 256 QAM
	};
	int qam = p->u.qam.modulation;

	dprintk(20, "%s > qam %d, sr %d, inv %d\n", __func__, qam, p->u.qam.symbol_rate, p->inversion);

	/* fixme: orig app path */
	tda10023_reset_all(state);
	tda10023_write_init(state, p->frequency / 1000);
	/* *** orig app path */

	if (qam < 0 || qam > 5)
	{
		return -EINVAL;
	}
	if (fe->ops.tuner_ops.set_params)
	{
		fe->ops.tuner_ops.set_params(fe, p);
		if (fe->ops.i2c_gate_ctrl)
		{
			fe->ops.i2c_gate_ctrl(fe, 0);
		}
	}
	else if (fe->ops.tuner_ops.set_state)
	{
		struct tuner_state tstate;

		tstate.frequency = p->frequency;

		if (fe->ops.i2c_gate_ctrl)
		{
			fe->ops.i2c_gate_ctrl(fe, 1);
		}
		fe->ops.tuner_ops.set_state(fe, DVBFE_TUNER_FREQUENCY, &tstate);
		
		if (fe->ops.i2c_gate_ctrl)
		{
			fe->ops.i2c_gate_ctrl(fe, 0);
		}
	}
	else
	{
		/* fixme: orig app path */
		if (tda10023_pll_set_freq(state, p->frequency / 1000) < 0)
		{
			dprintk(1, "%s: pll_set_freq failed\n", __func__);
		}
#if 0
		if (tda10023_pll_set_freq(state,p->frequency / 1000) < 0)
		{
			dprintk(1, "%s: pll_set_freq failed\n", __func__);
		}
		if (tda10023_pll_set_freq(state, p->frequency / 1000) < 0)
		{
			dprintk(1, "%s: pll_set_freq failed\n", __func__);
		}
#endif
		/* *** orig app path */

	}
	/* fixme: orig app path */
	tda10023_reset_all(state);
	/* *** orig app path */

	tda10023_set_symbolrate(state, p->u.qam.symbol_rate);
	tda10023_writereg(state, 0x05, qamvals[qam][1]);
	tda10023_writereg(state, 0x08, qamvals[qam][2]);
	tda10023_writereg(state, 0x09, qamvals[qam][3]);
	tda10023_writereg(state, 0xb4, qamvals[qam][4]);
	tda10023_writereg(state, 0xb6, qamvals[qam][5]);

//	tda10023_writereg(state, 0x04, (p->inversion ? 0x12 : 0x32));
//	tda10023_writebit(state, 0x04, 0x60, (p->inversion ? 0 : 0x20));
	tda10023_writebit(state, 0x04, 0x40, 0x40);

	/* fixme: orig app path
	tda10023_setup_reg0(state, qamvals[qam][0]);
	*/
	tda10023_writebit(state, 0x00, 0x1c, qamvals[qam][0]);
	/* *** orig app path */

	cnt = 10; /* fixme think on this */
	do
	{
		fe_status_t status = 0;

		tda10023_read_status(fe, &status);

		if (status & FE_HAS_LOCK)
		{
			break;
		}
		msleep(20);  /* fixme: think on this */
		dprintk(20, "%s: Waiting for lock %d...\n", __func__, cnt);
	} while (--cnt);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int tda10023_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct tda10023_state *state = fe->demodulator_priv;
	int sync;

	dprintk(100, "%s >\n", __func__);

	*status = 0;

	//0x11[1] == CARLOCK -> Carrier locked
	//0x11[2] == FSYNC -> Frame synchronisation
	//0x11[3] == FEL -> Front End locked
	//0x11[6] == NODVB -> DVB Mode Information
	sync = tda10023_readreg (state, 0x11);

	if (sync & 2)
	{
		*status |= FE_HAS_SIGNAL|FE_HAS_CARRIER;
	}
	if (sync & 4)
	{
		*status |= FE_HAS_SYNC|FE_HAS_VITERBI;
	}
	if (sync & 8)
	{
		*status |= FE_HAS_LOCK;
	}
	dprintk(100, "%s < status %d\n", __func__, *status);
	return 0;
}

static int tda10023_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct tda10023_state* state = fe->demodulator_priv;
	u8 a, b, c;

	a = tda10023_readreg(state, 0x14);
	b = tda10023_readreg(state, 0x15);
	c = tda10023_readreg(state, 0x16) & 0xf;
	tda10023_writebit(state, 0x10, 0xc0, 0x00);

	*ber = a | (b << 8) | (c << 16);
	return 0;
}

static int tda10023_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct tda10023_state* state = fe->demodulator_priv;
	u8 ifgain = tda10023_readreg(state, 0x2f);
	u16 gain = ((255 - tda10023_readreg(state, 0x17))) + (255 - ifgain) / 16;

	dprintk(100, "%s >\n", __func__);

	// Max raw value is about 0xb0 -> Normalize to >0xf0 after 0x90
	if (gain > 0x90)
	{
		gain = gain + 2 * (gain - 0x90);
	}
	if (gain > 255)
	{
		gain = 255;
	}
	*strength = (gain << 8) | gain;

	dprintk(100, "%s < strength %d\n", __func__, *strength);
	return 0;
}

static int tda10023_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8                    quality;

	dprintk(100, "%s >\n", __func__);
	quality = ~tda10023_readreg(state, 0x18);
	*snr = (quality << 8) | quality;

	dprintk(100, "%s < snr %d\n", __func__, *snr);
	return 0;
}

static int tda10023_read_ucblocks(struct dvb_frontend *fe, u32* ucblocks)
{
	struct tda10023_state *state = fe->demodulator_priv;
	u8 a, b, c, d;

	a = tda10023_readreg (state, 0x74);
	b = tda10023_readreg (state, 0x75);
	c = tda10023_readreg (state, 0x76);
	d = tda10023_readreg (state, 0x77);
	*ucblocks = a | (b << 8) | (c << 16) | (d << 24);

	tda10023_writebit (state, 0x10, 0x20, 0x00);
	tda10023_writebit (state, 0x10, 0x20, 0x20);
	tda10023_writebit (state, 0x13, 0x01, 0x00);
	return 0;
}

static int tda10023_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct tda10023_state* state = fe->demodulator_priv;
	int sync,inv;
	s8 afc = 0;

	sync = tda10023_readreg(state, 0x11);
	afc = tda10023_readreg(state, 0x19);
	inv = tda10023_readreg(state, 0x04);

	if (paramDebug > 149)
	{
		/* AFC only valid when carrier has been recovered */
		printk(sync & 2 ? "%s TDA10023(%d): AFC (%d) %dHz\n" :
				  "%s TDA10023(%d): [AFC (%d) %dHz]\n",
			__func__, state->frontend.dvb->num, afc,
		       -((s32)p->u.qam.symbol_rate * afc) >> 10);
	}
	p->inversion = (inv & 0x20 ? 0 : 1);
	p->u.qam.modulation = ((state->reg0 >> 2) & 7) + QAM_16;

	p->u.qam.fec_inner = FEC_NONE;
	p->frequency = ((p->frequency + 31250) / 62500) * 62500;

	if (sync & 2)
	{
		p->frequency -= ((s32)p->u.qam.symbol_rate * afc) >> 10;
	}
	return 0;
}

static int tda10023_sleep(struct dvb_frontend *fe)
{
	struct tda10023_state* state = fe->demodulator_priv;

	tda10023_writereg (state, 0x1b, 0x02);  /* pdown ADC */
	tda10023_writereg (state, 0x00, 0x80);  /* standby */
	return 0;
}

static int tda10023_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct tda10023_state* state = fe->demodulator_priv;

	if (enable)
	{
		dprintk(20, "Enable i2c gate\n");
		lock_tuner(state);
	}
	else
	{
		dprintk(20, "Disable i2c gate\n");
		unlock_tuner(state);
	}
	return 0;
}

static void tda10023_release(struct dvb_frontend *fe)
{
	struct tda10023_state *state = fe->demodulator_priv;
	kfree(state);
}

static struct dvb_frontend_ops tda10023_ops;

struct dvb_frontend *tda10023_attach(const struct tda10023_config *config, struct i2c_adapter *i2c, u8 pwm)
{
	struct tda10023_state* state = NULL;

	dprintk(100, "%s >\n", __func__);

	/* allocate memory for the internal state */
	state = kzalloc(sizeof(struct tda10023_state), GFP_KERNEL);
	if (state == NULL)
	{
		goto error;
	}
	/* setup the state */
	state->config = config;
	state->i2c = i2c;

	/* wakeup if in standby */
	tda10023_writereg (state, 0x00, 0x33);
	/* check if the demod is there */
	if ((tda10023_readreg(state, 0x1a) & 0xf0) != 0x70)
	{
		goto error;
	}
	/* create dvb_frontend */
	memcpy(&state->frontend.ops, &tda10023_ops, sizeof(struct dvb_frontend_ops));
	state->pwm = pwm;
	state->reg0 = REG0_INIT_VAL;
	if (state->config->xtal)
	{
		state->xtal  = state->config->xtal;
		state->pll_m = state->config->pll_m;
		state->pll_p = state->config->pll_p;
		state->pll_n = state->config->pll_n;
	}
	else
	{
		/* set default values if not defined in config */
		state->xtal  = 28920000;
		state->pll_m = 8;
		state->pll_p = 4;
		state->pll_n = 1;
	}
	/* calc sysclk */
	state->sysclk = (state->xtal * state->pll_m / (state->pll_n * state->pll_p));
	state->frontend.ops.info.symbol_rate_min = (state->sysclk / 2) / 64;
	state->frontend.ops.info.symbol_rate_max = (state->sysclk / 2) / 4;

	dprintk(20, "%s: TDA10023 xtal:%d pll_m:%d pll_p:%d pll_n:%d\n",
		__func__, state->xtal, state->pll_m, state->pll_p, state->pll_n);

	state->frontend.demodulator_priv = state;
	dprintk(100, "%s < ok\n", __func__);
	return &state->frontend;

error:
	kfree(state);
	dprintk(1, "%s < failed\n", __func__);
	return NULL;
}
EXPORT_SYMBOL(tda10023_attach);

#if DVB_API_VERSION >= 5
static int tda10023_get_property(struct dvb_frontend *fe, struct dtv_property* tvp)
{
	/* get delivery system info */
	if (tvp->cmd==DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBC_ANNEX_AC:
			case SYS_DVBC_ANNEX_B:
			{
				break;
			}
			default:
			{
				return -EINVAL;
			}
		}
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}
#endif

static struct dvb_frontend_ops tda10023_ops =
{
	.info =
	{
		.name               = "Philips TDA10023 DVB-C",
		.type               = FE_QAM,
		.frequency_stepsize = 62500,
		.frequency_min      =  47000000,
		.frequency_max      = 862000000,
		.symbol_rate_min    = 0,  /* set in tda10023_attach */
		.symbol_rate_max    = 0,  /* set in tda10023_attach */
		.caps               = 0x400 //FE_CAN_QAM_4
		                    | FE_CAN_QAM_16
		                    | FE_CAN_QAM_32
		                    | FE_CAN_QAM_64
		                    | FE_CAN_QAM_128
		                    | FE_CAN_QAM_256
		                    | FE_CAN_FEC_AUTO
	},
	.release                = tda10023_release,
	.init                   = tda10023_init,
	.sleep                  = tda10023_sleep,
	.i2c_gate_ctrl          = tda10023_i2c_gate_ctrl,
	.set_frontend           = tda10023_set_parameters,
	.get_frontend           = tda10023_get_frontend,
	.read_status            = tda10023_read_status,
	.read_ber               = tda10023_read_ber,
	.read_signal_strength   = tda10023_read_signal_strength,
	.read_snr               = tda10023_read_snr,
	.read_ucblocks          = tda10023_read_ucblocks,

#if DVB_API_VERSION >= 5
	.get_property           = tda10023_get_property,
#endif
};

#if 0
module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Turn on/off frontend debugging (default:off).");

MODULE_DESCRIPTION("Philips TDA10023 DVB-C demodulator driver");
MODULE_AUTHOR("Georg Acher, Hartmut Birr");
MODULE_LICENSE("GPL");
#endif
// vim:ts=4
