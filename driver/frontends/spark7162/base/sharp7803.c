/*
 IX7306 8PSK/QPSK tuner driver
 Copyright (C) Manu Abraham <abraham.manu@gmail.com>

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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "dvb_frontend.h"
#include "sharp7803.h"

struct sharp7803_state
{
	struct dvb_frontend *fe;
	struct i2c_adapter *i2c;
	const struct sharp7803_config *config;

	/* state cache */
	u32 frequency;
	u32 bandwidth;
};

static int sharp7803_write(struct sharp7803_state *state, u8 reg, u8 *buf, u8 len)
{
	const struct sharp7803_config *config = state->config;
	int err = 0;
	int i;
	u8 aBuf[len + 1];
	struct i2c_msg msg = { .addr = config->addr, .flags = 0, .buf = buf, .len = len + 1 };
	aBuf[0] = reg;
	memcpy(&aBuf[1], buf, len);
	for (i = 0; i < len + 1; i++)
	{
		printk("%02x ", aBuf[i]);
	}
	printk("\n");
	err = i2c_transfer(state->i2c, &msg, 1);
	if (err != 1)
		printk("%s: write error, err=%d\n", __func__, err);
	return err;
}

static int sharp7803_read(struct sharp7803_state *state, u8 reg, u8 *buf)
{
	const struct sharp7803_config *config = state->config;
	int err = 0;
	u8 tmp = reg;
	struct i2c_msg msg[] =
	{
		{ .addr = config->addr, .flags = 0, .buf = &tmp, .len = 1 },
		{ .addr = config->addr, .flags = I2C_M_RD, .buf = buf, .len = 1 }
	};
	//err = i2c_transfer(state->i2c, msg, 1);
	//err = i2c_transfer(state->i2c, &msg[1], 1);
	err = i2c_transfer(state->i2c, msg, 2);
	if (err != 2)
		printk("%s: read error, err=%d\n", __func__, err);
	return err;
}

static int sharp7803_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct sharp7803_state *state = fe->tuner_priv;
	u8 result[2];
	int err = 0;
	*status = 0;
	err = sharp7803_read(state, 0x0d, result);
	if (err < 0)
	{
		printk("%s: I/O Error\n", __func__);
		return err;
	}
	if (result[0] & 0x40)
	{
		printk("%s: Tuner Phase Locked\n", __func__);
		*status = 1;
	}
	else
	{
		printk("%s: Tuner Phase Not Locked result - 0x%x, 0x%x,\n", __func__, result[0], result[1]);
	}
	return err;
}

static int sharp7803_release(struct dvb_frontend *fe)
{
	struct sharp7803_state *state = fe->tuner_priv;
	fe->tuner_priv = NULL;
	kfree(state);
	return 0;
}

static int sharp7803_set_params(struct dvb_frontend *fe,
								struct dvb_frontend_parameters *p)
{
	struct sharp7803_state *state = fe->tuner_priv;
	int Error = 0;
	u8 data[16]; // data[15] is i2c read_only
	u32 Rs, BW;
	u8 LPF = 13;
	u8 BA = 1;
	u8 DIV = 1;
	u8 i;
	u8 search_mode = 0;
	u8 F_ref, pll_ref_div, alpha, N, A, CSEL_Offset, BB_Gain;
	u32 M, sd;
	u32 freq_KHz;
	u32 sym;
	freq_KHz = p->frequency;
	sym = p->u.qpsk.symbol_rate;
	BB_Gain = 0x01; // 00:? 01:-1db, 11:-4db
	for (i = 0; i < 16; i++)
		data[i] = 0;
	/* LPF cut_off */
	Rs = sym;
	if (freq_KHz > 2200)
		freq_KHz = 2200;
	if (Rs == 0)
		Rs = 45000;
	BW = Rs * 135 / 200; // rolloff factor is 35%
	if (Rs < 6500) BW = BW + 4000; // add 3M when Rs<5M, since we need shift 3M to avoid DC
	BW = BW + 2000; // add 2M for LNB frequency shifting
//ZCY: the cutoff freq of IX2410 is not 3dB point, it more like 0.1dB, so need not 30%
// BW = BW*130/100; // 0.1dB to 3dB need about 30% transition band for 7th order LPF
	BW = BW * 108 / 100; // add 8% margin since fc is not very accurate
	if (BW < 4000) BW = 4000; // Sharp2410 LPF can be tuned form 10M to30M, step is 2.0M
	if (BW > 34000) BW = 34000; // the range can be 6M~~34M actually, 4Mis not good
	if (BW <= 4000) LPF = 0;
	else if (BW <= 6000) LPF = 1;
	else if (BW <= 8000) LPF = 2;
	else if (BW <= 10000) LPF = 3;
	else if (BW <= 12000) LPF = 4;
	else if (BW <= 14000) LPF = 5;
	else if (BW <= 16000) LPF = 6;
	else if (BW <= 18000) LPF = 7;
	else if (BW <= 20000) LPF = 8;
	else if (BW <= 22000) LPF = 9;
	else if (BW <= 24000) LPF = 10;
	else if (BW <= 26000) LPF = 11;
	else if (BW <= 28000) LPF = 12;
	else if (BW <= 30000) LPF = 13;
	else if (BW <= 32000) LPF = 14;
	else LPF = 15;
	if (LPF < 3) CSEL_Offset = 3;
	else if (LPF < 6) CSEL_Offset = 2;
	else if (LPF < 13) CSEL_Offset = 1;
	else CSEL_Offset = 0;
	/* local oscillator select */
	DIV = 1;
	if (freq_KHz <= 975)
	{
		BA = 6;
		DIV = 0;
	}
	else if (freq_KHz <= 1200)
	{
		BA = 7;
		DIV = 0;
	}
	else if (freq_KHz <= 1250)
		BA = 2;
	else if (freq_KHz <= 1450)
		BA = 3;
	else if (freq_KHz <= 1600)
		BA = 4;
	else if (freq_KHz <= 1800)
		BA = 5;
	else if (freq_KHz <= 1950)
		BA = 6;
	else //if(freq <= 2150)
		BA = 7;
	F_ref = 16;
	pll_ref_div = 0;
	M = (freq_KHz << 16);
	alpha = (((M >> 19) + 1) >> 1);
	if (M > (alpha << 20))
		sd = M - (alpha << 20);
	else
		sd = 0x800000 - (alpha << 20) + M;
	N = (alpha - 12) / 4;
	A = alpha - 4 * (N + 1) - 5;
	// Set VCO_TM and LFP_TM
	Error |= sharp7803_read(state, 0x0c, data);
	data[0] &= 0x3F;
	Error |= sharp7803_write(state, 0x0c, data, 1);
	mdelay(2);
	data[2] = (DIV << 7) | (BA << 4) ;
	data[3] = 0x10;
	if (1 == search_mode)
	{
		data[3] |= 0x01; //fast search
	}
	else
	{
		data[3] &= 0xfe; //normal
	}
	Error |= sharp7803_write(state, 0x02, data + 2, 2);
	//data[4] = 0xBC;
	data[5] = 0xC5;
	data[6] = ((pll_ref_div << 7) | (N));
	data[7] = (A & 0x0F) | 0x30;
	//data[8] = (LPF&0x0f);
	data[8] = 0x02;
	data[9] = (sd >> 16) & 0x3f;
	data[10] = (sd >> 8) & 0xff;
	data[11] = (sd) & 0xff;
// data[12] = 0xc0;
	Error |= sharp7803_write(state, 0x05, data + 5, 7);
	/*VCO_TM , LPF_TM*/
	Error |= sharp7803_write(state, 0x0c, data, 1);
	data[0] |= 0xC0;
	Error |= sharp7803_write(state, 0x0c, data, 1);
	mdelay(20);
	data[0] = (LPF & 0x0f);
	Error |= sharp7803_write(state, 0x08, data, 1);
	data[0] = 0xf3;
	Error |= sharp7803_write(state, 0x12, data, 1);
	data[0] = ((CSEL_Offset & 0x03) << 5);
	Error |= sharp7803_write(state, 0x13, data, 1);
	return Error;
}

