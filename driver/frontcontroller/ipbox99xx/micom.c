/*
 * micom.c, (c) 2010 Sisyfos
 *
 * Original code from:
 * Kathrein UFS922 MICOM Kernelmodule ported from MARUSYS uboot source,
 * from vfd driver and from tf7700 frontpanel handling.
 *
 * Devices:
 *	- /dev/dbox/vfd (vfd ioctls and read/write function)
 *	- /dev/dbox/rc  (reading of key events)
 *
 * GPL
 *
 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>

//#define INIT_EFECT
//#define DEBUG_DEV

#include "micom.h"
#include "vfd_translate.h"

#define MICOM_MODULE_VERSION   103 // 100 = 1.00

int debug = 0;
#define dprintk(x...) do { if (debug) printk(KERN_WARNING x); } while (0)

#define GREETS " Sisyfos band "
#define EMPTY  "                   "
#define BYES   "    bye bye        "

static int leaving = 0;
static struct input_dev *fp_button_dev;

#define FP_KEYS_MAX	8
#define FP_KEY_LEN	5

struct fp_keys_s
{
	unsigned char raw[FP_KEY_LEN];
	int key;
	char name[16];
} fp_keys[FP_KEYS_MAX] =
{
	{ "\x01\x05\x04\xFB\xFB", KEY_DOWN, "KEY DOWN" },
	{ "\x01\x05\x08\xF7\xFB", KEY_UP, "KEY UP" },
	{ "\x01\x05\x02\xFD\xFB", KEY_RIGHT, "KEY RIGHT" },
	{ "\x01\x05\x10\xEF\xFB", KEY_LEFT, "KEY LEFT" },
	{ "\x01\x05\x01\xFE\xFB", KEY_POWER, "KEY POWER" },
	{ "\x01\x05\x20\xDF\xFB", KEY_ESC, "KEY ESC" },
	{ "\x01\x05\x40\xBF\xFB", KEY_OK, "KEY OK" },
	{ "\x01\x05\x80\x7F\xFB", KEY_MENU, "KEY MENU" },
};

#define FIXME_FAKED_FOR_NOW(cmd, descr) do { printk("FP: WARN: IOCTL 0x%x (%s) not yet implemented\n", cmd, descr); } while(0)

/* structure to queue transmit data is necessary because
 * after most transmissions we need to wait for an acknowledge
 */

struct transmit_s
{
	char 		command;
	unsigned char 	buffer[VFD_MAX_DATA_LEN]; // w/o header & footer
	int		len;
	int  		needAck; /* should we increase ackCounter? */
	int		requeueCount;
	int		isGetter;
};

#define cMaxTransQueue	100

/* I make this static, I think if there are more
 * then 100 commands to transmit something went
 * wrong ... point of no return
 */
struct transmit_s transmit[cMaxTransQueue];

static int transmitCount = 0;

struct receive_s
{
	unsigned char buffer[VFD_MAX_DATA_LEN];
};

#define cMaxReceiveQueue	100

struct receive_s receive[cMaxReceiveQueue];

static int receiveCount = 0;

static wait_queue_head_t   wq;
static wait_queue_head_t   task_wq;
static wait_queue_head_t   ioctl_wq;

#define cMaxAckAttempts	150
#define cMaxQueueCount	5

/* waiting retry counter, to stop waiting on ack */
static int 		   waitAckCounter = 0;

static int 		   timeoutOccured = 0;
static int 		   dataReady = 0;

struct semaphore 	   write_sem;
struct semaphore 	   rx_int_sem;
struct semaphore 	   transmit_sem;
struct semaphore 	   receive_sem;

struct saved_data_s
{
	int   length;
	char  data[VFD_MAX_DATA_LEN];
};

static struct saved_data_s lastdata;

/* last received ioctl command. we dont queue answers
 * from "getter" requests to the fp. they are protected
 * by a semaphore and the threads goes to sleep until
 * the answer has been received or a timeout occurs.
 */
static char ioctl_data[8];

#define BUFFERSIZE                256     //must be 2 ^ n

int writePosition = 0;
int readPosition = 0;
unsigned char receivedData[BUFFERSIZE];

