#ifndef _DVB_CA_CORE_H_
#define _DVB_CA_CORE_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

#define SLOTSTATUS_NONE		1
#define SLOTSTATUS_PRESENT	2
#define SLOTSTATUS_RESET	4
#define SLOTSTATUS_READY	8

#define cNumberSlots 2

struct dvb_ca_state {
	struct dvb_adapter		*dvb_adap;
	struct dvb_ca_en50221   ca;
    struct i2c_adapter      *i2c;
    int					    i2c_addr;

    int                     module_status[cNumberSlots];
};

int init_cimax(struct dvb_adapter *dvb_adap);
int setCiSource(int slot, int source);

#endif
