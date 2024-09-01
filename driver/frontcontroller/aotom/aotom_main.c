/*
 * aotom_main.c
 *
 * (c) 2010 Spider-Team
 * (c) 2011 oSaoYa
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
 * fulan front panel driver.
 *
 * Devices:
 *	- /dev/vfd (vfd ioctls and read/write function)
 *
 */


#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>
#include <linux/workqueue.h>

#include <linux/pm.h>

#include "aotom_ywdefs.h"
#include "aotom_main.h"
#include "utf.h"

static short paramDebug = 0;
#define TAGDEBUG "[aotom] "

#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
	} while (0)

#define DISPLAYWIDTH_MAX  8

#define INVALID_KEY      -1

#define NO_KEY_PRESS     -1
#define KEY_PRESS_DOWN    1
#define KEY_PRESS_UP      0

static char *gmt = "+0000";

typedef struct
{
	int minor;
	int open_count;
} tFrontPanelOpen;

#define FRONTPANEL_MINOR_VFD    0
#define FRONTPANEL_MINOR_RC     1
#define LASTMINOR               2

static tFrontPanelOpen FrontPanelOpen [LASTMINOR];

typedef struct
{
	int state;
	int period;
	int stop;
	struct task_struct *led_task;
	struct semaphore led_sem;
} tLedState;

static tLedState led_state[LASTLED];

#define BUFFERSIZE              256    //must be 2 ^ n

struct receive_s
{
	int len;
	unsigned char buffer[BUFFERSIZE];
};

#define cMaxReceiveQueue	100
static wait_queue_head_t   wq;

static struct receive_s receive[cMaxReceiveQueue];
static int receiveCount = 0;

static struct semaphore 	   write_sem;
static struct semaphore 	   receive_sem;
static struct semaphore 	   draw_thread_sem;

unsigned char ASCII[48][2] =
{
	{0xF1, 0x38},	//A
	{0x74, 0x72},	//B
	{0x01, 0x68},	//C
	{0x54, 0x72},	//D
	{0xE1, 0x68},	//E
	{0xE1, 0x28},	//F
	{0x71, 0x68},	//G
	{0xF1, 0x18},	//H
	{0x44, 0x62},	//I
	{0x45, 0x22},	//J
	{0xC3, 0x0C},	//K
	{0x01, 0x48},	//L
	{0x51, 0x1D},	//M
	{0x53, 0x19},	//N
	{0x11, 0x78},	//O
	{0xE1, 0x38},	//P
	{0x13, 0x78},	//Q
	{0xE3, 0x38},	//R
	{0xF0, 0x68},	//S
	{0x44, 0x22},	//T
	{0x11, 0x58},	//U
	{0x49, 0x0C},	//V
	{0x5B, 0x18},	//W
	{0x4A, 0x05},	//X
	{0x44, 0x05},	//Y
	{0x48, 0x64},	//Z
	/* A--Z  */

	{0x01, 0x68},
	{0x42, 0x01},
	{0x10, 0x70},	//
	{0x43, 0x09},	//
	{0xE0, 0x00},	//
	{0xEE, 0x07},	//
	{0xE4, 0x02},	//
	{0x50, 0x00},	//
	{0xE0, 0x00},	//
	{0x05, 0x00},	//
	{0x48, 0x04},	//

	{0x11, 0x78},	//
	{0x44, 0x02},	//
	{0xE1, 0x70},	//
	{0xF0, 0x70},	//
	{0xF0, 0x18},	//
	{0xF0, 0x68},	//
	{0xF1, 0x68},	//
	{0x10, 0x30},	//
	{0xF1, 0x78},	//
	{0xF0, 0x78},	//
	/* 0--9  */
	{0x0, 0x0}
};

static int VFD_Show_Time(u8 hh, u8 mm)
{
	if ((hh > 24) || (mm > 59))
	{
		dprintk(2, "%s bad parameter!\n", __func__);
		return -1;
	}

	return YWPANEL_FP_SetTime(hh * 3600 + mm * 60);
}

