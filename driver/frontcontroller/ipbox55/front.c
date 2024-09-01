/*
 * kfront.c : kernel front display & key input device driver using IK2102 controller
 *
 * Copyright (C) 2010 Noolix
 *
 */

/*=============================================================================
	Includes
=============================================================================*/
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#include <linux/delay.h>
#include <linux/input.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include "dvb_ringbuffer.h"

#define FP55_MODULE_VERSION	104 // ex: 113 => version 1.13

/*=============================================================================
	Definitions
=============================================================================*/

#define REPEAT_KEY_SUPPORT
#ifdef REPEAT_KEY_SUPPORT
#define REPEAT_DEACCEL_SUPPORT
#endif

static int debug = 1;

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Turn on/off debugging (default:off).");

/*	04000000: 5-0, 5-1, 05000000: 1-0, 1-1	*/
static int pio_key1_port = 5;
static int pio_key1_bit = 0;
static int pio_key0_port = 5;
static int pio_key0_bit = 1;

module_param(pio_key1_port, int, 0644);
module_param(pio_key1_bit, int, 0644);
module_param(pio_key0_port, int, 0644);
module_param(pio_key0_bit, int, 0644);

#ifdef DEBUG
#define dprintk(fmt, args...) printk(fmt, ##args)
#else
#define dprintk(fmt, args...)
#endif

#define KFRONT_MAJOR	255

#define KEY_BITMASK     0x1F1F

#define PIO_CLOCK_PORT		5
#define PIO_CLOCK_BIT		1
#define PIO_DATA_PORT		5
#define PIO_DATA_BIT		0
#define PIO_STB_PORT		2
#define PIO_STB_BIT		7

//	front key code
#define	FRONT_STBY	    	0x01
#define	FRONT_VolUp		0x02
#define	FRONT_PgDn		0x04
#define	FRONT_PgUp		0x08
#define	FRONT_VolDn		0x10
#define	FRONT_EXIT		0x20
#define	FRONT_OK		0x40
#define	FRONT_Menu		0x80

#define KEY_BUF_SIZE	8

#define LOOP_DELAY	50
#define MSEC_TO_LOOP_CNT(x)	((x) / LOOP_DELAY)

#define KFRONT_IOC_BASE		'K'
#define KFRONT_IOC_DISPLAY	_IO(KFRONT_IOC_BASE, 0x01/*, struct dmx7101_tag*/)
#define KFRONT_IOC_MODE		_IO(KFRONT_IOC_BASE, 0x02/*, struct dmx7101_tag*/)
#define KFRONT_IOC_DISPLAY_MODE	_IO(KFRONT_IOC_BASE, 0x03/*, struct dmx7101_tag*/)

#define	VFDDISPLAYCHARS 	0xc0425a00
#define VFDSTANDBY		0xc0425afc
#define VFDDISPLAYCLEAR		0xc0425a82

#define DEV_DISP_DIGITS 8

/*=============================================================================
	Typedefs
=============================================================================*/

struct kfront_ioc_tag
{
	char str[DEV_DISP_DIGITS + 1];			/*	NULL terminating 4 bytes ASCII string	*/
	int mode;	/*	=0: standby, =1: run	*/
	int display_mode; /* 0=normal, 1=off, 2=all */
	int bright; /* -1=default, 0=stay current, >0=new value */
};


struct kfront_tag
{
	struct task_struct *th;
	int bQuit;
	struct stpio_pin *clk;
	struct stpio_pin *data;
	struct stpio_pin *stb;
};

/*=============================================================================
	Import Functions
=============================================================================*/

/*=============================================================================
	Import Variables
=============================================================================*/

/*=============================================================================
	Global Variables
=============================================================================*/

/*=============================================================================
	Private variables (static)
=============================================================================*/
static struct kfront_tag *kfront = NULL;
static int kfront_mode = 1;
static int kfront_display_mode = 0;
struct dvb_ringbuffer	 ci_rbuffer;
static struct semaphore sem;
static struct semaphore sem_lock;

static int display_flag = 0;
static unsigned char display_data[8];

