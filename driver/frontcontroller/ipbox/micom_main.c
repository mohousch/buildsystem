/*
 * micom_main.c
 *
 * (c) 2011 konfetti
 * partly copied from user space implementation from M.Majoor
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

/*
 * Cuberevo 900/9000 HD Frontcontroller
 *
 * Devices:
 *  - /dev/vfd (vfd ioctls and read/write function)
 *  - /dev/rc  (reading of key events)
 *
 */

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

#include "micom.h"
#include "micom_asc.h"

//----------------------------------------------

#define EVENT_BTN_HI                  0xe1
#define EVENT_BTN_LO                  0xe2

#define EVENT_RC                      0xe0

#define EVENT_ANSWER_GETWAKEUP_SEC       0xe3
#define EVENT_ANSWER_GETWAKEUP_MIN       0xe4
#define EVENT_ANSWER_GETWAKEUP_HOUR      0xe5
#define EVENT_ANSWER_GETWAKEUP_DAY       0xe6
#define EVENT_ANSWER_GETWAKEUP_MONTH     0xe7
#define EVENT_ANSWER_GETWAKEUP_YEAR      0xe8

#define EVENT_ANSWER_GETMICOM_DAY        0xe9
#define EVENT_ANSWER_GETMICOM_MONTH      0xea
#define EVENT_ANSWER_GETMICOM_YEAR       0xeb

#define EVENT_ANSWER_RAM                 0xec
#define EVENT_ANSWER_WAKEUP_STATUS       0xed

#define EVENT_ANSWER_UNKNOWN1            0xee
#define EVENT_ANSWER_UNKNOWN2            0xef

//----------------------------------------------
short paramDebug = 0;

static unsigned char expectEventData = 0;
static unsigned char expectEventId = 1;

#define ACK_WAIT_TIME msecs_to_jiffies(500)

#define cPackageSize             2
#define cPackageSizeMicom        6
#define cPackageSizeDateTime     12
#define cPackageSizeWakeupReason 2

#define cMinimumSize   2

#define BUFFERSIZE   512     //must be 2 ^ n
static unsigned char RCVBuffer [BUFFERSIZE];
static int           RCVBufferStart = 0, RCVBufferEnd = 0;

static unsigned char KeyBuffer [BUFFERSIZE];
static int           KeyBufferStart = 0, KeyBufferEnd = 0;

static unsigned char OutBuffer [BUFFERSIZE];
static int           OutBufferStart = 0, OutBufferEnd = 0;

static wait_queue_head_t   wq;
static wait_queue_head_t   rx_wq;
static wait_queue_head_t   ack_wq;
static int dataReady = 0;

const char *driver_version = "1.07";

//----------------------------------------------

/* queue data ->transmission is done in the irq */
void micom_putc(unsigned char data)
{
	unsigned int *ASC_X_INT_EN = (unsigned int *)(ASCXBaseAddress + ASC_INT_EN);

	OutBuffer [OutBufferStart] = data;
	OutBufferStart = (OutBufferStart + 1) % BUFFERSIZE;

	/* if irq is not enabled, enable it */
	if (!(*ASC_X_INT_EN & ASC_INT_STA_THE))
		*ASC_X_INT_EN = *ASC_X_INT_EN | ASC_INT_STA_THE;
}

//----------------------------------------------

void ack_sem_up(void)
{
	dataReady = 1;
	wake_up_interruptible(&ack_wq);
}

int ack_sem_down(void)
{
	int err = 0;

	dataReady = 0;

	err  = wait_event_interruptible_timeout(ack_wq, dataReady == 1, ACK_WAIT_TIME);
	if (err == -ERESTARTSYS)
	{
		printk("wait_event_interruptible failed\n");
		return err;
	}
	else if (err == 0)
	{
		printk("timeout waiting on ack\n");
	}
	else
		dprintk(20, "command processed - remaining jiffies %d\n", err);

	return 0;
}

EXPORT_SYMBOL(ack_sem_down);