static int VFD_Show_Ico(LogNum_T log_num, int log_stat)
{
	return YWPANEL_VFD_ShowIco(log_num, log_stat);
}

static struct task_struct *draw_task = 0;
static int draw_thread_stop  = 1;

int aotomSetIcon(int which, int on);

void clear_display(void)
{
	YWPANEL_VFD_ShowString("        ");
}

static void VFD_set_all_icons(void)
{
	int i;

	for (i = 1; i <= 45; i++)
		aotomSetIcon(i, 1);
}

static void VFD_clear_all_icons(void)
{
	int i;

	for (i = 1; i <= 45; i++)
		aotomSetIcon(i, 0);
}

static void VFD_clr(void)
{
	YWPANEL_VFD_ShowTimeOff();
	clear_display();
	VFD_clear_all_icons();
}

static int draw_thread(void *arg)
{
	struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
	char buf[sizeof(data->data) + 2 * DISPLAYWIDTH_MAX];
	int len = data->length;
	int off = 0;
	int doton3 = 0;

	if (YWPANEL_width == 4 && len == 5 && data->data[2] == '.')
		doton3 = 1;

	if (len > YWPANEL_width + doton3)
	{
		memset(buf, ' ', sizeof(buf));
		off = YWPANEL_width - 1;
		memcpy(buf + off, data->data, len);
		len += off;
		buf[len + YWPANEL_width] = 0;
	}
	else
	{
		memcpy(buf, data->data, len);
		buf[len] = 0;
	}

	draw_thread_stop = 0;

	if (len > YWPANEL_width + doton3)
	{
		int pos;
		for (pos = 0; pos < len; pos++)
		{
			int i;
			if (kthread_should_stop())
			{
				draw_thread_stop = 1;
				return 0;
			}
			YWPANEL_VFD_ShowString(buf + pos);
			// sleep 200 ms
			for (i = 0; i < 5; i++)
			{
				if (kthread_should_stop())
				{
					draw_thread_stop = 1;
					return 0;
				}
				msleep(40);
			}
		}
	}

	clear_display();
	if (len > 0)
		YWPANEL_VFD_ShowString(buf + off);

	draw_thread_stop = 1;
	return 0;
}

static int led_thread(void *arg)
{
	int led = (int) arg;
	// toggle LED status for a given time period

	led_state[led].stop = 0;

	while (!kthread_should_stop())
	{
		if (!down_interruptible(&led_state[led].led_sem))
		{
			if (kthread_should_stop())
				break;
			while (!down_trylock(&led_state[led].led_sem)); // make sure semaphore is at 0
			YWPANEL_VFD_SetLed(led, led_state[led].state ? LOG_OFF : LOG_ON);
			while ((led_state[led].period > 0) && !kthread_should_stop())
			{
				msleep(10);
				led_state[led].period -= 10;
			}
			// switch LED back to manually set state
			YWPANEL_VFD_SetLed(led, led_state[led].state);
		}
	}
	led_state[led].stop = 1;
	led_state[led].led_task = 0;
	return 0;
}

static struct vfd_ioctl_data last_draw_data;

int run_draw_thread(struct vfd_ioctl_data *draw_data)
{
	if (down_interruptible(&draw_thread_sem))
		return -ERESTARTSYS;

	// return if there's already a draw task running for the same text
	if (!draw_thread_stop && draw_task && (last_draw_data.length == draw_data->length)
			&& !memcmp(&last_draw_data.data, draw_data->data, draw_data->length))
	{
		up(&draw_thread_sem);
		return 0;
	}

	memcpy(&last_draw_data, draw_data, sizeof(struct vfd_ioctl_data));

	// stop existing thread, if any
	if (!draw_thread_stop && draw_task)
	{
		kthread_stop(draw_task);
		while (!draw_thread_stop)
			msleep(1);
		draw_task = 0;
	}

	if (draw_data->length < YWPANEL_width)
	{
		char buf[DISPLAYWIDTH_MAX + 1];
		memset(buf, 0, sizeof(buf));
		memset(buf, ' ', sizeof(buf) - 1);
		if (draw_data->length)
			memcpy(buf, draw_data->data, draw_data->length);
		YWPANEL_VFD_ShowString(buf);
	}
	else
	{
		draw_thread_stop = 2;
		draw_task = kthread_run(draw_thread, draw_data, "draw_thread");

		//wait until thread has copied the argument
		while (draw_thread_stop == 2)
			msleep(1);
	}
	up(&draw_thread_sem);
	return 0;
}