const static u8 ascii_seg[256] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	//	0x00:
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	//	0x10:
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x00, 	//	0x20:
	0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	//	0x30: 0~9
	/*       0     1     2     3     4     5     6     7     8     9                                       */
	0x00, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x3d, 0x76, 0x06, 0x1e, 0x70, 0x38, 0x37, 0x37, 0x3f, 	//	0x40: 0x41: A~Z
	/*             A     B     C     D     E     F     G     H     I     J     K     L     M      N    O   */
	0x73, 0x67, 0x50, 0x6d, 0x78, 0x3e, 0x3e, 0x6a, 0x49, 0x6e, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x00, 	//	0x50:
	/*       P     Q     R     S     T     U     V     W     X     Y     Z                                 */
	0x00, 0x5f, 0x7c, 0x58, 0x5e, 0x7b, 0x71, 0x3d, 0x74, 0x10, 0x0e, 0x70, 0x38, 0x54, 0x54, 0x5c, 	//	0x60 0x61: a~z
	/*             a     b     c     d     e     f     g     h     i     j     k     l     m      n    o   */
	0x73, 0x67, 0x50, 0x6d, 0x78, 0x1c, 0x1c, 0x1c, 0x49, 0x6e, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x00,	        //	0x70:
	/*       p     q     r     s     t     u      v    w     x     y     z                                 */
	0x73, 0x58, 0x78, 0x3e, 0x5e, 0x76, 0x3e, 0x66, 0x7e, 0x7e, 0x00, 0x1c, 0x00, 0x4f, 0x7f, 0x67, 	//	0x80:
	0x77, 0x7d, 0x7f, 0x31, 0x5f, 0x79, 0x32, 0x4f, 0x56, 0x56, 0x70, 0x57, 0x72, 0x76, 0x3f, 0x37, 	//	0x90: Cyrillic
	0x73, 0x39, 0x07, 0x3e, 0x5e, 0x76, 0x3e, 0x66, 0x7e, 0x7e, 0x00, 0x7e, 0x00, 0x4f, 0x7f, 0x67, 	//	0xA0: Cyrillic
	0x77, 0x7d, 0x7f, 0x50, 0x5f, 0x79, 0x32, 0x4f, 0x98, 0x98, 0x70, 0x57, 0x55, 0x54, 0x3f, 0x74, 	//	0xB0: Cyrillic
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	//	0xC0:
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	//	0xD0:
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	//	0xE0:
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00		//	0xF0:
};

static struct input_dev *fp_button_dev; // input device

#define FP_KEYS_MAX	8
#define FP_KEY_LEN	1

struct fp_keys_s
{
	unsigned char raw; //FP_KEY_LEN
	int key;
	char name[16];
} fp_keys[FP_KEYS_MAX] =
{
	{ FRONT_STBY, KEY_POWER, "KEY POWER" },
	{ FRONT_PgUp, KEY_UP, "KEY UP" },
	{ FRONT_PgDn, KEY_DOWN, "KEY DOWN" },
	{ FRONT_VolUp, KEY_LEFT, "KEY LEFT" },
	{ FRONT_VolDn, KEY_RIGHT, "KEY RIGHT" },
	{ FRONT_EXIT, KEY_EXIT, "KEY EXIT" },
	{ FRONT_OK, KEY_OK, "KEY OK" },
	{ FRONT_Menu, KEY_MENU, "KEY MENU" },
};

#ifdef REPEAT_KEY_SUPPORT
static u32 prev_key_data = 0;
#ifdef REPEAT_DEACCEL_SUPPORT
static u32 repeat_key_count = 0;
#endif
static u32 cur_loop_count = 0;
static u32 prev_loop_count = 0;
#endif

/*=============================================================================
	Functions prototypes
=============================================================================*/

/*=============================================================================
	Functions definitions
=============================================================================*/

static int translate_fp_key(unsigned int data, int dlen)
{
	int i;

	for (i = 0; i < FP_KEYS_MAX; i++)
		if ((unsigned char)data == fp_keys[i].raw)
		{
#ifdef DEBUG_DEV
			printk(" => found key [%s] = %x\n", fp_keys[i].name, fp_keys[i].key);
#endif
			return fp_keys[i].key;
		}
	return 0;
}

