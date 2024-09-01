/***********************************************************************
 *
 * File: linux/kernel/drivers/stm/coredisplay/sti5206/sti5206.c
 * Copyright (c) 2010 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

/*
 * Note that the STi5206 is only supported on 2.6.23 and above kernels
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <asm/irq.h>

#include <stmdisplay.h>
#include <linux/stm/stmcoredisplay.h>

#include <soc/sti5206/sti5206reg.h>
#include <soc/sti5206/sti5206device.h>

static const unsigned long whitelist[] = {
    STi5206_REGISTER_BASE + STi5206_DENC_BASE,
    _ALIGN_DOWN(STi5206_REGISTER_BASE + STi5206_BLITTER_BASE, PAGE_SIZE),
};


static struct stmcore_display_pipeline_data platform_data[] = {
  {
    .owner                    = THIS_MODULE,
    .name                     = "STi5206-main",
    .device                   = 0,
    .vtg_irq                  = evt2irq(0x1540),
    .blitter_irq              = evt2irq(0x15C0),
    .blitter_irq_kernel       = evt2irq(0x15E0),

/* HDMI is connected via External HDMI TX
 * which is feed by SII9024 DVO */
    .hdmi_irq                 = -1,
    .hdmi_i2c_adapter_id      = -1,
    .hdmi_output_id           = -1,
    .main_output_id           = STi5206_OUTPUT_IDX_VDP0_MAIN,


    .dvo_output_id            = STi5206_OUTPUT_IDX_DVO0,

    .blitter_id               = STi5206_BLITTER_IDX_VDP0_MAIN,
    .blitter_id_kernel        = STi5206_BLITTER_IDX_KERNEL,
    .blitter_type             = STMCORE_BLITTER_BDISPII,

    .preferred_graphics_plane = OUTPUT_GDP2,
    .preferred_video_plane    = OUTPUT_VID2,
    .planes                   = {
    	{ OUTPUT_GDP1, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS },
    	{ OUTPUT_GDP2, STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS },
    	{ OUTPUT_GDP3, STMCORE_PLANE_GFX | STMCORE_PLANE_MEM_SYS },
    	{ OUTPUT_VID2, STMCORE_PLANE_VIDEO | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS },
    	{ OUTPUT_VID1, STMCORE_PLANE_VIDEO | STMCORE_PLANE_SHARED | STMCORE_PLANE_MEM_SYS },
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,

    /* this is later mmap()ed, so it must be page aligned! */
    .mmio                     = _ALIGN_DOWN (STi5206_REGISTER_BASE + STi5206_BLITTER_BASE, PAGE_SIZE),
    .mmio_len                 = PAGE_SIZE,
  },
  {
    .owner                    = THIS_MODULE,
    .name                     = "STi5206-aux",
    .device                   = 0,
    .vtg_irq                  = evt2irq(0x1560),
    .blitter_irq              = evt2irq(0x15A0),
    .blitter_irq_kernel       = -1, /* only one instance of kernel blitter, handled above */
    .hdmi_irq                 = -1,
    .hdmi_i2c_adapter_id      = -1,
    .main_output_id           = STi5206_OUTPUT_IDX_VDP1_MAIN,
    .hdmi_output_id           = -1,
    .dvo_output_id            = -1,

    .blitter_id               = STi5206_BLITTER_IDX_VDP1_MAIN,
    .blitter_id_kernel        = STi5206_BLITTER_IDX_KERNEL,
    .blitter_type             = STMCORE_BLITTER_BDISPII,

    .preferred_graphics_plane = OUTPUT_GDP1,
    .preferred_video_plane    = OUTPUT_VID1,
    .planes                   = {
    	{ OUTPUT_GDP1, STMCORE_PLANE_GFX | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS },
    	{ OUTPUT_VID1, STMCORE_PLANE_VIDEO | STMCORE_PLANE_SHARED | STMCORE_PLANE_PREFERRED | STMCORE_PLANE_MEM_SYS }
    },
    .whitelist                = whitelist,
    .whitelist_size           = ARRAY_SIZE(whitelist),
    .io_offset                = 0,

    /* this is later mmap()ed, so it must be page aligned! */
    .mmio                     = _ALIGN_DOWN (STi5206_REGISTER_BASE + STi5206_BLITTER_BASE, PAGE_SIZE),
    .mmio_len                 = PAGE_SIZE,
  }
};


/*
 * Default DAC voltage range based on datasheet values; however different
 * board designs will effect this.
 */
#if defined(CONFIG_SH_ST_MB796)
/* TODO: Measure this */
static const int maxDAC123Voltage = 1375; // Rref=7.87Kohm Rload=140ohm, Vmax=(77.31/7870)*140
static const int maxDAC456Voltage = 1375;

#elif defined(CONFIG_SH_ST_HDK5289)
/* TODO: Measure this */
static const int maxDAC123Voltage = 1375; // Rref=7.87Kohm Rload=140ohm, Vmax=(77.31/7870)*140
static const int maxDAC456Voltage = 1375;

#else
#error Unknown 5206/5289 board, add your board values here
#endif

static const int DAC123SaturationPoint; // Use Default (1023 for a 10bit DAC)
static const int DAC456SaturationPoint;

int __init stmcore_probe_device(struct stmcore_display_pipeline_data **pd, int *nr_platform_devices)
{
  if(boot_cpu_data.type == CPU_STX5206)
  {
    *pd = platform_data;
    *nr_platform_devices = N_ELEMENTS (platform_data);

    printk(KERN_WARNING "stmcore-display: STi5206 display: probed\n");
    return 0;
  }

  printk(KERN_WARNING "stmcore-display: STi5206 display: platform unknown\n");

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
}
