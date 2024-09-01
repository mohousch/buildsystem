#ifndef tda10023_platform_123
#define tda10023_platform_123

#include "tuner.h"
#include "tda1002x.h"

struct tda10023_private_data_s
{
	u32 usedLNB;

	/* clock settings */
	u32 xtal; /* defaults: 28920000 */
	u8  pll_m; /* defaults: 8 */
	u8  pll_p; /* defaults: 4 */
	u8  pll_n; /* defaults: 1 */

	/* MPEG2 TS output mode */
	u8  output_mode;

	/* input freq offset + baseband conversion type */
	u16 deltaf;
};
#endif
// vim:ts=4