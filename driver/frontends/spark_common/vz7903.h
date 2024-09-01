#ifndef __LLD_VZ7903_H
#define __LLD_VZ7903_H

#define REF_OSC_FREQ 4000 /* 4MHZ */

#ifdef __cplusplus
extern "C"
{
#endif

//#include "porting.h"

struct vz7903_config
{
	u8 I2cAddr;
	u8 DemodI2cAddr;

};
struct vz7903_state
{
	struct dvb_frontend *fe;
	struct i2c_adapter *i2c;
	struct vz7903_config *config;

	/* state cache */
	u32 frequency;
	u32 symbolrate;
	u32 bandwidth;

	u32 index;
};

int nim_vz7903_init(u8 *ptuner_id);
//INT32 nim_vz7903_init(UINT8 *ptuner_id);

int nim_vz7903_control(u8 tuner_id, u32 freq, u32 sym);
int nim_vz7903_status(u8 tuner_id, u8 *lock);

struct dvb_frontend *vz7903_attach(struct dvb_frontend *fe,
								   struct vz7903_config *i2cConfig,
								   struct i2c_adapter *i2c);
int vz7903_get_frequency(struct dvb_frontend *fe, u32 *frequency);
int vz7903_set_frequency(struct dvb_frontend *fe, u32 frequency);
int vz7903_set_bandwidth(struct dvb_frontend *fe, u32 bandwidth);
int vz7903_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth);
int vz7903_get_status(struct dvb_frontend *fe, u32 *status);

int tuner_Sharp7903_Identify(struct dvb_frontend *fe,
							 struct vz7903_config *I2cConfig,
							 struct i2c_adapter *i2c);

#ifdef __cplusplus
}
#endif

#endif /* __LLD_VZ7903_H__ */

