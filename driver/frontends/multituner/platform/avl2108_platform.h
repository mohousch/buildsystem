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
	u8  lock_mode;
	u8  iq_swap;
	u8  auto_iq_swap;

	u16 agc_ref;

	s8  mpeg_data_clk; /* enable/disable mpeg persistent clock mode
                            * -1 = do not touch register
                            * 0  = disable
                            * 1  = enable
                            */
};
#endif
// vim:ts=4