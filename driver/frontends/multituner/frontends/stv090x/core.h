#ifndef __CORE_DVB__
#define __CORE_DVB__

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
#endif
// vim:ts=4
