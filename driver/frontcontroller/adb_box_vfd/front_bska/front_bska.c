#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/input.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/stm/pio.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include "../../vfd/utf.h"

#include "front_bska.h"

static int buttoninterval = 350/*ms*/;
static struct timer_list button_timer;

static char *button_driver_name = "NBOX frontpanel buttons";
static struct input_dev *button_dev;

struct vfd_driver
{
	struct semaphore      sem;
	int                   opencount;
};

spinlock_t mr_lock = SPIN_LOCK_UNLOCKED;

static Global_Status_t status;

static struct vfd_driver vfd;
static int bad_polling = 1;

static int debug  = 0;

static char ICON1	=	0;
static char ICON2	=	0;
static char ICON3	=	0;
static char ICON4	=	0;

static char button_reset = 0;

static struct workqueue_struct *wq;


#define DBG(fmt, args...) if ( debug ) printk(KERN_INFO "[vfd] :: " fmt "\n", ## args )
#define ERR(fmt, args...) printk(KERN_ERR "[vfd] :: " fmt "\n", ## args )

#define	PORT_STB	1
#define PIN_STB		6

#define PORT_CLK	4
#define PIN_CLK		0

#define PORT_DIN	4
#define PIN_DIN		1

#define PORT_DOUT	4
#define PIN_DOUT	1

#define PORT_KEY_INT	2
#define PIN_KEY_INT	2

#define VFD_Delay 	5

static unsigned char key_group1, key_group2;
static unsigned int  key_front = 0;

struct stpio_pin *pinreset1;
struct stpio_pin *pinreset2;
struct stpio_pin *key_int;
struct stpio_pin *din;
struct stpio_pin *clk;
struct stpio_pin *stb;

static void pt6958_free(void)
{
	if (stb) stpio_set_pin(stb, 1);
	if (din) stpio_free_pin(din);
	if (clk) stpio_free_pin(clk);
	if (stb) stpio_free_pin(stb);
	DBG("free PT6958\n");
};

static unsigned char pt6958_init(void)
{
	DBG("init pt6958\n");

	DBG("request stpio %d,%d", PORT_STB, PIN_STB);
	stb = stpio_request_pin(PORT_STB, PIN_STB, "PT6958_STB", STPIO_OUT);

	if (stb == NULL)
	{
		ERR("Request stpio scs failed. abort.");
		goto pt_init_fail;
	}

	DBG("request stpio %d,%d", PORT_CLK, PIN_CLK);
	clk = stpio_request_pin(PORT_CLK, PIN_CLK, "PT6958_CLK", STPIO_OUT);


	if (clk == NULL)
	{
		ERR("Request stpio clk failed. abort.");
		goto pt_init_fail;
	}

	DBG("request stpio %d,%d", PORT_DIN, PIN_DIN);
	din = stpio_request_pin(PORT_DIN, PIN_DIN, "PT6958_DIN", STPIO_BIDIR);

	if (din == NULL)
	{
		ERR("Request stpio din failed. abort.");
		goto pt_init_fail;
	}

	stpio_set_pin(stb, 1);
	stpio_set_pin(clk, 1);
	stpio_set_pin(din, 1);

	return 1;

pt_init_fail:
	pt6958_free();
	return 0;
};

static unsigned char PT6958_ReadChar(void)
{
	unsigned char i;
	unsigned char dinn = 0;

	for (i = 0; i < 8; i++)
	{
		stpio_set_pin(din, 1);
		stpio_set_pin(clk, 0);
		udelay(VFD_Delay);
		stpio_set_pin(clk, 1);
		udelay(VFD_Delay);
		dinn = (dinn >> 1) | (stpio_get_pin(din) > 0 ? 0x80 : 0);
	}
	return dinn;
}


