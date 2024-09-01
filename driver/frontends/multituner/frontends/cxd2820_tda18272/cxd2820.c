/*
 * cxd2820 - Sony CXD2820 DVB-T2/T Demodulator driver
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
#include "frontend_platform.h"
#include "socket.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[cxd2820] "

extern int tda18272_attach(struct dvb_frontend *fe, struct tda18272_private_data_s *tda18272, u8 i2c_addr, int (*i2c_readwrite)(void *p, u8 i2c_addr, u8 read, u8 *pbytes, u32 nbytes), void *private);
extern int tda18272_setup_dvbt(struct dvb_frontend *fe);
extern int tda18272_setup_dvbt2(struct dvb_frontend *fe);

/* ****************************************** */
/* saved platform config                      */
static struct platform_frontend_config_s *frontend_cfg = NULL;
struct tda18272_private_data_s tda18272;
struct cxd2820_private_data_s  cxd2820;

#define cMaxSockets 4
static u8 numSockets = 0;
static struct socket_s socketList[cMaxSockets];

struct cxd2820_config
{
	u8               tuner_no;
	struct stpio_pin *tuner_enable_pin;
	u32              tuner_active_lh;
	u32              demod_address;
	u32              tuner_address;
	u8               ts_out; // parallel
	u8               si;     // inversion_on
};

struct cxd2820_state
{
	struct i2c_adapter      *i2c;
	u8                      i2c_address;
	struct dvb_frontend_ops ops;
	struct dvb_frontend     frontend;

	u32                     ber;
	u32                     ucb;
	u32                     si;
	u32                     bw;

	struct cxd2820_config   *config;
};

/* ****************************************** */
#define INRANGE(x, y, z) (((x)<=(y) && (y)<=(z))||((z)<=(y) && (y)<=(x)))
static u32 _lookup(s32 val, int table[][2], u32 size)
{
	u32  ret;
	u32  i;

	ret = table[0][1];
	size = ( size / (sizeof(u32) << 1)) - 1;

	if (INRANGE(table[size][0], val, table[0][0]))
	{
		for( i = 0; i < size; i++)
		{
			if (INRANGE(table[i + 1][0], val, table[i][0]))
			{
				ret = table[i + 1][1] + (table[i][1] - table[i + 1][1]) *
				      (val- table[i + 1][0]) / (table[i][0] - table[i + 1][0]);
				break;
			}
		}
	}
	else
	{
		ret = (val > table[0][0]) ? table[0][1] : table[size][1];
	}
	return ret;
}

static int cxd2820_snr_lut_dvbt2[][2]=
{
	{ 8192, 3010 },
	{ 8064, 3003 },
	{ 7936, 2997 },
	{ 7808, 2989 },
	{ 7680, 2982 },
	{ 7552, 2975 },
	{ 7424, 2968 },
	{ 7296, 2960 },
	{ 7168, 2952 },
	{ 7040, 2944 },
	{ 6912, 2937 },
	{ 6784, 2928 },
	{ 6656, 2920 },
	{ 6528, 2912 },
	{ 6400, 2903 },
	{ 6272, 2894 },
	{ 6144, 2885 },
	{ 6016, 2876 },
	{ 5888, 2867 },
	{ 5760, 2857 },
	{ 5632, 2848 },
	{ 5504, 2838 },
	{ 5376, 2827 },
	{ 5248, 2817 },
	{ 5120, 2806 },
	{ 4992, 2795 },
	{ 4864, 2784 },
	{ 4736, 2772 },
	{ 4608, 2760 },
	{ 4480, 2748 },
	{ 4352, 2736 },
	{ 4224, 2723 },
	{ 4096, 2709 },
	{ 3968, 2695 },
	{ 3840, 2681 },
	{ 3712, 2667 },
	{ 3584, 2651 },
	{ 3456, 2635 },
	{ 3328, 2619 },
	{ 3200, 2602 },
	{ 3072, 2584 },
	{ 2944, 2566 },
	{ 2816, 2547 },
	{ 2688, 2526 },
	{ 2560, 2505 },
	{ 2432, 2483 },
	{ 2304, 2459 },
	{ 2176, 2435 },
	{ 2048, 2408 },
	{ 1920, 2380 },
	{ 1792, 2350 },
	{ 1664, 2318 },
	{ 1536, 2283 },
	{ 1408, 2246 },
	{ 1280, 2204 },
	{ 1152, 2158 },
	{ 1024, 2107 },
	{  896, 2049 },
	{  768, 1982 },
	{  640, 1903 },
	{  512, 1806 },
	{  384, 1681 },
	{  256, 1505 },
	{  128, 1204 },
	{    8,    0 }
};

static int cxd2820_snr_lut_dvbt[][2] =
{
	{ 8192, 3010 },
	{ 8064, 3003 },
	{ 7936, 2997 },
	{ 7808, 2989 },
	{ 7680, 2982 },
	{ 7552, 2975 },
	{ 7424, 2968 },
	{ 7296, 2960 },
	{ 7168, 2952 },
	{ 7040, 2944 },
	{ 6912, 2937 },
	{ 6784, 2928 },
	{ 6656, 2920 },
	{ 6528, 2912 },
	{ 6400, 2903 },
	{ 6272, 2894 },
	{ 6144, 2885 },
	{ 6016, 2876 },
	{ 5888, 2867 },
	{ 5760, 2857 },
	{ 5632, 2848 },
	{ 5504, 2838 },
	{ 5376, 2827 },
	{ 5248, 2817 },
	{ 5120, 2806 },
	{ 4992, 2795 },
	{ 4864, 2784 },
	{ 4736, 2772 },
	{ 4608, 2760 },
	{ 4480, 2748 },
	{ 4352, 2736 },
	{ 4224, 2723 },
	{ 4096, 2709 },
	{ 3968, 2695 },
	{ 3840, 2681 },
	{ 3712, 2667 },
	{ 3584, 2651 },
	{ 3456, 2635 },
	{ 3328, 2619 },
	{ 3200, 2602 },
	{ 3072, 2584 },
	{ 2944, 2566 },
	{ 2816, 2547 },
	{ 2688, 2526 },
	{ 2560, 2505 },
	{ 2432, 2483 },
	{ 2304, 2459 },
	{ 2176, 2435 },
	{ 2048, 2408 },
	{ 1920, 2380 },
	{ 1792, 2350 },
	{ 1664, 2318 },
	{ 1536, 2283 },
	{ 1408, 2246 },
	{ 1280, 2204 },
	{ 1152, 2158 },
	{ 1024, 2107 },
	{  896, 2049 },
	{  768, 1982 },
	{  640, 1903 },
	{  512, 1806 },
	{  384, 1681 },
	{  256, 1505 },
	{  128, 1204 },
	{    8,    0 }
};


static const u8 cxd2820_power_d3[] =
{
	0x00, 0x00,
	0xFE, 0x01,
	0x85, 0x00,
	0x88, 0x01,
	0x81, 0x00,
	0x80, 0x00,
	0x00, 0x20,
	0x69, 0x00,
	0xFF, 0xFF
};

static u8 cxd2820_init_val_dvbt2[] =
{
	0x00, 0x00,
	0xCB, 0x08,
	0x89, 0x55,
	0x8B, 0xBA,
	0x00, 0x20,
	0xCB, 0x08,
	0xFF, 0xFF
};

