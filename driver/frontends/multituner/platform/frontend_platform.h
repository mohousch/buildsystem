/*
 * @brief frontend_platform.h
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

#ifndef _platform_123
#define _platform_123

extern short paramDebug;
#define dprintk(level, x...) do \
{ \
	if ((paramDebug) && (paramDebug >= level) || level == 0) \
	{ \
		printk(TAGDEBUG x); \
	} \
} while (0)

struct platform_frontend_config_s
{
	char *name;

	/*
	 *  - i2c address
	 */
	int  demod_i2c;
	int  tuner_i2c;

	/* specific stuff can be passed here */
	void *private;
};

#if defined UFS913
struct platform_frontend_s
{
	int numConfigs;
	struct platform_frontend_config_s *config;
};
#endif
#endif
// vim:ts=4
