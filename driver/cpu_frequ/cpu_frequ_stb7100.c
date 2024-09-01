/*
 * cpu_frequ.c
 * 
 * nit 08.11.2010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/clock.h>
#include <asm/timer.h>

#ifdef DEBUG
#define dprintk(fmt, args...) printk(fmt, ##args)
#else
#define dprintk(fmt, args...)
#endif

//#define STB7100
#define STX7105

#ifdef STB7100
#define CKGA_BASE_ADDR          0xb9213000
#define CKGA_LCK                (CKGA_BASE_ADDR + 0x00) 
#define CKGA_MD_STA             (CKGA_BASE_ADDR + 0x04) 
#define CKGA_PLL0_CFG           (CKGA_BASE_ADDR + 0x08) 
#define CKGA_PLL0_LCK_STA       (CKGA_BASE_ADDR + 0x10) 
#define CKGA_PLL0_CLK1          (CKGA_BASE_ADDR + 0x14)
#define CKGA_PLL0_CLK2          (CKGA_BASE_ADDR + 0x18)
#define CKGA_PLL0_CLK3          (CKGA_BASE_ADDR + 0x1c)
#define CKGA_PLL0_CLK4          (CKGA_BASE_ADDR + 0x20)
#define CKGA_PLL1_CFG           (CKGA_BASE_ADDR + 0x24) 
#define CKGA_PLL1_LCK_STA       (CKGA_BASE_ADDR + 0x2c) 
#define CKGA_CLK_DIV            (CKGA_BASE_ADDR + 0x30) 
#define CKGA_CLK_EN             (CKGA_BASE_ADDR + 0x34) 
#define CKGA_CLKOUT_SEL         (CKGA_BASE_ADDR + 0x38) 
#define CKGA_PLL1_BYPASS        (CKGA_BASE_ADDR + 0x3c) 

#define TMU_TSTR_INIT           0x3    /* enable both TMU0 and TMU1 */
#endif

#ifdef STX7105 
#define CKGA_BASE_ADDR          0xFE213000

/*
#define CKGA_LCK                (CKGA_BASE_ADDR + 0x00) 
#define CKGA_MD_STA             (CKGA_BASE_ADDR + 0x04) 
#define CKGA_PLL0_LCK_STA       (CKGA_BASE_ADDR + 0x10) 
#define CKGA_PLL0_CLK1          (CKGA_BASE_ADDR + 0x14)
#define CKGA_PLL0_CLK2          (CKGA_BASE_ADDR + 0x18)
#define CKGA_PLL0_CLK3          (CKGA_BASE_ADDR + 0x1c)
#define CKGA_PLL0_CLK4          (CKGA_BASE_ADDR + 0x20)
#define CKGA_PLL1_LCK_STA       (CKGA_BASE_ADDR + 0x2c) 
#define CKGA_CLK_DIV            (CKGA_BASE_ADDR + 0x30) 
#define CKGA_CLK_EN             (CKGA_BASE_ADDR + 0x34) 
#define CKGA_CLKOUT_SEL         (CKGA_BASE_ADDR + 0x38) 
#define CKGA_PLL1_BYPASS        (CKGA_BASE_ADDR + 0x3c) 
*/

#define CKGA_PLL0_CFG           (CKGA_BASE_ADDR + 0x00) 
#define CKGA_PLL1_CFG           (CKGA_BASE_ADDR + 0x04) 

#define TMU_TSTR_INIT           0x7   /* enable TMU0/1/2 */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  void set_actual_latch(unsigned long val_actual_latch);
#else
  #define TMU_TSTR TMU_012_TSTR
  #define CONFIG_SH_EXTERNAL_CLOCK 27000000
#endif

int modul_hz = HZ;
static struct proc_dir_entry 
*cpu_dir, 
*pll0_ndiv_mdiv, 
*pll1_ndiv_mdiv, 
#ifdef STB7100
*pll1_clk_div, 
*pll1_fdma_bypass, 
*sh4_ratio, 
*sh4_ic_ratio, 
*module_ratio, 
*slim_ratio, 
*sysaclkout, 
#endif
*m_hz; 

void update_bogomips(unsigned long sh4_hz)
{
  unsigned long bogomips;

  bogomips=(sh4_hz / 1000000) - 3;
  bogomips=bogomips * (500000/modul_hz);
  boot_cpu_data.loops_per_jiffy = bogomips;
  current_cpu_data.loops_per_jiffy = bogomips;
  loops_per_jiffy = bogomips;
}