static u8 cxd2820_spectrum_nor_dvbt2[] =
{
	0x00, 0x20,
	0xB5, 0x00,
	0xFF, 0xFF
};

static u8 cxd2820_spectrum_inv_dvbt2[] =
{
	0x00, 0x20,
	0xB5, 0x10,
	0xFF, 0xFF
};

static const u8 cxd2820_reset_dvbt2[]=
{
	0x00, 0x00,
	0x80, 0x02,
	0x81, 0x20,
	0x85, 0x07,
	0x88, 0x02,
	0x00, 0x20,
	0x69, 0x01,
	0xFF, 0xFF
};

static const u8 cxd2820_hp_dvbt2[] =
{
	0x00, 0x23,
	0xAD, 0x00,
	0xFF, 0xFF
};

static const u8 cxd2820_lp_dvbt2[] =
{
	0x00, 0x23,
	0xAD, 0x01,
	0xFF, 0xFF
};

static const u8 cxd2820_bw8_dvbt2[]=
{
	0x00, 0x00,
	0x82, 0xB6,
	0x00, 0x20,
	0xA5, 0x00,
	0x9F, 0x11,
	0xA0, 0xF0,
	0xA1, 0x00,
	0xA2, 0x00,
	0xA3, 0x00,
	0xB6, 0x18,
	0xB7, 0xF9,
	0xB8, 0xC2,
	0xD7, 0x02,
	0x82, 0x0A,
	0x83, 0x0A,
	0x95, 0x1A,
	0x96, 0x50,
	0x00, 0x25,
	0x67, 0x07,
	0x69, 0x03,
	0x00, 0x2A,
	0x8C, 0x00,
	0x8D, 0x34,
	0x00, 0x27,
	0xE6, 0x14,
	0x00, 0x3F,
	0x10, 0x0D,
	0x11, 0x02,
	0x12, 0x01,
	0x23, 0x2C,
	0x51, 0x13,
	0x52, 0x01,
	0x53, 0x00,
	0x00, 0x27,
	0x45, 0x0F,
	0x46, 0x08,
	0x47, 0x06,
	0x48, 0x06,
	0x86, 0x22,
	0x87, 0x30,
	0xEF, 0x15,
	0x00, 0x2A,
	0x45, 0x06,
	0x00, 0x20,
	0x6F, 0x18,
	0x72, 0x18,
	0x00, 0x20,
	0xC2, 0x03,
	0x00, 0x00,
	0xFF, 0x18,
	0x00, 0x00,
	0xFE, 0x01,
	0x00, 0x20,
	0xE0, 0x0F,
	0xE1, 0xFF,
	0xFF, 0xFF
};

static const u8 cxd2820_bw7_dvbt2[] =
{
	0x00, 0x00,
	0x82, 0xB6,
	0x00, 0x20,
	0xA5, 0x00,
	0x9F, 0x14,
	0xA0, 0x80,
	0xA1, 0x00,
	0xA2, 0x00,
	0xA3, 0x00,
	0xB6, 0x18,
	0xB7, 0xF9,
	0xB8, 0xC2,
	0xD7, 0x42,
	0x82, 0x0A,
	0x83, 0x0A,
	0x95, 0x1A,
	0x96, 0x50,
	0x00, 0x25,
	0x67, 0x07,
	0x69, 0x03,
	0x00, 0x2A,
	0x8C, 0x00,
	0x8D, 0x34,
	0x00, 0x27,
	0xE6, 0x14,
	0x00, 0x3F,
	0x10, 0x0D,
	0x11, 0x02,
	0x12, 0x01,
	0x23, 0x2C,
	0x51, 0x13,
	0x52, 0x01,
	0x53, 0x00,
	0x00, 0x27,
	0x45, 0x0F,
	0x46, 0x08,
	0x47, 0x06,
	0x48, 0x06,
	0x86, 0x22,
	0x87, 0x30,
	0xEF, 0x15,
	0x00, 0x2A,
	0x45, 0x06,
	0x00, 0x20,
	0x6F, 0x18,
	0x72, 0x18,
	0x00, 0x20,
	0xC2, 0x03,
	0x00, 0x00,
	0xFF, 0x18,
	0x00, 0x00,
	0xFE, 0x01,
	0x00, 0x20,
	0xE0, 0x0F,
	0xE1, 0xFF,
	0xFF, 0xFF
};

static const u8 cxd2820_bw6_dvbt2[] =
{
	0x00, 0x00,
	0x82, 0xB6,
	0x00, 0x20,
	0xA5, 0x00,
	0x9F, 0x17,
	0xA0, 0xEA,
	0xA1, 0xAA,
	0xA2, 0xAA,
	0xA3, 0xAA,
	0xB6, 0x15,
	0xB7, 0xDA,
	0xB8, 0x89,
	0xD7, 0x82,
	0x82, 0x0A,
	0x83, 0x0A,
	0x95, 0x1A,
	0x96, 0x50,
	0x00, 0x25,
	0x67, 0x07,
	0x69, 0x03,
	0x00, 0x2A,
	0x8C, 0x00,
	0x8D, 0x34,
	0x00, 0x27,
	0xE6, 0x14,
	0x00, 0x3F,
	0x10, 0x0D,
	0x11, 0x02,
	0x12, 0x01,
	0x23, 0x2C,
	0x51, 0x13,
	0x52, 0x01,
	0x53, 0x00,
	0x00, 0x27,
	0x45, 0x0F,
	0x46, 0x08,
	0x47, 0x06,
	0x48, 0x06,
	0x86, 0x22,
	0x87, 0x30,
	0xEF, 0x15,
	0x00, 0x2A,
	0x45, 0x06,
	0x00, 0x20,
	0x6F, 0x18,
	0x72, 0x18,
	0x00, 0x20,
	0xC2, 0x03,
	0x00, 0x00,
	0xFF, 0x18,
	0x00, 0x00,
	0xFE, 0x01,
	0x00, 0x20,
	0xE0, 0x0F,
	0xE1, 0xFF,
	0xFF, 0xFF
};

static const u8 cxd2820_ts_serial[] =
{
	0x00, 0x00,
	0x70, 0x08,
	0x00, 0x20,
	0x70, 0x08,
	0xFF, 0xFF
};

static const u8 cxd2820_ts_parallel[] =
{
	0x00, 0x00,
	0x70, 0x38,
	0x00, 0x20,
	0x70, 0x38,
	0xFF, 0xFF
};

static u8 cxd2820_init_val_dvbt[] =
{
	0x00, 0x27,
	0x48, 0x06,
	0x00, 0x00,
	0xCB, 0x08,
	0x89, 0x55,
	0x8B, 0xBA,
	0x00, 0x20,
	0xCB, 0x08,
	0xFF, 0xFF
};

static const u8 cxd2820_reset_dvbt[] =
{
	0x00, 0x00,
	0xD3, 0x00,
	0x00, 0x04,
	0x10, 0x00,
	0x00, 0x00,
	0x80, 0x00,
	0x81, 0x03,
	0x85, 0x07,
	0x88, 0x02,
	0xFF, 0xFF
};

static const u8 cxd2820_hp_dvbt[] =
{
	0x00, 0x04,
	0x10, 0x00,
	0x00, 0x00,
	0xFF, 0xFF
};

static const u8 cxd2820_lp_dvbt[] =
{
	0x00, 0x04,
	0x10, 0x01,
	0x00, 0x00,
	0xFF, 0xFF
};

