#ifndef stv6419_123
#define stv6419_123

/*
 *   stv6419.h - audio/video switch driver
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

int stv6419_init(struct i2c_client *client);
int stv6419_command(struct i2c_client *client, unsigned int cmd, void *arg );
int stv6419_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);
int stv6419_set_volume( struct i2c_client *client, int vol );
int stv6419_get_volume(void);
int stv6419_get_status(struct i2c_client *client);

#endif

