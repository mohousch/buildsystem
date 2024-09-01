#ifdef UFS912
/*
    CLKB adjustments for ufs912 to avoid problems on 1080i hd channels.

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

unsigned long    reg_clkb_config = 0;

#define ClkbConfigBaseAddress 0xfe000000

#define CKGB_LCK 		0x10
#define CKGB_FS1_MD3		0x80
#define CKGB_FS1_PE3		0x84
#define CKGB_FS1_SDIV3		0x8C
#define CKGB_FS1_PRG_EN3	0x88

#define CKGB_FS0_PE1		0x1C
#define CKGB_FS0_PRG_EN1	0x20

#define CKGB_FS1_PE1		0x64
#define CKGB_FS1_PRG_EN1	0x68


#define CKGB_DISPLAY_CFG	0xA4

#define CKGB_REF_CLK_SEL	0xB8

#define CKGB_CLK_SRC		0xA8

int FS1_MD3 = 0x00000019;
int FS1_PE3 = 0x00003334;
int FS1_SDIV3 = 0x00000000;

void ufs912_setup_clks(void)
{  
    printk("setting up clks for ufs912\n");
    
    reg_clkb_config = ioremap(ClkbConfigBaseAddress, 0x100);
    printk("clkb: mapping from 0x%.8x to 0x%.8x\n", ClkbConfigBaseAddress, reg_clkb_config);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);

    /* enable programming */
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN3);
	
    ctrl_outl(FS1_MD3, reg_clkb_config + CKGB_FS1_MD3);
    ctrl_outl(FS1_PE3, reg_clkb_config + CKGB_FS1_PE3);
    ctrl_outl(FS1_SDIV3, reg_clkb_config + CKGB_FS1_SDIV3);

    /* take settings into effect */	
    ctrl_outl(0x1, reg_clkb_config + CKGB_FS1_PRG_EN3);
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN3);

    /* lock clkb config */	
    ctrl_outl(0x0000, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);

    /* enable programming */
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS0_PRG_EN1);
	
    ctrl_outl(0x00001213, reg_clkb_config + CKGB_FS0_PE1);

    /* take settings into effect */	
    ctrl_outl(0x1, reg_clkb_config + CKGB_FS1_PRG_EN3);
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN3);

    /* lock clkb config */	
    ctrl_outl(0x0000, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);

    /* enable programming */
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN1);
	
    ctrl_outl(0x00001c6d, reg_clkb_config + CKGB_FS1_PE1);

    /* take settings into effect */	
    ctrl_outl(0x1, reg_clkb_config + CKGB_FS1_PRG_EN3);
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN3);

    /* lock clkb config */	
    ctrl_outl(0x0000, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);
 
    /* set the video display clocks */
    ctrl_outl(0x00003000, reg_clkb_config + CKGB_DISPLAY_CFG);

    /* lock clkb config */	
    ctrl_outl(0x0000, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);
 
    /* set the video display clocks */
    ctrl_outl(0x00000000, reg_clkb_config + CKGB_REF_CLK_SEL);

    /* lock clkb config */	
    ctrl_outl(0x0000, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);
 
    /* set the video display clocks */
    ctrl_outl(0x00000007, reg_clkb_config + CKGB_CLK_SRC);

    /* lock clkb config */	
    ctrl_outl(0x0000, reg_clkb_config + CKGB_LCK);
}
#endif
