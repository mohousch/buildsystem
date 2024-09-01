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
#include "ix7306.h"

struct ix7306_state
{
	struct dvb_frontend *fe;
	struct i2c_adapter *i2c;
	const struct ix7306_config *config;

	/* state cache */
	u32 frequency;
	u32 bandwidth;
};

static int reg_data[10];

static void tun_setfreq_QM1D1B0004(struct dvb_frontend *fe, long freq, long baud, int *byte_);
static void calculate_pll_vco_QM1D1B0004(long freq, int *byte_);
static int calculate_local_frequency_band(long freq);
static void calculate_band_to_byte(int band, int *byte_);
static void calculate_pll_divider_byte_QM1D1B0004(long freq, int *byte_);
static int calculate_dividing_factor_of_prescaler(int *byte_);
static void calculate_pll_lpf_bw_from_baud_QM1D1B0004(long baud, int *byte_);
static long calculate_LPF_from_baud(long baud);
static void calculate_pll_lpf_to_byte(long LPF, int *byte);
static void pll_setdata_QM1D1B0004(struct dvb_frontend *fe, int *byte_);
static int calculate_pll_step_QM1D1B0004(int byte4);
static int calculate_pll_xtal_QM1D1B0004(void);

static int ix7306_write(struct ix7306_state *state, u8 *buf, u8 len)
{
	const struct ix7306_config *config = state->config;
	int err = 0;
	struct i2c_msg msg = { .addr = config->addr, .flags = 0, .buf = buf, .len = len };
#if defined(IX7306_DEBUG)
	int i;
	for (i = 0; i < len; i++)
	{
		printk("%02x ", buf[i]);
	}
	printk("\n");
#endif
	err = i2c_transfer(state->i2c, &msg, 1);
	//printk("err = %d\n", err);
	if (err != 1)
	{
		printk("msg.addr = %d\n", msg.addr);
		printk("config->addr = %d\n", config->addr);
		printk("%s: write error, err=%d\n", __func__, err);
		return -1;
	}
	return 0;
}

static int ix7306_read(struct ix7306_state *state, u8 *buf)
{
	const struct ix7306_config *config = state->config;
	int err = 0;
	u8 tmp = 1;
	struct i2c_msg msg[] =
	{
		{ .addr = config->addr, .flags = 0, .buf = &tmp, .len = 1 },
		{ .addr = config->addr, .flags = I2C_M_RD, .buf = buf, .len = 1 }
	};
	//err = i2c_transfer(state->i2c, &msg[0], 1);
	state->fe->ops.i2c_gate_ctrl(state->fe, 0);
	//msleep(12);
	state->fe->ops.i2c_gate_ctrl(state->fe, 1);
	err = i2c_transfer(state->i2c, &msg[1], 1);
	if (err != 1)
	{
		printk("%s: read error, err=%d\n", __func__, err);
		return -1;
	}
	return 0;
}

/*-------------------------------------------------------------*

 bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0

 byte1
 byte2
 byte3
 byte4
 byte5 BA2 BA1 BA0 PSC DIV/TS1

*--------------------------------------------------------------*/

static void tun_setfreq_QM1D1B0004(struct dvb_frontend *fe, long freq, long baud, int *byte_)
{
	/*set byte5 BA2 BA1 BA0 PSC DIV/TS1*/
	calculate_pll_vco_QM1D1B0004(freq, byte_);
	calculate_pll_divider_byte_QM1D1B0004(freq, byte_);
	calculate_pll_lpf_bw_from_baud_QM1D1B0004(baud, byte_);
	pll_setdata_QM1D1B0004(fe, byte_);
}

static void calculate_pll_vco_QM1D1B0004(long freq, int *byte_)
{
	int band;
	band = calculate_local_frequency_band(freq);
	calculate_band_to_byte(band, byte_);
}

static int calculate_local_frequency_band(long freq)
{
	int band;
//	if(950000<=freq && freq<986000)
	if (freq < 986000) band = 1;
	if (986000 <= freq && freq < 1073000) band = 2;
	if (1073000 <= freq && freq < 1154000) band = 3;
	if (1154000 <= freq && freq < 1291000) band = 4;
	if (1291000 <= freq && freq < 1447000) band = 5;
	if (1447000 <= freq && freq < 1615000) band = 6;
	if (1615000 <= freq && freq < 1791000) band = 7;
	if (1791000 <= freq && freq < 1972000) band = 8;
	if (1972000 <= freq) band = 9;
	return band;
}

