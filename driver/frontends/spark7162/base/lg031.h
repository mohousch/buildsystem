
#ifndef __LG031_H
#define __LG031_H

struct lg031_config
{
	char name[128];

	u8 addr;
	fe_bandwidth_t bandwidth;
	u32 Frequency;
	u32 IF;
	u32 TunerStep;
};
extern struct dvb_frontend *lg031_attach(struct dvb_frontend *fe,
										 const struct lg031_config *config,
										 struct i2c_adapter *i2c);

#endif /* __LG031_H */

