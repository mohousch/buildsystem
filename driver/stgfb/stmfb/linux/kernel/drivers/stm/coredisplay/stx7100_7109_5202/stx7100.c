/***********************************************************************
 *
 * File: linux/kernel/drivers/stm/coredisplay/stx7100_7109_5202/stx7100.c
 * Copyright (c) 2007,2010 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/stm/gpio.h>

#include <stmdisplay.h>
#include <linux/stm/stmcoredisplay.h>

#include "soc/stb7100/stb7100reg.h"


static const unsigned long whitelist[] = {
    STb7100_REGISTER_BASE + STb7100_HD_DISPLAY_BASE,
    STb7100_REGISTER_BASE + STb7100_SD_DISPLAY_BASE,
    STb7100_REGISTER_BASE + 0x01004000, /* LMU */
    STb7100_REGISTER_BASE + STb7100_VOS_BASE,0x19005000, /* VOS */
    STb7100_REGISTER_BASE + STb7100_DENC_BASE,
};

#ifndef __TDT__

static struct stmcore_display_pipeline_data platform_data[] = {
  {
    .owner                    = THIS_MODULE,
    .name                     = "STx7100-main",
    .device                   = 0,
    .vtg_irq                  = 154,
    .blitter_irq              = 156,
    .blitter_irq_kernel       = -1, /* handled by main blitter */
    .hdmi_irq                 = 158,
#if defined(CONFIG_SH_ST_MB442)
    .hdmi_i2c_adapter_id      = 1,
#elif defined(CONFIG_SH_ST_MB411)
    .hdmi_i2c_adapter_id      = 1,
#elif defined(CONFIG_SH_ST_HMS1)
    .hdmi_i2c_adapter_id      = 2,
#else
    .hdmi_i2c_adapter_id      = 0, /* Add your board definition here */
#endif
    .main_output_id           = 0,
    .hdmi_output_id           = 3,
    .dvo_output_id            = 2,

    .blitter_id               = 0,
    .blitter_id_kernel        = -1, /* just use main blitter */
    .blitter_type             = STMCORE_BLITTER_GAMMA,

    .preferred_graphics_plane = OUTPUT_GDP1,
    .preferred_video_plane    = OUTPUT_VID1,
    .planes                   = {
       { OUTPUT_GDP1, STMCORE_PLANE_GFX | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS},
       { OUTPUT_GDP2, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS},
       { OUTPUT_VID1, STMCORE_PLANE_VIDEO | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_VIDEO},
       { OUTPUT_CUR , STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS}
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,
  },
  {
    .owner                    = THIS_MODULE,
    .name                     = "STx7100-aux",
    .device                   = 0,
    .vtg_irq                  = 155,
    .blitter_irq              = -1, /* Handled by the main pipeline instance */
    .blitter_irq_kernel       = -1, /* Handled by the main pipeline instance */
    .hdmi_irq                 = -1,
    .hdmi_i2c_adapter_id      = -1,
    .main_output_id           = 1,
    .hdmi_output_id           = -1,
    .dvo_output_id            = -1,

    .blitter_id               = 0,
    .blitter_id_kernel        = -1, /* just use main blitter */
    .blitter_type             = STMCORE_BLITTER_GAMMA,

    .preferred_graphics_plane = OUTPUT_GDP2,
    .preferred_video_plane    = OUTPUT_VID2,
    .planes                   = {
       { OUTPUT_GDP2, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS},
       { OUTPUT_VID2, STMCORE_PLANE_VIDEO | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS} /* Note, MEM_SYS _not_ MEM_VIDEO */
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,
  }
};
#else
static struct stmcore_display_pipeline_data platform_data[] = {
  {
    .owner                    = THIS_MODULE,
    .name                     = "STx7100-main",
    .device                   = 0,
    .vtg_irq                  = 154,
    .blitter_irq              = 156,
    .hdmi_irq                 = 158, 
//#if defined(CONFIG_SH_STB7100_REF) || defined(CONFIG_SH_ST_MB442)
//    .hdmi_i2c_adapter_id      = 1,
//#elif defined(CONFIG_SH_STB7100_MBOARD) || defined(CONFIG_SH_ST_MB411)
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23)
#if defined(ADB_BOX)
    .hdmi_i2c_adapter_id = 1,
#else
    .hdmi_i2c_adapter_id = 2,
#endif 
//#else
//    .hdmi_i2c_adapter_id      = 1,
//#endif
//#elif defined(CONFIG_SH_HMS1) || defined(CONFIG_SH_ST_HMS1)
//    .hdmi_i2c_adapter_id      = 2,
//#else
//    .hdmi_i2c_adapter_id      = 0, /* Add your board definition here */
//#endif
    .main_output_id           = 0,
    .aux_output_id            = -1,
    .hdmi_output_id           = 3,
    .dvo_output_id            = 2,

