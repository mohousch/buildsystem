/*
 *  siinfo.c - core box info
 *
 */

#include <linux/module.h>	/* Specifically, a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	
#include <linux/init.h>
#include <linux/i2c.h>
#include <asm/uaccess.h>

#include "siinfo.h"

#define SIINFO_MODULE_VERSION 102 // 1.00

#define MSGHEADER "siinfo: "

#define CICAM_I2C_BUS		2
#define CICAM_I2C_ADDR		(0x80>>1)
#define AVS_I2C_BUS		2
#define AVS_I2C_ADDR		(0x96>>1)

static int detected_boxid;

// -------------------------------------------------------------

int siinfo_get_boxid(void)
{
	return detected_boxid;
}
EXPORT_SYMBOL(siinfo_get_boxid);

static int i2c_autodetect (struct i2c_adapter *adapter, unsigned char i2c_addr, unsigned char dev_addr)
{
  	unsigned char buf[2] = { 0, 0 };
  	struct i2c_msg msg[] = { 
		{ .addr = i2c_addr, .flags = 0, .buf = &dev_addr, .len = 1 },
		{ .addr = i2c_addr, .flags = I2C_M_RD, .buf = &buf[0], .len = 1 } 
	};
  	int b;

  	b = i2c_transfer(adapter,msg,1);
  	b |= i2c_transfer(adapter,msg+1,1);

  	if (b != 1) 
		return -1;

  	return buf[0];
}

static int detect_boxid(void)
{
	struct i2c_adapter *adap;

	// model detection:
	//   1, cicam check? ok => 9900hd
	if((adap = i2c_get_adapter(CICAM_I2C_BUS)) == NULL) {
		printk(MSGHEADER "Autodetection failed. I2C bus error\n");
		return 0;
	}
  	if(i2c_autodetect(adap, CICAM_I2C_ADDR, 0x0) != -1 ) {
		printk(MSGHEADER "Detected model 9900HD\n");
		return SIINFO_BOXID_9900HD;
	}
	//   2, avs check? fail => 55hd
	if((adap = i2c_get_adapter(AVS_I2C_BUS)) == NULL) {
		printk(MSGHEADER "Autodetection failed. I2C bus error\n");
		return 0;
	}
  	if(i2c_autodetect(adap, AVS_I2C_ADDR, 0x0) < 0 ) {
		printk(MSGHEADER "Detected model 55HD\n");
		return SIINFO_BOXID_55HD;
	}
	//   => 99hd
	printk(MSGHEADER "Detected model 55HD\n");
	return SIINFO_BOXID_99HD;
}

int siinfo_get_nims(int boxid)
{
	if(!boxid)
		boxid = detected_boxid;

	switch(boxid) {
		case SIINFO_BOXID_55HD:
			return 1;
		case SIINFO_BOXID_99HD:
		case SIINFO_BOXID_9900HD:
			return 2;
		default:
			return 0;
	}
}
EXPORT_SYMBOL(siinfo_get_nims);

int siinfo_get_cicams(int boxid)
{
	if(!boxid)
		boxid = detected_boxid;

	switch(boxid) {
		case SIINFO_BOXID_55HD:
		case SIINFO_BOXID_99HD:
		default:
			return 0;
		case SIINFO_BOXID_9900HD:
			return 2;
	}
}
EXPORT_SYMBOL(siinfo_get_cicams);

int siinfo_get_creaders(int boxid)
{
	if(!boxid)
		boxid = detected_boxid;

	switch(boxid) {
		case SIINFO_BOXID_55HD:
		case SIINFO_BOXID_99HD:
			return 1;
		case SIINFO_BOXID_9900HD:
			return 2;
		default:
			return 0;
	}
}
EXPORT_SYMBOL(siinfo_get_creaders);

int siinfo_get_avs(int boxid)
{
	if(!boxid)
		boxid = detected_boxid;

	switch(boxid) {
		case SIINFO_BOXID_55HD:
		default:
			return 0;
		case SIINFO_BOXID_99HD:
		case SIINFO_BOXID_9900HD:
			return 1;
	}
}
EXPORT_SYMBOL(siinfo_get_avs);

int siinfo_get_usbs(int boxid)
{
	if(!boxid)
		boxid = detected_boxid;

	switch(boxid) {
		case SIINFO_BOXID_55HD:
		case SIINFO_BOXID_99HD:
			return 1;
		case SIINFO_BOXID_9900HD:
			return 2;
		default:
			return 0;
	}
}
EXPORT_SYMBOL(siinfo_get_usbs);

int siinfo_get_rfmods(int boxid)
{
	if(!boxid)
		boxid = detected_boxid;

	switch(boxid) {
		case SIINFO_BOXID_55HD:
		case SIINFO_BOXID_99HD:
		default:
			return 0;
		case SIINFO_BOXID_9900HD:
			return 1;
	}
}
EXPORT_SYMBOL(siinfo_get_rfmods);

int siinfo_get_fans(int boxid)
{
	if(!boxid)
		boxid = detected_boxid;

	switch(boxid) {
		case SIINFO_BOXID_55HD:
		default:
			return 0;
		case SIINFO_BOXID_99HD:
		case SIINFO_BOXID_9900HD:
			return 1;
	}
}
EXPORT_SYMBOL(siinfo_get_fans);

// -------------------------------------------------------------

int __init siinfo_init(void)
{
	printk("Sisyfos informer, version %d.%d, (c) 2010 Sisyfos\n",
			SIINFO_MODULE_VERSION / 100, SIINFO_MODULE_VERSION % 100);

	// get serial number
	detected_boxid = detect_boxid();
	
	// FIXME: double check by reading eeprom and test serial?
	
	return 0;
}

void __exit siinfo_exit(void)
{
	printk(MSGHEADER "unloaded\n");
}

module_init(siinfo_init);
module_exit(siinfo_exit);

MODULE_AUTHOR("sisyfos");
MODULE_DESCRIPTION("Sisyfos informer");
MODULE_LICENSE("GPL");