static const u8 cxd2820_bw8_dvbt[] =
{
	0x00, 0x00,
	0xA5, 0x00,
	0x82, 0xB6,
	0xC2, 0x11,
	0x00, 0x01,
	0x6A, 0x50,
	0x00, 0x04,
	0x27, 0x41,
	0x00, 0x00,
	0x9F, 0x11,
	0xA0, 0xF0,
	0xA1, 0x00,
	0xA2, 0x00,
	0xA3, 0x00,
	0x9F, 0x11,
	0xB6, 0x19,
	0xB7, 0xE9,
	0xB8, 0x85,
	0xD7, 0x03,
	0xD9, 0x01,
	0xDA, 0xE0,
	0x00, 0x00,
	0xFF, 0x18,
	0x00, 0x00,
	0xFE, 0x01,
	0x00, 0x00,
	0xFF, 0xFF
};

static const u8 cxd2820_bw7_dvbt[] =
{
	0x00, 0x00,
	0xA5, 0x00,
	0x82, 0xB6,
	0x00, 0x01,
	0x6A, 0x50,
	0x00, 0x04,
	0x27, 0x41,
	0x00, 0x00,
	0x9F, 0x14,
	0xA0, 0x80,
	0xA1, 0x00,
	0xA2, 0x00,
	0xA3, 0x00,
	0x9F, 0x14,
	0xB6, 0x19,
	0xB7, 0xE9,
	0xB8, 0x85,
	0xD7, 0x43,
	0xD9, 0x12,
	0xDA, 0xF8,
	0x00, 0x00,
	0xFF, 0x18,
	0x00, 0x00,
	0xFE, 0x01,
	0x00, 0x00,
	0xFF, 0xFF
};

static const u8 cxd2820_bw6_dvbt[] =
{
	0x00, 0x00,
	0xA5, 0x00,
	0x82, 0xB6,
	0x00, 0x01,
	0x6A, 0x50,
	0x00, 0x04,
	0x27, 0x41,
	0x00, 0x00,
	0x9F, 0x17,
	0xA0, 0xEA,
	0xA1, 0xAA,
	0xA2, 0xAA,
	0xA3, 0xAA,
	0x9F, 0x17,
	0xB6, 0x19,
	0xB7, 0xE9,
	0xB8, 0x85,
	0xD7, 0x83,
	0xD9, 0x1F,
	0xDA, 0xDC,
	0x00, 0x00,
	0xFF, 0x18,
	0x00, 0x00,
	0xFE, 0x01,
	0x00, 0x00,
	0xFF, 0xFF
};

/* ****************** get_frontend values ************ */

static const u32 cxd2820_fft[8] =
{
	TRANSMISSION_MODE_2K,
	TRANSMISSION_MODE_8K,
	TRANSMISSION_MODE_AUTO,  // TRANSMISSION_MODE_4K
	TRANSMISSION_MODE_AUTO,  // TRANSMISSION_MODE_1K
	TRANSMISSION_MODE_AUTO,  // TRANSMISSION_MODE_16K
	TRANSMISSION_MODE_AUTO,  // TRANSMISSION_MODE_32K
	TRANSMISSION_MODE_8K,
	TRANSMISSION_MODE_AUTO   // TRANSMISSION_MODE_32K
};

static const u32 cxd2820_gi[8] =
{
	GUARD_INTERVAL_1_32,
	GUARD_INTERVAL_1_16,
	GUARD_INTERVAL_1_8,
	GUARD_INTERVAL_1_4,
	GUARD_INTERVAL_AUTO, // 1_128
	GUARD_INTERVAL_AUTO, // 19_128
	GUARD_INTERVAL_AUTO, // 19_256
	GUARD_INTERVAL_AUTO
};

static const u32 cxd2820_mod[8] =
{
	QPSK,
	QAM_16,
	QAM_64,
	QAM_256
};

static const u32 cxd2820_cr[8] =
{
	FEC_1_2,
	FEC_2_3,
	FEC_3_4,
	FEC_5_6,
	FEC_7_8,
	FEC_AUTO,
	FEC_AUTO,
	FEC_AUTO
};

static const u32 cxd2820_cr_plp[8] =
{
	FEC_1_2,
	FEC_3_5,
	FEC_2_3,
	FEC_3_4,
	FEC_4_5,
	FEC_5_6,
	FEC_AUTO,
	FEC_AUTO
};


/* ********************* i2c *********************** */

static int cxd2820_i2c_write(struct cxd2820_state *state, u32 index, u32 num, const u8 *buf)
{
	int    res = 0, i;
	u8     bytes[256];
	struct i2c_msg msg[2];

	if (index >> 8)
	{
		if (index >= 0xff00)
		{
			index &= 0xff;
		}
		bytes[0] = 0x00;
		bytes[1] = (u8) ( index >> 8);

		dprintk(200, "%s: 0x%02x 0x%02x\n", __func__, bytes[0], bytes[1]);

		/* write */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = 0;
		msg[0].buf   = bytes;
		msg[0].len   = 2;

		if ((res = i2c_transfer(state->i2c, msg, 1)) != 1)
		{
			dprintk(1, "%s: 1. error on i2c_transfer (%d)\n", __func__, res);
			return res;
		}
	}
	bytes[0] = (u8) index;
	for (i = 0; i < num; i++)
	{
		bytes[i + 1] = buf[i];
	}
	{
		u8 dstr[512];

		dstr[0] = '\0';
		for (i = 0; i < num + 1; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, bytes[i]);
		}
		dprintk(200, "%s(): n: %u b: %s\n", __func__, num + 1, dstr);
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

static u8 cxd2820_i2c_read(struct cxd2820_state *state, u32 index, u32 num, u8 *buf)
{
	struct i2c_msg msg[2];
	int    ret = 0;
	u8     bytes[1];

	dprintk(200, "%s: index 0x%02x %d\n", __func__, index, num);
	bytes[0] = (u8) index;

	if (index >> 8)
	{
		if (index >= 0xff00)
		{
			index &= 0xff;
		}
		bytes[0] = 0x00;
		bytes[1] = (u8)(index >> 8);

		dprintk(200, "%s: w: 0x%02x 0x%02x\n", __func__, bytes[0], bytes[1]);

		/* write */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = 0;
		msg[0].buf   = bytes;
		msg[0].len   = 2;

		if ((ret = i2c_transfer(state->i2c, msg, 1)) != 1)
		{
			dprintk(1, "%s: 1. error on i2c_transfer (%d)\n", __func__, ret);
			return ret;
		}
	}
	bytes[0] = (u8) index;
	dprintk(200, "%s: w: 0x%02x\n", __func__, bytes[0]);

	/* write */
	msg[0].addr  = state->i2c_address;
	msg[0].flags = 0;
	msg[0].buf   = bytes;
	msg[0].len   = 1;

	if ((ret = i2c_transfer(state->i2c, msg, 1)) != 1)
	{
		dprintk(1, "%s: 2. error on i2c_transfer (%d)\n", __func__, ret);
		return ret;
	}

	/* read */
	msg[0].addr  = state->i2c_address;
	msg[0].flags = I2C_M_RD;
	msg[0].buf   = buf;
	msg[0].len   = num;

	if ((ret = i2c_transfer(state->i2c, msg, 1)) != 1)
	{
		dprintk(1, "%s: 3. error on i2c_transfer (%d)\n", __func__, ret);
		return ret;
	}
	{
		u8 i;
		u8 dstr[512];

		dstr[0] = '\0';
		for (i = 0; i < num; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, buf[i]);
		}
		dprintk(200, "%s(): n: %u r: 0x%02x b: %s\n", __func__, num, index, dstr);
	}
	return 0;
}

