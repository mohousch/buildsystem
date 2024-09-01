/*
  TDA10024  - NXP TDA10024 DVB-C Demod

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
#include "frontend_platform.h"
#include "tda10024.h"
#include "socket.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[tda10024] "

extern int mxl201_attach(struct dvb_frontend *fe, struct mxl201_private_data_s *mxl201, struct i2c_adapter *i2c, u8 i2c_address);

/* ****************************************** */
/* saved platform config */
static struct platform_frontend_config_s *frontend_cfg = NULL;
struct mxl201_private_data_s   mxl201;
struct tda10024_private_data_s tda10024;

#define cMaxSockets 4
static u8 numSockets = 0;
static struct socket_s socketList[cMaxSockets];

struct tda10024_config
{
	u8               tuner_no;
	struct stpio_pin *tuner_enable_pin;
	u32              tuner_active_lh;
	u32              demod_address;
	u32              tuner_address;

	u32              ts_out;
	u32              si;
	u32              power;
	u32              agc_th;
};

struct tda10024_state
{
	struct i2c_adapter      *i2c;
	u8                      i2c_address;
	struct dvb_frontend_ops ops;
	struct dvb_frontend     frontend;

	u32                     ucb;
	u32                     ber;
	u32                     sr;
	u32                     mod;
	u32                     power;
	u32                     ber_dep;

	u8                      initDone;

	struct tda10024_config  *config;
};

/* ****************************************** */

static u8 tda10024_init_tab[] =
{
	0x2A,0x0F, 0xFF,0x64, 0x28,0x0B, 0x29,0x80,
	0x00,0x33, 0xFF,0x64, 0x01,0x30, 0x02,0x9B,
	0x03,0x1A, 0x04,0x52, 0x05,0x26, 0x06,0x77,
	0x07,0x1A, 0x08,0x23, 0x09,0x6C, 0x0A,0x00,
	0x0B,0x80, 0x0C,0x1B, 0x0D,0x95, 0x0E,0xB2,
	0x0F,0x40, 0x10,0xB8, 0x12,0xA1, 0x13,0x01,
	0x1B,0xC8, 0x1C,0xB0, 0x1E,0xA3, 0x1F,0x7F,
	0x20,0x04, 0x28,0x0B, 0x29,0x80, 0x2A,0x0C,
	0x2B,0xA0, 0x2C,0x00, 0x2D,0x96, 0x2E,0x20,
	0x30,0x00, 0x31,0x00, 0x32,0x00, 0x33,0x04,
	0x34,0x00, 0x35,0xFF, 0x36,0x00, 0x37,0x02,
	0x38,0xBC, 0x3B,0xFF, 0x3C,0xFF, 0x3D,0x02,
	0xB4,0x5C, 0xB5,0x19, 0xB6,0x3C, 0xB7,0x3C,
	0xB8,0xE6, 0xBE,0xA3, 0xBF,0x02, 0xC0,0x10,
	0xC2,0x80, 0xC3,0x35, 0xC4,0x0A, 0xC5,0x5B,
	0xC6,0xB5, 0xCA,0x0F, 0xCB,0x26, 0xD0,0x02,
	0xD1,0x00, 0xD2,0x20, 0xD4,0x25, 0xD5,0x7F,
	0xD6,0x7F, 0xD7,0x7F, 0xDC,0x80, 0xDD,0x80,
	0xE1,0x25, 0xE6,0x07, 0x00,0x32, 0x00,0x33,
	0xFF,0xFF
};

static u8 tda10024_gate_on[] =
{
	0x0F,0xC0, 0x0F,0xC0, 0xFF,0xFF
};

static u8 tda10024_gate_off[] =
{
	0x0F,0x40, 0xFF,0xFF
};

static u8 tda10024_ucb_clear[] =
{
	0x10,0x18, 0x10,0x38, 0xFF,0xFF
};

static u8 tda10024_power_d0[] =
{
	0xFF,0xFF
};

static u8 tda10024_ts_off[] =
{
	0x2C,0x03, 0xFF,0xFF
};

static u8 tda10024_ts_par[] =
{
	0x20,0x06, 0x2C,0x01, 0xFF,0xFF
};

static u8 tda10024_ts_ser[] =
{
	0x20,0x03, 0x2C,0x02, 0xFF,0xFF
};

static u8 tda10024_qam256_dvbc_ac[] =
{
	0x04,0x52, 0x05,0x26, 0x08,0x23, 0x09,0x6C,
	0x1C,0xB0, 0xB4,0x5C, 0xB6,0x3C, 0x00,0x32,
	0x00,0x33, 0xFF,0xFF
};

static u8 tda10024_qam128_dvbc_ac[] =
{
	0x04,0x52, 0x05,0x36, 0x08,0x34, 0x09,0x7E,
	0x1C,0xB0, 0xB4,0x70, 0xB6,0x4C, 0x00,0x2E,
	0x00,0x2F, 0xFF,0xFF
};

