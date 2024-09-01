/*
 * lnb24.c - Dummy LNB power controller
 *
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/types.h>

#include <linux/i2c.h>

#include "lnb_core.h"

int lnb24_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
	dprintk(10, "%s (%x)\n", __func__, cmd);
	switch (cmd)
	{
		case LNB_VOLTAGE_OFF:
		{
			dprintk(20, "Switch LNB power off\n");
		}
		case LNB_VOLTAGE_VER:
		{
			dprintk(20, "Set LNB voltage vertical\n");
		}
		case LNB_VOLTAGE_HOR:
		{
			dprintk(20, "Set LNB voltage horizontal\n");
		}
	}
	return 0;
}

int lnb24_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return lnb24_command_kernel(client, cmd, NULL);
}

int lnb24_init(struct i2c_client *client)
{
	dprintk(10, "%s\n", __func__);
	return 0;
}

