/*
    StarCI2Win driver for TF7700, FORTIS.

    Copyright (C) 2007 konfetti <konfetti@ufs910.de>
	many thanx to jolt for good support

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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include <asm/system.h>
#include <asm/io.h>

#include "dvb_ca_en50221.h"

#include "dvb_ca_core.h"

static int debug;
static int extmoduldetect = 0;
#define dprintk(args...) \
	do { \
		if (debug) printk (args); \
	} while (0)

#if defined(FORTIS_HDBOX) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
struct stpio_pin*	cic_enable_pin = NULL;
struct stpio_pin*	module_A_pin = NULL;
struct stpio_pin*	module_B_pin = NULL;
#endif

#if defined(CUBEREVO) \
 || defined(CUBEREVO_MINI) \
 || defined(CUBEREVO_MINI2) \
 || defined(CUBEREVO_250HD) \
 || defined(CUBEREVO_9500HD) \
 || defined(CUBEREVO_2000HD) \
 || defined(CUBEREVO_MINI_FTA) \
 || defined(CUBEREVO_3000HD)
#define CUBEBOX
#else
#undef  CUBEBOX
#endif



/* StarCI2Win register definitions */
#define MODA_CTRL_REG 0x00
#define MODA_MASK_HIGH_REG 0x01
#define MODA_MASK_LOW_REG 0x02
#define MODA_PATTERN_HIGH_REG 0x03
#define MODA_PATTERN_LOW_REG 0x04
#define MODA_TIMING_REG 0x05
#define MODB_CTRL_REG 0x09
#define MODB_MASK_HIGH_REG 0x0a
#define MODB_MASK_LOW_REG 0x0b
#define MODB_PATTERN_HIGH_REG 0x0c
#define MODB_PATTERN_LOW_REG 0x0d
#define MODB_TIMING_REG 0x0e
#define SINGLE_MODE_CTRL_REG 0x10
#define TWIN_MODE_CTRL_REG 0x11
#define DEST_SEL_REG 0x17
#define INT_STATUS_REG 0x1a
#define INT_MASK_REG 0x1b
#define INT_CONFIG_REG 0x1c
#define STARCI_CTRL_REG 0x1f

/* hardware specific register values */
#if defined(TF7700)
unsigned char default_values[33] =
{
  0x00, /* register address for block transfer */
  0x02, 0x00, 0x01, 0x00, 0x00, 0x33, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x33, 0x00,
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00
};
#elif defined(CUBEBOX)
unsigned char default_values[33] =
{
   0x00, /* register address for block transfer */
/* 0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07 */
   0x00, 0x00, 0x01, 0x00, 0x00, 0x33, 0x00, 0x00,
/* 0x08  0x09  0x0a  0x0b  0x0c  0x0d  0x0e  0x0f */
   0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x33, 0x00,
/* 0x10  0x11  0x12  0x13  0x14  0x15  0x16  0x17 */
   0x02, 0x9a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* 0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e  0x1f */
   0xc0, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x00
 };
#elif defined (FORTIS_HDBOX)
unsigned char default_values[33] =
{
  0x00, /* register address for block transfer */
  0x00, 0x00, 0x02, 0x00, 0x00, 0x44, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x44, 0x00,
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x03, 0x06, 0x00, 0x03, 0x01
};
#elif defined (ATEVIO7500)
unsigned char default_values[33] =
{
  0x00,
  0x00, 0x00, 0x02, 0x00, 0x00, 0x44, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x44, 0x00,
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x03, 0x06, 0x00, 0x03, 0x01
};
#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
unsigned char default_values[33] =
{
  0x00,
  0x00, 0x00, 0x02, 0x00, 0x00, 0x44, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x44, 0x00,
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x03, 0x06, 0x00, 0x03, 0x01
};
#endif

/* EMI configuration */
unsigned long reg_config = 0;
unsigned long reg_buffer = 0;

#if defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
unsigned long reg_sysconfig = 0;
#endif

#if defined(FORTIS_HDBOX) \
 || defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
static unsigned char *slot_membase[2];
#else
/* for whatever reason the access has to be done though a short pointer */
static unsigned short *slot_membase[2];
#endif