static void PT6958_SendChar(unsigned char Value)
{
	unsigned char i;

	for (i = 0; i < 8; i++)
	{
		if (Value & 0x01)
			stpio_set_pin(din, 1);
		else
			stpio_set_pin(din, 0);

		stpio_set_pin(clk, 0);
		udelay(VFD_Delay);
		stpio_set_pin(clk, 1);
		udelay(VFD_Delay);
		Value >>= 1;

	}
}

static void PT6958_WriteChar(unsigned char Value)
{
	stpio_set_pin(stb, 0);
	PT6958_SendChar(Value);
	stpio_set_pin(stb, 1);
	udelay(VFD_Delay);
}

static void PT6958_WriteData(unsigned char *data, unsigned int len)
{
	unsigned char i;
	stpio_set_pin(stb, 0);
	for (i = 0; i < len; i++) PT6958_SendChar(data[i]);
	stpio_set_pin(stb, 1);
	udelay(VFD_Delay);
}


static void ReadKey(void)
{
	spin_lock(&mr_lock);

	stpio_set_pin(stb, 0);
	PT6958_SendChar(0x42);
	key_group1 = PT6958_ReadChar();
	key_group2 = PT6958_ReadChar();
	stpio_set_pin(stb, 1);
	udelay(VFD_Delay);

	spin_unlock(&mr_lock);
}

static void PT6958_Show(unsigned char DIG1, unsigned char DIG2, unsigned char DIG3, unsigned char DIG4, unsigned char DOT1, unsigned char DOT2, unsigned char DOT3, unsigned char DOT4)
{
	spin_lock(&mr_lock);

	PT6958_WriteChar(0x40); //Command 1 mode set,( Fixed address)  01xx????B
	udelay(VFD_Delay);

	stpio_set_pin(stb, 0);

	PT6958_SendChar(0xc0); //Command 2 address set,(start from 0 ľăÁÁcom1)   11xx????B
	if (DOT1 == 1)
		DIG1 = DIG1 + 0x80;
	PT6958_SendChar(DIG1);

	PT6958_SendChar(ICON1); //led power 01-czerwony 02-zielony
	if (DOT2 == 1)
		DIG2 = DIG2 + 0x80;
	PT6958_SendChar(DIG2);

	PT6958_SendChar(ICON2); //led zegar
	if (DOT3 == 1)
		DIG3 = DIG3 + 0x80;
	PT6958_SendChar(DIG3);

	PT6958_SendChar(ICON3); //led @
	if (DOT4 == 1)
		DIG4 = DIG4 + 0x80;
	PT6958_SendChar(DIG4);

	PT6958_SendChar(ICON4); //led !

	stpio_set_pin(stb, 1);
	udelay(VFD_Delay);

	spin_unlock(&mr_lock);
}

