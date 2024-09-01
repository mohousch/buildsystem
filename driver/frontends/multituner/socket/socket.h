/*
 * @brief socket.h
 *
 * @author konfetti
 *
 * @brief handling of mixed tuner's in one box.
 *
 * Copyright (C) 2011 duckbox project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef socket_123
#define socket_123

#include "dvb_frontend.h"

struct socket_s
{
	char *name;

	/*
	 * tuner enable pin
	 *  - pio port
	 *  - pio pin
	 *  - active low/high
	 */
	int tuner_enable[3];

	/* the following arrays define
	 *  - i2c-bus
	 *  - i2c address
	 *  - alternative i2c address (hacky: support for LNBH23)
	 *  - voltage off
	 *  - vsel
	 *  - hsel
	 *
	 * or (depending on the lnb supplier)
	 *  - pio port (enable pin)
	 *  - pio pin   (enable pin)
	 *  - active low/high
	 *  - pio port (v/h sel pin)
	 *  - pio pin  (v/h sel pin)
	 *  - vertical
	 */
	u32 lnb[6];

	/* tuners i2c bus */
	int i2c_bus;
};

struct tunersocket_s
{
	int numSockets;

	struct socket_s *socketList;
};

struct frontend_s
{
	char *name;

	int (*demod_detect)(struct socket_s *socket, struct frontend_s *frontend);
	int (*demod_attach)(struct dvb_adapter *adapter, struct socket_s *socket, struct frontend_s *frontend);
};

extern int socket_register_frontend(struct frontend_s *frontend);
extern int socket_register_adapter(struct dvb_adapter *_adapter);

#endif
// vim:ts-4