#define EMI_DATA0_WE_USE_OE(a)		(a<<26)
#define EMI_DATA0_WAIT_POL(a)		(a<<25)
#define EMI_DATA0_LATCH_POINT(a)	(a<<20)
#define EMI_DATA0_DATA_DRIVE(a)		(a<<15)
#define EMI_DATA0_BUS_RELEASE(a)	(a<<11)
#define EMI_DATA0_CS_ACTIVE(a)		(a<<9)
#define EMI_DATA0_OE_ACTIVE(a)		(a<<7)
#define EMI_DATA0_BE_ACTIVE(a)		(a<<5)
#define EMI_DATA0_PORT_SIZE(a)		(a<<3)
#define EMI_DATA0_DEVICE_TYPE(a)	(a<<0)

#define EMI_DATA1_CYCLE(a)		(a<<31)
#define EMI_DATA1_ACCESS_READ(a)	(a<<24)
#define EMI_DATA1_CSE1_READ(a)		(a<<20)
#define EMI_DATA1_CSE2_READ(a)		(a<<16)
#define EMI_DATA1_OEE1_READ(a)		(a<<12)
#define EMI_DATA1_OEE2_READ(a)		(a<<8)
#define EMI_DATA1_BEE1_READ(a)		(a<<4)
#define EMI_DATA1_BEE2_READ(a)		(a<<0)

#define EMI_DATA2_CYCLE(a)		(a<<31)
#define EMI_DATA2_ACCESS_WRITE(a)	(a<<24)
#define EMI_DATA2_CSE1_WRITE(a)		(a<<20)
#define EMI_DATA2_CSE2_WRITE(a)		(a<<16)
#define EMI_DATA2_OEE1_WRITE(a)		(a<<12)
#define EMI_DATA2_OEE2_WRITE(a)		(a<<8)
#define EMI_DATA2_BEE1_WRITE(a)		(a<<4)
#define EMI_DATA2_BEE2_WRITE(a)		(a<<0)

#if defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
#define EMIConfigBaseAddress 0xfe700000
#define SysConfigBaseAddress 0xFE001000
#else
#define EMIConfigBaseAddress 0x1A100000
#endif

#define EMIBufferBaseAddress 0x1A100800

#define EMIBank0 0x100
#define EMIBank1 0x140
#define EMIBank2 0x180
#define EMIBank3 0x1C0
#define EMIBank4 0x200
#define EMIBank5 0x240 /* virtual */

#define EMIBank0BaseAddress EMIConfigBaseAddress + EMIBank0
#define EMIBank1BaseAddress EMIConfigBaseAddress + EMIBank1
#define EMIBank2BaseAddress EMIConfigBaseAddress + EMIBank2
#define EMIBank3BaseAddress EMIConfigBaseAddress + EMIBank3
#define EMIBank4BaseAddress EMIConfigBaseAddress + EMIBank4
#define EMIBank5BaseAddress EMIConfigBaseAddress + EMIBank5

/* konfetti: first base address of EMI. the specification
 * says that all other can be calculated from the top address
 * but this doesnt work for me. so I set the address fix later on
 * and dont waste time on that.
 */
#define EMI_BANK0_BASE_ADDRESS  0x40000000

/* ConfigBase */
#define EMI_STA_CFG	0x0010
#define EMI_STA_LCK 	0x0018
#define EMI_LCK 	0x0020
/* general purpose config register
 * 32Bit, R/W, reset=0x00
 * Bit 31-5 reserved
 * Bit 4 = PCCB4_EN: Enable PC card bank 4
 * Bit 3 = PCCB3_EN: Enable PC card bank 3
 * Bit 2 = EWAIT_RETIME
 * Bit 1/0 reserved
 */
#define EMI_GEN_CFG 	0x0028

#define EMI_FLASH_CLK_SEL 0x0050 /* WO: 00, 10, 01 */
#define EMI_CLK_EN 0x0068 /* WO: must only be set once !!*/

/* BankBase */
#define EMI_CFG_DATA0	0x0000
#define EMI_CFG_DATA1	0x0008
#define EMI_CFG_DATA2	0x0010
#define EMI_CFG_DATA3	0x0018


/* **************************** */
/* EMIBufferBaseAddress + Offset*/
/* **************************** */
#define EMIB_BANK0_TOP_ADDR 0x000
#define EMIB_BANK1_TOP_ADDR 0x010
#define EMIB_BANK2_TOP_ADDR 0x020
 /* 32Bit, R/W, reset=0xFB
  * Bits 31- 8: reserved
  * Bits 7 - 0: Bits 27-22 off Bufferadresse
  */
