/*
 STV6110(A) Silicon tuner driver

 Copyright (C) Manu Abraham <abraham.manu@gmail.com>

 Copyright (C) ST Microelectronics

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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <asm/uaccess.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include "dvb_frontend.h"
#include "core.h"
#include "stv6110x_reg.h"
#include "stv6110x.h"
#include "stv6110x_priv.h"

extern short paramDebug;
#define TAGDEBUG "[stv6110a] "

#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
	} while (0)

static int stv6110x_set_refclock(struct dvb_frontend *fe, u32 refclock);
static int stv6110x_get_frequency(struct dvb_frontend *fe, u32 *frequency);
static int stv6110x_set_bandwidth(struct dvb_frontend *fe, u32 bandwidth);

//static u8 stv6110x_regs[] = {0x07, 0x11, 0xdc, 0x85, 0x17, 0x01, 0xe6, 0x1e};

static int stv6110x_read_reg(struct stv6110x_state *stv6110x, u8 reg, u8 *data)
{
	int ret;
	const struct stv6110x_config *config = stv6110x->config;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };
	struct i2c_msg msg[] =
	{
		{ .addr = config->addr, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = config->addr, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	};
	if (stv6110x->fe->ops.i2c_gate_ctrl)
		stv6110x->fe->ops.i2c_gate_ctrl(stv6110x->fe, 1);
	ret = i2c_transfer(stv6110x->i2c, msg, 2);
	if (ret != 2)
	{
		printk("stv6110x_read_reg I/O Error %d, addr=0x%02x, reg=0x%02x (res 0x%02x - 0x%02x) ret = %d (expected 2)\n", ret, config->addr, reg, *data, b1[0], ret);
		return -EREMOTEIO;
	}
	*data = b1[0];
	if (stv6110x->fe->ops.i2c_gate_ctrl)
		stv6110x->fe->ops.i2c_gate_ctrl(stv6110x->fe, 0);
	dprintk(200, "%s: reg=0x%02x , data=%02x\n", __func__, reg, *data);
	return 0;
}

static int stv6110x_write_reg(struct stv6110x_state *stv6110x, u8 reg, u8 data)
{
	int ret;
	const struct stv6110x_config *config = stv6110x->config;
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = config->addr, .flags = 0, . buf = buf, .len = 2 };
	if (stv6110x->fe->ops.i2c_gate_ctrl)
		stv6110x->fe->ops.i2c_gate_ctrl(stv6110x->fe, 1);
	dprintk(200, "%s: reg = 0x%02x val = 0x%02x\n", __func__, reg, data);
	ret = i2c_transfer(stv6110x->i2c, &msg, 1);
	if (ret != 1)
	{
		printk("stv6110x_write_reg I/O Error %d, 0x%x\n", ret, config->addr);
		return -EREMOTEIO;
	}
	if (stv6110x->fe->ops.i2c_gate_ctrl)
		stv6110x->fe->ops.i2c_gate_ctrl(stv6110x->fe, 0);
	return 0;
}

static int stv6110x_write_init(struct stv6110x_state *stv6110x)
{
	int ret;
	const struct stv6110x_config *config = stv6110x->config;
	static u8 init_data[] = {0x00, 0x07, 0x11, 0xdc, 0x85, 0x17, 0x01, 0xe6, 0x1e};
	stv6110x->stv6110x_regs[0] = 0x07;
	stv6110x->stv6110x_regs[1] = 0x11;
	stv6110x->stv6110x_regs[2] = 0xdc;
	stv6110x->stv6110x_regs[3] = 0x85;
	stv6110x->stv6110x_regs[4] = 0x17;
	stv6110x->stv6110x_regs[5] = 0x01;
	stv6110x->stv6110x_regs[6] = 0xe6;
	stv6110x->stv6110x_regs[7] = 0x1e;
	struct i2c_msg msg = { .addr = config->addr, .flags = 0, . buf = init_data, .len = 9};
	dprintk(10, "stv6110x_write_regN >\n");
	if (stv6110x->fe->ops.i2c_gate_ctrl)
		stv6110x->fe->ops.i2c_gate_ctrl(stv6110x->fe, 1);
	ret = i2c_transfer(stv6110x->i2c, &msg, 1);
	if (ret != 1)
	{
		printk("stv6110x_write_regN I/O Error %d, 0x%x, i2c-%d\n", ret, config->addr, stv6110x->i2c->nr);
		return -EREMOTEIO;
	}
	if (stv6110x->fe->ops.i2c_gate_ctrl)
		stv6110x->fe->ops.i2c_gate_ctrl(stv6110x->fe, 0);
	dprintk(10, "stv6110x_write_regN <\n");
	return 0;
}

static int stv6110x_init(struct dvb_frontend *fe)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	int ret;
	dprintk(10, "stv6110x_init> i2c-%d\n", stv6110x->i2c->nr);
	ret = stv6110x_write_init(stv6110x);
	if (ret < 0)
	{
		printk("stv6110x_init Initialization failed\n");
		return -1;
	}
	stv6110x->gain = 10;
//tdt
	stv6110x_set_refclock(fe, 2);
	dprintk(10, "stv6110x_init success<\n");
	return 0;
}

static s32 abssub(s32 a, s32 b)
{
	if (a > b)
		return a - b;
	else
		return b - a;
};

static int stv6110x_set_frequency(struct dvb_frontend *fe, u32 frequency)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	u32 divider, ref, p, presc, i, result_freq, vco_freq;
	s32 p_calc, p_calc_opt = 1000, r_div, r_div_opt = 0, p_val;
	dprintk(10, "%s: frequency = %d >\n", __func__, frequency);
	dprintk(20, "%s, freq=%d kHz, mclk=%d Hz\n", __func__,
			frequency, stv6110x->config->refclk);
	/* K = (Reference / 1000000) - 16 */
	stv6110x->stv6110x_regs[STV6110x_CTRL1] &= ~(0x1f << 3);
	stv6110x->stv6110x_regs[STV6110x_CTRL1] |=
		((((stv6110x->config->refclk / 1000000) - 16) & 0x1f) << 3);
	stv6110x->stv6110x_regs[STV6110x_CTRL2] &= ~0x0f;
	stv6110x->stv6110x_regs[STV6110x_CTRL2] |= (stv6110x->gain & 0x0f);
	if (frequency <= 1023000)
	{
		p = 1;
		presc = 0;
	}
	else if (frequency <= 1300000)
	{
		p = 1;
		presc = 1;
	}
	else if (frequency <= 2046000)
	{
		p = 0;
		presc = 0;
	}
	else
	{
		p = 0;
		presc = 1;
	}
	/* DIV4SEL = p*/
	stv6110x->stv6110x_regs[STV6110x_TNG1] &= ~(1 << 4);
	stv6110x->stv6110x_regs[STV6110x_TNG1] |= (p << 4);
	/* PRESC32ON = presc */
	stv6110x->stv6110x_regs[STV6110x_TNG1] &= ~(1 << 5);
	stv6110x->stv6110x_regs[STV6110x_TNG1] |= (presc << 5);
	p_val = (int)(1 << (p + 1)) * 10;/* P = 2 or P = 4 */
	for (r_div = 0; r_div <= 3; r_div++)
	{
		p_calc = (stv6110x->config->refclk / 100000);
		p_calc /= (1 << (r_div + 1));
		if ((abssub(p_calc, p_val)) < (abssub(p_calc_opt, p_val)))
			r_div_opt = r_div;
		p_calc_opt = (stv6110x->config->refclk / 100000);
		p_calc_opt /= (1 << (r_div_opt + 1));
	}
	ref = stv6110x->config->refclk / ((1 << (r_div_opt + 1)) * (1 << (p + 1)));