static int cxd2820_i2c_write_bulk(struct cxd2820_state *state, const u8 *bytes)
{
	int  res = 0;
	u8   buf[64];
	u8   a = 0xFF;
	u8   r = 0;

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
		if (a + r != bytes[0])
		{
			if (a != 0xFF)
			{
				res |= cxd2820_i2c_write(state, a, r, buf);
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
		if (bytes[0] == 0xff)
		{
			if (bytes[1] == 0xff)
			{
				break;
			}
			else
			{
				res |= cxd2820_i2c_write(state, *bytes, 1,  bytes+1);
			}
		}
	}
	dprintk(10, "%s: < res %d\n", __func__, res);
	return res;
}

/* ********************* i2c gw *********************** */

static int cxd2820_i2cgw_readwrite(void *p, u8 i2c_addr, u8 read, u8 *pbytes, u32 nbytes)
{
	struct cxd2820_state *state = (struct cxd2820_state*) p;
	int                  res   = 0;
	u32                  addr = i2c_addr;
	u8                   buf[256];
	u8                   dstr[512];
	u32                  i;
	struct               i2c_msg msg[2];

	dprintk(200, "%s >\n", __func__);

	if (read == 0)
	{
		buf[0] = 0x09;
		buf[1] = (u8) addr;

		for (i = 0; i < nbytes; i++)
		{
			buf[2 + i] = pbytes[i];
		}

		dstr[0] = '\0';
	
		for (i = 0; i < nbytes + 2; i++)
		{
			sprintf(dstr, "%s 0x%02x", dstr, buf[i]);
		}
		dprintk(200, "%s(): n: %u b: %s\n", __func__, nbytes + 2, dstr);

		/* write */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = 0;
		msg[0].buf   = buf;
		msg[0].len   = nbytes + 2;

		if ((res = i2c_transfer(state->i2c, msg, 1)) != 1)
		{
			dprintk(1, "%s: 1. error on i2c_transfer (%d)\n", __func__, res);
			return res;
		}
	}
	else
	{
		dprintk(200, "%s read\n", __func__);
		buf[0] = 0x09;
		buf[1] = (u8) addr | 0x01;

		dprintk(200, "%s: writing 0x%02x 0x%02x\n", __func__, buf[0], buf[1]);

		/* write */
		msg[0].addr  = state->i2c_address;
		msg[0].flags = 0;
		msg[0].buf   = buf;
		msg[0].len   = 2;

		/* read */
		msg[1].addr  = state->i2c_address;
		msg[1].flags = I2C_M_RD;
		msg[1].buf   = pbytes;
		msg[1].len   = nbytes;

		if ((res = i2c_transfer(state->i2c, msg, 2)) != 2)
		{
			dprintk(1, "%s: 2. error on i2c_transfer (%d)\n", __func__, res);
			return res;
		}
		{
			u8 i;

			dstr[0] = '\0';
			for (i = 0; i < nbytes; i++)
			{
				sprintf(dstr, "%s 0x%02x", dstr, pbytes[i]);
			}
			dprintk(200, "%s(): n: %d b: %s\n", __func__, nbytes, dstr);
		}
	}
	dprintk(200, "%s < res %d\n", __func__, res);
	return 0;
}
/* ********************* i2c end *********************** */

static int cxd2820_set_ts_out(struct dvb_frontend *fe)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	int res = 0;

	res |= cxd2820_i2c_write_bulk(state, state->config->ts_out ? cxd2820_ts_serial : cxd2820_ts_parallel);
	return res;
}

/* ************************ dvb-t2 functions ********************** */

static int cxd2820_read_status_dvbt2(struct dvb_frontend *fe, fe_status_t *status)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	int res = 0;
	u8 bytes[4];

	*status = 0;

	res |= cxd2820_i2c_read(state, 0x2010 ,1 , bytes);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}

	if (bytes[0] & 0x20)
	{
		*status |= FE_HAS_LOCK;
	}
/* hmm > 0x04 is same as >= 0x06 so we could remove first ? */
	if (bytes[0] >= 0x06)
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC;
	}
	else if (bytes[0] > 0x04)
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC;
	}
	else if (bytes[0] > 0x03)
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI;
	}
	else if (bytes[0] > 0x02)
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER;
	}
	dprintk(100, "%s: < res %d, status %d\n", __func__, res, *status);
	return res;
}

static int cxd2820_read_ber_dvbt2(struct dvb_frontend *fe, u32 *ber)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	u8  bytes[4];
	u32 c, p;
	int res = 0;

	*ber = 0;

	res |= cxd2820_i2c_read(state, 0x2039, 4, bytes);
	c = ((bytes[0] & 0x0F) << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

	res |= cxd2820_i2c_read(state, 0x206F, 1, bytes);
	p = (1 << (bytes[0] & 0x0F));

	res |= cxd2820_i2c_read(state, 0x225E, 1, bytes);
	p *= (bytes[0] & 0x03) ? 64800 : 16200;

	if (res == 0)
	{
		if (c)
		{
			state->ber =  ((100000000) << 4);
			state->ber /= (((p / c) << 4) + ((( p % c) << 4) / c));
		}
		else
		{
			state->ber = c;
		}
	}
	*ber = state->ber;

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
	}
	dprintk(40, "%s: < res %d ber %d\n", __func__, res, *ber);
	return res;
}

static int cxd2820_read_signal_strength_dvbt2(struct dvb_frontend *fe, u16 *signal_strength)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	u8                   bytes[2];
	int                  res = 0;

	*signal_strength = 0;

	res |= cxd2820_i2c_read(state, 0x2026, 2, bytes);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}

	*signal_strength = (u16) (((bytes[0] & 0xF) << 8) | bytes[1]);
	*signal_strength ^= 0xFFFF;

	dprintk(40, "%s: < res %d, singnal_strength %d\n", __func__, res, *signal_strength);
	return res;
}

static int cxd2820_read_snr_dvbt2(struct dvb_frontend *fe, u16 *snr)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	u8  bytes[8] = {0x00, 0x20, 0x01, 0x01, 0xFF, 0xFF, 0x00, 0x00};
	u32 i;
	int res = 0;

	*snr = 0;

	res |= cxd2820_i2c_write_bulk(state, bytes);
	res |= cxd2820_i2c_read(state, 0x28, 2, &bytes[4]);
	i = ((bytes[4] & 0x0F) << 8) | bytes[5];

	res |= cxd2820_i2c_write(state, 0x01, 1, bytes);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	*snr = (u16)_lookup(i, cxd2820_snr_lut_dvbt2, sizeof(cxd2820_snr_lut_dvbt2));

	dprintk(40, "%s: < snr %d\n", __func__, *snr);
	return 0;
}

static int cxd2820_read_ucblocks_dvbt2(struct dvb_frontend *fe, u32 *ucblocks)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	int res = 0;
	u8  bytes[4];

	*ucblocks = 0;

	res |= cxd2820_i2c_read(state, 0x2075, 3, bytes);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	if (bytes[0] & 0x01)
	{
		state->ucb += ((bytes[1] & 0x7F) << 8) | (bytes[2]);
	}
	*ucblocks = state->ucb;
	dprintk(40, "%s: < res %d, ucb %d\n", __func__, res, *ucblocks);
	return res;
}
/* ************************ end dvb-t2 functions ********************** */

