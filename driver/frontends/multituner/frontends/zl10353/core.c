/*
 * @brief core.c
 *
 * @author konfetti
 *
 * Copyright (C) 2011 duckbox
 *
 * Core part for Zarlink ZL10353 demodulator
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

#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/io.h>

#include "zl10353.h"
#include "zl10353_priv.h"
#include "zl10353_platform.h"
#include "frontend_platform.h"
#include "socket.h"

#include "tuner.h"

short paramDebug = 0;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[core_zl10353] "

/* saved platform config */
static struct platform_frontend_config_s* frontend_cfg = NULL;

#define cMaxSockets 4
static u8 numSockets = 0;
static struct socket_s socketList[cMaxSockets];

/* fixme: cannot include dvb-pll.h */
#define DVB_PLL_TUA6034 7
extern struct dvb_frontend *dvb_pll_attach(struct dvb_frontend *fe, int pll_addr, struct i2c_adapter *i2c, unsigned int pll_desc_id);

static int zl10353_write_zero(struct i2c_adapter *adapter, u8 addr, u8 reg)
{
	int ret;
	u8 data = 0x00;
	u8 buf [] = { reg, data };
	struct i2c_msg msg = { .addr = addr, .flags = 0, .buf = buf, .len = 1 };

	ret = i2c_transfer (adapter, &msg, 1);

	if (ret != 1)
	{
		dprintk(1, "%s: writereg error (reg == 0x%02x ret == %i)\n", __func__, reg, ret);
	}
	return (ret != 1) ? -EREMOTEIO : 0;
}

static int zl10353_read_register(struct i2c_adapter *adapter, u8 addr, u8 reg)
{
	int ret;
	u8 b0[1] = { reg };
	u8 b1[1] = { 0 };
	struct i2c_msg msg[2] =
	{
		{ .addr = addr,
		  .flags = 0,
		  .buf = b0,
		  .len = 1
		},
		{ .addr = addr,
		  .flags = I2C_M_RD,
		  .buf = b1,
		  .len = 1
		}
	};
	ret = i2c_transfer(adapter, msg, 2);

	if (ret != 2)
	{
		dprintk(1, "%s: readreg error (reg=%d, ret==%i)\n", __func__, reg, ret);
		return ret;
	}
	return b1[0];
}