static u8 tda10024_qam64_dvbc_ac[] =
{
	0x04,0x52, 0x05,0x46, 0x08,0x43, 0x09,0x6A,
	0xB4,0x6A, 0xB6,0x44, 0x00,0x2C, 0x00,0x2B,
	0xFF,0xFF
};

static u8 tda10024_qam32_dvbc_ac[] =
{
	0x04,0x52, 0x05,0x64, 0x08,0x74, 0x09,0x96,
	0x1C,0xB0, 0xB4,0x8C, 0xB6,0x57, 0x00,0x27,
	0x00,0x26, 0x00,0x27, 0xFF,0xFF
};

static u8 tda10024_qam16_dvbc_ac[] =
{
	0x04,0x52, 0x05,0x87, 0x08,0xA2, 0x09,0x91,
	0x1C,0xB0, 0xB4,0x8C, 0xB6,0x57, 0x00,0x22,
	0x00,0x23, 0xFF,0xFF
};

static u8 tda10024_sr_7000[] =
{
	0x03,0x1A, 0x0A,0x00, 0x0B,0x00, 0x0C,0x1C,
	0x0D,0x92, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_6952[] =
{
	0x03,0x1A, 0x0A,0xD9, 0x0B,0xCE, 0x0C,0x1B,
	0x0D,0x93, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_6900[] =
{
	0x03,0x1A, 0x0A,0x99, 0x0B,0x99, 0x0C,0x1B,
	0x0D,0x94, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_6875[] =
{
	 0x03,0x1A, 0x0A,0x00, 0x0B,0x80, 0x0C,0x1B,
	 0x0D,0x95, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	 0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_6125[] =
{
	0x03,0x0A, 0x0A,0x00, 0x0B,0x80, 0x0C,0x18,
	0x0D,0xA7, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_6000[] =
{
	0x03,0x0A, 0x0A,0x00, 0x0B,0x00, 0x0C,0x18,
	0x0D,0xAB, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_5274[] =
{
	0x03,0x1A, 0x0A,0x93, 0x0B,0x18, 0x0C,0x15,
	0x0D,0xC2, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_5217[] =
{
	0x03,0x1A, 0x0A,0x35, 0x0B,0xDE, 0x0C,0x14,
	0x0D,0xC4, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_5200[] =
{
	0x03,0x1A, 0x0A,0xCC, 0x0B,0xCC, 0x0C,0x14,
	0x0D,0xC5, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x82, 0xFF,0xFF
};

static u8 tda10024_sr_5361[] =
{
	0x03,0x1A, 0x0A,0xA9, 0x0B,0x71, 0x0C,0x15,
	0x0D,0xBF, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x02, 0xFF,0xFF
};

static u8 tda10024_sr_5057[] =
{
	0x03,0x1A, 0x0A,0x5E, 0x0B,0x3A, 0x0C,0x14,
	0x0D,0xCA, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x82, 0xFF,0xFF
};

static u8 tda10024_sr_4000[] =
{
	0x03,0x0A, 0x0A,0x00, 0x0B,0x00, 0x0C,0x10,
	0x0D,0xFF, 0x0E,0xB2, 0x37,0x02, 0x38,0xBC,
	0x3D,0x82, 0xFF,0xFF
};

/* ****************************************** */

static s32 tda10024_snr_lut_qam16[][2]=
{
	{	72	,	1810	}, /*	18.1	dB	*/
	{	57	,	2020	}, /*	20.2	dB	*/
	{	45	,	2240    }, /*	22.4	dB	*/
	{	34	,	2570	}, /*	25.7	dB	*/
	{	26	,	3080	}, /*	30.8	dB	*/
	{	25	,	3010	}, /*	30.1	dB	*/
	{	21	,	3360	}, /*	33.6	dB	*/
	{	17	,	3790	}, /*	37.9	dB	*/
	{	15	,	4330	}, /*	43.3	dB	*/
	{	13	,	4420	}, /*	44.2	dB	*/
	{	11	,	4850	}, /*	48.5	dB	*/
	{	10	,	5300	}, /*	53	dB	*/
	{	9	,	5800 	}, /*	58	dB	*/
	{	8	,	5900	}, /*	59	dB	*/
	{	7	,	6100	}, /*	61	dB	*/
	{	6	,	6000	} /*	60	dB	*/
};

static s32 tda10024_snr_lut_qam64[][2]=
{
	{	37	,	2300 	}, /*	23	dB	*/
	{	30	,	2520 	}, /*	25.2	dB	*/
	{	26	,	2670	}, /*	26.7	dB	*/
	{	21	,	2940	}, /*	29.4	dB	*/
	{	16	,	3090	}, /*	30.9	dB	*/
	{	14	,	3310	}, /*	33.1	dB	*/
	{	12	,	3390	}, /*	33.9	dB	*/
	{	10	,	3580	}, /*	35.8	dB	*/
	{	9	,	3690	}, /*	36.9	dB	*/
	{	8	,	3810	}, /*	38.1	dB	*/
	{	7	,	4000    }  /*	40	dB	*/
};

static s32 tda10024_snr_lut_qam256[][2]=
{
	{	23	,	2900	}, /*	29	dB	*/
	{	18	,	3100	}, /*	31	dB	*/
	{	17	,	3150	}, /*	31.5	dB	*/
	{	15	,	3200	}, /*	32	dB	*/
	{	13	,	3300	}, /*	33	dB	*/
	{	12	,	3500	}, /*	35	dB	*/
	{	11	,	3700	}, /*	37	dB	*/
	{	10	,	3900	}, /*	39	dB	*/
	{	8	,	4100	}, /*	41	dB	*/
	{	7	,	4200    }  /*	42	dB	*/
};


#define TDA10024_INRANGE( x,  y, z) (((x)<=(y) && (y)<=(z))||((z)<=(y) && (y)<=(x)))
static int tda10024_lookup_snr(s32 val, s32 table[][2], u32 size)
{
	u32  ret = 0, i;

	dprintk(100, "%s > val %d\n", __func__, val);

	size= (size / (sizeof(u32) << 1)) - 1;

	if (TDA10024_INRANGE(table[size][0], val, table[0][0]))
	{
		for (i = 0; i < size; i++)
		{
			if (TDA10024_INRANGE(table[i+1][0], val, table[i][0]) )
			{
				ret = table[i + 1][1] +
					  (table[i][1] - table[i + 1][1]) *
					  (val - table[i+1][0]) /
					  (table[i][0] - table[i+1][0]);
				break;
			}
		}
	}
	else
	{
		ret = (val > table[0][0]) ? table[0][1] : table[size][1];
	}
	dprintk(100, "%s < ret = %d\n", __func__, ret);
	return ret;
}

/* ********************* i2c *********************** */

static int tda10024_i2c_write(struct tda10024_state *state, u32 index, u32 num, u8 *buf)
{
	int            res = 0, i;
	u8             bytes[256];
	struct i2c_msg msg[2];

	bytes[0] = (u8)index;

	for (i = 0; i < num; i++)
	{
		bytes[i + 1] = buf[i];
	}
	{
		u8 dstr[1024];

		dstr[0] = '\0';
		for (i = 0; i < num + 1; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, bytes[i]);
		}
		dprintk(200, "%s n: %u b: %s\n", __func__, num + 1, dstr);
	}
	/* write */
	msg[0].addr  = state->i2c_address;
	msg[0].flags = 0;
	msg[0].buf   = bytes;
	msg[0].len   = num + 1;

	if ((res = i2c_transfer(state->i2c, msg, 1)) != 1)
	{
		dprintk(1, "%s: 2. error on i2c_transfer 0x%02x %d (%d)\n", __func__, index, num, res);
		return res;
	}
	return 0;
}

static u8 tda10024_i2c_read(struct tda10024_state *state, u32 index, u32 num, u8* buf)
{
	struct i2c_msg msg[2];
	int    ret = 0;
	u8     bytes[1];

	dprintk(200, "%s: index 0x%02x %d\n", __func__, index, num);
	bytes[0] = (u8) index;

	/* write */
	msg[0].addr  = state->i2c_address;
	msg[0].flags = 0;
	msg[0].buf   = bytes;
	msg[0].len   = 1;
	/* read */
	msg[1].addr  = state->i2c_address;
	msg[1].flags = I2C_M_RD;
	msg[1].buf   = buf;
	msg[1].len   = num;

	if (( ret = i2c_transfer(state->i2c, msg, 2)) != 2)
	{
		dprintk(1, "%s: 2. error on i2c_transfer (%d)\n", __func__, ret);
		return ret;
	}
	{
		u8 i;
		u8 dstr[1024];

		dstr[0] = '\0';
		for (i = 0; i < num; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, buf[i]);
		}
		dprintk(200, "%s: n: %u r: 0x%02x b: %s\n", __func__, num, index, dstr);
	}
	return 0;
}

static int tda10024_i2c_write_bulk(struct tda10024_state *state, u8 *bytes)
{
	int  res = 0;
	u8   buf[64];
	u8   a = 0xFF;
	u8   r = 0;

/* funny function ;)
 * this function writes the bytes in bytes array. if the even bytes
 * (the registers) are sequenced registers, the bytes are not written
 * direct but send as one sequence:
 * [0] = 0x00 [2] = 0x01 and so on ->sequences in this case the bytes
 * in bytes[odd] are buffered and written together.
* if the even byte is 0xff the even and next odd is ignored :-/
 * not sure what's the sense of this...
 */

	dprintk(100, "%s >\n", __func__);

	for ( ; ; bytes += 2)
	{
		/* a + r ->last register + 1
		 * bytes[0] current register
		 */
		if ( a + r != bytes[0])
		{
			if (a != 0xFF)
			{
				res |= tda10024_i2c_write(state, a , r, buf);
				if (res != 0)
				{
					dprintk(1, "%s: writing data failed (%d)\n", __func__, res);
					return -1;
				}
			}
			a = bytes[0];
			buf[0] = bytes[1];
			r = 1;
		}
		else
		{
			buf[r] = bytes[1];
			r++;
		}

		if ((bytes[0] == 0xff) && (bytes[1] == 0xff))
		{
			break;
		}
		if ((bytes[0] == 0xff) && (bytes[1] != 0xff))
		{
			if (bytes[1] & 0x80)
			{
				mdelay((bytes[1] & 0x7F) * 10);
			}
			else
			{
				mdelay(bytes[1]);
			}
		}
	}
	dprintk(100, "%s < res %d\n", __func__, res);
	return res;
}

static int tda10024_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct tda10024_state *state = fe->demodulator_priv;
	int res = 0;

	dprintk(100, "%s >\n", __func__);
	res = tda10024_i2c_write_bulk(state , enable ? tda10024_gate_on : tda10024_gate_off);
	dprintk(100, "%s < %d\n", __func__, res);
	return res;
}
/* ********************* i2c end *********************** */

static int tda10024_set_ts_out(struct tda10024_state* state, u32 ts_out)
{
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	switch (ts_out)
	{
		case TDA10024_TS_PARALLEL:
		{
			res |= tda10024_i2c_write_bulk(state , tda10024_ts_par);
			break;
		}
		case TDA10024_TS_SERIAL:
		{
			res |= tda10024_i2c_write_bulk(state , tda10024_ts_ser);
			break;
		}
		case TDA10024_TS_OFF:
		{
			res |= tda10024_i2c_write_bulk(state , tda10024_ts_off);
			break;
		}
		default:
		{
			dprintk(1, "%s: wrong parameter %d\n", __func__, ts_out);
			res = -1;
			break;
		}
	}
	dprintk(100, "%s < res = %d\n", __func__, res);
	return res;
}

static int tda10024_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct tda10024_state *state = fe->demodulator_priv;
	int res = 0;
	u8 bytes[2];

	*status = 0;

	res |= tda10024_i2c_read(state , 0x11, 1, &bytes[0]);
	res |= tda10024_i2c_read(state , 0x17, 1, &bytes[1]);

	if (res != 0)
	{
		dprintk(1, "%s: Error reading data (%d)\n", __func__, res);
		return res;
	}

	if ((bytes[1] < state->config->agc_th))
	{
		*status |= FE_HAS_SIGNAL;
	}
	if ((bytes[0] & 7) == 7)
	{
		*status |= FE_HAS_CARRIER;
	}
	if (bytes[0] & 4)
	{
		*status |= FE_HAS_SYNC | FE_HAS_VITERBI;
	}
	if (bytes[0] & 0x8)
	{
		*status |= FE_HAS_LOCK;
		dprintk(20, "%s: FE_HAS_LOCK\n", __func__);
	}
	dprintk(100, "%s < res = %d, status = %d\n", __func__, res, *status);
	return res;
}

static int tda10024_read_ber(struct dvb_frontend *fe, u32* ber)
{
	struct tda10024_state *state = fe->demodulator_priv;
	u8 bytes[3];
	int res = 0;

	*ber = 0;

	res |= tda10024_i2c_read(state , 0x14, 3, &bytes[0]);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	*ber = ((bytes[0] | bytes[1] << 8 | bytes[2] << 16) * state->ber_dep);

	dprintk(40, "%s: < res = %d ber = %d\n", __func__, res, *ber);
	return res;
}

static int tda10024_read_signal_strength(struct dvb_frontend *fe, u16 *signal_strength)
{
	struct tda10024_state *state = fe->demodulator_priv;
	u8                    byte = 0;
	int                   res = 0;

	*signal_strength = 0;

	res |= tda10024_i2c_read(state, 0x17, 1, &byte);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	if (byte > 0)
	{
		*signal_strength = (u16) (0xFFFF - ((byte << 8) | byte));
	}
	dprintk(40, "%s: < res = %d, singnal_strength = %d\n", __func__, res, *signal_strength);
	return res;
}

static int tda10024_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct tda10024_state *state = fe->demodulator_priv;
	u8  bytes[1];
	int res = 0;

	*snr = 0;

	res |= tda10024_i2c_read(state , 0x18, 1, &bytes[0]);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}

	if (state->mod == QAM_16)
	{
		*snr = (u16)tda10024_lookup_snr(bytes[0], tda10024_snr_lut_qam16, sizeof(tda10024_snr_lut_qam16));
	}
	if (state->mod == QAM_32)
	{
		*snr = (u16) tda10024_lookup_snr(bytes[0], tda10024_snr_lut_qam64, sizeof(tda10024_snr_lut_qam64));
	}
	if (state->mod & (QAM_64 | QAM_32))
	{
		*snr = (u16) tda10024_lookup_snr(bytes[0], tda10024_snr_lut_qam64, sizeof(tda10024_snr_lut_qam64));
	}
	if (state->mod & (QAM_128 | QAM_32))
	{
		*snr = (u16) tda10024_lookup_snr(bytes[0], tda10024_snr_lut_qam256, sizeof(tda10024_snr_lut_qam256));
	}
	if (state->mod & (QAM_256 | QAM_128))
	{
		*snr = (u16) tda10024_lookup_snr(bytes[0], tda10024_snr_lut_qam256, sizeof(tda10024_snr_lut_qam256));
	}
	dprintk(40, "%s: < snr %d\n", __func__, *snr);
	return 0;
}

static int tda10024_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	struct tda10024_state *state = fe->demodulator_priv;
	int res = 0;
	u8  bytes[4];
	u32 ucb;

	*ucblocks = 0;

	res |= tda10024_i2c_read(state, 0x74, 4, &bytes[0]);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	state->ucb += ucb = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

	*ucblocks = state->ucb;

	if (ucb)
	{
		res |= tda10024_i2c_write_bulk(state, tda10024_ucb_clear);

		if (res != 0)
		{
			dprintk(1, "%s: error reading data (%d)\n", __func__, res);
			return res;
		}
	}
	dprintk(40, "%s: < res = %d, ucb = %d\n", __func__, res, *ucblocks);
	return res;
}

static void tda10024_release(struct dvb_frontend *fe)
{
	struct tda10024_state *state = fe->demodulator_priv;

	dprintk(100, "%s >\n",__func__);
	kfree(state);
}

static struct dvb_frontend_ops tda10024_ops;

struct dvb_frontend* tda10024_attach(struct tda10024_config *config, struct mxl201_private_data_s *mxl201, struct i2c_adapter *i2c)
{
	struct tda10024_state* state = NULL;

	dprintk(40, "%s >\n", __func__);

	/* Allocate memory for the internal state */
	state = kmalloc(sizeof(struct tda10024_state), GFP_KERNEL);
	if (state == NULL)
	{
		dprintk(1, "%s Unable to kmalloc\n", __func__);
		return NULL;
	}
	/* Setup the state used everywhere */
	memset(state, 0, sizeof(struct tda10024_state));

	state->config      = config;
	state->i2c         = i2c;
	state->i2c_address = config->demod_address;

	state->initDone    = 0;

	memcpy(&state->frontend.ops, &tda10024_ops, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;

	mxl201_attach(&state->frontend, mxl201, i2c, config->tuner_address);

	dprintk(40, "%s <\n", __func__);
	return &state->frontend;
}

static int tda10024_init(struct dvb_frontend* fe)
{
	struct tda10024_state* state = fe->demodulator_priv;
	int res = 0;
	u8  byte;

	dprintk(40, "%s >\n", __func__);

	if (state->initDone == 1)
	{
		 dprintk(1, "%s: init already done. ignoring ... <\n", __func__);
		 return 0;
	}
	res |= tda10024_i2c_write_bulk(state, tda10024_power_d0);
	res |= tda10024_i2c_write_bulk(state, tda10024_init_tab);
	res |= tda10024_i2c_write_bulk(state, tda10024_ucb_clear);

	state->mod = QAM_64;
	res |= tda10024_i2c_read(state, 0x10, 1, &byte);

	switch (byte & 0xC0)
	{
		case 0x00:
		{
			state->ber_dep = 1000;
			break;
		}
		case 0x40:
		{
			state->ber_dep = 100;
			break;
		}
		case 0x80:
		{
			state->ber_dep = 10;
			break;
		}
		case 0xC0:
		{
			state->ber_dep = 1;
			break;
		}
	}
	res |= tda10024_set_ts_out(state, state->config->ts_out);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	state->initDone = 1;
	dprintk(40, "%s < res = %d\n", __func__, res);
	return res;
}

static int tda10024_sleep(struct dvb_frontend *fe)
{
	dprintk(100, "%s <>\n",__func__);
	return 0;
}

#if DVB_API_VERSION >= 5
static int tda10024_set_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	dprintk(100, "%s <>\n", __func__);
	return 0;
}

static int tda10024_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
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
#else
static struct dvbfe_info dvbc_info =
{
	.name = "NXP TDA10024 DVB-C",
	.delivery = DVBFE_DELSYS_DVBC,
	.delsys =
	{
		.dvbc.modulation = DVBFE_MOD_QAM16
		                 | DVBFE_MOD_QAM32
		                 | DVBFE_MOD_QAM64
		                 | DVBFE_MOD_QAM128
		                 | DVBFE_MOD_QAM256
		                 | DVBFE_MOD_QAMAUTO
	},
	.frequency_min = 47000000,
	.frequency_max = 897000000,
	.frequency_step = 62500,
	.frequency_tolerance = 0,
	.symbol_rate_min = 3000000,
	.symbol_rate_max = 7000000
};

static int tda10024_get_info (struct dvb_frontend *fe, struct dvbfe_info *fe_info)
{
	dprintk (100, "%s >\n", __func__);

	switch (fe_info->delivery)
	{
		case DVBFE_DELSYS_DVBC:
		{
			dprintk (10, "%s (DVBC)\n", __func__);
			memcpy (fe_info, &dvbc_info, sizeof (dvbc_info));
			break;
		}
		default:
		{
			return -EINVAL;
		}
	}
	return 0;
}
#endif

static void tda10024_sr_calc(u32 sr, u8 *bulk)
{
	u32 uBDR, uBDRI, uNDec, uSFil;

	dprintk(40, "%s: > sr = %d\n", __func__, sr);

	if (sr < 4000000)
	{
		uNDec = 1;
		uSFil = 0;
	}
	else if (sr < 5203000)
	{
		uNDec = 0;
		uSFil = 0x82;
	}
	else
	{
		uNDec = 0;
		uSFil = 0;
	}
	bulk[0] = 0x3D; bulk[1] = (u8)(uSFil ? 0x82 : 0x02);
	bulk[2] = 0x03; bulk[3] = (u8)((uNDec<<6) | 0xA);

	uBDRI   = 64000000*16;
	uBDRI   >>= uNDec;
	uBDRI   += sr/2;
	uBDRI   /= sr;
	uBDRI = (uBDRI > 255) ? 255 : uBDRI;

	/*uBDR = _mul64div32(1<<(24+uNDec), sr, 64000000);*/
	uBDR = (68719 * ( sr >> 10)) >> 8;

	bulk[4] = 0x0A; bulk[5] = (u8)(uBDR);
	bulk[6] = 0x0B; bulk[7] = (u8)(uBDR >> 8);
	bulk[8] = 0x0C; bulk[9] = (u8)(uBDR >> 16);
	bulk[10] = 0x0D; bulk[11] = (u8)uBDRI;
	bulk[12] = bulk[13] = 0xFF;

	dprintk(40, "%s <\n", __func__);
}

#define TDA10024_ABOUT(x,y) ((y-1000<(x))&((x)<y+1000))
static int tda10024_set_frontend(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct tda10024_state *state = fe->demodulator_priv;
#if DVB_API_VERSION >= 5
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
#endif
	int res = 0;
	u8  bytes[16];
	u32 symbol_rate;
	u8  cnt;

#if DVB_API_VERSION >= 5
	symbol_rate = c->symbol_rate;
	dprintk(10, "%s: symbol_rate = %d, modulation = %d\n", __func__, symbol_rate, c->modulation);
#else
	symbol_rate = p->u.qam.symbol_rate;
	dprintk(10, "%s: symbol_rate = %d, modulation = %d\n", __func__, symbol_rate, p->u.qam.modulation);
#endif

	if (state->power)
	{
		res |= tda10024_i2c_write_bulk(state, tda10024_power_d0);
		state->power = 0;
	}
	if (res != 0)
	{
		dprintk(1, "%s: Oops, setting power failed %d\n", __func__, res);
	}
	res = fe->ops.tuner_ops.set_params(fe, p);

	if (res != 0)
	{
		dprintk(1, "%s: Tuner set failed (%d)\n", __func__, res);
		return res;
	}

	if (fe->ops.i2c_gate_ctrl)
	{
		res |= fe->ops.i2c_gate_ctrl(fe, 0);
	}
	if (res == 0)
	{
		if (fe->ops.info.symbol_rate_min > symbol_rate
		||  symbol_rate > fe->ops.info.symbol_rate_max)
		{
			dprintk(1, "%s: Symbol rate out of range\n", __func__);
			res = -1;
		}
		else if (TDA10024_ABOUT(symbol_rate,7000000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_7000);
		}
		else  if (TDA10024_ABOUT(symbol_rate,6952000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_6952);
		}
		else if (TDA10024_ABOUT(symbol_rate,6900000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_6900);
		}
		else if (TDA10024_ABOUT(symbol_rate,6875000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_6875);
		}
		else if (TDA10024_ABOUT(symbol_rate,6125000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_6125);
		}
		else if (TDA10024_ABOUT(symbol_rate,6000000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_6000);
		}
		else if (TDA10024_ABOUT(symbol_rate,5361000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_5361);
		}
		else if (TDA10024_ABOUT(symbol_rate,5274000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_5274);
		}
		else if (TDA10024_ABOUT(symbol_rate,5217000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_5217);
		}
		else if (TDA10024_ABOUT(symbol_rate,5200000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_5200);
		}
		else if (TDA10024_ABOUT(symbol_rate,5057000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_5057);
		}
		else if (TDA10024_ABOUT(symbol_rate, 4000000))
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_sr_4000);
		}
		else
		{
			 dprintk(1, "%s: no about value ... calc ...\n", __func__);
			 tda10024_sr_calc(symbol_rate, bytes);
			 res |= tda10024_i2c_write_bulk(state, bytes);
		}
	}
	else
	{
		dprintk(1, "%s: 1. Oops an error...\n", __func__);
	}
	if (res == 0)
	{
		state->sr = symbol_rate;

#if DVB_API_VERSION >= 5
		if (c->modulation == QAM_AUTO)
#else
		if (p->u.qam.modulation == QAM_AUTO)
#endif
		{
/* fixme: not implemented !!! */
			dprintk(1, "%s: qam auto currently not implemented\n", __func__);
		}
		else
		{
#if DVB_API_VERSION >= 5
			state->mod = c->modulation;
#else
			state->mod = p->u.qam.modulation;
#endif
		}
		if (state->mod == QAM_256)
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_qam256_dvbc_ac);
		}
		else if (state->mod == QAM_128)
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_qam128_dvbc_ac);
		}
		else if (state->mod == QAM_64)
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_qam64_dvbc_ac);
		}
		else if (state->mod == QAM_32)
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_qam32_dvbc_ac);
		}
		else if (state->mod == QAM_16)
		{
			res |= tda10024_i2c_write_bulk(state, tda10024_qam16_dvbc_ac);
		}
		else
		{
			dprintk(1, "%s: wrong qam delivered\n", __func__);
			res = -1;
		}
	}
	else
	{
		dprintk(1, "%s: 2. oops an error ...\n", __func__);
	}
	if (res == 0)
	{
		res |= tda10024_i2c_write_bulk(state, tda10024_ucb_clear);
	}

	if (res != 0)
	{
		dprintk(1, "%s: error clearing lock status\n", __func__);
	}

	cnt = 10; /* FIXME think on this */
	do
	{
		fe_status_t status = 0;

		res |= tda10024_read_status(fe, &status);

		if (res == 0)
		{
			if (status & FE_HAS_LOCK)
			{
				break;
			}
		}
		else
		{
			dprintk(1, "%s: error reading status %d\n", __func__, res);
			break;
		}
		msleep(20); /* fixme: think on this */
		dprintk(20, "%s: Waiting for lock %d ...\n", __func__, cnt);
	} while (--cnt);

	if (cnt == 0)
	{
		dprintk(1, "%s(%d): timeout tuning!\n", __func__, res);
	}
	else
	{
		dprintk(20, "%s(%d,%d): Tuner successfully set!\n", __func__, res, cnt);
	}
	return res;
}

