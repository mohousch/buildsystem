/*
	STV6110(A) Silicon tuner driver

	Copyright (C) Manu Abraham <abraham.manu@gmail.com>

	Copyright (C) ST Microelectronics

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __STV6110x_H
#define __STV6110x_H

struct stv6110x_config {
	u8	addr;
	u32	refclk;
};

enum tuner_mode {
	TUNER_SLEEP = 1,
	TUNER_WAKE,
};

enum tuner_status {
	TUNER_PHASELOCKED = 1,
};

extern struct tuner_devctl *stv6110x_attach(struct dvb_frontend *fe,
					       const struct stv6110x_config *config,
					       struct i2c_adapter *i2c);

#endif /* __STV6110x_H */