#define EMIB_BANK3_TOP_ADDR 0x030
 /* 32Bit, R/W, reset=0xFB
  * Bits 31- 8: reserved
  * Bits 7 - 0: Bits 27-22 off Bufferadresse
  */
#define EMIB_BANK4_TOP_ADDR 0x040
#define EMIB_BANK5_TOP_ADDR 0x050

/* 32 Bit, R/W, reset=0x110
 * Enable/Disable the banks
 */
#define EMIB_BANK_EN 	0x0060

static struct dvb_ca_state ca_state;

static int starci_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address);

/* EMI Banks End ************************************ */

/* ************************** */
/*     StarCI2Win control     */
/* ************************** */

static int
starci_writeregN (struct dvb_ca_state *state, u8 * data, u16 len)
{
  int 	ret = -EREMOTEIO;
  struct i2c_msg msg;

  msg.addr = state->i2c_addr;
  msg.flags = 0;
  msg.buf = data;
  msg.len = len;

  if ((ret = i2c_transfer (state->i2c, &msg, 1)) != 1)
  {
    printk ("%s: writereg error(err == %i, reg == 0x%02x\n",
            __func__, ret, data[0]);
    ret = -EREMOTEIO;
  }

  return ret;
}

static int starci_writereg(struct dvb_ca_state* state, int reg, int data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = state->i2c_addr,
		.flags = 0, .buf = buf, .len = 2 };
	int err;

	dprintk("cimax: %s:  write reg 0x%02x, value 0x%02x\n",
					__func__,reg, data);

	if ((err = i2c_transfer(state->i2c, &msg, 1)) != 1) {
		dprintk("%s: writereg error(err == %i, reg == 0x%02x,"
			 " data == 0x%02x)\n", __func__, err, reg, data);
		return -EREMOTEIO;
	}

	return 0;
}

static int starci_readreg(struct dvb_ca_state* state, u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };
	struct i2c_msg msg[] = {
		{ .addr = state->i2c_addr, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = state->i2c_addr, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	};

	ret = i2c_transfer(state->i2c, msg, 2);

	if (ret != 2) {
		dprintk("%s: reg=0x%x (error=%d)\n", __func__, reg, ret);
		return ret;
	}

	dprintk("%s read reg 0x%02x, value 0x%02x\n",__func__, reg, b1[0]);

	return b1[0];
}

/* This function gets the CI source
   Params:
     slot - CI slot number (0|1)
     source - tuner number (0|1)
*/
void getCiSource(int slot, int* source)
{
#if !defined(ATEVIO7500) \
 && !defined(HS7110) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7119) \
 && !defined(HS7819)
  int val;
  val = starci_readreg(&ca_state, TWIN_MODE_CTRL_REG);
  val &= 0x20;

  if(slot == 0)
  {
#if defined(TF7700)
    if(val != 0)
      *source = 1;
    else
      *source = 0;
#else
    if(val != 0)
      *source = 0;
    else
      *source = 1;
#endif
  }

  if(slot == 1)
  {
#if defined(TF7700)
    if(val != 0)
      *source = 0;
    else
      *source = 1;
#else
    if(val != 0)
      *source = 1;
    else
      *source = 0;
#endif
  }
#endif
}

/* This function sets the CI source
   Params:
     slot - CI slot number (0|1)
     source - tuner number (0|1)
*/
int setCiSource(int slot, int source)
{
  int val;

  printk("%s> %d %d\n", __func__, slot, source);

  if((slot < 0) || (slot > 1) ||
     (source < 0) || (source > 1))
    return -1;

#if defined(CUBEBOX)
/* fixme: think on this */
  if (slot == 0)
  {
       return starci_writereg(&ca_state, TWIN_MODE_CTRL_REG, 0x82);
  } else
  {
       return starci_writereg(&ca_state, TWIN_MODE_CTRL_REG, 0xa2);
  }
#else
  val = starci_readreg(&ca_state, TWIN_MODE_CTRL_REG);
  if(slot != source)
  {
    /* send stream A through module B and stream B through module A */
#if defined(TF7700) || defined(ATEVIO7500) || defined(FORTIS_HDBOX)
    val |= 0x20;
#else
    val &= ~0x20;
#endif
  }
  else
  {
    /* enforce direct mapping */
    /* send stream A through module A and stream B through module B */
#if defined(TF7700) || defined(ATEVIO7500) || defined(FORTIS_HDBOX)
    val &= ~0x20;
#else
    val |= 0x20;
#endif
  }

  return starci_writereg(&ca_state, TWIN_MODE_CTRL_REG, val);
#endif
}