//------------------------------------------------------------------
void getRCData(unsigned char *data, int *len)
{
	int i, j;

	*len = 0;

	while (KeyBufferStart == KeyBufferEnd)
	{
		dprintk(200, "%s %d - %d\n", __func__, KeyBufferStart, KeyBufferEnd);

		if (wait_event_interruptible(wq, KeyBufferStart != KeyBufferEnd))
		{
			printk("wait_event_interruptible failed\n");
			return;
		}
	}

	i = KeyBufferEnd % BUFFERSIZE;

	*len = cPackageSize;

	if (*len <= 0)
	{
		*len = 0;
		return;
	}

	i = 0;
	j = KeyBufferEnd;

	while (i != *len)
	{
		data[i] = KeyBuffer[j];
		j++;
		i++;

		if (j >= BUFFERSIZE)
			j = 0;

		if (j == KeyBufferStart)
		{
			break;
		}
	}

	KeyBufferEnd = (KeyBufferEnd + *len) % BUFFERSIZE;

	dprintk(150, " <len %d, data[0] 0x%x, data[1] 0x%x, End %d\n", *len, data[0], data[1], KeyBufferEnd);
}

void handleCopyData(int len)
{
	int i, j;

	unsigned char *data = kmalloc(len / 2, GFP_KERNEL);

	i = 0;
	j = (RCVBufferEnd + 1) % BUFFERSIZE;

	while (i != len / 2)
	{
		dprintk(50, "%d. = 0x%02x\n", j,  RCVBuffer[j]);
		data[i] = RCVBuffer[j];
		j += 2; //filter answer tag
		j %= BUFFERSIZE;
		i++;

		if (j == RCVBufferStart)
		{
			break;
		}
	}

	copyData(data, len / 2);

	kfree(data);
}

int getLen(void)
{
	int len;

	len = RCVBufferStart - RCVBufferEnd;

	if (len < 0)
	{
		len += BUFFERSIZE;
	}

	return len;
}

void dumpData(void)
{
	int i, j, len;

	if (paramDebug < 150)
		return;

	len = getLen();

	if (len == 0)
		return;

	i = RCVBufferEnd;
	for (j = 0; j < len; j++)
	{
		printk("0x%02x ", RCVBuffer[i]);

		i++;

		if (i >= BUFFERSIZE)
		{
			i = 0;
		}

		if (i == RCVBufferStart)
		{
			i = -1;
			break;
		}
	}
	printk("\n");
}

void dumpValues(void)
{
	dprintk(150, "BuffersStart %d, BufferEnd %d, len %d\n", RCVBufferStart, RCVBufferEnd, getLen());

	if (RCVBufferStart != RCVBufferEnd)
		if (paramDebug >= 50)
			dumpData();
}