static int PT6958_ShowBuf(unsigned char *data, unsigned char len)
{
	/* eliminating UTF-8 chars first */
	unsigned char  kbuf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	int i = 0;
	int wlen = 0;

	while ((i < len) && (wlen < 8))
	{
		if (data[i] == '\n' || data[i] == 0)
		{
			DBG("[%s] BREAK CHAR detected (0x%X)\n", __func__, data[i]);
			break;
		}
		else if (data[i] < 0x20)
		{
			DBG("[%s] NON_PRINTABLE_CHAR '0x%X'\n", __func__, data[i]);
			i++;
		}
		else if (data[i] < 0x80)
		{
			kbuf[wlen] = data[i];
			DBG("[%s] ANSI_Char_Table '0x%X'\n", __func__, data[i]);
			wlen++;
		}
		else if (data[i] < 0xE0)
		{
			DBG("[%s] UTF_Char_Table= 0x%X", __func__, data[i]);
			switch (data[i])
			{
				case 0xc2:
					UTF_Char_Table = UTF_C2;
					break;
				case 0xc3:
					UTF_Char_Table = UTF_C3;
					break;
				case 0xc4:
					UTF_Char_Table = UTF_C4;
					break;
				case 0xc5:
					UTF_Char_Table = UTF_C5;
					break;
				case 0xd0:
					UTF_Char_Table = UTF_D0;
					break;
				case 0xd1:
					UTF_Char_Table = UTF_D1;
					break;
				default:
					UTF_Char_Table = NULL;
			}
			i++;
			if (UTF_Char_Table)
			{
				DBG("[%s] UTF_Char= 0x%X, index=%i", __func__, UTF_Char_Table[data[i] & 0x3f], i);
				kbuf[wlen] = UTF_Char_Table[data[i] & 0x3f];
				wlen++;
			}
		}
		else
		{
			if (data[i] < 0xF0)
				i += 2;
			else if (data[i] < 0xF8)
				i += 3;
			else if (data[i] < 0xFC)
				i += 4;
			else
				i += 5;
		}
		i++;
	}
	/* end */
	unsigned char  z1, z2, z3, z4, k1, k2, k3, k4;
	k1 = 0;
	k2 = 0;
	k3 = 0;
	k4 = 0;
	z1 = 0x20;
	z2 = 0x20;
	z3 = 0x20;
	z4 = 0x20;

	if ((wlen >= 2) && (kbuf[1] == '.' || kbuf[1] == "'" || kbuf[1] == ':'))
		k1 = 1;
	if ((wlen >= 3) && (kbuf[2] == '.' || kbuf[2] == "'" || kbuf[2] == ':'))
		k2 = 1;
	if ((wlen >= 4) && (kbuf[3] == '.' || kbuf[3] == "'" || kbuf[3] == ':'))
		k3 = 1;
	if ((wlen >= 5) && (kbuf[4] == '.' || kbuf[4] == "'" || kbuf[4] == ':'))
		k4 = 1;

	//assigning correct chars
	if (wlen >= 1) z1 = ROM[kbuf[0]];
	if (wlen >= 2) z2 = ROM[kbuf[1 + k1]];
	if (wlen >= 3) z3 = ROM[kbuf[2 + k1 + k2]];
	if (wlen >= 4) z4 = ROM[kbuf[3 + k1 + k2 + k3]];

	PT6958_Show(z1, z2, z3, z4, k1, k2, k3, k4);

	return 0;
}

static void pt6958_set_icon(unsigned char *kbuf, unsigned char len)
{
	unsigned char poz = 0, ico = 0;

	spin_lock(&mr_lock);

	if (len == 5)
	{
		poz = kbuf[0];
		ico = kbuf[4];

		if (poz == 1)
		{
			ICON1 = ico;        //led power 01-czerwony 02-zielony
			poz = 0xc1;
		}
		if (poz == 2)
		{
			if (ico == 1)
			{
				ICON1 = 2;
				ico = 2;
			}
			else
			{
				ICON1 = 0;
				ico = 0;
			}
			poz = 0xc1;
		} //led power 01-czerwony 02-zielony

		if (poz == 3)
		{
			ICON2 = ico;
			poz = 0xc3;
		}
		if (poz == 4)
		{
			ICON3 = ico;
			poz = 0xc5;
		}
		if (poz == 5)
		{
			ICON4 = ico;
			poz = 0xc7;
		}

		PT6958_WriteChar(0x44);     	//Command 1 mode set,( Fixed address)  01xx????B
		udelay(VFD_Delay);

		stpio_set_pin(stb, 0);
		PT6958_SendChar(poz);  			//Command 2 address set,(start from 0 ľăÁÁcom1)   11xx????B
		PT6958_SendChar(ico);
		stpio_set_pin(stb, 1);
		udelay(VFD_Delay);
	}
	spin_unlock(&mr_lock);
}

static void PT6958_setup(void)
{
	unsigned char i;

	PT6958_WriteChar(0x40);		//Command 1, increment address write
	udelay(VFD_Delay);

	stpio_set_pin(stb, 0);
	PT6958_SendChar(0xc0);          	//Command 2, RAM address = 0
	for (i = 0; i < 10; i++)            	//10 bytes
		PT6958_SendChar(0);
	stpio_set_pin(stb, 1);
	udelay(VFD_Delay);

	PT6958_WriteChar(0x8c);         //Command 3 display control, (Display ON) 10xx????B

	ICON1 = 0;
	ICON1 = 2;	//dioda zasilania zielona
	ICON2 = 0;
	ICON3 = 0;
	PT6958_Show(0x40, 0x40, 0x40, 0x40, 0, 0, 0, 0);
}

