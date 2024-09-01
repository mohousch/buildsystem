/*
 *  spider-team 2011
 *
 *  sh4 smartcard driver
 *
 *  this driver ported from QboxHd open source and csctapi together
 *  tested on spark STX7111 and HL101 STi7101 boxes
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/fs.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>
#endif
#include <asm/system.h>
#include <linux/cdev.h>

#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>

#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/stm/pio.h>
#include <linux/ioport.h>
#include <asm/signal.h>
#include <asm/io.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/device.h> /* class_creatre */

#include "sci_types.h"
#include "sci.h"
#include "atr.h"

/*****************************
 * MACROS
 *****************************/

static struct cdev sci_cdev;

static unsigned char byte_invert[] =
{
	0xFF, 0x7F, 0xBF, 0x3F, 0xDF, 0x5F, 0x9F, 0x1F, 0xEF, 0x6F, 0xAF, 0x2F, 0xCF, 0x4F, 0x8F, 0x0F,
	0xF7, 0x77, 0xB7, 0x37, 0xD7, 0x57, 0x97, 0x17, 0xE7, 0x67, 0xA7, 0x27, 0xC7, 0x47, 0x87, 0x07,
	0xFB, 0x7B, 0xBB, 0x3B, 0xDB, 0x5B, 0x9B, 0x1B, 0xEB, 0x6B, 0xAB, 0x2B, 0xCB, 0x4B, 0x8B, 0x0B,
	0xF3, 0x73, 0xB3, 0x33, 0xD3, 0x53, 0x93, 0x13, 0xE3, 0x63, 0xA3, 0x23, 0xC3, 0x43, 0x83, 0x03,
	0xFD, 0x7D, 0xBD, 0x3D, 0xDD, 0x5D, 0x9D, 0x1D, 0xED, 0x6D, 0xAD, 0x2D, 0xCD, 0x4D, 0x8D, 0x0D,
	0xF5, 0x75, 0xB5, 0x35, 0xD5, 0x55, 0x95, 0x15, 0xE5, 0x65, 0xA5, 0x25, 0xC5, 0x45, 0x85, 0x05,
	0xF9, 0x79, 0xB9, 0x39, 0xD9, 0x59, 0x99, 0x19, 0xE9, 0x69, 0xA9, 0x29, 0xC9, 0x49, 0x89, 0x09,
	0xF1, 0x71, 0xB1, 0x31, 0xD1, 0x51, 0x91, 0x11, 0xE1, 0x61, 0xA1, 0x21, 0xC1, 0x41, 0x81, 0x01,
	0xFE, 0x7E, 0xBE, 0x3E, 0xDE, 0x5E, 0x9E, 0x1E, 0xEE, 0x6E, 0xAE, 0x2E, 0xCE, 0x4E, 0x8E, 0x0E,
	0xF6, 0x76, 0xB6, 0x36, 0xD6, 0x56, 0x96, 0x16, 0xE6, 0x66, 0xA6, 0x26, 0xC6, 0x46, 0x86, 0x06,
	0xFA, 0x7A, 0xBA, 0x3A, 0xDA, 0x5A, 0x9A, 0x1A, 0xEA, 0x6A, 0xAA, 0x2A, 0xCA, 0x4A, 0x8A, 0x0A,
	0xF2, 0x72, 0xB2, 0x32, 0xD2, 0x52, 0x92, 0x12, 0xE2, 0x62, 0xA2, 0x22, 0xC2, 0x42, 0x82, 0x02,
	0xFC, 0x7C, 0xBC, 0x3C, 0xDC, 0x5C, 0x9C, 0x1C, 0xEC, 0x6C, 0xAC, 0x2C, 0xCC, 0x4C, 0x8C, 0x0C,
	0xF4, 0x74, 0xB4, 0x34, 0xD4, 0x54, 0x94, 0x14, 0xE4, 0x64, 0xA4, 0x24, 0xC4, 0x44, 0x84, 0x04,
	0xF8, 0x78, 0xB8, 0x38, 0xD8, 0x58, 0x98, 0x18, 0xE8, 0x68, 0xA8, 0x28, 0xC8, 0x48, 0x88, 0x08,
	0xF0, 0x70, 0xB0, 0x30, 0xD0, 0x50, 0x90, 0x10, 0xE0, 0x60, 0xA0, 0x20, 0xC0, 0x40, 0x80, 0x00
};


SCI_ERROR sci_hw_init(SCI_CONTROL_BLOCK *sci);
static int SCI_Set_Clock(SCI_CONTROL_BLOCK *sci);
static int SCI_ClockDisable(SCI_CONTROL_BLOCK *sci);
static int detect_ATR(SCI_CONTROL_BLOCK *sci);
static void sci_cb_init(SCI_CONTROL_BLOCK *sci);

INT     debug = 0;
ULONG   sci_driver_init;
SCI_CONTROL_BLOCK   sci_cb[SCI_NUMBER_OF_CONTROLLERS];

#ifdef DEBUG
void *checked_ioremap(unsigned long offset, unsigned long size)
{
	void *rv = ioremap(offset, size);
	if (rv == NULL)
		PERROR("IOREMAP %lx failed.\n", offset);
	else
		PDEBUG("IOREMAP %lx-%lx ok.\n", offset, offset + size);
	return rv;
}
#else
#define checked_ioremap(x,y) ioremap(x,y)
#endif

/**************************************************************************
 *  Registers ops
 **************************************************************************/

/**
 * @brief  Read the value of a RO or RW register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @return The value of the register
 */

ULONG get_reg(SCI_CONTROL_BLOCK *sci, BASE_ADDR base_address, ULONG reg)
{
	ULONG reg_address = 0x0;
	ULONG map_base;
	ULONG val;
	switch (base_address)
	{
		case BASE_ADDRESS_SYSCFG:
			reg_address = (ULONG)(sci->base_address_syscfg + reg);
			break;
		case BASE_ADDRESS_PIO0:
			reg_address = (ULONG)(sci->base_address_pio0 + reg);
			break;
		case BASE_ADDRESS_PIO1:
			reg_address = (ULONG)(sci->base_address_pio1 + reg);
			break;
		case BASE_ADDRESS_PIO3:
			reg_address = (ULONG)(sci->base_address_pio3 + reg);
			break;
		case BASE_ADDRESS_PIO4:
			reg_address = (ULONG)(sci->base_address_pio4 + reg);
			break;
		case BASE_ADDRESS_ASC0:
		case BASE_ADDRESS_ASC1:
			reg_address = (ULONG)(sci->base_address_asc + reg);
			break;
		case BASE_ADDRESS_SCI0:
		case BASE_ADDRESS_SCI1:
			reg_address = (ULONG)(sci->base_address_sci + reg);
			break;
		default:
			return 0;
	}
	map_base = (ULONG)checked_ioremap(reg_address, 4);
	if (!map_base)
		return 0;
	val = ctrl_inl(map_base);
	iounmap((void *)map_base);
	return val;
}

/**
 * @brief  Write a RW register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @param  mask The mask
 * @return The value of the register
 */
void set_reg(SCI_CONTROL_BLOCK *sci, BASE_ADDR base_address, ULONG reg, UINT bits, UINT mask)
{
	ULONG reg_address = 0x0;
	ULONG map_base;
	UINT val;
	switch (base_address)
	{
		case BASE_ADDRESS_SYSCFG:
			reg_address = (ULONG)(sci->base_address_syscfg + reg);
			break;
		case BASE_ADDRESS_PIO0:
			reg_address = (ULONG)(sci->base_address_pio0 + reg);
			break;
		case BASE_ADDRESS_PIO1:
			reg_address = (ULONG)(sci->base_address_pio1 + reg);
			break;
		case BASE_ADDRESS_PIO3:
			reg_address = (ULONG)(sci->base_address_pio3 + reg);
			break;
		case BASE_ADDRESS_PIO4:
			reg_address = (ULONG)(sci->base_address_pio4 + reg);
			break;
		case BASE_ADDRESS_ASC0:
		case BASE_ADDRESS_ASC1:
			reg_address = (ULONG)(sci->base_address_asc + reg);
			break;
		case BASE_ADDRESS_SCI0:
		case BASE_ADDRESS_SCI1:
			reg_address = (ULONG)(sci->base_address_sci + reg);
			break;
		default:
			return;
	}
	map_base = (ULONG)checked_ioremap(reg_address, 4);
	if (!map_base)
		return;
	val = ctrl_inl(map_base);
	val &= ~(mask);
	val |= bits;
	ctrl_outl(val, map_base);
	iounmap((void *)map_base);
}

/**
 * @brief  Write a WO register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @return The value of the register
 */
void set_reg_writeonly(SCI_CONTROL_BLOCK *sci, BASE_ADDR base_address, ULONG reg, UINT bits)
{
	ULONG reg_address = 0x0;
	ULONG map_base;
	PDEBUG(" ...\n");
	switch (base_address)
	{
		case BASE_ADDRESS_SYSCFG:
			reg_address = (ULONG)(sci->base_address_syscfg + reg);
			break;
		case BASE_ADDRESS_PIO0:
			reg_address = (ULONG)(sci->base_address_pio0 + reg);
			break;
		case BASE_ADDRESS_PIO1:
			reg_address = (ULONG)(sci->base_address_pio1 + reg);
			break;
		case BASE_ADDRESS_PIO3:
			reg_address = (ULONG)(sci->base_address_pio3 + reg);
			break;
		case BASE_ADDRESS_PIO4:
			reg_address = (ULONG)(sci->base_address_pio4 + reg);
			break;
		case BASE_ADDRESS_ASC0:
		case BASE_ADDRESS_ASC1:
			reg_address = (ULONG)(sci->base_address_asc + reg);
			break;
		case BASE_ADDRESS_SCI0:
		case BASE_ADDRESS_SCI1:
			reg_address = (ULONG)(sci->base_address_sci + reg);
			break;
		default:
			return;
	}
	map_base = (ULONG)checked_ioremap(reg_address, 4);
	if (!map_base)
		return;
	PDEBUG("reg_address=%lx, bits=%x\n", map_base, bits);
	ctrl_outl(bits, map_base);
	iounmap((void *)map_base);
}

/**
 * @brief  Write a WO 16 bit register
 * @param  sci_id zero-based number to identify smart card controller
 * @param  base_address The port base address
 * @param  reg The port's offset/register starting from the base address
 * @return The value of the register
 */
