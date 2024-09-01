/*
 *   ak4705.h - AK4705 audio/video switch driver - Kathrein UFS-910
 *
 *   written by captaintrip - 19.Nov 2007
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
int ak4705_init(struct i2c_client *client);
int ak4705_command(struct i2c_client *client, unsigned int cmd, void *arg);
int ak4705_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);
#endif