/* konfetti: quick and dirty open handling */
typedef struct
{
	struct file 		*fp;
	int			read;
	struct semaphore 	sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC             1
#define LASTMINOR                 	2

static tFrontPanelOpen FrontPanelOpen [LASTMINOR];

enum
{
	cStateIdle,
	cStateTransmission,
	cStateWaitAck,
	cStateWaitDataAck,
	cStateWaitEvent,
	cStateWaitStartOfAnswer
};

static int state = cStateIdle;

enum
{
	ICON_STANDBY = 0x40,	// grid  1
	ICON_PLAY,		// grid  2
	ICON_PAUSE,		// grid  3
	ICON_RECORD,		// grid  4
	ICON_DOLBY,		// grid  5
	ICON_HD,
	ICON_480P,
	ICON_480I,
	ICON_720P,
	ICON_1080I,
	ICON_1080P,
	ICON_576I,
	ICON_576P,
	ICON_MP3,		// grid 14

	ICON_MIN = ICON_STANDBY,
	ICON_MAX = ICON_MP3,
};

// Micom commands
#define MICOM_ANSWER_KEY			0x01 // len=5 (MICOM->STB) vfd front key. When you press the front key, then micom send to stb this cmd with key data.
#define MICOM_ANSWER_PING			0x02 // len=3 (MICOM->STB) ping cmd. If ping is valid, you can get this cmd. You can reference this function name as “ping_process”
#define MICOM_ANSWER_CHECK			0x03 // len=9 MICOM->STB) current micom status. You can reference this function name as “micom_status_process” 
// First, you have to send to micom this cmd which “MICOM_CMD_STATUS”, then you can get it.
#define MICOM_ANSWER_SERIAL			0x04 // len=11 do not used
#define MICOM_ANSWER_CURTIME			0x05 // len=10 => Second/Min/Hour/Date/Month/Year(MICOM->STB) when you want to get current time from micom. You can reference this function name as “rtc_time_process”. 
// First, you have to send to micom this cmd which “MICOM_CMD_TRANSFER_TIME”, then you can get it.

#define MICOM_CMD_NO_PING			0x04 // (STB->MICOM) when you want to set for the ping condition of the micom
#define MICOM_CMD_VFD_DISP			0x05 // (STB->MICOM) string display cmd
#define MICOM_CMD_PONG				0x06 // (STB->MICOM) pong cmd. If ping_pong is valid and you got the ping cmd, then you have to send the pong cmd to micom.
#define MICOM_CMD_RESET				0x07 //  do not used
#define MICOM_CMD_DEEP_STANDBY			0x08 // (STB->MICOM) standby off cmd
#define MICOM_CMD_TEXT_MODE			0x14 // (STB->MICOM) display mode cmd
#define MICOM_CMD_TIME_CONTROL			0x17 // (STB->MICOM) when you want to set the rtc time
#define MICOM_CMD_STATUS			0x18 // (STB->MICOM) when you want to get the status of the micom.
#define MICOM_CMD_TIMER_SET			0x19 // len=9 => Year/Month/Date/Hour/Min 
// (STB->MICOM) when you want to set the wakeup time or reserved time(reserved record ch, reserved view ch etc..) before power off.
#define MICOM_CMD_SERNUM			0x1A // do not used
#define MICOM_CMD_TRANSFER_TIME			0x1B // (STB->MICOM) when you want to get the time of the micom.
#define MICOM_CMD_SEG_MODE			0x60 // (STB->MICOM) segment mode cmd
#define MICOM_CMD_REBOOT			0x61 // do not used

// MICOM_PARAM_TEXT_MODE
#define TEXT_MODE_NORMAL			"\x00"
#define TEXT_MODE_BLINKING			"\x01"
#define TEXT_MODE_MOVE_LEFT			"\x02"
#define TEXT_MODE_MOVE_RIGHT			"\x03"
#define TEXT_MODE_TIME				"\x04"
// MICOM_PARAM_SEG_MODE
#define SEG_MODE_CURRENT_STATE			"\x00"
#define SEG_MODE_ALL_OFF			"\x01"
#define SEG_MODE_ALL_ON				"\x02"
#define SEG_MODE_ALL_ON_HIGH_PRIORITY		"\x03"


#define cMaxCommandLen 	8

/* ************************************************** */
/* Access ASC3; from u-boot; copied from TF7700 ;-)   */
/* ************************************************** */
#define ASC0BaseAddress 0xb8030000
#define ASC1BaseAddress 0xb8031000
#define ASC2BaseAddress 0xb8032000
#define ASC3BaseAddress 0xb8033000
#define ASC_BAUDRATE    0x000
#define ASC_TX_BUFF     0x004
#define ASC_RX_BUFF     0x008
#define ASC_CTRL        0x00c
#define ASC_INT_EN      0x010
#define ASC_INT_STA     0x014
#define ASC_GUARDTIME   0x018
#define ASC_TIMEOUT     0x01c
#define ASC_TX_RST      0x020
#define ASC_RX_RST      0x024
#define ASC_RETRIES     0x028

#define ASC_INT_STA_RBF   0x01
#define ASC_INT_STA_TE    0x02
#define ASC_INT_STA_THE   0x04
#define ASC_INT_STA_PE    0x08
#define ASC_INT_STA_FE    0x10
#define ASC_INT_STA_OE    0x20
#define ASC_INT_STA_TONE  0x40
#define ASC_INT_STA_TOE   0x80
#define ASC_INT_STA_RHF   0x100
#define ASC_INT_STA_TF    0x200
#define ASC_INT_STA_NKD   0x400

#define ASC_CTRL_FIFO_EN  0x400

/*  GPIO Pins  */

#define PIO0BaseAddress   0xb8020000
#define PIO1BaseAddress   0xb8021000
#define PIO2BaseAddress   0xb8022000
#define PIO3BaseAddress   0xb8023000
#define PIO4BaseAddress   0xb8024000
#define PIO5BaseAddress   0xb8025000

#define PIO_CLR_PnC0      0x28
#define PIO_CLR_PnC1      0x38
#define PIO_CLR_PnC2      0x48
#define PIO_CLR_PnCOMP    0x58
#define PIO_CLR_PnMASK    0x68
#define PIO_CLR_PnOUT     0x08
#define PIO_PnC0          0x20
#define PIO_PnC1          0x30
#define PIO_PnC2          0x40
#define PIO_PnCOMP        0x50
#define PIO_PnIN          0x10
#define PIO_PnMASK        0x60
#define PIO_PnOUT         0x00
#define PIO_SET_PnC0      0x24
#define PIO_SET_PnC1      0x34
#define PIO_SET_PnC2      0x44
#define PIO_SET_PnCOMP    0x54
#define PIO_SET_PnMASK    0x64
#define PIO_SET_PnOUT     0x04

static void dump(char *label, unsigned char *data, int len)
{
#ifdef DEBUG_DEV
	int i;

	printk(label);
	for (i = 0; i < len; i++)
	{
		printk("%02X ", data[i]);
	}
	printk(" [len=%02X]\n", len);
#endif
}


static void hexdump(unsigned char *label, unsigned char *data, unsigned int len)
{
#ifdef DEBUG_DEV
	unsigned short y = 0;
	unsigned int x = 0;
	int z;
	int offset = strlen(label);

//    if (offset > 0)
//	printf ("%*c", offset, ' ');
	printk(label);
	for (x = 0; x < len; x++)
	{
		printk("%.2X ", data[x]);
		y++;
		if (y == 16)
		{
			printk("  ");
			for (z = 0; z < 16; z++)
			{
				if ((data[((x / 16) * 16) + z] > 0x1F) &&
						(data[((x / 16) * 16) + z] < 0x7F))
				{
					printk("%c", data[((x / 16) * 16) + z]);
				}
				else
				{
					printk("?");
				}
			}
			y = 0;
			if (x < (len - 1) && offset > 0)
				printk("\n%*c", offset, ' ');
		}
	}
	if (y > 0)
	{
		for (z = y; z < 16; z++)
			printk("   ");
		printk("  ");
		for (z = 0; z < y; z++)
		{
			if ((data[(len - (len % 16)) + z] > 0x1F) &&
					(data[(len - (len % 16)) + z] < 0x7F))
			{
				printk("%c", data[(len - (len % 16)) + z]);
			}
			else
			{
				printk(".");
			}
		}
	}
	printk("\n");
	printk(" len=0x%02X\n", len);
#endif
}

static irqreturn_t FP_interrupt(int irq, void *dev_id)
{
	unsigned int         *ASC_3_INT_STA = (unsigned int *)(ASC3BaseAddress + ASC_INT_STA);
	char                 *ASC_3_RX_BUFF = (char *)(ASC3BaseAddress + ASC_RX_BUFF);
	int 			dataArrived = 0;

	while (*ASC_3_INT_STA & ASC_INT_STA_RBF)
	{
		receivedData[writePosition] = *ASC_3_RX_BUFF;
#ifdef DEBUG_DEV
		printk(" <- 0x%x\n", receivedData[writePosition]);
#endif

		writePosition = (writePosition + 1) % BUFFERSIZE;

		dataArrived = 1;
		if (writePosition == readPosition)
			printk("FP: ERR: input buffer overflow\n");

	}

	if (dataArrived)
		up(&rx_int_sem);

	return IRQ_HANDLED;
}

static int serial3_putc(char Data)
{
	char                  *ASC_3_TX_BUFF = (char *)(ASC3BaseAddress + ASC_TX_BUFF);
	unsigned int          *ASC_3_INT_STA = (unsigned int *)(ASC3BaseAddress + ASC_INT_STA);
	unsigned long         Counter = 200000;

	while (((*ASC_3_INT_STA & ASC_INT_STA_THE) == 0) && --Counter);

	if (Counter == 0)
	{
#ifdef DEBUG_DEV
		printk("FP: ERR: writing char (%c) \n", Data);
#endif
//  	*(unsigned int*)(ASC3BaseAddress + ASC_TX_RST)   = 1;
//	return 0;
	}

	*ASC_3_TX_BUFF = Data;
	return 1;
}

#define BAUDRATE_VAL_M1(bps, clk)	( ((bps * (1 << 14)) / ((clk) / (1 << 6)) ) + 1 )

static const char *tx_pio_name = "micom_tx";
static const char *rx_pio_name = "micom_rx";

static void serial3_init(void)
{

	/* Configure the PIO pins */
	stpio_request_pin(5, 0, tx_pio_name, STPIO_ALT_OUT); /* Tx */
	stpio_request_pin(5, 1, rx_pio_name, STPIO_IN);      /* Rx */

	*(unsigned int *)(PIO5BaseAddress + PIO_CLR_PnC0) = 0x07;
	*(unsigned int *)(PIO5BaseAddress + PIO_CLR_PnC1) = 0x06;
	*(unsigned int *)(PIO5BaseAddress + PIO_SET_PnC1) = 0x01;
	*(unsigned int *)(PIO5BaseAddress + PIO_SET_PnC2) = 0x07;

	*(unsigned int *)(ASC3BaseAddress + ASC_INT_EN)   = 0x00000000;
	*(unsigned int *)(ASC3BaseAddress + ASC_CTRL)     = 0x00001589;
	*(unsigned int *)(ASC3BaseAddress + ASC_TIMEOUT)  = 0x00000010;
	*(unsigned int *)(ASC3BaseAddress + ASC_BAUDRATE) = 100; //0x000000c9; FIXME: may be switch to mode 0?
	*(unsigned int *)(ASC3BaseAddress + ASC_TX_RST)   = 0;
	*(unsigned int *)(ASC3BaseAddress + ASC_RX_RST)   = 0;
}

/* process commands where we expect data from frontcontroller
 * e.g. getTime and getWakeUpMode
 */
static int processDataAck(unsigned char *data, int count)
{
	switch (data[0])
	{
		case 0xB9: /* timeval ->len = 6 */
			if (count != 6)
				return 0;

			/* 0. claim semaphore */
			down_interruptible(&receive_sem);

			dprintk("command getTime complete\n");

			/* 1. copy data */

			memcpy(ioctl_data, data, 8);

			/* 2. free semaphore */
			up(&receive_sem);

			/* 3. wake up thread */
			dataReady = 1;
			wake_up_interruptible(&ioctl_wq);
			return 1;
			break;
		case 0x77: /* wakeup ->len = 8 */
			/* fixme: determine the real len here
			 * I think it will be two but I'm not sure
			 * ->this will be block until forever if
			 * the number does not match!
			 */
			if (count != 8)
				return 0;

			/* 0. claim semaphore */
			down_interruptible(&receive_sem);

			dprintk("command getWakeupMode complete\n");

			/* 1. copy data */

			memcpy(ioctl_data, data, 8);

			/* 2. free semaphore */
			up(&receive_sem);

			/* 3. wake up thread */
			dataReady = 1;
			wake_up_interruptible(&ioctl_wq);
			return 1;
			break;
	}

	return 0;
}

/* process acknowledges from frontcontroller. this is for
 * one shot commands which we have send to the controller.
 * all setXY Functions
 */
static int processAck(unsigned char c)
{
	switch (c)
	{
		case 0xF5: /* cmd_error ->len = 1 */
			printk("receive error from fp\n");
			return 2;
			break;
		case 0xFA: /* cmd_ok ->len = 1*/
		case 0xF1:
			return 1;
			break;
		case 0xd: /* this is not a real response */
			return 0;
			break;
	}

	return 0;
}

static int translate_fp_key(unsigned char *data, int dlen)
{
	int i;

	for (i = 0; i < FP_KEYS_MAX; i++)
		if (memcmp(data, fp_keys[i].raw, FP_KEY_LEN) == 0)
		{
#ifdef DEBUG_DEV
			printk(" => found key [%s] = %x\n", fp_keys[i].name, fp_keys[i].key);
#endif
			return fp_keys[i].key;
		}
	return 0;
}

/* process event data from frontcontroller. An event is
 * defined as something which is _not_ initiated
 * by a command _to_ the frontcontroller.
 */
static int processEvent(unsigned char *data, int count)
{

	if (debug)
		dump("processEvent: ", data, count);

	if (count < 5)
		return 0; // no enough data size

	if (data[0] == 0x01 && data[1] == 0x05 && data[4] == 0xFB)
	{
		int key;

		/* 0. claim semaphore */
		down_interruptible(&receive_sem);

		dprintk("command RCU complete. Added %i. item\n", receiveCount + 1);

		/* 1. copy data */

		if (receiveCount == cMaxReceiveQueue)
		{
			printk("FP: ERR: receive queue full!\n");
			/* 2. free semaphore */
			up(&receive_sem);
			return 1;
		}

		memcpy(receive[receiveCount].buffer, data, 5);
		receiveCount++;

		// event subsystem passing
		key = translate_fp_key(data, 5);
		if (key)
		{
			input_report_key(fp_button_dev, key, 1);
			input_sync(fp_button_dev);
			msleep(100);
			input_report_key(fp_button_dev, key, 0);
			input_sync(fp_button_dev);
			//FIXME: Add delay to not report one key press ducplicated
		}

		/* 2. free semaphore */
		up(&receive_sem);

		/* 3. wake up threads */
		wake_up_interruptible(&wq);
		return 1;
	}

	return 0;
}

/* detect the start of an event from the frontcontroller */
static int detectEvent(unsigned char c)
{
	switch (c)
	{
		case MICOM_ANSWER_KEY:
			return 1;
			break;
	}

	return 0;
}


/* search for the start of an answer of an command
 * we have send to the frontcontroller where we
 * expect data from the controller.
 */
static int searchAnswerStart(unsigned char c)
{
	switch (c)
	{
		case 0xF5: /* cmd_error ->len = 1 */
			printk("FP: ERR: receive error from fp\n");
			return 2;
			break;
		case 0xFA: /* cmd_ok ->len = 1*/
		case 0xB9: /* timeval ->len = 8 */
		case 0x77: /* wakeup ->len = 8 */
		case 0xC1: /* wakeup from rcu ->len = 8 */
		case 0xC2: /* wakeup from front ->len = 8 */
		case 0xC3: /* wakeup from time ->len = 8 */
		case 0xC4: /* wakeup from ac ->len = 8 */
			return 1;
			break;
	}

	return 0;
}

/* if the frontcontroller has returned an error code
 * or if the trial counter overflows then we requeue
 * the data for currently 5 times. the command will
 * be re-send automatically to the controller.
 */
static void requeueData(void)
{
	dprintk("requeue data %d\n", state);
	transmit[0].requeueCount++;

	if (transmit[0].requeueCount == cMaxQueueCount)
	{
		printk("max requeueCount reached aborting transmission %d\n", state);
		down_interruptible(&transmit_sem);

		transmitCount--;

		memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));

		if (transmitCount != 0)
			dprintk("next command will be 0x%x\n", transmit[0].command);

		up(&transmit_sem);

		dataReady = 0;
		timeoutOccured = 1;
		/* 3. wake up thread */
		wake_up_interruptible(&ioctl_wq);

	}
}

