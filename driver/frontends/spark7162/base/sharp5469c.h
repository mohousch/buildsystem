
#ifndef __SHARP5469C_H
#define __SHARP5469C_H

struct sharp5469c_config
{
	char name[128];

	u8 addr;
};
extern struct dvb_frontend *sharp5469c_attach(struct dvb_frontend *fe,
											  const struct sharp5469c_config *config,
											  struct i2c_adapter *i2c);

#endif /* __SHARP5469C_H */