static struct dvb_frontend_ops tda10024_ops =
{
	.info =
	{
		.name = "TDA10024 DVB-C",
		.type = FE_QAM,
		.frequency_stepsize = 62500,
		.frequency_min = 47000000,
		.frequency_max = 897000000,
		.symbol_rate_min = 3000000,
		.symbol_rate_max = 7000000,

		.caps = FE_CAN_QAM_16
		      | FE_CAN_QAM_32
		      | FE_CAN_QAM_64
		      | FE_CAN_QAM_128
		      | FE_CAN_QAM_256
#ifdef WE_CAN_QAM_AUTO
		      | FE_CAN_QAM_AUTO
#endif
	},
	.release              = tda10024_release,

	.init                 = tda10024_init,
	.sleep                = tda10024_sleep,

	.set_frontend         = tda10024_set_frontend,
	.read_status          = tda10024_read_status,
	.read_ber             = tda10024_read_ber,
	.read_signal_strength = tda10024_read_signal_strength,
	.read_snr             = tda10024_read_snr,
	.read_ucblocks        = tda10024_read_ucblocks,
	.i2c_gate_ctrl        = tda10024_gate_ctrl,

#if DVB_API_VERSION >= 5
	.set_property         = tda10024_set_property,
	.get_property         = tda10024_get_property,
#else
	.get_info             = tda10024_get_info,
#endif
};