void setDestination(struct dvb_ca_state *state, int slot)
{
   int result = 0;
   int loop = 0;
   int activationMask[2] = {0x02, 0x04};
   int deactivationMask[2] = {0x04, 0x02};

   dprintk("%s (slot = %d)>\n", __func__, slot);

   if((slot < 0) || (slot > 1))
     return;

   /* read destination register */
   result = starci_readreg(state, DEST_SEL_REG);

   while ((result & activationMask[slot]) == 0)
   {
        /* unselect the other slot if this was selected before */
        result = result & ~deactivationMask[slot];
        starci_writereg(state, DEST_SEL_REG, result | activationMask[slot]);

	/* try it first time without sleep */
        if (loop > 0) msleep(10);

	/* re-read the destination register */
        result = starci_readreg(state, DEST_SEL_REG);

	dprintk("%s (slot = %d, loop = %d): result = 0x%x\n", __func__, slot,loop, result);

      	loop++;
      	if (loop >= 10)
	{
           dprintk("->abort setting slot destination\n");
	   break;
      	}
   }
   dprintk("%s (slot = %d)<\n", __func__, slot);
}

static int starci_poll_slot_status(struct dvb_ca_en50221 *ca, int slot, int open)
{
   struct dvb_ca_state *state = ca->data;
   int    slot_status = 0;
   int    ctrlReg[2] = {MODA_CTRL_REG, MODB_CTRL_REG};

   dprintk("%s (%d; open = %d) >\n", __func__, slot, open);

   if ((slot < 0) || (slot > 1))
	  return 0;

   slot_status = starci_readreg(state, ctrlReg[slot]) & 0x01;

   if (slot_status)
   {
      if (state->module_status[slot] & SLOTSTATUS_RESET)
      {
          unsigned int result = starci_read_attribute_mem(ca, slot, 0); 

          dprintk("result = 0x%02x\n", result);

          if (result == 0x1d)
               state->module_status[slot] = SLOTSTATUS_READY;
      }
      else
      if (state->module_status[slot] & SLOTSTATUS_NONE)
      {
#if defined(ATEVIO7500) || defined(FORTIS_HDBOX)
          if (slot == 0)
          {
        	 stpio_set_pin(module_A_pin, 1);
	      }
          else
          {
        	  stpio_set_pin(module_B_pin, 1);
          }
#endif

           dprintk("Modul now present\n");
	       state->module_status[slot] = SLOTSTATUS_PRESENT;
      }
   } else
   {
      if (!(state->module_status[slot] & SLOTSTATUS_NONE))
      {
#if defined(ATEVIO7500) || defined(FORTIS_HDBOX)
          if (slot == 0)
          {
        	 stpio_set_pin(module_A_pin, 0);
	      }
          else
          {
        	  stpio_set_pin(module_B_pin, 0);
          }
#endif

           dprintk("Modul now not present\n");
	       state->module_status[slot] = SLOTSTATUS_NONE;
      }
   }

   if (state->module_status[slot] != SLOTSTATUS_NONE)
      slot_status = DVB_CA_EN50221_POLL_CAM_PRESENT;
   else
      slot_status = 0;
   
   if (state->module_status[slot] & SLOTSTATUS_READY)
      slot_status |= DVB_CA_EN50221_POLL_CAM_READY;

   dprintk("Module %c (%d): result = %d, status = %d\n",
			  slot ? 'B' : 'A', slot, slot_status,
			  state->module_status[slot]);

  dprintk("%s 2<\n", __func__);

  return slot_status;
}

