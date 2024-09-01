/*
  Driver for ST STV0288 demodulator
  Copyright (C) 2006 Georg Acher, BayCom GmbH, acher (at) baycom (dot) de
                     for Reel Multimedia
  Copyright (C) 2008 TurboSight.com, Bob Liu <bob@turbosight.com>
  Copyright (C) 2008 Igor M. Liplianin <liplianin@me.by>
                     Removed stb6000 specific tuner code and revised some
                     procedures.

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
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <asm/div64.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <asm/io.h>

#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"
#include "stv0288.h"

#include "lnb.h"
#include "frontend_platform.h"  // for dprintk definition

extern short paramDebug;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[stv0288] "

int tuner_write_for_agc_stv0288( struct dvb_frontend *fe );

#if DVB_API_VERSION < 5
static struct dvbfe_info dvbs_info =
{
	.name = "ST STV0288 DVB-S",
	.delivery = DVBFE_DELSYS_DVBS,
	.delsys =
	{
		.dvbs.modulation = DVBFE_MOD_QPSK,
		.dvbs.fec        = DVBFE_FEC_1_2
		                 | DVBFE_FEC_2_3
		                 | DVBFE_FEC_3_4
		                 | DVBFE_FEC_5_6
		                 | DVBFE_FEC_7_8
		                 | DVBFE_MOD_QPSK
		                 | DVBFE_FEC_AUTO
	},
	.frequency_min       =  950000,
	.frequency_max       = 2150000,
	.frequency_step      = 1000,
	.frequency_tolerance = 0,
	.symbol_rate_min     = 1000000,
	.symbol_rate_max     = 45000000
};
#endif

#define STATUS_BER 0

#define ABS(X) ((X)<0 ? (-(X)) : (X))

#define BYTES2WORD(X,Y) ((X<<8)+(Y))

#define LSB(X) ((X & 0xFF))
#define MSB(Y) ((Y>>8)& 0xFF)
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#ifndef TRUE
#define TRUE 1
#endif

static int stv0288_writeregI(struct stv0288_state *state, u8 reg, u8 data)
{
	int ret;
	u8 buf[] = { reg, data };
	struct i2c_msg msg =
	{
		.addr = state->config->demod_address,
		.flags = 0,
		.buf = buf,
		.len = 2
	};

	dprintk(150, "%s reg = 0x%02x data = 0x%02x\n", __func__, reg, data);
	ret = i2c_transfer(state->i2c, &msg, 1);

	if (ret != 1)
	{
		dprintk(1, "%s: writereg error (reg == 0x%02x, val == 0x%02x, ret == %i)\n", __func__, reg, data, ret);
	}
	return (ret != 1) ? -EREMOTEIO : 0;
}

static int stv0288_write(struct dvb_frontend *fe, u8 *buf, int len)
{
	struct stv0288_state *state = fe->demodulator_priv;

	if (len != 2)
	{
		return -EINVAL;
	}
	return stv0288_writeregI(state, buf[0], buf[1]);
}

static u8 stv0288_readreg(struct stv0288_state *state, u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };
	struct i2c_msg msg[] =
	{
		{
			.addr  = state->config->demod_address,
			.flags = 0,
			.buf   = b0,
			.len   = 1
		},
		{
			.addr  = state->config->demod_address,
			.flags = I2C_M_RD,
			.buf   = b1,
			.len   = 1
		}
	};
	ret = i2c_transfer(state->i2c, msg, 2);

	if (ret != 2)
	{
		dprintk(1, "%s: readreg error (reg == 0x%02x, ret == %i)\n", __func__, reg, ret);
	}
	else
	{
		dprintk(150, "%s: reg=0x%02x , result=0x%02x\n",__func__, reg, b1[0]);
	}
	return b1[0];
}

static int stv0288_send_diseqc_msg(struct dvb_frontend *fe, struct dvb_diseqc_master_cmd *m)
{
	struct stv0288_state *state = fe->demodulator_priv;
	int i;

	dprintk(100, "%s >\n", __func__);
	stv0288_writeregI(state, 0x09, 0);
	msleep(30);
	stv0288_writeregI(state, 0x05, 0x16);

	for (i = 0; i < m->msg_len; i++)
	{
		if (stv0288_writeregI(state, 0x06, m->msg[i]))
		{
			return -EREMOTEIO;
		}
		msleep(12);
	}
	return 0;
}

static int stv0288_send_diseqc_burst(struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
{
	struct stv0288_state *state = fe->demodulator_priv;

	dprintk(100, "%s >\n", __func__);

	if (stv0288_writeregI(state, 0x05, 0x16))  /* burst mode */
	{
		return -EREMOTEIO;
	}
	if (stv0288_writeregI(state, 0x06, burst == SEC_MINI_A ? 0x00 : 0xff))
	{
		return -EREMOTEIO;
	}
	if (stv0288_writeregI(state, 0x06, 0x12))
	{
		return -EREMOTEIO;
	}
	return 0;
}

static int stv0288_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
{
	struct stv0288_state *state = fe->demodulator_priv;

	switch (tone)
	{
		case SEC_TONE_ON:
		{
			if (stv0288_writeregI(state, 0x05, 0x10))  /* burst mode */
			{
				return -EREMOTEIO;
			}
			return stv0288_writeregI(state, 0x06, 0xff);
		}
		case SEC_TONE_OFF:
		{
			if (stv0288_writeregI(state, 0x05, 0x13))  /* burst mode */
			{
				return -EREMOTEIO;
			}
			return stv0288_writeregI(state, 0x06, 0x00);
		}
		default:
		{
			return -EINVAL;
		}
	}
}

static int stv0288_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t volt)
{
	struct stv0288_state *state = fe->demodulator_priv;

	dprintk(10, "%s: %s\n", __func__, volt == SEC_VOLTAGE_13 ? "SEC_VOLTAGE_13" : volt == SEC_VOLTAGE_18 ? "SEC_VOLTAGE_18" : "??");
	return state->equipment.lnb_set_voltage(state->lnb_priv, fe, volt);
}

static unsigned char register_[] =
{
//	  00   01   02   03   04   05   06   07   08   09   0A   0B   0C   0D   0E   0F
	0x11,0x15,0x20,0x8E,0x8E,0x12,0x00,0x20,0x00,0x00,0x04,0x00,0x00,0x00,0xC1,0x54, //00
	0x40,0x7A,0x03,0x48,0x84,0xC5,0xB8,0x9C,0x00,0xA6,0x88,0x8F,0xF0,0x00,0x80,0x1A, //01
	0x0B,0x54,0xFF,0x01,0x9A,0x7F,0x00,0x00,0x46,0x66,0x90,0xFA,0xD9,0x02,0xB1,0x00, //02
	0x00,0x1E,0x14,0x0F,0x09,0x0C,0x05,0x2F,0x16,0xBC,0x00,0x13,0x11,0x30,0x00,0x00, //03
	0x63,0x04,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xD1,0x33,0x00,0x00,0x00, //04
	0x10,0x36,0x21,0x94,0xB2,0x29,0x64,0x2B,0x54,0x86,0x00,0x9B,0x08,0x7F,0xFF,0x8D, //05
	0x82,0x82,0x82,0x02,0x02,0x02,0x82,0x82,0x82,0x82,0x38,0x0C,0x00,0x00,0x00,0x00, //06
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //07
	0x00,0x00,0x3F,0x3F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //08
	0x00,0x00,0x00,0x00,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //09
	0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0A
	0xB8,0x3A,0x10,0x82,0x80,0x82,0x82,0x82,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0B
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0C
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0D
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0E
	0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0F
};