static void tda10024_register_frontend(struct dvb_adapter *dvb_adap, struct socket_s* socket)
{
	struct dvb_frontend* frontend;
	struct tda10024_config* cfg;

	dprintk(100, "%s >\n", __func__);

	if (numSockets + 1 == cMaxSockets)
	{
		dprintk(1, "%s: Max number sockets reached ... cannot register\n");
		return;
	}
	socketList[numSockets] = *socket;
	numSockets++;
	cfg = kmalloc(sizeof(struct tda10024_config), GFP_KERNEL);

	if (cfg == NULL)
	{
		dprintk(1, "%s malloc error\n", __func__);
		return;
	}
	cfg->tuner_no = numSockets + 1;
	cfg->tuner_enable_pin = stpio_request_pin (socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);

	dprintk(20, "%s tuner_enable_pin %p\n", __func__, cfg->tuner_enable_pin);
	stpio_set_pin(cfg->tuner_enable_pin, !socket->tuner_enable[2]);
	stpio_set_pin(cfg->tuner_enable_pin, socket->tuner_enable[2]);

	msleep(250);

	cfg->tuner_active_lh = socket->tuner_enable[2];

	cfg->demod_address   = frontend_cfg->demod_i2c;
	cfg->tuner_address   = frontend_cfg->tuner_i2c;

	cfg->ts_out          = tda10024.ts_out;
	cfg->si              = tda10024.si;
	cfg->power           = tda10024.power;
	cfg->agc_th          = tda10024.agc_th;

	dprintk(20, "%s: ts_out %d %d\n", __func__, cfg->ts_out, tda10024.ts_out);
	dprintk(20, "%s: power %d %d\n", __func__, cfg->power, tda10024.power);

	frontend =  tda10024_attach(cfg, &mxl201, i2c_get_adapter(socket->i2c_bus));

	if (frontend == NULL)
	{
		dprintk(1, "%s Attching TDA10024 failed\n");

		if (cfg->tuner_enable_pin)
		{
			stpio_free_pin(cfg->tuner_enable_pin);
		}
		kfree(cfg);
		return;
	}
	if (dvb_register_frontend (dvb_adap, frontend))
	{
		dprintk (1, "%s: Frontend registration failed !\n", __func__);
		if (frontend->ops.release)
		{
			frontend->ops.release (frontend);
		}
		return;
	}
	return;
}