    .blitter_id               = 0,
    .blitter_type             = STMCORE_BLITTER_GAMMA,

    .preferred_graphics_plane = OUTPUT_GDP1,
    .preferred_video_plane    = OUTPUT_VID1,
    .planes                   = {
    	{ OUTPUT_GDP1, STMCORE_PLANE_GFX | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS},
    	{ OUTPUT_GDP2, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS},
    	{ OUTPUT_VID1, STMCORE_PLANE_VIDEO | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_VIDEO},
    	{ OUTPUT_CUR , STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS}
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0
  },
  {
    .owner                    = THIS_MODULE,
    .name                     = "STx7100-aux",
    .device                   = 0,
    .vtg_irq                  = 155,
    .blitter_irq              = -1, // Handled by the main pipeline instance
    .hdmi_irq                 = -1, 
    .hdmi_i2c_adapter_id      = -1,
    .main_output_id           = 1,
    .aux_output_id            = -1,
    .hdmi_output_id           = -1,
    .dvo_output_id            = -1,

    .blitter_id               = 0,
    .blitter_type             = STMCORE_BLITTER_GAMMA,

    .preferred_graphics_plane = OUTPUT_GDP2,
    .preferred_video_plane    = OUTPUT_VID2,
    .planes                   = {
    	{ OUTPUT_GDP2, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS},
    	{ OUTPUT_VID2, STMCORE_PLANE_VIDEO | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS} /* Note, MEM_SYS _not_ MEM_VIDEO */
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0
  }
};
#endif

#if defined(CONFIG_SH_ST_MB411)
static const int refClockError = 359; /* ppm */
#else
static const int refClockError;
#endif

/*
 * For ST boards, we assume the video DAC load resistor network is the same,
 * customers will need to tune this for their board designs.
 */
static const int maxDAC123Voltage = 1409;   // Rref=7.68Kohm Rload=140ohm, Vmax=(77.312/7680)*140
static const int maxDAC456Voltage = 1409;

static const int DAC123SaturationPoint; // Use Default (1023 for a 10bit DAC)
static const int DAC456SaturationPoint;

/* For HDMI hotplug to work, the kernel's board support must have set the pin
 * to STPIO_BIDIR_Z1 sometime before we run. */
#if defined(ADB_BOX) 
#define GPIO_PIN_HOTPLUG stm_gpio(3,7)
#else
#define GPIO_PIN_HOTPLUG stm_gpio(2,2)
#endif
static bool claimed_gpio_hotplug;

#if defined(CONFIG_SH_ST_MB411) || \
    defined(CONFIG_SH_ST_MB442) || \
    defined(CONFIG_SH_ST_HMS1)
#define SYSCONF_DEVICEID 0x19001000
#else
#define SYSCONF_DEVICEID 0
#endif

enum _clocks { CLOCK_PCM0, CLOCK_PCM1, CLOCK_SPDIF };
struct coredisplay_clk {
  struct clk *clk;
  const char *name;
};
static struct coredisplay_clk coredisplay_clks[] = {
  [CLOCK_PCM0]  = { .name = "CLKC_FS0_CH1" },
  [CLOCK_PCM1]  = { .name = "CLKC_FS0_CH2" },
  [CLOCK_SPDIF] = { .name = "CLKC_FS0_CH3" }
};


int __init stmcore_probe_device(struct stmcore_display_pipeline_data **pd,
                                int *nr_platform_devices)
{
  if(SYSCONF_DEVICEID != 0)
  {
    int i;

    unsigned long *devid = ioremap(SYSCONF_DEVICEID, sizeof(unsigned long));
    unsigned long chipid = readl(devid);

    int is7100 = (((chipid>>12)&0x3ff) != 0x02c);
    iounmap(devid);

    if(is7100)
    {
      *pd = platform_data;
      *nr_platform_devices = ARRAY_SIZE(platform_data);

      if(gpio_request(GPIO_PIN_HOTPLUG, "HDMI Hotplug") >= 0)
        claimed_gpio_hotplug = true;
      /* We expect the gpio pin function to have been set up correctly by the
         kernel already, see comment above. */
      if(!claimed_gpio_hotplug)
      {
        printk(KERN_WARNING "stmcore-display: Hotplug PIO already in use (by SSC driver?)\n");
        printk(KERN_WARNING "stmcore-display: HDMI will not work in this board configuration\n");
      }

      for(i = 0; i < N_ELEMENTS (coredisplay_clks); ++i)
      {
        coredisplay_clks[i].clk = clk_get_sys ("hdmi", coredisplay_clks[i].name);
        if(coredisplay_clks[i].clk)
          clk_enable (coredisplay_clks[i].clk);
      }

      printk(KERN_INFO "stmcore-display: STx7100 display: probed\n");
      return 0;
    }
  }

  printk(KERN_WARNING "stmcore-display: STx7100: platform unknown\n");

  return -ENODEV;
}


/*
 * The following FIR filter setups for the DENC luma filter have been derived
 * for the MB411; FIR2C is the default already used by the core driver. The
 * others give a variety of different frequency responses.
 *
 * We do not have specific filters for MB442 or HMS1; however, they both use the
 * same video output stage configuration as the MB411, so it is expected that
 * they will give very similar frequency responses.

static stm_display_filter_setup_t luma_filter_FIR2C = {
  DENC_COEFF_LUMA,
  { .denc = { STM_FLT_DIV_1024,
              {0x03, 0x01, 0xfffffff7, 0xfffffffe, 0x1E, 0x05, 0xffffffa4, 0xfffffff9, 0x144, 0x206}
            }
  }
};

static stm_display_filter_setup_t luma_filter_FIR2D = {
  DENC_COEFF_LUMA,
  { .denc = { STM_FLT_DIV_1024,
              {0x0f, 0xffffffe0, 0xffffffde, 0x3C, 0x2E, 0xffffffc5, 0xffffff95, 0x03F, 0x150, 0x1C0}
            }
  }
};

static stm_display_filter_setup_t luma_filter_FIR2E = {
  DENC_COEFF_LUMA,
  { .denc = { STM_FLT_DIV_1024,
              {0xfffffffd, 0xfffffffc, 0xfffffff9, 0xffffffff, 0x23, 0x05, 0xffffffa0, 0xfffffff1, 0x147, 0x21E}
            }
  }
};

static stm_display_filter_setup_t luma_filter_FIR2F = {
  DENC_COEFF_LUMA,
  { .denc = { STM_FLT_DIV_1024,
              {0x03, 0x00, 0xfffffff7, 0xffffffff, 0x1f, 0x03, 0xffffffa6, 0xfffffff4, 0x141, 0x214}
            }
  }
};

*/

int __init stmcore_display_postinit(struct stmcore_display *p)
{
  /*
   * Setup internal configuration controls
   */
  if(maxDAC123Voltage != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC123_MAX_VOLTAGE, maxDAC123Voltage);

  if(maxDAC456Voltage != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC456_MAX_VOLTAGE, maxDAC456Voltage);

  if(DAC123SaturationPoint != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC123_SATURATION_POINT, DAC123SaturationPoint);

  if(DAC456SaturationPoint != 0)
    stm_display_output_set_control(p->main_output, STM_CTRL_DAC456_SATURATION_POINT, DAC456SaturationPoint);

#ifdef __TDT__
#ifdef USE_EXT_CLK 
  stm_display_output_setup_clock_reference(p->main_output, STM_CLOCK_REF_27MHZ, refClockError); 
#else 
  stm_display_output_setup_clock_reference(p->main_output, STM_CLOCK_REF_30MHZ, refClockError);
#endif
#else 
  stm_display_output_setup_clock_reference(p->main_output, STM_CLOCK_REF_30MHZ, refClockError);
#endif

/*
 * Uncomment this if you want to change the default DENC luma filter

  stm_display_output_set_filter_coefficients(p->main_output, &luma_filter_FIR2C);
 */

  if(claimed_gpio_hotplug)
    p->hotplug_poll_pio = GPIO_PIN_HOTPLUG;

  return 0;
}


void stmcore_cleanup_device(void)
{
  int i;

  for(i = 0; i < N_ELEMENTS (coredisplay_clks); ++i)
  {
    if(coredisplay_clks[i].clk)
    {
      clk_disable (coredisplay_clks[i].clk);
      clk_put (coredisplay_clks[i].clk);
    }
  }

  if(claimed_gpio_hotplug)
  {
    gpio_free(GPIO_PIN_HOTPLUG);
    claimed_gpio_hotplug = false;
  }
}
