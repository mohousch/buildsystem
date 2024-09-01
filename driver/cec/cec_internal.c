/*
 *
 * (c) 2010 konfetti, schischu
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
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/version.h>

#include <linux/platform_device.h>

#include <asm/system.h>
#include <asm/io.h>

#if defined (CONFIG_KERNELVERSION) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#include "cec_internal.h"
#include "cec_debug.h"

//----
#define PIO1BaseAddress       0xFD021000
#define CECBaseAddress        0xFE030C00
#define SysConfigBaseAddress  0xFE001000

#define PIO_PC0                0x24
#define PIO_PC1                0x34
#define PIO_PC2                0x44

#define SYS_CFG2               0x108
#define SYS_CFG5               0x114

#define CEC_PRESCALER_LEFT     0x00
#define CEC_PRESCALER_RIGHT    0x04
#define CEC_STATUS_ERROR       0x08
#define CEC_CONTROL            0x0C
#define CEC_TX                 0x10
#define CEC_RX                 0x14
#define CEC_CFG                0x18
#define CEC_ADDRESS            0x1C

#define CEC_TRANSM_SOM            1 /* start of message */
#define CEC_TRANSM_EOM            2 /* end of message */
#define CEC_TRANSMIT_ERROR        4
#define CEC_TRANSMITTING          8
#define CEC_RECEIVE_SOM          16
#define CEC_RECEIVE_EOM          32
#define CEC_RECEIVE_ERROR        64
#define CEC_RECEIVING           128


/* *************************** */
/* map and write to a register */
/* *************************** */

void cec_write_register_u32(unsigned long address, u32 value)
{
	unsigned long mapped_register = (unsigned long) ioremap(address, 4);

	writel(value, mapped_register);

	iounmap((void *) mapped_register);
}

u32 cec_read_register_u32(unsigned long address)
{
	u32 result;

	unsigned long mapped_register = (unsigned long) ioremap(address, 4);

	result = readl(mapped_register);

	iounmap((void *) mapped_register);

	return result;
}

/* ***************************** */
/* write data to cec             */
/* ***************************** */

void cec_write_data(u32 value)
{
	cec_write_register_u32(CECBaseAddress + CEC_TX, value);
}

u32 cec_read_data(void)
{
	return cec_read_register_u32(CECBaseAddress + CEC_RX);
}

void cec_start_sending(unsigned char isPing)
{
	dprintk(2, "start_sending %d\n", isPing);
	if (isPing == 1)
	{
		cec_write_register_u32(CECBaseAddress + CEC_CONTROL, CEC_TRANSM_EOM | CEC_TRANSM_SOM);
	}
	else
	{
		cec_write_register_u32(CECBaseAddress + CEC_CONTROL, CEC_TRANSM_SOM);
	}
}

void cec_end_sending(void)
{
	dprintk(2, "end_sending\n");
	cec_write_register_u32(CECBaseAddress + CEC_CONTROL, CEC_TRANSM_EOM);
}

u8 cec_get_status(void)
{
	u32 res = cec_read_register_u32(CECBaseAddress + CEC_CONTROL);
	return res & 0xFF;
}

u8 cec_get_error(void)
{
	u32 res = cec_read_register_u32(CECBaseAddress + CEC_STATUS_ERROR);
	return res & 0xFF;
}

void cec_acknowledge(void)
{
	dprintk(3, "ack\n");
	cec_write_register_u32(CECBaseAddress + CEC_CONTROL, 0x00);
}

void cec_acknowledge_eom(void)
{
	dprintk(3, "ack eom\n");
	cec_write_register_u32(CECBaseAddress + CEC_CONTROL, 0x02);
}

void cec_set_own_address(u32 own_address)
{
	cec_write_register_u32(CECBaseAddress + CEC_ADDRESS, own_address);
}

//------------------------------

