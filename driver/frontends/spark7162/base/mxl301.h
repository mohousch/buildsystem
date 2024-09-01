
#ifndef __MXL301_H
#define __MXL301_H

struct MXL301_config
{
	char name[128];

	u8 addr;
	fe_bandwidth_t bandwidth;
	u32 Frequency;
	u32 IF;
	u32 TunerStep;
};
extern struct dvb_frontend *mxl301_attach(struct dvb_frontend *fe,
										  const struct MXL301_config *config,
										  struct i2c_adapter *i2c);

#endif