static int tda10024_demod_detect(struct socket_s *socket, struct frontend_s *frontend)
{
	struct stpio_pin *pin = stpio_request_pin(socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);

	dprintk(40, "%s > %s: i2c-%d addr 0x%x\n", __func__, socket->name, socket->i2c_bus, frontend_cfg->demod_i2c);

	if (pin != NULL)
	{
		struct tda10024_state state;
		u8  bytes[4];
		int res = 0;

		stpio_set_pin(pin, !socket->tuner_enable[2]);
		stpio_set_pin(pin, socket->tuner_enable[2]);
		msleep(250);

		memset(&state, 0, sizeof(struct tda10024_state));

		state.i2c         = i2c_get_adapter(socket->i2c_bus);
		state.i2c_address = frontend_cfg->demod_i2c;

		*bytes = 0xCC;

		res |= tda10024_i2c_read(&state , 0x1A, 1, bytes);

		if (res != 0)
		{
			dprintk(1, "%s: 1. failed to detect demod %d\n", __func__, res);
			stpio_free_pin(pin);
			return -1;
		}
		if (bytes[0] != 0x7D)
		{
			dprintk(1, "%s: 2. failed to detect demod\n", __func__);
			stpio_free_pin(pin);
			return -1;
		}
	}
	else
	{
		dprintk(1, "%s: failed to allocate pio pin\n", __func__);
		return -1;
	}

stpio_free_pin(pin);
	dprintk(40, "%s <\n", __func__);
	return 0;
}

