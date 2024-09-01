#ifndef _PVR_CONFIG_H_
#define _PVR_CONFIG_H_

struct plat_tuner_config
{
	int adapter; /* DVB adapter number */
	int i2c_bus; /* i2c adapter number */
	int i2c_addr; /* i2c address */
	/* the following arrays define
		- PIO port number
		- PIO pin number
		- active state of the pin (0 - active-low, 1 - active-high  */
	int tuner_enable[3];
	int lnb_enable[3];
	int lnb_vsel[3];
};

struct plat_tuner_data
{
	int num_entries;
	struct plat_tuner_config *tuner_cfg;
};

#endif