static int starci_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
  struct dvb_ca_state *state = ca->data;
  int reg[2] = {MODA_CTRL_REG, MODB_CTRL_REG};
  int result;

  dprintk("%s >\n", __func__);

  if((slot < 0) || (slot > 1))
    return -1;

  state->module_status[slot] = SLOTSTATUS_RESET;

  result = starci_readreg(state, reg[slot]);

  /* only reset if modul is present */
  if (result & 0x01)
  {
#if defined(CUBEBOX)
    starci_writereg(state, reg[slot], 0x80);

    msleep(200);

    /* reset "rst" bit */
    result = starci_readreg(state, reg[slot]);
    starci_writereg(state, reg[slot], 0x00);

    dprintk(KERN_ERR "Reset Module %c\n", (slot == 0) ? 'A' : 'B');

#else

    result = starci_readreg(state, reg[slot]);
    printk("%s: result = 0x%x\n", __func__, result);
    starci_writereg(state, reg[slot], result | 0x80);

    starci_writereg(state, DEST_SEL_REG, 0x0);
#if defined(ATEVIO7500) \
 || defined(FORTIS_HDBOX) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
    msleep(200);
#else
    msleep(60);
#endif

    /* reset "rst" bit */
    result = starci_readreg(state, reg[slot]);
    starci_writereg(state, reg[slot], result & ~0x80);

    dprintk(KERN_ERR "Reset Module %c\n", (slot == 0) ? 'A' : 'B');
#endif
  }

  dprintk("%s <\n", __func__);

  return 0;
}

static int starci_read_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address)
{
  struct dvb_ca_state *state = ca->data;
  int res = 0;
  int result;
  int reg[2] = {MODA_CTRL_REG, MODB_CTRL_REG};

  dprintk("%s > slot = %d, address = %d\n", __func__, slot, address);

  if((slot < 0) || (slot > 1))
    return -1;

  result = starci_readreg(state, reg[slot]);

  if (result & 0xC)
    starci_writereg(state, reg[slot], (result & ~0xC));

  setDestination(state, slot);

  res = slot_membase[slot][address] & 0xFF;

  if (address <= 2)
     dprintk ("address = %d: res = 0x%.x\n", address, res);
  else
  {
     if ((res > 31) && (res < 127))
	  dprintk("%c\n", res);
     else
	  dprintk(".\n");
  }

  return res;
}

static int starci_write_attribute_mem(struct dvb_ca_en50221 *ca, int slot, int address, u8 value)
{
  struct dvb_ca_state *state = ca->data;
  int result;
  int reg[2] = {MODA_CTRL_REG, MODB_CTRL_REG};

  dprintk("%s > slot = %d, address = %d, value = %d\n", __func__, slot, address, value);

  if((slot < 0) || (slot > 1))
    return -1;

  result = starci_readreg(state, reg[slot]);

  /* delete bit 3/4 ->access to attribute mem */
  if (result & 0xC)
      starci_writereg(state, reg[slot], (result & ~0xC));

  setDestination(state, slot);

  slot_membase[slot][address] = value;

  return 0;
}

static int starci_read_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address)
{
  struct dvb_ca_state *state = ca->data;
  int res = 0;
  int result;
  int reg[2] = {MODA_CTRL_REG, MODB_CTRL_REG};

  dprintk("%s > slot = %d, address = %d\n", __func__, slot, address);

  if((slot < 0) || (slot > 1))
    return -1;

  result = starci_readreg(state, reg[slot]);

  /* FIXME: handle "result" ->is the module really present */

  /* access i/o mem (bit 3) */
  if (!(result & 0x4))
      starci_writereg(state, reg[slot], (result & ~0xC) | 0x4);

  setDestination(state, slot);

  res = slot_membase[slot][address] & 0xFF;

  if (address > 2)
  {
     if ((res > 31) && (res < 127))
	  dprintk("%c", res);
     else
	  dprintk(".");
  }

  return res;
}

static int starci_write_cam_control(struct dvb_ca_en50221 *ca, int slot, u8 address, u8 value)
{
  struct dvb_ca_state *state = ca->data;
  int result;
  int reg[2] = {MODA_CTRL_REG, MODB_CTRL_REG};

  dprintk("%s > slot = %d, address = %d, value = %d\n", __func__, slot, address, value);

  if((slot < 0) || (slot > 1))
    return -1;

  result = starci_readreg(state, reg[slot]);

  /* FIXME: handle "result" ->is the module really present */

  /* access to i/o mem */
  if (!(result & 0x4))
      starci_writereg(state, reg[slot], (result & ~0xC) | 0x4);

  setDestination(state, slot);

  slot_membase[slot][address] = value;

	//without this some modules not working (unicam evo, unicam twin, zetaCam)
	//i have tested with 9 modules an all working with this code
  if(extmoduldetect == 1 && value == 8 && address == 1)
    slot_membase[slot][address] = 0;

  return 0;
}

static int starci_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
  dprintk("%s > slot = %d\n", __func__, slot);

  /*Power control : (@18h); quatsch slot shutdown ->0x17*/

  return 0;
}

