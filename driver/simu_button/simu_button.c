/*
 * simu_button.c
 * 
 * (c) 2008 teamducktales
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/*
 * RC event driver
 */

#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include "simu_button.h"

static char *button_driver_name = "TDT RC event driver";
static struct input_dev *button_dev;

int button_dev_init(void)
{
	int error;
	int vLoop = 0;

	dprintk("allocating and registering button device\n");

	button_dev = input_allocate_device();
	if (!button_dev)
		return -ENOMEM;

	button_dev->name = button_driver_name;
	button_dev->open = NULL;
	button_dev->close = NULL;


	set_bit(EV_KEY, button_dev->evbit);
  set_bit(EV_REP, button_dev->evbit);
	
	for(vLoop = 0; vLoop < KEY_MAX; vLoop++)
		set_bit(vLoop, button_dev->keybit); 


	error = input_register_device(button_dev);
	if (error) {
		input_free_device(button_dev);
		return error;
	}

	return 0;
}

void button_dev_exit(void)
{
	dprintk("unregistering button device\n");
	input_unregister_device(button_dev);
}

int __init button_init(void)
{
	dprintk("initializing ...\n");

	if(button_dev_init() != 0)
		return 1;

	return 0;
}

void __exit button_exit(void)
{
	dprintk("unloading ...\n");

	button_dev_exit();
}

module_init(button_init);
module_exit(button_exit);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled 1=enabled(default)");

MODULE_DESCRIPTION("TDT RC event driver");
MODULE_AUTHOR("Team Ducktales");
MODULE_LICENSE("GPL");