void set_reg_writeonly16(SCI_CONTROL_BLOCK *sci, BASE_ADDR base_address, ULONG reg, USHORT bits)
{
	ULONG reg_address = 0x0;
	ULONG map_base;
	PDEBUG(" ...\n");
	switch (base_address)
	{
		case BASE_ADDRESS_SYSCFG:
			reg_address = (ULONG)(sci->base_address_syscfg + reg);
			break;
		case BASE_ADDRESS_PIO0:
			reg_address = (ULONG)(sci->base_address_pio0 + reg);
			break;
		case BASE_ADDRESS_PIO1:
			reg_address = (ULONG)(sci->base_address_pio1 + reg);
			break;
		case BASE_ADDRESS_PIO3:
			reg_address = (ULONG)(sci->base_address_pio3 + reg);
			break;
		case BASE_ADDRESS_PIO4:
			reg_address = (ULONG)(sci->base_address_pio4 + reg);
			break;
		case BASE_ADDRESS_ASC0:
		case BASE_ADDRESS_ASC1:
			reg_address = (ULONG)(sci->base_address_asc + reg);
			break;
		case BASE_ADDRESS_SCI0:
		case BASE_ADDRESS_SCI1:
			reg_address = (ULONG)(sci->base_address_sci + reg);
			break;
		default:
			return;
	}
	map_base = (ULONG)checked_ioremap(reg_address, 4);
	if (!map_base)
		return;
	PDEBUG("reg_address=%lx, bits=%x\n", map_base, bits);
	ctrl_outw(bits, map_base);
	iounmap((void *)map_base);
}

/*******************************/
/* Select the serial interrupt */
/*******************************/
static void set_serial_irq(SCI_CONTROL_BLOCK *sci, unsigned char type)
{
	if (sci->id == 0)
	{
		if (type == RX_FULL_IRQ)
		{
			sci->irq_mode = RX_FULL_IRQ;
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_INT_EN, RX_FULL_IRQ , 0x1FF);
			PDEBUG(" ### Set RX_FULL_IRQ interrupt\n");
		}
		else if (type == TX_EMPTY_IRQ)
		{
			sci->irq_mode = TX_EMPTY_IRQ;
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_INT_EN, TX_EMPTY_IRQ , 0x1FF);
			PDEBUG(" ### Set TX_EMPTY_IRQ interrupt\n");
		}
		else if (type == RX_FULL_TX_EMPTY_IRQ)
		{
			sci->irq_mode = RX_FULL_TX_EMPTY_IRQ;
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_INT_EN, RX_FULL_TX_EMPTY_IRQ , 0x1FF);
			PDEBUG(" ### Set RX_FULL_TX_EMPTY_IRQ interrupt\n");
		}
		else if (type == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ))
		{
			sci->irq_mode = (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ);
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_INT_EN, (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ) , 0x1FF);
			PDEBUG(" ### Set RX_FULL_TX_EMPTY_IRQ and TX_HALF_EMPTY_IRQ interrupt\n");
		}
		else
			PDEBUG("Type of interrupt is not implemented\n");
	}
	else if (sci->id == 1)
	{
		if (type == RX_FULL_IRQ)
		{
			sci->irq_mode = RX_FULL_IRQ;
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_INT_EN, RX_FULL_IRQ , 0x1FF);
			PDEBUG(" ### Set RX_FULL_IRQ interrupt\n");
		}
		else if (type == TX_EMPTY_IRQ)
		{
			sci->irq_mode = TX_EMPTY_IRQ;
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_INT_EN, TX_EMPTY_IRQ , 0x1FF);
			PDEBUG(" ### Set TX_EMPTY_IRQ interrupt\n");
		}
		else if (type == RX_FULL_TX_EMPTY_IRQ)
		{
			sci->irq_mode = RX_FULL_TX_EMPTY_IRQ;
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_INT_EN, RX_FULL_TX_EMPTY_IRQ , 0x1FF);
			PDEBUG(" ### Set RX_FULL_TX_EMPTY_IRQ interrupt\n");
		}
		else if (type == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ))
		{
			sci->irq_mode = (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ);
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_INT_EN, (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ) , 0x1FF);
			PDEBUG(" ### Set RX_FULL_TX_EMPTY_IRQ and TX_HALF_EMPTY_IRQ interrupt\n");
		}
		else
			PDEBUG("Type of interrupt is not implemented\n");
	}
}

/**
 * @brief  Reset the smartcard setting the signal high-low-high <br/>
 *         Between the low-high we wait 100 ms and reset the RX FIFO
 * @param  sci_id zero-based number to identify smart card controller
 */
void smartcard_reset(SCI_CONTROL_BLOCK *sci, unsigned char wait)
{
	PDEBUG(" ...\n");
	if (sci->id == 0)
	{
		disable_irq(SCI0_INT_RX_TX);
#if defined(ADB_BOX)
		/* VCC cmd low */
		stpio_set_pin(sci->cmdvcc, 0);
#else
		/* VCC cmd high */
		stpio_set_pin(sci->cmdvcc, 1);
#endif
		/* Reset low */
		stpio_set_pin(sci->reset, 0);
		//mdelay(500);
		//change to non Busy-Waiting
		msleep(500);
#if defined(ADB_BOX)
		/* VCC cmd low */
		stpio_set_pin(sci->cmdvcc, 1);
#else
		/* VCC cmd high */
		stpio_set_pin(sci->cmdvcc, 0);
#endif
	}
	else if (sci->id == 1)
	{
		disable_irq(SCI1_INT_RX_TX);
#if defined(ADB_BOX)
		/* VCC cmd low */
		stpio_set_pin(sci->cmdvcc, 0);
#else
		/* VCC cmd high */
		stpio_set_pin(sci->cmdvcc, 1);
#endif
		/* Reset low */
		stpio_set_pin(sci->reset, 0);
		//mdelay(500);
		//change to non Busy-Waiting
		msleep(500);
#if defined(ADB_BOX)
		/* VCC cmd low */
		stpio_set_pin(sci->cmdvcc, 1);
#else
		/* VCC cmd high */
		stpio_set_pin(sci->cmdvcc, 0);
#endif
	}
	else
	{
		PERROR("Invalid SC ID controller '%d'", sci->id);
		return;
	}
	//mdelay(6);
	//change to non Busy-Waiting
	msleep(6);
	sci->rx_rptr = 0;
	sci->rx_wptr = 0;
	sci->tx_rptr = 0;
	sci->tx_wptr = 0;
	/* Wait 100 ms */
	//mdelay(100);
	//change to non Busy-Waiting
	msleep(100);
	if (sci->id == 0)
	{
		/* Reset RX FIFO */
		set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_TX_RST, 0xFF);
		set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_RX_RST, 0xFF);
		set_serial_irq(sci, RX_FULL_IRQ);
		enable_irq(SCI0_INT_RX_TX);
		//mdelay(20);
		//change to non Busy-Waiting
		msleep(20);
		/* Reset high */
		stpio_set_pin(sci->reset, 1);
	}
	else if (sci->id == 1)
	{
		/* Reset RX FIFO */
		set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_TX_RST, 0xFF);
		set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_RX_RST, 0xFF);
		set_serial_irq(sci, RX_FULL_IRQ);
		enable_irq(SCI1_INT_RX_TX);
	}
	/* Reset high */
	stpio_set_pin(sci->reset, 1);
	PDEBUG(" OK\n");
}

/**
 * @brief  Set the Vcc to 3V or 5V <br/>
 * @param  sci_id zero-based number to identify smart card controller
 * @param  vcc The voltage
 * @return SCI_ERROR_OK if there was no error <br/>
 *         SCI_ERROR_VCC_INVALID if voltage is != 3|5
 *         SCI_ERROR_SCI_INVALID if invalid SC
 */
INT smartcard_voltage_config(SCI_CONTROL_BLOCK *sci, UINT vcc)
{
	PDEBUG(" ...\n");
	if (sci->id == 0)
	{
		if (vcc == SCI_VCC_3)
		{
			sci->sci_atr_class = SCI_CLASS_B;
#if !defined(SUPPORT_NO_VOLTAGE)
#if !defined(SPARK) && !defined(HL101) && !defined(ATEVIO7500) && !defined(ADB_BOX) && !defined(VITAMIN_HD5000)  // no votage control
			set_reg_writeonly(sci, BASE_ADDRESS_PIO4, PIO_CLR_P4OUT, 0x40);
#endif
#endif
		}
		else if (vcc == SCI_VCC_5)
		{
			sci->sci_atr_class = SCI_CLASS_A;
#if !defined(SUPPORT_NO_VOLTAGE)
#if !defined(SPARK) && !defined(HL101) && !defined(ATEVIO7500) && !defined(ADB_BOX) && !defined(VITAMIN_HD5000)  // no votage control
			set_reg_writeonly(sci, BASE_ADDRESS_PIO4, PIO_SET_P4OUT, 0x40);
#endif
#endif
		}
		else
		{
			PERROR("Invalid Vcc value '%d', set Vcc 5V", vcc);
			sci->sci_atr_class = SCI_CLASS_A;
#if !defined(SUPPORT_NO_VOLTAGE)
#if !defined(SPARK) && !defined(HL101) && !defined(ATEVIO7500) && !defined(ADB_BOX) // no votage control
			set_reg_writeonly(sci, BASE_ADDRESS_PIO4, PIO_SET_P4OUT, 0x40);
#endif
#endif
			return SCI_ERROR_VCC_INVALID;
		}
	}
	else if (sci->id == 1)
	{
		if (vcc == SCI_VCC_3)
		{
			sci->sci_atr_class = SCI_CLASS_B;
#if !defined(SUPPORT_NO_VOLTAGE)
#if !defined(SPARK) && !defined(HL101) && !defined(ATEVIO7500) && !defined(ADB_BOX)  // no votage control
			set_reg_writeonly(sci, BASE_ADDRESS_PIO3, PIO_CLR_P3OUT, 0x40);
#endif
#endif
		}
		else if (vcc == SCI_VCC_5)
		{
			sci->sci_atr_class = SCI_CLASS_A;
#if !defined(SUPPORT_NO_VOLTAGE)
#if !defined(SPARK) && !defined(HL101) && !defined(ATEVIO7500) && !defined(ADB_BOX)  // no votage control
			set_reg_writeonly(sci, BASE_ADDRESS_PIO3, PIO_SET_P3OUT, 0x40);
#endif
#endif
		}
		else
		{
			PERROR("Invalid Vcc value '%d', set Vcc 5V", vcc);
			sci->sci_atr_class = SCI_CLASS_A;
#if !defined(SUPPORT_NO_VOLTAGE)
#if !defined(SPARK) && !defined(HL101) && !defined(ATEVIO7500) && !defined(ADB_BOX)  // no votage control
			set_reg_writeonly(sci, BASE_ADDRESS_PIO3, PIO_CLR_P3OUT, 0x40);
#endif
#endif
			return SCI_ERROR_VCC_INVALID;
		}
	}
	else
	{
		PERROR("Invalid SC ID controller '%d'", sci->id);
		return SCI_ERROR_SCI_INVALID;
	}
	return SCI_ERROR_OK;
}

/**
 * @brief  Set the Clock to 3.579MHz or 6.25MHz <br/>
 * @param  sci_id zero-based number to identify smart card controller
 * @param  clock The clock
 * @return SCI_ERROR_OK if there was no error <br/>
 *         SCI_ERROR_VCC_INVALID if clock is != 1|2
 *         SCI_ERROR_SCI_INVALID if invalid SC
 */
