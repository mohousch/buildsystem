/*
  TDA10021/TDA10023  - Single Chip Cable Channel Receiver driver module
                       used on the the Siemens DVB-C cards

  Copyright (C) 1999 Convergence Integrated Media GmbH <ralph@convergence.de>
  Copyright (C) 2004 Markus Schulz <msc@antzsystem.de>
                     Support for TDA10021

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef TDA1002x_H
#define TDA1002x_H

#include <linux/i2c.h>
#include <linux/dvb/frontend.h>

enum tda10023_output_mode
{
	TDA10023_OUTPUT_MODE_PARALLEL_A = 0xe0,
	TDA10023_OUTPUT_MODE_PARALLEL_B = 0xa1,
	TDA10023_OUTPUT_MODE_PARALLEL_C = 0xa0,
	TDA10023_OUTPUT_MODE_SERIAL, /* TODO: not implemented */
};

struct tda10023_config
{
	/* the demodulator's i2c address */
	u8  demod_address;
	u8  invert;

	/* clock settings */
	u32 xtal; /* default: 28920000 */
	u8  pll_m; /* default: 8 */
	u8  pll_p; /* default: 4 */
	u8  pll_n; /* default: 1 */

	/* MPEG2 TS output mode */
	u8  output_mode;

	/* input freq offset + baseband conversion type */
	u16 deltaf;

	struct stpio_pin *tuner_enable_pin;
	u32              tuner_active_lh;
	u8               tuner_address;
};

extern struct dvb_frontend *tda10023_attach(const struct tda10023_config *config, struct i2c_adapter *i2c, u8 pwm);
#endif // TDA1002x_H
// vim:ts=4
