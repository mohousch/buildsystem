#ifndef avl6222_platform_123
#define avl6222_platform_123

#define dprintk(level, x...) do \
{ \
	if ((paramDebug) && (paramDebug >= level) || level == 0) \
	{ \
		printk(TAGDEBUG x); \
	} \
} while (0)

struct avl_private_data_s
{
	u16 demod_freq;
	u16 fec_freq;
	u16 mpeg_freq;
	u16 i2c_speed_khz;
	u32 agc_polarization;
	u32 mpeg_mode;
	u16 mpeg_serial;
	u16 mpeg_clk_mode;
	u32 pll_config;  /* pll-arrayindex */
	u32 usedTuner;
	u32 usedLNB;

	u16 max_lpf;
	u32 lpf;
	u8  lock_mode;
	u8  iq_swap;
	u8  auto_iq_swap;

	u16 agc_tri;
	u16 mpeg_tri;
};
#endif
// vim:ts=4