static int stv0288_init(struct dvb_frontend *fe)
{
	struct stv0288_state *state = fe->demodulator_priv;
	int i;

	for (i = 0x00; i <= 0x1c; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0x1e; i <= 0x47; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0x4a; i <= 0x4c; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0x50; i <= 0x76; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0x81; i <= 0x85; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0x88; i <= 0x8C; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0x90; i <= 0x94; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0x97; i <= 0x97; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0xA0; i <= 0xA1; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0xB0; i <= 0xB9; i++) stv0288_writeregI(state,i,register_[i]);
	for (i = 0xF0; i <= 0xF2; i++) stv0288_writeregI(state,i,register_[i]);
	tuner_write_for_agc_stv0288(fe);
	return 0;
}

static int stv0288_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct stv0288_state *state = fe->demodulator_priv;

	u8 sync = stv0288_readreg(state, 0x24);
	if (sync == 255)
	{
		sync = 0;
	}
	dprintk(50, "%s : FE_READ_STATUS : VSTATUS: 0x%02x\n", __func__, sync);
	*status = 0;

	if ((sync & 0x08) == 0x08)
	{
		*status |= FE_HAS_LOCK;
		dprintk(100, "stv0288 has locked\n");
	}
	return 0;
}

static int stv0288_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct stv0288_state *state = fe->demodulator_priv;

	if (state->errmode != STATUS_BER)
	{
		return 0;
	}
	*ber = (stv0288_readreg(state, 0x26) << 8)
	     | 	stv0288_readreg(state, 0x27);
	dprintk(50, "stv0288_read_ber %d\n", *ber);
	return 0;
}

static int stv0288_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct stv0288_state *state = fe->demodulator_priv;

	s32 signal =  0xffff - ((stv0288_readreg(state, 0x10) << 8));

	signal = signal * 5 / 4;
	*strength = (signal > 0xffff) ? 0xffff : (signal < 0) ? 0 : signal;
	dprintk(50, "stv0288_read_signal_strength %d\n", *strength);
	return 0;
}

static int stv0288_sleep(struct dvb_frontend *fe)
{
	struct stv0288_state *state = fe->demodulator_priv;

	stv0288_writeregI(state, 0x41, 0x84);
	state->initialised = 0;

	return 0;
}

static int stv0288_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct stv0288_state *state = fe->demodulator_priv;

	s32 xsnr = ((stv0288_readreg(state, 0x2d) << 8)
	         |   stv0288_readreg(state, 0x2e));
	/*xsnr = 3 * (xsnr - 0xa100);*/
	*snr = (xsnr * 100 / 8900) * 65536 / 100;
	// *snr = (xsnr > 0xffff) ? 0xffff : (xsnr < 0) ? 0 : xsnr;
	dprintk(50, "stv0288_read_snr %d\n", *snr);
	return 0;
}

static int stv0288_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	struct stv0288_state *state = fe->demodulator_priv;

	if (state->errmode != STATUS_BER)
	{
		return 0;
	}
	*ucblocks = (stv0288_readreg(state, 0x26) << 8)
	          |  stv0288_readreg(state, 0x27);
	dprintk(50, "stv0288_read_ber %d\n", *ucblocks);
	return 0;
}

static int stv0288_enable_high_lnb_voltage(struct dvb_frontend *fe, long arg)
{
	/* E2 needs this */
	dprintk(50, "%s\n", __FUNCTION__);
	return 0;
}

static int stv0288_get_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct stv0288_state *state = fe->demodulator_priv;

	p->frequency = state->tuner_frequency;
	p->u.qpsk.fec_inner = state->fec_inner;
	p->u.qpsk.symbol_rate = state->symbol_rate;
	return 0;
}

static int demod_288_algo(struct dvb_frontend *fe, unsigned long freq, unsigned long baud);

static int stv0288_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *dfp)
{
	struct stv0288_state *state = fe->demodulator_priv;

	/* konfetti: for hack fix for 22000 */
	state->symbol_rate = dfp->u.qpsk.symbol_rate;

	demod_288_algo(fe, dfp->frequency, dfp->u.qpsk.symbol_rate);

	state->tuner_frequency = dfp->frequency;
	state->fec_inner = FEC_AUTO;
	state->symbol_rate = dfp->u.qpsk.symbol_rate;
	return 0;
}

static int stv0288_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct stv0288_state *state = fe->demodulator_priv;

	if (enable)
	{
		stv0288_writeregI(state, 0x01, 0xb5);
	}
	else
	{
		stv0288_writeregI(state, 0x01, 0x35);
	}
	udelay(1);
	return 0;
}

static void stv0288_release(struct dvb_frontend *fe)
{
	struct stv0288_state *state = fe->demodulator_priv;

	kfree (state->config);
	kfree(state);
}

#if DVB_API_VERSION < 5
static int stv0288_get_info(struct dvb_frontend *fe, struct dvbfe_info *fe_info)
{
  dprintk (10, "%s\n", __FUNCTION__);

	switch (fe_info->delivery)
	{
		case DVBFE_DELSYS_DVBS:
		{
			dprintk (10, "%s(DVBS)\n", __FUNCTION__);
			memcpy (fe_info, &dvbs_info, sizeof (dvbs_info));
			break;
		}
		default:
		{
			dprintk (0, "%s() invalid arg\n", __FUNCTION__);
			return -EINVAL;
		}
	}
	return 0;
}

/* TODO: The hardware does DSS too, how does the kernel demux handle this? */
static int stv0288_get_delsys(struct dvb_frontend *fe, enum dvbfe_delsys *fe_delsys)
{
	dprintk (100 "%s >\n", __FUNCTION__);
	*fe_delsys = DVBFE_DELSYS_DVBS;
	return 0;
}