static INT smartcard_clock_config(SCI_CONTROL_BLOCK *sci, UINT clock)
{
	PDEBUG(" ...\n");
	sci->clk = clock;
	if (!SCI_Set_Clock(sci))
		return SCI_ERROR_VCC_INVALID;
	//mdelay(3);
	//change to non Busy-Waiting
	msleep(3);
	return SCI_ERROR_OK;
}

/**************************************************************************
 *  IRQ handling
 **************************************************************************/
#if 0
If RX interrupt is actived....
rx_rptr -> pointer when the byte is read from the card
rx_wptr -> pointer when the byte is written to user

If TX interrupt is actived....
tx_rptr -> pointer when the byte is read from user
tx_wptr -> pointer when the byte is written to the card

#endif

/**
 * @brief Interrupt (top-half) called when we have something to read
 * @param irq The irq number
 * @param dev_id Ptr to the dev data struct
 */
/************************************/
void red_read(SCI_CONTROL_BLOCK *sci)
{
	ULONG data;
	unsigned char nv = 0;
	dprintk(8, "...\n");
	for (nv = 0; nv < 16; nv++)
	{
		if (sci->id == 0)
			data = get_reg(sci, BASE_ADDRESS_ASC0, ASC0_RX_BUF);
		else if (sci->id == 1)
			data = get_reg(sci, BASE_ADDRESS_ASC1, ASC1_RX_BUF);
		else
			continue;
		if ((sci->atr_status != SCI_ATR_READY) && (sci->byte_invert == NO_DEFINED))
		{
			if ((UCHAR)data == 0x03)
			{
				sci->byte_invert = INVERT;
			}
			else if ((UCHAR)data == 0x3B)
			{
				sci->byte_invert = UNINVERT;
			}
		}
		if (sci->byte_invert == INVERT)
			data = byte_invert[(UCHAR)data];
		sci->read_buf[sci->rx_rptr] = (UCHAR)data;
		sci->rx_rptr++;
	}
	dprintk(8, " OK\n");
}

irqreturn_t sci_irq1_rx_tx_handler(int irq, void *dev_id)
{
	ULONG res, data;
	SCI_CONTROL_BLOCK *sci = &sci_cb[1];
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	/* konfetti: disabling an irq waits for completion
	 * of all pending irq's, so its not possible in
	 * an irq handler. nevertheless this seems to be
	 * no problem in older kernel versions. But I think
	 * its not necessary to disable the irq, otherwise
	 * we should use the ctrl regs to disable it.
	 */
	disable_irq(SCI1_INT_RX_TX);
#else
	set_reg(sci, BASE_ADDRESS_ASC1, ASC1_INT_EN, 0x00 , 0x1FF);
#endif
	res = get_reg(sci, BASE_ADDRESS_ASC1, ASC1_STA);
	if ((res & RX_FULL_IRQ) && /* Rx interrupt active */((sci->irq_mode == RX_FULL_IRQ) ||
								(sci->irq_mode == RX_FULL_TX_EMPTY_IRQ) ||
								(sci->irq_mode == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ))))
	{
		do
		{
			if (res & 0x1)
			{
				/* Skip parity error check. Must not be considered when reading the ATR */
				/* Frame error not checked*/
				/* Overrun error */
				if (res & 0x20)
				{
					PERROR("Data are received when the buffer is full\n");
					red_read(sci);
					res = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
					/* se below */
					enable_irq(SCI1_INT_RX_TX);
#else
					set_reg(sci, BASE_ADDRESS_ASC1, ASC1_INT_EN, sci->irq_mode , 0x1FF);
#endif
					return IRQ_HANDLED;
				}
				else
				{
					data = get_reg(sci, BASE_ADDRESS_ASC1, ASC1_RX_BUF);
					if ((sci->atr_status != SCI_ATR_READY) && (sci->byte_invert == NO_DEFINED))
					{
						if ((UCHAR)data == 0x03)
						{
							sci->byte_invert = INVERT;
						}
						else if ((UCHAR)data == 0x3B)
						{
							sci->byte_invert = UNINVERT;
						}
					}
					if (sci->byte_invert == INVERT)
						data = byte_invert[(UCHAR)data];
					sci->read_buf[sci->rx_rptr] = (UCHAR)data;
					sci->rx_rptr++;
				}
			}
			//break;
			res = get_reg(sci, BASE_ADDRESS_ASC1, ASC1_STA);
		}
		while (res & 0x1);
		PDEBUG("Read byte in INT: 0x%02x\n", sci->read_buf[sci->rx_rptr - 1]);
	}
	if ((res & TX_EMPTY_IRQ) && ((sci->irq_mode == TX_EMPTY_IRQ) || /* Tx interrupt active */
								 (sci->irq_mode == RX_FULL_TX_EMPTY_IRQ) ||
								 (sci->irq_mode == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ))))
	{
		if ((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i = 0, size = 0;
			if ((sci->tx_rptr - sci->tx_wptr) > HW_FIFO_SIZE)
				size = HW_FIFO_SIZE;
			else
				size = (sci->tx_rptr - sci->tx_wptr);
			for (i = 0; i < size; i++)
			{
				if (sci->byte_invert == INVERT)
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_TX_BUF, (UCHAR)byte_invert[sci->write_buf[sci->tx_wptr]]);
				}
				else
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_TX_BUF, (UCHAR)sci->write_buf[sci->tx_wptr]);
				}
				sci->tx_wptr++;
				sci->rx_wptr++;
			}
			if (sci->tx_rptr == sci->tx_wptr)
				set_serial_irq(sci, RX_FULL_IRQ);
		}
	}
	else if ((res & TX_HALF_EMPTY_IRQ) && ((sci->irq_mode == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ)))) /* Tx HALF interrupt active */
	{
		if ((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i = 0, size = 0;
			if ((sci->tx_rptr - sci->tx_wptr) > (HW_FIFO_SIZE / 2))
				size = (HW_FIFO_SIZE / 2);
			else
				size = (sci->tx_rptr - sci->tx_wptr);
			for (i = 0; i < size; i++)
			{
				if (sci->byte_invert == INVERT)
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_TX_BUF, (UCHAR)byte_invert[sci->write_buf[sci->tx_wptr]]);
				}
				else
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_TX_BUF, (UCHAR)sci->write_buf[sci->tx_wptr]);
				}
				sci->tx_wptr++;
				sci->rx_wptr++;
			}
			if (sci->tx_rptr == sci->tx_wptr)
				set_serial_irq(sci, RX_FULL_IRQ);
		}
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	/* se below */
	enable_irq(SCI1_INT_RX_TX);
#else
	set_reg(sci, BASE_ADDRESS_ASC1, ASC1_INT_EN, sci->irq_mode , 0x1FF);
#endif
	return IRQ_HANDLED;
}

/**
 * @brief Interrupt (top-half) called when we have something to read
 * @param irq The irq number
 * @param dev_id Ptr to the dev data struct
 */
irqreturn_t sci_irq0_rx_tx_handler(int irq, void *dev_id)
{
	ULONG res, data;
	SCI_CONTROL_BLOCK *sci = &sci_cb[0];
	dprintk(8, "...\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	/* konfetti: disabling an irq waits for completion
	 * of all pending irq's, so its not possible in
	 * an irq handler. nevertheless this seems to be
	 * no problem in older kernel versions. But I think
	 * its not necessary to disable the irq, otherwise
	 * we should use the ctrl regs to disable it.
	 */
	disable_irq(SCI0_INT_RX_TX);
#else
	set_reg(sci, BASE_ADDRESS_ASC0, ASC0_INT_EN, 0x00, 0x1FF);
#endif
	res = get_reg(sci, BASE_ADDRESS_ASC0, ASC0_STA);
	if ((res & RX_FULL_IRQ) && /* Rx interrupt active */((sci->irq_mode == RX_FULL_IRQ) ||
								(sci->irq_mode == RX_FULL_TX_EMPTY_IRQ) ||
								(sci->irq_mode == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ))))
	{
		do
		{
			if (res & 0x1)
			{
				/* Skip parity error check. Must not be considered when reading the ATR */
				/* Frame error not checked */
				/* Overrun error */
				if (res & 0x20)
				{
					PDEBUG("Data are received when the buffer is full\n");
					red_read(sci);
					res = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
					/* se below */
					enable_irq(SCI0_INT_RX_TX);
#else
					set_reg(sci, BASE_ADDRESS_ASC0, ASC0_INT_EN, sci->irq_mode , 0x1FF);
#endif
					return IRQ_HANDLED;
				}
				else
				{
					data = get_reg(sci, BASE_ADDRESS_ASC0, ASC0_RX_BUF);
					if ((sci->atr_status != SCI_ATR_READY) && (sci->byte_invert == NO_DEFINED))
					{
						if ((UCHAR)data == 0x03)
						{
							sci->byte_invert = INVERT;
						}
						else if ((UCHAR)data == 0x3B)
						{
							sci->byte_invert = UNINVERT;
						}
					}
					if (sci->byte_invert == INVERT)
						data = byte_invert[(UCHAR)data];
					sci->read_buf[sci->rx_rptr] = (UCHAR)data;
					sci->rx_rptr++;
				}
			}
			res = get_reg(sci, BASE_ADDRESS_ASC0, ASC0_STA);
		}
		while (res & 0x1);
		PDEBUG("Read byte in INT: 0x%02x\n", sci->read_buf[sci->rx_rptr - 1]);
	}
	if ((res & TX_EMPTY_IRQ) &&  /* Tx interrupt active */((sci->irq_mode == TX_EMPTY_IRQ) ||
								 (sci->irq_mode == RX_FULL_TX_EMPTY_IRQ) ||
								 (sci->irq_mode == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ))))
	{
		if ((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i = 0, size = 0;
			if ((sci->tx_rptr - sci->tx_wptr) > HW_FIFO_SIZE)
				size = HW_FIFO_SIZE;
			else
				size = (sci->tx_rptr - sci->tx_wptr);
			for (i = 0; i < size; i++)
			{
				if (sci->byte_invert == INVERT)
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)byte_invert[sci->write_buf[sci->tx_wptr]]);
				}
				else
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)sci->write_buf[sci->tx_wptr]);
				}
				sci->tx_wptr++;
				sci->rx_wptr++;
			}
			if (sci->tx_rptr == sci->tx_wptr)
				set_serial_irq(sci, RX_FULL_IRQ);
		}
	}
	else if ((res & TX_HALF_EMPTY_IRQ) && ((sci->irq_mode == (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ)))) /* Tx HALF interrupt active */
	{
		if ((res & 0x200) != 0x200) /* TX is not FULL */
		{
			unsigned char i = 0, size = 0;
			if ((sci->tx_rptr - sci->tx_wptr) > (HW_FIFO_SIZE / 2))
				size = (HW_FIFO_SIZE / 2);
			else
				size = (sci->tx_rptr - sci->tx_wptr);
			for (i = 0; i < size; i++)
			{
				if (sci->byte_invert == INVERT)
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)byte_invert[sci->write_buf[sci->tx_wptr]]);
				}
				else
				{
					set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_TX_BUF, (UCHAR)sci->write_buf[sci->tx_wptr]);
				}
				sci->tx_wptr++;
				sci->rx_wptr++;
			}
			if (sci->tx_rptr == sci->tx_wptr)
				set_serial_irq(sci, RX_FULL_IRQ);
		}
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	/* se below */
	enable_irq(SCI0_INT_RX_TX);