static int fpReceiverTask(void *dummy)
{
	unsigned int  *ASC_3_INT_EN = (unsigned int *)(ASC3BaseAddress + ASC_INT_EN);
	unsigned char command[16];
	int		count = 0;

	daemonize("fp_rcv");

	allow_signal(SIGTERM);

	//we are on so enable the irq
	*ASC_3_INT_EN = *ASC_3_INT_EN | 0x00000001;

	while (1)
	{
		if (down_interruptible(&rx_int_sem))
			break;

		if (leaving)
		{
			leaving++;
			break;
		}

		while (1)
		{

			if (leaving)
				break;

			/* FIXME FIXME FIXME FIXME
			-fixmefixmefixme
			- sometime the state cStateWaitAck stalled
			->must lock this if it happens again
			->using the rc sometimes solves the problem
			- all other things seems to work
			*/

			/* since we process all data here it may be that there
			 * are more up's than we need so check _before_ processing
			 * anything if there is data. this is not the best but
			 * better than the fact of loosing interrupts ...
			 */
			if (readPosition == writePosition)
			{
				break;
			}

			/* process our state-machine */
			switch (state)
			{
				case cStateIdle: /* nothing do to, search for command start */
					if (detectEvent(receivedData[readPosition]))
					{
						count = 0;
						command[count++] = receivedData[readPosition];
						state = cStateWaitEvent;
						readPosition = (readPosition + 1) % BUFFERSIZE;
						dprintk("0. current readPos = %d\n", readPosition);
					}
					else
					{
						readPosition = (readPosition + 1) % BUFFERSIZE;
						dprintk("0.1 current readPos = %d\n", readPosition);
					}
					break;
				case cStateWaitAck: /* each setter */
				{
					unsigned char c = receivedData[readPosition];
					int err = processAck(c);

					readPosition = (readPosition + 1) % BUFFERSIZE;

					dprintk("1. current readPos = %d\n", readPosition);

					if (err == 2)
					{
						/* an error is detected from fp requeue data ...
						 */
						requeueData();

						state = cStateIdle;
						waitAckCounter = 0;
						count = 0;
					}
					else if (err == 1)
					{
						/* data is processed remove it from queue */

						down_interruptible(&transmit_sem);

						transmitCount--;

						memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));

						if (transmitCount != 0)
							dprintk("next command will be 0x%x\n", transmit[0].command);

						up(&transmit_sem);

						waitAckCounter = 0;
						dprintk("detect ACK %d\n", state);
						state = cStateIdle;
						count = 0;
					}
					else if (err == 0)
					{
						udelay(1);
						waitAckCounter--;

						dprintk("1. %d, %d 0x%x\n", waitAckCounter, readPosition, c);

						if (waitAckCounter <= 0)
						{
							dprintk("missing ACK from micom ->requeue data %d\n", state);

							requeueData();

							count = 0;
							waitAckCounter = 0;
							state = cStateIdle;
						}
					}
				}
				break;
				case cStateWaitDataAck: /* each getter */
				{
					int err;

					command[count++] = receivedData[readPosition];
					readPosition = (readPosition + 1) % BUFFERSIZE;

					dprintk("2. current readPos = %d\n", readPosition);

					err = processDataAck(command, count);

					if (err == 1)
					{
						/* data is processed remove it from queue */

						down_interruptible(&transmit_sem);

						transmitCount--;

						memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));

						if (transmitCount != 0)
							dprintk("next command will be 0x%x\n", transmit[0].command);

						up(&transmit_sem);

						waitAckCounter = 0;
						dprintk("detect ACK %d\n", state);
						state = cStateIdle;
						count = 0;
					}
				}
				break;
				case cStateWaitEvent: /* key or button */
					if (receiveCount < cMaxReceiveQueue)
					{
						command[count++] = receivedData[readPosition];
						readPosition = (readPosition + 1) % BUFFERSIZE;
						dprintk("3. current readPos = %d\n", readPosition);

						if (processEvent(command, count))
						{
							/* command completed */
							count = 0;
							state = cStateIdle;
						}

					}
					else
					{
						/* noop: wait that someone reads data */
						dprintk("overflow, wait for readers\n");
					}
					break;
				case cStateWaitStartOfAnswer: /* each getter */
				{
					int err = searchAnswerStart(receivedData[readPosition]);
					unsigned char c = receivedData[readPosition];

					readPosition = (readPosition + 1) % BUFFERSIZE;
					dprintk("4. current readPos = %d\n", readPosition);

					if (err == 2)
					{
						/* error detected ->requeue data ... */

						requeueData();

						state = cStateIdle;
						waitAckCounter = 0;
						count = 0;
					}
					else if (err == 1)
					{
						/* answer start detected now process data */
						count = 0;
						command[count++] = c;
						state = cStateWaitDataAck;
					}
					else
					{
						/* no answer start detected */
						udelay(1);
						waitAckCounter--;

						dprintk("2. %d\n", waitAckCounter);

						if (waitAckCounter <= 0)
						{
							dprintk("missing ACK from micom ->requeue data %d\n", state);
							requeueData();

							count = 0;
							waitAckCounter = 0;
							state = cStateIdle;
						}
					}
				}
				break;
				case cStateTransmission:
					/* we currently transmit data so ignore all */
					break;
			}
		} /* while */
	}

	printk("FP: task %s stopped\n", __func__);

	return 0;
}

