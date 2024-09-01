/*
  Driver for ST STV0288 demodulator

  Copyright (C) 2006 Georg Acher, BayCom GmbH, acher (at) baycom (dot) de
	                 for Reel Multimedia
  Copyright (C) 2008 TurboSight.com, <bob@turbosight.com>
  Copyright (C) 2008 Igor M. Liplianin <liplianin@me.by>
	                 Removed stb6000 specific tuner code and revised some
	                 procedures.

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

#ifndef STV0288_H
#define STV0288_H

#include "dvb_frontend.h"
#include "equipment.h"

struct stv0288_config
{
	u8               i2c_bus;
	/* the demodulator's i2c address */
	u8               demod_address;

	u8*              inittab;

	/* minimum delay before retuning */
	int              min_delay_ms;

	int (*set_ts_params)(struct dvb_frontend *fe, int is_punctured);

	u8               tuner_address;
	u32              tuner_refclk;
  
	struct stpio_pin *tuner_enable_pin;
	u32              tuner_active_lh;
	u32              lnb[6];

	u32              usedLNB;
};

struct stv0288_state
{
	struct i2c_adapter    *i2c;
	struct stv0288_config *config;
	struct dvb_frontend   frontend;

	u8                    initialised:1;
	u32                   tuner_frequency;
	u32                   symbol_rate;
	fe_code_rate_t        fec_inner;
	int                   errmode;

	struct equipment_s    equipment;
	void                  *lnb_priv;
};

typedef enum
{
	NOAGC1 = 0,
	AGC1OK,
	NOTIMING,
	ANALOGCARRIER,
	TIMINGOK,
	NOAGC2,
	AGC2OK,
	NOCARRI,
	CARRIEROK,
	NODATA,
	FALSELOCK,
	DATAOK,
	OUTOFRANGE,
	RANGEOK
} FE_288_SIGNALTYPE_t;

typedef enum
{
	FE_1_2  = 1,
	FE_2_3  = 1 << 1,
	FE_3_4  = 1 << 2,
	FE_5_6  = 1 << 3,
	FE_6_7  = 1 << 4,
	FE_7_8  = 1 << 5
} FE_288_Rate_t;

static inline int stv0288_writereg(struct dvb_frontend *fe, u8 reg, u8 val)
{
	int r = 0;
	u8 buf[] = { reg, val };

	if (fe->ops.write)
	{
		r = fe->ops.write(fe, buf, 2);
	}
	return r;
}

extern struct dvb_frontend *stv0288_attach(struct stv0288_config *config, struct i2c_adapter *i2c);

#endif /* STV0288_H */
// vim:ts=4
