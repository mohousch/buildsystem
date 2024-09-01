/***********************************************************************
 *
 * File: linux/kernel/drivers/stm/coredisplay/sti7105_7106/sti7105.c
 * Copyright (c) 2008-2010 STMicroelectronics Limited.
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
#include <linux/stm/sysconf.h>
#include <linux/stm/stx7105.h>

#include <asm/irq.h>

#include <stmdisplay.h>
#include <linux/stm/stmcoredisplay.h>

#include <soc/sti7105/sti7105reg.h>
#include <soc/sti7105/sti7105device.h>
#include <STMCommon/stmhdmiregs.h>


static const unsigned long whitelist[] = {
    STi7111_REGISTER_BASE + STi7111_DENC_BASE,
    STi7111_REGISTER_BASE + STi7111_DENC_BASE+PAGE_SIZE,
    STi7111_REGISTER_BASE + STi7111_DENC_BASE+(PAGE_SIZE*2),
    STi7111_REGISTER_BASE + STi7111_HDMI_BASE,
    _ALIGN_DOWN(STi7111_REGISTER_BASE + STi7111_BLITTER_BASE, PAGE_SIZE),
};


/* BDisp IRQs on 7105: all aq sharing 0x1220, cq1 ???, cq2 ??? */
static struct stmcore_display_pipeline_data platform_data[] = {
  {
    .owner                    = THIS_MODULE,
    .name                     = "STi7105-main",
    .device                   = 0,
    .vtg_irq                  = evt2irq(0x1540),
    .blitter_irq              = evt2irq(0x1220),
    .blitter_irq_kernel       = evt2irq(0x1220),
    .hdmi_irq                 = evt2irq(0x15C0),
#if defined(CONFIG_SH_ST_MB680)
#if defined(SAGEMCOM88) || defined(PACE7241)
    .hdmi_i2c_adapter_id      = 1,
#else
    .hdmi_i2c_adapter_id      = 2,
#endif    
#elif defined(CONFIG_SH_ST_HDK7105)
    .hdmi_i2c_adapter_id      = 0,
#elif defined(CONFIG_SH_ST_IPTV7105)
    .hdmi_i2c_adapter_id      = 1,
#else
    .hdmi_i2c_adapter_id      = 0,
#endif
    .main_output_id           = STi7111_OUTPUT_IDX_VDP0_MAIN,
    .hdmi_output_id           = STi7111_OUTPUT_IDX_VDP0_HDMI,
    .dvo_output_id            = STi7111_OUTPUT_IDX_DVO0,

    .blitter_id               = STi7111_BLITTER_IDX_VDP0_MAIN,
    .blitter_id_kernel        = STi7111_BLITTER_IDX_KERNEL,
    .blitter_type             = STMCORE_BLITTER_BDISPII,