static int fpTransmitterTask(void *dummy)
{
	unsigned char 	micom_cmd[VFD_MAX_DATA_LEN];
	int  	vLoop;
	int prev_state = 666, prev_transmitCount = 666; // faked with reason ;)

	daemonize("fp_transmit");

	allow_signal(SIGTERM);

	while (1)
	{
		//wait_event_interruptible(task_wq, (waitAck == 0) && (transmitCount != 0));

		/* only send new command if we dont wait already for acknowledge
		 * or we dont wait for a command/answer completion and we have
		 * something to transmit
		 */

		if (leaving)
		{
			leaving++;
			break;
		}

		if ((state == cStateIdle) && (transmitCount > 0))
		{
			int sendFailed = 0;

			dprintk("send data to fp (0x%x)\n", transmit[0].command);

			/* 0. claim sema */
			down_interruptible(&transmit_sem);

			/* 1. send it to the frontpanel */
			memset(micom_cmd, 0, VFD_MAX_DATA_LEN);

			micom_cmd[0] = transmit[0].command;
			micom_cmd[1] = transmit[0].len + 3;
			memcpy(micom_cmd + 2, transmit[0].buffer, transmit[0].len);
			micom_cmd[transmit[0].len + 2] = 0xAA;

			state = cStateTransmission;
			int i;
			for (vLoop = 0 ; vLoop < micom_cmd[1]; vLoop++)
			{
				udelay(10000);
				if (serial3_putc((micom_cmd[vLoop])) == 0)
				{
					printk("FP: ERR: sending char = %c \n", micom_cmd[vLoop]);
					state = cStateIdle;
					sendFailed = 1;
					break;
				}
				else
					dprintk(" -> 0x%x\n", micom_cmd[vLoop]);
			}

			state = cStateIdle;

			if (sendFailed == 0)
			{
				if (transmit[0].needAck)
				{
					waitAckCounter = cMaxAckAttempts;

					if (transmit[0].isGetter)
					{
						timeoutOccured = 0;
						state = cStateWaitStartOfAnswer;
					}
					else
						state = cStateWaitAck;
				}
				else
				{
					/* no acknowledge needed so remove it direct */
					transmitCount--;

					memmove(&transmit[0], &transmit[1], (cMaxTransQueue - 1) * sizeof(struct transmit_s));

					if (transmitCount != 0)
						dprintk("%s: next command will be 0x%x\n", __func__, transmit[0].command);

				}
			}

			/* 4. free sem */
			up(&transmit_sem);
		}
		else
		{
			if (state != prev_state || transmitCount != prev_transmitCount)
			{
				dprintk("%d, %d\n", state, transmitCount);
				prev_state = state;
				prev_transmitCount = transmitCount;
			}
			msleep(100);

		}
	}

	printk("FP: task %s stopped\n", __func__);

	return 0;
}


