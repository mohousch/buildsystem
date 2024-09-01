/*
 * @brief socket.c
 *
 * @author konfetti
 *
 * @brief handling of mixed tuner's in one box.
 *
 * Copyright (C) 2011 duckbox project
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
#include <linux/version.h>

#include "socket.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#define dprintk(level, x...) do \
{ \
	if ((paramDebug) && (paramDebug >= level) || level == 0) \
	{ \
		printk(TAGDEBUG x); \
	} \
} while (0)

#define TAGDEBUG "[socket] "

static int         numSockets = 0;
struct socket_s    *sockets = NULL;
struct dvb_adapter *adapter = NULL;

#define cMaxFrontends 8

static int         numFrontends = 0;
struct frontend_s  frontends[cMaxFrontends];

void addFrontend(struct frontend_s *frontend)
{
	frontends[numFrontends] = *frontend;
	numFrontends++;
}

/* ******************************* */
/* public functions                */
/* ******************************* */

/* will be called from the demods/frontends device probe function */
int socket_register_frontend(struct frontend_s *frontend)
{
	if (numFrontends + 1 == cMaxFrontends)
	{
		dprintk(1, "Error: maximum number frontends reached\n");
		return -1;
	}
	addFrontend(frontend);
	return 0;
}
EXPORT_SYMBOL(socket_register_frontend);

/* will be called from stmdvb.ko */
int socket_register_adapter(struct dvb_adapter *_adapter)
{
	int i, j;

	if (adapter != NULL)
	{
		dprintk(1, "%s: Error, socket already in use\n", __func__);
		return -ENODEV;
	}
	adapter = _adapter;

	for (i = 0; i < numSockets; i++)
	{
		for (j = 0; j < numFrontends; j++)
		{
			if (frontends[j].demod_detect(&sockets[i], &frontends[j]) == 0)
			{
				dprintk(10, "Found %s on socket %s\n", frontends[j].name, sockets[i].name);

				frontends[j].demod_attach(adapter, &sockets[i], &frontends[j]);
				break;
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(socket_register_adapter);

/* ******************************* */
/* driver functions                */
/* ******************************* */

static int socket_probe(struct platform_device *pdev)
{
	struct tunersocket_s *socket_config = pdev->dev.platform_data;
	int    i;

	dprintk(100, "%s >\n", __func__);

	numSockets = socket_config->numSockets;
	sockets = (struct socket_s *) kzalloc(sizeof(struct socket_s) * numSockets, GFP_KERNEL);
	if (sockets == NULL)
	{
		printk(KERN_ERR "%s: kzalloc failed.\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < numSockets; i++)
	{
		sockets[i] = socket_config->socketList[i];

		dprintk(0, "***************************\n");
		dprintk(0, "socket     %d\n", i + 1);
		dprintk(0, "name       %s\n", sockets[i].name);
		dprintk(0, "pio port   %d\n", sockets[i].tuner_enable[0]);
		dprintk(0, "pio pin    %d\n", sockets[i].tuner_enable[1]);
		dprintk(0, "active l/h %d\n", sockets[i].tuner_enable[2]);
		dprintk(0, "i2c_bus    %d\n", sockets[i].i2c_bus);
	}
	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int socket_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver socket_driver =
{
	.probe  = socket_probe,
	.remove = socket_remove,
	.driver	=
	{
		.name  = "socket",
		.owner = THIS_MODULE,
	},
};

/* ******************************* */
/* module functions                */
/* ******************************* */

int __init socket_init(void)
{
	int ret;

	dprintk(100, "%s >\n", __func__);

	ret = platform_driver_register(&socket_driver);
	dprintk(100, "%s < %d\n", __func__, ret);
	return ret;
}

static void socket_cleanup(void)
{
	dprintk(100, "%s >\n", __func__);

	driver_unregister((struct device_driver *)&socket_driver);
}

module_init(socket_init);
module_exit(socket_cleanup);

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Activates frontend debugging (default:0)");

MODULE_DESCRIPTION("Multituner Socket Handling");

MODULE_AUTHOR("konfetti");
MODULE_LICENSE("GPL");
// vim:ts=4