static void calculate_band_to_byte(int band, int *byte_)
{
	//byte5
	int PSC;
	int DIV, BA210;
	if ((band == 1) || (band == 2)) PSC = 1;
	else PSC = 0;
	*(byte_ + 4) &= 0xEF;
	*(byte_ + 4) |= (PSC << 4);
	//
	if (band == 1) DIV = 1, BA210 = 0x05;
	if (band == 2) DIV = 1, BA210 = 0x06;
	if (band == 3) DIV = 1, BA210 = 0x07;
	if (band == 4) DIV = 0, BA210 = 0x01;
	if (band == 5) DIV = 0, BA210 = 0x02;
	if (band == 6) DIV = 0, BA210 = 0x03;
	if (band == 7) DIV = 0, BA210 = 0x04;
	if (band == 8) DIV = 0, BA210 = 0x05;
	if (band == 9) DIV = 0, BA210 = 0x06;
	*(byte_ + 4) &= 0x1D;
	*(byte_ + 4) |= (DIV << 1);
	*(byte_ + 4) |= (BA210 << 5);
}

static void calculate_pll_divider_byte_QM1D1B0004(long freq, int *byte_)
{
	long data;
	int P, N, A;
	if (freq < 1024000) byte_[3] |= 0x01;
	else byte_[3] &= 0xfe;
	if (freq < 1450000)
	{
		if (freq < 1300000) byte_[3] &= 0xbf;
		else if (freq < 1375000) byte_[3] &= 0xdf;
	}
	else
	{
		if (freq < 1850000) byte_[3] &= 0xbf;
		else if (freq < 2045000) byte_[3] &= 0xdf;
	}
	data = (long)((freq * 10LL) / calculate_pll_step_QM1D1B0004(*(byte_ + 3)) + 5) / 10 ;
	P = calculate_dividing_factor_of_prescaler(byte_);
	N = data / P;
	A = data - P * N;
	data = (N << 5) | A;
	//040408
	//BG should not be changed...
	*(byte_ + 1) &= 0x60; //byte2:BG set
	*(byte_ + 1) |= (int)((data >> 8) & 0x1F); //byte2
	*(byte_ + 2) = (int)(data & 0xFF); //byte3
}

static int calculate_pll_step_QM1D1B0004(int byte4)
{
	int REF;
	int R;
	int pll_step;
	REF = byte4 & 0x01;
	if (REF == 0) R = 4;
	else R = 8;
	pll_step = calculate_pll_xtal_QM1D1B0004() / R;
	return pll_step;
}

static int calculate_pll_xtal_QM1D1B0004()
{
	return 4000;
}

static int calculate_dividing_factor_of_prescaler(int *byte_)
{
	int PSC;
	PSC = (*(byte_ + 4) >> 4);
	PSC &= 0x01;
	if (PSC) return 16; //PSC=1
	else return 32; //PSC=0
}

static void calculate_pll_lpf_bw_from_baud_QM1D1B0004(long baud, int *byte_)
{
	long LPF;
	if (baud > 0) //calculate LPF automatically
	{
		LPF = calculate_LPF_from_baud(baud);
		calculate_pll_lpf_to_byte(LPF, byte_);
	}
}

static long calculate_LPF_from_baud(long baud)
{
	long LPF;
	if ((34000 < baud)) LPF = 34000;
	if ((32000 < baud) && (baud <= 34000)) LPF = 34000;
	if ((30000 < baud) && (baud <= 32000)) LPF = 32000;
	if ((28000 < baud) && (baud <= 30000)) LPF = 30000;
	if ((26000 < baud) && (baud <= 28000)) LPF = 28000;
	if ((24000 < baud) && (baud <= 26000)) LPF = 26000;
	if ((22000 < baud) && (baud <= 24000)) LPF = 24000;
	if ((20000 < baud) && (baud <= 22000)) LPF = 22000;
	if ((18000 < baud) && (baud <= 20000)) LPF = 20000;
	if ((16000 < baud) && (baud <= 18000)) LPF = 18000;
	if ((14000 < baud) && (baud <= 16000)) LPF = 16000;
	if ((12000 < baud) && (baud <= 14000)) LPF = 16000;
	if ((10000 < baud) && (baud <= 12000)) LPF = 14000;
	if ((8000 < baud) && (baud <= 10000)) LPF = 14000;
	if ((6000 < baud) && (baud <= 8000)) LPF = 12000;
	if ((4000 < baud) && (baud <= 6000)) LPF = 10000;
	if (baud <= 4000) LPF = 10000;
//	if(baud>=20000) LPF=34000;
//	else LPF=20000;
	return LPF;
}