void delay(int times)             /*delay times*/
{
	udelay(times);
}

void longdelay(unsigned int times)             /*delay times*/
{
	msleep(times);
}


void read_bit(void)
{
	/*data read command rising edge*/
	stpio_set_pin(kfront->clk, 0);
	delay(1);
	stpio_set_pin(kfront->clk, 1);
	delay(1);
}

void command(unsigned int a)              /* command1~4 8 bits data */
{
	int i;

	stpio_set_pin(kfront->clk, 1);

	for (i = 0 ; i < 8 ; i++)
	{
		if (a & 1)
			stpio_set_pin(kfront->data, 1);
		else
			stpio_set_pin(kfront->data, 0);
		delay(1);
		a >>= 1;
		read_bit();
	}
}

void stb_enable(void)               /*stb pin process*/
{
	stpio_set_pin(kfront->stb, 1);

	stpio_set_pin(kfront->clk, 1);
	delay(5);
	stpio_set_pin(kfront->stb, 0);

	delay(1);

}

void initial(void)
{
	stb_enable();
	delay(5);

	command(0x02);
	stb_enable(); 		/* command1 display mode setting ( 6 digits, 12 segments ) */

	command(0x80);
	stb_enable(); 		/* command4 display off setting */

	command(0x02); 	/* command1 display mode setting ( 6 digits, 12 segments ) */
	stb_enable();

	command(0x8e); 	/* command4 display on setting */
	stb_enable();

	command(0x02); 	/* command1 display mode setting ( 6 digits, 12 segments ) */
	stb_enable();

	command(0x8e);	         /* command4 display on setting */
	stb_enable();
}


void full(unsigned char value)
{
	unsigned char i;

	command(0x02);
	stb_enable(); 		/* command1 display mode setting ( 6 digits, 12 segments ) */

	command(0x40);	/* command2 write data to Display increment mode*/
	stb_enable();

	command(0xc0);  	/* command3  address and data write setting*/
	for (i = 0xc0 ; i <= 0xcd ; i ++)
		command(value);
	stb_enable();

	command(0x8e);  /* command4 display on setting */
	stb_enable();
}

/*----------------------------------------------------------------------------------------*/

#ifdef REPEAT_KEY_SUPPORT
int is_repeat_key(u32 key)
{
	return (key == FRONT_VolUp || key == FRONT_VolDn || key == FRONT_PgDn || key == FRONT_PgUp);
}
#endif

void scan_key(void)
{
	int i, j;
	u32 key_data = 0;
	unsigned char key_buf[5];

	memset(key_buf, 0, sizeof(key_buf));

	command(0x42);                  /*command2 read key data to Display increment mode*/

	stpio_set_pin(kfront->data, 1);

	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 8; j++)
		{
			stpio_set_pin(kfront->clk, 1);
			delay(1);
			stpio_set_pin(kfront->clk, 0);
			delay(1);
			if (stpio_get_pin(kfront->data))
			{
				key_buf[i] |= (1 << j);
			}
		}
	}

#if 0
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 8; j++)
		{
			dprintk("%d ", (key_buf[i] >> j) & 0x01);
		}
		dprintk("\n");
	}
	dprintk("\n");
