/*
 * @brief core.c
 *
 * @author konfetti
 *
 *  Copyright (C) 2011 duckbox
 *
 *  Core part for STM STV090x demodulator
 *
 *  Version for Sharp BS2F7VZ0169 frontend (STM STV6110A + STV0903)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#include "stv090x.h"
#include "stv090x_reg.h"
#include "stv090x_priv.h"

#include "stv090x_platform.h"
#include "frontend_platform.h"
#include "socket.h"

#include "tuner.h"
#include "lnb.h"

short paramDebug = 0;
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[core_stv09x] "

int bbgain = -1;

/* saved platform config */
static struct platform_frontend_config_s *frontend_cfg = NULL;

#define cMaxSockets 4
static u8 numSockets = 0;
static struct socket_s socketList[cMaxSockets];

extern int stv090x_read_reg(struct stv090x_state *state, unsigned int reg);

static void stv090x_register_frontend(struct dvb_adapter *dvb_adap, struct socket_s *socket)
{
	struct dvb_frontend *frontend;
	struct stv090x_config *cfg;
	struct stv090x_private_data_s *priv;

	dprintk(100, "%s >\n", __func__);

	if (numSockets + 1 == cMaxSockets)
	{
		dprintk(1, "Max number sockets reached ... cannot register\n");
		return;
	}
	socketList[numSockets] = *socket;
	numSockets++;

	priv = (struct stv090x_private_data_s *)frontend_cfg->private;

	cfg = kmalloc(sizeof(struct stv090x_config), GFP_KERNEL);

	if (cfg == NULL)
	{
		dprintk(1, "error malloc\n");
		return;
	}

	if (socket->tuner_enable[0] != -1)
	{
		cfg->tuner_enable_pin = stpio_request_pin(socket->tuner_enable[0],
							  socket->tuner_enable[1],
							  "tun_enab",
							  STPIO_OUT);

		dprintk(20, "tuner_enable_pin %p\n", cfg->tuner_enable_pin);
		stpio_set_pin(cfg->tuner_enable_pin, !socket->tuner_enable[2]);
		stpio_set_pin(cfg->tuner_enable_pin, socket->tuner_enable[2]);

		msleep(250);
		cfg->tuner_active_lh = socket->tuner_enable[2];
	}
	else
	{
		cfg->tuner_enable_pin = NULL;
	}
	cfg->address              = frontend_cfg->demod_i2c;
	cfg->tuner_address        = frontend_cfg->tuner_i2c;
	cfg->usedTuner            = priv->usedTuner;
	cfg->usedLNB              = priv->usedLNB;
	cfg->alternativePath      = priv->alternativePath;
	cfg->shouldSleep          = priv->shouldSleep;
	cfg->device               = priv->device;
	cfg->demod_mode           = priv->demod_mode;
	cfg->clk_mode             = priv->clk_mode;

	cfg->xtal                 = priv->xtal;
	cfg->ref_clk              = priv->ref_clk;
	cfg->ts1_mode             = priv->ts1_mode;
	cfg->ts2_mode             = priv->ts2_mode;
	cfg->ts1_clk              = priv->ts1_clk;
	cfg->ts2_clk              = priv->ts2_clk;
	cfg->repeater_level       = priv->repeater_level;
	cfg->tuner_bbgain         = priv->tuner_bbgain;
	cfg->adc1_range           = priv->adc1_range;

	cfg->adc2_range           = priv->adc2_range;
	cfg->diseqc_envelope_mode = priv->diseqc_envelope_mode;
	cfg->tuner_refclk         = priv->tuner_refclk;

	memcpy(cfg->lnb, socket->lnb, sizeof(cfg->lnb));

	if (numSockets == 1)
	{
		frontend =  stv090x_attach(cfg, i2c_get_adapter(socket->i2c_bus), priv->demod, STV090x_TUNER1);
	}
	else
	{
		frontend =  stv090x_attach(cfg, i2c_get_adapter(socket->i2c_bus), priv->demod, STV090x_TUNER2);
	}
	if (frontend == NULL)
	{
		dprintk(1, "stv090x_attach failed\n");

		if (cfg->tuner_enable_pin)
		{
			stpio_free_pin(cfg->tuner_enable_pin);
		}
		kfree(cfg);
		return;
	}

	if (dvb_register_frontend(dvb_adap, frontend))
	{
		dprintk(1, "%s: Frontend registration failed\n", __func__);
		if (frontend->ops.release)
		{
			frontend->ops.release(frontend);
		}
		return;
	}
	return;
}