void str_status(unsigned char status)
{
	dprintk(2, "Control Status:\n");
	if (status & CEC_STATUS_RECV_BTF)
		dprintk(2, "\tRECV_BTF\n");
	if (status & CEC_STATUS_RECV_ERR)
		dprintk(2, "\tRECV_ERR\n");
	if (status & CEC_STATUS_RECV_EOMSG)
		dprintk(2, "\tRECV_EOMSG\n");
	if (status & CEC_STATUS_RECV_SOMSG)
		dprintk(2, "\tRECV_SOMSG\n");
	if (status & CEC_STATUS_SEND_BTF)
		dprintk(2, "\tSEND_BTF\n");
	if (status & CEC_STATUS_SEND_ERR)
		dprintk(2, "\tSEND_ERR\n");
	if (status & CEC_STATUS_SEND_EOMSG)
		dprintk(2, "\tSEND_EOMSG\n");
	if (status & CEC_STATUS_SEND_SOMSG)
		dprintk(2, "\tSEND_SOMSG\n");
}

void str_error(unsigned char error)
{
	dprintk(2, "Error Status:\n");
	if (error & CEC_ERROR_SEND_BTF)
		dprintk(2, "\tSEND_BTF\n");
	if (error & CEC_ERROR_ON_LINE)
		dprintk(2, "\tON_LINE - Collision\n");
	if (error & CEC_ERROR_ACK)
		dprintk(2, "\tACK - No one answered\n");
	if (error & CEC_ERROR_START)
		dprintk(2, "\tSTART\n");
	if (error & CEC_ERROR_RECV_BTF)
		dprintk(2, "\tRECV_BTF\n");
	if (error & CEC_ERROR_PERIOD)
		dprintk(2, "\tPERIOD\n");
	if (error & CEC_ERROR_TIMING)
		dprintk(2, "\tTIMING\n");
}

int cec_internal_init(void)
{
	u32 res = 0;
	/* *********** */
	/* basic setup */

	// The org firmware sets the following register

#ifdef STx7105
	res = cec_read_register_u32(SysConfigBaseAddress + SYS_CFG2);
	/* hdmi cec rx enable */
	res |= (1 << (29));
	cec_write_register_u32(SysConfigBaseAddress + SYS_CFG2, res);
#endif
#ifdef STx7111
	/* pio 1.7 open drain */
	cec_write_register_u32(PIO1BaseAddress + PIO_PC0, 128);
	cec_write_register_u32(PIO1BaseAddress + PIO_PC1, 128);
	cec_write_register_u32(PIO1BaseAddress + PIO_PC2, 128);

	res = cec_read_register_u32(SysConfigBaseAddress + SYS_CFG5);
	/* pio 1.7 pad */
	res |= (1 << (8));
	cec_write_register_u32(SysConfigBaseAddress + SYS_CFG5, res);

	res = cec_read_register_u32(SysConfigBaseAddress + SYS_CFG5);
	/* pio 1.7 alt */
	res |= (1 << (24));
	cec_write_register_u32(SysConfigBaseAddress + SYS_CFG5, res);
#endif


	res = cec_read_register_u32(CECBaseAddress + CEC_CFG);
	/* enable */
	res |= 0x3;
	cec_write_register_u32(CECBaseAddress + CEC_CFG, res);

	/* prescaler */
	cec_write_register_u32(CECBaseAddress + CEC_PRESCALER_LEFT, 0x88);
	cec_write_register_u32(CECBaseAddress + CEC_PRESCALER_RIGHT, 0x13);

	return 0;
}

void cec_internal_exit(void)
{
	u32 res = 0;
#ifdef STx7105
	/* hdmi cec rx disable */
	res = cec_read_register_u32(SysConfigBaseAddress + SYS_CFG2);
	res &= ~(1 << (29));
	cec_write_register_u32(SysConfigBaseAddress + SYS_CFG2, res);
#endif
#ifdef STx7111
	/* pio 1.7 close drain */
	res = cec_read_register_u32(SysConfigBaseAddress + SYS_CFG5);
	res &= ~(256);
	cec_write_register_u32(SysConfigBaseAddress + SYS_CFG5, res);
#endif

	res = cec_read_register_u32(CECBaseAddress + CEC_CFG);
	/* disable */
	res &= (u32)~ 0x03;
	cec_write_register_u32(CECBaseAddress + CEC_CFG, res);

	cec_write_register_u32(CECBaseAddress + CEC_PRESCALER_LEFT,  0x0);
	cec_write_register_u32(CECBaseAddress + CEC_PRESCALER_RIGHT, 0x0);

}