#else
	set_reg(sci, BASE_ADDRESS_ASC0, ASC0_INT_EN, sci->irq_mode , 0x1FF);
#endif
	dprintk(8, " OK\n");
	return IRQ_HANDLED;
}

static void sci_detect_change(SCI_CONTROL_BLOCK *sci)
{
	/* Debounce time for the OFF pin in the TDA8024 */
	//mdelay(8);
	//change to non Busy-Waiting
	msleep(8);
	if (sci->card_detect == SCI_CARD_NOT_PRESENT)
	{
		dprintk(1, "Removing Smartcard %d!\n", sci->id);
		sci->atr_status = SCI_WITHOUT_ATR;
#if defined(ADB_BOX)
		/* VCC cmd low */
		stpio_set_pin(sci->cmdvcc, 0);
#else
		/* VCC cmd high */
		stpio_set_pin(sci->cmdvcc, 1);
#endif
		sci_hw_init(sci);
		memset(sci->read_buf, 0, SCI_BUFFER_SIZE);
		sci->rx_rptr = 0;
		sci->rx_wptr = 0;
		memset(sci->write_buf, 0, SCI_BUFFER_SIZE);
		sci->tx_rptr = 0;
		sci->tx_wptr = 0;
		/* Set the interrupt in RX_MOD */
		set_serial_irq(sci, RX_FULL_IRQ);
	}
	else
	{
		dprintk(1, "Inserting Smartcard %d!\n", sci->id);
		sci_hw_init(sci);
#if defined(ADB_BOX)
		stpio_set_pin(sci->cmdvcc, 1);
#else
		stpio_set_pin(sci->cmdvcc, 0);
#endif
	}
}

/**
 * @brief Install interrupts in both SCs for card detect and read op
 */
static int sci_irq_install(SCI_CONTROL_BLOCK *sci)
{
	INT res;
	if (sci->id == 0)
	{
		/* Set SC 0 RX buffer interrupt */
		res = request_irq(SCI0_INT_RX_TX, sci_irq0_rx_tx_handler, IRQF_DISABLED, SCI0_INT_RX_TX_NAME, NULL);
		if (res < 0)
		{
			PERROR("Could not request irq '%d' for '%s'\n",
				   SCI0_INT_RX_TX, SCI0_INT_RX_TX_NAME);
			return -1;
		}
	}
	else if (sci->id == 1)
	{
		/* Set SC 1 RX buffer interrupt */
		res = request_irq(SCI1_INT_RX_TX, sci_irq1_rx_tx_handler, IRQF_DISABLED, SCI1_INT_RX_TX_NAME, NULL);
		if (res < 0)
		{
			PERROR("Could not request irq '%d' for '%s'\n",
				   SCI1_INT_RX_TX, SCI1_INT_RX_TX_NAME);
			return -1;
		}
	}
	else
	{
		PERROR("Invalid SC ID controller '%d'", sci->id);
		return -1;
	}
	return 0;
}

/**************************************************************************
 * Init/Uninit funcs
 **************************************************************************/

/**
 * @brief Initialize and set to default values all the registers needed <br/>
 *          for the Smart Card interface controller
 * @param  sci_id zero-based number to identify smart card controller
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if the sci_id is not valid
 */

SCI_ERROR sci_hw_init(SCI_CONTROL_BLOCK *sci)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	/*Reset*/
	stpio_set_pin(sci->reset, 0);
	if (sci->id == 0)
	{
		set_reg(sci, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x2787, 0x3FFF);
		set_reg(sci, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x28B, 0xFFFF);
		set_reg(sci, BASE_ADDRESS_ASC0, ASC0_GUARDTIME, GT_DEFAULT, 0x1FF);
		smartcard_voltage_config(sci, SCI_VCC_5);
	}
	else if (sci->id == 1)
	{
		set_reg(sci, BASE_ADDRESS_ASC1, ASC1_CTRL, 0x2787, 0x3FFF);
		set_reg(sci, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x28B, 0xFFFF);
		set_reg(sci, BASE_ADDRESS_ASC1, ASC1_GUARDTIME, GT_DEFAULT, 0x1FF);
		smartcard_voltage_config(sci, SCI_VCC_5);
	}
	else
	{
		rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
	}
	smartcard_clock_config(sci, 357);
	sci_cb_init(sci);
	if (sci->id == 0)
	{
		set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_TX_RST, 0xFF);
		set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_RX_RST, 0xFF);
	}
	else if (sci->id == 1)
	{
		set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_TX_RST, 0xFF);
		set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_RX_RST, 0xFF);
	}
	return rc;
}


static void sci_cb_init(SCI_CONTROL_BLOCK *sci)
{
	PDEBUG(" ...\n");
	sci->atr_status        = SCI_WITHOUT_ATR;
	sci->waiting           = 0;
	sci->sci_modes.emv2000 = 0;
	sci->sci_modes.dma     = 0;
	sci->sci_modes.man_act = 0;
	sci->sci_modes.rw_mode = SCI_SYNC;
	sci->error             = SCI_ERROR_OK;
	/* Set default ATR parameters defined by standard */
	sci->sci_parameters.T      = SCI_ATR_T;
	sci->sci_parameters.f      = SCI_ATR_F;
	sci->sci_parameters.ETU    = SCI_ATR_ETU;
	sci->sci_parameters.WWT    = SCI_ATR_WWT;
	sci->sci_parameters.CWT    = SCI_ATR_CWT;
	sci->sci_parameters.BWT    = SCI_ATR_BWT;
	sci->sci_parameters.EGT    = SCI_ATR_EGT;
	sci->sci_parameters.check  = SCI_ATR_CHECK;
	sci->sci_parameters.P      = SCI_ATR_P;
	sci->sci_parameters.I      = SCI_ATR_I;
	sci->sci_parameters.U      = SCI_ATR_U;
	sci->sci_parameters.clock_stop_polarity = SCI_CLOCK_STOP_DISABLED;
	sci->DI     = 0;
	sci->FI     = 0;
	sci->ETU    = 0;
	sci->WWT    = 0;
	sci->CLOCK  = 0;
	sci->rx_rptr           = 0;
	sci->rx_wptr           = 0;
	sci->tx_rptr           = 0;
	sci->tx_wptr           = 0;
	sci->atr_len           = 0;
	sci->byte_invert     = NO_DEFINED;
	sci->sci_atr_class   = SCI_UNKNOWN_CLASS;
	PDEBUG(" OK\n");
}

void sci_exit(void)
{
	int i;
	SCI_CONTROL_BLOCK  *sci;
	for (i = 0; i < SCI_NUMBER_OF_CONTROLLERS; i++)
	{
		sci = &sci_cb[i];
		if (sci->polling)
		{
			sci->polling = 0;
			//kthread_stop(sci->thread);
		}
		//wait thread stop
		while (!sci->polling)
		{
			/*mdelay(1); change to non Busy-Waiting*/ msleep(1);
		}
		PDEBUG("[SCI %d] thread stopped\n", sci->id);
		if (i == 0)
			free_irq(SCI0_INT_RX_TX, NULL);
		if (i == 1)
			free_irq(SCI1_INT_RX_TX, NULL);
		SCI_ClockDisable(sci);
		//mdelay(10);
		//change to non Busy-Waiting
		msleep(10);
		if (sci->cmdvcc != NULL)
		{
#if defined(ADB_BOX)
			stpio_set_pin(sci->cmdvcc, 0); // active low
#else
			stpio_set_pin(sci->cmdvcc, 1);
#endif
			stpio_free_pin(sci->cmdvcc);
			sci->cmdvcc = NULL;
		}
		if (sci->detect != NULL)
		{
			stpio_free_pin(sci->detect);
			sci->detect = NULL;
		}
		if (sci->reset != NULL)
		{
			stpio_free_pin(sci->reset);
			sci->reset = NULL;
		}
		if (sci->clock != NULL)
		{
			stpio_free_pin(sci->clock);
			sci->clock = NULL;
		}
		if (sci->rxd != NULL)
		{
			stpio_free_pin(sci->rxd);
			sci->rxd = NULL;
		}
		if (sci->txd != NULL)
		{
			stpio_free_pin(sci->txd);
			sci->txd = NULL;
		}
	}
	sci_driver_init = 0;
}

static int SCI_SetClockSource(SCI_CONTROL_BLOCK *sci)
{
	U32 reg_address = 0;
	U32 val = 0;

	PDEBUG(" ...\n");
	/* Configure smart clock coming from smartclock generator */
#if defined(CONFIG_CPU_SUBTYPE_STB7100) \
 || defined(CONFIG_CPU_SUBTYPE_STX7100) \
 || defined(CONFIG_SH_ST_MB442) \
 || defined(CONFIG_SH_ST_MB411) \
 || defined(CONFIG_CPU_SUBTYPE_STX7111) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(UFS912) \
 || defined(SPARK)
	reg_address = (U32)checked_ioremap(SYS_CFG_BASE_ADDRESS + SYS_CFG7, 4);
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	val |= 0x1B0;
#if defined(CUBEBOX) || defined(IPBOX)
	/* configure SC0_nSETVCC: derived from SC0_DETECT input */
	val |= (1 << 7);
#else
	val &= ~(1 << 7);
#endif
	/* set polarity of SC0_nSETVCC: SC0_nSETVCC = NOT(SC0_DETECT) */
	val |= (1 << 8);
	ctrl_outl(val, reg_address);
	iounmap((void *)reg_address);
#if defined(CONFIG_CPU_SUBTYPE_STX7111) \
 || defined(UFS912) \
 || defined(SPARK) \
 || defined(HS7110) \
 || defined(HS7119) \
 || defined(HS7420) \
 || defined(HS7429) \
 || defined(HS7810A) \
 || defined(HS7819) \
 || defined(ATEMIO520) \
 || defined(ATEMIO530) \
 || defined(VITAMIN_HD5000)
	reg_address = (U32)checked_ioremap(SYS_CFG_BASE_ADDRESS + SYS_CFG5, 4);
	if (!reg_address)
		return 0;
#endif
	val = ctrl_inl(reg_address);
	ctrl_outl(val & ~(1 << 23), reg_address);
	iounmap((void *)reg_address);
#endif
#if defined(CONFIG_CPU_SUBTYPE_STX7105) || defined(ATEVIO7500)
	reg_address = (U32)checked_ioremap(EPLD_BASE_ADDRESS / +EPLD_SCREG, 4);
	if (reg_address)
	{
		val = ctrl_inl(reg_address);
		val = (val & ~0x77) | 0x77;
		ctrl_outl(val, reg_address);
		iounmap((void *)reg_address);
	}
	reg_address = (U32)checked_ioremap(SYS_CFG_BASE_ADDRESS + SYS_CFG7, 4);
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	val = (val & ~0xdf8) | 0xdf8;
#ifdef STX7105_SETPOWERLOW
	val &= ~(1 << 7);
	val &= ~(1 << 11);
	val |= (1 << 3);
	val |= (1 << 8);
#endif
	ctrl_outl(val, reg_address);
	iounmap((void *)reg_address);
	reg_address = (U32)checked_ioremap(SYS_CFG_BASE_ADDRESS + SYS_CFG19, 4);
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	val = (val & ~0xff00) | 0xff00;
	ctrl_outl(val, reg_address);
	iounmap((void *)reg_address);
	reg_address = (U32)checked_ioremap(SYS_CFG_BASE_ADDRESS + SYS_CFG20, 4);
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	val = (val & ~0xff00) | 0xff00;
	ctrl_outl(val, reg_address);
	iounmap((void *)reg_address);
#ifdef STX7105_SETPOWERLOW
	reg_address = (U32)checked_ioremap(SYS_CFG_BASE_ADDRESS + SYS_CFG7, 4);
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	ctrl_outl(val & ~(1 << 23), reg_address);
	iounmap((void *)reg_address);
#endif
#endif
	/* Configure smartcard control register */
	reg_address = (U32)checked_ioremap(sci->base_address_sci + 4, 4); // SCI_n_CLK_CTRL
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	ctrl_outl(val & (~(U32)CLOCK_SOURCE_MASK), reg_address);
	val = ctrl_inl(reg_address);
	ctrl_outl(val | CLOCK_SOURCE_GLOBAL, reg_address);
	iounmap((void *)reg_address);
	PDEBUG(" OK\n");
	return 1;
}

