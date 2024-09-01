#ifndef _UFS9XX_CIC_H_
#define _UFS9XX_CIC_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

#define cNumberSlots   2

struct ufs9xx_cic_core {
	struct dvb_adapter		*dvb_adap;
	struct dvb_ca_en50221           ca; /* cimax */
};

#define SLOTSTATUS_NONE		1
#define SLOTSTATUS_PRESENT	2
#define SLOTSTATUS_RESET	4
#define SLOTSTATUS_READY	8

struct ufs9xx_cic_state {
	struct dvb_frontend_ops             ops;
	struct ufs9xx_cic_core		        *core;

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
        struct i2c_adapter      	    *i2c;
        int                             i2c_addr;
#endif

	struct stpio_pin                    *ci_reset;
	struct stpio_pin                    *module_ready_pin[cNumberSlots];

	struct stpio_pin                    *slot_enable[cNumberSlots];
	struct stpio_pin                    *slot_reset[cNumberSlots];
	struct stpio_pin                    *slot_status[cNumberSlots];

	int                                 module_status[cNumberSlots];

#if defined(UFS922) || defined(UFS913) || defined(UFC960)
	int					                module_source[cNumberSlots];
#endif

	volatile unsigned long              slot_attribute_read[cNumberSlots];
	volatile unsigned long              slot_attribute_write[cNumberSlots];
	volatile unsigned long              slot_control_read[cNumberSlots];
	volatile unsigned long              slot_control_write[cNumberSlots];

	unsigned long                       detection_timeout[cNumberSlots];
};

int init_ufs9xx_cic(struct dvb_adapter *dvb_adap);

int setCiSource(int slot, int source);
void getCiSource(int slot, int* source);

#endif
