/*
 *   avs_none.c - 
 *
 *   For av receiver without av switch. the e2_core in stmdvb need some functions
 *   from avs module but fake_avs is not a real fake because it sets pio pins.
 *
 *   mainly based on avs_core.c from Gillem gillem@berlios.de / Tuxbox-Project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#include <linux/stpio.h>
#else
#include <linux/stm/pio.h>
#endif

#include <linux/i2c.h>

#include "avs_core.h"

/* ---------------------------------------------------------------------- */
 
int avs_none_command(struct i2c_client *client, unsigned int cmd, void *arg )
{
	return 0;
}

/* ---------------------------------------------------------------------- */
 
int avs_none_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return 0;
}

/* ---------------------------------------------------------------------- */

int avs_none_init(struct i2c_client *client)
{
	return 0;
}

/* ---------------------------------------------------------------------- */
