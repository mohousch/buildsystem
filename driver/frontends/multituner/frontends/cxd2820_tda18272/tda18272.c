/*
 * tda18272.c - NXP TDA18272 RF IC DVB-T2/T Tuner driver
 *
 * Copyright (C) 2011 duckbox
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include "cxd2820_platform.h"

extern short paramDebug;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[tda18272] "

/* ************************************************** */

struct tda18272_config
{
	int  lt;       //1 = lt ->really ??
	u8   stdby;    //3 = d3
	u8   iic_mode; //0 = iic_0
	u8   xtout;    //0 = off
	u8   i2c_addr; //0xC0

	int  (*i2c_readwrite)(void* p, u8 i2c_addr, u8 read, u8* pbytes, u32 nbytes);
	void *demod;
};

struct tda18272_state
{
	u32                    bw;
	u32                    freq;
	u32                    power;
	u32                    rssi;    //rf strength in iic_mode
	u8                     initDone;
	struct tda18272_config *config;
};

/* ************************************************** */
static const u8 tda18272_power_d0[] =
{
	0x5F, 0x00,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_power_d1[] =
{
	0x5F, 0xE0,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_power_d2[] =
{
	0x5F, 0x00,
	0x06, 0x08,
	0xFF, 0xFF
};

static const u8 tda18272_power_d3[] =
{
	0x5F, 0xE0,
	0x06, 0x08,
	0xFF, 0xFF
};

static const u8 tda18272_lt_on_dvb_t2[] =
{
	0x0C, 0xB0,
	0xFF, 0xFF
};

static const u8 tda18272_lt_off_dvb_t2[] =
{
	0x0C, 0x30,
	0xFF, 0xFF
};

static const u8 tda18272_init_dvbt2[] =
{
	0x04, 0x01,
	0x05, 0x01,
	0x30, 0x4D,
	0x25, 0x60,
	0x0C, 0x30,
	0x0E, 0xFE,
	0x0F, 0xBB,
	0x10, 0x0B,
	0x11, 0x4B,
	0x12, 0x02,
	0x1E, 0x08,
	0x1F, 0xA0,
	0x36, 0x0C,
	0x24, 0x49,
	0x26, 0x22,
	0x27, 0x22,
	0x28, 0x22,
	0x29, 0x22,
	0x2A, 0x22,
	0x2B, 0x22,
	0x22, 0x3D,
	0x37, 0xC8,
	0x0A, 0x8C,
	0x06, 0x00,
	0x14, 0x43,
	0x19, 0x3B,
	0x1A, 0x01,
	0x1B, 0x60,
	0xFF, 0xFF
};

static const u8 tda18272_xtout_on[] =
{
	0x14, 0x43,
	0xFF, 0xFF
};
static const u8 tda18272_xtout_off[] =
{
	0x14, 0x40,
	0xFF, 0xFF
};

static const u8 tda18272_8mhz_vh_dvbt2[] =
{
	0x15, 0x50,
	0x13, 0x22,
	0x23, 0x01,
	0x0F, 0xBB,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_8mhz_uhf_dvbt2[]=
{
	0x15, 0x50,
	0x13, 0x22,
	0x23, 0x01,
	0x0F, 0xBC,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_7mhz_vh_dvbt2[] =
{
	0x23, 0x03,
	0x15, 0x50,
	0x13, 0x72,
	0x0F, 0xBB,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_7mhz_uhf_dvbt2[] =
{
	0x23, 0x03,
	0x15, 0x50,
	0x13, 0x72,
	0x0F, 0xBC,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_6mhz_vh_dvbt2[] =
{
	0x15, 0x41,
	0x13, 0x20,
	0x23, 0x03,
	0x0F, 0xBB,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_6mhz_uhf_dvbt2[] =
{
	0x15, 0x41,
	0x13, 0x20,
	0x23, 0x03,
	0x0F, 0xBC,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_8mhz_vh_dvbt[] =
{
	0x15, 0x53,
	0x13, 0x22,
	0x23, 0x01,
	0x0F, 0xBB,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_8mhz_uhf_dvbt[] =
{
	0x15, 0x53,
	0x13, 0x22,
	0x23, 0x01,
	0x0F, 0xBC,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_7mhz_vh_dvbt[] =
{
	0x15, 0x4A,
	0x13, 0x31,
	0x23, 0x01,
	0x0F, 0xBB,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_7mhz_uhf_dvbt[] =
{
	0x15, 0x4A,
	0x13, 0x31,
	0x23, 0x01,
	0x0F, 0xBC,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_6mhz_vh_dvbt[] =
{
	0x15, 0x47,
	0x13, 0x20,
	0x23, 0x03,
	0x0F, 0xBB,
	0x06, 0x00,
	0xFF, 0xFF
};

static const u8 tda18272_6mhz_uhf_dvbt[] =
{
	0x15, 0x47,
	0x13, 0x20,
	0x23, 0x03,
	0x0F, 0xBC,
	0x06, 0x00,
	0xFF, 0xFF
};

/* ************************************************** */

static int tda18272_i2c_read(struct tda18272_state *state, u32 index, int num, u8 *buf)
{
	int res = 0;
	u8  bytes[1];
	u8  dstr[512];

	dprintk(200, "%s: index 0x%02x %d\n", __func__, index, num);

	bytes[0] = (u8) index;
	res |= state->config->i2c_readwrite(state->config->demod, state->config->i2c_addr, 0, bytes, 1);

	if (res != 0)
	{
		dprintk(1, "%s: writing data failed (%d)\n", __func__, res);
		return res;
	}
	res |= state->config->i2c_readwrite(state->config->demod, state->config->i2c_addr, 1, buf, num);

	if (res != 0)
	{
		dprintk(1, "%s: reading data failed (%d)\n", __func__, res);
		return res;
	}
	{
		u8 i;

		dstr[0] = '\0';
		for (i = 0; i < num; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, buf[i]);
		}
		dprintk(200, "%s(): n: %u r: 0x%02x b: %s\n", __func__, num, index, dstr);
	}
	return 0;
}

static int tda18272_i2c_write(struct tda18272_state *state, u32 index, u32 num, u8 *buf)
{
	int res = 0;
	u8  bytes[256];
	int i;

	bytes[0] = (u8)index;
	for (i = 0; i < num; i++)
	{
		bytes[i + 1] = (u8) buf[i];
	}
	res |= state->config->i2c_readwrite(state->config->demod, state->config->i2c_addr, 0, bytes, num + 1);

	if (res != 0)
	{
		dprintk(1, "%s: writing data failed (%d)\n", __func__, res);
		return res;
	}
	return 0;
}

static int tda18272_i2c_write_bulk(struct tda18272_state *state, const u8 *bytes)
{
	int res = 0;
	u8  buf[64];
	u8  a = 0xFF;
	u8  r = 0;

/* funny function ;)
 * this function writes the bytes in bytes array. if the even bytes
 * (the registers) are sequenced registers, the bytes are not written
 * directly but sent as one sequence:
 * [0] = 0x00 [2] = 0x01 and so on ->sequences in this case the bytes
 * in bytes[odd] are buffered and written together.
 * if the even byte is 0xff the even and next odd is ignored :-/
 * not sure what's the sense of this...
 */
	dprintk(10, "%s: >\n", __func__);

	for (; ; bytes += 2)
	{
		/* a + r ->last register + 1
		 * bytes[0] current register
		 */
		if ( a + r != bytes[0])
		{
			if (a != 0xFF)
			{
				res |= tda18272_i2c_write(state, a, r, buf);

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
	dprintk(10, "%s: < res %d\n", __func__, res);
	return res;
}
/* **************** end i2c ***************************** */

static int tda18272_wait_irq(struct dvb_frontend *fe)
{
	struct tda18272_state *state = fe->tuner_priv;
	int                    res = 0;
	u32                    cnt;
	u8                     byte;

	dprintk(10, "%s: >\n", __func__);

	if (state->config->iic_mode == 2)
	{
		msleep(100);
	}
	else
	{
		for (cnt = 40; cnt--;)
		{
			res |= tda18272_i2c_read(state, 0x08, 1, &byte);

			if (res != 0)
			{
				dprintk(1, "%s: reading data failed (%d)\n", __func__, res);
				return res;
			}
			if (0x80 & byte)
			{
				break;
			}
			msleep(30);
		}
		if (!cnt)
		{
			res = -1;
			dprintk(1, "%s: timeout\n", __func__);
			}
	}
	dprintk(10, "%s: < res %d\n", __func__, res);
	return res;
}

static int tda18272_sleep(struct dvb_frontend *fe)
{
	struct tda18272_state *state = fe->tuner_priv;
	int                   res = 0;

	dprintk(10, "%s: >\n", __func__);

	if (state->config->stdby)
	{
		if (state->config->stdby == 1)
		{
			res |= tda18272_i2c_write_bulk(state, tda18272_power_d1);
			state->power = 1;
		}
		else if (state->config->stdby == 2)
		{
			res |= tda18272_i2c_write_bulk(state, tda18272_power_d2);
			state->power = 2;
		}
		else if (state->config->stdby == 3)
		{
			res |= tda18272_i2c_write_bulk(state, tda18272_power_d3);
			state->power = 3;
		}
		else
		{
			res |= -EINVAL;
		}
	}
	dprintk(10, "%s: < res %d\n", __func__, res);
	return res;
}

static int tda18272_set_params_dvbt2(struct dvb_frontend *fe, struct dvb_frontend_parameters *params)
{
	struct tda18272_state *state = fe->tuner_priv;
	u8                    bytes[8];
	int                   res = 0;
	const u8              *p_bytes = NULL;
	u32                   f = (params->frequency / 1000);

	dprintk(10, "%s: >\n", __func__);

	if (state->power)
	{
		res |= tda18272_i2c_write_bulk(state, tda18272_power_d0);
		state->power = 0;

		if (res != 0)
		{
			dprintk(1, "%s: writing data failed (%d)\n", __func__, res);
			return res;
		}
	}
	if (params->frequency < 291000000)
	{
		if (params->u.ofdm.bandwidth == BANDWIDTH_8_MHZ)
		{
			p_bytes = tda18272_8mhz_vh_dvbt2;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_7mhz_vh_dvbt2;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_6mhz_vh_dvbt2;
		}
		else
		{
			res |= -EINVAL;
		}
	}
	else
	{
		if (params->u.ofdm.bandwidth == BANDWIDTH_8_MHZ)
		{
			p_bytes = tda18272_8mhz_uhf_dvbt2;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_7mhz_uhf_dvbt2;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_6mhz_uhf_dvbt2;
		}
		else
		{
			res |= -EINVAL;
		}
	}
	if (res == 0)
	{
		res |= tda18272_i2c_write_bulk(state, p_bytes);
	}
	else
	{
		dprintk(1, "%s: bandwidth wrong value\n", __func__);
		return res;
	}
	if (res != 0)
	{
		dprintk(1, "%s: error setting bandwidth\n", __func__);
		return res;
	}

	bytes[0] = (u8)((f >> 16) & 0xFF);
	bytes[1] = (u8)((f >>  8) & 0xFF);
	bytes[2] = (u8)((f      ) & 0xFF);
	bytes[3] = (u8)(0xC1);
	bytes[4] = (u8)(0x01);
	res |= tda18272_i2c_write(state, 0x16, 5, bytes);

	if (res != 0)
	{
		dprintk(1, "%s: Write failed res %d\n", __func__, res);
		return res;
	}
	state->freq = params->frequency;
	res |= tda18272_wait_irq(fe);
	if (res != 0)
	{
		dprintk(1, "%s: wait irq failed %d\n", __func__, res);
		return res;
	}
	if (state->config->iic_mode == 2)
	{
		state->rssi = 0;
	}
	else
	{
		msleep(60);

		res |= tda18272_i2c_read(state, 0x07, 1, bytes);

		if (res != 0)
		{
			dprintk(1, "%s: read failed %d\n", __func__, res);
			return res;
		}
		state->rssi = (u16)((bytes[0] << 8));
	}
	dprintk(10, "%s: < %d\n", __func__, res);
	return res;
}

static int tda18272_set_params_dvbt(struct dvb_frontend *fe, struct dvb_frontend_parameters *params)
{
	struct tda18272_state *state = fe->tuner_priv;
	u8                    bytes[8];
	int                   res = 0;
	const u8              *p_bytes = NULL;
	u32                   f = (params->frequency / 1000);

	if (state->power)
	{
		res |= tda18272_i2c_write_bulk(state, tda18272_power_d0);
		state->power = 0;

		if (res != 0)
		{
			dprintk(1, "%s: writing data failed (%d)\n", __func__, res);
			return res;
		}
	}
	if (params->frequency < 291000000)
	{
		if (params->u.ofdm.bandwidth == BANDWIDTH_8_MHZ)
		{
			p_bytes = tda18272_8mhz_vh_dvbt;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_7mhz_vh_dvbt;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_6mhz_vh_dvbt;
		}
		else
		{
			res |= -EINVAL;
		}
	}
	else
	{
		if (params->u.ofdm.bandwidth == BANDWIDTH_8_MHZ)
		{
			p_bytes = tda18272_8mhz_uhf_dvbt;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_7mhz_uhf_dvbt;
		}
		else if (params->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			p_bytes = tda18272_6mhz_uhf_dvbt;
		}
		else
		{
			res |= -EINVAL;
		}
	}
	if (res == 0)
	{
		res |= tda18272_i2c_write_bulk(state, p_bytes);
	}
	else
	{
		dprintk(1, "%s: bandwidth wrong value\n", __func__);
		return res;
	}
	if (res != 0)
	{
		dprintk(1, "%s: error setting bandwidth\n", __func__);
		return res;
	}
	bytes[0] = (u8)((f >> 16) & 0xFF);
	bytes[1] = (u8)((f >>  8) & 0xFF);
	bytes[2] = (u8)((f      ) & 0xFF);
	bytes[3] = (u8)(0xC1);
	bytes[4] = (u8)(0x01);
	res |= tda18272_i2c_write(state, 0x16, 5, bytes);

	if (res != 0)
	{
		dprintk(1, "%s: Write failed res %d\n", __func__, res);
		return res;
	}
	state->freq = params->frequency;
	res |= tda18272_wait_irq(fe);

	if (res != 0)
	{
		dprintk(1, "%s: wait irq failed %d\n", __func__, res);
		return res;
	}
	if (state->config->iic_mode == 2)
	{
		state->rssi = 0;
	}
	else
	{
		msleep(60);
		res |= tda18272_i2c_read(state, 0x07, 1, bytes);
		if (res != 0)
		{
			dprintk(1, "%s: read failed %d\n", __func__, res);
			return res;
		}
		state->rssi = (u16)((bytes[0] << 8));
	}
	dprintk(10, "%s: < %d\n", __func__, res);
	return res;
}

static int tda18272_init(struct dvb_frontend *fe)
{
	struct tda18272_state *state = fe->tuner_priv;
	int                   res = 0;

	dprintk(10, "%s: >\n", __func__);

	if (state->initDone)
	{
		dprintk(1, "%s: init already done; ignoring\n", __func__);
		return 0;
	}
	res |= tda18272_i2c_write_bulk(state, tda18272_power_d0);
	state->power = 0;

	res |= tda18272_i2c_write_bulk(state, tda18272_init_dvbt2);
	res |= tda18272_i2c_write_bulk(state, state->config->lt ? tda18272_lt_on_dvb_t2 : tda18272_lt_off_dvb_t2);
	res |= tda18272_i2c_write_bulk(state, state->config->xtout ? tda18272_xtout_on : tda18272_xtout_off);
	res |= tda18272_wait_irq(fe);

	if (res != 0)
	{
		dprintk(1, "%s: init failed %d\n", __func__, res);
	}
	state->initDone  = 1;
	dprintk(10, "%s: < %d\n", __func__, res);
	return res;
}

static int tda18272_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct tda18272_state *state = fe->tuner_priv;
	int                   res = 0;

	dprintk(10, "%s: >\n", __func__);
	*frequency = state->freq;
	dprintk(10, "%s: < res %d, frequ %d\n", __func__, res, *frequency);
	return res;
}


static int tda18272_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct tda18272_state *state = fe->tuner_priv;
	int                   res = 0;

	dprintk(100, "%s: >\n", __func__);
	*bandwidth = state->bw;
	dprintk(10, "%s: < res = %d, bw = %d\n", __func__, res, *bandwidth);
	return res;
}

static int tda18272_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct tda18272_state *state = fe->tuner_priv;
	int                   res = 0;
	u8                    bytes[3] = {0,1,0};

	dprintk(100, "%s: >\n", __func__);

	*status = 0;

	res |= tda18272_i2c_read(state, 0x05, 0x01, &bytes[1]); //lo1
	res |= tda18272_i2c_read(state, 0x08, 1,    &bytes[2]); //lo2
	res |= tda18272_i2c_read(state, 0x03, 1,    &bytes[0]);

	*status = bytes[0] << 16;

	dprintk(10, "%s: < res = %d, status = %d\n", __func__, res, *status);
	return res;
}

static int tda18272_release(struct dvb_frontend *fe)
{
	int res = 0;

	/* noop ? */
	return res;
}

static struct dvb_tuner_ops tda18272_tuner_ops =
{
	.info          =
	{
		.name           = "NXP TDA18272",
		.frequency_min  = 47000000,
		.frequency_max  = 892000000,
		.frequency_step = 62500,
		.bandwidth_min  = 6000000,
		.bandwidth_max  = 7000000,
		.bandwidth_step = 1000000
	},
	.release       = tda18272_release,
	.init          = tda18272_init,
	.sleep         = tda18272_sleep,
	.set_params    = tda18272_set_params_dvbt2,
	.get_frequency = tda18272_get_frequency,
	.get_bandwidth = tda18272_get_bandwidth,
	.get_status    = tda18272_get_status,
};

int tda18272_attach(struct dvb_frontend *fe, struct tda18272_private_data_s *tda18272, u8 i2c_addr, int (*i2c_readwrite)(void* p, u8 i2c_addr, u8 read, u8* pbytes, u32 nbytes), void* private)
{
	struct tda18272_state *state = kmalloc(sizeof(struct tda18272_state), GFP_KERNEL);
	struct tda18272_config *cfg = kmalloc(sizeof(struct tda18272_config), GFP_KERNEL);

	dprintk(100, "%s >\n", __func__);

	memcpy(&fe->ops.tuner_ops, &tda18272_tuner_ops, sizeof(struct dvb_tuner_ops));
	fe->tuner_priv = state;

	cfg->lt       = tda18272->lt;
	cfg->stdby    = tda18272->stdby;
	cfg->iic_mode = tda18272->iic_mode;
	cfg->xtout    = tda18272->xtout;
	cfg->i2c_addr = i2c_addr;

	state->config = cfg;
	state->config->i2c_readwrite = i2c_readwrite;
	state->config->demod         = private;

	state->power     = 1;
	state->initDone  = 0;

	dprintk(100, "%s <\n", __func__);

	return 0;
}

int tda18272_setup_dvbt2(struct dvb_frontend *fe)
{
	fe->ops.tuner_ops.set_params = tda18272_set_params_dvbt2;
	return 0;
}

int tda18272_setup_dvbt(struct dvb_frontend *fe)
{
	fe->ops.tuner_ops.set_params = tda18272_set_params_dvbt;
	return 0;
}
// vim:ts=4