static int starci_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
  struct dvb_ca_state *state = ca->data;
  int reg[2] = {MODA_CTRL_REG, MODB_CTRL_REG};
  int result;

  dprintk("%s > slot = %d\n", __func__, slot);

  if((slot < 0) || (slot > 1))
    return -1;

  result = starci_readreg(state, reg[slot]);

#if !defined(ATEVIO7500) \
 && !defined(FORTIS_HDBOX) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7819) \
 && !defined(CUBEBOX) 
  starci_writereg(state, reg[slot], 0x23);
#else
  starci_writereg(state, reg[slot], 0x21);
#endif

  /* reading back from the register implements the delay */
  result = starci_readreg(state, reg[slot]);

  dprintk("%s: writing 0x%x\n", __func__, result | 0x40);
  starci_writereg(state, reg[slot], result | 0x40);
  result = starci_readreg(state, reg[slot]);

  dprintk("%s: result 0x%x (%d)\n", __func__, result, slot);

  if (!(result & 0x40))
      printk("Error setting ts enable on slot 0\n");

  return 0;
}

int init_ci_controller(struct dvb_adapter* dvb_adap)
{
  struct dvb_ca_state *state = &ca_state;
  int result = 0;

  dprintk("%s >\n", __func__);

  state->dvb_adap = dvb_adap;
  state->i2c_addr = 0x40;

#if defined(FORTIS_HDBOX) || defined(ATEVIO7500)
  state->i2c = i2c_get_adapter(2);
#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
  state->i2c = i2c_get_adapter(1);
  state->i2c_addr = 0x43;
#else
  state->i2c = i2c_get_adapter(0);
#endif

  memset(&state->ca, 0, sizeof(struct dvb_ca_en50221));

  /* register CI interface */
  state->ca.owner = THIS_MODULE;

  state->ca.read_attribute_mem 	= starci_read_attribute_mem;
  state->ca.write_attribute_mem = starci_write_attribute_mem;
  state->ca.read_cam_control 	= starci_read_cam_control;
  state->ca.write_cam_control 	= starci_write_cam_control;
  state->ca.slot_shutdown 	    = starci_slot_shutdown;
  state->ca.slot_ts_enable 	    = starci_slot_ts_enable;

  state->ca.slot_reset 		    = starci_slot_reset;
  state->ca.poll_slot_status 	= starci_poll_slot_status;

  state->ca.data 		        = state;

  state->module_status[0] = SLOTSTATUS_NONE;
  state->module_status[1] = SLOTSTATUS_NONE;

  reg_config = (unsigned long)ioremap(EMIConfigBaseAddress, 0x7ff);

#if !defined(ATEVIO7500) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7819)
  reg_buffer = (unsigned long)ioremap(EMIBufferBaseAddress, 0x40);
#endif

#if defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
  reg_sysconfig = (unsigned long)ioremap(SysConfigBaseAddress, 0x200);
#endif

  dprintk (KERN_ERR "ioremap 0x%.8x -> 0x%.8lx\n", EMIConfigBaseAddress, reg_config);

#if !defined(ATEVIO7500) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7819)
  dprintk (KERN_ERR "ioremap 0x%.8x -> 0x%.8lx\n", EMIBufferBaseAddress, reg_buffer);
#endif

#if defined(FORTIS_HDBOX)
  cic_enable_pin = stpio_request_pin (3, 6, "StarCI", STPIO_OUT);
  stpio_set_pin (cic_enable_pin, 1);
  msleep(250);
  stpio_set_pin (cic_enable_pin, 0);
  msleep(250);

  module_A_pin = stpio_request_pin (1, 2, "StarCI_ModA", STPIO_OUT);
  module_B_pin = stpio_request_pin (2, 7, "StarCI_ModB", STPIO_OUT);

  stpio_set_pin (module_A_pin, 0);
  stpio_set_pin (module_B_pin, 0);
#elif defined(ATEVIO7500)
  /* the magic potion - some clkb settings */
  ctrl_outl(0x0000c0de, 0xfe000010);
  ctrl_outl(0x00000008, 0xfe0000b4);
  ctrl_outl(0x0000c1a0, 0xfe000010);

  /* necessary to access i2c register */
  ctrl_outl(0x1c, reg_sysconfig + 0x160);

  module_A_pin = stpio_request_pin (11, 0, "StarCI_ModA", STPIO_OUT);
  module_B_pin = stpio_request_pin (11, 1, "StarCI_ModB", STPIO_OUT);

  cic_enable_pin = stpio_request_pin (15, 1, "StarCI", STPIO_OUT);
  stpio_set_pin (cic_enable_pin, 1);
  msleep(250);
  stpio_set_pin (cic_enable_pin, 0);
  msleep(250);

  stpio_set_pin (module_A_pin, 0);
  stpio_set_pin (module_B_pin, 0);

#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
  /* the magic potion - some clkb settings */
  ctrl_outl(0x0000c0de, 0xfe000010);
  ctrl_outl(0x00000008, 0xfe0000b4);
  ctrl_outl(0x0000c1a0, 0xfe000010);

  cic_enable_pin = stpio_request_pin (6, 2, "StarCI_RST", STPIO_OUT);
  stpio_set_pin (cic_enable_pin, 1);
  msleep(250);
  stpio_set_pin (cic_enable_pin, 0);
  msleep(250);
#endif

  /* reset the chip */
  starci_writereg(state, 0x1f, 0x80);

  starci_writeregN(state, default_values, 33);

  /* lock the configuration */
  starci_writereg(state, 0x1f, 0x01);

  /* power on (only possible with LOCK = 1)
     other bits cannot be set when LOCK is = 1 */

#if defined(ATEVIO7500) \
 || defined(FORTIS_HDBOX) \
 || defined(HS7110) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7119) \
 || defined(HS7810A) \
 || defined(HS7819) \
 || defined(CUBEBOX)
  starci_writereg(state, 0x18, 0x21);