/* End ASC3 */

static void micomWriteCommand(char command, char *buffer, int len, int needAck, int isGetter)
{

	dprintk("%s >\n", __func__);

	int i;
	dprintk("command 0x%x -", command);
	for (i = 0; i < len; i++)
		dprintk(" 0x%x", buffer[i]);
	dprintk(" ; len = %d\n", len);

	/* 0. claim semaphore */
	down_interruptible(&transmit_sem);

	if (transmitCount >= cMaxTransQueue)
	{
		//FIXME: change to cyclic buffer
		printk("FP: WARN: transmit queue overflow");

		memmove(&transmit[0], &transmit[1], sizeof(struct transmit_s) * (cMaxTransQueue - 1));
		transmitCount--;
	}

	/* 1. setup new entry */
	memset(transmit[transmitCount].buffer, 0, VFD_MAX_DATA_LEN);
	if (len && buffer && strlen(buffer) > 0)
		memcpy(transmit[transmitCount].buffer, buffer, len);

	transmit[transmitCount].len = len;
	transmit[transmitCount].command = command;
	transmit[transmitCount].needAck = 0; //needAck;
	transmit[transmitCount].requeueCount = 0;
	transmit[transmitCount].isGetter = isGetter;
	transmitCount++;

	/* 2. free semaphore */
	up(&transmit_sem);

	dprintk("%s < \n", __func__);
}

