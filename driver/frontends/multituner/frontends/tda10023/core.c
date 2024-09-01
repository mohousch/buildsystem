/*
 * @brief core.c
 *
 * @author konfetti
 *
 * Copyright (C) 2011 duckbox
 *
 * Core part for TDA10023 DVB-C demodulator
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

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>

#include "tda1002x.h"
#include "tda10023_platform.h"
#include "frontend_platform.h"
#include "socket.h"

#include "tuner.h"

short paramDebug;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[core-tda10023] "

/* saved platform config */
static struct platform_frontend_config_s* frontend_cfg = NULL;

#define cMaxSockets 4
static u8 numSockets = 0;
static struct socket_s socketList[cMaxSockets];

static int i2c_writereg(struct i2c_adapter *adapter, u8 addr, u8 reg, u8 data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = addr, .flags = 0, .buf = buf, .len = 2 };
	int ret;

	ret = i2c_transfer(adapter, &msg, 1);
	if (ret != 1)
	{
		dprintk(1, "%s: I/O error (reg == 0x%02x, val == 0x%02x, ret == %i)\n", __func__, reg, data, ret);
	}
	return (ret != 1) ? -EREMOTEIO : 0;
}

static u8 i2c_readreg(struct i2c_adapter *adapter, u8 addr, u8 reg)
{
	u8 b0 [] = { reg };
	u8 b1 [] = { 0 };
	struct i2c_msg msg [] = { { .addr = addr, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = addr, .flags = I2C_M_RD, .buf = b1, .len = 1 } };
	int ret;

	ret = i2c_transfer(adapter, msg, 2);

	if (ret != 2) 
	{
		dprintk(1, "%s: I/O error (reg == 0x%02x, ret == %i)\n", __func__, reg, ret);
	}
	return b1[0];
}

static void tda10023_register_frontend(struct dvb_adapter *dvb_adap, struct socket_s *socket)
{
	struct dvb_frontend* frontend;
	struct tda10023_config* cfg;
	struct tda10023_private_data_s* priv;
	
	dprintk(100, "%s >\n", __func__);

	if (numSockets + 1 == cMaxSockets)
	{
		dprintk(1, "Maximum number of sockets reached; cannot register\n");
		return;
	}
	socketList[numSockets] = *socket;
	numSockets++;

	priv = (struct tda10023_private_data_s*) frontend_cfg->private;

	cfg = kmalloc(sizeof(struct tda10023_config), GFP_KERNEL);

	if (cfg == NULL)
	{
		dprintk(1, "malloc error\n");
		return;
	}

	if (socket->tuner_enable[0] != -1)
	{
		cfg->tuner_enable_pin = stpio_request_pin (socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);

		stpio_set_pin(cfg->tuner_enable_pin, !socket->tuner_enable[2]);
		stpio_set_pin(cfg->tuner_enable_pin, socket->tuner_enable[2]);
		dprintk(20, "tuner_enable_pin PIO %d.%d (%p)\n", socket->tuner_enable[0], socket->tuner_enable[1], cfg->tuner_enable_pin);
 
		msleep(250);
		cfg->tuner_active_lh = socket->tuner_enable[2];
	}
	else
	{
	   cfg->tuner_enable_pin = NULL;
	}

	cfg->demod_address = frontend_cfg->demod_i2c;
	cfg->tuner_address = frontend_cfg->tuner_i2c;

	cfg->xtal          = priv->xtal;
	cfg->pll_m         = priv->pll_m;
	cfg->pll_p         = priv->pll_p;
	cfg->pll_n         = priv->pll_n;
	cfg->output_mode   = priv->output_mode;
	cfg->deltaf        = priv->deltaf;

	frontend =  tda10023_attach(cfg, i2c_get_adapter(socket->i2c_bus), 0);

	if (frontend == NULL)
	{
		dprintk(1, "Attaching TDA10023 failed\n");

		if (cfg->tuner_enable_pin)
		{
			stpio_free_pin(cfg->tuner_enable_pin);
		}
		kfree(cfg);
		return;
	}

	if (dvb_register_frontend (dvb_adap, frontend))
	{
		dprintk (1, "%s: Frontend registration failed\n", __func__);
		if (frontend->ops.release)
		{
			frontend->ops.release (frontend);
		}
		return;
	}
	return;
}

static int tda10023_demod_detect(struct socket_s *socket, struct frontend_s *frontend)
{
	int              ret = 0;
	struct stpio_pin *pin = NULL;

	dprintk(100, "%s >\n", __func__);

	if (socket->tuner_enable[0] != -1)
	{
		pin = stpio_request_pin(socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);
	}
	dprintk(20, "%s: bus: i2c-%d, addr: 0x%02x\n", socket->name, socket->i2c_bus, frontend_cfg->demod_i2c);

	if (pin != NULL)
	{
		stpio_set_pin(pin, !socket->tuner_enable[2]);
		stpio_set_pin(pin, socket->tuner_enable[2]);
		msleep(250);
	}
	/* Wakeup if in standby */
	i2c_writereg (i2c_get_adapter(socket->i2c_bus), frontend_cfg->demod_i2c, 0x00, 0x2b);

	/* check if the demod is there */
	if ((i2c_readreg(i2c_get_adapter(socket->i2c_bus), frontend_cfg->demod_i2c, 0x1a) & 0xf0) != 0x70) 
	{
		dprintk (70, "ret = %d\n", ret);
		dprintk (1, "Invalid probe, probably not a TDA10023 device\n");

		if (pin != NULL)
		{
			stpio_free_pin(pin);
		}
		return -EREMOTEIO;
	}

	dprintk(20, "%s: TA10023 detected\n", __func__);

	if (pin != NULL)
	{
		stpio_free_pin(pin);
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int tda10023_demod_attach(struct dvb_adapter* adapter, struct socket_s *socket, struct frontend_s *frontend)
{
	dprintk(100, "%s >\n", __func__);
	tda10023_register_frontend(adapter, socket);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ******************************* */
/* platform device functions       */
/* ******************************* */

static int tda10023_probe(struct platform_device *pdev)
{
	struct platform_frontend_config_s *plat_data = pdev->dev.platform_data;
	struct frontend_s frontend;

	dprintk(100, "%s >\n", __func__);

	frontend_cfg = kmalloc(sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontend_cfg, plat_data, sizeof(struct platform_frontend_config_s));

	dprintk(20, "Found frontend \"%s\" in platform config\n", frontend_cfg->name);

	frontend.demod_detect = tda10023_demod_detect;
	frontend.demod_attach = tda10023_demod_attach;
	frontend.name         = "tda10023";

	if (socket_register_frontend(&frontend) < 0)
	{
		dprintk(1, "%s: Failed to register frontend\n", __func__);
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int tda10023_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver tda10023_driver =
{
	.probe  = tda10023_probe,
	.remove = tda10023_remove,
	.driver	=
	{
		.name	= "tda10023",
		.owner  = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init tda10023_init_c(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);
	ret = platform_driver_register(&tda10023_driver);
	dprintk(100, "%s < %d\n", __func__, ret);
	return ret;
}

static void tda10023_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_init (tda10023_init_c);
module_exit (tda10023_cleanup);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_DESCRIPTION ("Tuner driver");
MODULE_AUTHOR      ("konfetti");
MODULE_LICENSE     ("GPL");
// vim:ts=4
