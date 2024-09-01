/*
 * @brief avl2108_platform.c
 *
 * @author konfetti
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

#include <linux/platform_device.h>

#include "avl2108_platform.h"
#include "avl2108_reg.h"
#include "avl2108.h"

#if defined(ATEVIO7500)
static struct avl_private_data_s avl_tuner_priv =
{
	.ref_freq = 1,
	.demod_freq = 11200, /* fixme: the next three could be determined by the pll config!!! */
	.fec_freq = 16800,
	.mpeg_freq = 22400,
	.i2c_speed_khz = TUNER_I2C_CLK,
	.agc_polarization = AGC_POL_INVERT,
	.mpeg_mode = MPEG_FORMAT_TS_PAR,
	.mpeg_serial = MPEG_MODE_PARALLEL,
	.mpeg_clk_mode = MPEG_CLK_MODE_RISING,
	.max_lpf = 0,
	.pll_config = 5,
	.usedTuner = cTUNER_INT_STV6306,
	.usedLNB = cLNB_PIO,
	.lpf = 193,
	.lock_mode = LOCK_MODE_ADAPTIVE,
	.iq_swap = CI_FLAG_IQ_NO_SWAPPED,
	.auto_iq_swap = CI_FLAG_IQ_AUTO_BIT_AUTO,
	.agc_ref = 0x30,
};

static struct platform_frontend_s avl2108_config =
{
	.numFrontends = 2,
	.frontendList = (struct platform_frontend_config_s[])
	{
		[0] =
		{
			.name = "avl2108-1",

			.tuner_enable = {3, 3, 1},
			.lnb = {2, 6, 0, 2, 5, 1},
			.i2c_bus = 0,

			.demod_i2c = 0x0C,
			.tuner_i2c = 0xC0,
			.private = &avl_tuner_priv,
		},
		[1] =
		{
			.name = "avl2108-2",

			.tuner_enable = {3, 2, 1},
			.lnb = {4, 4, 0, 4, 3, 1},
			.i2c_bus = 1,

			.demod_i2c = 0x0C,
			.tuner_i2c = 0xC0,
			.private = &avl_tuner_priv,
		},
	},
};
#elif defined(OCTAGON1008)
static struct avl_private_data_s avl_tuner_priv =
{
	.ref_freq = 1,
	.demod_freq = 11200, /* fixme: the next three could be determined by the pll config!!! */
	.fec_freq = 16800,
	.mpeg_freq = 22400,
	.i2c_speed_khz = TUNER_I2C_CLK,
	.agc_polarization = AGC_POL_INVERT,
	.mpeg_mode = MPEG_FORMAT_TS_PAR,
	.mpeg_serial = MPEG_MODE_PARALLEL,
	.mpeg_clk_mode = MPEG_CLK_MODE_RISING,
	.max_lpf = 320,
	.pll_config = 5,
	.usedTuner = cTUNER_INT_STV6306,
	.usedLNB = cLNB_PIO,
	.lpf = 340,
	.lock_mode = LOCK_MODE_ADAPTIVE,
	.iq_swap = CI_FLAG_IQ_NO_SWAPPED,
	.auto_iq_swap = CI_FLAG_IQ_AUTO_BIT_AUTO,
	.agc_ref = 0x0,
};

static struct platform_frontend_s avl2108_config =
{
	.numFrontends = 1,
	.frontendList = (struct platform_frontend_config_s[])
	{
		[0] =
		{
			.name = "avl2108-1",

			.tuner_enable = {2, 2, 1},
			.lnb = {1, 6, 0, 1, 4, 1},
			.i2c_bus = 0,

			.demod_i2c = 0x0C,
			.tuner_i2c = 0xC0,
			.private = &avl_tuner_priv,
		},
	},
};
#elif defined(UFS922)
static struct avl_private_data_s avl_tuner_priv =
{
	.ref_freq = 1,
	.demod_freq = 11200, /* fixme: the next three could be determined by the pll config!!! */
	.fec_freq = 16800,
	.mpeg_freq = 19200,
	.i2c_speed_khz = TUNER_I2C_CLK,
	.agc_polarization = AGC_POL_NORMAL,
	.mpeg_mode = MPEG_FORMAT_TS_PAR,
	.mpeg_serial = MPEG_MODE_PARALLEL,
	.mpeg_clk_mode = MPEG_CLK_MODE_RISING,
	.max_lpf = 340,
	.pll_config = 3,
	.usedTuner = cTUNER_EXT_STV6110A,
	.usedLNB = cLNB_LNBH221,
	.lpf = 340,
	.lock_mode = LOCK_MODE_FIXED,
	.iq_swap = CI_FLAG_IQ_NO_SWAPPED,
	.auto_iq_swap = CI_FLAG_IQ_AUTO_BIT_AUTO,
	.agc_ref = 0x0,
};

static struct platform_frontend_s avl2108_config =
{
	.numFrontends = 2,
	.frontendList = (struct platform_frontend_config_s[])
	{
		[0] =
		{
			.name = "avl2108-1",

			.tuner_enable = {2, 4, 1},
			.lnb = {0, 0x0a, 0x08, 0xd0, 0xd4, 0xdc},
			.i2c_bus = 0,

			.demod_i2c = 0x0C,
			.tuner_i2c = 0xC0,
			.private = &avl_tuner_priv,
		},
		[1] =
		{
			.name = "avl2108-2",

			.tuner_enable = {2, 5, 1},
			.lnb = {1, 0x0a, 0x08, 0xd0, 0xd4, 0xdc},
			.i2c_bus = 1,

			.demod_i2c = 0x0C,
			.tuner_i2c = 0xC0,
			.private = &avl_tuner_priv,
		},
	},
};
#else
#warning unsupported arch
#endif

static struct platform_device avl2108_device =
{
	.name = "avl2108",
	.id = -1,
	.dev = {
		.platform_data = &avl2108_config,
	},
	.num_resources = 0,
	.resource = NULL,
};

static struct platform_device *platform[] __initdata =
{
	&avl2108_device,
};

int __init avl2108_platform_init(void)
{
	int ret;
	ret = platform_add_devices(platform, sizeof(platform)
							   / sizeof(struct platform_device *));
	if (ret != 0)
	{
		printk("failed to register avl2108 platform device\n");
	}
	return ret;
}

MODULE_DESCRIPTION("Unified avl2108 driver using platform device model");

MODULE_AUTHOR("konfetti");
MODULE_LICENSE("GPL");

module_init(avl2108_platform_init);