static int AOTOMfp_Get_Key_Value(void)
{
	int ret, key_val;

	ret =  YWPANEL_VFD_GetKeyValue();

	switch (ret)
	{
		case 105:
			key_val = KEY_LEFT;
			break;
		case 103:
			key_val = KEY_UP;
			break;
		case 28:
			key_val = KEY_OK;
			break;
		case 106:
			key_val = KEY_RIGHT;
			break;
		case 108:
			key_val = KEY_DOWN;
			break;
		case 88:
			key_val = KEY_POWER;
			break;
		case 102:
			key_val = KEY_MENU;
			break;
		case 48:
			key_val = KEY_EXIT;
			break;
		default:
			key_val = INVALID_KEY;
			break;
	}

	return key_val;
}

int aotomSetTime(char *time)
{
	int res = 0;

	dprintk(5, "%s >\n", __func__);
	dprintk(5, "%s time: %02d:%02d\n", __func__, time[2], time[3]);

	res = VFD_Show_Time(time[2], time[3]);
#if defined(SPARK) || defined(SPARK7162)
	YWPANEL_FP_ControlTimer(true);
#endif
	dprintk(5, "%s <\n", __func__);
	return res;
}

int vfd_init_func(void)
{
	dprintk(5, "%s >\n", __func__);
	printk("Fulan VFD module initializing\n");
	return 0;
}