static void calculate_pll_lpf_to_byte(long LPF, int *byte)
{
	int data, PD2, PD3, PD4, PD5;
	data = (int)(LPF / 1000 / 2 - 2);
	PD2 = (data >> 3) & 0x01;
	PD3 = (data >> 2) & 0x01;
	PD4 = (data >> 1) & 0x01;
	PD5 = (data) & 0x01;
	*(byte + 3) &= 0xE7;
	*(byte + 4) &= 0xF3;
	*(byte + 3) |= ((PD5 << 4) | (PD4 << 3));
	*(byte + 4) |= ((PD3 << 3) | (PD2 << 2));
}

static void pll_setdata_QM1D1B0004(struct dvb_frontend *fe, int *byte_)
{
	struct ix7306_state *state = fe->tuner_priv;
	u8 ucOperData[5];
	u8 byte1,/*byte2,*/byte3, byte4;
	//in this function ,we operator ucOperData instead of byte_
	memset(ucOperData, 0, sizeof(ucOperData));
	ucOperData[0] = *(byte_ + 1);
	ucOperData[1] = *(byte_ + 2);
	ucOperData[2] = *(byte_ + 3);
	ucOperData[3] = *(byte_ + 4);
	byte1 = ucOperData[0]; //byte2
	byte3 = ucOperData[2]; //byte4
	byte4 = ucOperData[3]; //byte5
	ucOperData[2] &= 0xE3; //TM=0,LPF=4MHz
	ucOperData[3] &= 0xF3; //LPF=4MHz
	//byte2 / BG=01(VCO wait:2ms)
	ucOperData[0] &= 0x9F;
	ucOperData[0] |= 0x20;
	/*open i2c repeater gate*/
//	if (fe->ops.i2c_gate_ctrl(fe,1) < 0)
//		goto err;
	/*write tuner*/
	if (ix7306_write(state, ucOperData, 4) < 0)
		goto err;
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto err;
	ucOperData[2] |= 0x04; //TM=1
	/*open i2c repeater gate*/
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto err;
	/*write tuner*/
	if (ix7306_write(state, ucOperData + 2, 1) < 0)
		goto err;
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto err;
	msleep(12);
	ucOperData[2] = byte3;
	//[040108] TM bit always finis with "1".
	ucOperData[2] |= 0x04; //TM=1
	/********************************************************************
	*(byte_+3)|=0x04; //TM=1
	*(byte_+3)=byte4; //byte_4 original value
	********************************************************************/
	ucOperData[3] = byte4; //byte_5 original value
	/*open i2c repeater gate*/
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto err;
	/*write tuner*/
	if (ix7306_write(state, ucOperData + 2, 2) < 0)
		goto err;
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		goto err;
	ucOperData[0] = byte1;
	/*open i2c repeater gate*/
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		goto err;
	/*write tuner*/
	if (ix7306_write(state, ucOperData, 1) < 0)
		goto err;
	return;
err:
	printk("%s: write i2c failed\n", __func__);
}

static int ix7306_set_freq(struct dvb_frontend *fe, u32 freq_KHz, u32 tuner_BW)
{
	u32 tuner_bw_K = tuner_BW / 1000;
#if defined(IX7306_DEBUG)
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	memset(reg_data, 0, sizeof(reg_data));
	reg_data[1] = 0x44;
	reg_data[2] = 0x7e;
	reg_data[3] = 0xe1;
	reg_data[4] = 0x42;
#if defined(IX7306_DEBUG)
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	tun_setfreq_QM1D1B0004(fe, freq_KHz, tuner_bw_K, &reg_data[0]);
	return 0;
}

static int ix7306_set_state(struct dvb_frontend *fe,
							enum tuner_param param,
							struct tuner_state *tstate)
{
	struct ix7306_state *state = fe->tuner_priv;
#if defined(IX7306_DEBUG)
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	if (param & DVBFE_TUNER_FREQUENCY)
	{
		state->frequency = tstate->frequency;
		state->bandwidth = tstate->bandwidth;
#if defined(IX7306_DEBUG)
		printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
		ix7306_set_freq(fe, state->frequency, state->bandwidth);
	}
	else
	{
		printk("%s: Unknown parameter (param=%d)\n", __func__, param);
		return -EINVAL;
	}
	return 0;
}