void micomSetBrightness(int level)
{
	char buffer[8];

	dprintk("%s > %d\n", __func__, level);
	if (level < 1 || level > 8)
	{
		printk("VFD/MICOM brightness out of range %d\n", level);
		return;
	}

	memset(buffer, 0, 8);
	buffer[0] = level;

	micomWriteCommand(0x0A, buffer, 7, 1 , 0);

	dprintk("%s <\n", __func__);

}
/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBrightness);


void micomSetIcon(int which, int on)
{
	unsigned char buf = on;

	which &= 0x0F;
	if (which < 0 || which > 15)
	{
		printk("FP: icon number out of range %d\n", which);
		return;
	}
	which += ICON_MIN;

	if (on == 1)
		micomWriteCommand(which, &buf, 1, 0 , 0);
	else
		micomWriteCommand(which, &buf, 1, 0 , 0);
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetIcon);

static void micomSetTime(char *time)
{
	dprintk("%s\n", time);
	char 	   buffer[8];
	memset(buffer, 0, 8);
	memcpy(buffer, time, 7);
	// sec   min   hour  dayweek  day   month  year
	dprintk("TIME SET 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n"
		, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6]);
	micomWriteCommand(MICOM_CMD_TIME_CONTROL, buffer, 7, 1 , 0);
}

static void micomSetTimerWakeUp(char *timer)
{
	printk("%s\n", timer);
	char 	   buffer[7];
	memset(buffer, 0, 7);
	memcpy(buffer, timer, 6); // Year Month  Date   Hour   Min    Sec
	dprintk("WAKEUP TIMER SET  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n"
		, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
	micomWriteCommand(MICOM_CMD_TIMER_SET, buffer, 6, 0 , 0);
}

static void micomReset(void)
{
	char 	   buffer[8];
	buffer[0] = 0;
	micomWriteCommand(MICOM_CMD_RESET, buffer, 0, 0, 0);
}

static void micomSetStandby(void)
{
	micomWriteCommand(MICOM_CMD_TEXT_MODE, (char *)TEXT_MODE_TIME, 1, 0, 0);
}

static void micomSetDeepStandby(void)
{
	char 	   buffer[8];
	buffer[0] = 0;
	micomWriteCommand(MICOM_CMD_DEEP_STANDBY, buffer, 1, 0 , 0);
}

static int UnicodeToVFDindexedstring(unsigned char *input, unsigned char *output, int len)
{
	char    res, escape;
	unsigned short tempUnicode;
	int i, idx, out_idx = 0, unknown_code = 0;

	for (idx = 0; idx < len; idx++)
	{
		escape = 0;
		tempUnicode = input[idx];
		if (input[idx] >= 0xC0 && input[idx] <= 0xDF)
		{
			escape = input[idx];
			idx++;
			if (idx >= len)
			{
				output[out_idx] = 0x00;
				return out_idx;
			}
			tempUnicode = ((tempUnicode & 0x1F) << 6) | (input[idx] & 0x3F);
		}
		if (0x0020 <= tempUnicode && tempUnicode < 0x0080)
			output[out_idx] = (char)(tempUnicode & 0x00FF);
		else if (tempUnicode == 0x0000)
			break;
		else
		{
			res = 0x00;

			for (i = 0 ; unicode_vfd[i][0] ; i++)
			{
				if (unicode_vfd[i][0] == tempUnicode)
				{
					res = unicode_vfd[i][1];
					break;
				}
			}
			if (res != 0x00)
				output[out_idx] = res;
			else if (tempUnicode == 0xfeff)
				out_idx--;
			else
			{
				output[out_idx] = '?';
				unknown_code++;
				printk("FP: translate: unsupported unicode 0x%02x 0x%02x\n", escape, input[idx]);
			}
		}
		out_idx++;
	}
	output[out_idx] = 0x00;

	return out_idx;
}

static void micomWriteString(unsigned char *aBuf, int len, int raw)
{
	unsigned char bBuf[VFD_MAX_DATA_LEN + 1];
	//int i = 0, j = 0;
	int newlen;

	memset(bBuf, ' ', sizeof(bBuf));

	/* check for string size */
	if (len > VFD_MAX_DATA_LEN)
	{
		newlen = VFD_MAX_DATA_LEN;
		aBuf[newlen] = '\0';
	}
	else
		newlen = len;

	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, newlen);
	lastdata.length = newlen;

	if (!raw)
	{
		hexdump(" --> WRITE_STRING_DUMP: before: ", aBuf, newlen);
		newlen = UnicodeToVFDindexedstring(aBuf, bBuf, newlen);
	}
	else
		memcpy(bBuf, aBuf, newlen);

	//printk(" WRITE_STRING: %s (len=%d)\n", bBuf, newlen);
	hexdump(" --> WRITE_STRING_DUMP: after : ", bBuf, newlen);

	micomWriteCommand(MICOM_CMD_VFD_DISP, bBuf, 20, 0 , 0);
}

static int micom_init_func(void)
{
	int vLoop;

	printk("FP: Micom/VFD module initializing\n");

	//micomWriteCommand(MICOM_CMD_REBOOT, /* ON */ "\x01" , 1, 0, 0);
	//msleep(100);
	micomWriteCommand(MICOM_CMD_NO_PING, /* ON */ "\x01" , 1, 0, 0);

	micomWriteString(EMPTY, strlen(EMPTY), 0);
#if 1//def INIT_EFECT
	{
		char buf[VFD_MAX_DATA_LEN];

		memset(buf, 0, sizeof(buf));
		for (vLoop = 0; vLoop < strlen(GREETS); vLoop++)
		{
			buf[vLoop] = GREETS[vLoop];
			micomWriteString(buf, strlen(buf), 1);
			msleep(30);
		}
	}
#else
	micomWriteString(GREETS, strlen(GREETS), 0);
	micomWriteCommand(MICOM_CMD_TEXT_MODE, (char *)TEXT_MODE_MOVE_LEFT, 1, 0, 0);
#endif


	for (vLoop = ICON_MIN; vLoop <= ICON_MAX; vLoop++)
		micomSetIcon(vLoop, 0);

#ifdef INIT_EFECT
	for (vLoop = ICON_MIN; vLoop <= ICON_MAX; vLoop++)
	{
		msleep(100);
		micomSetIcon(vLoop, 1);
	}
	for (vLoop = ICON_MAX; vLoop >= ICON_MIN; vLoop--)
	{
		msleep(100);
		micomSetIcon(vLoop, 0);
	}
#endif
	return 0;
}