int aotomSetIcon(int which, int on)
{
	int  res = 0;

	dprintk(5, "%s > %d, %d\n", __func__, which, on);
	if (which < 1 || which > 45)
	{
		printk("VFD/AOTOM icon number out of range %d\n", which);
		return -EINVAL;
	}

	which -= 1;
	res = VFD_Show_Ico(((which / 15) + 11) * 16 + (which % 15) + 1, on);

	dprintk(10, "%s <\n", __func__);

	return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(aotomSetIcon);

static ssize_t AOTOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int res = 0;

	struct vfd_ioctl_data data;

	dprintk(5, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	//printk("buff = %s\n", buff);

	if (((tFrontPanelOpen *)(filp->private_data))->minor != FRONTPANEL_MINOR_VFD)
	{
		printk("Error Bad Minor\n");
		return -EOPNOTSUPP;
	}

	kernel_buf = kmalloc(len + 1, GFP_KERNEL);

	memset(kernel_buf, 0, len + 1);
	memset(&data, 0, sizeof(struct vfd_ioctl_data));

	if (kernel_buf == NULL)
	{
		printk("%s return no mem<\n", __func__);
		return -ENOMEM;
	}
	copy_from_user(kernel_buf, buff, len);

	if (len > sizeof(data.data))
		data.length = sizeof(data.data);
	else
		data.length = len;

	while ((data.length > 0) && (kernel_buf[data.length - 1 ] == '\n'))
		data.length--;

	if (data.length > sizeof(data.data))
		len = data.length = sizeof(data.data);

	memcpy(data.data, kernel_buf, data.length);
	res = run_draw_thread(&data);

	kfree(kernel_buf);

	dprintk(10, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
		return res;
	return len;
}

static void flashLED(int led, int ms)
{
	if (!led_state[led].led_task || ms < 1)
		return;
	led_state[led].period = ms;
	up(&led_state[led].led_sem);
}

static ssize_t AOTOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	dprintk(5, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	if (((tFrontPanelOpen *)(filp->private_data))->minor == FRONTPANEL_MINOR_RC)
	{
		while (receiveCount == 0)
		{
			if (wait_event_interruptible(wq, receiveCount > 0))
				return -ERESTARTSYS;
		}
		flashLED(LED_GREEN, 100);

		/* 0. claim semaphore */
		if (down_interruptible(&receive_sem))
			return -ERESTARTSYS;

		/* 1. copy data to user */
		copy_to_user(buff, receive[0].buffer, receive[0].len);

		/* 2. copy all entries to start and decrease receiveCount */
		receiveCount--;
		memmove(&receive[0], &receive[1], (cMaxReceiveQueue - 1) * sizeof(struct receive_s));

		/* 3. free semaphore */
		up(&receive_sem);

		return receive[0].len;
	}
	return -EOPNOTSUPP;
}

static int AOTOMdev_open(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk(5, "%s >\n", __func__);
	minor = MINOR(inode->i_rdev);
	dprintk(1, "open minor %d\n", minor);

	if (minor == FRONTPANEL_MINOR_RC && FrontPanelOpen[minor].open_count)
	{
		printk("EUSER\n");
		return -EUSERS;
	}

	FrontPanelOpen[minor].open_count++;
	filp->private_data = &FrontPanelOpen[minor];

	dprintk(5, "%s <\n", __func__);
	return 0;
}

static int AOTOMdev_close(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk(5, "%s >\n", __func__);
	minor = MINOR(inode->i_rdev);
	dprintk(1, "close minor %d\n", minor);

	if (FrontPanelOpen[minor].open_count > 0)
		FrontPanelOpen[minor].open_count--;

	dprintk(5, "%s <\n", __func__);
	return 0;
}

static struct aotom_ioctl_data aotom_data;
static struct vfd_ioctl_data vfd_data;

static int AOTOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	int icon_nr = 0;
	static int mode = 0;
	int res = -EINVAL;
	dprintk(5, "%s > 0x%.8x\n", __func__, cmd);

	if (down_interruptible(&write_sem))
		return -ERESTARTSYS;

	switch (cmd)
	{
		case VFDSETMODE:
		case VFDSETLED:
		case VFDICONDISPLAYONOFF:
		case VFDSETTIME:
		case VFDBRIGHTNESS:
			if (copy_from_user(&aotom_data, (void *) arg, sizeof(aotom_data)))
				return -EFAULT;
	}

	switch (cmd)
	{
		case VFDSETMODE:
			mode = aotom_data.u.mode.compat;
			break;
		case VFDSETLED:
#if defined(SPARK) || defined(SPARK7162)
			if (aotom_data.u.led.led_nr > -1 && aotom_data.u.led.led_nr < LED_MAX)
			{
				switch (aotom_data.u.led.on)
				{
					case LOG_OFF:
					case LOG_ON:
						res = YWPANEL_VFD_SetLed(aotom_data.u.led.led_nr, aotom_data.u.led.on);
						led_state[aotom_data.u.led.led_nr].state = aotom_data.u.led.on;
						break;
					default: // toggle (for aotom_data.u.led.on * 10) ms
						flashLED(aotom_data.u.led.led_nr, aotom_data.u.led.on * 10);
				}
			}
#endif
			break;
		case VFDBRIGHTNESS:
			if (aotom_data.u.brightness.level < 0)
				aotom_data.u.brightness.level = 0;
			else if (aotom_data.u.brightness.level > 7)
				aotom_data.u.brightness.level = 7;
			res = YWPANEL_VFD_SetBrightness(aotom_data.u.brightness.level);
			break;
		case VFDICONDISPLAYONOFF:
		{
#if defined(SPARK)
			switch (aotom_data.u.icon.icon_nr)
			{
				case 0:
					res = YWPANEL_VFD_SetLed(LED_RED, aotom_data.u.icon.on);
					led_state[LED_RED].state = aotom_data.u.icon.on;
					break;
				case 35:
					res = YWPANEL_VFD_SetLed(LED_GREEN, aotom_data.u.icon.on);
					led_state[LED_GREEN].state = aotom_data.u.icon.on;
					break;
				default:
					break;
			}
#endif
#if defined(SPARK7162)
			icon_nr = aotom_data.u.icon.icon_nr;
			//e2 icons workarround
			//printk("icon_nr = %d\n", icon_nr);
			if (icon_nr >= 256)
			{
				icon_nr = icon_nr / 256;
				switch (icon_nr)
				{
					case 0x11:
						icon_nr = 0x0E; //widescreen
						break;
					case 0x13:
						icon_nr = 0x0B; //CA
						break;
					case 0x15:
						icon_nr = 0x19; //mp3
						break;
					case 0x17:
						icon_nr = 0x1A; //ac3
						break;
					case 0x1A:
						icon_nr = 0x03; //play
						break;
					case 0x1e:
						icon_nr = 0x07; //record
						break;
					case 38:
						break; //cd part1
					case 39:
						break; //cd part2
					case 40:
						break; //cd part3
					case 41:
						break; //cd part4
					default:
						icon_nr = 0; //no additional symbols at the moment
						break;
				}
			}
			if (aotom_data.u.icon.on != 0)
				aotom_data.u.icon.on = 1;
			if (icon_nr > 0 && icon_nr <= 45)
				res = aotomSetIcon(icon_nr, aotom_data.u.icon.on);
			if (icon_nr == 46)
			{
				switch (aotom_data.u.icon.on)
				{
					case 1:
						VFD_set_all_icons();
						res = 0;
						break;
					case 0:
						VFD_clear_all_icons();
						res = 0;
						break;
					default:
						break;
				}
			}
#endif
			mode = 0;
			break;
		}

		case VFDSTANDBY:
		{
#if defined(SPARK) || defined(SPARK7162)
			u32 uTime = 0;
			get_user(uTime, (int *) arg);

			YWPANEL_FP_SetPowerOnTime(uTime);

			clear_display();
			YWPANEL_FP_ControlTimer(true);
			YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_STANDBY);
			res = 0;
#endif
			break;
		}
		case VFDSETTIME2:
		{
			u32 uTime = 0;
			res = get_user(uTime, (int *)arg);
			if (! res)
			{
				res = YWPANEL_FP_SetTime(uTime);
				YWPANEL_FP_ControlTimer(true);
			}
			break;
		}
		case VFDSETTIME:
			res = aotomSetTime(aotom_data.u.time.time);
			break;
		case VFDGETTIME:
		{
#if defined(SPARK) || defined(SPARK7162)
			u32 uTime = 0;
			uTime = YWPANEL_FP_GetTime();
			//printk("uTime = %d\n", uTime);
			res = put_user(uTime, (int *) arg);
#endif
			break;
		}
		case VFDGETWAKEUPMODE:
			break;
		case VFDDISPLAYCHARS:
			if (mode == 0)
			{
				if (copy_from_user(&vfd_data, (void *) arg, sizeof(vfd_data)))
					return -EFAULT;
				if (vfd_data.length > sizeof(vfd_data.data))
					vfd_data.length = sizeof(vfd_data.data);
				while ((vfd_data.length > 0) && (vfd_data.data[vfd_data.length - 1 ] == '\n'))
					vfd_data.length--;
				res = run_draw_thread(&vfd_data);
			}
			else
				mode = 0;
			break;
		case VFDDISPLAYWRITEONOFF:
			break;
		case VFDDISPLAYCLR:
			vfd_data.length = 0;
			res = run_draw_thread(&vfd_data);
			break;
#if defined(SPARK)
		case 0x5305:
			res = 0;
			break;
#endif
		case 0x5401:
			res = 0;
			break;
		case VFDGETSTARTUPSTATE:
		{
			YWPANEL_STARTUPSTATE_t State;
			if (YWPANEL_FP_GetStartUpState(&State))
				res = put_user(State, (int *) arg);
			break;
		}
		case VFDGETVERSION:
		{
			YWPANEL_Version_t panel_version;
			memset(&panel_version, 0, sizeof(YWPANEL_Version_t));
			if (YWPANEL_FP_GetVersion(&panel_version))
				res = put_user(panel_version.DisplayInfo, (int *)arg);
			break;
		}

		default:
			printk("VFD/AOTOM: unknown IOCTL 0x%x\n", cmd);
			mode = 0;
			break;
	}

	up(&write_sem);

	dprintk(5, "%s <\n", __func__);
	return res;
}