static int stv090x_demod_detect(struct socket_s *socket, struct frontend_s *frontend)
{
	struct stv090x_state *state = NULL;
	int ret = 0;
	struct stpio_pin *pin = NULL;

	dprintk(100, "%s >\n", __func__);

	if (socket->tuner_enable[0] != -1)
	{
		pin = stpio_request_pin(socket->tuner_enable[0],
					socket->tuner_enable[1],
					"tun_enab",
					STPIO_OUT);
	}

	dprintk(20, "%s: i2c-%d addr 0x%x\n", socket->name, socket->i2c_bus, frontend_cfg->demod_i2c);

	if (pin != NULL)
	{
		stpio_set_pin(pin, !socket->tuner_enable[2]);
		stpio_set_pin(pin, socket->tuner_enable[2]);

		msleep(250);
	}

	state = kmalloc(sizeof(struct stv090x_state), GFP_KERNEL);

	state->config = kmalloc(sizeof(struct stv090x_config), GFP_KERNEL);

	state->i2c = i2c_get_adapter(socket->i2c_bus);

	state->config->address = frontend_cfg->demod_i2c;

	if ((ret = stv090x_read_reg(state, STV090x_MID)) < 0)
	{
		dprintk(50, "ret = %d\n", ret);
		dprintk(1, "Invalid probe, probably not a stv090x device\n");

		if (pin != NULL)
		{
			stpio_free_pin(pin);
		}
		kfree(state->config);
		kfree(state);

		return -EREMOTEIO;
	}

	dprintk(20, "Detected stv090x\n");

	if (pin != NULL)
	{
		stpio_free_pin(pin);
	}
	kfree(state->config);
	kfree(state);

	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int stv090x_demod_attach(struct dvb_adapter *adapter, struct socket_s *socket, struct frontend_s *frontend)
{
	dprintk(100, "%s >\n", __func__);

	stv090x_register_frontend(adapter, socket);
	dprintk(100, "%s <\n", __func__);
	return 0;
}

/* ******************************* */
/* platform device functions       */
/* ******************************* */

static int stv090x_probe(struct platform_device *pdev)
{
	struct platform_frontend_config_s *plat_data = pdev->dev.platform_data;
	struct frontend_s frontend;

	dprintk(100, "%s >\n", __func__);

	frontend_cfg = kmalloc(sizeof(struct platform_frontend_config_s), GFP_KERNEL);
	memcpy(frontend_cfg, plat_data, sizeof(struct platform_frontend_config_s));

	dprintk(20, "Found frontend \"%s\" in platform config\n", frontend_cfg->name);

	frontend.demod_detect = stv090x_demod_detect;
	frontend.demod_attach = stv090x_demod_attach;
	frontend.name         = "stv090x";

	if (socket_register_frontend(&frontend) < 0)
	{
		dprintk(1, "failed to register frontend\n");
	}

	dprintk(100, "%s <\n", __func__);

	return 0;
}

static int stv090x_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver stv090x_driver =
{
	.probe     = stv090x_probe,
	.remove    = stv090x_remove,
	.driver    =
	{
		.name  = "stv090x",
		.owner = THIS_MODULE,
	},
};


/* ******************************* */
/* module functions                */
/* ******************************* */

int __init stv090x_init(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);

	ret = platform_driver_register(&stv090x_driver);

	dprintk(100, "%s < %d\n", __func__, ret);
	return ret;
}

static void stv090x_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);
}

module_init(stv090x_init);
module_exit(stv090x_cleanup);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(bbgain, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(bbgain, "default=-1 (use default config = 10");

MODULE_DESCRIPTION("Tunerdriver");
MODULE_AUTHOR("Manu Abraham; adapted by TDT");
MODULE_LICENSE("GPL");
// vim:ts=4