/*
 *   stv6412.h - audio/video switch driver (dbox-II-project)
 *
 *   Homepage: http://www.tuxbox.org
 *
 *   Copyright (C) 2000-2002 Gillem gillem@berlios.de
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef __KERNEL__
int stv6412_init(struct i2c_client *client);
int stv6412_command(struct i2c_client *client, unsigned int cmd, void *arg );
int stv6412_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);
int stv6412_set_volume( struct i2c_client *client, int vol );
int stv6412_get_volume(void);
int stv6412_get_status(struct i2c_client *client);
#endif

