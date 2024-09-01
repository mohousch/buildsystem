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
#include <linux/stm/pio.h>

#include "../vfd/utf.h"

#define LED1 stm_gpio(4, 0)
#define LED2 stm_gpio(4, 1)

#define KEYPp	stm_gpio(5, 4)
#define KEYPm	stm_gpio(5, 5)

//#define KEY_R	stm_gpio(5, 6)
//#define KEY_L	stm_gpio(5, 7)
//#define KEY_PWR		stm_gpio(11, 6)

static char *button_driver_name = "BOX frontpanel buttons";
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

#define DBG(fmt, args...) if ( paramDebug > 0 ) printk(KERN_INFO "[VFD] :: " fmt "\n", ## args )
#define ERR(fmt, args...) printk(KERN_ERR "[vfd] :: " fmt "\n", ## args )

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
void button_bad_polling(void)
#else
void button_bad_polling(struct work_struct *ignored)
#endif
{
	DBG("[%s] >>>\n", __func__);
	unsigned int  key_front;

	while (bad_polling == 1)
	{
		msleep(100);

//DBG("-%d\n",gpio_get_value(KEY_LEFT));

		key_front = 0;
		//if(gpio_get_value(KEY_L)==0) key_front=KEY_LEFT;
		if (gpio_get_value(KEYPm) == 0) key_front = KEY_LEFT;
		//if(gpio_get_value(KEY_PWR)==0) key_front=KEY_POWER;
		//if(gpio_get_value(KEY_R)==0) key_front=KEY_RIGHT;
		if (gpio_get_value(KEYPp) == 0) key_front = KEY_RIGHT;

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
	DBG("[%s] >>>\n", __func__);
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

/* VFD code based on vacfludis - vacuum fluorescent display by qrt@qland.de
 * it's using the same protocol to HCS-12SS59T with one difference. 
 * Characters are stored in ASCII, no need to convert them.
*/

#define VFD_RST  	stm_gpio(11,5) //reset by 0
#if 0
#define VFD_VDON 	stm_gpio(4, 7) //Vdisp 0=ON, 1=OFF
#endif
#define VFD_CLK  	stm_gpio(3, 4) //ShiftClockInput' - the serial data is shifted at the rising edge of CLK
#define VFD_DAT  	stm_gpio(3, 5) //serial data
#define VFD_CS   	stm_gpio(5, 3) //enable transfer by 0

#define LINORM		0x00		// normal display
#define LIOFF		0x01		// lights OFF
#define LION		0x02		//        ON

#define VFD_DCRAM_WR	0x10		// ccccaaaa dddddddd dddddddd ..
#define VFD_CGRAM_WR	0x20		// "
#define VFD_ADRAM_WR	0x30		// ccccaaaa ******dd ******dd ..
#define VFD_DUTY	0x50		// set brightness vfdcmd 80 <0-15>
#define VFD_NUMDIGIT	0x60		// set number of digits vfdcmd 96 15
#define VFD_LIGHTS	0x70		// set lights vfdcmd 112 <0-3>
#define VFD_DELAY	8
#define VFD_DEBUG	0

static void vfdSendByte(uint8_t port_digit)
{
	unsigned char i;
	unsigned char digit;
	digit = port_digit;
	
	//DBG("%d=", digit);
	for(i=0;i<8;i++) {
/*	    if (digit & 1) 
		DBG("1");
	    else
		DBG("0");
*/	    gpio_set_value(VFD_DAT, digit & 1);
	    udelay(1);
	    gpio_set_value(VFD_CLK, 1);
	    udelay(1);
	    gpio_set_value(VFD_CLK, 0);
	    udelay(2);
	    gpio_set_value(VFD_CLK, 1);
	    udelay(2);
	    digit >>= 1;
	}
	DBG("\n");
	udelay(VFD_DELAY);
}

static void vfdSelect(unsigned char s)
{
	if(s){
		//enable transfer mode
		gpio_set_value(VFD_CS, 1);
		udelay(VFD_DELAY * 2);
		gpio_set_value(VFD_CS, 0);
		udelay(VFD_DELAY * 2);
	} else {
		//disable transfer mode
		udelay(VFD_DELAY * 2);
		gpio_set_value(VFD_CS, 1);
	}
}

static void vfdSendCMD(uint8_t vfdcmd, uint8_t vfdarg)
{
	DBG("vfdcmd=%d, vfdarg=%d >> (vfdcmd | vfdarg) = %d\n",vfdcmd, vfdarg, (vfdcmd | vfdarg));

	//transfering data
	vfdSelect(1);
	vfdSendByte(vfdcmd | vfdarg);
	vfdSelect(0);
}

int VFD_WriteFront(unsigned char* data, unsigned char len )
{
	unsigned char lcd_buf[16] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
	int i = 0;
	int j = 0;

	while ((i < len) && (j < 16))
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
			lcd_buf[j] = data[i];
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
				lcd_buf[j] = UTF_Char_Table[data[i] & 0x3f];
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

	if (j < 16)
	{
		for (i = j; i < 16; i++)
		{
			lcd_buf[i] = 0x20;
		}
	}

	//enable transfer mode
	vfdSelect(1);

	//transfering data
	vfdSendByte(VFD_DCRAM_WR | 0); //set first char
	for(i=0;i<16;i++) {
		//DBG("[%s] CHAR=%d >> ",__func__, lcd_buf[i]);
		vfdSendByte(lcd_buf[i]);
	}

	//disable transfer mode
	vfdSelect(0);
	return 0;
}

static void VFD_setIcon(unsigned char *kbuf, unsigned char len)
{
	spin_lock(&mr_lock);

	switch (kbuf[0])
	{
		case 1://red
			if (kbuf[4] == 1) gpio_set_value(LED1, 0);
			else gpio_set_value(LED1, 1);
			break;
		case 2://blie
			if (kbuf[4] == 1) gpio_set_value(LED2, 1);
			else gpio_set_value(LED2, 0);
			break;
		default:
			ERR("icon unknown %d", kbuf[0]);
			break;
	}

	spin_unlock(&mr_lock);
}


static void VFD_init(void)
{
	DBG("[%s] >>>\n", __func__);
	//reset VFD
#if 0
	gpio_set_value(VFD_VDON, 0);
	udelay(2000);
#endif
	gpio_set_value(VFD_RST, 1);
	udelay(2000);
	gpio_set_value(VFD_RST, 0);
	udelay(2000);
	gpio_set_value(VFD_RST, 1);
	udelay(2000);
	vfdSendCMD(VFD_NUMDIGIT, 15);
	vfdSendCMD(VFD_DUTY, 15);
	vfdSendCMD(VFD_LIGHTS,LINORM);
	VFD_WriteFront("", 0);
}

static void VFD_setBrightness(int level)
{
	DBG("[%s] >>>\n", __func__);
	if (level < 0)
		vfdSendCMD(VFD_DUTY, 0);
	else if (level > 15)
		vfdSendCMD(VFD_DUTY, 15);
	else
		vfdSendCMD(VFD_DUTY, level);
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
	DBG("[%s] >>>\n", __func__);
	struct vfd_ioctl_data vfddata;

	switch (cmd)
	{
		case VFDIOC_DCRAMWRITE:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			return VFD_WriteFront(vfddata.data, vfddata.length);
			break;
		case VFDIOC_BRIGHTNESS:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			VFD_setBrightness(vfddata.address);
			break;
		case VFDIOC_DISPLAYWRITEONOFF:
			break;
		case VFDIOC_ICONDISPLAYONOFF:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			VFD_setIcon(vfddata.data, vfddata.length);
			break;
		case VFDIOC_DRIVERINIT:
			VFD_init();
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
	VFD_WriteFront(buf, len);
	return len;
}

static ssize_t vfd_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	return len;
}

static int vfd_open(struct inode *inode, struct file *file)
{

	DBG("[%s] >>>\n", __func__);

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
	DBG("[%s] >>>\n", __func__);
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
	DBG("[%s] >>>\n", __func__);
	unregister_chrdev(VFD_MAJOR, "vfd");
	input_unregister_device(button_dev);
}

static int __init led_module_init(void)
{
	int error;

	DBG("VFD PACE7241 init >>>");
	DBG("register character device %d.", VFD_MAJOR);
	if (register_chrdev(VFD_MAJOR, "vfd", &vfd_fops))
	{
		ERR("register major %d failed", VFD_MAJOR);
		goto led_init_fail;
	} else {
		DBG("character device registered properly");
	}

	sema_init(&(vfd.sem), 1);
	vfd.opencount = 0;

	gpio_request(VFD_RST, "VFD_RST");
	if (VFD_RST == NULL)
	{
		ERR("gpio_request VFD_RST failed.");
	} else {
		DBG("gpio_request VFD_RST successful, setting STM_GPIO_DIRECTION_OUT");
		gpio_direction_output(VFD_RST, STM_GPIO_DIRECTION_OUT);
	}
#if 0	
	gpio_request(VFD_VDON, "VFD_VDON");
	if (VFD_VDON == NULL)
	{
		ERR("gpio_request VFD_VDON failed.");
	} else {
		DBG("gpio_request VFD_VDON successful, setting STM_GPIO_DIRECTION_OUT");
		gpio_direction_output(VFD_VDON, STM_GPIO_DIRECTION_OUT);
		gpio_set_value(VFD_VDON, 1);
	}
#endif	
	gpio_request(VFD_DAT, "VFD_DAT");
	if (VFD_DAT == NULL)
	{
		ERR("gpio_request VFD_DAT failed.");
	} else {
		DBG("gpio_request VFD_DAT successful, setting STM_GPIO_DIRECTION_OUT");
		gpio_direction_output(VFD_DAT, STM_GPIO_DIRECTION_OUT);
		gpio_set_value(VFD_DAT, 0);
	}
	
	gpio_request(VFD_CS, "VFD_CS");
	if (VFD_CS == NULL)
	{
		ERR("gpio_request VFD_CS failed.");
	} else {
		DBG("gpio_request VFD_CS successful, setting STM_GPIO_DIRECTION_OUT");
		gpio_direction_output(VFD_CS, STM_GPIO_DIRECTION_OUT);
		gpio_set_value(VFD_CS, 0);
	}
	
	VFD_init();

	gpio_request(LED1, "LED1");
	if (LED1 == NULL)
	{
		ERR("Request LED1 failed. abort.");
		goto led_init_fail;
	} else {
		DBG("LED1 init successful");
		gpio_direction_output(LED1, STM_GPIO_DIRECTION_OUT);
		gpio_set_value(LED1, 0);//red
	}

	gpio_request(LED2, "LED2");
	if (LED2 == NULL)
	{
		ERR("Request LED2 failed. abort.");
		goto led_init_fail;
	} else {
		DBG("LED2 init successful");
		gpio_direction_output(LED2, STM_GPIO_DIRECTION_OUT);
		gpio_set_value(LED2, 1);//blue
	}

	DBG("Keys init");
	gpio_request(KEYPm, "KEY_LEFT_");
	if (KEYPm == NULL)
	{
		ERR("Request KEYPm failed. abort.");
		goto led_init_fail;
	} else {
		DBG("KEYPm init successful");
	}
//gpio_direction_output(KEY_LEFT_, STM_GPIO_DIRECTION_IN);
	gpio_direction_input(KEYPm);

	gpio_request(KEYPp, "KEY_RIGHT_");
	if (KEYPp == NULL)
	{
		ERR("Request KEY_RIGHT_ failed. abort.");
		goto led_init_fail;
	} else {
		DBG("KEYPp init successful");
	}
//gpio_direction_output(KEY_RIGHT_, STM_GPIO_DIRECTION_IN);
	gpio_direction_input(KEYPp);

	button_dev = input_allocate_device();
	if (!button_dev)
	{
		ERR("Error : input_allocate_device");
		goto led_init_fail;
	} else {
		DBG("button_dev allocated properly");
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
	} else {
		DBG("Request input_register_device successful");
	}
	return 0;

led_init_fail:
	led_module_exit();
	ERR("led_init_fail !!!");
	return -EIO;
}

module_init(led_module_init);
module_exit(led_module_exit);

MODULE_DESCRIPTION("Pace7241 front vfd driver");
MODULE_AUTHOR("j00zek");
MODULE_LICENSE("GPL");

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled, 1=enabled");