static ssize_t MICOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int minor, vLoop;
	dprintk("%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	minor = -1;
	for (vLoop = 0; vLoop < LASTMINOR; vLoop++)
	{
		if (FrontPanelOpen[vLoop].fp == filp)
		{
			minor = vLoop;
		}
	}

	if (minor == -1)
	{
		printk("Error Bad Minor\n");
		return -1; //FIXME
	}

	dprintk("minor = %d\n", minor);

	/* dont write to the remote control */
	if (minor == FRONTPANEL_MINOR_RC)
		return -EOPNOTSUPP;

	kernel_buf = kmalloc(len, GFP_KERNEL);

	if (kernel_buf == NULL)
	{
		dprintk("%s return no mem<\n", __func__);
		return -ENOMEM;
	}
	copy_from_user(kernel_buf, buff, len);

	if (down_interruptible(&write_sem))
		return -ERESTARTSYS;

	micomWriteString(kernel_buf, len - 1, 0);

	kfree(kernel_buf);

	up(&write_sem);

	dprintk("%s <\n", __func__);
	return len;
}

static ssize_t MICOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;
	dprintk("%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	minor = -1;
	for (vLoop = 0; vLoop < LASTMINOR; vLoop++)
	{
		if (FrontPanelOpen[vLoop].fp == filp)
		{
			minor = vLoop;
		}
	}

	if (minor == -1)
	{
		printk("Error Bad Minor\n");
		return -EUSERS;
	}

	dprintk("minor = %d\n", minor);

	if (minor == FRONTPANEL_MINOR_RC)
	{

		while (receiveCount == 0)
		{
			if (wait_event_interruptible(wq, receiveCount > 0))
				return -ERESTARTSYS;
		}

		/* 0. claim semaphore */
		down_interruptible(&receive_sem);

		/* 1. copy data to user */
		copy_to_user(buff, receive[0].buffer, 8);

		/* 2. copy all entries to start and decreas receiveCount */
		receiveCount--;
		memmove(&receive[0], &receive[1], (cMaxTransQueue - 1) * sizeof(struct receive_s));

		/* 3. free semaphore */
		up(&receive_sem);

		return 8;
	}

	/* copy the current display string to the user */
	if (down_interruptible(&FrontPanelOpen[minor].sem))
	{
		dprintk("%s return erestartsys<\n", __func__);
		return -ERESTARTSYS;
	}

	if (FrontPanelOpen[minor].read == lastdata.length)
	{
		FrontPanelOpen[minor].read = 0;

		up(&FrontPanelOpen[minor].sem);
		dprintk("%s return 0<\n", __func__);
		return 0;
	}

	if (len > lastdata.length)
		len = lastdata.length;

	/* fixme: needs revision because of utf8! */
	if (len > 16)
		len = 16;

	FrontPanelOpen[minor].read = len;
	copy_to_user(buff, lastdata.data, len);

	up(&FrontPanelOpen[minor].sem);

	dprintk("%s < (len %d)\n", __func__, len);
	return len;
}

static int MICOMdev_open(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk("%s >\n", __func__);

	minor = MINOR(inode->i_rdev);

	dprintk("open minor %d\n", minor);

	if (FrontPanelOpen[minor].fp != NULL)
	{
		printk("EUSER\n");
		return -EUSERS;
	}
	FrontPanelOpen[minor].fp = filp;
	FrontPanelOpen[minor].read = 0;

	dprintk("%s <\n", __func__);
	return 0;

}

static int MICOMdev_close(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk("%s >\n", __func__);

	minor = MINOR(inode->i_rdev);

	dprintk("close minor %d\n", minor);

	if (FrontPanelOpen[minor].fp == NULL)
	{
		printk("EUSER\n");
		return -EUSERS;
	}
	FrontPanelOpen[minor].fp = NULL;
	FrontPanelOpen[minor].read = 0;

	dprintk("%s <\n", __func__);
	return 0;
}

static int MICOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	static int mode = 0;
	struct micom_ioctl_data *micom = (struct micom_ioctl_data *)arg;

	dprintk("%s > 0x%.8x\n", __func__, cmd);

	if (down_interruptible(&write_sem))
		return -ERESTARTSYS;

	switch (cmd)
	{
		case VFDDRIVERINIT:
			micom_init_func();
			mode = 0;
			break;
		case VFDBRIGHTNESS:
#if 0
			if (mode != 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				int level = data->start_address;
				printk(" level = %d\n", level);
				/* scale level from 0 - 7 to a range from 1 - 5 where 5 is off */
				level = 7 - level;

				level = ((level * 100) / 7 * 5) / 100 + 1;

				micomSetBrightness(level);
			}
			else
			{
				printk("level = %d\n", micom->u.brightness.level);
				micomSetBrightness(micom->u.brightness.level);
			}
			mode = 0;
#else
			FIXME_FAKED_FOR_NOW(cmd, "VFDBRIGHTNESS");
#endif
			break;
		case VFDICONDISPLAYONOFF:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				int icon_nr = (data->data[0] & 0xf) + ICON_MIN;
				int on = data->data[4];

				micomSetIcon(icon_nr, on);
			}
			else
			{
				micomSetIcon(micom->u.icon.icon_nr, micom->u.icon.on);
			}

			mode = 0;
			break;
		case VFDDISPLAYCLEAR:
			micomWriteString(EMPTY, strlen(EMPTY), 0);
			break;
		case VFDSTANDBY:
			micomSetStandby();
			break;
		case VFDRESET:
			micomReset();
			break;
		case VFDDEEPSTANDBY:
			micomSetDeepStandby();
			break;
		case VFDSETTIME:
			micomSetTime(micom->u.standby.time);
			break;
		case VFDSETTIMERWAKEUP:
			micomSetTimerWakeUp(micom->u.wakeup.timer);
			break;
		case VFDDISPLAYCHARS:
		case VFDDISPLAYCHARSRAW:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				int raw = 0;

				if (data->length < 1)
					break;

				if (cmd == VFDDISPLAYCHARSRAW)
					raw = 1;