static void pt6958_set_brightness(int level)
{
	spin_lock(&mr_lock);

	if (level < 0) level = 0;
	if (level > 7) level = 7;
	PT6958_WriteChar(0x88 + level);       //Command 3 display control, (Display ON) 10xx????B

	spin_unlock(&mr_lock);
}

static void pt6958_set_lights(int onoff)
{
	spin_lock(&mr_lock);

	if (onoff == 1)
		PT6958_WriteChar(0x8c);         //Command 3 display control, (Display ON) 10xx????B
	else
		PT6958_WriteChar(0x80);         //Command 3 display control, (Display ON) 10xx????B

	spin_unlock(&mr_lock);
}





//#define	button_polling
//#define	button_interrupt
#define	button_interrupt2


#ifdef button_interrupt2
#warning !!!!!!!!! button_interrupt2 !!!!!!!!
//----------------------------------------------------------------------------------
static void button_delay(unsigned long dummy)
{
	if (button_reset == 100)
	{
		kernel_restart(NULL);
		//pinreset1 = stpio_request_pin( 3,2, "pinreset", STPIO_OUT );
		//pinreset2 = stpio_request_pin( 2,3, "pinreset", STPIO_OUT );
	}
	else
	{
		if (key_front > 0)
		{
			input_report_key(button_dev, key_front, 0);
			input_sync(button_dev);
			key_front = 0;
		}
		enable_irq(FPANEL_PORT2_IRQ);
	}
}


//void fpanel_irq_handler(void *dev1,void *dev2)
irqreturn_t fpanel_irq_handler(void *dev1, void *dev2)
{
	disable_irq(FPANEL_PORT2_IRQ);

	if ((stpio_get_pin(key_int) == 1) && (key_front == 0))
	{
		ReadKey();
		key_front = 0;
		if ((key_group1 == 32) && (key_group2 == 0)) key_front = KEY_MENU; //menu
		if ((key_group1 == 00) && (key_group2 == 4)) key_front = KEY_EPG; //epg
		if ((key_group1 == 04) && (key_group2 == 0)) key_front = KEY_HOME; //res
		if ((key_group1 == 16) && (key_group2 == 0)) key_front = KEY_UP; //up
		if ((key_group1 == 00) && (key_group2 == 1)) key_front = KEY_DOWN; //down
		if ((key_group1 == 64) && (key_group2 == 0)) key_front = KEY_RIGHT; //right
		if ((key_group1 == 00) && (key_group2 == 2)) key_front = KEY_LEFT; //left
		if ((key_group1 == 02) && (key_group2 == 0)) key_front = KEY_OK; //ok
		if ((key_group1 == 01) && (key_group2 == 0)) key_front = KEY_POWER; //pwr
		if (key_front > 0)
		{

			if (key_front == KEY_HOME)
			{
				button_reset++;
				if (button_reset > 4)
				{
					DBG("!!! Restart system !!!\n");
					button_reset = 100;
				}
			}
			else
				button_reset = 0;

			input_report_key(button_dev, key_front, 1);
			input_sync(button_dev);
			DBG("Key:%d\n", key_front);
			init_timer(&button_timer);
			button_timer.function = button_delay;
			mod_timer(&button_timer, jiffies + buttoninterval);
		}
		else
			enable_irq(FPANEL_PORT2_IRQ);
	}
	else
		enable_irq(FPANEL_PORT2_IRQ);

	return IRQ_HANDLED;
}


//----------------------------------------------------------------------------------
#endif

//----------------------------------------------------------------------------------

#ifdef button_interrupt
#warning !!!!!!!!! button_interrupt !!!!!!!!

