#ifndef DVB_D0367_FE_OFDM_H
#define DVB_D0367_FE_OFDM_H

#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

extern struct dvb_frontend *
dvb_d0367_fe_ofdm_attach(struct i2c_adapter *i2c);
extern struct dvb_frontend *
dvb_d0367_fe_qam_attach(struct i2c_adapter *i2c);
extern int demod_d0367ter_Identify(struct i2c_adapter *i2c, U8 ucID);

#endif // DVB_D0367_FE_OFDM_H

