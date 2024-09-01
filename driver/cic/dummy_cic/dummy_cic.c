/*
 * dummy ci controller handling.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

#include <asm/system.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/mutex.h>
#include <linux/dvb/dmx.h>

#include "dvb_frontend.h"
#include "dvbdev.h"
#include "demux.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_filter.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

static int debug=0;

#define TAGDEBUG "[dummy_cic] "

#define dprintk(level, x...) do { \
if ((debug) && (debug >= level)) printk(TAGDEBUG x); \
} while (0)

#ifdef UFS912
static int waitMS = 200; //100 enough for AClight but not for Diablo
#else
static int waitMS = 100;
#endif


int setCiSource(int slot, int source)
{
   return 0;
}

void getCiSource(int slot, int* source)
{
   *source = 0;
}

int cic_init_hw(void)
{
     return -1;
}

EXPORT_SYMBOL(cic_init_hw);

int init_ci_controller(struct dvb_adapter* dvb_adap)
{
	printk("init_dummy_cic >\n");

	return 0;
}

EXPORT_SYMBOL(init_ci_controller);
EXPORT_SYMBOL(setCiSource);
EXPORT_SYMBOL(getCiSource);

int __init dummy_cic_init(void)
{
    printk("dummy_cic loaded\n");
    return 0;
}

static void __exit dummy_cic_exit(void)
{  
   printk("dummy_cic unloaded\n");
}

module_init             (dummy_cic_init);
module_exit             (dummy_cic_exit);

MODULE_DESCRIPTION      ("CI Controller");
MODULE_AUTHOR           ("Spider-Team");
MODULE_LICENSE          ("GPL");

module_param(debug, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(waitMS, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(waitMS, "waiting time between pio settings for reset/enable purpos in milliseconds (default=200)");