static int SCI_ClockEnable(SCI_CONTROL_BLOCK *sci)
{
	U32 val;
	U32 reg_address;

	PDEBUG(" ...\n");
	reg_address = (U32)checked_ioremap(sci->base_address_sci + 4, 4); // SCI_n_CLK_CTRL
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	ctrl_outl(val | ((U32)CLOCK_ENABLE_MASK), reg_address);
	iounmap((void *)reg_address);
	PDEBUG(" OK\n");
	return 1;
}

static int SCI_ClockDisable(SCI_CONTROL_BLOCK *sci)
{
	U32 val;
	U32 reg_address;

	PDEBUG(" ...\n");
	reg_address = (U32)checked_ioremap(sci->base_address_sci + 4, 4); // SCI_n_CLK_CTRL
	if (!reg_address)
		return 0;
	val = ctrl_inl(reg_address);
	ctrl_outl(val | (~(U32)CLOCK_ENABLE_MASK), reg_address);
	iounmap((void *)reg_address);
	PDEBUG(" OK\n");
	return 1;
}

static int SCI_Set_Clock(SCI_CONTROL_BLOCK *sci)
{
	U32 val;
	U32 reg_address;
	U32 clkg = (U32)SCI_CLK_GLOBAL;
	U32 clkdiv = clkg / (2 * 10000 * (sci->clk - 1)); //SCI_CLK_GLOBAL/(2*clk)

	dprintk(1, "Setting clock to: %u.%02uMhz\n", sci->clk / 100, sci->clk % 100);

	if ((clkdiv > 0) && (clkdiv < 0x1F))
		val = clkdiv;
	else
		val = 0x0e;  /* 3.578 Mhz */
	dprintk(2, "clkdiv = 0x%02X\n", (unsigned int)val);
	reg_address = (U32)checked_ioremap(sci->base_address_sci, 4); // SCI_n_CLK_VAL
	if (!reg_address)
		return 0;
	ctrl_outl(val, reg_address);
	iounmap((void *)reg_address);
	PDEBUG(" OK\n");
	return 1;
}

static int SCI_IO_init(SCI_CONTROL_BLOCK *sci)
{
	PDEBUG(" ...\n");
	sci->txd    = stpio_request_pin(sci->pio_port, 0, "sc_io",    STPIO_ALT_BIDIR);
	sci->rxd    = stpio_request_pin(sci->pio_port, 1, "sc_rxd",   STPIO_IN);
	sci->clock  = stpio_request_pin(sci->pio_port, 3, "sc_clock", STPIO_ALT_OUT);
	sci->reset  = stpio_request_pin(sci->pio_port, 4, "sc_reset", STPIO_OUT);
	sci->detect = stpio_request_pin(sci->pio_port, 7, "sc_detect", STPIO_IN);
	sci->cmdvcc = stpio_request_pin(sci->pio_port, 5, "sc_cmdvcc", STPIO_OUT); // must be after detect
	if (!SCI_SetClockSource(sci))
		return 0;
	if (!SCI_ClockEnable(sci))
		return 0;
	PDEBUG(" OK\n");
	return 1;
}

void sci_detect_handler(void *params)
{
	SCI_CONTROL_BLOCK *sci = (SCI_CONTROL_BLOCK *)params;
	unsigned int status;
	int last_status = 0;
	sci->polling = 1;
#if defined(ADB_BOX)
	/* VCC cmd low */
	stpio_set_pin(sci->cmdvcc, 0);
#else
	stpio_set_pin(sci->cmdvcc, 1);
#endif
	PDEBUG(" sci %d...\n", sci->id);
	while (sci->polling)
	{
		status = stpio_get_pin(sci->detect);
		if (status != last_status)
		{
			last_status = status;
			if (status)
				sci->card_detect = SCI_CARD_PRESENT;
			else
				sci->card_detect = SCI_CARD_NOT_PRESENT;
			sci_detect_change(sci);
			dprintk(1, "[SCI %d] card %s\n", sci->id, status ? "detected" : "ejected");
		}
		msleep(10);
	}
	PDEBUG(" sci %d stopped\n", sci->id);
	sci->polling = 1;
}

SCI_ERROR sci_init(void)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	SCI_CONTROL_BLOCK *sci;
	UCHAR i;
	/* Init and set to defaults the main sci control struct and the hw registers */
	for (i = 0; i < SCI_NUMBER_OF_CONTROLLERS; i++)
	{
		PDEBUG("Init Smartcard %d...\n", i);
		sci = &sci_cb[i];
		if (i == 0)
		{
			sci->base_address_pio0    = (ULONG)PIO0_BASE_ADDRESS;
			sci->base_address_asc     = (ULONG)ASC0_BASE_ADDRESS;
			sci->base_address_sci     = (ULONG)SCI0_BASE_ADDRESS;
			sci->base_address_pio4    = (ULONG)PIO4_BASE_ADDRESS;
			sci->pio_port = 0;
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_CTRL, 0x2787, 0x3FFF);
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x28B, 0xFFFF);
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_GUARDTIME, GT_DEFAULT, 0x1FF);
		}
		else if (i == 1)
		{
			sci->base_address_pio0    = (ULONG)PIO1_BASE_ADDRESS;
			sci->base_address_asc     = (ULONG)ASC1_BASE_ADDRESS;
			sci->base_address_sci     = (ULONG)SCI1_BASE_ADDRESS;
			sci->base_address_pio4    = (ULONG)PIO3_BASE_ADDRESS;
			sci->pio_port = 1;
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_CTRL, 0x2787, 0x3FFF);
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x28B, 0xFFFF);
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_GUARDTIME, GT_DEFAULT, 0x1FF);
		}
		sci->base_address_syscfg = (ULONG)SYS_CFG_BASE_ADDRESS;
		sci->driver_inuse = 0;
		sci->id = i;
		set_serial_irq(sci, RX_FULL_IRQ);
		if (!SCI_IO_init(sci) || (sci_irq_install(sci) < 0))
		{
			PERROR("Could not initialize hw of sc[%d]\n", i);
			sci_exit();
			return SCI_ERROR_DRIVER_NOT_INITIALIZED;
		}
		if (sci_hw_init(sci) != SCI_ERROR_OK)
		{
			PERROR("Could not initialize hw of sc[%d]\n", i);
			sci_exit();
			return SCI_ERROR_DRIVER_NOT_INITIALIZED;
		}
		sci->thread = kthread_run((int (*)(void *))sci_detect_handler, (void *)sci, "SMART/%d", i);
		if (sci->thread)
		{
			PDEBUG("create sci%d task successful...\n", i);
		}
		else
		{
			PERROR("Could not create sci%d task\n", i);
			return SCI_ERROR_DRIVER_NOT_INITIALIZED;
		}
	}
	sci_driver_init = 1;
	return (rc);
}

/**************************************************************************
 * Read/Write ops
 **************************************************************************/

/**
 * @brief  Write data to the SC
 * @param  sci_id zero-based number to identify smart card controller
 *         p_buffer input pointer to write buffer
 *         num_bytes number of bytes to write from p_buffer
 *         mode_flags flags to indicate behavior of write
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to <br/>
 *             sci_init() has been made <br/>
 *         SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or <br/>
 *             p_buffer is zero or num_bytes is zero <br/>
 *         SCI_ERROR_CARD_NOT_PRESENT: if card is not activated <br/>
 *         SCI_ERROR_NO_ATR: if there isn't an ATR
 */
#if 0
/* It isn't used ...*/
SCI_ERROR sci_write(SCI_CONTROL_BLOCK *sci,
					UCHAR *p_buffer,
					ULONG num_bytes,
					ULONG mode_flags
				   )
{
}
#endif

/**
 * @brief  Read data from the Smart Card.
 * @param  sci_id zero-based number to identify smart card controller
 * @param  p_buffer input pointer to read buffer
 * @param  num_bytes number of bytes to read into p_buffer
 * @param  p_bytes_read number of bytes actually read into p_buffer
 * @param  mode_flags flags to indicate behavior of read
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to <br/>
 *             sci_init() has been made <br/>
 *         SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or <br/>
 *             p_buffer is zero or num_bytes is zero or p_bytes_read is zero <br/>
 *         SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
 */

SCI_ERROR sci_read_buf(SCI_CONTROL_BLOCK *sci, UCHAR **p_buffer, ULONG num_bytes, ULONG *p_bytes_read, ULONG flags)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	INT real_num_bytes = 0;
	PDEBUG("sc[%d] enter: userspace wants to read %ld bytes\n", sci->id, num_bytes);
	PDEBUG("rx_rptr: 0x%02x, rx_wptr: 0x%02x\n", sci->rx_rptr, sci->rx_wptr);
	SCI_CHECK_INIT_COND(sci->id, rc);
	if (rc != SCI_ERROR_OK)
	{
		PERROR("sc[%d] error=%d\n", sci->id, rc);
	}
	else if ((p_buffer == 0) || (num_bytes == 0) || (p_bytes_read == 0))
	{
		rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
		PERROR("sc[%d] error=%d\n", sci->id, rc);
	}
	else
	{
		real_num_bytes = sci->rx_rptr - sci->rx_wptr;
		if (real_num_bytes > num_bytes)
			real_num_bytes = num_bytes;
		*p_bytes_read = real_num_bytes;
		*p_buffer = &(sci->read_buf[sci->rx_wptr]);
		sci->rx_wptr += real_num_bytes;
	}
	return (rc);
}