// version interrupt
static void button_delay(unsigned long dummy)
{
	if (button_reset == 100)
		kernel_restart(NULL);
	//pinreset = stpio_request_pin( 3,2, "pinreset", STPIO_OUT );
	else
	{
		if (key_front > 0)
		{
			input_report_key(button_dev, key_front, 0);
			input_sync(button_dev);
			key_front = 0;
		}
		enable_irq(FPANEL_PORT2_IRQ);
	}
}


irqreturn_t fpanel_irq_handler(int irq, void *dev_id)
{
	disable_irq(FPANEL_PORT2_IRQ);

	if ((stpio_get_pin(key_int) == 1) && (key_front == 0))
	{
		ReadKey();
		key_front = 0;
		if ((key_group1 == 32) && (key_group2 == 0)) key_front = KEY_MENU; //menu
		if ((key_group1 == 00) && (key_group2 == 4)) key_front = KEY_EPG; //epg
		if ((key_group1 == 04) && (key_group2 == 0)) key_front = KEY_HOME; //res
		if ((key_group1 == 16) && (key_group2 == 0)) key_front = KEY_UP; //up
		if ((key_group1 == 00) && (key_group2 == 1)) key_front = KEY_DOWN; //down
		if ((key_group1 == 64) && (key_group2 == 0)) key_front = KEY_RIGHT; //right
		if ((key_group1 == 00) && (key_group2 == 2)) key_front = KEY_LEFT; //left
		if ((key_group1 == 02) && (key_group2 == 0)) key_front = KEY_OK; //ok
		if ((key_group1 == 01) && (key_group2 == 0)) key_front = KEY_POWER; //pwr
		if (key_front > 0)
		{
			if (key_front == KEY_HOME)
			{
				button_reset++;
				if (button_reset > 4)
				{
					DBG("!!! Restart system !!!\n");
					button_reset = 100;
				}
			}
			else
				button_reset = 0;
			input_report_key(button_dev, key_front, 1);
			input_sync(button_dev);
			DBG("Key:%d\n", key_front);
			init_timer(&button_timer);
			button_timer.function = button_delay;
			mod_timer(&button_timer, jiffies + buttoninterval);
		}
		else
			enable_irq(FPANEL_PORT2_IRQ);
	}
	else
		enable_irq(FPANEL_PORT2_IRQ);

	return IRQ_HANDLED;
}
#endif


