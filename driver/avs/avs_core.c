/*
 *   avs_core.c - audio/video switch core driver - Kathrein UFS-910
 *
 *   written by captaintrip - 19.Nov 2007
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <asm/string.h>

#include "avs_core.h"
#include "ak4705.h"
#include "ak4708.h"
#include "stv6412.h"
#include "stv6417.h"
#include "stv6418.h"
#include "stv6419.h"
#include "cxa2161.h"
#include "vip2_avs.h"
#include "vip1_avs.h"
#include "fake_avs.h"
#include "avs_pio.h"
#include "avs_none.h"

static int debug = AVS_DEBUG;

enum
{
	AK4705,
	AK4708,
	STV6412,
	STV6417,
	STV6418,
	STV6419,
	CXA2161,
	VIP2_AVS,
	VIP1_AVS,
	FAKE_AVS,
	SPARK_AVS,
	SPARK7162_AVS,
	AVS_PIO,
	AVS_NONE,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
static const struct i2c_device_id avs_id[] = {
	{ "ak4705", AK4705 },
	{ "ak4708", AK4708 },
	{ "stv6412", STV6412 },
	{ "stv6417", STV6417 },
	{ "stv6418", STV6418 },
	{ "stv6419", STV6419 },
	{ "cxa2161", CXA2161 },
	{ "vip2_avs", VIP2_AVS },
	{ "vip1_avs", VIP1_AVS },
	{ "fake_avs", FAKE_AVS },
	{ "avs_pio", AVS_PIO },
	{ "avs_none", AVS_NONE },
	{ }
};
MODULE_DEVICE_TABLE(i2c, avs_id);
#endif

static int devType;
static char *type = "ak4705";

/*
 * Addresses to scan
 */
static unsigned short normal_i2c[] = {
#if defined(UFS922) \
 || defined(CUBEREVO) \
 || defined(VITAMIN_HD5000) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_2000HD)
	0x4a, /* stv6412" */
#elif defined(UFC960) \
 || defined(FORTIS_HDBOX) \
 || defined(TF7700) \
 || defined(HL101) \
 || defined(UFS912) \
 || defined(UFS913) \
 || defined(ATEVIO7500) \
 || defined(IPBOX9900) \
 || defined(IPBOX99) \
 || defined(ADB_BOX) \
 || defined(CUBEREVO_3000HD)
	0x4b, /* stv6412 / stv6417 / stv6418 */
#elif defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_250HD) \
 || defined(IPBOX55) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(ARIVALINK200)
	/* CUBEREVO_MINI_FTA does not register */
	/* CUBEREVO_250HD seems to use fake_avs, but does not register */
	0,
#else /* default */
	0x11, /* ak4705 */
#endif
	I2C_CLIENT_END
};
I2C_CLIENT_INSMOD;

static struct i2c_driver avs_i2c_driver;
static struct i2c_client *avs_client = NULL;

/*
 * i2c probe
 */

static int avs_newprobe(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (avs_client) {
		dprintk("[AVS]: failure, client already registered\n");
		return -ENODEV;
	}

	dprintk("[AVS]: chip found @ 0x%x\n", client->addr);

	switch(devType)
	{
	case AK4705:   ak4705_init(client);   break;
	case AK4708:   ak4708_init(client);   break;
	case STV6412:  stv6412_init(client);  break;
	case STV6417:  stv6417_init(client);  break;
	case STV6418:  stv6418_init(client);  break;
	case STV6419:  stv6419_init(client);  break;
	case CXA2161:  cxa2161_init(client);  break;
	case FAKE_AVS: fake_avs_init(client); break;
	case AVS_NONE: avs_none_init(client); break;
	default: return -ENODEV;
	}
	avs_client = client;
	return 0;
}

