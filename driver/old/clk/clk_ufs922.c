#ifdef UFS922
/*
    CLKB adjustments for ufs922
    
Not sure if this really make things better on ufs922 but
I think we should test this. Insmod this module after all
other drivers and take look on H264 problems, avdsync problems
etc ...

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

unsigned long    reg_clka_config = 0;
unsigned long    reg_clkb_config = 0;

#define ClkaConfigBaseAddress 0x19213000

#define CKGA_LCK 		0x00
#define CKGA_CLKOUT_SEL         0x0038

#define ClkbConfigBaseAddress 0x19000000

#define CKGB_LCK 		0x10

#define CKGB_FS0_MD1		0x18
#define CKGB_FS0_PE1		0x1C
#define CKGB_FS0_PRG_EN1	0x20
#define CKGB_FS0_SDIV1		0x24

#define CKGB_FS1_MD3		0x80
#define CKGB_FS1_PE3		0x84
#define CKGB_FS1_SDIV3		0x8C
#define CKGB_FS1_PRG_EN3	0x88

#define CKGB_FS1_MD1		0x60
#define CKGB_FS1_PE1		0x64
#define CKGB_FS1_PRG_EN1	0x68
#define CKGB_FS1_SDIV1		0x6C

#define CKGB_DISPLAY_CFG	0xA4

#define CKGB_CLKOUT_SEL		0xB4
#define CKGB_REF_CLK_SEL	0xB8

#define CKGB_CLK_SRC		0xA8

void ufs922_setup_clks(void)
{  
    printk("setting up clks for ufs912\n");

    reg_clka_config = ioremap(ClkaConfigBaseAddress, 0x100);
    printk("clka: mapping from 0x%.8x to 0x%.8x\n", ClkaConfigBaseAddress, reg_clka_config);

    reg_clkb_config = ioremap(ClkbConfigBaseAddress, 0x100);
    printk("clkb: mapping from 0x%.8x to 0x%.8x\n", ClkbConfigBaseAddress, reg_clkb_config);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clka_config + CKGA_LCK);

    ctrl_outl(0xb, reg_clka_config + CKGA_CLKOUT_SEL);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clka_config + CKGA_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);

    /* enable programming */
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS0_PRG_EN1);
	
    ctrl_outl(0x0017, reg_clkb_config + CKGB_FS0_MD1);
    ctrl_outl(0x5d10, reg_clkb_config + CKGB_FS0_PE1);
    ctrl_outl(0x0000, reg_clkb_config + CKGB_FS0_SDIV1);

    /* take settings into effect */	
    ctrl_outl(0x1, reg_clkb_config + CKGB_FS0_PRG_EN1);
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS0_PRG_EN1);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);

    /* enable programming */
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN1);
	
    ctrl_outl(0x0018, reg_clkb_config + CKGB_FS1_MD1);
    ctrl_outl(0x7ffc, reg_clkb_config + CKGB_FS1_PE1);
    ctrl_outl(0x0003, reg_clkb_config + CKGB_FS1_SDIV1);

    /* take settings into effect */	
    ctrl_outl(0x1, reg_clkb_config + CKGB_FS1_PRG_EN1);
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN1);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);

    /* enable programming */
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN3);
	
    ctrl_outl(0x0017, reg_clkb_config + CKGB_FS1_MD3);
    ctrl_outl(0x7ae1, reg_clkb_config + CKGB_FS1_PE3);
    ctrl_outl(0x0000, reg_clkb_config + CKGB_FS1_SDIV3);

    /* take settings into effect */	
    ctrl_outl(0x1, reg_clkb_config + CKGB_FS1_PRG_EN3);
    ctrl_outl(0x0, reg_clkb_config + CKGB_FS1_PRG_EN3);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);
 
    /* set the video display clocks */
    ctrl_outl(0x0000000, reg_clkb_config + CKGB_DISPLAY_CFG);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);
 
    /* set the video display clocks */
    ctrl_outl(0x00000001, reg_clkb_config + CKGB_REF_CLK_SEL);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);
 
    /* set the video display clocks */
    ctrl_outl(0x00000007, reg_clkb_config + CKGB_CLK_SRC);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clkb_config + CKGB_LCK);

/* *** */
    /* unlock clkb */	
    ctrl_outl(0xc0de, reg_clkb_config + CKGB_LCK);
 
    ctrl_outl(0x00000000, reg_clkb_config + CKGB_CLKOUT_SEL);

    /* lock clkb config */	
    ctrl_outl(0xc1a0, reg_clkb_config + CKGB_LCK);
}
#endif