static unsigned int AOTOMdev_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &wq, wait);

	if (receiveCount > 0)
		mask = POLLIN | POLLRDNORM;

	return mask;
}

static struct file_operations vfd_fops =
{
	.owner = THIS_MODULE,
	.ioctl = AOTOMdev_ioctl,
	.write = AOTOMdev_write,
	.read  = AOTOMdev_read,
	.poll  = AOTOMdev_poll,
	.open  = AOTOMdev_open,
	.release  = AOTOMdev_close
};

/*----- Button driver -------*/

static char *button_driver_name = "fulan front panel buttons";
static struct input_dev *button_dev;
static int bad_polling = 1;
static struct workqueue_struct *fpwq;

static void button_bad_polling(struct work_struct *work)
{
	int btn_pressed = 0;

	int report_key = 0;

	while (bad_polling == 1)
	{
		int button_value;
		msleep(50);
		button_value = AOTOMfp_Get_Key_Value();
		if (button_value != INVALID_KEY)
		{
			dprintk(5, "got button: %X\n", button_value);
			flashLED(LED_GREEN, 100);
			//VFD_Show_Ico(DOT2,LOG_ON);
			//YWPANEL_VFD_SetLed(1, LOG_ON);
			if (1 == btn_pressed)
			{
				if (report_key == button_value)
					continue;
				input_report_key(button_dev, report_key, 0);
				input_sync(button_dev);
			}
			report_key = button_value;
			btn_pressed = 1;
			switch (button_value)
			{
				case KEY_LEFT:
				case KEY_RIGHT:
				case KEY_UP:
				case KEY_DOWN:
				case KEY_OK:
				case KEY_MENU:
				case KEY_EXIT:
				case KEY_POWER:
					input_report_key(button_dev, button_value, 1);
					input_sync(button_dev);
					break;
				default:
					dprintk(5, "[BTN] unknown button_value %d\n", button_value);
			}
		}
		else
		{
			if (btn_pressed)
			{
				btn_pressed = 0;
				//msleep(80);
				//VFD_Show_Ico(DOT2,LOG_OFF);
				//YWPANEL_VFD_SetLed(1, LOG_OFF);
				input_report_key(button_dev, report_key, 0);
				input_sync(button_dev);
			}
		}
	}
	bad_polling = 2;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
static DECLARE_WORK(button_obj, button_bad_polling);
#else
static DECLARE_WORK(button_obj, button_bad_polling, NULL);
#endif
static int button_input_open(struct input_dev *dev)
{
	fpwq = create_workqueue("button");
	if (queue_work(fpwq, &button_obj))
	{
		dprintk(5, "[BTN] queue_work successful ...\n");
		return 0;
	}
	dprintk(5, "[BTN] queue_work not successful, exiting ...\n");
	return 1;
}

static void button_input_close(struct input_dev *dev)
{
	bad_polling = 0;
	while (bad_polling != 2)
		msleep(1);
	bad_polling = 1;

	if (fpwq)
	{
		destroy_workqueue(fpwq);
		dprintk(5, "[BTN] workqueue destroyed\n");
	}
}

int button_dev_init(void)
{
	int error;

	dprintk(5, "[BTN] allocating and registering button device\n");

	button_dev = input_allocate_device();
	if (!button_dev)
		return -ENOMEM;

	button_dev->name = button_driver_name;
	button_dev->open = button_input_open;
	button_dev->close = button_input_close;

	set_bit(EV_KEY    , button_dev->evbit);
	set_bit(KEY_UP    , button_dev->keybit);
	set_bit(KEY_DOWN  , button_dev->keybit);
	set_bit(KEY_LEFT  , button_dev->keybit);
	set_bit(KEY_RIGHT , button_dev->keybit);
	set_bit(KEY_POWER , button_dev->keybit);
	set_bit(KEY_MENU  , button_dev->keybit);
	set_bit(KEY_OK    , button_dev->keybit);
	set_bit(KEY_EXIT  , button_dev->keybit);

	error = input_register_device(button_dev);
	if (error)
		input_free_device(button_dev);

	return error;
}

void button_dev_exit(void)
{
	dprintk(5, "[BTN] unregistering button device\n");
	input_unregister_device(button_dev);
}

static void aotom_standby(void)
{
	while (down_interruptible(&write_sem))
		msleep(10);
	printk(KERN_EMERG "Initiating standby with pm_power_off hook.\n");
	clear_display();
	YWPANEL_FP_ControlTimer(true);
	YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_STANDBY);
	// not reached
}