static int ix7306_get_state(struct dvb_frontend *fe,
							enum tuner_param param,
							struct tuner_state *tstate)
{
	struct ix7306_state *state = fe->tuner_priv;
	int err = 0;
	switch (param)
	{
		case DVBFE_TUNER_FREQUENCY:
			tstate->frequency = state->frequency;
			break;
		case DVBFE_TUNER_BANDWIDTH:
			tstate->bandwidth = state->bandwidth; /* FIXME! need to calculate Bandwidth */
			break;
		default:
			printk("%s: Unknown parameter (param=%d)\n", __func__, param);
			err = -EINVAL;
			break;
	}
	return err;
}

static int ix7306_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct ix7306_state *state = fe->tuner_priv;
	u8 result[2];
	int err = 0;
	*status = 0;
	err = ix7306_read(state, result);
	if (err < 0)
	{
		printk("%s: I/O Error\n", __func__);
		return err;
	}
	if (result[0] & 0x40)
	{
#if defined(IX7306_DEBUG)
		printk("%s: Tuner Phase Locked\n", __func__);
#endif
		*status = 1;
	}
	else
	{
#if defined(IX7306_DEBUG)
		printk("%s: Tuner Phase Not Locked result - 0x%x, 0x%x,\n", __func__, result[0], result[1]);
#endif
	}
	return err;
}

static int ix7306_release(struct dvb_frontend *fe)
{
	struct ix7306_state *state = fe->tuner_priv;
	fe->tuner_priv = NULL;
	kfree(state);
	return 0;
}

#define REF_OSC_FREQ 4000 /* 4MHZ */
static int ix7306_set_params(struct dvb_frontend *fe,
							 struct dvb_frontend_parameters *p)
{
	struct ix7306_state *state = fe->tuner_priv;
	int err = 0;
	u32 freq, sym;
	u8 data[4];
	u16 Npro, tmp;
	u32 Rs, BW;
	u8 Nswa;
	u8 LPF = 15;
	u8 BA = 1;
	u8 DIV = 0;
	int result = 0;
	freq = p->frequency;
	sym = p->u.qpsk.symbol_rate;
#if 0
	u32 ratio = 135;
#endif
	Rs = sym;
	if (Rs == 0)
		Rs = 45000;
#if 1
	BW = Rs * 135 / 200;
	BW = BW * 130 / 100;
	if (Rs < 6500)
		BW = BW + 3000;
	BW = BW + 2000;
	BW = BW * 108 / 100;
#else
	if (ratio == 0)
		BW = 34000;
	else
		BW = Rs * ratio / 100;
#endif
	if (BW < 10000)
		BW = 10000;
	if (BW > 34000)
		BW = 34000;
	if (BW <= 10000)
		LPF = 3;
	else if (BW <= 12000)
		LPF = 4;
	else if (BW <= 14000)
		LPF = 5;
	else if (BW <= 16000)
		LPF = 6;
	else if (BW <= 18000)
		LPF = 7;
	else if (BW <= 20000)
		LPF = 8;
	else if (BW <= 22000)
		LPF = 9;
	else if (BW <= 24000)
		LPF = 10;
	else if (BW <= 26000)
		LPF = 11;
	else if (BW <= 28000)
		LPF = 12;
	else if (BW <= 30000)
		LPF = 13;
	else if (BW <= 32000)
		LPF = 14;
	else
		LPF = 15;
	if (freq <= 1154)
		DIV = 1;
	else
		DIV = 0;
	if (freq <= 986)
		BA = 5;
	else if (freq <= 1073)
		BA = 6;
	else if (freq <= 1154)
		BA = 7;
	else if (freq <= 1291)
		BA = 1;
	else if (freq <= 1447)
		BA = 2;
	else if (freq <= 1615)
		BA = 3;
	else if (freq <= 1791)
		BA = 4;
	else if (freq <= 1972)
		BA = 5;
	else //if (freq <= 2150)
		BA = 6;
	tmp = freq * 1000 * 8 / REF_OSC_FREQ;
	Nswa = tmp % 32;
	Npro = tmp / 32;
	data[0] = (u8)((Npro >> 3) & 0x1F);
	data[0] = data[0] | 0x40;
	data[1] = Nswa | (((u8)Npro & 0x07) << 5);
	data[2] = 0xE1;
	data[3] = (BA << 5) | (DIV << 1);
	if (ix7306_write(state, data, 4) < 0)
	{
		printk("nim_vz7306_control: I2C write error\n");
		return result;
	}
	data[2] = 0xE5;
	if (ix7306_write(state, data + 2, 1) < 0)
	{
		printk("nim_vz7306_control: I2C write error\n");
		return result;
	}
	mdelay(10);
	data[2] |= ((LPF & 0x01) << 4) | ((LPF & 0x02) << 2);
	data[3] |= ((LPF & 0x04) << 1) | ((LPF & 0x08) >> 1);
	if (ix7306_write(state, data + 2, 2) < 0)
	{
		printk("nim_vz7306_control: I2C write error\n");
		return result;
	}
	return err;
}