static void processResponse(void)
{
	int len, i;

	dumpData();

	if (expectEventId)
	{
		expectEventData = RCVBuffer[RCVBufferEnd];
		expectEventId = 0;
	}

	len = getLen();

	dprintk(100, "event 0x%02x %d %d %d\n", expectEventData, RCVBufferStart, RCVBufferEnd, len);

	if (expectEventData)
	{
		switch (expectEventData)
		{
			case EVENT_BTN_HI:
			case EVENT_BTN_LO:
			{
				if (len == 0)
					goto out_switch;

				if (len < cPackageSize)
					goto out_switch;

				dprintk(1, "EVENT_BTN complete\n");

				if (paramDebug >= 50)
					dumpData();

				/* copy data */
				for (i = 0; i < cPackageSize; i++)
				{
					int from, to;

					from = (RCVBufferEnd + i) % BUFFERSIZE;
					to = KeyBufferStart % BUFFERSIZE;

					KeyBuffer[to] = RCVBuffer[from];

					KeyBufferStart = (KeyBufferStart + 1) % BUFFERSIZE;
				}

				wake_up_interruptible(&wq);

				RCVBufferEnd = (RCVBufferEnd + cPackageSize) % BUFFERSIZE;
			}
			break;
			case EVENT_RC:
			{
				if (len == 0)
					goto out_switch;

				if (len < cPackageSize)
					goto out_switch;

				dprintk(1, "EVENT_RC complete %d %d\n", RCVBufferStart, RCVBufferEnd);

				if (paramDebug >= 50)
					dumpData();

				/* copy data */
				for (i = 0; i < cPackageSize; i++)
				{
					int from, to;

					from = (RCVBufferEnd + i) % BUFFERSIZE;
					to = KeyBufferStart % BUFFERSIZE;

					KeyBuffer[to] = RCVBuffer[from];

					KeyBufferStart = (KeyBufferStart + 1) % BUFFERSIZE;
				}

				wake_up_interruptible(&wq);

				RCVBufferEnd = (RCVBufferEnd + cPackageSize) % BUFFERSIZE;
			}
			break;
			case EVENT_ANSWER_GETMICOM_DAY:
			case EVENT_ANSWER_GETMICOM_MONTH:
			case EVENT_ANSWER_GETMICOM_YEAR:
				if (len == 0)
					goto out_switch;

				if (len < cPackageSizeMicom)
					goto out_switch;

				handleCopyData(len);

				dprintk(1, "Pos. response received (0x%0x)\n", expectEventData);
				errorOccured = 0;
				ack_sem_up();

				RCVBufferEnd = (RCVBufferEnd + cPackageSizeMicom) % BUFFERSIZE;

				break;
			case EVENT_ANSWER_GETWAKEUP_SEC:
			case EVENT_ANSWER_GETWAKEUP_MIN:
			case EVENT_ANSWER_GETWAKEUP_HOUR:
			case EVENT_ANSWER_GETWAKEUP_DAY:
			case EVENT_ANSWER_GETWAKEUP_MONTH:
			case EVENT_ANSWER_GETWAKEUP_YEAR:
				if (len == 0)
					goto out_switch;

				if (len < cPackageSizeDateTime)
					goto out_switch;

				handleCopyData(len);

				dprintk(1, "Pos. response received (0x%0x)\n", expectEventData);
				errorOccured = 0;
				ack_sem_up();

				RCVBufferEnd = (RCVBufferEnd + cPackageSizeDateTime) % BUFFERSIZE;

				break;
			case EVENT_ANSWER_RAM:
			case EVENT_ANSWER_UNKNOWN1:
			case EVENT_ANSWER_UNKNOWN2:
				if (len == 0)
					goto out_switch;

				if (len < cPackageSize)
					goto out_switch;

				handleCopyData(len);

				dprintk(1, "Pos. response received (0x%0x)\n", expectEventData);
				errorOccured = 0;
				ack_sem_up();

				RCVBufferEnd = (RCVBufferEnd + cPackageSize) % BUFFERSIZE;

				break;
			case EVENT_ANSWER_WAKEUP_STATUS:
				if (len == 0)
					goto out_switch;

				if (len < cPackageSizeWakeupReason)
					goto out_switch;

				handleCopyData(len);

				dprintk(1, "Pos. response received (0x%0x)\n", expectEventData);
				errorOccured = 0;
				ack_sem_up();

				RCVBufferEnd = (RCVBufferEnd + cPackageSizeWakeupReason) % BUFFERSIZE;

				break;
			default: // Ignore Response
				dprintk(1, "Invalid Response %02x\n", expectEventData);
				dprintk(1, "start %d end %d\n",  RCVBufferStart,  RCVBufferEnd);
				dumpData();

				/* discard all data, because this happens currently
				 * sometimes. dont know the problem here.
				 */
				RCVBufferEnd = RCVBufferStart;
				break;
		}
	}
out_switch:
	expectEventId = 1;
	expectEventData = 0;
}

static irqreturn_t FP_interrupt(int irq, void *dev_id)
{
	unsigned int *ASC_X_INT_STA = (unsigned int *)(ASCXBaseAddress + ASC_INT_STA);
	unsigned int *ASC_X_INT_EN = (unsigned int *)(ASCXBaseAddress + ASC_INT_EN);
	char         *ASC_X_RX_BUFF = (char *)(ASCXBaseAddress + ASC_RX_BUFF);
	char         *ASC_X_TX_BUFF = (char *)(ASCXBaseAddress + ASC_TX_BUFF);
	int          dataArrived = 0;

	if (paramDebug > 100)
		printk("i - ");

	while (*ASC_X_INT_STA & ASC_INT_STA_RBF)
	{
		RCVBuffer [RCVBufferStart] = *ASC_X_RX_BUFF;
		RCVBufferStart = (RCVBufferStart + 1) % BUFFERSIZE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		// We are to fast, lets make a break
		udelay(0);
#endif

		dataArrived = 1;

		if (RCVBufferStart == RCVBufferEnd)
		{
			printk("FP: RCV buffer overflow!!! (%d - %d)\n", RCVBufferStart, RCVBufferEnd);
		}

	}

	if (dataArrived)
	{
		wake_up_interruptible(&rx_wq);
	}

	while ((*ASC_X_INT_STA & ASC_INT_STA_THE) &&
			(*ASC_X_INT_EN & ASC_INT_STA_THE) &&
			(OutBufferStart != OutBufferEnd))
	{
		*ASC_X_TX_BUFF = OutBuffer[OutBufferEnd];
		OutBufferEnd = (OutBufferEnd + 1) % BUFFERSIZE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		// We are to fast, lets make a break
		udelay(0);
#endif
	}

	/* if all the data is transmitted disable irq, otherwise
	 * system is overflowed with irq's
	 */
	if (OutBufferStart == OutBufferEnd)
		if (*ASC_X_INT_EN & ASC_INT_STA_THE)
			*ASC_X_INT_EN &= ~ASC_INT_STA_THE;

	return IRQ_HANDLED;
}