#endif

	stpio_set_pin(kfront->clk, 1);

	/*	parse key	*/
	if (key_buf[0] & 0x01)			/*	exit		*/
		key_data = FRONT_EXIT;
	else if (key_buf[0] & 0x08)		/*	ok		*/
		key_data = FRONT_OK;
	else if (key_buf[1] & 0x01)		/*	vol+		*/
		key_data = FRONT_VolUp;
	else if (key_buf[1] & 0x08)		/*	vol-		*/
		key_data = FRONT_VolDn;
	else if (key_buf[2] & 0x01)		/*	power	*/
		key_data = FRONT_STBY;
	else if (key_buf[2] & 0x08)		/*	ch-		*/
		key_data = FRONT_PgDn;
	else if (key_buf[3] & 0x01)		/*	menu	*/
		key_data = FRONT_Menu;
	else if (key_buf[3] & 0x08)		/*	ch+		*/
		key_data = FRONT_PgUp;

	if (key_data)
	{
		int key;
#ifdef REPEAT_KEY_SUPPORT
#ifdef REPEAT_DEACCEL_SUPPORT
		if (prev_key_data != key_data)
			repeat_key_count = 0;

		repeat_key_count++;
#endif
		if (is_repeat_key(key_data))
		{
#ifdef REPEAT_DEACCEL_SUPPORT
			if (repeat_key_count > 1 && repeat_key_count <= MSEC_TO_LOOP_CNT(500))
				return;
#endif
		}
		else
		{
			if (prev_key_data == key_data)
				return;
		}

		prev_key_data = key_data;
		prev_loop_count = cur_loop_count;
#endif

		// event subsystem passing
		key = translate_fp_key(key_data, 5);
		if (key)
		{
			input_report_key(fp_button_dev, key, 1);
			input_sync(fp_button_dev);
			msleep(100);
			input_report_key(fp_button_dev, key, 0);
			input_sync(fp_button_dev);
			//FIXME: Add delay to not report one key press ducplicated
		}

		DVB_RINGBUFFER_WRITE_BYTE(&ci_rbuffer, key_data);
		wake_up_interruptible(&ci_rbuffer.queue);
	}
	else
	{
#ifdef REPEAT_KEY_SUPPORT
		/*	delete chattering	*/
		if ((prev_loop_count + MSEC_TO_LOOP_CNT(200)) <= cur_loop_count)
		{
			prev_key_data = 0;
#ifdef REPEAT_DEACCEL_SUPPORT
			repeat_key_count = 0;
#endif
		}
#endif
	}
}


unsigned char get_display_font(unsigned char ch)
{
	if (ch > 255)
		return 0x00;
	else
		return ascii_seg[ch];
}

void display_msg(unsigned char data[])
{
	int i;
	int n = 8;

	stb_enable();

	command(0x40);	/*command2 write data to Display increment mode*/
	stb_enable();

	command(0xc0);  	/*command3  address and data write setting*/
	for (i = 0; i < n; i++)
	{
		dprintk("[%s] msg data = %d\n", __func__, data[i]);
		if ((data[i] != 208) && (data[i] != 209))
		{
			command(get_display_font(data[i])); 	/*	display 4 digits	*/
			command(0x00); 			/*	display 4 digits	*/
		}
	}

	for (i = 0; i < 2; i++)
	{
		command(0x00);		/*	display dummy 2 digits	*/
		command(0x00);
	}

	stb_enable();

	command(0x8e);	/* command4 display on setting */
	stb_enable();
}

void toggle_time_pulse(void)
{
	static int on = 0;

	stb_enable();

	command(0x40);	/*command2 write data to Display increment mode*/
	stb_enable();

	command(0xca);  	/*command3  address and data write setting*/
	if (on)
	{
		command(0xff);
		command(0xff);
		on = 0;
	}
	else
	{
		command(0x00);
		command(0x00);
		on = 1;
	}

	stb_enable();

	command(0x8e);	/* command4 display on setting */
	stb_enable();
}



/*=============================================================================
	Function	: kfront_open
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
static int kfront_open(struct inode *inode, struct file *file)
{
	dprintk("%s: \n", __func__);
	dvb_ringbuffer_flush_spinlock_wakeup(&ci_rbuffer);
	return 0;
}


/*=============================================================================
	Function	: kfront_close
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
/*
static int kfront_close(struct inode *inode, struct file *file)
{
	dprintk("%s: \n", __func__);
	return 0;
}
*/