static unsigned char init_data1[] = { 0x44, 0x7e, 0xe1, 0x42 };
static unsigned char init_data2[] = { 0xe5 };
static unsigned char init_data3[] = { 0xfd, 0x0d };

static int ix7306_init(struct dvb_frontend *fe)
{
	int result = 0;
	struct ix7306_state *state = fe->tuner_priv;
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
	{
		printk("ix7306_init i2c_gate_ctrl 1 error\n");
		return -1;
	}
	/* tuner */
	if (ix7306_write(state, init_data1, sizeof(init_data1)) < 0)
	{
		printk("nim_vz7306_control: I2C write error init_data1\n");
		return result;
	}
	if (ix7306_write(state, init_data2, sizeof(init_data2)) < 0)
	{
		printk("nim_vz7306_control: I2C write error init_data2\n");
		return result;
	}
	//osal_delay(10000);
	mdelay(10);
	if (ix7306_write(state, init_data3, sizeof(init_data3)) < 0)
	{
		printk("nim_vz7306_control: I2C write error init_data3\n");
		return result;
	}
	return result;
}

static struct dvb_tuner_ops ix7306_ops =
{

	.info = {
		.name = "IX7306",
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_step = 0
	},

	.set_state = ix7306_set_state,
	.get_state = ix7306_get_state,
	.get_status = ix7306_get_status,
	.set_params = ix7306_set_params,
	.release = ix7306_release,
	.init = ix7306_init
};

int ix7306_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct dvb_frontend_ops *frontend_ops = NULL;
	struct dvb_tuner_ops *tuner_ops = NULL;
	struct tuner_state t_state;
	int err = 0;
	if (&fe->ops)
		frontend_ops = &fe->ops;
#if defined(IX7306_DEBUG)
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	if (&frontend_ops->tuner_ops)
		tuner_ops = &frontend_ops->tuner_ops;
#if defined(IX7306_DEBUG)
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	if (tuner_ops->get_state)
	{
		if ((err = tuner_ops->get_state(fe, DVBFE_TUNER_FREQUENCY, &t_state)) < 0)
		{
			printk("%s: Invalid parameter\n", __func__);
			return err;
		}
		*frequency = t_state.frequency;
#if defined(IX7306_DEBUG)
		printk("%s: Frequency=%d\n", __func__, t_state.frequency);
#endif
	}
	return 0;
}

int ix7306_set_frequency(struct dvb_frontend *fe, u32 frequency)
{
	struct ix7306_state *state = fe->tuner_priv;
	struct dvb_frontend_ops *frontend_ops = NULL;
	struct dvb_tuner_ops *tuner_ops = NULL;
	struct tuner_state t_state;
	int err = 0;
	t_state.frequency = frequency;
	t_state.bandwidth = state->bandwidth;
	if (&fe->ops)
		frontend_ops = &fe->ops;
	if (&frontend_ops->tuner_ops)
		tuner_ops = &frontend_ops->tuner_ops;
	if (tuner_ops && tuner_ops->set_state)
	{
		if ((err = tuner_ops->set_state(fe, DVBFE_TUNER_FREQUENCY, &t_state)) < 0)
		{
			printk("%s: Invalid parameter\n", __func__);
			return err;
		}
	}
#if defined(IX7306_DEBUG)
	printk("%s: Frequency=%d\n", __func__, t_state.frequency);
#endif
	return 0;
}

