/*
 * @brief hs8200_platform.h
 *
 * @author konfetti
 *
 *  Copyright (C) 2011 duckbox
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

#ifndef _hs8200_123
#define _hs8200_123

#include <linux/version.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>

#include "avl2108_platform.h"
#include "avl2108_reg.h"
#include "avl2108.h"

#include "tda10024_platform.h"
#include "cxd2820_platform.h"
//#include "s5h1432_platform.h"
#include "mxl201.h"
#include "tda10024.h"
//#include "mxl5007t.h"

#include "socket.h"
#include "tuner.h"
#include "lnb.h"

struct cxd2820_private_data_s cxd2820_priv =
{
	.ts_out    = 0, // parallel
	.si        = INVERSION_ON,
};

struct tda18272_private_data_s tda18272_priv =
{
	.lt        = 1, // lt
	.stdby     = 3, // d3
	.iic_mode  = 0, // iic_0
	.xtout     = 0, // off
}; 

struct cxd2820_s cxd2820 =
{
	.tda18272  = &tda18272_priv,
	.cxd2820   = &cxd2820_priv,
};

#if 0
struct s5h1432_private_data_s s5h1432_priv =
{
	.ts_out    = TDA10024_TS_PARALLEL,
	.si        = FE_SPECTRUMINVERTED,
//	.power     = TDA10024_POWER_D3,
//	.agc_th    = 0x99,
};

struct mxl5007t_private_data_s mxl5007t_priv =
{
	.if_freq   = 6000000,
	.xtal      = (u32) MxL_XTAL_16_MHZ,
	.xtal_cap  = 22,
	.clk_out   = 12,
	.clk_amp   = 12, 
	.si        = FE_SPECTRUMINVERTED,
	.mode      = 0,
	.power     = MXL5007T_POWER_D3,
	.lt        = MXL5007T_LT_OFF,
};

struct s5h1432_s s5h1432 =
{
	.mxl5007t  = &mxl5007t_priv,
	.s5h1432   = &s5h1432_priv,
};
#endif

struct tda10024_private_data_s tda10024_priv =
{
	.ts_out    = TDA10024_TS_PARALLEL,
	.si        = FE_SPECTRUMINVERTED,
	.power     = TDA10024_POWER_D3,
	.agc_th    = 0x99,
};

struct mxl201_private_data_s mxl201_priv =
{
	.if_freq   = 6000000,
	.xtal      = (u32) MxL_XTAL_16_MHZ,
	.xtal_cap  = 22,
	.clk_out   = 12,
	.clk_amp   = 12, 
	.si        = FE_SPECTRUMINVERTED,
	.mode      = 0,
	.power     = MXL201_POWER_D3,
	.lt        = MXL201_LT_OFF,
};

struct tda10024_s tda10024 =
{
      .mxl201      = &mxl201_priv,
      .tda10024    = &tda10024_priv,
};

struct avl_private_data_s avl_tuner_priv =
{
	.ref_freq         = 1,
	.demod_freq       = 11200, /* fixme: the next three could be determined by the pll config!!! */
	.fec_freq         = 16800,
	.mpeg_freq        = 22400,
	.i2c_speed_khz    = TUNER_I2C_CLK,
	.agc_polarization = AGC_POL_INVERT,
	.mpeg_mode        = MPEG_FORMAT_TS_PAR,
	.mpeg_serial      = MPEG_MODE_PARALLEL,
	.mpeg_clk_mode    = MPEG_CLK_MODE_RISING,
	.max_lpf          = 0,
	.pll_config       = 5,
	.usedTuner        = cTUNER_INT_STV6306,
	.usedLNB          = cLNB_PIO,
	.lpf              = 193,
	.lock_mode        = LOCK_MODE_FIXED,
	.iq_swap          = CI_FLAG_IQ_NO_SWAPPED,
	.auto_iq_swap     = CI_FLAG_IQ_AUTO_BIT_AUTO,
	.agc_ref          = 0x30,
	.mpeg_data_clk    = -1,
};

struct platform_frontend_config_s avl2108_frontend =
{
	.name             = "avl2108",

	.demod_i2c        = 0x0C,
	.tuner_i2c        = 0xC0,
	.private          = &avl_tuner_priv,
};

struct platform_frontend_config_s tda10024_frontend =
{
	.name             = "tda10024",

	.demod_i2c        = 0x0C,
	.tuner_i2c        = 0x60,
	.private          = &tda10024,
};

struct platform_frontend_config_s cxd2820_frontend =
{
	.name             = "cxd2820",

	.demod_i2c        = 0x6C,
	.tuner_i2c        = 0xC0,
	.private          = &cxd2820,
};

#if 0
struct platform_frontend_config_s s5h1432_frontend =
{
	.name               = "s5h1432",

	.demod_i2c          = 0x02,
	.tuner_i2c          = 0xC0,
	.private            = &s5h_tuner_priv,
};
#endif
struct tunersocket_s hs8200_socket =
{
	.numSockets       = 2,
	.socketList       = (struct socket_s[])
	{
		[0] =
		{
			.name               = "socket-1",

			.tuner_enable       = {3, 3, 1},
			.lnb                = {2, 6, 0, 2, 5, 1},
			.i2c_bus            = 0,
		},
		[1] =
		{
			.name               = "socket-2",

			.tuner_enable       = {3, 2, 1},
			.lnb                = {4, 4, 0, 4, 3, 1},
			.i2c_bus            = 1,
		},
	},
};

struct platform_device avl2108_frontend_device =
{
	.name          = "avl2108",
	.id            = -1,
	.dev           =
	{
		.platform_data = &avl2108_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device tda10024_frontend_device =
{
	.name          = "tda10024",
	.id            = -1,
	.dev           = {
		.platform_data = &tda10024_frontend,
	},
    .num_resources        = 0,
    .resource             = NULL,
};

struct platform_device cxd2820_frontend_device = {
	.name          = "cxd2820",
	.id            = -1,
	.dev           =
	{
		.platform_data = &cxd2820_frontend,
	},
	.num_resources = 0,
	.resource      = NULL,
};

#if 0
struct platform_device s5h1432_frontend_device =
{
	.name    = "s5h1432",
	.id      = -1,
	.dev     =
	{
		.platform_data = &s5h1432_frontend,
	},
	.num_resources        = 0,
	.resource             = NULL,
};
#endif

struct platform_device hs8200_socket_device =
{
	.name          = "socket",
	.id            = -1,
	.dev           =
	{
		.platform_data = &hs8200_socket,
	},
	.num_resources = 0,
	.resource      = NULL,
};

struct platform_device *platform[] __initdata =
{
	&avl2108_frontend_device,
	&tda10024_frontend_device,
	&cxd2820_frontend_device,
//	&s5h1432_frontend_device,
	&hs8200_socket_device,
};
#endif
// vim:ts=4