static int sharp7803_init(struct dvb_frontend *fe)
{
	int result = 0;
	struct sharp7803_state *state = fe->tuner_priv;
	int Error = 0;
	u8 chip_id = 0x44;
	u8 data[16] = {0x48, 0x1c, 0xA0, 0x10, 0xBC, 0xC5, 0x20, 0x33,
				   0x06, 0x00, 0x00, 0x00, 0x03
				  };
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);
	// Soft reset
	data[1] = 0x0c;
	Error |= sharp7803_write(state, 0x01, data + 1, 1);
	mdelay(20);
	Error |= sharp7803_write(state, 0x01, data + 1, 1);
	mdelay(20);
	Error |= sharp7803_write(state, 0x01, data + 1, 1);
	mdelay(20);
	data[0x0c] = 0x40;
	Error |= sharp7803_write(state, 0x0c, data + 0x0c, 1);
	mdelay(20);
	/*Ð´ tuner*/
	Error |= sharp7803_write(state, 0, &chip_id, 1);
	mdelay(1);
	/*¶Átuner*/
	Error |= sharp7803_read(state, 0, &chip_id);
	printk("chip_id = %d\n", chip_id);
	// Write fixed value
	Error |= sharp7803_write(state, 0x01, data + 1, 12);
	data[0] = 0xFF;
	data[1] = 0xF3;
	data[2] = 0x00;
	data[3] = 0x2a; // set bb gain BB_Gain = 0x01; // 00:? 01:-1db, 11:-4db
	data[4] = 0x64;
	data[5] = 0xa6;
	data[6] = 0x86;
	data[7] = 0xcc;
	data[8] = 0xcf;
	data[9] = 0x95;
	data[10] = 0xf1;
	data[11] = 0xa8;
	data[12] = 0xf2;
	data[13] = 0x09;
	data[14] = 0x00;
	Error |= sharp7803_write(state, 0x11, data, 15);
	return result;
}

static struct dvb_tuner_ops sharp7803_ops =
{

	.info = {
		.name = "sharp7803",
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_step = 0
	},

	.get_status = sharp7803_get_status,
	.set_params = sharp7803_set_params,
	//.init = sharp7803_init,
	.release = sharp7803_release,
};

struct dvb_frontend *sharp7803_attach(struct dvb_frontend *fe,
									  const struct sharp7803_config *config,
									  struct i2c_adapter *i2c)
{
	struct sharp7803_state *state = NULL;
	if ((state = kzalloc(sizeof(struct sharp7803_state), GFP_KERNEL)) == NULL)
		goto exit;
	state->config = config;
	state->i2c = i2c;
	state->fe = fe;
	state->bandwidth = 125000;
	fe->tuner_priv = state;
	fe->ops.tuner_ops = sharp7803_ops;
	printk("%s: Attaching %s Sharp7803 8PSK/QPSK tuner\n",
		   __func__, config->name);
	sharp7803_init(fe);
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
	return fe;
exit:
	kfree(state);
	return NULL;
}

EXPORT_SYMBOL(sharp7803_attach);
