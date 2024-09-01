/*
 *   rmu_core.h - rf modulator core driver 
 *
 *
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

#define VENDOR_UNKNOWN	     		0
#define VENDOR_STMICROELECTRONICS	2

#define RMU_COUNT 		        3
#define RMU_MAJOR 		        10
#define RMU_MINOR		        240
#define I2C_DRIVERID_RMU	        2


#ifdef __KERNEL__

#ifdef DEBUG
#define dprintk(fmt, args...) printk(fmt, ##args)
#else
#define dprintk(fmt, args...)
#endif

extern int debug;

int scart_command( unsigned int cmd, void *arg );

struct rmu_ioctl_data {
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};

#endif