static int __init aotom_init_module(void)
{
	int i;

	dprintk(5, "%s >\n", __func__);

	printk("Fulan front panel driver\n");

	if (YWPANEL_VFD_Init())
	{
		printk("unable to init module\n");
		return -1;
	}

	VFD_clr();

	if (button_dev_init() != 0)
		return -1;

	if (register_chrdev(VFD_MAJOR, "VFD", &vfd_fops))
		printk("unable to get major %d for VFD\n", VFD_MAJOR);

	if (pm_power_off)
		printk("pm_power_off hook already applied! Do not register anything\n");
	else
		pm_power_off = 	aotom_standby;

	sema_init(&write_sem, 1);
	sema_init(&receive_sem, 1);
	sema_init(&draw_thread_sem, 1);

	for (i = 0; i < LASTMINOR; i++)
	{
		FrontPanelOpen[i].open_count = 0;
		FrontPanelOpen[i].minor = i;
	}

	for (i = 0; i < LASTLED; i++)
	{
		led_state[i].state = LOG_OFF;
		led_state[i].period = 0;
		led_state[i].stop = 1;
		sema_init(&led_state[i].led_sem, 0);
		led_state[i].led_task = kthread_run(led_thread, (void *) i, "led_thread");
	}


	dprintk(5, "%s <\n", __func__);

	return 0;
}

