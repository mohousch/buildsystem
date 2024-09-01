#ifndef cxd2820_platform_123
#define cxd2820_platform_123

/*
 * @brief cxd2820_platform.h
 *
 * @author
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

struct cxd2820_private_data_s
{
	u32 ts_out;	
	u32 si;
};

struct tda18272_private_data_s  
{ 
	int    lt;       //1 = lt ->really ??
	u8     stdby;    //3 = d3
	u8     iic_mode; //0 = iic_0
	u8     xtout;    //0 = off
};

struct cxd2820_s
{
	struct tda18272_private_data_s *tda18272;
	struct cxd2820_private_data_s * cxd2820;
};
#endif
// vim:ts=4