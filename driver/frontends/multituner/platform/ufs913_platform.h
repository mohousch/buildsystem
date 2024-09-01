/*
 * @brief ufs913_platform.h
 *
 * @author konfetti
 *
 * Copyright (C) 2012 duckbox
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

#ifndef _ufs913_123
#define _ufs913_123

#include <linux/version.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>

#include "avl6222_platform.h"
#include "avl6222_reg.h"
#include "avl6222.h"

#include "socket.h"
#include "tuner.h"
#include "lnb.h"

struct avl_private_data_s avl_tuner_priv =
{
	.demod_freq       = 11200,  /* fixme: the next three could be determined by the pll config!!! */
	.fec_freq         = 16800,
	.mpeg_freq        = 25200,

	.i2c_speed_khz    = TUNER_I2C_CLK,

	.agc_polarization = AGC_POL_NORMAL,
	.agc_tri          = 0x01,
	.mpeg_mode        = MPEG_FORMAT_TS_PAR,
	.mpeg_serial      = MPEG_MODE_PARALLEL,
	.mpeg_clk_mode    = MPEG_CLK_MODE_RISING,
	.mpeg_tri         = 1,
	.max_lpf          = 360,
	.pll_config       = 2,
	.usedTuner        = cTUNER_EXT_STV6110A,
	.iq_swap          = CI_FLAG_IQ_NO_SWAPPED,
	.auto_iq_swap     = CI_FLAG_IQ_AUTO_BIT_AUTO,
	.lock_mode        = LOCK_MODE_FIXED,
	.usedLNB          = cLNB_A8293,

//Todo
	.lpf              = 193,
};

struct platform_frontend_s avl6222_frontend =
{
	.numConfigs = 2,
	.config = (struct platform_frontend_config_s[])
	{
		[0] =
		{
			.name      = "avl6222-1",
			.demod_i2c = 0x0D,
			.tuner_i2c = 0xC0,
			.private   = &avl_tuner_priv,
		},
		[1] =
		{
			.name      = "avl6222-2",
			.demod_i2c = 0x0C,
			.tuner_i2c = 0xC0,
			.private   = &avl_tuner_priv,
		},
	},
};

struct tunersocket_s ufs913_socket =
{
	.numSockets = 2,
	.socketList = (struct socket_s[])
	{
		[0] =
		{
			.name         = "socket-1",

			.tuner_enable = {3, 3, 1},
			.lnb          = {2, 0x08, 0x00, 0x04, 0x0B, 0x10},
			.i2c_bus      = 2,
		},
		[1] =
		{
			.name         = "socket-2",

			.tuner_enable = {3, 2, 1},
			.lnb          = {3, 0x08, 0x00, 0x04, 0x0B, 0x10},
			.i2c_bus      = 2,
		},
	},
};

struct platform_device avl6222_frontend_device =
{
	.name          = "avl6222",
	.id            = -1,
	.dev           =
	{
		.platform_data = &avl6222_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device ufs913_socket_device =
{
	.name          = "socket",
	.id            = -1,
	.dev           =
	{
		.platform_data = &ufs913_socket,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device *platform[] __initdata =
{
	&avl6222_frontend_device,
	&ufs913_socket_device,
};
#endif
// vim:ts=4