/*=============================================================================
	Function	: kfront_read
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
static ssize_t kfront_read(struct file *file, char __user *buf, size_t count,
			   loff_t *ppos)
{
	int avail;
	int non_blocking = file->f_flags & O_NONBLOCK;

	if (!ci_rbuffer.data || !count)
		return 0;
	if (non_blocking && (dvb_ringbuffer_empty(&ci_rbuffer)))
		return -EWOULDBLOCK;
	if (wait_event_interruptible(ci_rbuffer.queue,
				     !dvb_ringbuffer_empty(&ci_rbuffer)))
		return -ERESTARTSYS;
	avail = dvb_ringbuffer_avail(&ci_rbuffer);
	if (avail < 1)
		return 0;

	return dvb_ringbuffer_read_user(&ci_rbuffer, buf, 1);
}

/*=============================================================================
	Function	: kfront_write
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
static ssize_t kfront_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{

	char *kbuf;

	dprintk("%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	kbuf = kmalloc(len + 1, GFP_KERNEL);

	if (kbuf == NULL)
	{
		printk("FP: no free memory\n");
		return -ENOMEM;
	}
	copy_from_user(kbuf, buff, len);
	kbuf[len] = '\0';

	if (down_interruptible(&sem_lock))
		return -ERESTARTSYS;

	display_msg(kbuf);
	kfree(kbuf);
	up(&sem_lock);

	return len;
}

/*=============================================================================
	Function	: kfront_ioctl
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
static int kfront_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	struct kfront_ioc_tag kfront_info;
	void __user *argp = (void __user *)arg;

	dprintk("%s: \n", __func__);

	switch (cmd)
	{
		case KFRONT_IOC_DISPLAY:
		case VFDDISPLAYCHARS:
			dprintk("%s: KFRONT_IOC_DISPLAY\n", __func__);
			if (copy_from_user(&kfront_info, argp, sizeof(struct kfront_ioc_tag)))
				return -EFAULT;

			if (down_interruptible(&sem_lock))
				return -ERESTARTSYS;

			memcpy(display_data, kfront_info.str, 8);
			dprintk("%s: %s\n", __func__, kfront_info.str);
			display_flag = 1;
			up(&sem_lock);
			break;

		case KFRONT_IOC_MODE:
		case VFDSTANDBY:
			dprintk("%s: KFRONT_IOC_MODE\n", __func__);
			if (copy_from_user(&kfront_info, argp, sizeof(struct kfront_ioc_tag)))
				return -EFAULT;

			dprintk("%s: mode=%d\n", __func__, kfront_info.mode);
			kfront_mode = kfront_info.mode;
			break;

		case KFRONT_IOC_DISPLAY_MODE:
		case VFDDISPLAYCLEAR:
			dprintk("%s: KFRONT_IOC_DISPLAY_MODE\n", __func__);
			if (copy_from_user(&kfront_info, argp, sizeof(struct kfront_ioc_tag)))
				return -EFAULT;

			dprintk("%s: display_mode=%d\n", __func__, kfront_info.display_mode);
			kfront_display_mode = kfront_info.display_mode;

			break;

		default:
			return -EINVAL;
	}

	return 0;
}


/*=============================================================================
	Function	: kfront_poll
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
static unsigned int kfront_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	dprintk("%s: \n", __func__);

	poll_wait(file, &ci_rbuffer.queue, wait);

	if (!dvb_ringbuffer_empty(&ci_rbuffer))
		mask |= (POLLIN | POLLRDNORM);

	return mask;
}


static struct file_operations kfront_fops =
{
	.owner = THIS_MODULE,
	.read = kfront_read,
	.write = kfront_write,
	.ioctl = kfront_ioctl,
	.open = kfront_open,
	.poll = kfront_poll,
};


/*=============================================================================
	Function	: kfront_thread
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
static int kfront_thread(void *__us)
{
	struct kfront_tag *us = (struct kfront_tag *)__us;
	int pulse = 0;
	display_msg("----");

	while (!us->bQuit)
	{
		scan_key();
		msleep(LOOP_DELAY);
#ifdef REPEAT_KEY_SUPPORT
		cur_loop_count++;
#endif
		if (kfront_mode)		//	run
		{
			if (kfront_display_mode == 1)
				full(0x00);
			else if (kfront_display_mode == 2)
				full(0xff);
			else
			{
			}
		}
		else		//	standby
		{
			/*	display time pulse	*/
			pulse++;
			if (pulse >= (500 / LOOP_DELAY))
			{
				toggle_time_pulse();
				pulse = 0;
			}
		}

		/*	display new string	*/
		if (display_flag)
		{
			display_msg(display_data);
			display_flag = 0;
		}
	}

}

