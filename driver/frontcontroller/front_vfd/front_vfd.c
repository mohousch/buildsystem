#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/delay.h>

#include <linux/workqueue.h>
#include <linux/input.h>

#include <linux/gpio.h>
#include <linux/stm/gpio.h>
#include "asc.h"
#include "../vfd/utf.h"

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif

#define LED1 stm_gpio(14, 1)
#define LED2 stm_gpio(14, 2)

#define KEY_L_	stm_gpio(10, 2)
#define KEY_R_	stm_gpio(10, 3)

//#define KEY_R	stm_gpio(6, 6)
//#define KEY_L	stm_gpio(6, 7)
//#define KEY_PWR		stm_gpio(6, 5)

static char *button_driver_name = "SagemCom88 frontpanel buttons";
static struct input_dev *button_dev;
static struct workqueue_struct *wq;
static int bad_polling = 1;

#define VFD_MAJOR	147

struct vfd_ioctl_data
{
	unsigned char address;
	unsigned char data[64];
	unsigned char length;
};

struct vfd_driver
{
	struct semaphore      sem;
	int opencount;
};

spinlock_t mr_lock = SPIN_LOCK_UNLOCKED;

static struct vfd_driver vfd;
static short paramDebug = 0;
static unsigned char jasnosc = 0xff;

