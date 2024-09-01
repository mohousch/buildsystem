#ifndef equipment_123
#define equipment_123

//fixme: dont like this very much - think on int

#include "dvb_frontend.h"

struct equipment_s
{
	/* must be set by demod */
	u16(*demod_i2c_repeater_send)(void *state, u8 *buf, u16 size);
	u16(*demod_i2c_repeater_recv)(void *state, u8 *buf, u16 size);
	u16(*demod_i2c_write)(void *state, u8 *buf, u16 buf_size);
	u16(*demod_i2c_write16)(void *state, u32 addr, u16 data);
	u16(*demod_send_op)(u8 ucOpCmd, void *state);
	u16(*demod_get_op_status)(void *state);
	u16(*demod_i2c_read16)(void *state, u32 addr, u16 *data);

	/* must be set by tuner */
	u16(*tuner_load_fw)(struct dvb_frontend *fe);
	u16(*tuner_init)(struct dvb_frontend *fe);
	u16(*tuner_lock)(struct dvb_frontend *fe, u32 frequency, u32 srate, u32 _lfp);
	u16(*tuner_lock_status)(struct dvb_frontend *fe);

	/* must be set by lnb */
	u16(*lnb_set_voltage)(void *lnb_priv, struct dvb_frontend *fe, fe_sec_voltage_t voltage);
};
#endif
// vim:ts=4