/*=============================================================================
	Function	: kfront_init
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
int kfront_init(void)
{
	void *data;
	int rv = -1;
	int i;

	printk("FP driver for abip 55hd\nVersion %d.%02d (c) 2010 Sysifos\n", FP55_MODULE_VERSION / 100, FP55_MODULE_VERSION % 100);
	printk("Built %s %s\n", __DATE__, __TIME__);

	sema_init(&sem, 0);
	sema_init(&sem_lock, 1);

	/*	alloc key buffer	*/
	data = vmalloc(KEY_BUF_SIZE);
	if (!data)
		return -ENOMEM;

	dvb_ringbuffer_init(&ci_rbuffer, data, KEY_BUF_SIZE);

	kfront = vmalloc(sizeof(struct kfront_tag));
	if (!kfront)
		return -ENOMEM;

	/*	open pio	*/
	kfront->clk = stpio_request_pin(PIO_CLOCK_PORT, PIO_CLOCK_BIT, "clock", STPIO_OUT);
	kfront->data = stpio_request_pin(PIO_DATA_PORT, PIO_DATA_BIT, "data", STPIO_BIDIR/*STPIO_OUT*/);
	kfront->stb = stpio_request_pin(PIO_STB_PORT, PIO_STB_BIT, "stb", STPIO_OUT);

	if (!kfront->clk || !kfront->data || !kfront->stb)
	{
		dprintk("%s: kfront->clk=%p, kfront->data=%p kfront->stb=%p open error\n", __func__, kfront->clk, kfront->data, kfront->stb);
		goto error1;
	}

	/*	initialize	*/
	initial();

	/*	start task	*/
	kfront->bQuit = 0;

	kfront->th = kthread_create(kfront_thread, kfront, "kfront");

	if (IS_ERR(kfront->th))
	{
		dprintk("%s: unable to start task\n", __func__);
		goto error2;
	}
	else
	{
		wake_up_process(kfront->th);
	}

	/*	register device	*/
	if (register_chrdev(KFRONT_MAJOR, "kfront0", &kfront_fops))
	{
		dprintk("%s:: Unable to register driver\n", __func__);
		goto error3;
	}

	// input device init
	fp_button_dev = input_allocate_device();
	if (!fp_button_dev)
	{
		printk("FP: ERR: Not enough memory\n");

		rv = -ENOMEM;
		goto error3;
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

	if (input_register_device(fp_button_dev))
	{
		printk("FP: ERR: Failed to register input device\n");

		rv = -ENOMEM;
		goto error3;
	}

	return 0; // all is ok


error3:
	if (kfront->th)
	{
		kfront->bQuit = 1;
		kthread_stop(kfront->th);
	}
error2:
	if (kfront->clk)
		stpio_free_pin(kfront->clk);
	if (kfront->data)
		stpio_free_pin(kfront->data);
	if (kfront->stb)
		stpio_free_pin(kfront->stb);
error1:
	if (kfront)
	{
		vfree(kfront);
		kfront = NULL;
	}
	if (data)
		vfree(data);

	return -1;
}


/*=============================================================================
	Function	: kfront_release
	Description	:
	Input		:
	Output		:
	Return		:
=============================================================================*/
void kfront_release(void)
{
	dprintk("%s: \n", __func__);

	if (!kfront)
		return;

	if (fp_button_dev)
		input_unregister_device(fp_button_dev);

	/*	close task	*/
	if (kfront->th)
	{
		kfront->bQuit = 1;
		kthread_stop(kfront->th);
	}

	/*	close pio	*/
	if (kfront->clk)
		stpio_free_pin(kfront->clk);
	if (kfront->data)
		stpio_free_pin(kfront->data);
	if (kfront->stb)
		stpio_free_pin(kfront->stb);

	/*	unregister device	*/
	unregister_chrdev(KFRONT_MAJOR, "kfront0");

	vfree(kfront);
	kfront = NULL;

	/*	free key buff	*/
	vfree(ci_rbuffer.data);
	ci_rbuffer.data = NULL;
}


module_init(kfront_init);
module_exit(kfront_release);

MODULE_DESCRIPTION("7 segment & key input device driver for abip 55hd");
MODULE_AUTHOR("Sisyfos band");
MODULE_LICENSE("GPL");
