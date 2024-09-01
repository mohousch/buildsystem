/*
 *
 * (c) 2010 konfetti, schischu
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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/platform_device.h>

#include <asm/system.h>
#include <asm/io.h>

#include <linux/interrupt.h>
#include <linux/version.h>

#include "cec_debug.h"
#include "cec_worker.h"
#include "cec_opcodes.h"
#include "cec_opcodes_def.h"
#include "cec_internal.h"
#include "cec_proc.h"
#include "cec_rc.h"
#include "cec_dev.h"

//----------------------------

static unsigned char cancelStart = 0;
int activemode = 0;
int debug = 0;
char *deviceName = "DUCKBOX";
unsigned char deviceType = DEVICE_TYPE_DVD;

//----------------------------

int __init cec_init(void)
{
	dprintk(0, "init - starting\n");

	cec_internal_init();

	udelay(10000);

	startTask();

	/* ********* */
	/* irq setup */

	dprintk(2, "init - starting interrupt (%d)\n", CEC_IRQ);

#if defined (CONFIG_KERNELVERSION) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
	if (!request_irq(CEC_IRQ, (void *)cec_interrupt, IRQF_DISABLED, "cec", NULL))
#else
	if (!request_irq(CEC_IRQ, (void *)cec_interrupt, SA_INTERRUPT, "cec", NULL))
#endif
	{

	}
	else
	{
		dprintk(0, "Can't get irq\n");
	}

	if (activemode)
	{
		init_e2_proc();

		input_init();
	}
	else
	{
		init_dev();
	}

	// TODO: how can we implement hotplug support?
	//while(!cancelStart && getPhysicalAddress() == 0) // HDMI is not plugged in
	//  msleep(200);

	cec_worker_init();

	return 0;
}

static void __exit cec_exit(void)
{
	dprintk(0, "unloaded\n");

	cancelStart = 1;
	udelay(20000);

	endTask();

	if (activemode)
	{
		cleanup_e2_proc();

		input_cleanup();
	}
	else
	{
		cleanup_dev();
	}

	free_irq(CEC_IRQ, NULL);

	cec_internal_exit();
}

module_init(cec_init);
module_exit(cec_exit);

MODULE_DESCRIPTION("CEC Driver");
MODULE_AUTHOR("konfetti & schischu");
MODULE_LICENSE("GPL");

module_param(debug, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(activemode, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(deviceType, byte, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
module_param(deviceName, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, "Debug Output 0=disabled >0=enabled(debuglevel)");
MODULE_PARM_DESC(activemode, "Active mode 0=disabled >0=enabled(activemode)");
MODULE_PARM_DESC(deviceType, "Device type (default: 4 (DVD))");
MODULE_PARM_DESC(deviceName, "Name (default: DUCKBOX)");