static int avs_remove(struct i2c_client *client)
{
	avs_client = NULL;
	dprintk("[AVS]: remove\n");
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int avs_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *client;
	int err;

	dprintk("[AVS]: attach\n");

	if (!(client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
		dprintk("[AVS]: attach nomem 1\n");
		return -ENOMEM;
	}

	/*
		NB: this code only works for a single client
	*/
	client->adapter = adap;
	client->addr = addr;
	client->driver = &avs_i2c_driver;
	strlcpy(client->name, "AVS", I2C_NAME_SIZE);

	if ((err = avs_newprobe(client, NULL))) {
		kfree(client);
		return err;
	}

	dprintk("[AVS]: attach final\n");
	i2c_attach_client(client);
	dprintk("[AVS]: attach final ok\n");

	return 0;
}

static int avs_probe(struct i2c_adapter *adap)
{
	int ret = 0;

#if !defined(CUBEREVO_MINI_FTA) && !defined(CUBEREVO_250HD) && !defined(IPBOX55)
	dprintk("[AVS]: probe\n");
	ret = i2c_probe(adap, &addr_data, avs_attach);
	dprintk("[AVS]: probe end %d\n",ret);
#endif

	return ret;
}

static int avs_detach(struct i2c_client *client)
{
	int err = avs_remove(client);
	i2c_detach_client(client);
	if (client)
		kfree(client);

	return err;
}

#endif /* < 2.6.30 */

/*
 * devfs fops
 */

static int avs_command_ioctl(struct i2c_client *client, unsigned int cmd, void *arg)
{
	int err = 0;

#if !defined(VIP1_V2) \
 && !defined(VIP2_V1) \
 && !defined(SPARK) \
 && !defined(SPARK7162) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7819) \
 && !defined(ATEMIO520) \
 && !defined(ATEMIO530) // none i2c avs !!!
	if (!client)
		return -1;
#endif

	dprintk("[AVS]: %s (%u)\n", __func__, cmd);

	switch(devType)
	{
	case AK4705:   err = ak4705_command(client, cmd, arg);   break;
	case AK4708:   err = ak4708_command(client, cmd, arg);   break;
	case STV6412:  err = stv6412_command(client, cmd, arg);  break;
	case STV6417:  err = stv6417_command(client, cmd, arg);  break;
	case STV6418:  err = stv6418_command(client, cmd, arg);  break;
	case STV6419:  err = stv6419_command(client, cmd, arg);  break;
	case CXA2161:  err = cxa2161_command(client, cmd, arg);  break;
	case FAKE_AVS: err = fake_avs_command(client, cmd, arg); break;
	case AVS_NONE: err = avs_none_command(client, cmd, arg); break;
	/* none i2c avs */
	case VIP2_AVS: 		err = vip2_avs_command(cmd, arg); 	 break;
	case VIP1_AVS: 		err = vip1_avs_command(cmd, arg);	 break;
	case AVS_PIO:		err = avs_pio_command(cmd, arg); 	 break;
	}
	return err;
}

int avs_command_kernel(unsigned int cmd, void *arg)
{
	int err = 0;

#if !defined(VIP1_V2) \
 && !defined(VIP2_V1) \
 && !defined(SPARK) \
 && !defined(SPARK7162) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7819) \
 && !defined(ATEMIO520) \
 && !defined(ATEMIO530) // i2c avs !!!
	struct i2c_client *client = avs_client;
	if (!client)
		return -1;
#endif

	dprintk("[AVS]: %s (%u)\n", __func__, cmd);

	switch(devType)
	{
#if defined(VIP1_V2) \
 || defined(VIP2_V1) \
 || defined(SPARK) \
 || defined(SPARK7162) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) // none i2c avs !!!
	case AVS_PIO: 		err = avs_pio_command_kernel(cmd, arg); 	break;
	case VIP2_AVS: 		err = vip2_avs_command_kernel(cmd, arg); 	break;
	case VIP1_AVS: 		err = vip1_avs_command_kernel(cmd, arg); 	break;
#else
	case AK4705:   err = ak4705_command_kernel(client, cmd, arg);   break;
	case AK4708:   err = ak4708_command_kernel(client, cmd, arg);   break;
	case STV6412:  err = stv6412_command_kernel(client, cmd, arg);  break;
	case STV6417:  err = stv6417_command_kernel(client, cmd, arg);  break;
	case STV6418:  err = stv6418_command_kernel(client, cmd, arg);  break;
	case STV6419:  err = stv6419_command_kernel(client, cmd, arg);  break;
	case CXA2161:  err = cxa2161_command_kernel(client, cmd, arg);  break;
	case FAKE_AVS: err = fake_avs_command_kernel(client, cmd, arg); break;
	case AVS_NONE: err = avs_none_command_kernel(client, cmd, arg); break;
#endif
	}
	return err;
}

EXPORT_SYMBOL(avs_command_kernel);

static int avs_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	dprintk("[AVS]: IOCTL\n");
	return avs_command_ioctl(avs_client, cmd, (void *) arg);
}

static struct file_operations avs_fops = {
	owner:		THIS_MODULE,
	ioctl:		avs_ioctl
};


