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
#include "stv6110x.h"

#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

#include <linux/version.h>
#include <linux/module.h>
#include <linux/dvb/version.h>

#include <linux/stm/pio.h>

#if DVB_API_VERSION < 5
#include "compat.h"
#endif

#include <linux/mutex.h>
#include "stv090x.h"

enum
{
	STV6110X = 0,
	SHARP7306,
};

struct core_config
{
	struct i2c_adapter *i2c_adap; /* i2c bus of the tuner */
	struct stpio_pin *tuner_enable_pin;
	u8 i2c_addr; /* i2c address of the tuner */
	u8 i2c_addr_lnb_supply; /* i2c address of the lnb_supply */
	u8 vertical; /* i2c value */
	u8 horizontal; /* i2c value */
	u8 tuner_enable_act; /* active state of the pin */
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

struct tuner_devctl
{
	int (*tuner_init)(struct dvb_frontend *fe);
	int (*tuner_sleep)(struct dvb_frontend *fe);
	int (*tuner_set_mode)(struct dvb_frontend *fe, enum tuner_mode mode);
	int (*tuner_set_frequency)(struct dvb_frontend *fe, u32 frequency);
	int (*tuner_get_frequency)(struct dvb_frontend *fe, u32 *frequency);
	int (*tuner_set_bandwidth)(struct dvb_frontend *fe, u32 bandwidth);
	int (*tuner_get_bandwidth)(struct dvb_frontend *fe, u32 *bandwidth);
	int (*tuner_set_bbgain)(struct dvb_frontend *fe, u32 gain);
	int (*tuner_get_bbgain)(struct dvb_frontend *fe, u32 *gain);
	int (*tuner_set_refclk)(struct dvb_frontend *fe, u32 refclk);
	int (*tuner_get_status)(struct dvb_frontend *fe, u32 *status);
};

extern void st90x_register_frontend(struct dvb_adapter *dvb_adap);
#endif
