
#ifndef __SHARP6465_H
#define __SHARP6465_H

struct sharp6465_config
{
	char name[128];

	u8 addr;
	fe_bandwidth_t bandwidth;
	u32 Frequency;
	u32 IF;
	u32 TunerStep;
};
extern struct dvb_frontend *sharp6465_attach(struct dvb_frontend *fe,
											 const struct sharp6465_config *config,
											 struct i2c_adapter *i2c);

#endif /* __SHARP6465_H */