static int led_thread_active(void)
{
	int i;

	for (i = 0; i < LASTLED; i++)
		if (!led_state[i].stop && led_state[i].led_task)
			return -1;
	return 0;
}

static void __exit aotom_cleanup_module(void)
{
	int i;

	if (aotom_standby == pm_power_off)
		pm_power_off = NULL;

	if (!draw_thread_stop && draw_task)
		kthread_stop(draw_task);

	for (i = 0; i < LASTLED; i++)
		if (!led_state[i].stop && led_state[i].led_task)
		{
			up(&led_state[i].led_sem);
			kthread_stop(led_state[i].led_task);
		}

	while (!draw_thread_stop && !led_thread_active())
		msleep(1);

	dprintk(5, "[BTN] unloading ...\n");
	button_dev_exit();

	unregister_chrdev(VFD_MAJOR, "VFD");
	YWPANEL_VFD_Term();
	printk("Fulan front panel module unloading\n");
}

module_init(aotom_init_module);
module_exit(aotom_cleanup_module);

module_param(paramDebug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(paramDebug, "Debug Output 0=disabled >0=enabled(debuglevel)");

module_param(gmt, charp, 0);
MODULE_PARM_DESC(gmt, "gmt offset (default +0000");

MODULE_DESCRIPTION("VFD module for fulan boxes");
MODULE_AUTHOR("Spider-Team, oSaoYa");
MODULE_LICENSE("GPL");