#if 1
				if (data->length < MICOM_MAX_DISPLAY_LEN)
				{
					// smaller => centre
					char text[MICOM_MAX_DISPLAY_LEN + 1];
					int pad = (MICOM_MAX_DISPLAY_LEN - data->length) / 2;

					memset(text, 0, sizeof(text));
					memset(text, ' ', pad);
					strncpy(text + pad, data->data, data->length);
					micomWriteString(text, strlen(text), raw);
				}
				else
#endif
					if (data->length > MICOM_MAX_DISPLAY_LEN)
					{
						// bigger => enable scrolling
						micomWriteString(data->data, data->length, raw);
						micomWriteCommand(MICOM_CMD_TEXT_MODE, (char *)TEXT_MODE_MOVE_LEFT, 1, 0, 0);
					}
					else
						// simple reuse
						micomWriteString(data->data, data->length, raw);

			}
			else
			{
				//not suppoerted
			}
			mode = 0;
			break;
		case VFDDISPLAYWRITEONOFF:
			/* ->alles abschalten ? VFD_Display_Write_On_Off */
			//printk("VFDDISPLAYWRITEONOFF ->not yet implemented\n");
			FIXME_FAKED_FOR_NOW(cmd, "VFDDISPLAYWRITEONOFF");
			break;
		case VFDGETSTATUS:
			FIXME_FAKED_FOR_NOW(cmd, "VFDGETSTATUS");
			break;
		default:
			printk("FP: WARN: unknown IOCTL 0x%x\n", cmd);

			mode = 0;
			break;
	}

	up(&write_sem);
	return 0;
}

static unsigned int MICOMdev_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &wq, wait);

	if (receiveCount > 0)
	{
		mask = POLLIN | POLLRDNORM;
	}

	return mask;
}

static struct file_operations vfd_fops =
{
	.owner = THIS_MODULE,
	.ioctl = MICOMdev_ioctl,
	.write = MICOMdev_write,
	.read  = MICOMdev_read,
	.poll  = (void *) MICOMdev_poll,
	.open  = MICOMdev_open,
	.release  = MICOMdev_close
};

static int __init micom_init_module(void)
{
	unsigned int         *ASC_3_INT_EN = (unsigned int *)(ASC3BaseAddress + ASC_INT_EN);
	unsigned int         *ASC_3_CTRL   = (unsigned int *)(ASC3BaseAddress + ASC_CTRL);
	int i;

	printk("Frontpanel VFD/MICOM driver for ABIP 9900/99HD, version %d.%02d (c) 2010 Sysifos\n", MICOM_MODULE_VERSION / 100, MICOM_MODULE_VERSION % 100);

	//Disable all ASC 3 interrupts
	*ASC_3_INT_EN = *ASC_3_INT_EN & ~0x000001ff;

	serial3_init();

	sema_init(&rx_int_sem, 0);
	sema_init(&write_sem, 1);
	sema_init(&receive_sem, 1);
	sema_init(&transmit_sem, 1);

	for (i = 0; i < LASTMINOR; i++)
		sema_init(&FrontPanelOpen[i].sem, 1);

	init_waitqueue_head(&wq);
	init_waitqueue_head(&task_wq);
	init_waitqueue_head(&ioctl_wq);

	//Enable the FIFO
	*ASC_3_CTRL = *ASC_3_CTRL | ASC_CTRL_FIFO_EN;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
	i = request_irq(120, (void *)FP_interrupt, IRQF_DISABLED /* SA_INTERRUPT */, "FP_serial", NULL);
#else
	i = request_irq(120, (void *)FP_interrupt, SA_INTERRUPT, "FP_serial", NULL);
#endif

	if (!i)
	{
		*ASC_3_INT_EN = *ASC_3_INT_EN | 0x00000001;

	}
	else
	{
		printk("FP: ERR: Can't get irq 120\n");
		return -EINVAL;
	}

	kernel_thread(fpReceiverTask, NULL, 0);
	kernel_thread(fpTransmitterTask, NULL, 0);

#if 1
	msleep(1000);
	micom_init_func();
#endif

	if (register_chrdev(VFD_MAJOR, "VFD", &vfd_fops))
	{
		printk("FP: ERR: unable to get major %d for VFD/MICOM\n", VFD_MAJOR);

		//FIXME: stop threads, free irq

		return -EINVAL;
	}

	// input device init
	fp_button_dev = input_allocate_device();
	if (!fp_button_dev)
	{
		printk("FP: ERR: Not enough memory\n");

		//FIXME: stop threads, free irq
		return -ENOMEM;
	}


	printk("FP: register key events:");
	set_bit(EV_KEY, fp_button_dev->evbit);
	memset(fp_button_dev->keybit, 0, sizeof(fp_button_dev->keybit));
	for (i = 0; i < FP_KEYS_MAX; i++)
		if (fp_keys[i].key)
		{
			set_bit(fp_keys[i].key, fp_button_dev->keybit);
			printk(" [%s]", fp_keys[i].name);
		}
	printk("\n");

	fp_button_dev->name = "Frontpanel";

	i = input_register_device(fp_button_dev);
	if (i)
	{
		printk("FP: ERR: Failed to register input device\n");

		//FIXME: stop threads, free irq
		return -ENOMEM;
	}

	dprintk("%s <\n", __func__);
	return 0;
}

static void __exit micom_cleanup_module(void)
{
	printk("FP: unloading started: press any button to finish\n");

	if (leaving == 0)
		leaving = 1;

	while (leaving != 3)
		msleep(100);

	free_irq(120, NULL);

	if (fp_button_dev)
		input_unregister_device(fp_button_dev);
	unregister_chrdev(VFD_MAJOR, "VFD");


	printk("FP: VFD/MICOM module unloaded successfully\n");
}


module_init(micom_init_module);
module_exit(micom_cleanup_module);

module_param(debug, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, "Debug Output 0=disabled 1..=enabled(debuglevel)");

MODULE_DESCRIPTION("Frontpanel VFD/MICOM module for abcom ABIP 9900/99HD");
MODULE_AUTHOR("Dagobert,Sisyfos");
MODULE_LICENSE("GPL");
