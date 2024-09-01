#ifndef __CORE_DVB__
#define __CORE_DVB__

#include "dvb_frontend.h"
#include "dvbdev.h"
#include "demux.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_filter.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

#include <linux/module.h>
#include "compat.h"
#include <linux/mutex.h>
#include "stv6110x.h"
#include "ix7306.h"
#include "stv090x.h"

enum tuner_type
{
	STB6100,
	STV6110X,
	SHARP7306,
};

struct core_config
{
	struct i2c_adapter *i2c_adap; /* i2c bus of the tuner */
	u8 i2c_addr; /* i2c address of the tuner */
	u8 i2c_addr_lnb_supply; /* i2c address of the lnb_supply */
	u8 vertical; /* i2c value */
	u8 horizontal; /* i2c value */
	struct stpio_pin *tuner_enable_pin;
	u8 tuner_enable_act; /* active state of the pin */

};

struct core_state
{
	struct dvb_frontend_ops ops;
	struct dvb_frontend frontend;

	const struct core_config *config;

	int thread_id;

	int not_responding;

};

struct core_info
{
	char *name;
	int type;
};

/* place to store all the necessary device information */
struct core
{

	/* devices */
	struct dvb_device dvb_dev;
	struct dvb_net dvb_net;

	struct core_info *card;

	unsigned char *grabbing;

	struct tasklet_struct fidb_tasklet;
	struct tasklet_struct vpe_tasklet;

	struct dmxdev dmxdev;
	struct dvb_demux demux;

	struct dmx_frontend hw_frontend;
	struct dmx_frontend mem_frontend;

	int ci_present;
	int video_port;

	u32 buffer_width;
	u32 buffer_height;
	u32 buffer_size;
	u32 buffer_warning_threshold;
	u32 buffer_warnings;
	unsigned long buffer_warning_time;

	u32 ttbp;
	int feeding;

	spinlock_t feedlock;

	spinlock_t debilock;

	struct dvb_adapter *dvb_adapter;
	struct dvb_frontend *frontend[MAX_TUNERS_PER_ADAPTER];
	int (*read_fe_status)(struct dvb_frontend *fe, fe_status_t *status);
	int fe_synced;

	void *priv;
};
extern void st90x_register_frontend(struct dvb_adapter *dvb_adap);
#endif