#ifdef button_polling
#warning !!!!!!!!! button_queue !!!!!!!!
#endif
//----------------------------------------------------------------------------------
// verion queue
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
void button_bad_polling(void)
#else
void button_bad_polling(struct work_struct *ignored)
#endif
{
	while (bad_polling == 1)
	{
		msleep(10);
		if (stpio_get_pin(key_int) == 1)
		{
			ReadKey();
			key_front = 0;
			if ((key_group1 == 32) && (key_group2 == 0)) key_front = KEY_MENU; //menu
			if ((key_group1 == 00) && (key_group2 == 4)) key_front = KEY_EPG; //epg
			if ((key_group1 == 04) && (key_group2 == 0)) key_front = KEY_HOME; //res
			if ((key_group1 == 16) && (key_group2 == 0)) key_front = KEY_UP; //up
			if ((key_group1 == 00) && (key_group2 == 1)) key_front = KEY_DOWN; //down
			if ((key_group1 == 64) && (key_group2 == 0)) key_front = KEY_RIGHT; //right
			if ((key_group1 == 00) && (key_group2 == 2)) key_front = KEY_LEFT; //left
			if ((key_group1 == 02) && (key_group2 == 0)) key_front = KEY_OK; //ok
			if ((key_group1 == 01) && (key_group2 == 0)) key_front = KEY_POWER; //pwr
			if (key_front > 0)
			{
				if (key_front == KEY_HOME)
				{
					button_reset++;
					if (button_reset > 4)
					{
						DBG("!!! Restart system !!!\n");
						button_reset = 0;
						kernel_restart(NULL);
					}
				}
				else
					button_reset = 0;
				input_report_key(button_dev, key_front, 1);
				input_sync(button_dev);
				DBG("Key:%d\n", key_front);
				msleep(250);
				input_report_key(button_dev, key_front, 0);
				input_sync(button_dev);
			}
		}
	}//while
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static DECLARE_WORK(button_obj, button_bad_polling, NULL);
#else
static DECLARE_WORK(button_obj, button_bad_polling);
#endif

static int button_input_open(struct input_dev *dev)
{
#ifdef button_polling
	wq = create_workqueue("button");
	if (queue_work(wq, &button_obj))
	{
		DBG("[BTN] queue_work successful ...\n");
	}
	else
	{
		DBG("[BTN] queue_work not successful, exiting ...\n");
		return 1;
	}
#endif

	return 0;
}

static void button_input_close(struct input_dev *dev)
{
#ifdef button_polling
	bad_polling = 0;
	msleep(55);
	bad_polling = 1;

	if (wq)
	{
		destroy_workqueue(wq);
		DBG("[BTN] workqueue destroyed\n");
	}
#endif

}
//-------------------------------------------------------------------------------------------
// VFD
//-------------------------------------------------------------------------------------------

#define VFDIOC_DCRAMWRITE			0xc0425a00
#define VFDIOC_BRIGHTNESS			0xc0425a03
#define VFDIOC_DISPLAYWRITEONOFF	0xc0425a05
#define VFDIOC_DRIVERINIT			0xc0425a08
#define VFDIOC_ICONDISPLAYONOFF		0xc0425a0a

static int vfd_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct vfd_ioctl_data vfddata;

	switch (cmd)
	{
		case VFDIOC_DCRAMWRITE:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			return PT6958_ShowBuf(vfddata.data, vfddata.length);
			break;
		case VFDIOC_BRIGHTNESS:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			pt6958_set_brightness(vfddata.address);
			break;
		case VFDIOC_DISPLAYWRITEONOFF:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			pt6958_set_lights(vfddata.address);
			break;
		case VFDIOC_ICONDISPLAYONOFF:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			pt6958_set_icon(vfddata.data, vfddata.length);
			break;
		case VFDIOC_DRIVERINIT:
			PT6958_setup();
			break;
		default:
			ERR("[vfd] unknown ioctl %08x", cmd);
			break;
	}
	return 0;

}

static ssize_t vfd_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	DBG("[%s] text = '%s', len= %d\n", __func__, buf, len);
	if (len == 0)
		PT6958_ShowBuf("    ", 4);
	else
		PT6958_ShowBuf(buf, len);
	return len;
}

static ssize_t vfd_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	return len;
}

static int vfd_open(struct inode *inode, struct file *file)
{

	DBG("vfd_open");

	if (down_interruptible(&(vfd.sem)))
	{
		DBG("interrupted while waiting for sema.");
		return -ERESTARTSYS;
	}

	if (vfd.opencount > 0)
	{
		DBG("device already opened.");
		up(&(vfd.sem));
		return -EUSERS;
	}

	vfd.opencount++;
	up(&(vfd.sem));

	return 0;
}

static int vfd_close(struct inode *inode, struct file *file)
{
	DBG("vfd_close");
	vfd.opencount = 0;
	return 0;
}

static struct file_operations vfd_fops =
{
	.owner   = THIS_MODULE,
	.ioctl   = vfd_ioctl,
	.write   = vfd_write,
	.read    = vfd_read,
	.open    = vfd_open,
	.release = vfd_close
};

static void __exit vfd_module_exit(void)
{
	unregister_chrdev(VFD_MAJOR, "vfd");
#ifdef button_interrupt2
	stpio_free_irq(key_int);
#endif
	pt6958_free();
	input_unregister_device(button_dev);
}

