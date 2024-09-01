/*
    Copyright (C) 2010 konfetti <konfetti@XXX.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#if defined(UFS912)
extern void ufs912_setup_clks(void);
#elif defined(SPARK)
extern void spark_setup_clks(void);
#elif defined(UFS922) || defined(UFC960)
extern void ufs922_setup_clks(void);
#elif defined(HS7110)
extern void hs7110_setup_clks(void);
#elif defined(HS7420)
extern void hs7420_setup_clks(void);
#elif defined(HS7810A)
extern void hs7810a_setup_clks(void);
#elif defined(HS7119)
extern void hs7119_setup_clks(void);
#elif defined(HS7429)
extern void hs7429_setup_clks(void);
#elif defined(HS7819)
extern void hs7819_setup_clks(void);
#elif defined(ATEMIO520)
extern void atemio520_setup_clks(void);
#elif defined(ATEMIO530)
extern void atemio530_setup_clks(void);
#endif

int my_init_module(void)
{  
#if defined(UFS912)
   ufs912_setup_clks();
#elif defined(SPARK)
   spark_setup_clks();
#elif defined(HS7110)
   hs7110_setup_clks();
#elif defined(HS7119)
   hs7119_setup_clks();
#elif defined(HS7420)
   hs7420_setup_clks();
#elif defined(HS7429)
   hs7429_setup_clks();
#elif defined(HS7810A)
   hs7810a_setup_clks();
#elif defined(HS7819)
   hs7819_setup_clks();
#elif defined(ATEMIO520)
   atemio520_setup_clks();
#elif defined(ATEMIO530)
   atemio530_setup_clks();
#elif defined(UFS922) || defined(UFC960)
   ufs922_setup_clks();
#else
#warning unsupported receiver
#endif

    return 0;
}

void my_cleanup_module(void)
{
}

module_init(my_init_module);
module_exit(my_cleanup_module);

MODULE_DESCRIPTION      ("Setting up clk's");
MODULE_AUTHOR           ("konfetti");
MODULE_LICENSE          ("GPL");