int micomTask(void *dummy)
{
	daemonize("micomTask");

	allow_signal(SIGTERM);

	while (1)
	{
		int dataAvailable = 0;

		if (wait_event_interruptible(rx_wq, (RCVBufferStart != RCVBufferEnd)))
		{
			printk("wait_event_interruptible failed\n");
			continue;
		}

		if (RCVBufferStart != RCVBufferEnd)
			dataAvailable = 1;

		while (dataAvailable)
		{
			processResponse();

			if (RCVBufferStart == RCVBufferEnd)
				dataAvailable = 0;

			dprintk(150, "start %d end %d\n",  RCVBufferStart,  RCVBufferEnd);

		}
	}

	printk("micomTask died!\n");

	return 0;
}

//----------------------------------------------

static int __init micom_init_module(void)
{
	int i = 0;

	// Address for Interrupt enable/disable
	unsigned int         *ASC_X_INT_EN     = (unsigned int *)(ASCXBaseAddress + ASC_INT_EN);
	// Address for FiFo enable/disable
	unsigned int         *ASC_X_CTRL       = (unsigned int *)(ASCXBaseAddress + ASC_CTRL);

	dprintk(5, "%s >\n", __func__);

	//Disable all ASC 2 interrupts
	*ASC_X_INT_EN = *ASC_X_INT_EN & ~0x000001ff;

	serial_init();

	init_waitqueue_head(&wq);
	init_waitqueue_head(&rx_wq);
	init_waitqueue_head(&ack_wq);

	for (i = 0; i < LASTMINOR; i++)
		sema_init(&FrontPanelOpen[i].sem, 1);

	kernel_thread(micomTask, NULL, 0);

	//Enable the FIFO
	*ASC_X_CTRL = *ASC_X_CTRL | ASC_CTRL_FIFO_EN;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
	i = request_irq(InterruptLine, (void *)FP_interrupt, IRQF_DISABLED, "FP_serial", NULL);
#else
	i = request_irq(InterruptLine, (void *)FP_interrupt, SA_INTERRUPT, "FP_serial", NULL);
#endif

	if (!i)
		*ASC_X_INT_EN = *ASC_X_INT_EN | ASC_INT_STA_RBF;
	else printk("FP: Can't get irq\n");

	msleep(1000);
	micom_init_func();

	if (register_chrdev(VFD_MAJOR, "VFD", &vfd_fops))
		printk("unable to get major %d for VFD/MICOM\n", VFD_MAJOR);

	dprintk(10, "%s <\n", __func__);

	return 0;
}


static void __exit micom_cleanup_module(void)
{
	printk("MICOM frontcontroller module unloading\n");

	unregister_chrdev(VFD_MAJOR, "VFD");

	free_irq(InterruptLine, NULL);
}


//----------------------------------------------

module_init(micom_init_module);
module_exit(micom_cleanup_module);

#if defined(CUBEREVO_MINI)
MODULE_DESCRIPTION("MICOM frontcontroller module (CUBEREVO_MINI)");
#elif defined(CUBEREVO_MINI2)
MODULE_DESCRIPTION("MICOM frontcontroller module (CUBEREVO_MINI2)");
#elif defined(CUBEREVO_250HD)
MODULE_DESCRIPTION("MICOM frontcontroller module (CUBEREVO_250HD)");
#elif defined(CUBEREVO)
MODULE_DESCRIPTION("MICOM frontcontroller module (CUBEREVO)");
#elif defined(CUBEREVO_2000HD)
MODULE_DESCRIPTION("MICOM frontcontroller module (CUBEREVO_2000HD)");
#elif defined(CUBEREVO_3000HD)
MODULE_DESCRIPTION("MICOM frontcontroller module (CUBEREVO_3000HD)");
#else
MODULE_DESCRIPTION("MICOM frontcontroller module (UNKNOWN)");
#endif

MODULE_AUTHOR("Konfetti");
MODULE_LICENSE("GPL");

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");