/* ************************ dvb-t functions ********************** */

static int cxd2820_read_status_dvbt(struct dvb_frontend *fe, fe_status_t *status)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	int res = 0;
	u8 bytes[4];

	*status = 0;

	res |= cxd2820_i2c_read(state, 0xFF10, 1 , bytes);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}

	if (bytes[0] == 0x06)
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC;
	}
	else if (bytes[0] == 0x05)
	{
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC;
	}
	else
	{
		dprintk(100, "%s: < res %d, status %d\n", __func__, res, *status);
		return res;
	}
	res |= cxd2820_i2c_read(state, 0xFF73, 1, bytes);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	if (bytes[0] & 0x08)
	{
		*status |= FE_HAS_LOCK;
	}
	dprintk(100, "%s: < res %d, status %d\n", __func__, res, *status);
	return res;
}

static int cxd2820_read_ber_dvbt(struct dvb_frontend *fe, u32 *ber)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	u8  bytes[4];
	u32 c, p;
	int res = 0;

	*ber = 0;

	res |= cxd2820_i2c_read(state, 0x76, 3, bytes);
	c = ((bytes[2] & 0x0F) << 16) | (bytes[1] << 8) | bytes[0];

	if (!(bytes[2] & 0x80))
	{
		res |= -1;
	}
	if (res == 0)
	{
		res |= cxd2820_i2c_read(state, 0x72, 1, bytes);
	}
	if (res == 0)
	{
		if (c)
		{
			p = (1 << (bytes[0] & 0x1F)) * 204 * 8;
			state->ber  = ((100 * 1000000) << 4) ;
			state->ber /= ((( p / c) << 4) + ((( p % c) << 4) / c)) ;
		}
		else
		{
			state->ber = c;
		}
	}
	*ber = state->ber;
	dprintk(40, "%s: < res %d ber %d\n", __func__, res, *ber);
	return res;
}

static int cxd2820_read_signal_strength_dvbt(struct dvb_frontend *fe, u16 *signal_strength)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	u8                   bytes[2];
	int                  res = 0;

	*signal_strength = 0;

	res |= cxd2820_i2c_read(state, 0xFF26, 2, bytes);

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	*signal_strength = (u16) (((bytes[0] & 0x0F) << 8) | bytes[1]);
	*signal_strength ^= 0xFFFF;
	dprintk(40, "%s: < res %d, signal_strength %d\n", __func__, res, *signal_strength);
	return res;
}

static int cxd2820_read_snr_dvbt(struct dvb_frontend *fe, u16 *snr)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	u8                   bytes[4];
	u32                  i;
	int                  res = 0;

	*snr = 0;

	res |= cxd2820_i2c_read(state, 0xFF28, 2, &bytes[0]);
	i = ((bytes[0] & 0x1F) << 8) | bytes[1];

	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	*snr = (u16)_lookup(i, cxd2820_snr_lut_dvbt, sizeof(cxd2820_snr_lut_dvbt));
	dprintk(40, "%s: < snr %d\n", __func__, *snr);
	return 0;
}

static int cxd2820_read_ucblocks_dvbt(struct dvb_frontend *fe, u32 *ucblocks)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	int                  res = 0;
	u8                   bytes[4];

	*ucblocks = 0;
	res |= cxd2820_i2c_read(state, 0xFFEA, 2, bytes);
	if (res != 0)
	{
		dprintk(1, "%s: error reading data (%d)\n", __func__, res);
		return res;
	}
	state->ucb += (bytes[0] << 8) | bytes[1];
	*ucblocks = state->ucb;
	dprintk(40, "%s: < res %d, ucb %d\n", __func__, res, *ucblocks);
	return res;
}
/* ************************ end dvb-t functions ********************** */

static void cxd2820_release(struct dvb_frontend *fe)
{
	struct cxd2820_state *state = fe->demodulator_priv;

	dprintk(10, "%s()\n",__func__);

	kfree(state);
}

static struct dvb_frontend_ops cxd2820_ops_dvbt2;

struct dvb_frontend* cxd2820_attach(struct cxd2820_config *config, struct tda18272_private_data_s *tda18272, struct i2c_adapter *i2c)
{
	struct cxd2820_state* state = NULL;

	dprintk(40, "%s: >\n", __func__);

	/* Allocate memory for the internal state */
	state = kmalloc(sizeof(struct cxd2820_state), GFP_KERNEL);
	if (state == NULL)
	{
		dprintk(1, "Unable to kmalloc\n");
		return NULL;
	}
	/* Setup the state used everywhere */
	memset(state, 0, sizeof(struct cxd2820_state));

	state->config      = config;
	state->i2c         = i2c;
	state->i2c_address = config->demod_address;

	memcpy(&state->frontend.ops, &cxd2820_ops_dvbt2, sizeof(struct dvb_frontend_ops));
	state->frontend.demodulator_priv = state;

	tda18272_attach(&state->frontend, tda18272, config->tuner_address, cxd2820_i2cgw_readwrite, state);

	dprintk(40, "%s: <\n", __func__);
	return &state->frontend;
}

static int cxd2820_init_dvbt2(struct dvb_frontend *fe)
{
	struct cxd2820_state* state = fe->demodulator_priv;
	int res = 0;

	dprintk(40, "%s: >\n", __func__);

	res |= cxd2820_i2c_write_bulk(state, cxd2820_power_d3);
	res |= cxd2820_i2c_write_bulk(state, cxd2820_init_val_dvbt2);

	if (state->config->si == INVERSION_OFF)
	{
		state->si = INVERSION_OFF;
		res |= cxd2820_i2c_write_bulk(state, cxd2820_spectrum_nor_dvbt2);
	}
	else
	{
		state->si = INVERSION_ON;
		res |= cxd2820_i2c_write_bulk(state, cxd2820_spectrum_inv_dvbt2);
	}

	res |= cxd2820_i2c_write_bulk(state, cxd2820_reset_dvbt2);
	res |= cxd2820_set_ts_out(fe);
	if (res != 0)
	{
		dprintk(1, "%s: error init dvb-t2 (%d)\n", __func__, res);
		return res;
	}
	dprintk(40, "%s: < res %d\n", __func__, res);
	return res;
}

static int cxd2820_sleep(struct dvb_frontend *fe)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	int                  res = 0;

	dprintk(10, "%s()\n",__func__);

/* fixme: hmm is it really d3 here? orig driver says so */
	res |= cxd2820_i2c_write_bulk(state, cxd2820_power_d3);
	return res;
}

#if DVB_API_VERSION >= 5
static int cxd2820_set_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	dprintk(20, "%s()\n", __func__);
	return 0;
}