int ix7306_set_bandwidth(struct dvb_frontend *fe, u32 bandwidth)
{
	struct ix7306_state *state = fe->tuner_priv;
	state->bandwidth = bandwidth;
//	ix7306_set_freq (fe, state->frequency, state->bandwidth);
	return 0;
}
int ix7306_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct dvb_frontend_ops *frontend_ops = &fe->ops;
	struct dvb_tuner_ops *tuner_ops = &frontend_ops->tuner_ops;
	struct tuner_state t_state;
	int err = 0;
	if (&fe->ops)
		frontend_ops = &fe->ops;
	if (&frontend_ops->tuner_ops)
		tuner_ops = &frontend_ops->tuner_ops;
	if (tuner_ops->get_state)
	{
		if ((err = tuner_ops->get_state(fe, DVBFE_TUNER_BANDWIDTH, &t_state)) < 0)
		{
			printk("%s: Invalid parameter\n", __func__);
			return err;
		}
		*bandwidth = t_state.bandwidth;
	}
#if defined(IX7306_DEBUG)
	printk("%s: Bandwidth=%d\n", __func__, t_state.bandwidth);
#endif
	return 0;
}

struct dvb_frontend *ix7306_attach(struct dvb_frontend *fe,
								   const struct ix7306_config *config,
								   struct i2c_adapter *i2c)
{
	struct ix7306_state *state = NULL;
	if ((state = kzalloc(sizeof(struct ix7306_state), GFP_KERNEL)) == NULL)
		goto exit;
	state->config = config;
	state->i2c = i2c;
	state->fe = fe;
	state->bandwidth = 125000;
	fe->tuner_priv = state;
	fe->ops.tuner_ops = ix7306_ops;
	printk("%s: Attaching %s IX7306 8PSK/QPSK tuner\n",
		   __func__, config->name);
	return fe;
exit:
	kfree(state);
	return NULL;
}

int tuner_Sharp7306_Identify(struct dvb_frontend *fe,
							 const struct ix7306_config *config,
							 struct i2c_adapter *i2c)//lwj change
{
	int Err = 0;
	unsigned char ucIOBuffer[4 + 1];
	unsigned char ucData = 0;
	struct ix7306_state state;
	memset(&state, 0, sizeof(struct ix7306_state));
	state.config = config;
	state.i2c = i2c;
	state.fe = fe;
	ucIOBuffer[0] = 0x44;
	ucIOBuffer[1] = 0x7e;
	ucIOBuffer[2] = 0xe1;
	ucIOBuffer[3] = 0x42;
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
	{
		printk("i2c_gate_ctrl 1 error\n");
		return -1;
	}
	Err = ix7306_write(&state, ucIOBuffer, 4);
	printk("ix7306_write ucIOBuffer 4 Err = %d\n", Err);
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
	{
		printk("i2c_gate_ctrl 1 error\n");
		return -1;
	}
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
	{
		printk("i2c_gate_ctrl 0 error\n");
		return -1;
	}
	Err = ix7306_write(&state, ucIOBuffer, 1);
	printk("ix7306_write ucIOBuffer 1 Err = %d\n", Err);
	msleep(12);
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
	{
		printk("i2c_gate_ctrl 0 error\n");
		return -1;
	}
	memset(ucIOBuffer, 0, sizeof(ucIOBuffer));
	ucIOBuffer[0] = 0xfd;
	ucIOBuffer[1] = 0x0d;
	if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
	{
		printk("i2c_gate_ctrl 1 error\n");
		return -1;
	}
	Err = ix7306_write(&state, ucIOBuffer, 2);
	printk("ix7306_write ucIOBuffer Err 2 = %d\n", Err);
	if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
	{
		printk("i2c_gate_ctrl 0 error\n");
		return -1;
	}
	if (Err == 0)
	{
		msleep(50);
		if (fe->ops.i2c_gate_ctrl(fe, 1) < 0)
		{
			printk("i2c_gate_ctrl 1 error\n");
			return -1;
		}
		Err = ix7306_read(&state, &ucData);
		printk("ix7306_read Err = %d ucData = 0x%x\n", Err, ucData);
		if (Err == 0)
		{
			if ((ucData & 0x3f) == 0x18)
			{
				printk("========tuner_sharp7306_identify =====\n");
				Err = 0;
			}
			else
			{
				Err = -1;
			}
		}
		if (fe->ops.i2c_gate_ctrl(fe, 0) < 0)
		{
			printk("i2c_gate_ctrl 0 error\n");
			return -1;
		}
	}
	return Err;
}

EXPORT_SYMBOL(ix7306_attach);