/*****************************************************************************
** Function:    sci_deactivate
**
** Purpose:     Initiate a deactivation (enter deac state).
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_deactivate(SCI_CONTROL_BLOCK *sci)
{
	SCI_ERROR rc = SCI_ERROR_OK;
#if 0
	PDEBUG("card[%d] enter\n", (UINT) sci_id);
	if (sci_driver_init == 1)
	{
		if (sci_id < SCI_NUMBER_OF_CONTROLLERS)
		{
			if (sci->state != SCI_STATE_DEAC)
			{
				k_state = os_enter_critical_section();
				/* abort any current transactions                         */
				/* assign abort error code if one is not already assigned */
				if (sci->waiting == 1)
				{
					sci->waiting = 0;
					wake_up_interruptible(sci_wait_q[sci_id]);
					//os_release_mutex(sci->mutex);
					sci->error = SCI_ERROR_TRANSACTION_ABORTED;
					sci->rx_complete = 1;
				}
				sci_atom_deactivate(sci_id);
				sci->state = SCI_STATE_DEAC;
				os_leave_critical_section(k_state);
			}
		}
		else
		{
			rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
		}
	}
	else
	{
		rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
	}
	if (rc != SCI_ERROR_OK)
	{
		PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
	}
	PDEBUG("card[%d] exit\n", (UINT) sci_id);
#endif
	PDEBUG("Deactivate is dummy ...\n");
	return (rc);
}


/**
 * @brief  Initiate a reset (enter atr state).
 * @param  sci_id zero-based number to identify smart card controller
 * @return SCI_ERROR_OK: if successful <br/>
 *         SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to <br/>
 *             sci_init() has been made <br/>
 *         SCI_ERROR_PARAMETER_OUT_OF_RANGE:  if sci_id is invalid <br/>
 *         SCI_ERROR_CARD_NOT_PRESENT: if no Smart Card is <br/>
 *             present in the reader
 */
SCI_ERROR sci_reset(SCI_CONTROL_BLOCK *sci)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	PDEBUG("sc[%d] enter\n", sci->id);
	SCI_CHECK_INIT_COND(sci->id, rc);
	if (rc != SCI_ERROR_OK)
	{
		PDEBUG("sc[%d] error=%d\n", sci->id, rc);
		return rc;
	}
	/* FIXME: Add wake_up_interruptible() */
#if defined(ADB_BOX) || defined(VITAMIN_HD5000)
	/* VCC cmd low  (active) */
	stpio_set_pin(sci->cmdvcc, 1);
#else
	stpio_set_pin(sci->cmdvcc, 0);
#endif
	smartcard_reset(sci, 0);
	PDEBUG("sc[%d] exit\n", sci->id);
	return (rc);
}

/**
 * @brief  Determine if a card is present in the reader
 * @param  sci_id zero-based number to identify smart card controller
 * @return 0: card is not present  <br/>
 *         1: card is present
 */
SCI_ERROR sci_is_card_present(SCI_CONTROL_BLOCK *sci)
{
	UCHAR rc;
	if (sci_driver_init == 1)
		rc = sci->card_detect;
	else
	{
		/* sc not present or an error occurred while detecting the sc */
		rc = SCI_CARD_NOT_PRESENT;
	}
	return (rc);
}

/**
 * @brief  Compatibility func. It calls sci_is_card_present()
 * @param  sci_id zero-based number to identify smart card controller
 * @return 0: card is not present <br/>
 *         1: card is present
 */
SCI_ERROR sci_is_card_activated(SCI_CONTROL_BLOCK *sci)
{
	return sci_is_card_present(sci);
}


/*****************************************************************************
** Function:    sci_clock_stop
**
** Purpose:     Stop the SCI/Smart Card clock at a given polarity.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
**              SCI_ERROR_CLOCK_STOP_DISABLED: if clock stop is disabled
*****************************************************************************/
SCI_ERROR sci_clock_stop(SCI_CONTROL_BLOCK *sci)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	PDEBUG("card[%d] enter\n", sci->id);
	if (sci_driver_init == 1)
	{
		if (sci->id < SCI_NUMBER_OF_CONTROLLERS)
		{
			if (sci_is_card_activated(sci) == 1)
			{
				/* check for clock stop enabled */
				if (sci->sci_parameters.clock_stop_polarity != SCI_CLOCK_STOP_DISABLED)
				{
					SCI_ClockDisable(sci);
				}
				else
				{
					rc = SCI_ERROR_CLOCK_STOP_DISABLED;
				}
			}
			else
			{
				rc = SCI_ERROR_CARD_NOT_ACTIVATED;
			}
		}
		else
		{
			rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
		}
	}
	else
	{
		rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
	}
	if (rc != SCI_ERROR_OK)
	{
		PDEBUG("card[%d] error=%d\n", sci->id, rc);
	}
	PDEBUG("card[%d] exit\n", sci->id);
	return (rc);
}

/*****************************************************************************
** Function:    sci_clock_start
**
** Purpose:     Start the SCI/Smart Card clock.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_clock_start(SCI_CONTROL_BLOCK *sci)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	PDEBUG("card[%d] enter\n", sci->id);
	if (sci_driver_init == 1)
	{
		if (sci->id < SCI_NUMBER_OF_CONTROLLERS)
		{
			if (sci_is_card_activated(sci) == 1)
			{
				/* start the clock */
				SCI_ClockEnable(sci);
			}
			else
			{
				rc = SCI_ERROR_CARD_NOT_ACTIVATED;
			}
		}
		else
		{
			rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
		}
	}
	else
	{
		rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
	}
	if (rc != SCI_ERROR_OK)
	{
		PDEBUG("card[%d] error=%d\n", sci->id, rc);
	}
	PDEBUG("card[%d] exit\n", sci->id);
	return (rc);
}

/*****************************************************************************
** Function:    sci_set_modes
**
** Purpose:     Set the current Smart Card driver modes.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_modes: input pointer to Smart Card modes
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_modes is zero.
*****************************************************************************/
SCI_ERROR sci_set_modes(SCI_CONTROL_BLOCK *sci, SCI_MODES *p_sci_modes)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	PDEBUG("card[%d] enter\n", sci->id);
	if (sci_driver_init == 1)
	{
		if ((p_sci_modes != 0) && (sci->id < SCI_NUMBER_OF_CONTROLLERS))
		{
			if ((p_sci_modes->emv2000 == 0) || (p_sci_modes->emv2000 == 1))
			{
				sci->sci_modes.emv2000 = p_sci_modes->emv2000;
			}
			else
			{
				rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
			}
			if ((p_sci_modes->dma == 0) || (p_sci_modes->dma == 1))
			{
				/*sci->sci_modes.dma = p_sci_modes->dma;*/
				/* not yet supported */
				if (p_sci_modes->dma == 1)
				{
					printk("DMA mode is not supported\n");
					//rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
				}
			}
			else
			{
				rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
			}
			if ((p_sci_modes->man_act == 0) || (p_sci_modes->man_act == 1))
			{
				sci->sci_modes.man_act = p_sci_modes->man_act;
			}
			else
			{
				rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
			}
			if (p_sci_modes->rw_mode < 4)
			{
				sci->sci_modes.rw_mode = p_sci_modes->rw_mode;
			}
			else
			{
				rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
			}
		}
		else
		{
			rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
		}
	}
	else
	{
		rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
	}
	if (rc != SCI_ERROR_OK)
	{
		PDEBUG("card[%d] error=%d\n", sci->id, rc);
	}
	PDEBUG("card[%d] exit\n", sci->id);
	return (rc);
}

/****************************************************************************
** Function:    sci_get_modes
**
** Purpose:     Retrieve the current Smart Card modes.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              sci_get_modes: output pointer to Smart Card modes
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_modes is zero.
*****************************************************************************/
SCI_ERROR sci_get_modes(SCI_CONTROL_BLOCK *sci, SCI_MODES *p_sci_modes)
{
	SCI_ERROR rc = SCI_ERROR_OK;
	PDEBUG("sc[%d] enter\n", sci->id);
	if (sci_driver_init == 1)
	{
		if ((p_sci_modes != 0) && (sci->id < SCI_NUMBER_OF_CONTROLLERS))
		{
			p_sci_modes->emv2000 = sci->sci_modes.emv2000;
			p_sci_modes->dma     = sci->sci_modes.dma;
			p_sci_modes->man_act = sci->sci_modes.man_act;
			p_sci_modes->rw_mode = sci->sci_modes.rw_mode;
		}
		else
		{
			rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
		}
	}
	else
	{
		rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
	}
	if (rc != SCI_ERROR_OK)
	{
		PERROR("sc[%d] error=%d\n", sci->id, rc);
	}
	PDEBUG("sc[%d] exit\n", sci->id);
	return (rc);
}

/**************************************************************************
 * Driver File Operations
 **************************************************************************/

/**
 * @brief POSIX device open
 * @param inode i_rdev contains the dev char number
 * @param filp Ptr to struct file
 * @return =0: success <br/>
 *         <0: if any error occur
 */
static int sci_open(struct inode *inode, struct file *filp)
{
	ULONG sci_id = MINOR(inode->i_rdev);
	int rc = 0;
	dprintk(1, "Opening device sci%ld!\n", sci_id);
	if (sci_id < SCI_NUMBER_OF_CONTROLLERS)
	{
		SCI_CONTROL_BLOCK *sci = &sci_cb[sci_id];
		if (sci->driver_inuse == 0)
		{
			sci->driver_inuse++;
			dprintk(2, "Smartcard[%d] char device available: %d\n", sci->id, sci->driver_inuse);
		}
		else
		{
			dprintk(2, "sc[%d] is busy\n", sci->id);
			rc = -EBUSY;
		}
	}
	else
	{
		dprintk(1, "Couldn't open device for sc[%ld]\n", sci_id);
		rc = -ENODEV;
	}
	return (rc);
}

/**
 * @brief POSIX device close
 * @param inode i_rdev contains the dev char number
 * @param filp Ptr to struct file
 * @return =0: success <br/>
 *         <0: if any error occur
 */
static int sci_close(struct inode *inode, struct file *filp)
{
	ULONG sci_id = MINOR(inode->i_rdev);
	int rc = 0;
	dprintk(1, "Closing device sci%ld!\n", sci_id);
	if (sci_id < SCI_NUMBER_OF_CONTROLLERS)
	{
		SCI_CONTROL_BLOCK *sci = &sci_cb[sci_id];
		if (sci->driver_inuse > 0)
		{
			sci->driver_inuse--;
		}
		else
		{
			dprintk(1, "sc[%d] device is not opend: %d\n", sci->id, sci->driver_inuse);
			rc = -EINVAL;
		}
#if defined(ADB_BOX)
		stpio_set_pin(sci->cmdvcc, 0); // disable TDA8024
#else
		stpio_set_pin(sci->cmdvcc, 1);
#endif
	}
	else
	{
		dprintk(1, "Couldn't close sc[%ld]\n", sci_id);
		rc = -ENODEV;
	}
	return rc;
}

