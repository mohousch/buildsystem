/*
 * @brief frontend_platform.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/version.h>

#include "frontend_platform.h"

short paramDebug = 0;  // debug print level is zero as default (0=nothing, 1= errors, 10=some detail, 20=more detail, 50=open/close functions, 100=all)
#if defined TAGDEBUG
#undef TAGDEBUG
#endif
#define TAGDEBUG "[frontend_platform] "

#if defined(ATEVIO7500)

#include "at7500_platform.h"

#elif defined(HS8200)
#include "hs8200_platform.h"

#elif defined(HS9510)
#include "hs9510_platform.h"

#elif defined(UFS922)
#include "ufs922_platform.h"

#elif defined (CUBEREVO_MINI2) \
   || defined (CUBEREVO_MINI) \
   || defined (CUBEREVO_250HD) \
   || defined (CUBEREVO_MINI_FTA) \
   || defined (CUBEREVO_3000HD)
#include "cuberevo_mini_platform.h"

#elif defined (CUBEREVO) \
   || defined (CUBEREVO_9500HD) \
   || defined (CUBEREVO_2000HD)
#include "cuberevo_platform.h"

#elif defined(UFS913)
#include "ufs913_platform.h"

#else

#error unsupported arch

#endif

int __init frontend_platform_init(void)
{
	int ret;

	ret = platform_add_devices(platform, sizeof(platform) / sizeof(struct platform_device *));
	if (ret != 0)
	{
		printk("failed to register platform device\n");
	}
	return ret;
}

module_init(frontend_platform_init);

module_param(paramDebug, short, 0644);
MODULE_PARM_DESC(paramDebug, "Activates frontend debugging (default:0)");

MODULE_DESCRIPTION("Frontend platform module");

MODULE_AUTHOR("konfetti");
MODULE_LICENSE("GPL");
// vim:ts=4
