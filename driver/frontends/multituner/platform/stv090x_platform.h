#ifndef stv090x_platform_123
#define stv090x_platform_123

#include "tuner.h"

enum stv090x_alternative
{
	cNONE,
	cFS9000,
};


enum stv090x_tuner
{
	STV090x_TUNER1 = 0x100,
	STV090x_TUNER2
};

enum stv090x_demodulator
{
	STV090x_DEMODULATOR_0 = 1,
	STV090x_DEMODULATOR_1
};

enum stv090x_device
{
	STV0903	=  0,
	STV0900,
	STX7111
};

enum stv090x_mode
{
	STV090x_DUAL = 0,
	STV090x_SINGLE
};

enum stv090x_tsmode
{
	STV090x_TSMODE_NOTSET = 0,
	STV090x_TSMODE_SERIAL_PUNCTURED	= 1,
	STV090x_TSMODE_SERIAL_CONTINUOUS,
	STV090x_TSMODE_PARALLEL_PUNCTURED,
	STV090x_TSMODE_DVBCI
};

enum stv090x_clkmode
{
	STV090x_CLK_INT = 0, /* Clk i/p = CLKI */
	STV090x_CLK_EXT = 2 /* Clk i/p = XTALI */
};

enum stv090x_i2crpt
{
	STV090x_RPTLEVEL_256	= 0,
	STV090x_RPTLEVEL_128	= 1,
	STV090x_RPTLEVEL_64		= 2,
	STV090x_RPTLEVEL_32		= 3,
	STV090x_RPTLEVEL_16		= 4,
	STV090x_RPTLEVEL_8		= 5,
	STV090x_RPTLEVEL_4		= 6,
	STV090x_RPTLEVEL_2		= 7,
};

enum stv090x_adc_range
{
	STV090x_ADC_2Vpp	= 0,
	STV090x_ADC_1Vpp	= 1
};

struct stv090x_private_data_s
{
	u32                      usedLNB;
	u32                      usedTuner;

	u32                      alternativePath; /* for e.g. fortis fs9000 */
	u32                      shouldSleep;     /* some tuner frozes when they sleep */

	enum stv090x_device      device;
	enum stv090x_demodulator demod;
	enum stv090x_mode        demod_mode;
	enum stv090x_clkmode	 clk_mode;

	u32                      xtal; /* default: 8000000 */

	u32                      ref_clk; /* default: 16000000 FIXME to tuner config */

	u8                       ts1_mode;
	u8                       ts2_mode;
	u32                      ts1_clk;
	u32                      ts2_clk;

	enum stv090x_i2crpt      repeater_level;

	u8                       tuner_bbgain; /* default: 10db */
	enum stv090x_adc_range	 adc1_range; /* default: 2Vpp */
	enum stv090x_adc_range	 adc2_range; /* default: 2Vpp */
	bool                     diseqc_envelope_mode;

	/* FIXME: later move this to a separated tuner config
	 * if we have separated demod's and tuner
	 */
	u32                      tuner_refclk;
};
#endif
// vim:ts=4