/**
 * @brief  POSIX smart card read
 * @param  file input pointer to the file handle
 * @param  buffer output pointer to the output data buffer
 * @param  length the expect characters number
 * @param  offset offset of the file
 * @return >=0: the actually number of read characters <br/>
 *         <0: if any error occur
 */
static ssize_t sci_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
	ULONG sci_id;
	ULONG real_num_bytes;
	SCI_CONTROL_BLOCK *sci;

	/* sci_id is the Minor Num of this device */
	sci_id = MINOR(file->f_dentry->d_inode->i_rdev);
	sci = &sci_cb[sci_id];
	while (((sci->rx_wptr >= sci->rx_rptr) && (sci->card_detect == SCI_CARD_PRESENT)) ||
			((sci->irq_mode == RX_FULL_TX_EMPTY_IRQ) && (sci->card_detect == SCI_CARD_PRESENT)))
	{
		if ((file->f_flags & O_NONBLOCK) >> 0xB)
		{
			return -EWOULDBLOCK;
		}
		//mdelay(57);
		//change to non Busy-Waiting
		msleep(57);
	}
	if (sci->card_detect != SCI_CARD_PRESENT)
		return 0;
	real_num_bytes = sci->rx_rptr - sci->rx_wptr;
	if (!((file->f_flags & O_NONBLOCK) >> 0xB))
	{
		if (real_num_bytes < length)
		{
			unsigned char cnt_tmp = 0;
			do
			{
				//mdelay(10);
				//change to non Busy-Waiting
				msleep(10);
				cnt_tmp++;
				real_num_bytes = sci->rx_rptr - sci->rx_wptr;
			}
			while ((real_num_bytes < length) && (cnt_tmp < 100)); /* Wait a second */
		}
	}
	if (real_num_bytes > length)
		real_num_bytes = length;
	if (length > SCI_BUFFER_SIZE)
		real_num_bytes = SCI_BUFFER_SIZE;
	dprintk(7, "Request bytes: %d , Available bytes: %ld\n", length, real_num_bytes);
	copy_to_user((void *)buffer, (const void *) & (sci->read_buf[sci->rx_wptr]), real_num_bytes);
	sci->rx_wptr += real_num_bytes;
	if (sci->rx_wptr == sci->rx_rptr)
	{
		sci->rx_wptr = 0;
		sci->rx_rptr = 0;
		//mdelay(3);   /*Hellmaster1024: on Atevio we seem to have some timing probs without that delay */
		//change to non Busy-Waiting
		msleep(3);
	}
	return (ssize_t) real_num_bytes;
}

/**
 * @brief  POSIX smart card write
 * @param  file:   input pointer to the file handle
 * @param  buffer: input pointer to the input data buffer
 * @param  length: the characters number of input buffer
 * @param  offset: offset of the file
 * @return >=0: the actually number of writen characters <br/>
 *         <0: if any error occur
 */
static ssize_t sci_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
	ULONG sci_id;
	INT count = 0;
	SCI_CONTROL_BLOCK *sci;

	/* sci_id is the Minor Num of this device */
	sci_id = MINOR(file->f_dentry->d_inode->i_rdev);
	sci = &sci_cb[sci_id];
	while ((sci->tx_wptr != sci->tx_rptr) && (sci->card_detect == SCI_CARD_PRESENT))
	{
		if ((file->f_flags & O_NONBLOCK) >> 0xB)
		{
			return -EWOULDBLOCK;
		}
		//mdelay(57);
		//change to non Busy-Waiting
		msleep(57);
	}
	if (sci->card_detect != SCI_CARD_PRESENT)
		return 0;
	if (sci_id == 0)
		disable_irq(SCI0_INT_RX_TX);
	else if (sci_id == 1)
		disable_irq(SCI1_INT_RX_TX);
	if (length <= SCI_BUFFER_SIZE)
		count = length;
	else
		count = SCI_BUFFER_SIZE;
	memset(&(sci->write_buf), 0, SCI_BUFFER_SIZE);
	copy_from_user((void *) & (sci->write_buf), (const void *)buffer, count);
	if (sci_id == 0)
	{
		set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_TX_RST, 0xFF);
		set_reg_writeonly(sci, BASE_ADDRESS_ASC0, ASC0_RX_RST, 0xFF);
	}
	else if (sci_id == 1)
	{
		set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_TX_RST, 0xFF);
		set_reg_writeonly(sci, BASE_ADDRESS_ASC1, ASC1_RX_RST, 0xFF);
	}
	PDEBUG("Bytes to write: %d\n", count);
	sci->tx_rptr = count;
	sci->tx_wptr = 0;
	sci->rx_rptr = 0;
	sci->rx_wptr = 0;
	//  set_serial_irq(sci,TX_EMPTY_IRQ);
	if (count <= HW_FIFO_SIZE)
		set_serial_irq(sci, RX_FULL_TX_EMPTY_IRQ);
	else
		set_serial_irq(sci, (TX_HALF_EMPTY_IRQ | RX_FULL_TX_EMPTY_IRQ));
	if (sci_id == 0)
		enable_irq(SCI0_INT_RX_TX);
	else if (sci_id == 1)
		enable_irq(SCI1_INT_RX_TX);
	return count;
}

void parse_atr(SCI_CONTROL_BLOCK *sci)
{
	sci->FI    = GetParameterF(&sci->atr);
	sci->DI    = GetParameterD(&sci->atr);
	sci->CLOCK = GetClockRate(sci->clk);
	sci->WWT   = (sci->CLOCK * (sci->DI / 1000000) / sci->FI);
	sci->ETU   = sci->CLOCK / sci->WWT;
	dprintk(4, "FI=%ld  DI=%lu.%02lu  ETU=%ld  WWT=%ld  Clock=%ldHz\n", sci->FI,
			(sci->DI / 1000000), (sci->DI / 10000) % 100,
			sci->ETU, sci->WWT, sci->CLOCK);
}

void check_atr(SCI_CONTROL_BLOCK *sci)
{
	if ((sci->read_buf[sci->rx_wptr] == 0x3F) &&
			(sci->read_buf[sci->rx_wptr + 1] == 0xFD) &&
			(sci->read_buf[sci->rx_wptr + 2] == 0x13))
	{
		smartcard_voltage_config(sci, SCI_VCC_3);
	}
	else
	{
		smartcard_voltage_config(sci, SCI_VCC_5);
	}
	sci->atr_len = sci->rx_rptr;
	memcpy(sci->atr_buf, sci->read_buf, sci->atr_len);
	ATR_InitFromArray(&sci->atr, sci->atr_buf, sci->atr_len);
	parse_atr(sci);
}
/*****************************************************/

static int detect_ATR(SCI_CONTROL_BLOCK *sci)
{
	int rc = 0;
	sci_hw_init(sci);
	if (sci->id == 0)
	{
		set_reg(sci, BASE_ADDRESS_ASC0, ASC0_CTRL, 0, 0x1000);
		set_reg(sci, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x28B, 0xFFFF);
		set_reg(sci, BASE_ADDRESS_ASC0, ASC0_GUARDTIME, GT_DEFAULT, 0x1FF);
	}
	else if (sci->id == 1)
	{
		set_reg(sci, BASE_ADDRESS_ASC1, ASC1_CTRL, 0, 0x1000);
		set_reg(sci, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x28B, 0xFFFF);
		set_reg(sci, BASE_ADDRESS_ASC1, ASC1_GUARDTIME, GT_DEFAULT, 0x1FF);
	}
	else
	{
		return -1;
	}
	if (sci_reset(sci) == SCI_ERROR_OK)
		rc = 0;
	else
		rc = -1;
	//mdelay(1100);
	//change to non Busy-Waiting
	msleep(1100);
	// Change the clock
	if ((sci->read_buf[sci->rx_wptr] != 0x3B) && (sci->read_buf[sci->rx_wptr] != 0x3F))
	{
		memset(sci->read_buf, 0, SCI_BUFFER_SIZE);
		smartcard_clock_config(sci, 625);
		if (sci->id == 0)
			set_reg(sci, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x271, 0xFFFF);
		else if (sci->id == 1)
			set_reg(sci, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x271, 0xFFFF);
		sci->sci_parameters.ETU = 372;
		if (sci_reset(sci) == SCI_ERROR_OK)
			rc = 0;
		else
			rc = -1;
		//mdelay(750);
		//change to non Busy-Waiting
		msleep(750);
	}
	return (rc);
}
/*****************************************************/

/**
 * @brief  POSIX ioctl
 * @param inode i_rdev contains the dev char number
 * @param filp Ptr to struct file
 * @param  ioctl_num:   special operation number of this device
 * @param  ioctl_param: input/output parameters of ioctl
 * @return =0: success <br/>
 *         <0: if any error occur
 */