//	divider = (((frequency * 1000) + (ref >> 1)) / ref);
//TDT no rounding here
	divider = (((frequency * 1000)) / ref);
	/* RDIV = r_div_opt */
	stv6110x->stv6110x_regs[STV6110x_TNG1] &= ~(3 << 6);
	stv6110x->stv6110x_regs[STV6110x_TNG1] |= (((r_div_opt) & 3) << 6);
	/* NDIV_MSB = MSB(divider) */
	stv6110x->stv6110x_regs[STV6110x_TNG1] &= ~0x0f;
	stv6110x->stv6110x_regs[STV6110x_TNG1] |= (((divider) >> 8) & 0x0f);
	/* NDIV_LSB, LSB(divider) */
	stv6110x->stv6110x_regs[STV6110x_TNG0] = (divider & 0xff);
	/* CALVCOSTRT = 1 VCO Auto Calibration */
	stv6110x->stv6110x_regs[STV6110x_STAT1] |= 0x04;
	stv6110x_write_reg(stv6110x, STV6110x_CTRL2, stv6110x->stv6110x_regs[STV6110x_CTRL2]);
	stv6110x_write_reg(stv6110x, STV6110x_CTRL1, stv6110x->stv6110x_regs[STV6110x_CTRL1]);
// TDT from app
	stv6110x_write_reg(stv6110x, STV6110x_TNG1, stv6110x->stv6110x_regs[STV6110x_TNG1]);
	stv6110x->stv6110x_regs[STV6110x_CTRL3] |= 0x40;
	stv6110x_write_reg(stv6110x, STV6110x_CTRL3, stv6110x->stv6110x_regs[STV6110x_CTRL3]);
	for (i = 0; i < TRIALS; i++)
	{
		u8 reg;
		stv6110x_read_reg(stv6110x, STV6110x_TNG1, &reg);
		if (reg == stv6110x->stv6110x_regs[STV6110x_TNG1])
			break;
		msleep(1);
	}
	stv6110x->stv6110x_regs[STV6110x_CTRL3] &= ~0x40;
	stv6110x_write_reg(stv6110x, STV6110x_CTRL3, stv6110x->stv6110x_regs[STV6110x_CTRL3]);