static int cxd2820_get_property(struct dvb_frontend *fe, struct dtv_property *tvp)
{
	/* get delivery system info */
	if (tvp->cmd == DTV_DELIVERY_SYSTEM)
	{
		switch (tvp->u.data)
		{
			case SYS_DVBT:
			case SYS_DVBT2:  // was missing (thnx mrspeccy)
			{
				break;
			}
			default:
			{
				return -EINVAL;
			}
		}
	}
	dprintk(20, "%s()\n", __func__);
	return 0;
}
#else
static struct dvbfe_info dvbt_info =
{
	.name = "Sony CXD2820 DVB-T/T2",
	.delivery = DVBFE_DELSYS_DVBT,
	.delsys =
	{
		.dvbt.modulation      = DVBFE_MOD_QAM16
		                      | DVBFE_MOD_QAM64
		                      | DVBFE_MOD_QAM256
		                      | DVBFE_MOD_QAMAUTO,
		.dvbt.stream_priority = DVBFE_STREAM_PRIORITY_HP
		                      | DVBFE_STREAM_PRIORITY_LP,
	},

	.frequency_min            = 47000000,
	.frequency_max            = 862000000,
	.frequency_step           = 62500,
	.frequency_tolerance      = 0,
	.symbol_rate_min          = 5705357,
	.symbol_rate_max          = 7607143
};

