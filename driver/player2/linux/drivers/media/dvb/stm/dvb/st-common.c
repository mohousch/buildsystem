/*
 * ST DVB driver
 *
 * Copyright (c) STMicroelectronics 2005
 *
 * Author:Peter Bennett <peter.bennett@st.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/errno.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif

#include <asm/io.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/bpa2.h>
#else
#include <linux/bigphysarea.h>
#endif

#include "dvb_frontend.h"
#include "dmxdev.h"
#include "dvb_demux.h"
#include "dvb_net.h"

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/version.h>

#include "st-common.h"
#if (DVB_API_VERSION > 3)
DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);
#endif

//__TDT__ ->file moved in this directory

int debug = 0;

struct stfe_channel *stfe_channel_allocate(struct stfe *stfe)
{
	int i;
	if (down_interruptible(&stfe->sem))
		return NULL;
	/* lock! */
	for (i = 0; i < STFE_MAXCHANNEL; ++i)
	{
		if (!stfe->channel[i].active)
		{
			stfe->channel[i].active = 1;
			up(&stfe->sem);
			return stfe->channel + i;
		}
	}
	up(&stfe->sem);
	return NULL;
}

struct stfe *stfe_create(void *private_data, void *start_feed, void *stop_feed)
{
	struct stfe *stfe;
	int channel;
	int result;
	if (!(stfe = kmalloc(sizeof(struct stfe), GFP_KERNEL)))
		return NULL;
	printk("%s: Allocated stfe @ 0x%p\n", __FUNCTION__, stfe);
	memset(stfe, 0, sizeof(struct stfe));
	sema_init(&stfe->sem, 0);
	up(&stfe->sem);
	for (channel = 0; channel < STFE_MAXCHANNEL; ++channel)
	{
		stfe->channel[channel].id = channel;
		stfe->channel[channel].stfe = stfe;
		stfe->channel[channel].havana_id = private_data;
	}
	dvb_register_adapter(&stfe->adapter, "ST Generic Front End Driver", THIS_MODULE, NULL, adapter_nr);
	stfe->adapter.priv = stfe;
	memset(&stfe->dvb_demux, 0, sizeof(stfe->dvb_demux));
	stfe->dvb_demux.dmx.capabilities =
		DMX_TS_FILTERING | DMX_SECTION_FILTERING | DMX_MEMORY_BASED_FILTERING;
	stfe->dvb_demux.priv = private_data;
	stfe->dvb_demux.filternum = 48;
	stfe->dvb_demux.feednum = STFE_MAXCHANNEL;
	stfe->dvb_demux.start_feed = start_feed;
	stfe->dvb_demux.stop_feed = stop_feed;
	stfe->dvb_demux.write_to_decoder = NULL; /* write_to_decoder;*/
	if ((result = dvb_dmx_init(&stfe->dvb_demux)) < 0)
	{
		printk("stfe_init: dvb_dmx_init failed (errno = %d)\n",
			   result);
		goto err;
	}
	stfe->dmxdev.filternum = stfe->dvb_demux.filternum;
	stfe->dmxdev.demux = &stfe->dvb_demux.dmx;
	stfe->dmxdev.capabilities = 0;
	if ((result = dvb_dmxdev_init(&stfe->dmxdev, &stfe->adapter)) < 0)
	{
		printk("stfe_init: dvb_dmxdev_init failed (errno = %d)\n",
			   result);
		dvb_dmx_release(&stfe->dvb_demux);
		goto err;
	}
	stfe->hw_frontend.source = DMX_FRONTEND_0;
	result = stfe->dvb_demux.dmx.add_frontend(&stfe->dvb_demux.dmx, &stfe->hw_frontend);
	if (result < 0)
		return NULL;
	stfe->mem_frontend.source = DMX_MEMORY_FE;
	result = stfe->dvb_demux.dmx.add_frontend(&stfe->dvb_demux.dmx, &stfe->mem_frontend);
	if (result < 0)
		return NULL;
	result = stfe->dvb_demux.dmx.connect_frontend(&stfe->dvb_demux.dmx, &stfe->hw_frontend);
	if (result < 0)
		return NULL;
	stfe->driver_data = private_data;
//i2c_add_driver(&st_tuner_i2c_driver);
	return stfe;
err:
	return NULL;
}