//end tdt
	stv6110x_write_reg(stv6110x, STV6110x_TNG0, stv6110x->stv6110x_regs[STV6110x_TNG0]);
	stv6110x_write_reg(stv6110x, STV6110x_STAT1, stv6110x->stv6110x_regs[STV6110x_STAT1]);
	for (i = 0; i < TRIALS; i++)
	{
		stv6110x_read_reg(stv6110x, STV6110x_STAT1, &stv6110x->stv6110x_regs[STV6110x_STAT1]);
		if (!STV6110x_GETFIELD(STAT1_CALVCO_STRT, stv6110x->stv6110x_regs[STV6110x_STAT1]))
			break;
		msleep(1);
	}
	stv6110x_get_frequency(fe, &result_freq);
	vco_freq = divider * ((stv6110x->config->refclk / 1000) / ((1 << (r_div_opt + 1))));
	dprintk(20, "%s <, result:lo_freq=%d kHz, vco_frec=%d kHz\n", __func__,
			result_freq, vco_freq);
	dprintk(10, "%s <\n", __FUNCTION__);
	return 0;
}

static int stv6110x_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	dprintk(10, "stv6110x_get_frequency >\n");
	stv6110x_read_reg(stv6110x, STV6110x_TNG1, &stv6110x->stv6110x_regs[STV6110x_TNG1]);
	stv6110x_read_reg(stv6110x, STV6110x_TNG0, &stv6110x->stv6110x_regs[STV6110x_TNG0]);
	*frequency = (MAKEWORD16(STV6110x_GETFIELD(TNG1_N_DIV_11_8, stv6110x->stv6110x_regs[STV6110x_TNG1]),
							 STV6110x_GETFIELD(TNG0_N_DIV_7_0, stv6110x->stv6110x_regs[STV6110x_TNG0]))) * REFCLOCK_kHz;
	*frequency /= (1 << (STV6110x_GETFIELD(TNG1_R_DIV, stv6110x->stv6110x_regs[STV6110x_TNG1]) +
						 STV6110x_GETFIELD(TNG1_DIV4SEL, stv6110x->stv6110x_regs[STV6110x_TNG1])));
	*frequency >>= 2;
	dprintk(10, "%s frequency = %d <\n", __FUNCTION__, *frequency);
	return 0;
}

static int stv6110x_set_bandwidth(struct dvb_frontend *fe, u32 bandwidth)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	u8 r8, ret = 0x04;
	int i;
	dprintk(10, "%s: bandwidth = %d >\n", __func__, bandwidth);
	if ((bandwidth / 2) > 36000000) /*BW/2 max=31+5=36 mhz for r8=31*/
		r8 = 31;
	else if ((bandwidth / 2) < 5000000) /* BW/2 min=5Mhz for F=0 */
		r8 = 0;
	else /*if 5 < BW/2 < 36*/
		r8 = (bandwidth / 2) / 1000000 - 5;
	/* ctrl3, RCCLKOFF = 0 Activate the calibration Clock */
	/* ctrl3, CF = r8 Set the LPF value */
	stv6110x->stv6110x_regs[STV6110x_CTRL3] &= ~((1 << 6) | 0x1f);
	stv6110x->stv6110x_regs[STV6110x_CTRL3] |= (r8 & 0x1f);
	stv6110x_write_reg(stv6110x, STV6110x_CTRL3, stv6110x->stv6110x_regs[STV6110x_CTRL3]);
	/* stat1, CALRCSTRT = 1 Start LPF auto calibration*/
	stv6110x->stv6110x_regs[STV6110x_STAT1] |= 0x02;
	stv6110x_write_reg(stv6110x, STV6110x_STAT1, stv6110x->stv6110x_regs[STV6110x_STAT1]);
	i = 0;
	/* Wait for CALRCSTRT == 0 */
	while ((i < 10) && (ret != 0))
	{
		stv6110x_read_reg(stv6110x, STV6110x_STAT1, &ret);
		ret &= 0x02;
		mdelay(1); /* wait for LPF auto calibration */
		i++;
	}
	/* RCCLKOFF = 1 calibration done, desactivate the calibration Clock */
	stv6110x->stv6110x_regs[STV6110x_CTRL3] |= (1 << 6);
	stv6110x_write_reg(stv6110x, STV6110x_CTRL3, stv6110x->stv6110x_regs[STV6110x_CTRL3]);
	dprintk(10, "%s <\n", __FUNCTION__);
	return 0;
}