#define DBG(fmt, args...) if ( paramDebug > 0 ) printk(KERN_INFO "[vfd] :: " fmt "\n", ## args )
#define ERR(fmt, args...) printk(KERN_ERR "[vfd] :: " fmt "\n", ## args )

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
void button_bad_polling(void)
#else
void button_bad_polling(struct work_struct *ignored)
#endif
{
	unsigned int  key_front;

	while (bad_polling == 1)
	{
		msleep(100);

//DBG("-%d\n",gpio_get_value(KEY_LEFT));

		key_front = 0;
		//if(gpio_get_value(KEY_L)==0) key_front=KEY_LEFT;
		if (gpio_get_value(KEY_L_) == 0) key_front = KEY_LEFT;
		//if(gpio_get_value(KEY_PWR)==0) key_front=KEY_POWER;
		//if(gpio_get_value(KEY_R)==0) key_front=KEY_RIGHT;
		if (gpio_get_value(KEY_R_) == 0) key_front = KEY_RIGHT;

		if (key_front > 0)
		{
			input_report_key(button_dev, key_front, 1);
			input_sync(button_dev);
			DBG("Key:%d 0x%x\n", key_front, key_front);
			msleep(250);
			input_report_key(button_dev, key_front, 0);
			input_sync(button_dev);
		}

	}//while
	DBG("[BTN] button_bad_polling END\n");
	bad_polling = 2;
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static DECLARE_WORK(button_obj, button_bad_polling, NULL);
#else
static DECLARE_WORK(button_obj, button_bad_polling);
#endif

static int button_input_open(struct input_dev *dev)
{
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
	return 0;
}

static void button_input_close(struct input_dev *dev)
{
	DBG("[BTN] button_input_close\n");
	bad_polling = 0;
	while (bad_polling != 2)
		msleep(1);
	bad_polling = 1;

	if (wq)
	{
		DBG("[BTN] workqueue destroy start\n");
		destroy_workqueue(wq);
		DBG("[BTN] workqueue destroyed\n");
	}
}



unsigned int ASCXBaseAddress = ASC2BaseAddress;

//-------------------------------------

//#define PCLK			(get_peripheral_clk_rate())
//#define BAUDRATE_VAL_M0(bps)	(PCLK / (16 * (bps)))
#define PCLK 100000000
#define BAUDRATE_VAL_M1(bps)	((((bps * (1 << 14))+ (1<<13)) / (PCLK/(1 << 6))))

void serial_init(void)
{
	*(unsigned int *)(ASCXBaseAddress + ASC_INT_EN)   = 0x00000000; // TODO: Why do we set here the INT_EN again ???
	*(unsigned int *)(ASCXBaseAddress + ASC_CTRL)     = 0x00001589;
	*(unsigned int *)(ASCXBaseAddress + ASC_TIMEOUT)  = 0x00000010;
	*(unsigned int *)(ASCXBaseAddress + ASC_BAUDRATE) = 0x4b7; //BAUDRATE_VAL_M1(115200);//0x000000c9;
	*(unsigned int *)(ASCXBaseAddress + ASC_TX_RST)   = 0;
	*(unsigned int *)(ASCXBaseAddress + ASC_RX_RST)   = 0;
}

int serial_putc(char Data)
{
	char                  *ASC_3_TX_BUFF = (char *)(ASCXBaseAddress + ASC_TX_BUFF);
	unsigned int          *ASC_3_INT_STA = (unsigned int *)(ASCXBaseAddress + ASC_INT_STA);
	unsigned long         Counter = 200000;

	while (((*ASC_3_INT_STA & ASC_INT_STA_THE) == 0) && --Counter)
	{
		udelay(0);
	}

	if (Counter == 0)
	{
		printk("Error writing char (%c) \n", Data);
	}

	*ASC_3_TX_BUFF = Data;
	return 1;
}

int WriteChars(unsigned char *aData, int aSize)
{
	int iii;

	for (iii = 0 ; iii < aSize ; iii++) serial_putc((aData[iii]));
	return 0;

}

int ESI88_WriteFront(unsigned char *data, unsigned char len)
{
	unsigned char wdata[24] = {0x87, 0xff, 0x87, 0xff, 0x87, 0xff, 0x4f, 0x10, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
	int i = 0;
	int j = 8;

	wdata[1] = jasnosc;
	wdata[3] = jasnosc;
	wdata[5] = jasnosc;

	while ((i < len) && (j < 8 + 16))
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
			wdata[j] = data[i];
			DBG("[%s] data[%i] = '0x%X'\n", __func__, j, data[i]);
			j++;
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
				wdata[j] = UTF_Char_Table[data[i] & 0x3f];
				j++;
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

	if (j < 8 + 16)
	{
		for (i = j; i < 8 + 16; i++)
		{
			wdata[i] = 0x20;
		}
	}
	WriteChars(wdata, 24);
	return 0;
}


static void ESI88_set_icon(unsigned char *kbuf, unsigned char len)
{
	spin_lock(&mr_lock);

	switch (kbuf[0])
	{
		case 1://czer
			if (kbuf[4] == 1) gpio_set_value(LED1, 1);
			else gpio_set_value(LED1, 0);
			break;
		case 2://ziel
			if (kbuf[4] == 1) gpio_set_value(LED2, 1);
			else gpio_set_value(LED2, 0);
			break;
		default:
			ERR("icon unknown %d", kbuf[0]);
			break;
	}

	spin_unlock(&mr_lock);
}


static int ESI88_ShowBuf(unsigned char *kbuf, unsigned char len)
{
	return 0;
}

static void ESI88_setup(void)
{
}

static void ESI88_set_brightness(int level)
{
	spin_lock(&mr_lock);
	if (level < 0) level = 0;
	if (level > 4) level = 4;
	if (level == 4)jasnosc = 0xff;
	if (level == 3)jasnosc = 0xfe;
	if (level == 2)jasnosc = 0xfc;
	if (level == 1)jasnosc = 0xf8;
	if (level == 0)jasnosc = 0xf0;
	spin_unlock(&mr_lock);
}

static void ESI88_set_lights(int onoff)
{
	spin_lock(&mr_lock);
	spin_unlock(&mr_lock);
}



//-------------------------------------------------------------------------------------------
// VFD
//-------------------------------------------------------------------------------------------

#define VFDIOC_DCRAMWRITE			0xc0425a00
#define VFDIOC_BRIGHTNESS			0xc0425a03
#define VFDIOC_DISPLAYWRITEONOFF		0xc0425a05
#define VFDIOC_DRIVERINIT			0xc0425a08
#define VFDIOC_ICONDISPLAYONOFF			0xc0425a0a

static int vfd_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct vfd_ioctl_data vfddata;

	switch (cmd)
	{
		case VFDIOC_DCRAMWRITE:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			return ESI88_WriteFront(vfddata.data, vfddata.length);
			break;
		case VFDIOC_BRIGHTNESS:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			ESI88_set_brightness(vfddata.address);
			break;
		case VFDIOC_DISPLAYWRITEONOFF:
//    copy_from_user( &vfddata, (void*)arg, sizeof( struct vfd_ioctl_data ) );
//    ESI88_set_lights(vfddata.address );
			break;
		case VFDIOC_ICONDISPLAYONOFF:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			ESI88_set_icon(vfddata.data, vfddata.length);
			break;
		case VFDIOC_DRIVERINIT:
//	    ESI88_setup();
			break;
		default:
			ERR("[vfd] unknown ioctl %08x", cmd);
			break;
	}
	return 0;

}

static ssize_t vfd_write(struct file *filp, const unsigned char *buf, size_t len, loff_t *off)
{
	DBG("[%s] text = '%s', len= %d\n", __func__, buf, len);
	if (len == 0)
		ESI88_WriteFront("                ", 16);
	else
		ESI88_WriteFront(buf, len);

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

static void __exit led_module_exit(void)
{
	unregister_chrdev(VFD_MAJOR, "vfd");
	input_unregister_device(button_dev);
}

static int __init led_module_init(void)
{
	int error;

	DBG("LED SagemCom88 init.");
	DBG("register character device %d.", VFD_MAJOR);
	if (register_chrdev(VFD_MAJOR, "vfd", &vfd_fops))
	{
		ERR("register major %d failed", VFD_MAJOR);
		goto led_init_fail;
	}

	// Address for FiFo enable/disable
	unsigned int         *ASC_X_CTRL       = (unsigned int *)(ASCXBaseAddress + ASC_CTRL);

	serial_init();

	//Enable the FIFO
	*ASC_X_CTRL = *ASC_X_CTRL | ASC_CTRL_FIFO_EN;

	sema_init(&(vfd.sem), 1);
	vfd.opencount = 0;

	gpio_request(LED1, "LED1");
	if (LED1 == NULL)
	{
		ERR("Request LED1 failed. abort.");
		goto led_init_fail;
	}
	gpio_direction_output(LED1, STM_GPIO_DIRECTION_OUT);
	gpio_set_value(LED1, 0);//czerwona

	gpio_request(LED2, "LED2");
	if (LED2 == NULL)
	{
		ERR("Request LED2 failed. abort.");
		goto led_init_fail;
	}
	gpio_direction_output(LED2, STM_GPIO_DIRECTION_OUT);
	gpio_set_value(LED2, 1);//zielona

//Key P- PIO10/2
//Key P+ PIO10/3

//gpio_request(KEY_L, "KEY_LEFT");
//if (KEY_L==NULL){ERR("Request KEY_LEFT failed. abort.");goto led_init_fail;}
//gpio_direction_output(KEY_LEFT, STM_GPIO_DIRECTION_IN);
//gpio_direction_input(KEY_L);

//gpio_request(KEY_R, "KEY_RIGHT");
//if (KEY_R==NULL){ERR("Request KEY_RIGHT failed. abort.");goto led_init_fail;}
//gpio_direction_output(KEY_RIGHT, STM_GPIO_DIRECTION_IN);
//gpio_direction_input(KEY_R);

//gpio_request(KEY_PWR, "KEY_PWR");
//if (KEY_PWR==NULL){ERR("Request KEY_PWR failed. abort.");goto led_init_fail;}
//gpio_direction_output(KEY_PWR, STM_GPIO_DIRECTION_IN);
//gpio_direction_input(KEY_PWR);

	gpio_request(KEY_L_, "KEY_LEFT_");
	if (KEY_L_ == NULL)
	{
		ERR("Request KEY_LEFT_ failed. abort.");
		goto led_init_fail;
	}
//gpio_direction_output(KEY_LEFT_, STM_GPIO_DIRECTION_IN);
	gpio_direction_input(KEY_L_);

	gpio_request(KEY_R_, "KEY_RIGHT_");
	if (KEY_R_ == NULL)
	{
		ERR("Request KEY_RIGHT_ failed. abort.");
		goto led_init_fail;
	}
//gpio_direction_output(KEY_RIGHT_, STM_GPIO_DIRECTION_IN);
	gpio_direction_input(KEY_R_);

	button_dev = input_allocate_device();
	if (!button_dev)
	{
		ERR("Error : input_allocate_device");
		goto led_init_fail;
	}
	button_dev->name = button_driver_name;
	button_dev->open = button_input_open;
	button_dev->close = button_input_close;

	set_bit(EV_KEY, button_dev->evbit);
	set_bit(KEY_RIGHT, button_dev->keybit);
	set_bit(KEY_LEFT, button_dev->keybit);
	set_bit(KEY_POWER, button_dev->keybit);

	error = input_register_device(button_dev);
	if (error)
	{
		input_free_device(button_dev);
		ERR("Request input_register_device. abort.");
		goto led_init_fail;
	}



	return 0;

led_init_fail:
	led_module_exit();
	return -EIO;
}

module_init(led_module_init);
module_exit(led_module_exit);

MODULE_DESCRIPTION("SagemCom88 front vfd driver");
MODULE_AUTHOR("Nemo");
MODULE_LICENSE("GPL");

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled, 1=enabled");
