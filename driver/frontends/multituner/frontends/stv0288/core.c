/*
 * @brief core.c
 *
 * @author konfetti
 *
 * Copyright (C) 2011 duckbox
 *
 * core part for STM STV0288 DVB-S demodulator
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

#include "stv0288.h"
#include "stv0288_platform.h"
#include "frontend_platform.h"
#include "socket.h"

#include "tuner.h"
#include "lnb.h"

short paramDebug = 0;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[core_stv0288] "

/* saved platform config */
static struct platform_frontend_config_s* frontend_cfg = NULL;

#define cMaxSockets 4
static u8 numSockets = 0;
static struct socket_s socketList[cMaxSockets];

static int stv0288_writeregI(struct i2c_adapter *i2c, u32 demod_address, u8 reg, u8 data)
{
	int ret;
	u8 buf[] = { reg, data };
	struct i2c_msg msg =
	{
		.addr = demod_address,
		.flags = 0,
		.buf = buf,
		.len = 2
	};
	ret = i2c_transfer(i2c, &msg, 1);
	return (ret != 1) ? -EREMOTEIO : 0;
}

static u8 stv0288_readreg(struct i2c_adapter *i2c, u32 demod_address, u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };
	struct i2c_msg msg[] =
	{
		{
			.addr  = demod_address,
			.flags = 0,
			.buf   = b0,
			.len   = 1
		},
		{
			.addr  = demod_address,
			.flags = I2C_M_RD,
			.buf   = b1,
			.len   = 1
		}
	};
	ret = i2c_transfer(i2c, msg, 2);
	return b1[0];
}

static void stv0288_register_frontend(struct dvb_adapter *dvb_adap, struct socket_s *socket)
{
	struct dvb_frontend           *frontend;
	struct stv0288_config         *cfg;
	struct stv0288_private_data_s *priv;

	dprintk(100, "%s >\n", __func__);

	if (numSockets + 1 == cMaxSockets)
	{
		dprintk(1, "Max. number of sockets reached... cannot register\n");
		return;
	}
	socketList[numSockets] = *socket;
	numSockets++;

	priv = (struct stv0288_private_data_s*) frontend_cfg->private;

	cfg = kmalloc(sizeof(struct stv0288_config), GFP_KERNEL);

	if (cfg == NULL)
	{
		dprintk(1, "stv0288: error malloc\n");
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
	cfg->demod_address = frontend_cfg->demod_i2c;
	cfg->tuner_address = frontend_cfg->tuner_i2c;
	cfg->usedLNB       = priv->usedLNB;
	memcpy(cfg->lnb, socket->lnb, sizeof(cfg->lnb));

	cfg->inittab       = priv->inittab;
	cfg->min_delay_ms  = priv->min_delay_ms;

	frontend = stv0288_attach(cfg, i2c_get_adapter(socket->i2c_bus));

	if (frontend == NULL)
	{
		dprintk(1, "stv0288: stv0288_attach failed\n");
		if (cfg->tuner_enable_pin)
		{
			stpio_free_pin(cfg->tuner_enable_pin);
		}
		kfree(cfg);
	 	return;
	}

#if 0
	if (ix2476_attach(frontend, cfg->tuner_address, i2c_get_adapter(socket->i2c_bus), DVB_PLL_TUA6034) == NULL)
	{
		printk("ix2476: ix2476_attach failed at i2c-%d addr 0x%02x\n", socket->i2c_bus, cfg->tuner_address);

	        if (cfg->tuner_enable_pin)
		{
			stpio_free_pin(cfg->tuner_enable_pin);
		}
	    	kfree(cfg);
	        return;
	}
#endif
	if (dvb_register_frontend (dvb_adap, frontend))
	{
		dprintk(1, "%s: Frontend registration failed !\n", __FUNCTION__);
		if (frontend->ops.release)
		{
			frontend->ops.release (frontend);
		}
		return;
	}
	return;
}

static int stv0288_demod_detect(struct socket_s *socket, struct frontend_s *frontend)
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

	stv0288_writeregI(i2c_get_adapter(socket->i2c_bus), frontend_cfg->demod_i2c, 0x41, 0x04);
	msleep(200);
	id = stv0288_readreg(i2c_get_adapter(socket->i2c_bus), frontend_cfg->demod_i2c, 0x00);

	/* register 0x00 contains 0x11 for STV0288  */
	if (id != 0x11)
	{
		dprintk(50, "id = %02x\n", id);
		dprintk(1, "Invalid probe, probably not a STV0288 device\n");

		if (pin != NULL)
		{
			stpio_free_pin(pin);
		}
		return -EREMOTEIO;
	}
	dprintk(20, "Detected STV0288\n");
   
	if (pin != NULL)
	{
		stpio_free_pin(pin);
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int stv0288_demod_attach(struct dvb_adapter* adapter, struct socket_s *socket, struct frontend_s *frontend)
{
	dprintk(100, "%s >\n", __func__);

	stv0288_register_frontend(adapter, socket);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ******************************* */
/* platform device functions       */
/* ******************************* */

static int stv0288_probe (struct platform_device *pdev)
{
	struct platform_frontend_config_s *plat_data = pdev->dev.platform_data;
	struct frontend_s frontend;

	dprintk(100, "%s >\n", __func__);

	frontend_cfg = kmalloc(sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontend_cfg, plat_data, sizeof(struct platform_frontend_config_s));

	dprintk(20, "Found frontend \"%s\" in platform config\n", frontend_cfg->name);

	frontend.demod_detect = stv0288_demod_detect;
	frontend.demod_attach = stv0288_demod_attach;
	frontend.name         = "stv0288";
	
	if (socket_register_frontend(&frontend) < 0)
	{
		dprintk(1, "Failed to register frontend\n");
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int stv0288_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver stv0288_driver =
{
	.probe  = stv0288_probe,
	.remove = stv0288_remove,
	.driver	=
	{
	    .name   = "stv0288",
	    .owner  = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions                */
/* ******************************* */
int __init stv0288_init(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);
	ret = platform_driver_register (&stv0288_driver);
	dprintk(100, "%s < %d\n", __func__, ret);
	return ret;
}

static void stv0288_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_init (stv0288_init);
module_exit (stv0288_cleanup);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

MODULE_DESCRIPTION ("STV0288 demodulator driver");
MODULE_AUTHOR      ("konfetti");
MODULE_LICENSE     ("GPL");
// vim:ts=4