/* TODO: is this necessary? */
static enum dvbfe_algo stv0288_get_algo(struct dvb_frontend *fe)
{
	dprintk (100, "%s >\n", __FUNCTION__);
	return DVBFE_ALGO_SW;
}
#else
static int stv0288_get_property(struct dvb_frontend *fe, struct dtv_property* tvp)
{
	/* get delivery system info */
	if (tvp->cmd==DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBS:
			case SYS_DSS:
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

/* **********************************************
 *
 */
static int stv0288_get_tune_settings (struct dvb_frontend *fe, struct dvb_frontend_tune_settings *fetunesettings)
{
	struct stv0288_state *state = fe->demodulator_priv;

	dprintk(100, "%s >\n", __FUNCTION__);

/* FIXME: Hab das jetzt mal eingebaut, da bei SET_FRONTEND DVB-S2 nicht gehandelt wird.
 * Im Prinzip setze ich hier die Werte aus SET_FRONTEND (siehe dvb-core_stv0288) mal von min_delay
 * abgesehen.
 * DIES MUSS MAL BEOBACHTET WERDEN
 */
	fetunesettings->step_size = state->symbol_rate / 16000;
	fetunesettings->max_drift = state->symbol_rate / 2000;
	fetunesettings->min_delay_ms = 500; // For pilot auto tune

	dprintk(100, "%s < \n", __FUNCTION__);
	return 0;
}

static struct dvb_frontend_ops stv0288_ops =
{
	.info =
	{
		.name                  = "ST STV0288 DVB-S",
		.type                  = FE_QPSK,
		.frequency_min		   = 950000,
		.frequency_max         = 2150000,
		.frequency_stepsize    = 1000,  /* kHz for QPSK frontends */
		.frequency_tolerance   = 0,
		.symbol_rate_min       =  1000000,
		.symbol_rate_max       = 45000000,
		.symbol_rate_tolerance = 500,  /* ppm */
		.caps                  = FE_CAN_FEC_1_2
		                       | FE_CAN_FEC_2_3
		                       | FE_CAN_FEC_3_4
		                       | FE_CAN_FEC_5_6
		                       | FE_CAN_FEC_7_8
		                       | FE_CAN_QPSK
		                       | FE_CAN_FEC_AUTO
	},
	.release                   = stv0288_release,
	.init                      = stv0288_init,
	.sleep                     = stv0288_sleep,
	.write                     = stv0288_write,
	.i2c_gate_ctrl             = stv0288_i2c_gate_ctrl,
	.read_status               = stv0288_read_status,
	.read_ber                  = stv0288_read_ber,
	.read_signal_strength      = stv0288_read_signal_strength,
	.read_snr                  = stv0288_read_snr,
	.read_ucblocks             = stv0288_read_ucblocks,
	.diseqc_send_master_cmd    = stv0288_send_diseqc_msg,
	.diseqc_send_burst         = stv0288_send_diseqc_burst,
	.set_tone                  = stv0288_set_tone,
	.set_voltage               = stv0288_set_voltage,
	.enable_high_lnb_voltage   = stv0288_enable_high_lnb_voltage,
	.set_frontend              = stv0288_set_frontend,
	.get_frontend              = stv0288_get_frontend,
	.get_tune_settings         = stv0288_get_tune_settings,
#if DVB_API_VERSION < 5
	.get_info                  = stv0288_get_info,
	.get_delsys                = stv0288_get_delsys,
	.get_frontend_algo         = stv0288_get_algo,
#else
	.get_property 		       = stv0288_get_property,
#endif
};

static unsigned char t_buff[6];
static long tp_SymbolRate_Bds = 0;
static long tp_Frequency_Khz = 0;
static FE_288_SIGNALTYPE_t tp_signalType = NOAGC1;

static int pll_tuner_write (struct dvb_frontend *fe, unsigned char *buffer, int bufflen)
{
	struct stv0288_state *priv = fe->demodulator_priv;
	int /*address=0x1,*/ ret = 0;
	//register_[address]=0x95;

	struct i2c_msg msg =
	{
		.addr = 0x60,
		.flags = 0,
		.buf = buffer,
		.len = bufflen
	};
	stv0288_writeregI(priv, 0x01, 0x95); //stv0288_writeregI(stv0288,address,register_[address]);
	{
		int i;

		for (i = 0; i < bufflen; i++)
		{
			dprintk(150, " 0x%02x", buffer[i]);
		}
		dprintk(150, "\n");
	}
	ret = i2c_transfer(priv->i2c, &msg, 1); //i2c_write( tuner, (const char*)buffer, bufflen );

	//register_[address]=0x15;
	stv0288_writeregI(priv, 0x01, 0x15); //stv0288_writeregI(stv0288,address,register_[address]);
	return ret;
}

static void pll_calculate_byte( int cutoff, unsigned char *byte )
{
	int data, pd2, pd3, pd4, pd5;

	dprintk(100, "%s >\n", __func__);

	//lpf
	data = (int)((cutoff / 1000) / 2 - 2);
	pd2 = (data >> 1) & 0x04;
	pd3 = (data << 1) & 0x08;
	pd4 = (data << 2) & 0x08;
	pd5 = (data << 4) & 0x10;
	*(byte + 2) &= 0xE7;
	*(byte + 3) &= 0xF3;
	*(byte + 2) |= (pd5|pd4);
	*(byte + 3) |= (pd3|pd2);
}

static int pll_calculate_step( unsigned char byte4 )
{
	int R10;
	int Nref;
	int pll_step;
	R10 = byte4 & 0x03;

	dprintk(100, "%s >\n", __func__);

	switch(R10)
	{
		case 0:
		{
			Nref = 4 * 1;  // step_size = 1 MHz
			break;
		}
		case 1:
		{
			Nref = 4 * 2;  // step_size = 500 KHz
			break;
		}
		case 2:
		{
			Nref = 4 * 4;  // step_size = 250 KHz
			break;
		}
		default:
		{
			Nref = 4 * 2;
			break;
		}
	}
	pll_step = 4000 / Nref;  /* 4 Mhz */
	return (pll_step);
}

static void pll_calculate_divider_byte(int freq_khz)
{
	int data;

	dprintk(100, "%s >\n", __func__);

	data = (int) ((freq_khz) / pll_calculate_step(t_buff[2]));
	t_buff[0] = (int) ((data >> 8) & 0x7f);
	t_buff[1] = (int) (data & 0xff);
}

static void pll_calculate_lpf_cutoff(long baud)
{
	int lpf = 0;

	dprintk(100, "%s >\n", __func__);

	/* BAUD V.S. LPF Cutoff */
	if ((45000000 >= baud) && (baud >= 39000000))
		lpf = 30000;  // LPF = 30MHz
	if ((39000000 > baud) && (baud >= 35000000))
		lpf = 28000;  // LPF = 28MHz
	if ((35000000 > baud) && (baud >= 30000000))
		lpf = 26000;  // LPF = 26MHz
	if ((30000000 > baud) && (baud >= 26000000))
		lpf = 24000;  // LPF = 24MHz
	if ((26000000 > baud) && (baud >= 23000000))
		lpf = 22000;  // LPF = 22MHz
	if ((23000000 > baud) && (baud >= 21000000))
		lpf = 20000;  // LPF = 20MHz
	if ((21000000 > baud) && (baud >= 19000000))
		lpf = 18000;  // LPF = 18MHz
	if ((19000000 > baud) && (baud >= 18000000))
		lpf = 16000;  // LPF = 16MHz
	if ((18000000 > baud) && (baud >= 17000000))
		lpf = 14000;  // LPF = 14MHz
	if ((17000000 > baud) && (baud >= 16000000))
		lpf = 12000;  // LPF = 12MHz
	if (16000000 > baud)
		lpf = 10000;  // LPF = 10MHz

	pll_calculate_byte(lpf, t_buff);
}

static void pll_setdata( struct dvb_frontend *fe )
{
	long i;
	unsigned char data_4,data_5;
	unsigned char buffer[4];

	dprintk(100, "%s>\n", __func__);

	// TM = 0, LPF = 4MHz
	//adjustting of VCO & LPF
	data_4 = t_buff[2];
	data_5 = t_buff[3];

	t_buff[2] &= 0xE3;  // TM = 0, LPF = 4MHz
	t_buff[3] &= 0xF3;  // LPF = 4MHz

	for (i = 0; i < 4; i++)
	{
		buffer[i] = t_buff[i];
	}
	pll_tuner_write(fe, buffer, 4);
	//TM = 1,LPF = 4MHz
	t_buff[2] |= 0x04;
	buffer[0] = t_buff[2];
	pll_tuner_write(fe, buffer, 1);
	msleep(6);	//6ms
	msleep(20);	//20ms
	//TM = 1, LPF setting
	t_buff[2] = data_4;
	t_buff[3] = data_5;

	for (i = 0; i < 2; i++)
	{
		buffer[i] = t_buff[i + 2];
	}
	pll_tuner_write(fe, buffer, 2);
}

static void pll_calculate_vco(int freq)
{
	long div = 1, ba = 5;

	dprintk(10, "%s>\n", __func__);

	if (950000 <= freq && freq <1065000)
	{
		div = 1; ba = 6;
	}
	else if(1065000 <= freq && freq < 1170000)
	{
		div = 1; ba = 7;
	}
	else if(1170000 <= freq && freq < 1300000)
	{
		div = 0; ba = 1;
	}
	else if(1300000 <= freq && freq < 1445000)
	{
		div = 0; ba = 2;
	}
	else if(1445000 <= freq && freq < 1607000)
	{
		div = 0; ba = 3;
	}
	else if(1607000 <= freq && freq < 1778000)
	{
		div = 0; ba = 4;
	}
	else if(1778000 <= freq && freq < 1942000)
	{
		div = 0; ba = 5;
	}
	else if(1942000 <= freq && freq < 2150000)
	{
		div = 0; ba = 6;
	}
	t_buff[3] &= 0xFD;	t_buff[3] |= (div << 1);
	t_buff[3] &= 0x1F;	t_buff[3] |= (ba << 5);
}

static void pll_init(void)
{
//orig 0x00, 0x02, 0xe1, 0xa2,

	t_buff[0] = 0x00;
	t_buff[1] = 0x00;
	//t_buff[2] = 0xe5; /* bit[7] =1, charge pump = '11', TM = 1, step = 500Khz */
	t_buff[2] = 0xFD;
	t_buff[3] = 0x00;
}

static int pll_set_freq( struct dvb_frontend *fe, long freq, long baud) //kHz, ksps
{
	//if baud is unknown(blind search mode),
	//set the LPF cutoff should be widest.

	dprintk(10, "%s %ld b %ld\n", __func__, freq, baud);

	if (baud == 0)
	{
		baud = 45000;
	}
//	{
	pll_init();
	pll_calculate_divider_byte(freq);
	pll_calculate_lpf_cutoff(baud * 1000);
	pll_calculate_vco(freq);
//	}
	pll_setdata(fe);
	return 0;
}

static long demod_288_tuner_getfreq(void)
{
	long data;
	long freq;

	dprintk(100, "%s >\n", __func__);

	data = ( (t_buff[0] & 0x1F) << 8 ) | t_buff[1];
	freq = data *(long)(pll_calculate_step(*(t_buff + 2)));

	dprintk(100, "%s < %ld\n", __func__, freq);
	return freq;
}

static int demod_288_get_iq_wiring(void)
{
	return 1;
}

static void demod_288_froze_lock(struct stv0288_state *state, int flag,unsigned char *register_)
{
	int F288_FROZE_LOCK;
	int address = 0x50;

	F288_FROZE_LOCK = flag;

	dprintk(10, "%s: flag = %d\n",__func__, flag);

	register_[address] &= 0xDF;
	register_[address] |= (F288_FROZE_LOCK << 5);
	stv0288_writeregI(state,address,register_[address]);
}

static long BinaryFloatDiv(long n1, long n2, int precision)
{
	int i = 0;
	long result = 0;
	/* division de N1 par N2 avec N1 < N2 */
	while (i <= precision) /* n1>0 */
	{
		if (n1 < n2)
		{
			result <<= 1;
			n1 <<= 1;
		}
		else
		{
			result=(result << 1) + 1;
			n1=(n1 - n2) << 1;
		}
		i++;
	}
	return result;
}

static long demod_xtal(void)
{
	return 4000000;
}

static long demod_288_get_mclk_freq(unsigned char *register_) //Hz
{
	long mclk_Hz;		/* master clock frequency (Hz) */
	long pll_divider;	/* pll divider */
	long pll_selratio;	/* 4 or 6 ratio */
	long pll_bypass;	/* pll bypass */
	long ExtClk_Hz;

    dprintk(100, "%s >\n", __func__);

	ExtClk_Hz=demod_xtal();
	pll_divider=register_[0x40] + 1;
	pll_selratio=( ((register_[0x41] >> 2) & 0x01) ? 4 : 6);
	pll_bypass=register_[0x41] & 0x01;

	if (pll_bypass)
	{
		mclk_Hz=ExtClk_Hz;
	}
	else
	{
		mclk_Hz=(ExtClk_Hz*pll_divider)/pll_selratio;
	}
	dprintk(100, "%s < F = %ld\n", __func__, mclk_Hz);
	return mclk_Hz;
}

static long demod_288_set_symbolrate(struct stv0288_state *state, long SymbolRate, unsigned char *register_)//Hz
{
	long MasterClock_Hz;
	long temp;
	int address;
	/* warning : symbol rate is set to Masterclock/2 before setting the real value !!!!! */
	int F288_SYMB_FREQ_HSB = 0x80;
	int F288_SYMB_FREQ_MSB = 0x00;
	int F288_SYMB_FREQ_LSB = 0x00;

	dprintk(100, "%s > sr=%ld\n", __func__, SymbolRate);

	MasterClock_Hz = demod_288_get_mclk_freq(register_); //Hz
	//	/*
	//	** in order to have the maximum precision, the symbol rate entered into
	//	** the chip is computed as the closest value of the "true value".
	//	** In this case, the symbol rate value is rounded (1 is added on the bit
	//	** below the LSB )
	//	*/
	temp = BinaryFloatDiv(SymbolRate, MasterClock_Hz, 20);

	address = 0x28;
	register_[address] = F288_SYMB_FREQ_HSB;
	stv0288_writeregI(state,address,register_[address]);

	address += 1;
	register_[address]=F288_SYMB_FREQ_MSB;
	stv0288_writeregI(state,address,register_[address]);

	address += 1;
	register_[address]=F288_SYMB_FREQ_LSB;
	stv0288_writeregI(state,address,register_[address]);

/* konfetti: hack for sr 22000 */
	if (state->symbol_rate == 22000000)
	{
	/* SymbolRate 525163520 */
		F288_SYMB_FREQ_HSB = 0x38;
		F288_SYMB_FREQ_MSB = 0x51;
		F288_SYMB_FREQ_LSB = 0x1e;
/* ->my tester told with those values _all_ (not only sr 22000) channels
 * works. so this seem to be magic values :D
 */
	}
	else
	{
		F288_SYMB_FREQ_HSB=(temp >> 12) & 0xFF;
		F288_SYMB_FREQ_MSB=(temp >> 4)  & 0xFF;
	 	F288_SYMB_FREQ_LSB=(temp)       & 0xFF;
    	}
	address = 0x28;
	register_[address]=F288_SYMB_FREQ_HSB;
	stv0288_writeregI(state,address,register_[address]);

	address += 1;
	register_[address]=F288_SYMB_FREQ_MSB;
	stv0288_writeregI(state,address,register_[address]);

	address += 1;
	register_[address]=F288_SYMB_FREQ_LSB;
	stv0288_writeregI(state,address,register_[address]);

	address = 0x22; register_[address] = 0;
	stv0288_writeregI(state,address,register_[address]);
	address += 1; register_[address] = 0;
	stv0288_writeregI(state,address,register_[address]);
	return SymbolRate;
}

static void demod_288_set_derot_freq( struct stv0288_state *state, long DerotFreq_Hz,unsigned char *register_)
{
	int  address;
	long temp;
	long MasterClock_Hz;
	int  F288_CARRIER_FREQUENCY_MSB;
	int  F288_CARRIER_FREQUENCY_LSB;

	dprintk(100, "%s >\n", __func__);

	MasterClock_Hz=demod_288_get_mclk_freq(register_); //Hz
	temp=(long)(DerotFreq_Hz / (long)(MasterClock_Hz / 65536L));

	F288_CARRIER_FREQUENCY_MSB=(temp >> 8) & 0xFF;
	F288_CARRIER_FREQUENCY_LSB=temp & 0xFF;

	address = 0x2B;
	register_[address]=F288_CARRIER_FREQUENCY_MSB;
	stv0288_writeregI(state,address,register_[address]);

	address += 1;
	register_[address]=F288_CARRIER_FREQUENCY_LSB;
	stv0288_writeregI(state,address,register_[address]);
}

static void demod_288_set_frequency_offset_detector(struct stv0288_state *state, int flag,unsigned char *register_)
{
	int address = 0x15;

	dprintk(100, "%s >\n", __func__);

	if (flag)
	{
		register_[address] |= 0x80;
	}
	else
	{
		register_[address] &= 0x7F;
	}
	stv0288_writeregI(state,address,register_[address]);
}

static long demod_288_calc_derot_freq(struct stv0288_state *state, unsigned char *register_)
{
	long derotmsb;
	long derotlsb;
	long fm;
	long dfreq;
	long Itmp;
	int address;

	dprintk(100, "%s >\n", __func__);

	address = 0x2B;
	register_[address] = stv0288_readreg(state,address);
	address += 1;
	register_[address] = stv0288_readreg(state,address);

	derotmsb=register_[0x2B];
	derotlsb=register_[0x2C];
	fm=demod_288_get_mclk_freq(register_);

	Itmp = (derotmsb << 8) + derotlsb;
	if (Itmp > 0x10000 / 2)
	{ //2's complement
		Itmp = 0x10000 - Itmp;
		Itmp *= -1;
	}
	dfreq = Itmp * (fm / 10000L);
	dfreq = dfreq / 65536L;
	dfreq *= 10;

	dprintk(10, "%s < F = %ld\n", __func__, dfreq);
	return dfreq;
}

static long demod_288_get_derot_freq(struct stv0288_state *state, unsigned char *register_)
{
	int address = 0x2B;

	dprintk(100, "%s >\n", __func__);
	register_[address] = stv0288_readreg(state,address);
	return demod_288_calc_derot_freq(state,register_);
}

static long demod_288_calc_symbolrate(unsigned char *register_)
{
	long Ltmp;
	long Ltmp2;
	long Mclk;
	long Hbyte;
	long Mbyte;
	long Lbyte;
	int address = 0x28;

	dprintk(100, "%s >\n", __func__);

	Mclk = (long)(demod_288_get_mclk_freq(register_) / 4096L);  /* MasterClock * 10 / 2^20 */
	Hbyte=register_[address];
	Mbyte=register_[address + 1];
	Lbyte=register_[address + 2];
	Ltmp=((Hbyte << 12) + (Mbyte << 4)) / 16;
	Ltmp *= Mclk;
	Ltmp /= 16;
	Ltmp2 = (long)(Lbyte * Mclk) / 256;
	//tidelgo Ltmp2=(long)(Lbyte * Mclk) / 256 / 16; //V0.03[xxxxxx]
	Ltmp += Ltmp2;
	dprintk(100, "%s < sr = %ld\n", __func__, Ltmp);
	return Ltmp;
}

static long demod_288_get_symbolrate(struct stv0288_state *state, unsigned char *register_) //[sps]
{
	int address = 0x2A;

	dprintk(100, "%s >\n", __func__);

	register_[address] = stv0288_readreg(state,address);

	address -= 1;
	register_[address] = stv0288_readreg(state,address);

	address -= 1;
	register_[address] = stv0288_readreg(state,address);
	return demod_288_calc_symbolrate(register_);
}

static void demod_288_start_coarse_algorithm(struct stv0288_state *state, int flag,unsigned char *register_)
{
	int address = 0x50;

	dprintk(100, "%s >\n", __func__);

	register_[address] &= 0xFE;
	register_[address]|=(flag);
	stv0288_writeregI(state,address,register_[address]);
}

static long demod_288_coarse(struct stv0288_state *state, unsigned char *register_, long *Offset_Khz)
{
	long symbolrate_Bds = 0;

	int F288_AUTOCENTRE = 1;
	int F288_FINE = 0;
	int F288_COARSE = 1;
	int address = 0x50;

	dprintk(100, "%s >\n", __func__);

	register_[address] &= 0xF8;
	register_[address]|=(F288_AUTOCENTRE << 2);
	register_[address]|=(F288_FINE << 1);
	register_[address]|=(F288_COARSE << 0);
	stv0288_writeregI(state,address, register_[address]);
	msleep(50);
	symbolrate_Bds=demod_288_get_symbolrate(state,register_);
	*Offset_Khz=demod_288_get_derot_freq(state,register_);
	demod_288_start_coarse_algorithm(state, 0 ,register_); /* stop coarse algorithm */

	dprintk(20, "%s: symbolrate_Bds=%ld / *Offset_Khz=%ld)\n", __func__, symbolrate_Bds, *Offset_Khz);
	return symbolrate_Bds;
}

static void demod_288_start_fine_algorithm(struct stv0288_state *state, int flag, unsigned char *register_)
{
	int address = 0x50;

	dprintk(100, "%s >\n", __func__);
	register_[address] &= 0xFD;
	register_[address] |= (flag << 1);
	stv0288_writeregI(state, address, register_[address]);
}

static void demod_288_fine(struct stv0288_state *state, long Symbolrate_Bds, long Known, unsigned char *register_)
{
	int address;
	long i = 0;
	long fmin = 0;
	long fmax = 0;
	long fmid = 0;
	long MasterClock_Hz;
	unsigned char F288_FINE;
	int flag;

	dprintk(100, "%s >\n", __func__);

	MasterClock_Hz=demod_288_get_mclk_freq(register_);

	fmid = ((Symbolrate_Bds / 1000) * 32768) / (MasterClock_Hz / 1000);
	if (Known)
	{
		/* +/- 1% search range */
		fmin = (fmid * 99) / 100;
		fmax = (fmid * 101) / 100;
	}
	else
	{
		fmin = (fmid * 85) / 100;
		fmax = (fmid * 115) / 100;
		demod_288_set_symbolrate(state,(Symbolrate_Bds * 11) / 10,register_);
	}
	dprintk(20, "fmin %ld fmax %ld   MasterClock_Hz %ld  Symbolrate_Bds %ld\n", fmin, fmax, MasterClock_Hz, Symbolrate_Bds);
	dprintk(20, "Symbolrate_Bds*1.1  %ld \n", (Symbolrate_Bds * 11) / 10);

	address = 0x53;
	register_[address] = (1 << 7) | MSB(fmin); //F288_STOP_ON_FMIN=1
	stv0288_writeregI(state, address, register_[address]);

	address += 1;
	register_[address] = LSB(fmin);
	stv0288_writeregI(state, address, register_[address]);

	address += 1;
	register_[address] = MSB(fmax);
	stv0288_writeregI(state, address, register_[address]);

	address += 1;
	register_[address] = LSB(fmax);
	stv0288_writeregI(state, address, register_[address]);

	address += 1;
	register_[address] = MAX((Symbolrate_Bds / 1000000) ,1);
	stv0288_writeregI(state, address, register_[address]);

	demod_288_start_fine_algorithm(state, 1, register_); /* start fine algorithm */

	i = 0;
	address = 0x50;
	do
	{
		//1ms -> 10ms
		msleep(10); //<---

		i++;
		F288_FINE = stv0288_readreg(state, address);
		flag=(F288_FINE >> 1) & 0x01;
		dprintk(100, "  (demod_288_fine/[%ld] flag=%d)\n", i, flag);

	} while((flag) && (i < 100)); /* wait for end of fine algorithm */
	// <- loop num is 100 times in spite of wait time modified :1ms -> 10ms
	demod_288_start_fine_algorithm(state, 0, register_); /* stop fine algorithm */
}

static void demod_288_start_autocenter_algorithm(struct stv0288_state *state, int flag,unsigned char *register_)
{
	int address = 0x50;

	dprintk(100, "%s >\n", __func__);

	register_[address] &= 0xFB;
	register_[address] |= (flag << 2);
	stv0288_writeregI(state, address, register_[address]);
}

long demod_288_get_timing_loop( struct stv0288_state *state, unsigned char *register_)
{
	int address;
	unsigned char F288_TIMING_LOOP_FREQ_MSB;
	unsigned char F288_TIMING_LOOP_FREQ_LSB;
	long timing;

	address = 0x22;
	F288_TIMING_LOOP_FREQ_MSB = stv0288_readreg(state,address);
	F288_TIMING_LOOP_FREQ_LSB = stv0288_readreg(state,address + 1);
	timing=BYTES2WORD(F288_TIMING_LOOP_FREQ_MSB,F288_TIMING_LOOP_FREQ_LSB);

	dprintk(20, "  (demod_288_get_timing_loop: timing=%ld \t(%08x) )\n", timing, (unsigned int)timing);

	if (timing > 0x10000 / 2)
	{ //2's complement
		timing = 0x10000 - timing;
		timing *= -1;
	}
	return timing;
}

static long demod_288_autocenter(struct stv0288_state *state, unsigned char *register_)
{
	long timeout = 0,timing;
	demod_288_start_autocenter_algorithm(state, 1 ,register_); /* Start autocentre algorithm */

	dprintk(100, "%s >\n", __func__);

	do
	{
		dprintk(100, "  (%.3ld )\n",timeout);

		msleep(10); /* wait 10 ms */
		timeout++;
		timing = demod_288_get_timing_loop(state,register_);

		dprintk(10, "  (demod_288_autocenter: timing=%ld \t(%.8X) )\n",timing, (unsigned int)timing);

	} while ((ABS(timing) > 300) && (timeout < 100));
	/* timing loop is centered or timeout limit is reached */

	demod_288_start_autocenter_algorithm(state, 0 ,register_);	/* Stop autocentre algorithm */
	return timing;
}

static long demod_288_data_timing_constant(struct stv0288_state *state, long SymbolRate, unsigned char *register_)
{
	int address;
	unsigned char data;
	long Tviterbi = 0,
	     TimeOut = 0,
	     THysteresis = 0,
	     Tdata = 0,
	     PhaseNumber[6] = {2, 6, 4, 6, 14,8},
	     averaging[4] = {1024, 4096, 16384, 65536},
	     InnerCode = 1000,
	     HigherRate = 1000;
	long i;
	unsigned char Pr;
	int Sn,To,Hy;

	/*=======================================================================
	-- Data capture time (in ms)
	-- -------------------------
	-- This time is due to the Viterbi synchronisation.
	--
	-- For each authorized inner code, the Viterbi search time is calculated,
	-- and the results are cumulated in ViterbiSearch.
	-- InnerCode is multiplied by 1000 in order to obtain timings in ms
	=======================================================================*/
	address = 0x37;
	Pr = stv0288_readreg(state, address);
	address = 0x38;
	data = stv0288_readreg(state, address);
	Sn = (data >> 4) & 0x03;
	To = (data >> 2) & 0x03;
	Hy = data & 0x03;

	for (i = 0; i < 6; i++)
	{
		if (((Pr >> i) & 0x01) == 0x01)
		{
			switch(i)
			{
				case 0:  /* inner code 1/2 */
				{
					InnerCode = 2000;  /* 2.0 */
					break;
				}
				case 1:  /* inner code 2/3 */
				{
					InnerCode = 1500;  /* 1.5 */
					break;
				}
				case 2: /* inner code 3/4 */
				{
					InnerCode = 1333;  /* 1.333 */
					break;
				}
				case 3:  /* inner code 5/6 */
				{
					InnerCode = 1200;  /* 1.2 */
					break;
				}
				case 4:  /* inner code 6/7 */
				{
					InnerCode = 1167;  /* 1.667 */
					break;
				}
				case 5:  /* inner code 7/8 */
				{
					InnerCode = 1143;  /* 1.143 */
					break;
				}
			}
			Tviterbi += (2 * PhaseNumber[i] * averaging[Sn] * InnerCode);
			if (HigherRate < InnerCode)
			{
				HigherRate = InnerCode;
			}
		}
	}
	/* Time out calculation (TimeOut)
	-- ------------------------------
	-- This value indicates the maximum duration of the synchro word research. */
	TimeOut = (long)(HigherRate * 16384L * (1L << To)); /* 16384= 16 x 1024 bits */

	/* Hysteresis duration (Hysteresis)
	   -- ------------------------------ */
	THysteresis = (long)(HigherRate * 26112L * (1L << Hy)); /* 26112= 16 x 204 * 8 bits */
	Tdata =((Tviterbi + TimeOut + THysteresis) / (2 * (long)(SymbolRate)));

	/* a guard time of 1 mS is added */
	return (1L + (long)Tdata);
}

static int demod_288_waitlock(struct stv0288_state *state, long TData,unsigned char *register_)
{
	int address = 0x24;
	unsigned char data;
	long timeout = 0;
	int lock;

	dprintk(100, "%s >", __func__);

	do
	{
		msleep(100); //wait 1 ms //<---
		timeout += 10; //<---

		data = stv0288_readreg(state,address);
		lock=(data >> 3) & 0x01;

		dprintk(100, "  (demod_288_waitlock: lock= %d (%.2X) \t[%ld] )\n",lock, data, timeout);

		if (lock == 1)
		{
			dprintk(10, "---> demod_288_waitlock/OK!!\n");
		}
	} while (!lock && (timeout<TData + 10)); //<---
	return lock;
}

static int demod_288_get_pr(struct stv0288_state *state, unsigned char *register_)
{
	int address = 0x24;
	unsigned char data;

	data = stv0288_readreg(state, address);
	register_[address]=data;

	return data & 0x07;
}

static int demod_288_get_cf(struct stv0288_state *state, unsigned char *register_)
{
	int CF;
	unsigned char address = 0x24, data;

	data = stv0288_readreg(state,address);
	CF = (data >> 7) & 0x01;

	return CF;
}

static int demod_288_algo(struct dvb_frontend *fe, unsigned long freq, unsigned long baud)
{
	struct stv0288_state *state = fe->demodulator_priv;
	int address;
	int flag = 0;
	long MasterClock_Hz;
	long tunfreq_Khz; //kHz
	long SearchFreq_Khz; //kHz
	long SymbolRate_Bds; //sps??
	long coarseOffset_Khz = 0;
	long coarseSymbolRate_Bds = 0;
	long tdata;
	long kt;
	long timing = 0;
	int symbolrate_ok;
	int lock = 0;

	int known = 0;

	int tunerIQ;
	long MinOffset_Khz = -5000; //-5MHz
	long MaxOffset_Khz = 5000; //+5MHz

	//---pIntResults---
	long pIntResults_SymbolRate_Bds;
	int pIntResults_PunctureRate;
	long pIntResults_Frequency_Khz = 0;
	FE_288_SIGNALTYPE_t pIntResults_SignalType; //

	unsigned char F288_FECMODE;
	int F288_ALPHA = 7;
	int F288_BETA = 28;
	int F288_IND1_ACC = 0;
	int F288_IND2_ACC = 0xFF;

	FE_288_SIGNALTYPE_t signalType=NOAGC1;

	SearchFreq_Khz=freq; //kHz
	SymbolRate_Bds=baud * 1000; //sps?

	dprintk(20, "SEARCH>> FE_288_Algo::Begin\n");
	dprintk(20, "SEARCH>> FE_288_Algo::Searched frequency=%ld kHz\n", SearchFreq_Khz);
	dprintk(20, "SEARCH>> FE_288_Algo::Search range=+/-%ld kHz\n", ABS(MaxOffset_Khz));

	pll_set_freq(fe, freq, baud); /* Set tuner frequency */
	tunfreq_Khz=demod_288_tuner_getfreq(); /* Read tuner frequency */
	/* ( Check tuner status ) */
	msleep(100);

	tunerIQ=demod_288_get_iq_wiring(); /* Get tuner IQ wiring */
	kt = 56;
	known = (SymbolRate_Bds != 0);
	coarseOffset_Khz = tunerIQ * (SearchFreq_Khz - tunfreq_Khz);
	/* begin coarse algorithm with tuner residual offset */

	address = 0x30;
	F288_FECMODE = stv0288_readreg(state,address);
	F288_FECMODE >>= 4;
	F288_FECMODE &= 0x0F;

	do
	{
		dprintk(100, "SEARCH>> FE_288_Algo::Kt = %ld\n",kt);
		/* Setup of non-modified parameters */
		F288_ALPHA = 7;
		F288_BETA = 28;

		address = 0x16;
		register_[address] &= 0xF0;
		register_[address] |= F288_ALPHA;
		stv0288_writeregI(state, address, register_[address]); //F288_ALPHA

		address += 1;
		register_[address] &= 0xC0;
		register_[address] |= F288_BETA;
		stv0288_writeregI(state,address,register_[address]); //F288_BETA

		/* Set Kt value */
		address = 0x51;
		register_[address] = kt;
		stv0288_writeregI(state, address, register_[address]);

		demod_288_froze_lock(state, 1, register_);

		F288_IND1_ACC = 0;
		F288_IND2_ACC = 0xFF;
		address = 0x5E;
		register_[address] = F288_IND1_ACC;
		stv0288_writeregI(state, address, register_[address]);
		address += 1;
		register_[address] = F288_IND2_ACC;
		stv0288_writeregI(state, address, register_[address]);

		demod_288_set_symbolrate(state, 1000000, register_); //sps
		/* Set symbolrate to 1.000MBps (minimum symbol rate) */

		demod_288_set_derot_freq(state, coarseOffset_Khz * 1000, register_);
		//Hz /* Set carrier loop offset to 0KHz or previous iteration value */

		demod_288_set_frequency_offset_detector(state, 1, register_);
		/* Frequency offset detector on */

		coarseSymbolRate_Bds=demod_288_coarse(state, register_, &coarseOffset_Khz);
		/* Symbolrate coarse search */

		coarseOffset_Khz *= tunerIQ;

		/* symbol rate is already known, so keep only the offset and force the symbol rate */
		if (known)
		{
			demod_288_set_symbolrate(state, SymbolRate_Bds, register_);
		}
		else /* take into account the symbol rate returned by the coarse algorithm */
		{
			SymbolRate_Bds=coarseSymbolRate_Bds;
		}
		dprintk(20, "SEARCH>> FE_288_Algo::SymbolRate=%ld Kbds, Offset=%ld KHz\n", SymbolRate_Bds / 1000, coarseOffset_Khz);

		MasterClock_Hz=demod_288_get_mclk_freq(register_);
		if (SymbolRate_Bds > 1000)
		{
			symbolrate_ok = (MasterClock_Hz / (SymbolRate_Bds / 1000)) > 2100; /* to avoid a divide by zero error */
		}
		else
		{
			symbolrate_ok = TRUE;
		}
		if ((SymbolRate_Bds >= 1000000)	/* Check min symbolrate value */
		&&  (coarseOffset_Khz >= MinOffset_Khz) /*Check minimum derotator offset criteria */
		&&  (coarseOffset_Khz < MaxOffset_Khz)/* Check maximum derotator offset criteria */
		&&  (symbolrate_ok))  /* Check shannon criteria */
		{
			unsigned char F288_TMG_LOCK,F288_CF;

			if (SymbolRate_Bds < 5000000)
			{
				F288_ALPHA = 8;
				F288_BETA = 17;
			}
			else if(SymbolRate_Bds > 35000000)
			{
				F288_ALPHA = 8;
				F288_BETA = 36;
			}
			address = 0x16;
			register_[address] &= 0xF0;
			register_[address] |= F288_ALPHA;
			stv0288_writeregI(state, address, register_[address]);

			address += 1;
			register_[address] &= 0xC0;
			register_[address] |= F288_BETA;
			stv0288_writeregI(state, address, register_[address]);

			demod_288_froze_lock(state, 0, register_);

			demod_288_fine(state, SymbolRate_Bds, known,register_);

			F288_TMG_LOCK = stv0288_readreg(state, 0x1E);
			F288_TMG_LOCK &= 0x80;
			F288_CF = stv0288_readreg(state, 0x24);
			F288_CF &= 0x80;

			dprintk(20, "SEARCH>> FE_288_Algo::Timing=%s,Carrier=%s\n", F288_TMG_LOCK ? "LOCK" : "UNLOCK", F288_CF ? "LOCK" : "UNLOCK");

			demod_288_set_frequency_offset_detector(state, 0, register_);
			/* Frequency offset detector off */

			timing = demod_288_autocenter(state,register_);

			if (!demod_288_get_cf(state, register_) && (SymbolRate_Bds > 18000000))
			{
				demod_288_set_symbolrate(state, SymbolRate_Bds, register_);
				demod_288_set_frequency_offset_detector(state, 1 ,register_);
				demod_288_set_derot_freq(state, 0, register_);
				demod_288_set_frequency_offset_detector(state, 0, register_);
				timing=demod_288_autocenter(state, register_);
			}
			dprintk(20, "SEARCH>> FE_288_Algo::Timing offset = %ld(%.2X)\n", ABS(timing), (unsigned int)ABS(timing));

			if (ABS(timing) <= 300)
			{
				pIntResults_SymbolRate_Bds = demod_288_get_symbolrate(state, register_);
				tdata = 10 + (2 * demod_288_data_timing_constant(state, pIntResults_SymbolRate_Bds, register_));
				lock = demod_288_waitlock(state, tdata,register_);
				if (lock)
				{
					pIntResults_PunctureRate=(FE_288_Rate_t)(1 << demod_288_get_pr(state,register_));
				}
				dprintk(20, "SEARCH>> FE_288_Algo::FEC=%s\n", lock ? "LOCK" : "UNLOCK");

				if (lock)
				{
					/* update results */
					pIntResults_Frequency_Khz = demod_288_tuner_getfreq()-(tunerIQ * demod_288_get_derot_freq(state, register_));

					dprintk(20, "SEARCH>> FE_288_Algo::Transponder freq=%ld KHz\n", (long)(pIntResults_Frequency_Khz));

					if (ABS(SearchFreq_Khz-pIntResults_Frequency_Khz)>ABS(MaxOffset_Khz))
					{
						signalType = OUTOFRANGE;
					}
					else
					{
						signalType = RANGEOK;

						//set lock status for return value
						flag = 1;
					}
					pIntResults_SignalType = signalType;

					//Pop globals
					tp_SymbolRate_Bds = pIntResults_SymbolRate_Bds;
					tp_Frequency_Khz = pIntResults_Frequency_Khz;
					tp_signalType = signalType;
				}
			}
		}
		else
		{
			demod_288_froze_lock(state, 0, register_);
			dprintk(20, "  ( FE_288_Algo:: ABS(timing)<=300:NG )\n");
		}
		kt -= 10;  /* decrease Kt for next trial */
	} while ((signalType != RANGEOK) && (signalType != OUTOFRANGE) && (symbolrate_ok) && (kt >= 46));

	dprintk(10, "SEARCH>> FE_288_Algo::End\n");
	if (flag == 1)
	{
		dprintk(10, "Freq(%d), derot_freq(%d) ==> result_Freq(%d), Symbol(%d)\n", (int)SearchFreq_Khz, (int)(pIntResults_Frequency_Khz-SearchFreq_Khz), (int)pIntResults_Frequency_Khz, (int)SymbolRate_Bds );
	}
	else
	{
		dprintk(10, "Lock => NO Freq(%d), Symbol(%d)\n", (int)SearchFreq_Khz, (int)SymbolRate_Bds);
	}
	return flag;
}

int tuner_write_for_agc_stv0288( struct dvb_frontend *fe )
{
	dprintk(100, "%s >\n", __func__);

	pll_set_freq(fe, 1000, 10); /* Set tuner frequency */
	return 0;
}

struct dvb_frontend *stv0288_attach(struct stv0288_config *config, struct i2c_adapter *i2c)
{
	struct stv0288_state *state = NULL;
	int id;

	dprintk(100, "%s >\n", __func__);

	/* allocate memory for the internal state */
	state = kmalloc(sizeof(struct stv0288_state), GFP_KERNEL);
	if (state == NULL)
	{
		goto error;
	}
	/* setup the state */
	state->config          = config;
	state->i2c             = i2c;
	state->initialised     = 0;
	state->tuner_frequency = 0;
	state->symbol_rate     = 0;
	state->fec_inner       = 0;
	state->errmode         = STATUS_BER;

	if (config->usedLNB == cLNB_LNBH221)
	{
		state->lnb_priv = lnbh221_attach(state->config->lnb, &state->equipment);
	}
	else if (config->usedLNB == cLNB_PIO)
	{
		state->lnb_priv = lnb_pio_attach(state->config->lnb, &state->equipment);
	}
	stv0288_writeregI(state, 0x41, 0x04);
	msleep(200);
	id = stv0288_readreg(state, 0x00);

	dprintk(20, "stv0288 id: %x\n", id);

	/* register 0x00 contains 0x11 for STV0288  */
	if (id != 0x11)
	{
		goto error;
	}
	/* create dvb_frontend */
	memcpy(&state->frontend.ops, &stv0288_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;
	return &state->frontend;

error:
	kfree(state);
	return NULL;
}
// vim:ts=4