#else
  starci_writereg(state, 0x18, 0x01);
#endif

#if !defined(ATEVIO7500) \
 && !defined(FORTIS_HDBOX) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7819)
  ctrl_outl(0x0, reg_config + EMI_LCK);
  ctrl_outl(0x0, reg_config + EMI_GEN_CFG);
#endif

#if defined(FORTIS_HDBOX)
/* fixme: this is mysterious on HDBOX! there is no lock setting EMI_LCK and there is
* no EMI_CLK_EN, so the settings cant get into effect?
*/
  ctrl_outl(0x8486d9,reg_config + EMIBank1 + EMI_CFG_DATA0);
  ctrl_outl(0x9d220000,reg_config + EMIBank1 + EMI_CFG_DATA1);
  ctrl_outl(0x9d220000,reg_config + EMIBank1 + EMI_CFG_DATA2);
  ctrl_outl(0x8,reg_config + EMIBank1 + EMI_CFG_DATA3);

#elif defined(ATEVIO7500) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
  ctrl_outl(0x8486d9, reg_config + EMIBank3 + EMI_CFG_DATA0);
  ctrl_outl(0x9d220000,reg_config + EMIBank3 + EMI_CFG_DATA2);
  ctrl_outl(0x8,reg_config + EMIBank3 + EMI_CFG_DATA3);
  ctrl_outl(0x0, reg_config + EMI_GEN_CFG);
#else /* Cuberevo & TF7700  */
  ctrl_outl(	EMI_DATA0_WE_USE_OE(0x0) 	|
		  EMI_DATA0_WAIT_POL(0x0)	|
		  EMI_DATA0_LATCH_POINT(30)	|
		  EMI_DATA0_DATA_DRIVE(12)	|
		  EMI_DATA0_BUS_RELEASE(50)	|
		  EMI_DATA0_CS_ACTIVE(0x3)	|
		  EMI_DATA0_OE_ACTIVE(0x1)	|
		  EMI_DATA0_BE_ACTIVE(0x2)	|
		  EMI_DATA0_PORT_SIZE(0x2)	|
		  EMI_DATA0_DEVICE_TYPE(0x1),reg_config + EMIBank2 + EMI_CFG_DATA0);
  ctrl_outl(	EMI_DATA1_CYCLE(0x1)		|
		  EMI_DATA1_ACCESS_READ(100)	|
		  EMI_DATA1_CSE1_READ(0)	|
		  EMI_DATA1_CSE2_READ(0)	|
		  EMI_DATA1_OEE1_READ(10)	|
		  EMI_DATA1_OEE2_READ(10)	|
		  EMI_DATA1_BEE1_READ(10)	|
		  EMI_DATA1_BEE2_READ(10),reg_config + EMIBank2 + EMI_CFG_DATA1);
  ctrl_outl(	EMI_DATA2_CYCLE(1)		|
		  EMI_DATA2_ACCESS_WRITE(100)	|
		  EMI_DATA2_CSE1_WRITE(0)	|
		  EMI_DATA2_CSE2_WRITE(0)	|
		  EMI_DATA2_OEE1_WRITE(10)	|
		  EMI_DATA2_OEE2_WRITE(10)	|
		  EMI_DATA2_BEE1_WRITE(10)	|
		  EMI_DATA2_BEE2_WRITE(10),reg_config + EMIBank2 + EMI_CFG_DATA2);

  ctrl_outl(0x0, reg_config + EMIBank2 + EMI_CFG_DATA3);

