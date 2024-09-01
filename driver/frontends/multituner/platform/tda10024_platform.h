#ifndef tda_platform_123
#define tda_platform_123

/*
 * @brief tda10024_platform.h
 *
 * @author konfetti
 *
 * 	Copyright (C) 2011 duckbox
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
  
#define dprintk(level, x...) do \
{ \
	if ((paramDebug) && (paramDebug >= level) || level == 0) \
	{ \
		printk(TAGDEBUG x); \
	} \
} while (0)

struct tda10024_private_data_s
{
	u32 ts_out;	
	u32 si;
	u32 power;
	u32 agc_th;
};

struct mxl201_private_data_s  
{ 
	u32 if_freq; 
	u32 xtal; 
	u8  xtal_cap; 
	u8  clk_out; 
	u8  clk_amp; 
	u8  si; 
	u8  mode; 
	u8  power;
	u8  lt; 
};

struct tda10024_s
{
	struct mxl201_private_data_s*   mxl201;
	struct tda10024_private_data_s* tda10024;
};
#endif
// vim:ts=4