static int tda10024_demod_attach(struct dvb_adapter* adapter, struct socket_s *socket, struct frontend_s *frontend)
{
	dprintk(100, "%s >\n", __func__);
	tda10024_register_frontend(adapter, socket);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ******************************* */
/* platform device functions       */
/* ******************************* */

static int tda10024_probe (struct platform_device *pdev)
{
	struct platform_frontend_config_s *plat_data = pdev->dev.platform_data;
	struct frontend_s frontend;
	struct tda10024_s* data;

	dprintk(40, "%s >\n", __func__);

	frontend_cfg = kmalloc(sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontend_cfg, plat_data, sizeof(struct platform_frontend_config_s));

	data = (struct tda10024_s*) frontend_cfg->private;
	tda10024 = *data->tda10024;
	mxl201   = *data->mxl201;

	dprintk(10, "Found frontend \"%s\" in platform config\n", frontend_cfg->name);

	frontend.demod_detect = tda10024_demod_detect;
	frontend.demod_attach = tda10024_demod_attach;
	frontend.name         = "tda10024";

	if (socket_register_frontend(&frontend) < 0)
	{
		dprintk(1, "%s Failed to register frontend\n");
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int tda10024_remove (struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver tda10024_driver =
{
	.probe = tda10024_probe,
	.remove = tda10024_remove,
	.driver	=
	{
		.name	= "tda10024",
		.owner  = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init tda10024_init_module(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);
	ret = platform_driver_register (&tda10024_driver);
	dprintk(100, "%s < %d\n", __func__, ret);
	return ret;
}

static void tda10024_cleanup_module(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Activates frontend debugging (default:0)");

module_init(tda10024_init_module);
module_exit(tda10024_cleanup_module);

MODULE_DESCRIPTION("TDA10024 DVB-C demodulator driver");
MODULE_AUTHOR("Duckbox");
MODULE_LICENSE("GPL");
// vim:ts=4
