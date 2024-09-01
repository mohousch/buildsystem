#ifndef _CNBOX_CIC_H_
#define _CNBOX_CIC_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

#define cNumberSlots   1

struct cnbox_cic_core {
        struct dvb_adapter		*dvb_adap;
        struct dvb_ca_en50221           ca; /* cimax */
};

struct cnbox_cic_state {
    struct dvb_frontend_ops     ops;
    struct cnbox_cic_core		*core;

	struct stpio_pin			*ci_enable;
	struct stpio_pin			*module_detect;
	struct stpio_pin			*slot_reset[cNumberSlots];
#if defined(ATEMIO520)
	struct stpio_pin			*cam_enable;
	struct stpio_pin			*ts_enable;
#endif
#if defined(ATEMIO530)
	struct stpio_pin			*cam_enable;
	struct stpio_pin			*ts_enable;
#endif
    int                         module_ready[cNumberSlots];
    int                         module_present[cNumberSlots];
    unsigned long               detection_timeout[cNumberSlots];

};

int init_cnbox_cic(struct dvb_adapter *dvb_adap);

int setCiSource(int slot, int source);
void getCiSource(int slot, int* source);

#endif