int sci_ioctl(struct inode *inode,
			  struct file *file,
			  unsigned int ioctl_num,    /* The number of ioctl */
			  unsigned long ioctl_param  /* The parameter to it */)
{
	ULONG sci_id;
	INT rc = -1;
	SCI_MODES sci_mode;
	SCI_PARAMETERS sci_param;
	SCI_CONTROL_BLOCK *sci;
	UINT sci_rc;

	sci_id = MINOR(inode->i_rdev);
	if (sci_id >= SCI_NUMBER_OF_CONTROLLERS)
		return -1;
	sci = &sci_cb[sci_id];
	switch (ioctl_num)
	{
		case IOCTL_SET_ONLY_RESET: // 100
			dprintk(3, "ioctl IOCTL_SET_ONLY_RESET: sci_id[%d]\n", sci->id);
			if (sci->card_detect != SCI_CARD_PRESENT)
				return -1;
			smartcard_reset(sci, 1);
			rc = 0;
			break;
		case IOCTL_SET_RESET: //1
			dprintk(1, "ioctl IOCTL_SET_RESET: sci_id[%d]\n", sci->id);
			if (sci->card_detect != SCI_CARD_PRESENT)
				return -1;
			rc = detect_ATR(sci);
			// Check the ATR and change the voltage for hw security
			check_atr(sci);
			// Set the parity
			if (sci->byte_invert == INVERT)
			{
				if (sci_id == 0)
				{
					set_reg(sci, BASE_ADDRESS_ASC0, ASC0_CTRL, PARITY_ODD, 0);
				}
				else if (sci_id == 1)
				{
					set_reg(sci, BASE_ADDRESS_ASC1, ASC1_CTRL, PARITY_ODD, 0);
				}
			}
			else if (sci->byte_invert == UNINVERT)
			{
				if (sci_id == 0)
				{
					set_reg(sci, BASE_ADDRESS_ASC0, ASC0_CTRL, 0, PARITY_ODD);
				}
				else if (sci_id == 1)
				{
					set_reg(sci, BASE_ADDRESS_ASC1, ASC1_CTRL, 0, PARITY_ODD);
				}
			}
			if (rc || !sci->WWT)
			{
#if defined(ADB_BOX)
				stpio_set_pin(sci->cmdvcc, 0);
#else
				stpio_set_pin(sci->cmdvcc, 1);
#endif
				dprintk(1, "no atr detected!\n");
			}
			break;
		case IOCTL_SET_MODES: //2
			dprintk(3, "ioctl IOCTL_SET_MODES (dummy): sci_id[%d]\n", sci->id);
			copy_from_user((void *) &sci_param,
						   (const void *) ioctl_param,
						   sizeof(SCI_MODES));
			if (sci_set_modes(sci, &sci_mode) == SCI_ERROR_OK)
			{
				rc = 0;
			}
			else
			{
				rc = -1;
			}
			rc = 0;
			break;
		case IOCTL_GET_MODES: //3
			dprintk(3, "ioctl IOCTL_GET_MODES (dummy): sci_id[%d]\n", sci->id);
			if (sci_get_modes(sci, &sci_mode) == SCI_ERROR_OK)
			{
				copy_to_user((void *) ioctl_param,
							 (const void *) &sci_mode,
							 sizeof(SCI_MODES));
				rc = 0;
			}
			else
			{
				rc = -1;
			}
			rc = 0;
			break;
		case IOCTL_SET_PARAMETERS: //4
			dprintk(1, "ioctl IOCTL_SET_PARAMETERS: sci_id[%d]\n", sci->id);
			copy_from_user((void *)&sci_param, (const void *)ioctl_param, sizeof(SCI_PARAMETERS));
			if (sci_set_parameters(sci, &sci_param) == SCI_ERROR_OK)
			{
				rc = 0;
				dprintk(1, "f is set to %lu\n", sci_param.f);
				switch (sci_param.f)
				{
					case 3:
						smartcard_clock_config(sci, 357);
						break;
					default:
						smartcard_clock_config(sci, sci_param.f * 100);
						break;
				}
			}
			else
				rc = -1;
			break;
		case IOCTL_GET_PARAMETERS: //5
			dprintk(1, "ioctl IOCTL_GET_PARAMETERS: sci_id[%d]\n", sci->id);
			if (sci_get_parameters(sci, &sci_param) == SCI_ERROR_OK)
			{
				copy_to_user((void *) ioctl_param,
							 (const void *) &sci_param,
							 sizeof(SCI_PARAMETERS));
				rc = 0;
			}
			else
			{
				rc = -1;
			}
			break;
		case IOCTL_SET_CLOCK: //13
			{
				ULONG clock;
				dprintk(1, "ioctl IOCTL_SET_CLOCK: sci_id[%d]\n", sci->id);
				copy_from_user((void *)&clock, (const void *)ioctl_param, sizeof(clock));
				if (smartcard_clock_config(sci, clock) == SCI_ERROR_OK)
				{
					rc = 0;
				}
				else
				{
					rc = -1;
				}
			}
			break;
		case IOCTL_SET_CLOCK_START: //6
			dprintk(3, "ioctl IOCTL_SET_CLOCK_START: sci_id[%d]\n", sci->id);
			if (sci_clock_start(sci) == SCI_ERROR_OK)
			{
				rc = 0;
			}
			else
			{
				rc = -1;
			}
			break;
		case IOCTL_SET_CLOCK_STOP: //7
			dprintk(3, "ioctl IOCTL_IOCTL_SET_CLOCK_STOP: sci_id[%d]\n", sci->id);
			if (sci_clock_stop(sci) == SCI_ERROR_OK)
			{
				rc = 0;
			}
			else
			{
				rc = -1;
			}
			break;
		case IOCTL_GET_IS_CARD_PRESENT: //8
			dprintk(12, "ioctl IOCTL_GET_IS_CARD_PRESENT: sci_id[%d]\n", sci->id);
			sci_rc = sci_is_card_present(sci);
			copy_to_user((void *)ioctl_param, (const void *)&sci_rc, sizeof(UINT));
			rc = 0;
			break;
		case IOCTL_GET_IS_CARD_ACTIVATED: //9
			dprintk(3, "ioctl IOCTL_GET_IS_CARD_ACTIVATED: sci_id[%d]\n", sci->id);
			sci_rc = sci_is_card_present(sci);
			copy_to_user((void *)ioctl_param, (const void *)&sci_rc, sizeof(UINT));
			rc = 0;
			break;
		case IOCTL_SET_DEACTIVATE: //10
			dprintk(3, "ioctl IOCTL_SET_DEACTIVATE (dummy): sci_id[%d]\n", sci->id);
			if (sci_deactivate(sci) == SCI_ERROR_OK)
			{
				rc = 0;
			}
			else
			{
				rc = -1;
			}
			break;
		case IOCTL_SET_ATR_READY: //11
			dprintk(1, "ioctl IOCTL_SET_ATR_READY: sci_id[%d]\n", sci->id);
			sci->atr_status = SCI_ATR_READY;
			rc = 0;
			break;
		case IOCTL_GET_ATR_STATUS: //12
			dprintk(3, "ioctl IOCTL_GET_ATR_STATUS: sci_id[%d]\n", sci->id);
			copy_to_user((void *)ioctl_param,
						 (const void *) & (sci->atr_status),
						 sizeof(SCI_ATR_STATUS));
			rc = 0;
			break;
		case IOCTL_DUMP_REGS: //20
			dprintk(3, "ioctl IOCTL_DUMP_REGS: sci_id[%d]\n", sci->id);
			rc = 0;
			break;
		default:
			dprintk(1, "error ioctl_num %d\n", ioctl_num);
			rc = -1;
	}
	if (rc != 0)
	{
		PERROR("ioctl failed\n");
	}
	return rc;
}

static unsigned int sci_poll(struct file *file, poll_table *wait)
{
	ULONG sci_id;
	SCI_CONTROL_BLOCK *sci;

	PDEBUG("POLL is called\n");
	sci_id = MINOR(file->f_dentry->d_inode->i_rdev);
	sci = &sci_cb[sci_id];
	dprintk(7, "POLL is done, ret=%d\n", (sci->rx_rptr != sci->rx_wptr));
	if (sci->rx_rptr != sci->rx_wptr)
		return (POLLIN | POLLRDNORM);
	return 0;
}

#ifdef CONFIG_PROC_FS
#define MAXBUF 256
/**
 * @brief  procfs file handler
 * @param  buffer:
 * @param  start:
 * @param  offset:
 * @param  size:
 * @param  eof:
 * @param  data:
 * @return =0: success <br/>
 *         <0: if any error occur
 */
int sci_read_proc(char *buffer, char **start, off_t offset,  int size,  int *eof, void *data)
{
	char outbuf[MAXBUF] = "[ sci driver, version 1.0 ]\n";
	int blen, ix;
	SCI_CONTROL_BLOCK *sci;
	for (ix = 0; ix < SCI_NUMBER_OF_CONTROLLERS; ix++)
	{
		sci = &sci_cb[ix];
		blen = strlen(outbuf);
		if (sci_is_card_present(sci) == (SCI_ERROR)SCI_CARD_NOT_PRESENT)
			sprintf(outbuf + blen, "sci%d: no card\n", ix);
		else
			sprintf(outbuf + blen, "sci%d: card detected\n", ix);
	}
	blen = strlen(outbuf);
	if (size < blen)
		return -EINVAL;
	/*
	 * If file position is non-zero, then assume the string has
	 * been read and indicate there is no more data to be read.
	 */
	if (offset != 0)
		return 0;
	/*
	 * We know the buffer is big enough to hold the string.
	 */
	strcpy(buffer, outbuf);
	/*
	 * Signal EOF.
	 */
	*eof = 1;
	return blen;
}
#endif

static struct class *sci_module_class = 0;
static dev_t dev;
static struct file_operations Fops =
{
	.owner   = THIS_MODULE,
	.open    = sci_open,
	.release = sci_close,
	.read    = sci_read,
	.write   = sci_write,
	.ioctl   = sci_ioctl,
	.poll    = sci_poll,
	.llseek  = NULL
};

#define SMARTCARD_VERSION       "1.0.1"

/**************************************************************************
 * Module init/exit
 **************************************************************************/

static int __init sci_module_init(void)
{
	int i;
	dev = MKDEV(MAJOR_NUM, MINOR_START);
	sci_driver_init = 0;
	if (sci_init() == SCI_ERROR_OK)
	{
		if (register_chrdev_region(dev, SCI_NUMBER_OF_CONTROLLERS, DEVICE_NAME) < 0)
		{
			printk("Couldn't register '%s' driver region\n", DEVICE_NAME);
			return -1;
		}
		cdev_init(&sci_cdev, &Fops);
		sci_cdev.owner = THIS_MODULE;
		sci_cdev.ops = &Fops;
		if (cdev_add(&sci_cdev, dev, SCI_NUMBER_OF_CONTROLLERS) < 0)
		{
			printk("Couldn't register '%s' driver\n", DEVICE_NAME);
			return -1;
		}
		printk("Registering device '%s', major '%d'\n", DEVICE_NAME, MAJOR_NUM);
		// Register with sysfs so udev can create the proper devices
		sci_module_class = class_create(THIS_MODULE, DEVICE_NAME);
		for (i = 0; i < SCI_NUMBER_OF_CONTROLLERS; i++)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30))
			device_create(sci_module_class, NULL, MKDEV(MAJOR_NUM, MINOR_START + i), NULL, "sci%d", i);
#else
			class_device_create(sci_module_class, NULL, dev, NULL, "sci%d", i);
#endif
	}
	else
	{
		printk("Couldn't register '%s' driver\n", DEVICE_NAME);
		return -1;
	}
#ifdef CONFIG_PROC_FS
	if (create_proc_read_entry(SCI_PROC_FILENAME, 0, NULL, sci_read_proc, NULL) == 0)
	{
		PERROR("Unable to register '%s' proc file\n", SCI_PROC_FILENAME);
		return -ENOMEM;
	}
#endif
	printk("Version of smartcard driver: [%s]\n", SMARTCARD_VERSION);
	return 0;
}

static void __exit sci_module_cleanup(void)
{
	int i;
#ifdef CONFIG_PROC_FS
	remove_proc_entry(SCI_PROC_FILENAME, NULL);
#endif
	sci_exit();
	printk("Unregistering device '%s', major '%d'", DEVICE_NAME, MAJOR_NUM);
	cdev_del(&sci_cdev);
	unregister_chrdev_region(dev, SCI_NUMBER_OF_CONTROLLERS);
	for (i = 0; i < SCI_NUMBER_OF_CONTROLLERS; i++)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
		device_destroy(sci_module_class, dev);
#else
		class_device_destroy(sci_module_class, dev);
#endif
	class_destroy(sci_module_class);
	printk("Smartcard Driver v.[%s] removed. \n", SMARTCARD_VERSION);
}

module_init(sci_module_init);
module_exit(sci_module_cleanup);

MODULE_VERSION(SMARTCARD_VERSION);

module_param(debug, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, "Turn on/off SmartCard debugging (default:off)");

MODULE_AUTHOR("Spider-Team");
MODULE_DESCRIPTION("SmartCard Interface driver");
MODULE_LICENSE("GPL");

