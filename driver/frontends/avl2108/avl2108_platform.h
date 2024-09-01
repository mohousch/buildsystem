#ifndef avl2108_platform_123
#define avl2108_platform_123

struct avl_private_data_s
{
	u16 ref_freq;
	u16 demod_freq;
	u16 fec_freq;
	u16 mpeg_freq;
	u16 i2c_speed_khz;
	u32 agc_polarization;
	u32 mpeg_mode;
	u16 mpeg_serial;
	u16 mpeg_clk_mode;
	u32 pll_config; /* pll-arrayindex */
	u32 usedTuner;
	u32 usedLNB;

	u16 max_lpf;
	u32 lpf;
	u8 lock_mode;
	u8 iq_swap;
	u8 auto_iq_swap;

	u16 agc_ref;
};

struct platform_frontend_config_s
{
		char *name;

		/*
		 * tuner enable pin
		 * - pio port
		 * - pio pin
		 * - active low/high
		 */
		int tuner_enable[3];

		/* the following arrays define
		 * - i2c-bus
		 * - i2c address
		 * - alternative i2c address (hacky: support for LNBH23)
		 * - voltage off
		 * - vsel
		 * - hsel
		 *
		 * or (depending on the lnb supplier)
		 * - pio port (enable pin)
		 * - pio pin (enable pin)
		 * - active low/high
		 * - pio port (v/h sel pin)
		 * - pio pin (v/h sel pin)
		 * - vertical
		 */
		u32 lnb[6];

		/* tuners i2c bus */
		int i2c_bus;

		/*
		 * - i2c address
		 */
		int demod_i2c;
		int tuner_i2c;

		/* specific stuff can be passed here */
		void *private;
};

struct platform_frontend_s
{
	int numFrontends;

	struct platform_frontend_config_s *frontendList;
};

#endif