static struct miscdevice avs_dev = {
	.minor = AVS_MINOR,
	.name = "avs",
	.fops = &avs_fops
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
static int avs_detect(struct i2c_client *client, int kind, const char **info)
#else
static int avs_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
#endif
{
	const char *name = "";

    /*
     * Now we do the remaining detection. A negative kind means that
     * the driver was loaded with no force parameter (default), so we
     * must both detect and identify the chip. A zero kind means that
     * the driver was loaded with the force parameter, the detection
     * step shall be skipped. A positive kind means that the driver
     * was loaded with the force parameter and a given kind of chip is
     * requested, so both the detection and the identification steps
     * are skipped.
     */

    /* Default to an ak4705 if forced */
    if (kind == 0) {
    	name = "ak4705";
    	kind = AK4705;
    }

	if (kind < 0) { /* detection and identification */
		if (!type || !strcmp("ak4705", type))
			kind = AK4705;
		else if (!strcmp("ak4708", type))
			kind = AK4708;
		else if (!strcmp("stv6412", type))
			kind = STV6412;
		else if(!strcmp("stv6417", type))
			kind = STV6417;
		else if(!strcmp("stv6418", type))
			kind = STV6418;
		else if(!strcmp("stv6419", type))
			kind = STV6419;
		else if(!strcmp("cxa2161", type))
			kind = CXA2161;
		else if(!strcmp("vip2_avs", type))
			kind = VIP2_AVS;
		else if(!strcmp("vip1_avs", type))
			kind = VIP1_AVS;
		else if(!strcmp("fake_avs", type))
			kind = FAKE_AVS;
		else if(!strcmp("avs_pio", type))
			kind = AVS_PIO;
		else if(!strcmp("avs_none", type))
			kind = AVS_NONE;
		else
			return -ENODEV;
	}
	switch (kind) {
	case AK4705:   		name = "ak4705";   		break;
	case AK4708:   		name = "ak4708";   		break;
	case STV6412:  		name = "stv6412";  		break;
	case STV6417:  		name = "stv6417";  		break;
	case STV6418:  		name = "stv6418";  		break;
	case STV6419:  		name = "stv6419";  		break;
	case CXA2161:  		name = "cxa2161";  		break;
	case VIP2_AVS: 		name = "vip2_avs"; 		break;
	case VIP1_AVS: 		name = "vip1_avs"; 		break;
	case FAKE_AVS: 		name = "fake_avs"; 		break;
	case AVS_PIO: 		name = "avs_pio"; 		break;
	case AVS_NONE: 		name = "avs_none"; 		break;
	default: return -ENODEV;
	}
	devType = kind;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	*info = name;
#else
	strlcpy(info->type, name, I2C_NAME_SIZE);
#endif
	return 0;
}

/*
 * i2c
 */

static struct i2c_driver avs_i2c_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	.driver = {
		.owner = THIS_MODULE,
		.name = "avs_driver", /* can this be avs_driver? */
	},
	.id = I2C_DRIVERID_AVS,
	.attach_adapter = avs_probe,
	.detach_client = avs_detach,
#else
	.class = I2C_CLASS_TV_DIGITAL,
	.driver = {
		.owner = THIS_MODULE,
		.name = "avs_driver", /* 2.6.30 requires name without spaces */
	},
	.probe = avs_newprobe,
	.detect = avs_detect,
	.remove = avs_remove,
	.id_table = avs_id,
	.address_data = &addr_data,
#endif
	.command = avs_command_ioctl
};

/*
 * module init/exit
 */

int __init avs_init(void)
{
	int res, err;
	const char *name;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	err = avs_detect(NULL, -1, &name);
#else
	struct i2c_board_info info;
	err = avs_detect(NULL, -1, &info);
	name = info.type;
#endif

	if (err) {
		printk("[AVS]: Unknown AVS Driver!\n");
		return err;
	}

	dprintk("[AVS]: A/V switch handling for %s\n", name);

#if !defined(CUBEREVO_MINI_FTA) && !defined(CUBEREVO_250HD)
	if ((devType != FAKE_AVS) && (devType != AVS_NONE) && (devType != VIP2_AVS)
		&& (devType != VIP1_AVS) && (devType != AVS_PIO)) {
		if ((res = i2c_add_driver(&avs_i2c_driver))) {
			dprintk("[AVS]: i2c add driver failed\n");
			return res;
		}

		if (!avs_client){
			printk("[AVS]: no client found\n");
			i2c_del_driver(&avs_i2c_driver);
			return -EIO;
		}
	}else if (devType == AVS_PIO){
		if(avs_pio_init() != 0)
		{
			printk("[AVS]: init pio avs faild!\n");
			return -EIO;
		}
	}else if (devType == VIP2_AVS){
		if(vip2_avs_init() != 0)
		{
			printk("[AVS]: init vip2 avs faild!\n");
			return -EIO;
		}
	}else if (devType == VIP1_AVS){
		if(vip1_avs_init() != 0)
		{
			printk("[AVS]: init vip1 avs faild!\n");
			return -EIO;
		}
	}
#endif

	if (misc_register(&avs_dev)<0){
		printk("[AVS]: unable to register device\n");
		return -EIO;
	}

	return 0;
}

void __exit avs_exit(void)
{
	misc_deregister(&avs_dev);
	i2c_del_driver(&avs_i2c_driver);
	if(devType == AVS_PIO )
		avs_pio_exit();
}

module_init(avs_init);
module_exit(avs_exit);

MODULE_AUTHOR("Team Ducktales");
MODULE_DESCRIPTION("Multiplatform A/V scart switch driver");
MODULE_LICENSE("GPL");

module_param(type,charp,0);
MODULE_PARM_DESC(type, "device type (ak4705, ak4708, stv6412, cxa2161, stv6417, stv6418, stv6419, vip2_avs, vip1_avs, fake_avs, avs_pio, avs_none)");

