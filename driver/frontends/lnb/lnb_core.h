
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

#define I2C_DRIVERID_LNB 1
#define LNB_MINOR 0
#define LNB_MAJOR 149

/* IOCTL */
#define LNB_VOLTAGE_OFF 0x2b0010
#define LNB_VOLTAGE_VER 0x2b0011
#define LNB_VOLTAGE_HOR 0x2b0012

extern short paramDebug;
#define TAGDEBUG "[LNB] "

#define dprintk(level, x...) do { \
		if ((paramDebug) && (level <= paramDebug)) printk(TAGDEBUG x); \
	} while (0)

int a8293_init(struct i2c_client *client);
int a8293_command(struct i2c_client *client, unsigned int cmd, void *arg);
int a8293_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);

int lnb24_init(struct i2c_client *client);
int lnb24_command(struct i2c_client *client, unsigned int cmd, void *arg);
int lnb24_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);

int lnb_pio_init(void);
int lnb_pio_exit(void);
int lnb_pio_command(unsigned int cmd, void *arg);
int lnb_pio_command_kernel(unsigned int cmd, void *arg);

int lnb_init(void);

