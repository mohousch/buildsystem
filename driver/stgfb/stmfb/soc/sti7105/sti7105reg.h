/***********************************************************************
 *
 * File: soc/sti7105/sti7105reg.h
 * Copyright (c) 2008 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STI7105REG_H
#define _STI7105REG_H

#include <soc/sti7111/sti7111reg.h>

#define STi7105_FLEXDVO1_BASE            (STi7111_TVOUT_FDMA_BASE + 0x0600)

#define CKGB_CFG_656_1(x)           ((x)<<2)

/* TVOUT AUX GP_OUT pad sync select ECO definitions */
#define TVOUT_GP_AUX_NOT_MAIN_SYNC    (1L<<0)

#define TVOUT_GP_HSYNC_MAIN_HREF      (0L)
#define TVOUT_GP_HSYNC_MAIN_INT_1     (1L)
#define TVOUT_GP_HSYNC_MAIN_PROG_1    (2L)
#define TVOUT_GP_HSYNC_MAIN_2         (3L)
#define TVOUT_GP_HSYNC_MAIN_3         (4L)
#define TVOUT_GP_HSYNC_MAIN_MASK      (7L)

#define TVOUT_GP_HSYNC_AUX_HREF       (0L)
#define TVOUT_GP_HSYNC_AUX_INT_1      (1L)
#define TVOUT_GP_HSYNC_AUX_2          (2L)
#define TVOUT_GP_HSYNC_AUX_3          (3L)
#define TVOUT_GP_HSYNC_AUX_MASK       (3L)

#define TVOUT_GP_VSYNC_REF            (0L)
#define TVOUT_GP_VSYNC_1              (1L)
#define TVOUT_GP_VSYNC_2              (2L)
#define TVOUT_GP_VSYNC_3              (3L)
#define TVOUT_GP_BNT_REF              (4L)
#define TVOUT_GP_BNT_1                (5L)
#define TVOUT_GP_BNT_2                (6L)
#define TVOUT_GP_BNT_3                (7L)
#define TVOUT_GP_VSYNC_MASK           (7L)

#define TVOUT_GP_HSYNC_MUX_MAIN_SHIFT (1)
#define TVOUT_GP_HSYNC_MUX_MAIN(x)    ((x)<<TVOUT_GP_HSYNC_MUX_MAIN_SHIFT)
#define TVOUT_GP_VSYNC_MUX_MAIN_SHIFT (4)
#define TVOUT_GP_VSYNC_MUX_MAIN(x)    ((x)<<TVOUT_GP_VSYNC_MUX_MAIN_SHIFT)
#define TVOUT_GP_HSYNC_MUX_AUX_SHIFT  (7)
#define TVOUT_GP_HSYNC_MUX_AUX(x)     ((x)<<TVOUT_GP_HSYNC_MUX_AUX_SHIFT)
#define TVOUT_GP_VSYNC_MUX_AUX_SHIFT  (9)
#define TVOUT_GP_VSYNC_MUX_AUX(x)     ((x)<<TVOUT_GP_VSYNC_MUX_AUX_SHIFT)



/* STi7105 Sysconfig reg2, HDMI power off & PIO9/7 hotplug control -----------*/
#define SYS_CFG2_BCH_HDMI_BCH_DIVSEL       (1L<<28)
#define SYS_CFG2_CEC_RX_EN                 (1L<<29)
#define SYS_CFG2_HDMI_AUDIO_8CH_N_2CH      (1L<<30)

/* STi7105 Sysconfig reg3, Analogue DAC & HDMI PLL control -------------------*/
#define SYS_CFG3_DVO_TO_DVP_EN             (1L<<10)

/* STi7105 Sysconfig reg6, Digital Video out config -------------  -----------*/
#define SYS_CFG6_BOT_N_TOP_INVERSION       (1L<<1)
#define SYS_CFG6_DVO0_H_NOT_V              (1L<<2)
#define SYS_CFG6_DVO0_REF_NOT_SYNC         (1L<<3)
#define SYS_CFG6_DVO0_AUX_NOT_MAIN         (1L<<4)
#define SYS_CFG6_DVO0_OLD                  (1L<<5)
#define SYS_CFG6_DVO1_H_NOT_V              (1L<<12)
#define SYS_CFG6_DVO1_REF_NOT_SYNC         (1L<<13)
#define SYS_CFG6_DVO1_AUX_NOT_MAIN         (1L<<14)
#define SYS_CFG6_DVO1_OLD                  (1L<<15)

#endif // _STI7105REG_H
