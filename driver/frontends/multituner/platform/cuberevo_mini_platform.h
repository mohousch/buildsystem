/*
 * @brief cuberevo_mini_platform.c
 *
 * @author konfetti
 *
 * 	Copyright (C) 2011 duckbox
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/version.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>

#include "cx24116_platform.h"
#include "stv090x_platform.h"
#include "tda10023_platform.h"
#include "zl10353_platform.h"
#include "stv0288_platform.h"

#include "socket.h"
#include "tuner.h"
#include "lnb.h"

#ifndef _cuberevo_mini_123
#define _cuberevo_mini_123

struct cx24116_private_data_s cx24116_tuner_priv =
{
	.useUnknown       = 1,
	.usedLNB          = cLNB_PIO,
	.fw_name          = "dvb-fe-cx24116.fw",
	.fastDelay        = 1,
};

struct stv090x_private_data_s stv090x_tuner_priv =
{
	.usedLNB              = cLNB_PIO,
	.usedTuner            = cTUNER_EXT_STV6110X,
	.alternativePath      = cNONE,
	.shouldSleep          = 0,
	.device               = STV0903,
	.demod                = STV090x_DEMODULATOR_0,
	.demod_mode           = STV090x_DUAL,
	.clk_mode             = STV090x_CLK_EXT,
	.xtal                 = 8000000,
	.ref_clk              = 16000000,
	.ts1_mode             = STV090x_TSMODE_DVBCI,
	.ts2_mode             = STV090x_TSMODE_SERIAL_CONTINUOUS,
	.ts1_clk              = 0,
	.ts2_clk              = 0,
	.repeater_level       = STV090x_RPTLEVEL_16,
	.tuner_bbgain         = 3, //fixme move later to separated tuner part
	.adc1_range           = -1,
	.adc2_range           = -1,
	.diseqc_envelope_mode = false,
	.tuner_refclk         = 16000000,
};

struct tda10023_private_data_s tda10023_tuner_priv =
{
	.xtal             = 28920000,
	.pll_m            = 0xc,
	.pll_p            = 2,
	.pll_n            = 3,
	.output_mode      = TDA10023_OUTPUT_MODE_PARALLEL_B,
	.deltaf           = 0xffe3,
};

struct zl10353_private_data_s zl10353_tuner_priv =
{
	.adc_clock             = 450560,
	.if2                   = 361667, //45710,
	.no_tuner              = 0,
	.parallel_ts           = 1,
	.disable_i2c_gate_ctrl = 0,
	.clock_ctl_1           = 0,
	.pll_0                 = 0,
};

struct stv0288_private_data_s stv0288_tuner_priv =
{
	.usedLNB          = cLNB_PIO,
	.inittab          = NULL,
	.min_delay_ms     = 100,
};

struct platform_frontend_config_s cx24116_frontend =
{
	.name               = "cx24116",
	.demod_i2c          = 0x05,
	.tuner_i2c          = 0xff,
	.private            = &cx24116_tuner_priv,
};

struct platform_frontend_config_s stv090x_frontend =
{
	.name               = "stv090x",
	.demod_i2c          = 0x68,
	.tuner_i2c          = 0x60,
	.private            = &stv090x_tuner_priv,
};

struct platform_frontend_config_s tda10023_frontend =
{
	.name               = "tda10023",
	.demod_i2c          = 0x0C,
	.tuner_i2c          = 0x61,
	.private            = &tda10023_tuner_priv,
};

struct platform_frontend_config_s zl10353_frontend =
{
	.name               = "zl10353",
	.demod_i2c          = 0x0e,
	.tuner_i2c          = 0x61,
	.private            = &zl10353_tuner_priv,
};

struct platform_frontend_config_s stv0288_frontend =
{
	.name               = "stv0288",
	.demod_i2c          = 0x68,
	.tuner_i2c          = 0x60,
	.private            = &stv0288_tuner_priv,
};

struct tunersocket_s cuberevo_socket =
{
	.numSockets = 1,
	.socketList = (struct socket_s[])
	{
		[0] =
		{
			.name         = "socket-1",
			.tuner_enable = { -1, -1, -1 },
			.lnb          = { 2, 6, 1, 2, 5, 0 },
			.i2c_bus      = 0,
		},
	},
};

struct platform_device cx24116_frontend_device =
{
	.name         = "cx24116",
	.id           = -1,
	.dev          =
	{
		.platform_data = &cx24116_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device stv090x_frontend_device =
{
	.name          = "stv090x",
	.id            = -1,
	.dev           =
	{
		.platform_data = &stv090x_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device tda10023_frontend_device =
{
	.name          = "tda10023",
	.id            = -1,
	.dev           =
	{
		.platform_data = &tda10023_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device zl10353_frontend_device =
{
	.name          = "zl10353",
	.id            = -1,
	.dev           =
	{
		.platform_data = &zl10353_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device stv0288_frontend_device =
{
	.name          = "stv0288",
	.id            = -1,
	.dev           =
	{
	        .platform_data = &stv0288_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device cuberevo_socket_device =
{
	.name          = "socket",
	.id            = -1,
	.dev           =
	{
		.platform_data = &cuberevo_socket,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device *platform[] __initdata =
{
	&cx24116_frontend_device,
	&stv090x_frontend_device,
	&tda10023_frontend_device,
	&zl10353_frontend_device,
	&stv0288_frontend_device,
	&cuberevo_socket_device,
};
#endif
// vim:ts=4