    .preferred_graphics_plane = OUTPUT_GDP1,
    .preferred_video_plane    = OUTPUT_VID1,
    .planes                   = {
       { OUTPUT_GDP1, STMCORE_PLANE_GFX | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS },
       { OUTPUT_GDP2, STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS },
       { OUTPUT_GDP3, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS },
       { OUTPUT_VID1, STMCORE_PLANE_VIDEO | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS },
       { OUTPUT_VID2, STMCORE_PLANE_VIDEO | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS },
       { OUTPUT_CUR , STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS }
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,

    /* this is later mmap()ed, so it must be page aligned! */
    .mmio                     = _ALIGN_DOWN (STi7111_REGISTER_BASE + STi7111_BLITTER_BASE, PAGE_SIZE),
    .mmio_len                 = PAGE_SIZE,
  },
  {
    .owner                    = THIS_MODULE,
    .name                     = "STi7105-aux",
    .device                   = 0,
    .vtg_irq                  = evt2irq(0x1560),
    .blitter_irq              = evt2irq(0x1220),
    .blitter_irq_kernel       = -1, /* only one instance of kernel blitter, handled above */
    .hdmi_irq                 = -1,
    .hdmi_i2c_adapter_id      = -1,
    .main_output_id           = STi7111_OUTPUT_IDX_VDP1_MAIN,
    .hdmi_output_id           = -1,
    .dvo_output_id            = -1,

    .blitter_id               = STi7111_BLITTER_IDX_VDP1_MAIN,
    .blitter_id_kernel        = STi7111_BLITTER_IDX_KERNEL,
    .blitter_type             = STMCORE_BLITTER_BDISPII,

    .preferred_graphics_plane = OUTPUT_GDP3,
    .preferred_video_plane    = OUTPUT_VID2,
    .planes                   = {
       { OUTPUT_GDP3, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS },
       { OUTPUT_VID2, STMCORE_PLANE_VIDEO | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS }
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,

    /* this is later mmap()ed, so it must be page aligned! */
    .mmio                     = _ALIGN_DOWN (STi7111_REGISTER_BASE + STi7111_BLITTER_BASE, PAGE_SIZE),
    .mmio_len                 = PAGE_SIZE,
  }
};


/*
 * The theoretical DAC voltage range is 1.398v:
 * 1024 * 0.0624 * (1.2214 / Rref) * Rload
 * 1023 * 0.0624 * (1.2214 / 7810) *   140
 *
 * But different board designs effect this.
 */
#if defined(CONFIG_SH_ST_MB680)
static const int maxDAC123Voltage = 1360;
static const int maxDAC456Voltage = 1360;
#elif defined(CONFIG_SH_ST_HDK7105)
static const int maxDAC123Voltage = 1320;
static const int maxDAC456Voltage = 1320;
#else
static const int maxDAC123Voltage = 1398;
static const int maxDAC456Voltage = 1398;
#endif

static const int DAC123SaturationPoint; // Use Default (1023 for a 10bit DAC)
static const int DAC456SaturationPoint;

#define GPIO_PIN_HOTPLUG stm_gpio(9,7)
static bool claimed_gpio_hotplug;
static struct sysconf_field *syscfg2_27;

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
  if(boot_cpu_data.type == CPU_STX7105)
  {
    int i;

    *pd = platform_data;
    *nr_platform_devices = ARRAY_SIZE(platform_data);

    /*
     * Setup HDMI hotplug:
     * 1) request pin
     * 2) set direction
     * 3) configure sysconf
     *
     * If either 2) or 3) fail, we release the GPIO pin.
     */
    if(gpio_request(GPIO_PIN_HOTPLUG, "HDMI Hotplug") >= 0)
    {
      if(gpio_direction_input(GPIO_PIN_HOTPLUG) >= 0)
      {
        /* enable hotplug pio in syscfg */
        syscfg2_27 = sysconf_claim(SYS_CFG, 2, 27, 27, "HDMI Hotplug PIO enable");
        if(syscfg2_27)
        {
          sysconf_write(syscfg2_27, 1);
          claimed_gpio_hotplug = true;
        }
      }

      if(!claimed_gpio_hotplug)
        gpio_free(GPIO_PIN_HOTPLUG);
    }

    if(syscfg2_27)
      printk(KERN_INFO "stmcore-display: using HDMI hotplug\n");
    else
      printk(KERN_WARNING "stmcore-display: HDMI hotplug not available\n");

    for(i = 0; i < N_ELEMENTS (coredisplay_clks); ++i)
    {
      coredisplay_clks[i].clk = clk_get_sys ("hdmi", coredisplay_clks[i].name);
      if(coredisplay_clks[i].clk)
        clk_enable (coredisplay_clks[i].clk);
    }

    printk(KERN_INFO "stmcore-display: STi7105 display: probed\n");
    return 0;
  }

  printk(KERN_WARNING "stmcore-display: STi7105 display: platform unknown\n");

  return -ENODEV;
}


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

  if(syscfg2_27)
  {
    sysconf_release(syscfg2_27);
    syscfg2_27 = NULL;
  }
}