int update_tmu(unsigned long module_rate)
{
  struct clk *clk;
  unsigned long actual_latch = LATCH;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("tmu0_clk");
#else
  clk=clk_get(NULL, "tmu0_clk");
#endif
  if(clk)
  {
    clk->parent->rate = module_rate;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
    // set tmu0 to actual frequ
    actual_latch = (clk_get_rate(clk) + modul_hz / 2) / modul_hz;
    while(ctrl_inl(TMU0_TCNT) != 0x0);  // wait for tmu0 interrupt
    ctrl_outb(ctrl_inb(TMU_TSTR) & ~(TMU_TSTR_INIT), TMU_TSTR); // stop all tmu
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    set_actual_latch(actual_latch);  // set actual latch in kernel
#endif
    ctrl_outl(actual_latch, TMU0_TCOR);
    ctrl_outl(actual_latch, TMU0_TCNT);
  }
  else
    printk("[CPU_FREQU] tmu0_clk not found\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("tmu1_clk");
#else
  clk=clk_get(NULL, "tmu1_clk");
#endif
  if(clk)
  {
    clk->parent->rate = module_rate;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
    ctrl_outl(~0, TMU1_TCOR);
    ctrl_outl(~0, TMU1_TCNT);
    ctrl_outb(ctrl_inb(TMU_TSTR) | TMU_TSTR_INIT, TMU_TSTR); // start all tmu
  }
  else
    printk("[CPU_FREQU] tmu1_clk not found\n");

  return 0;
}

void __delay(unsigned long loops)
{
  __asm__ __volatile__(
    "tst    %0, %0\n\t"
    "1:\t"
    "bf/s   1b\n\t"
    " dt    %0"
    : "=r" (loops)
    : "0" (loops)
    : "t");
}

unsigned long calc_bogomips(void)
{
  unsigned long ticks, loopbit;
  int lps_precision = 8;

  loops_per_jiffy = (1<<12);

  while ((loops_per_jiffy <<= 1) != 0) {
    /* wait for "start of" clock tick */
    ticks = jiffies;
    while (ticks == jiffies)
      /* nothing */;
    /* Go .. */
    ticks = jiffies;
    __delay(loops_per_jiffy);
    ticks = jiffies - ticks;
    if (ticks)
      break;
  }
  /*
  * Do a binary approximation to get loops_per_jiffy set to
  * equal one clock (up to lps_precision bits)
  */
  loops_per_jiffy >>= 1;
  loopbit = loops_per_jiffy;
  while (lps_precision-- && (loopbit >>= 1)) {
    loops_per_jiffy |= loopbit;
    ticks = jiffies;
    while (ticks == jiffies)
      /* nothing */;
    ticks = jiffies;
    __delay(loops_per_jiffy);
    if (jiffies != ticks)   /* longer than 1 tick */
      loops_per_jiffy &= ~loopbit;
  }	

  return (loops_per_jiffy);
}

static int read_ratio(char *page, char **start,
                        off_t off, int count,
                        int *eof, void *data)
{
  int len=0;
  unsigned long value, bogomips;
#ifdef STB7100
  int sh4_div=2, sh4_ic_div=2, module_div=2, slim_div=2, slim_bypass=0;
  unsigned long freq_pll0, freq_pll1, ndiv, pdiv, mdiv, val_sysaclkout=0;
#endif

  len+=sprintf(page+len, "Modul HZ = %d\n", modul_hz);

#ifdef STB7100
  value = ctrl_inl(CKGA_LCK);
  len+=sprintf(page+len, "CKGA_LCK = %lx\n", value);

  value = ctrl_inl(CKGA_MD_STA);
  len+=sprintf(page+len, "CKGA_MD_STA = %lx\n", value);
#endif

  value = ctrl_inl(CKGA_PLL0_CFG);
  len+=sprintf(page+len, "CKGA_PLL0_CFG = %lx\n", value);

#ifdef STB7100
  mdiv = (value >>  0) & 0xff;
  ndiv = (value >>  8) & 0xff;
  pdiv = (value >> 16) & 0x7;
  freq_pll0 = (((2 * (CONFIG_SH_EXTERNAL_CLOCK / 1000) * ndiv) / mdiv) / (1 << pdiv)) * 1000;
  freq_pll0=freq_pll0 / 1000000;

  value = ctrl_inl(CKGA_PLL0_LCK_STA);
  len+=sprintf(page+len, "CKGA_PLL0_LCK_STA = %lx\n", value);

  value = ctrl_inl(CKGA_PLL0_CLK1);
  len+=sprintf(page+len, "CKGA_PLL0_CLK1 = %lx\n", value);

  switch(value) {
    case 0: sh4_div=2;break;
    case 1: sh4_div=4;break;
    case 2: sh4_div=6;break;
    case 3: sh4_div=8;break;
    case 4: sh4_div=12;break;
    case 5: sh4_div=16;break;
    case 6: sh4_div=2;break;
    case 7: sh4_div=2;break;
  }

  value = ctrl_inl(CKGA_PLL0_CLK2);
  len+=sprintf(page+len, "CKGA_PLL0_CLK2 = %lx\n", value);

  switch(value) {
    case 0: sh4_ic_div=2;break;
    case 1: sh4_ic_div=4;break;
    case 2: sh4_ic_div=6;break;
    case 3: sh4_ic_div=8;break;
    case 4: sh4_ic_div=12;break;
    case 5: sh4_ic_div=16;break;
    case 6: sh4_ic_div=4;break;
    case 7: sh4_ic_div=4;break;
  }

  value = ctrl_inl(CKGA_PLL0_CLK3);
  len+=sprintf(page+len, "CKGA_PLL0_CLK3 = %lx\n", value);

  switch(value) {
    case 0: module_div=8;break;
    case 1: module_div=4;break;
    case 2: module_div=8;break;
    case 3: module_div=8;break;
    case 4: module_div=12;break;
    case 5: module_div=16;break;
    case 6: module_div=8;break;
    case 7: module_div=8;break;
  }

  value = ctrl_inl(CKGA_PLL0_CLK4);
  len+=sprintf(page+len, "CKGA_PLL0_CLK4 = %lx\n", value);

  switch(value) {
    case 0: slim_div=2;break;
    case 1: slim_div=4;break;
    case 2: slim_div=6;break;
    case 3: slim_div=8;break;
    case 4: slim_div=12;break;
    case 5: slim_div=16;break;
    case 6: slim_div=6;break;
    case 7: slim_div=6;break;
  }
#endif

  value = ctrl_inl(CKGA_PLL1_CFG);
  len+=sprintf(page+len, "CKGA_PLL1_CFG = %lx\n", value);

#ifdef STB7100
  mdiv = (value >>  0) & 0xff;
  ndiv = (value >>  8) & 0xff;
  pdiv = (value >> 16) & 0x7;
  freq_pll1 = (((2 * (CONFIG_SH_EXTERNAL_CLOCK / 1000) * ndiv) / mdiv) / (1 << pdiv)) * 1000;
  freq_pll1=freq_pll1 / 1000000;

  value = ctrl_inl(CKGA_PLL1_LCK_STA);
  len+=sprintf(page+len, "CKGA_PLL1_LCK_STA = %lx\n", value);

  value = ctrl_inl(CKGA_CLK_DIV);
  len+=sprintf(page+len, "CKGA_CLK_DIV = %lx\n", value);

  value = ctrl_inl(CKGA_CLK_EN);
  len+=sprintf(page+len, "CKGA_CLK_EN = %lx\n", value);
	
  value = ctrl_inl(CKGA_PLL1_BYPASS);
  len+=sprintf(page+len, "CKGA_PLL1_BYPASS = %lx\n", value);
  slim_bypass=value & 0x01;

  value = ctrl_inl(CKGA_CLKOUT_SEL);
  len+=sprintf(page+len, "CKGA_CLKOUT_SEL = %lx\n", value);
	
  switch(value) {
    case 0: val_sysaclkout=freq_pll0/sh4_div;break;
    case 1: val_sysaclkout=freq_pll0/sh4_ic_div;break;
    case 2: val_sysaclkout=freq_pll0/module_div;break;
    case 3: if(slim_bypass == 0) 
              val_sysaclkout=freq_pll0/slim_div;
            else
              val_sysaclkout=freq_pll1;
            break;
    case 4: val_sysaclkout=freq_pll0/slim_div;break;
    case 5: val_sysaclkout=freq_pll1;break;
    case 6: val_sysaclkout=freq_pll1;break;
    case 7: val_sysaclkout=freq_pll1;break;
    case 8: val_sysaclkout=freq_pll1;break;
    case 9: val_sysaclkout=freq_pll1/2;break;
    case 10: val_sysaclkout=freq_pll1/4;break;
    case 11: val_sysaclkout=freq_pll1/4;break;
  }
  len+=sprintf(page+len, "SYSACLKOUT (standard 266MHZ) = %ldMHZ\n", val_sysaclkout);
#endif

  value = ctrl_inl(TMU0_TCOR);
  len+=sprintf(page+len, "TMU0_TCOR = %lx\n", value);

  value = ctrl_inl(TMU0_TCNT);
  len+=sprintf(page+len, "TMU0_TCNT = %lx\n", value);

  value = ctrl_inl(TMU1_TCOR);
  len+=sprintf(page+len, "TMU1_TCOR = %lx\n", value);

  value = ctrl_inl(TMU1_TCNT);
  len+=sprintf(page+len, "TMU1_TCNT = %lx\n", value);

#ifdef STX7105
  value = ctrl_inl(TMU1_TCOR);
  len+=sprintf(page+len, "TMU2_TCOR = %lx\n", value);

  value = ctrl_inl(TMU1_TCNT);
  len+=sprintf(page+len, "TMU2_TCNT = %lx\n", value);
#endif

#ifdef STB7100
  len+=sprintf(page+len, "\n");
  len+=sprintf(page+len, "BOGOMIPS (static)= %ld\n", (freq_pll0 / sh4_div) - 3);
#endif

  bogomips = calc_bogomips();
  bogomips = bogomips/(500000/modul_hz);
  len+=sprintf(page+len, "BOGOMIPS (measured)= %ld\n", bogomips);

#ifdef STB7100
  len+=sprintf(page+len, "\n");
  len+=sprintf(page+len, "PLL0     = %ld MHZ\n", freq_pll0);
  len+=sprintf(page+len, "SH4      = %ld MHZ\n", freq_pll0 / sh4_div);
  len+=sprintf(page+len, "SH4_IC   = %ld MHZ\n", freq_pll0 / sh4_ic_div);
  len+=sprintf(page+len, "MODULE   = %ld MHZ\n", freq_pll0 / module_div);

  if(slim_bypass == 0)
    len+=sprintf(page+len, "SLIM     = %ld MHZ\n", freq_pll0 / slim_div);
  else
    len+=sprintf(page+len, "SLIM     = %ld MHZ\n", freq_pll1);

  len+=sprintf(page+len, "PLL1     = %ld MHZ\n", freq_pll1);
  len+=sprintf(page+len, "COMMS    = %ld MHZ\n", freq_pll1 / 4);
  len+=sprintf(page+len, "TMU0     = %ld MHZ\n", (freq_pll0 / module_div) / 4);
  len+=sprintf(page+len, "TMU1     = %ld MHZ\n", (freq_pll0 / module_div) / 4);

  len+=sprintf(page+len, "\n");
  len+=sprintf(page+len, "sh4 ratio (2,4,6,8,12,16)\n");
  len+=sprintf(page+len, "sh4_ic ratio (2,4,6,8,12,16)\n");
  len+=sprintf(page+len, "module ratio (4,8,12,16)\n");
  len+=sprintf(page+len, "slim ratio (2,4,6,8,12,16)\n");
#endif

  return len;
}

static int write_pll1_ndiv_mdiv(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  struct clk *clk;
  unsigned long regdata;
  int ndiv=0, mdiv=0;
  int ndiv_mdiv=simple_strtoul(buffer, NULL, 10);

#ifdef STB7100
  mdiv = (ndiv_mdiv >> 0) & 0xff;
  ndiv = (ndiv_mdiv >> 8) & 0xff;
#endif
#ifdef STX7105
  mdiv = (ndiv_mdiv >> 0) & 0x07;
  ndiv = (ndiv_mdiv >> 8) & 0xff;
#endif

  if(mdiv<0 || mdiv>255)
  {
    dprintk("[CPU_FREQU] mdiv not correct, use another PLL1 Frequenze (%d)\n", mdiv);
    return count;
  }
  if(ndiv<3 || ndiv>255)
  {
    dprintk("[CPU_FREQU] ndiv not correct, use another PLL1 Frequenze (%d)\n", ndiv);
    return count;
  }

  regdata = ctrl_inl(CKGA_PLL1_CFG);  // get data from register
#ifdef STB7100
  regdata = regdata & 0xfff80000;  // clear mdiv, ndiv and pdiv
  regdata = regdata | mdiv | (ndiv << 8) | (0x01 << 16);  // set mdiv, ndiv and pdiv

  ctrl_outl(0xC0DE, CKGA_LCK);
#endif
#ifdef STX7105
  regdata = regdata & 0xfffff800;  // clear mdiv, ndiv
  regdata = regdata | mdiv | (ndiv << 8);  // set mdiv, ndiv
#endif

  //ctrl_outl(0x00000002, CKGA_PLL1_BYPASS);  // set to SYSACLKIN

/*
  regdata = ctrl_inl(CKGA_PLL1_CFG);  // get data from register
  regdata = regdata & ~(1 << 19);  // disable PLL1
  ctrl_outl(regdata1, CKGA_PLL1_CFG);

  regdata = regdata & ~(0xff);  // clear MDIV
  regdata = regdata & ~(0xff) << 8;   // clear NDIV
  regdata = regdata & ~(0x07) << 16;  // clear PDIV

  regdata = regdata | mdiv;  // set MDIV
  regdata = regdata | ndiv << 8;  // set NDIV
  regdata = regdata | (0x1 & 0x7)  << 16;  // set PDIV

  regdata = regdata | 1 << 19;  // aktivate PLL1
*/
  ctrl_outl(regdata, CKGA_PLL1_CFG);  // write register

  // wait for lock
  //while((ctrl_inl(CKGA_PLL1_LCK_STA) & 0x01) == 0x00);

  //ctrl_outl(0x00000000, CKGA_PLL1_BYPASS);  // set to PLL1_CLK

#ifdef STB7100
  ctrl_outl(0x0, CKGA_LCK);
#endif

  // set /proc/cpuinfo
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("pll1_clk");
#else
  clk=clk_get(NULL, "pll1_clk");
#endif
  if(clk)
  {
    clk_disable(clk);
    clk_enable(clk);
  }
  else
    printk("[CPU_FREQU] pll1_clk not found\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("comms_clk");
#else
  clk=clk_get(NULL, "comms_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
  }
  else
    printk("[CPU_FREQU] comms_clk not found\n");

  return count;
}

static int write_pll0_ndiv_mdiv(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  struct clk *clk;
  unsigned long regdata, module_rate=0, sh4_hz;
  int ndiv=0, mdiv=0;
  int ndiv_mdiv=simple_strtoul(buffer, NULL, 10);
#ifdef STB7100
  unsigned long sta;

  mdiv = (ndiv_mdiv >> 0) & 0xff;
  ndiv = (ndiv_mdiv >> 8) & 0xff;
#endif
#ifdef STX7105
  mdiv = (ndiv_mdiv >> 0) & 0x07;
  ndiv = (ndiv_mdiv >> 8) & 0xff;
#endif

  if(mdiv<0 || mdiv>255)
  {
    dprintk("[CPU_FREQU] mdiv not correct, use another PLL0 Frequenze (%d)\n", mdiv);
    return count;
  }
  if(ndiv<3 || ndiv>255)
  {
    dprintk("[CPU_FREQU] ndiv not correct, use another PLL0 Frequenze (%d)\n", ndiv);
    return count;
  }

#ifdef STB7100
  ctrl_outl(0xC0DE, CKGA_LCK);
#endif

  regdata = ctrl_inl(CKGA_PLL0_CFG);  // get data from register

  regdata = regdata | 1 << 20;  // set to SYSACLKIN
  ctrl_outl(regdata, CKGA_PLL0_CFG);

  regdata = regdata & ~(1 << 19);  // disable PLL0
  ctrl_outl(regdata, CKGA_PLL0_CFG);

#ifdef STB7100
  regdata = regdata & ~(0xff);  // clear MDIV
  regdata = regdata & ~(0xff) << 8;   // clear NDIV
  regdata = regdata & ~(0x07) << 16;  // clear PDIV

  regdata = regdata | mdiv;  // set MDIV
  regdata = regdata | ndiv << 8;  // set NDIV
  regdata = regdata | (0x0 & 0x7)  << 16;  // set PDIV
#endif
#ifdef STX7105
  regdata = regdata & ~(0x07);  // clear MDIV
  regdata = regdata & ~(0xff) << 8;   // clear NDIV

  regdata = regdata | mdiv;  // set MDIV
  regdata = regdata | ndiv << 8;  // set NDIV
#endif

  regdata = regdata | 1 << 19;  // aktivate PLL0
  ctrl_outl(regdata, CKGA_PLL0_CFG);

#ifdef STB7100
  // wait for lock
  sta = ctrl_inl(CKGA_PLL0_LCK_STA);
  while((sta & 0x01) == 0x00)
  {
    sta = ctrl_inl(CKGA_PLL0_LCK_STA);
  }
#endif

  regdata = regdata & ~(1 << 20);  // set to PLL0 CLK
  ctrl_outl(regdata, CKGA_PLL0_CFG);

#ifdef STB7100
  ctrl_outl(0x0, CKGA_LCK);
#endif

  // set /proc/cpuinfo
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("pll0_clk");
#else
  clk=clk_get(NULL, "pll0_clk");
#endif
  if(clk)
  {
    clk_disable(clk);
    clk_enable(clk);
  }
  else
    printk("[CPU_FREQU] pll0_clk not found\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("sh4_clk");
#else
  clk=clk_get(NULL, "sh4_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
    // set bogomips in /proc/cpuinfo (use sh4 clock - 3)
    // measured bogomit not always correct in running system
    sh4_hz=clk_get_rate(clk);
    update_bogomips(sh4_hz);
  }
  else
    printk("[CPU_FREQU] sh4_clk not found\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("sh4_ic_clk");
#else
  clk=clk_get(NULL, "sh4_ic_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
  }
  else
    printk("[CPU_FREQU] sh4_ic_clk not found\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("module_clk");
#else
  clk=clk_get(NULL, "module_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
    module_rate=clk_get_rate(clk);
  }
  else
    printk("[CPU_FREQU] module_clk not found\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("slim_clk");
#else
  clk=clk_get(NULL, "slim_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
  }
  else
    printk("[CPU_FREQU] slim_clk not found\n");

  if(module_rate)
    update_tmu(module_rate);

  return count;
}

#ifdef STB7100
static int write_sh4_ratio(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  unsigned long value, sh4_hz;
  int sh4_ratio=simple_strtoul(buffer, NULL, 10);
  struct clk *clk;

  switch(sh4_ratio) {
    case 2:value=0x0;break;
    case 4:value=0x1;break;
    case 6:value=0x2;break;
    case 8:value=0x3;break;
    case 12:value=0x4;break;
    case 16:value=0x5;break;
    default:
      dprintk("[CPU_FREQU] sh4 ratio (%d) not correct, use 2,4,8,12 or 16\n", sh4_ratio);
      return count;break;
  }

  ctrl_outl(0xC0DE, CKGA_LCK);
  //0x0=1, 0x1=2, 0x2=3, 0x3=4, 0x4=6, 0x5=8
  ctrl_outl(value, CKGA_PLL0_CLK1);  // set sh4 ratio
  ctrl_outl(0x0, CKGA_LCK);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("sh4_clk");
#else
  clk=clk_get(NULL, "sh4_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
    // set bogomips in /proc/cpuinfo (use sh4 clock - 3)
    // measured bogomit not always correct in running system
    sh4_hz=clk_get_rate(clk);
    update_bogomips(sh4_hz);
  }
  else
    printk("[CPU_FREQU] sh4_clk not found\n"); 

  return count;
}

static int write_sh4_ic_ratio(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  unsigned long value;
  int sh4_ic_ratio=simple_strtoul(buffer, NULL, 10);
  struct clk *clk;

  switch(sh4_ic_ratio) {
    case 2:value=0x0;break;
    case 4:value=0x1;break;
    case 6:value=0x2;break;
    case 8:value=0x3;break;
    case 12:value=0x4;break;
    case 16:value=0x5;break;
    default:
      dprintk("[CPU_FREQU] sh4_ic ratio (%d) not correct, use 2,4,6,8,12 or 16\n", sh4_ic_ratio);
      return count;break;
  }

  ctrl_outl(0xC0DE, CKGA_LCK);
  //0x0=1, 0x1=2, 0x2=3, 0x3=4, 0x4=6, 0x5=8
  ctrl_outl(value, CKGA_PLL0_CLK2);  // set sh4_ic ratio
  ctrl_outl(0x0, CKGA_LCK);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("sh4_ic_clk");
#else
  clk=clk_get(NULL, "sh4_ic_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
  }
  else
    printk("[CPU_FREQU] sh4_ic_clk not found\n");

  return count;
}

static int write_module_ratio(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  unsigned long value, module_rate;
  int module_ratio=simple_strtoul(buffer, NULL, 10);
  struct clk *clk;

  switch(module_ratio) {
    case 8:value=0x0;break;
    case 4:value=0x1;break;
    case 12:value=0x4;break;
    case 16:value=0x5;break;
    default:
      dprintk("[CPU_FREQU] module ratio (%d) not correct, use 4,8,12 or 16\n", module_ratio);
      return count;break;
  }

  ctrl_outl(0xC0DE, CKGA_LCK);
  //0x0=4, 0x1=2, 0x2=4, 0x3=4, 0x4=6, 0x5=8
  ctrl_outl(value, CKGA_PLL0_CLK3);  // set module ratio
  ctrl_outl(0x0, CKGA_LCK);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("module_clk");
#else
  clk=clk_get(NULL, "module_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
    module_rate=clk_get_rate(clk);
    update_tmu(module_rate);
  }
  else
    printk("[CPU_FREQU] module_clk not found\n");

  return count;
}

static int write_slim_ratio(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  unsigned long value;
  int slim_ratio=simple_strtoul(buffer, NULL, 10);
  struct clk *clk;

  switch(slim_ratio) {
    case 2:value=0x0;break;
    case 4:value=0x1;break;
    case 6:value=0x2;break;
    case 8:value=0x3;break;
    case 12:value=0x4;break;
    case 16:value=0x5;break;
    default:
      dprintk("[CPU_FREQU] slim ratio (%d) not correct, use 2,4,6,8,12 or 16\n", slim_ratio);
      return count;break;
  }

  ctrl_outl(0xC0DE, CKGA_LCK);
  //0x0=1, 0x1=2, 0x2=3, 0x3=4, 0x4=6, 0x5=8
  ctrl_outl(value, CKGA_PLL0_CLK4);  // set slim ratio
  ctrl_outl(0x0, CKGA_LCK);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("slim_clk");
#else
  clk=clk_get(NULL, "slim_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
  }
  else
    printk("[CPU_FREQU] slim_clk not found\n");

  return count;
}

static int write_pll1_fdma_bypass(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  unsigned long value;
  int pll1_bypass=simple_strtoul(buffer, NULL, 10);
  struct clk *clk;

  switch(pll1_bypass) {
    case 0:value=0x0;break;
    case 1:value=0x1;break;
    default:
      dprintk("[CPU_FREQU] pll1_fdma_bypass (%d) not correct, use 0 or 1\n", pll1_bypass);
      return count;break;
  }

  ctrl_outl(0xC0DE, CKGA_LCK);
  ctrl_outl(value, CKGA_PLL1_BYPASS);  // set FDMA to PPL1 HZ
  ctrl_outl(0x0, CKGA_LCK);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
  clk=clk_get("slim_clk");
#else
  clk=clk_get(NULL, "slim_clk");
#endif
  if(clk)
  {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
    clk_recalc_rate(clk);
#else
    clk->ops->recalc(clk);
#endif
  }
  else
    printk("[CPU_FREQU] slim_clk not found\n");

  return count;
}

static int write_sysaclkout(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  int val_sysaclkout=simple_strtoul(buffer, NULL, 10);

  if(val_sysaclkout<0 || val_sysaclkout>11)
  {
    dprintk("[CPU_FREQU] sysaclkout (%d) not correct, use 0 to 11\n", val_sysaclkout);
    return count;
  }
        
  ctrl_outl(0xC0DE, CKGA_LCK);
  ctrl_outl(val_sysaclkout, CKGA_CLKOUT_SEL);  // set CKGA_CLKOUT_SEL
  ctrl_outl(0x0, CKGA_LCK);

  return count;
}

static int write_pll1_clk_div(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  int clk_div=simple_strtoul(buffer, NULL, 10);
        
  if(clk_div<0 || clk_div>15)
  {
    dprintk("[CPU_FREQU] pll1_clk_div (%d) not correct, use 0 to 15\n", clk_div);
    return count;
  }
        
  ctrl_outl(0xC0DE, CKGA_LCK);
  ctrl_outl(clk_div, CKGA_CLK_DIV);  // set CKGA_CLK_DIV
  ctrl_outl(0x0, CKGA_LCK);

  return count;
}
#endif

static int write_m_hz(struct file *file, const char *buffer,
                        unsigned long count, void *data)
{
  modul_hz=simple_strtoul(buffer, NULL, 10);

  return count;
}

int __init cpu_frequ_init(void)
{
  dprintk("[CPU_FREQU] initializing ...\n");

  cpu_dir=proc_mkdir("cpu_frequ",NULL);
  if(cpu_dir==NULL)
    return -ENOMEM;

  pll0_ndiv_mdiv=create_proc_entry("pll0_ndiv_mdiv",0644,cpu_dir);
  pll0_ndiv_mdiv->read_proc=read_ratio;
  pll0_ndiv_mdiv->write_proc=write_pll0_ndiv_mdiv;

  pll1_ndiv_mdiv=create_proc_entry("pll1_ndiv_mdiv",0644,cpu_dir);
  pll1_ndiv_mdiv->read_proc=read_ratio;
  pll1_ndiv_mdiv->write_proc=write_pll1_ndiv_mdiv;

#ifdef STB7100
  sh4_ratio=create_proc_entry("sh4_ratio",0644,cpu_dir);
  sh4_ratio->read_proc=read_ratio;
  sh4_ratio->write_proc=write_sh4_ratio;

  sh4_ic_ratio=create_proc_entry("sh4_ic_ratio",0644,cpu_dir);
  sh4_ic_ratio->read_proc=read_ratio;
  sh4_ic_ratio->write_proc=write_sh4_ic_ratio;

  module_ratio=create_proc_entry("module_ratio",0644,cpu_dir);
  module_ratio->read_proc=read_ratio;
  module_ratio->write_proc=write_module_ratio;

  slim_ratio=create_proc_entry("slim_ratio",0644,cpu_dir);
  slim_ratio->read_proc=read_ratio;
  slim_ratio->write_proc=write_slim_ratio;

  pll1_fdma_bypass=create_proc_entry("pll1_fdma_bypass",0644,cpu_dir);
  pll1_fdma_bypass->read_proc=read_ratio;
  pll1_fdma_bypass->write_proc=write_pll1_fdma_bypass;
 	
  pll1_clk_div=create_proc_entry("pll1_clk_div",0644,cpu_dir);
  pll1_clk_div->read_proc=read_ratio;
  pll1_clk_div->write_proc=write_pll1_clk_div;
 	
  sysaclkout=create_proc_entry("sysaclkout",0644,cpu_dir);
  sysaclkout->read_proc=read_ratio;
  sysaclkout->write_proc=write_sysaclkout;
#endif
 	
  m_hz=create_proc_entry("modul_hz",0644,cpu_dir);
  m_hz->read_proc=read_ratio;
  m_hz->write_proc=write_m_hz;

 return 0;
}

void __exit cpu_frequ_exit(void)
{
  dprintk("[CPU_FREQU] unloading ...\n");
  remove_proc_entry("pll0_ndiv_mdiv", cpu_dir);
  remove_proc_entry("pll1_ndiv_mdiv", cpu_dir);
#ifdef STB7100
  remove_proc_entry("sh4_ratio", cpu_dir);
  remove_proc_entry("sh4_ic_ratio", cpu_dir);
  remove_proc_entry("module_ratio", cpu_dir);
  remove_proc_entry("slim_ratio", cpu_dir);
  remove_proc_entry("pll1_fdma_bypass", cpu_dir);
  remove_proc_entry("pll1_clk_div", cpu_dir);
  remove_proc_entry("sysaclkout", cpu_dir);
#endif
  remove_proc_entry("modul_hz", cpu_dir);
  remove_proc_entry("cpu_dir", &proc_root);
}

module_init(cpu_frequ_init);
module_exit(cpu_frequ_exit);

MODULE_DESCRIPTION("Set CPU Frequenze on STb710x");
MODULE_AUTHOR("nit");
MODULE_LICENSE("GPL");