static int stv6110x_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	dprintk(10, "%s: >\n", __func__);
	stv6110x_read_reg(stv6110x, STV6110x_CTRL3, &stv6110x->stv6110x_regs[STV6110x_CTRL3]);
	*bandwidth = (STV6110x_GETFIELD(CTRL3_CF, stv6110x->stv6110x_regs[STV6110x_CTRL3]) + 5) * 2000000;
	dprintk(10, "%s: bandwidth = %d <\n", __func__, *bandwidth);
	return 0;
}

static int stv6110x_set_refclock(struct dvb_frontend *fe, u32 refclock)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	dprintk(10, "%s: refclock = %d >\n", __func__, refclock);
	/* setup divider */
	switch (refclock)
	{
		default:
		case 1:
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL2], CTRL2_CO_DIV, 0);
			break;
		case 2:
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL2], CTRL2_CO_DIV, 1);
			break;
		case 4:
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL2], CTRL2_CO_DIV, 2);
			break;
		case 8:
		case 0:
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL2], CTRL2_CO_DIV, 3);
			break;
	}
	stv6110x_write_reg(stv6110x, STV6110x_CTRL2, stv6110x->stv6110x_regs[STV6110x_CTRL2]);
	dprintk(10, "%s: <\n", __func__);
	return 0;
}

static int stv6110x_get_bbgain(struct dvb_frontend *fe, u32 *gain)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	dprintk(10, "%s: >\n", __func__);
	stv6110x_read_reg(stv6110x, STV6110x_CTRL2, &stv6110x->stv6110x_regs[STV6110x_CTRL2]);
	*gain = STV6110x_GETFIELD(CTRL2_BBGAIN, stv6110x->stv6110x_regs[STV6110x_CTRL2]);
	dprintk(10, "%s gain = %d<\n", __func__, *gain);
	return 0;
}

static int stv6110x_set_bbgain(struct dvb_frontend *fe, u32 gain)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	dprintk(10, "%s: gain = %d >\n", __func__, gain);
	STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL2], CTRL2_BBGAIN, gain & 0xf);
	stv6110x_write_reg(stv6110x, STV6110x_CTRL2, stv6110x->stv6110x_regs[STV6110x_CTRL2]);
	stv6110x->gain = gain;
	dprintk(10, "%s: <\n", __func__);
	return 0;
}

static int stv6110x_set_mode(struct dvb_frontend *fe, enum tuner_mode mode)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	int ret;
	dprintk(10, "%s: mode = %d >\n", __func__, mode);
	switch (mode)
	{
		case TUNER_SLEEP:
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL1], CTRL1_SYN, 0);
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL1], CTRL1_RX, 0);
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL1], CTRL1_LPT, 0);
			break;
		case TUNER_WAKE:
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL1], CTRL1_SYN, 1);
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL1], CTRL1_RX, 1);
			STV6110x_SETFIELD(stv6110x->stv6110x_regs[STV6110x_CTRL1], CTRL1_LPT, 1);
			break;
	}
	ret = stv6110x_write_reg(stv6110x, STV6110x_CTRL1, stv6110x->stv6110x_regs[STV6110x_CTRL1]);
	if (ret < 0)
	{
		printk("stv6110x_set_mode I/O Error\n");
		return -EIO;
	}
	dprintk(10, "%s: <\n", __func__);
	return 0;
}

static int stv6110x_sleep(struct dvb_frontend *fe)
{
	int err;
	dprintk(10, "%s: >\n", __func__);
	err = stv6110x_set_mode(fe, TUNER_SLEEP);
	dprintk(10, "%s: <\n", __func__);
	return err;
}

