#ifndef zl10353_platform_123
#define zl10353_platform_123

#include "tuner.h"
#include "zl10353.h"

struct zl10353_private_data_s
{
	int adc_clock;	/* default: 450560 (45.056  MHz) */
	int if2;	/* default: 361667 (36.1667 MHz) */

	/* set if no pll is connected to the secondary i2c bus */
	int no_tuner;

	/* set if parallel ts output is required */
	int parallel_ts;

	/* set if i2c_gate_ctrl disable is required */
	u8 disable_i2c_gate_ctrl:1;

	/* clock control registers (0x51-0x54) */
	u8 clock_ctl_1;  /* default: 0x46 */
	u8 pll_0;        /* default: 0x15 */
};
#endif
// vim:ts=4