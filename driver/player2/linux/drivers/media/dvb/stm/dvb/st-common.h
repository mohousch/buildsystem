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
#ifndef _ST_COMMON_H
#define _ST_COMMON_H

#include "dvb_frontend.h"
#include "dmxdev.h"
#include "dvb_demux.h"
#include "dvb_net.h"

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

extern int debug ;
#define dprintk(x...) do { if (debug) printk(KERN_WARNING x); } while (0)

/* Maximum number of channels */
#define STFE_MAXADAPTER (4)
#define STFE_MAXCHANNEL 48

struct stfe
{
	struct dvb_demux dvb_demux;
	struct dmxdev dmxdev;

	struct dmx_frontend hw_frontend;
	struct dmx_frontend mem_frontend;

	int mapping;

	//struct dvb_net dvbnet;

	/* our semaphore, for channel allocation/deallocation */
	struct semaphore sem;

	struct dvb_adapter adapter;

	struct stfe_channel
	{
		void *havana_id; // Havana_id must be the first as havana does not have access to this structure
		struct stfe *stfe;
		struct dvb_demux_feed *dvbdmxfeed;

		int active;
		int id;
		int pid;
		int type; /* 1 - TS, 2 - Filter */
	} channel[STFE_MAXCHANNEL];

	//spinlock_t timer_lock;
	//struct timer_list timer; /* timer interrupts for outputs */

	struct dvb_frontend *fe;

	int running_feed_count;

	void *tuner_data;
	void *driver_data;
};

struct stfe_channel *stfe_channel_allocate(struct stfe *stfe);
//struct stfe *stfe_init(struct dvb_adapter *adapter,void *private_data, void *start_feed, void *stop_feed, void *write_to_decoder);

struct stfe *stfe_create(void *private_data, void *start_feed, void *stop_feed);
#endif