static int cxd2820_get_info (struct dvb_frontend *fe, struct dvbfe_info *fe_info)
{
	dprintk (100, "%s\n", __func__);

	switch (fe_info->delivery)
	{
		case DVBFE_DELSYS_DVBT:
		{
			dprintk (10, "%s (DVBT)\n", __func__);
			memcpy (fe_info, &dvbt_info, sizeof (dvbt_info));
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

static int cxd2820_set_frontend_dvbt2(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct cxd2820_state *state = fe->demodulator_priv;
#if DVB_API_VERSION >= 5
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
#else
	struct dvb_ofdm_parameters *op = &p->u.ofdm;
#endif

	int res = 0;
	u8  cnt;
#if DVB_API_VERSION < 5
	dprintk(20, "%s(): qam %x hierar %x hpcode %x lpcode %x guard %x tmode %x BW: %x Freq %d inversion: %d\n",
		__func__, op->constellation, op->hierarchy_information,
		op->code_rate_HP, op->code_rate_LP, op->guard_interval,
		op->transmission_mode, op->bandwidth, p->frequency, p->inversion);
#endif

	/* reset ops to dvb-t2 */
	fe->ops.read_status          = cxd2820_read_status_dvbt2,
	fe->ops.read_ber             = cxd2820_read_ber_dvbt2,
	fe->ops.read_signal_strength = cxd2820_read_signal_strength_dvbt2,
	fe->ops.read_snr             = cxd2820_read_snr_dvbt2,
	fe->ops.read_ucblocks        = cxd2820_read_ucblocks_dvbt2,
	tda18272_setup_dvbt2(fe);

	state->ber = 0;
	state->ucb = 0;

	cxd2820_init_dvbt2(fe);
	msleep(250);

	res = fe->ops.tuner_ops.set_params(fe, p);

	if (res != 0)
	{
		dprintk(1, "%s: Tuner set failed (%d)\n", __func__, res);
		return res;
	}
#if DVB_API_VERSION >= 5
	state->si = (u8) c->inversion;
#else
	state->si = (u8) p->inversion;
#endif
	res |= cxd2820_i2c_write_bulk(state, (state->si == INVERSION_ON) ? cxd2820_spectrum_inv_dvbt2 : cxd2820_spectrum_nor_dvbt2);

	if (res != 0)
	{
		dprintk(1, "%s: 1. Demod set failed (%d)\n", __func__, res);
		return res;
	}

	/* FIXME: the orig driver checks stream_selection here:
	* auto and HI Priority uses hp other lp
	* We have not got such value to differ here currently,
	* let us keep this in mind for later error searching!!!
	*/
	res |= cxd2820_i2c_write_bulk(state, cxd2820_hp_dvbt2);

	if (res != 0)
	{
		dprintk(1, "%s: 2. Demod set failed (%d)\n", __func__, res);
		return res;
	}
	res |= cxd2820_i2c_write_bulk(state, cxd2820_reset_dvbt2);

	if (res != 0)
	{
		dprintk(1, "%s: 3. Demod set failed (%d)\n", __func__, res);
		return res;
	}

/* fixme: dvbapi5 ??? */
	if (p->u.ofdm.bandwidth == BANDWIDTH_8_MHZ)
	{
		res |= cxd2820_i2c_write_bulk(state, cxd2820_bw8_dvbt2);
	}
	else if (p->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
	{
		res |= cxd2820_i2c_write_bulk(state, cxd2820_bw7_dvbt2);
	}
	else if (p->u.ofdm.bandwidth == BANDWIDTH_6_MHZ)
	{
		res |= cxd2820_i2c_write_bulk(state, cxd2820_bw6_dvbt2);
	}
	else
	{
/* FIXME we could make a software automatism here by switching bandwidth manually */
		dprintk(1, "%s: non-supported bandwidth passed %d\n", __func__, p->u.ofdm.bandwidth);
		res = -EINVAL;
	}
	state->bw = p->u.ofdm.bandwidth;

	if (res != 0)
	{
		dprintk(1, "%s: 4. Demod set failed (%d)\n", __func__, res);
		return res;
	}
//	cnt = 7;
	cnt = 60;  // request by mrspeccy
	do
	{
		fe_status_t status = 0;
		res |= cxd2820_read_status_dvbt2(fe, &status);

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
		msleep(30); /* fixme: think on this */

		dprintk(10, "%s: waiting for lock %d ...\n", __func__, cnt);
	} while (--cnt);

#if DVB_API_VERSION < 5
	if ((cnt == 0) && (op->constellation != MOD_QAM256))
#else
	if ((cnt == 0) && (c->modulation != QAM_256))
#endif
	{
		dprintk(1, "%s(%d): Timeout tuning DVB-T2 now trying DVB-T...\n", __func__, res);

		tda18272_setup_dvbt(fe);

		/* set-up ops to dvb-t */
		fe->ops.read_status          = cxd2820_read_status_dvbt,
		fe->ops.read_ber             = cxd2820_read_ber_dvbt,
		fe->ops.read_signal_strength = cxd2820_read_signal_strength_dvbt,
		fe->ops.read_snr             = cxd2820_read_snr_dvbt,
		fe->ops.read_ucblocks        = cxd2820_read_ucblocks_dvbt,

		/* init dvb-t */
		res |= cxd2820_i2c_write_bulk(state, cxd2820_power_d3);
		res |= cxd2820_i2c_write_bulk(state, cxd2820_init_val_dvbt);
		res |= cxd2820_i2c_write_bulk(state, cxd2820_reset_dvbt);
		res |= cxd2820_set_ts_out(fe);
		msleep(250);

		if (res != 0)
		{
			dprintk(1, "%s: error init DVB-T (%d)\n", __func__, res);
			return res;
		}
		res = fe->ops.tuner_ops.set_params(fe, p);

		if (res != 0)
		{
			dprintk(1, "%s: Tuner set failed (%d)\n", __func__, res);
			{
				return res;
			}
		}
		state->ber = 0;
		state->si  = INVERSION_AUTO;

		res |= cxd2820_i2c_write_bulk(state, cxd2820_reset_dvbt);

		if (res != 0)
		{
			dprintk(1, "%s: 4. Demod set failed (%d)\n", __func__, res);
			return res;
		}

		/* fixme: dvbapi5 ??? */
		if (p->u.ofdm.bandwidth == BANDWIDTH_8_MHZ)
		{
			res |= cxd2820_i2c_write_bulk(state, cxd2820_bw8_dvbt);
		}
		else if (p->u.ofdm.bandwidth == BANDWIDTH_7_MHZ)
		{
			res |= cxd2820_i2c_write_bulk(state, cxd2820_bw7_dvbt);
		}
		else if (p->u.ofdm.bandwidth == BANDWIDTH_6_MHZ)
		{
			res |= cxd2820_i2c_write_bulk(state, cxd2820_bw6_dvbt);
		}
		else
		{
/* FIXME we could make a software automatism here by switching bandwidth manually */
			dprintk(1, "%s: non-supported bandwidth passed %d\n", __func__, p->u.ofdm.bandwidth);
			res = -EINVAL;
		}
		state->bw = p->u.ofdm.bandwidth;
		/* fixme: the orig driver checks stream_selection here:
		 * auto and HI Priority uses hp other lp
		 * We have not got such value to differ here currently,
		 * let us keep this in mind for later error searching!!!
		 */
		res |= cxd2820_i2c_write_bulk(state, cxd2820_hp_dvbt);

		if (res != 0)
		{
			dprintk(1, "%s: 6. Demod set failed (%d)\n", __func__, res);
			return res;
		}
//		cnt = 7;
		cnt = 60;  // request by mrspeccy

		do
		{
			fe_status_t status = 0;
			res |= cxd2820_read_status_dvbt(fe, &status);

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
			msleep(30);

			dprintk(10, "%s: waiting for lock %d ...\n", __func__, cnt);
		} while (--cnt);

		if (cnt == 0)
		{
			dprintk(1, "%s(%d): timeout tuning DVB-T :(\n", __func__, res);
			return res;
		}
		dprintk(10, "%s(%d,%d): Tuner successfully set (DVB-T)!\n", __func__, res, cnt);
		return res;
	}
	else if (cnt == 0)
	{
		dprintk(1, "%s(%d): timeout tuning DVB-T2 (QAM256) :(\n", __func__, res);
	}
	else
	{
		dprintk(10, "%s(%d,%d): Tuner successfully set (DVB-T2)!\n", __func__, res, cnt);
	}
	return res;
}

static int cxd2820_get_frontend_dvbt2(struct dvb_frontend *fe, struct dvb_frontend_parameters *p)
{
	struct cxd2820_state *state = fe->demodulator_priv;
	int                  res = 0;
	s32                  val32;
#ifdef have_sampling_offset
	u32                  vuval32;
#endif
	u8                   bytes[32];

	dprintk(10, "%s >\n", __func__);
	if (fe->ops.tuner_ops.get_frequency)
	{
		res |= fe->ops.tuner_ops.get_frequency(fe, &p->frequency);

		if (res != 0)
		{
			dprintk(1, "%s: error reading frequency %d\n", __func__, res);
			return res;
		}
		if (p->frequency > 1000)
		{
			res |= cxd2820_i2c_read(state, 0x204C, 4, bytes);
			res |= cxd2820_i2c_read(state, 0x204C, 4, bytes + 4);
			if (res != 0)
			{
				dprintk(1, "%s: error reading data %d\n", __func__, res);
				return res;
			}
			val32  = ((bytes[0] & 0x0F) << 24)
			       |  (bytes[1] << 16)
			       |  (bytes[2] << 8)
			       |  (bytes[3]);
			val32 += ((bytes[4] & 0x0F) << 24)
			       |   (bytes[5] << 16)
			       |   (bytes[6] << 8)
			       |   (bytes[7]);
			val32 >>= 1;

			if (val32 & 0x8000000)
			{
				val32 |= 0xF0000000;
			}
			if (state->si == INVERSION_ON)
			{
				p->frequency += ((val32 >> 18) * ((s32)(state->bw >> 12) * 313)) >> 8;
			}
			else
			{
				p->frequency -= ((val32 >> 18) * ((s32)(state->bw >> 12) * 313)) >> 8;
			}
		}
	}
	res |= cxd2820_i2c_read(state, 0x221B, 32, bytes);
	p->u.ofdm.constellation      = cxd2820_mod[(bytes[12]&0x0F)];
	p->u.ofdm.guard_interval     = cxd2820_gi[(bytes[10]&0x07)];
	p->u.ofdm.transmission_mode  = cxd2820_fft[(bytes[8]&0x0F)>>1];
//	p->u.ofdm.bandwidth_extend   = bytes[6]&0x01;
//	p->u.ofdm.pilot_pattern      = bytes[21]&0x0F;

	res |= cxd2820_i2c_read(state, 0x2254,16, bytes);

//	p->u.ofdm.constellation_rotation = bytes[9]&0x01;
	p->u.ofdm.constellation          = cxd2820_mod[(bytes[8]&0x03)];
	p->u.ofdm.code_rate_HP           = cxd2820_cr_plp[(bytes[7]&0x07)];
	p->u.ofdm.code_rate_LP           = cxd2820_cr_plp[(bytes[7]&0x07)];
//	p->u.ofdm.id_cell                = (bytes[23]<<8)|(bytes[24]);
//	p->u.ofdm.id_system_network      = (bytes[27]<<24)|(bytes[28]<<16)|(bytes[25]<<8)|(bytes[26]);
//	p->u.ofdm.stream_selection       = (u32)((bytes[0]<<16) | state->stream_selection);
	p->inversion                     = state->si;
	res |= cxd2820_i2c_read(state, 0x2052, 5, bytes);
	if (res != 0)
	{
		dprintk(1,"%s: error reading data %d\n", __func__, res);
		return res;
	}
#ifdef have_sampling_offset
	vuval32 =  bytes[1] <<24;
	vuval32 |= bytes[2] << 16;
	vuval32 |= bytes[3] << 8;
	vuval32 |= bytes[4];

	if (state->bw == BANDWIDTH_8_MHZ)
	{
		if ((bytes[0] & 0x7F) != 0x11)
		{
			res |= -1;
		}
		p->u.ofdm.sampling_offset = (s32)(vuval32- 0xF0000000);
		p->u.ofdm.sampling_offset = (p->u.ofdm.sampling_offset + 4) / 8;
		p->u.ofdm.sampling_offset *= 831;
	}
	else if (state->bw == BANDWIDTH_6_MHZ)
	{
		if ((bytes[0] & 0x7F) != 0x17)
		{
			res |= -1;
		}
		p->u.ofdm.sampling_offset = (s32)(vuval32- 0xEAAAAAAA);
		p->u.ofdm.sampling_offset = (p->u.ofdm.sampling_offset + 4) / 8;
		p->u.ofdm.sampling_offset *= 623;
	}
	else if (state->bw == BANDWIDTH_7_MHZ)
	{
		if ((bytes[0] & 0x7F) != 0x14)
		{
			res |= -1;
		}
		p->u.ofdm.sampling_offset = (s32)(vuval32- 0x80000000);
		p->u.ofdm.sampling_offset =(p->u.ofdm.sampling_offset + 4) / 8;
		p->u.ofdm.sampling_offset *= 727;
	}
	else
	{
		res = -EINVAL;
	}
	p->u.ofdm.sampling_offset = (p->u.ofdm.sampling_offset + (5000*8))/(10000*8);
#endif
	res |= cxd2820_i2c_read(state, 0x227F, 1, bytes);
	if (res != 0)
	{
		dprintk(1, "%s: error reading data %d\n", __func__, res);
		return res;
	}
	if (bytes[0])
	{
		--bytes[0];
	}
	p->u.ofdm.hierarchy_information = bytes[0];
//	p->u.ofdm.stream_selection      |= (u32)(p->u.ofdm.hierarchy_information << 8);
	dprintk(10, "%s < res %d\n", __func__, res);
	return res;
}

static struct dvb_frontend_ops cxd2820_ops_dvbt2 =
{
	.info =
	{
		.name                = "Sony CXD2820 DVB-T/T2",
		.type                = FE_OFDM,
		.frequency_stepsize  = 62500,
		.frequency_min       = 47000000,
		.frequency_max       = 862000000,
		.frequency_tolerance = 0,
		.symbol_rate_min     = 5705357,
		.symbol_rate_max     = 7607143,

		.caps                = FE_CAN_FEC_1_2
   		                     | FE_CAN_FEC_2_3
		                     | FE_CAN_FEC_3_4
		                     | FE_CAN_FEC_5_6
		                     | FE_CAN_FEC_7_8
		                     | FE_CAN_FEC_AUTO
		                     | FE_CAN_QPSK
		                     | FE_CAN_QAM_16
		                     | FE_CAN_QAM_64
		                     | FE_CAN_QAM_256
		                     | FE_CAN_QAM_AUTO
		                     | FE_CAN_HIERARCHY_AUTO
		                     | FE_CAN_GUARD_INTERVAL_AUTO
	},
	.release                 = cxd2820_release,
	.init                    = cxd2820_init_dvbt2,
	.sleep                   = cxd2820_sleep,
	.set_frontend            = cxd2820_set_frontend_dvbt2,
	.get_frontend            = cxd2820_get_frontend_dvbt2,
	.read_status             = cxd2820_read_status_dvbt2,
	.read_ber                = cxd2820_read_ber_dvbt2,
	.read_signal_strength    = cxd2820_read_signal_strength_dvbt2,
	.read_snr                = cxd2820_read_snr_dvbt2,
	.read_ucblocks           = cxd2820_read_ucblocks_dvbt2,
#if DVB_API_VERSION >= 5
	.set_property            = cxd2820_set_property,
	.get_property            = cxd2820_get_property,
#else
	.get_info                = cxd2820_get_info,
#endif
};

static void cxd2820_register_frontend(struct dvb_adapter *dvb_adap, struct socket_s *socket)
{
	struct dvb_frontend   *frontend;
	struct cxd2820_config *cfg;

	dprintk(100, "%s\n", __func__);

	if (numSockets + 1 == cMaxSockets)
	{
		dprintk(1, "Max. number of sockets reached... cannot register\n");
		return;
	}
	socketList[numSockets] = *socket;
	numSockets++;

	cfg = kmalloc(sizeof(struct cxd2820_config), GFP_KERNEL);

	if (cfg == NULL)
	{
		dprintk(1, "cxd2820: error malloc\n");
		return;
	}
	cfg->tuner_no = numSockets + 1;
	cfg->tuner_enable_pin = stpio_request_pin (socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);

	dprintk(10, "tuner_enable_pin %p\n", cfg->tuner_enable_pin);
	stpio_set_pin(cfg->tuner_enable_pin, !socket->tuner_enable[2]);
	stpio_set_pin(cfg->tuner_enable_pin, socket->tuner_enable[2]);

	msleep(250);

	cfg->tuner_active_lh = socket->tuner_enable[2];

	cfg->demod_address   = frontend_cfg->demod_i2c;
	cfg->tuner_address   = frontend_cfg->tuner_i2c;

	cfg->ts_out          = cxd2820.ts_out;
	cfg->si              = cxd2820.si;

	dprintk(10, "%s: ts_out %d %d\n", __func__, cfg->ts_out, cxd2820.ts_out);

	frontend = cxd2820_attach(cfg, &tda18272, i2c_get_adapter(socket->i2c_bus));

	if (frontend == NULL)
	{
		dprintk(1, "%s: CXD2820 attach failed\n", __func__);

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

static int cxd2820_demod_detect(struct socket_s *socket, struct frontend_s *frontend)
{
	struct stpio_pin *pin = stpio_request_pin(socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);

	dprintk(100, "%s > %s: i2c-%d addr 0x%x\n", __func__, socket->name, socket->i2c_bus, frontend_cfg->demod_i2c);

	if (pin != NULL)
	{
		struct cxd2820_state *state = kmalloc(sizeof(struct cxd2820_state), GFP_KERNEL);
		u8                   bytes;
		int                  res = 0;

		stpio_set_pin(pin, !socket->tuner_enable[2]);
		stpio_set_pin(pin, socket->tuner_enable[2]);

		msleep(250);

		memset(state, 0, sizeof(struct cxd2820_state));

		state->i2c         = i2c_get_adapter(socket->i2c_bus);
		state->i2c_address = frontend_cfg->demod_i2c;

		bytes = 0xCC;

		res |= cxd2820_i2c_read(state , 0x20FD, 1, &bytes);

		if (res != 0)
		{
			dprintk(1, "%s: 1. failed to detect demod %d\n", __func__, res);
			stpio_free_pin(pin);
			kfree(state);
			return -1;
		}

		if ((bytes != 0xE0) && (bytes != 0xE1))
		{
			dprintk(1, "%s: 2. failed to detect demod 0x%0x\n", __func__, bytes);
			stpio_free_pin(pin);
			kfree(state);
			return -1;
		}
		kfree(state);
	}
	else
	{
		dprintk(1, "%s: failed to allocate pio pin\n", __func__);
		return -1;
	}
	stpio_free_pin(pin);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int cxd2820_demod_attach(struct dvb_adapter *adapter, struct socket_s *socket, struct frontend_s *frontend)
{
	dprintk(100, "%s >\n", __func__);
	cxd2820_register_frontend(adapter, socket);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ******************************* */
/* platform device functions       */
/* ******************************* */

static int cxd2820_probe(struct platform_device *pdev)
{
	struct platform_frontend_config_s *plat_data = pdev->dev.platform_data;
	struct frontend_s frontend;
	struct cxd2820_s* data;

	dprintk(100, "%s >\n", __func__);

	frontend_cfg = kmalloc(sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontend_cfg, plat_data, sizeof(struct platform_frontend_config_s));

	data = (struct cxd2820_s*)frontend_cfg->private;
	cxd2820  = *data->cxd2820;
	tda18272 = *data->tda18272;

	dprintk(10, "Found frontend \"%s\" in platform config\n", frontend_cfg->name);

	frontend.demod_detect = cxd2820_demod_detect;
	frontend.demod_attach = cxd2820_demod_attach;
	frontend.name         = "cxd2820";

	if (socket_register_frontend(&frontend) < 0)
	{
		dprintk(1, "failed to register frontend\n");
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int cxd2820_remove (struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver cxd2820_driver =
{
	.probe  = cxd2820_probe,
	.remove = cxd2820_remove,
	.driver	=
	{
		.name  = "cxd2820",
		.owner = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init cxd2820_init_module(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);
	ret = platform_driver_register(&cxd2820_driver);
	dprintk(100, "%s < %d\n", __func__, ret);
	return ret;
}

static void cxd2820_cleanup_module(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Activates frontend debugging (default:0)");

module_init(cxd2820_init_module);
module_exit(cxd2820_cleanup_module);

MODULE_DESCRIPTION("CXD2820 DVB-T/T2");
MODULE_AUTHOR("Duckbox");
MODULE_LICENSE("GPL");
// vim:ts=4