#if defined(CUBEBOX)
  ctrl_outl(0x2, reg_config + EMI_FLASH_CLK_SEL);
#else
  ctrl_outl(0x0, reg_config + EMI_FLASH_CLK_SEL);
#endif

#endif

#if !defined(ATEVIO7500) && !defined(FORTIS_HDBOX) && !defined(HS7110) && !defined(HS7119) && !defined(HS7810A) && !defined(HS7819)
  ctrl_outl(0x1, reg_config + EMI_CLK_EN);
#endif

#if defined(FORTIS_HDBOX)
//is [0] = top slot?
  slot_membase[0] = ioremap( 0xa2000000, 0x1000 );
#elif defined(ATEVIO7500)
  slot_membase[0] = ioremap( 0x06800000, 0x1000 );
#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
  slot_membase[0] = ioremap( 0x06000000, 0x1000 );
#elif defined(CUBEBOX)
  slot_membase[0] = ioremap( 0x3000000, 0x1000 );
  printk("membase-0 0x%08x\n", slot_membase[0]);
#else
  slot_membase[0] = ioremap( 0xa3000000, 0x1000 );
#endif
  if( slot_membase[0] == NULL )
  {
	  result = 1;
	  goto error;
  }
#if defined(TF7700)
  slot_membase[1] = ioremap( 0xa3400000, 0x1000 );
#elif defined(FORTIS_HDBOX)
//is [1] = bottom slot?
  slot_membase[1] = ioremap( 0xa2010000, 0x1000 );
#elif defined(ATEVIO7500)
  slot_membase[1] = ioremap( 0x06810000, 0x1000 );
#elif defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819)
  slot_membase[1] = ioremap( 0x06010000, 0x1000 );
#elif defined(CUBEBOX)
  slot_membase[1] = ioremap( 0x3010000, 0x1000 );
  printk("membase-1 0x%08x\n", slot_membase[1]);
#else
  slot_membase[1] = ioremap( 0xa3010000, 0x1000 );
#endif
  if( slot_membase[1] == NULL )
  {
	  iounmap( slot_membase[0] );
	  result = 1;
	  goto error;
  }

#if !defined(ATEVIO7500) \
 && !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(FORTIS_HDBOX) \
 && !defined(HS7420) \
 && !defined(HS7429) \
 && !defined(HS7810A) \
 && !defined(HS7819)
  ctrl_outl(0x1F,reg_config + EMI_LCK);
#endif

  dprintk("init_startci: call dvb_ca_en50221_init\n");

#if defined(CUBEREVO_250HD) || defined(CUBEREVO_2000HD) || defined(CUBEREVO_MINI_FTA)
  if ((result = dvb_ca_en50221_init(state->dvb_adap, &state->ca, 0, 1)) != 0) {
#else
  if ((result = dvb_ca_en50221_init(state->dvb_adap, &state->ca, 0, 2)) != 0) {
#endif
	  printk(KERN_ERR "ca0 initialisation failed.\n");
	  goto error;
  }

  dprintk(KERN_INFO "cimax: ca0 interface initialised.\n");

error:
  dprintk("init_starci <\n");

  return result;
}

EXPORT_SYMBOL(init_ci_controller);
EXPORT_SYMBOL(setCiSource);
EXPORT_SYMBOL(getCiSource);

int starci2win_init(void)
{
    printk("starci2win loaded\n");
    return 0;
}

void starci2win_exit(void)
{
   printk("starci2win unloaded\n");
}

module_init             (starci2win_init);
module_exit             (starci2win_exit);

MODULE_DESCRIPTION      ("CI Controller");
MODULE_AUTHOR           ("");
MODULE_LICENSE          ("GPL");

module_param(extmoduldetect, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(extmoduldetect, "Ext. Modul detect 0=disabled 1=enabled");
