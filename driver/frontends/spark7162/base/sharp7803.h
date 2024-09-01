/*
 IX2470 8PSK/QPSK tuner driver
 Copyright (C) Manu Abraham <abraham.manu@gmail.com>

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

#ifndef __SHARP7803_H
#define __SHARP7803_H

struct sharp7803_config
{
	u8 name[32];
	u8 addr;
};

extern struct dvb_frontend *sharp7803_attach(struct dvb_frontend *fe,
											 const struct sharp7803_config *config,
											 struct i2c_adapter *i2c);

#endif /* __SHARP7803_H */
