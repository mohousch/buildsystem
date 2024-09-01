/*
 * nuvoton_asc.c
 *
 * (c) 2009 Dagobert@teamducktales
 * (c) 2010 Schischu & konfetti: Add irq handling
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
 ****************************************************************************************
 *
 * Changes
 *
 * Date     By              Description
 * --------------------------------------------------------------------------------------
 * 20140416 Audioniek       Added HS7119 and HS7819
 * 20140920 Audioniek       Corrected HS7119 ASC address and IRQ
 * 20151231 Audioniek       Added HS7420 and HS7429
 *
 ****************************************************************************************/

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>

#include "nuvoton.h"
#include "nuvoton_asc.h"

//-------------------------------------

#if defined(ATEVIO7500)
unsigned int InterruptLine = 120;
unsigned int ASCXBaseAddress = ASC3BaseAddress;
#elif defined(HS7110) \
   || defined(HS7420) \
   || defined(HS7429) \
   || defined(HS7810A) \
   || defined(HS7819)
unsigned int InterruptLine = 274;
unsigned int ASCXBaseAddress = ASC3BaseAddress;
#elif defined(HS7119)
unsigned int InterruptLine = 122;
unsigned int ASCXBaseAddress = ASC1BaseAddress;
#else
unsigned int InterruptLine = 121;
unsigned int ASCXBaseAddress = ASC2BaseAddress;
#endif

//-------------------------------------

void serial_init(void)
{
#if defined(OCTAGON1008) //|| defined(HS7420) || defined(HS7429)
	/* Configure the PIO pins */
	stpio_request_pin(4, 3,  "ASC_TX", STPIO_ALT_OUT); /* Tx */
	stpio_request_pin(4, 2,  "ASC_RX", STPIO_IN);      /* Rx */
#endif

	// Configure the asc input/output settings
	*(unsigned int *)(ASCXBaseAddress + ASC_INT_EN)   = 0x00000000; // TODO: Why do we set here the INT_EN again ???
	*(unsigned int *)(ASCXBaseAddress + ASC_CTRL)     = 0x00001589;
	*(unsigned int *)(ASCXBaseAddress + ASC_TIMEOUT)  = 0x00000010;
	*(unsigned int *)(ASCXBaseAddress + ASC_BAUDRATE) = 0x000000c9;
	*(unsigned int *)(ASCXBaseAddress + ASC_TX_RST)   = 0;
	*(unsigned int *)(ASCXBaseAddress + ASC_RX_RST)   = 0;
}

int serial_putc(char Data)
{
	char          *ASCn_TX_BUFF = (char *)(ASCXBaseAddress + ASC_TX_BUFF);
	unsigned int  *ASCn_INT_STA = (unsigned int *)(ASCXBaseAddress + ASC_INT_STA);
	unsigned long Counter = 200000;

	while (((*ASCn_INT_STA & ASC_INT_STA_THE) == 0) && --Counter)
	{
		mdelay(1);
	}

	if (Counter == 0)
	{
		dprintk(1, "Error writing char\n");
	}

	*ASCn_TX_BUFF = Data;
	return 1;
}