static int stv6110x_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	dprintk(10, "%s: >\n", __func__);
	stv6110x_read_reg(stv6110x, STV6110x_STAT1, &stv6110x->stv6110x_regs[STV6110x_STAT1]);
	if (STV6110x_GETFIELD(STAT1_LOCK, stv6110x->stv6110x_regs[STV6110x_STAT1]))
		*status = TUNER_PHASELOCKED;
	else
		*status = 0;
	dprintk(10, "%s: status = %d <\n", __func__, *status);
	return 0;
}

#if 0
static int stv6110x_get_state(struct dvb_frontend *fe,
							  enum tuner_param param,
							  struct tuner_state *state)
{
	switch (param)
	{
		case DVBFE_TUNER_FREQUENCY:
			stv6110x_get_frequency(fe, &state->frequency);
			break;
		case DVBFE_TUNER_TUNERSTEP:
			break;
		case DVBFE_TUNER_IFFREQ:
			break;
		case DVBFE_TUNER_BANDWIDTH:
			stv6110x_get_bandwidth(fe, &state->bandwidth);
			break;
		case DVBFE_TUNER_REFCLOCK:
			break;
		default:
			break;
	}
	return 0;
}

static int stv6110x_set_state(struct dvb_frontend *fe,
							  enum tuner_param param,
							  struct tuner_state *tstate)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	switch (param)
	{
		case DVBFE_TUNER_FREQUENCY:
			stv6110x_set_frequency(fe, stv6110x->frequency);
			tstate->frequency = stv6110x->frequency;
			break;
		case DVBFE_TUNER_TUNERSTEP:
			break;
		case DVBFE_TUNER_IFFREQ:
			break;
		case DVBFE_TUNER_BANDWIDTH:
			stv6110x_set_bandwidth(fe, stv6110x->bandwidth);
			tstate->bandwidth = stv6110x->bandwidth;
			break;
		case DVBFE_TUNER_REFCLOCK:
			stv6110x_set_refclock(fe, stv6110x->reference);
			tstate->refclock = stv6110x->reference;
			break;
		default:
			break;
	}
	return 0;
}
#endif

static int stv6110x_release(struct dvb_frontend *fe)
{
	struct stv6110x_state *stv6110x = fe->tuner_priv;
	fe->tuner_priv = NULL;
	kfree(stv6110x);
	return 0;
}

static struct dvb_tuner_ops stv6110x_ops =
{
	.info = {
		.name = "STV6110(A) Silicon Tuner",
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_step = 0,
	},

	.init = stv6110x_init,
//workaround for tuner failed, a frontend open does not allways wakeup the tuner
#ifndef FORTIS_HDBOX
	.sleep = stv6110x_sleep,
#endif

	.release = stv6110x_release
};

static struct tuner_devctl stv6110x_ctl =
{
	.tuner_init = stv6110x_init,
	.tuner_sleep = stv6110x_sleep,
	.tuner_set_mode = stv6110x_set_mode,
	.tuner_set_frequency = stv6110x_set_frequency,
	.tuner_get_frequency = stv6110x_get_frequency,
	.tuner_set_bandwidth = stv6110x_set_bandwidth,
	.tuner_get_bandwidth = stv6110x_get_bandwidth,
	.tuner_set_bbgain = stv6110x_set_bbgain,
	.tuner_get_bbgain = stv6110x_get_bbgain,
	.tuner_set_refclk = stv6110x_set_refclock,
	.tuner_get_status = stv6110x_get_status,
};

struct tuner_devctl *stv6110x_attach(struct dvb_frontend *fe,
									 const struct stv6110x_config *config,
									 struct i2c_adapter *i2c)
{
	struct stv6110x_state *stv6110x;
	stv6110x = kzalloc(sizeof(struct stv6110x_state), GFP_KERNEL);
	if (stv6110x == NULL)
		goto error;
	dprintk(10, "%s: i2c-%d -> 0x%p\n", __func__, i2c->nr, i2c);
	stv6110x->i2c = i2c;
	stv6110x->config = config;
	stv6110x->devctl = &stv6110x_ctl;
	stv6110x->fe = fe;
	fe->tuner_priv = stv6110x;
	fe->ops.tuner_ops = stv6110x_ops;
	dprintk(10, "%s: Attaching STV6110x \n", __func__);
	return stv6110x->devctl;
error:
	kfree(stv6110x);
	return NULL;
}
EXPORT_SYMBOL(stv6110x_attach);