static void zl10353_register_frontend(struct dvb_adapter *dvb_adap, struct socket_s *socket)
{
	struct dvb_frontend           *frontend;
	struct zl10353_config         *cfg;
	struct zl10353_private_data_s *priv;

	dprintk(100, "%s >\n", __func__);

	if (numSockets + 1 == cMaxSockets)
	{
		dprintk(1, "Max. number of sockets reached... cannot register\n");
		return;
	}
	socketList[numSockets] = *socket;
	numSockets++;

	priv = (struct zl10353_private_data_s*) frontend_cfg->private;

	cfg = kmalloc(sizeof(struct zl10353_config), GFP_KERNEL);

	if (cfg == NULL)
	{
		dprintk(1, "malloc error\n");
		return;
	}
	if (socket->tuner_enable[0] != -1)
	{
		cfg->tuner_enable_pin = stpio_request_pin (socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);

		dprintk(20, "tuner_enable_pin %p\n", cfg->tuner_enable_pin);
		stpio_set_pin(cfg->tuner_enable_pin, !socket->tuner_enable[2]);
		stpio_set_pin(cfg->tuner_enable_pin, socket->tuner_enable[2]);
		msleep(250);
		cfg->tuner_active_lh   = socket->tuner_enable[2];
	}
	else
	{
		cfg->tuner_enable_pin = NULL;
	}
	cfg->demod_address         = frontend_cfg->demod_i2c;
	cfg->tuner_address         = frontend_cfg->tuner_i2c;

	cfg->adc_clock             = priv->adc_clock;
	cfg->if2                   = priv->if2;
	cfg->no_tuner              = priv->no_tuner;
	cfg->parallel_ts           = priv->parallel_ts;
	cfg->disable_i2c_gate_ctrl = priv->disable_i2c_gate_ctrl;
	cfg->clock_ctl_1           = priv->clock_ctl_1;
	cfg->pll_0                 = priv->pll_0;

	frontend = zl10353_attach(cfg, i2c_get_adapter(socket->i2c_bus));

	if (frontend == NULL)
	{
		dprintk(1, "Attaching ZL10353 failed\n");

		if (cfg->tuner_enable_pin)
		{
			stpio_free_pin(cfg->tuner_enable_pin);
		}
		kfree(cfg);
		return;
	}
	if (dvb_pll_attach(frontend, cfg->tuner_address, i2c_get_adapter(socket->i2c_bus), DVB_PLL_TUA6034) == NULL)
	{
		dprintk(1, "Attaching TUA6034 PLL failed at i2c-%d addr 0x%02x\n", socket->i2c_bus, cfg->tuner_address);

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

static int zl10353_demod_detect(struct socket_s *socket, struct frontend_s *frontend)
{
	int              id;
	struct stpio_pin *pin = NULL;
	
	dprintk(100, "%s >\n", __func__);

	if (socket->tuner_enable[0] != -1)
	{
		pin = stpio_request_pin(socket->tuner_enable[0], socket->tuner_enable[1], "tun_enab", STPIO_OUT);
	}
	dprintk(20, "%s: i2c-%d addr 0x%x\n", socket->name, socket->i2c_bus, frontend_cfg->demod_i2c);

	if (pin != NULL)
	{
		stpio_set_pin(pin, !socket->tuner_enable[2]);
		stpio_set_pin(pin, socket->tuner_enable[2]);
		msleep(250);
	}
	zl10353_write_zero(i2c_get_adapter(socket->i2c_bus), frontend_cfg->demod_i2c, 0x00);

	/* check if the demod is there */
	id = zl10353_read_register(i2c_get_adapter(socket->i2c_bus), frontend_cfg->demod_i2c, CHIP_ID);
	if ((id != ID_ZL10353) && (id != ID_CE6230) && (id != ID_CE6231))
	{
		dprintk(50, "id = %02x\n", id);
		dprintk(1, "Invalid probe, probably not a ZL10353 device\n");
		if (pin != NULL)
		{
			stpio_free_pin(pin);
		}
		return -EREMOTEIO;
	}
	dprintk(20, "%s: ZL10353 detected\n", __func__);
	
	if (pin != NULL)
	{
		stpio_free_pin(pin);
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int zl10353_demod_attach(struct dvb_adapter *adapter, struct socket_s *socket, struct frontend_s *frontend)
{
	dprintk(100, "%s >\n", __func__);
	zl10353_register_frontend(adapter, socket);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ******************************* */
/* platform device functions       */
/* ******************************* */

static int zl10353_probe(struct platform_device *pdev)
{
	struct platform_frontend_config_s *plat_data = pdev->dev.platform_data;
	struct frontend_s frontend;

	dprintk(100, "%s >\n", __func__);

	frontend_cfg = kmalloc(sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontend_cfg, plat_data, sizeof(struct platform_frontend_config_s));

	dprintk(20, "Found frontend \"%s\" in platform config\n", frontend_cfg->name);

	frontend.demod_detect = zl10353_demod_detect;
	frontend.demod_attach = zl10353_demod_attach;
	frontend.name         = "zl10353";
	
	if (socket_register_frontend(&frontend) < 0)
	{
		dprintk(1, "Failed to register frontend\n");
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int zl10353_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver zl10353_driver =
{
	.probe  = zl10353_probe,
	.remove = zl10353_remove,
	.driver	=
	{
		.name  = "zl10353",
		.owner = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init zl10353_init(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);
	ret = platform_driver_register(&zl10353_driver);
	dprintk(100, "%s < %d\n", __func__, ret);
	return ret;
}

static void zl10353_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_init (zl10353_init);
module_exit (zl10353_cleanup);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_DESCRIPTION ("ZL10353 Demodulator driver");
MODULE_AUTHOR      ("konfetti");
MODULE_LICENSE     ("GPL");
// vim:ts=4
