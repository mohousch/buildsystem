#ifndef _UFS9XX_CIC_H_
#define _UFS9XX_CIC_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

#define cNumberSlots   1

struct hs7110_cic_core {
        struct dvb_adapter		*dvb_adap;
        struct dvb_ca_en50221           ca; /* cimax */
};

struct hs7110_cic_state {
    struct dvb_frontend_ops     ops;
    struct hs7110_cic_core		*core;

	struct stpio_pin			*ci_enable;
	struct stpio_pin			*module_detect;
	struct stpio_pin			*slot_reset[cNumberSlots];

    int                         module_ready[cNumberSlots];
    int                         module_present[cNumberSlots];
    unsigned long               detection_timeout[cNumberSlots];

};

int init_hs7110_cic(struct dvb_adapter *dvb_adap);

int setCiSource(int slot, int source);
void getCiSource(int slot, int* source);

#endif