static int __init vfd_module_init(void)
{

	int error;
	int res;

	DBG("Front Nbox BSKA init.");

	DBG("probe PT6958.");
	if (pt6958_init() == 0)
	{
		ERR("unable to init driver. abort.");
		goto vfd_init_fail;
	}

	DBG("register character device %d.", VFD_MAJOR);
	if (register_chrdev(VFD_MAJOR, "vfd", &vfd_fops))
	{
		ERR("register major %d failed", VFD_MAJOR);
		goto vfd_init_fail;
	}

	sema_init(&(vfd.sem), 1);
	vfd.opencount = 0;

	PT6958_setup();


	DBG("request stpio %d,%d", PORT_KEY_INT, PIN_KEY_INT);
	key_int = stpio_request_pin(PORT_KEY_INT, PIN_KEY_INT, "Key_int_front", STPIO_IN);

	if (key_int == NULL)
	{
		ERR("Request stpio key_int failed. abort.");
		goto vfd_init_fail;
	}

	stpio_set_pin(key_int, 1);

//stpio_flagged_request_irq(struct stpio_pin *pin, int comp,
//<------><------>       void (*handler)(struct stpio_pin *pin, void *dev),
//<------><------>       void *dev, unsigned long flags)

#ifdef button_interrupt2
	DBG(">stpio_flagged_request_irq IRQ_TYPE_LEVEL_LOW\n");
	stpio_flagged_request_irq(key_int, 0, fpanel_irq_handler, NULL, NULL);
	DBG("<stpio_flagged_request_irq IRQ_TYPE_LEVEL_LOW\n");
#endif

	button_dev = input_allocate_device();
	if (!button_dev)goto vfd_init_fail;
	button_dev->name = button_driver_name;
	button_dev->open = button_input_open;
	button_dev->close = button_input_close;

	set_bit(EV_KEY, button_dev->evbit);
	set_bit(KEY_MENU, button_dev->keybit);
	set_bit(KEY_EPG, button_dev->keybit);
	set_bit(KEY_HOME, button_dev->keybit);
	set_bit(KEY_UP, button_dev->keybit);
	set_bit(KEY_DOWN, button_dev->keybit);
	set_bit(KEY_RIGHT, button_dev->keybit);
	set_bit(KEY_LEFT, button_dev->keybit);
	set_bit(KEY_OK, button_dev->keybit);
	set_bit(KEY_POWER, button_dev->keybit);

	error = input_register_device(button_dev);
	if (error)
	{
		input_free_device(button_dev);
		ERR("Request input_register_device. abort.");
		goto vfd_init_fail;
	}


#ifdef button_interrupt
	status.pio_addr = 0;
	status.pio_addr = (unsigned long)ioremap(PIO2_BASE_ADDRESS, (unsigned long)PIO2_IO_SIZE);
	if (!status.pio_addr)
	{
		printk("button pio map: FATAL: Memory allocation error\n");
		goto vfd_init_fail;
	}

	status.button = BUTTON_RELEASED;

	ctrl_outl(BUTTON_PIN, status.pio_addr + PIO_CLR_PnC0);
	ctrl_outl(BUTTON_PIN, status.pio_addr + PIO_CLR_PnC1);
	ctrl_outl(BUTTON_PIN, status.pio_addr + PIO_CLR_PnC2);

	ctrl_outl(BUTTON_RELEASED, status.pio_addr + PIO_PnCOMP);
	ctrl_outl(BUTTON_PIN, status.pio_addr + PIO_PnMASK);

	res = request_irq(FPANEL_PORT2_IRQ, fpanel_irq_handler, IRQF_DISABLED, "button_irq" , NULL);
	if (res < 0)
	{
		printk("button irq: FATAL: Could not request interrupt\n");
		goto vfd_init_fail;
	}
#endif

	return 0;

vfd_init_fail:
	vfd_module_exit();
	return -EIO;
}

module_init(vfd_module_init);
module_exit(vfd_module_exit);

MODULE_DESCRIPTION("PT6958 frontpanel driver");
MODULE_AUTHOR("freebox");
MODULE_LICENSE("GPL");
