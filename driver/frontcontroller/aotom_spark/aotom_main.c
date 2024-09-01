/*
 * aotom_main.c
 *
 * (c) 2010 Spider-Team
 * (c) 2011 oSaoYa
 * (c) 2012-2013 Stefan Seyfried
 * (c) 2012-2013 martii
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
 *  - /dev/vfd (vfd ioctls and read/write function)
 *
 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
/* for RTC / reboot_notifier hooks */
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>

#include "aotom_main.h"

static short paramDebug = 0;
#define TAGDEBUG "[aotom] "

#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug > level)) printk(TAGDEBUG x); \
	} while (0)

#define DISPLAYWIDTH_MAX 8

#define NO_KEY_PRESS    -1
#define KEY_PRESS_DOWN   1
#define KEY_PRESS_UP     0

static char *gmt = "+0000";

static int open_count = 0;

#define FRONTPANEL_MINOR_VFD    0

typedef struct
{
	int state;
	int period;
	int stop;
	struct task_struct *led_task;
	struct semaphore led_sem;
} tLedState;

static tLedState led_state[LED_COUNT + 1];
static int led_count = LED_COUNT;

static struct semaphore write_sem;
static struct semaphore draw_thread_sem;

#define RTC_NAME "aotom-rtc"
static struct platform_device *rtc_pdev;

extern YWPANEL_Version_t panel_version;

static int VFD_Show_Time(u8 hh, u8 mm)
{
	if ((hh > 24) || (mm > 59))
	{
		dprintk(2, "%s bad parameter!\n", __func__);
		return -1;
	}
	return YWPANEL_FP_SetTime(hh * 3600 + mm * 60);
}

static int VFD_Show_Icon(int which, int on)
{
	return YWPANEL_VFD_ShowIcon(which, on);
}

static struct task_struct *draw_task = 0;
#define DRAW_THREAD_STATUS_RUNNING  0
#define DRAW_THREAD_STATUS_STOPPED  1
#define DRAW_THREAD_STATUS_INIT     2
static int draw_thread_status = DRAW_THREAD_STATUS_STOPPED;

static int aotomSetIcon(int which, int on);

int vfdlen_open(struct inode *inode, struct  file *file) {
  return single_open(file, vfdlen_show, NULL);
}

int vfdlen_show(struct seq_file *m, void *v) {
  seq_printf(m, "%d\n", YWPANEL_width);
  return 0;
}

static const struct file_operations vfdlen_fops = {
  .owner = THIS_MODULE,
  .open = vfdlen_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};

static void clear_display(void)
{
	YWPANEL_VFD_ShowString("        ");
}

static void VFD_set_all_icons(int onoff)
{
	int i;
	for (i = 1; i < 46; i++)
		aotomSetIcon(i, onoff);
}

static void VFD_clr(void)
{
	YWPANEL_VFD_ShowTimeOff();
	clear_display();
	VFD_set_all_icons(LED_OFF);
}

int utf8charlen(unsigned char c)
{
	if (!c)
		return 0;
	if (!(c >> 7))      // 0xxxxxxx
		return 1;
	if (c >> 5 == 6)    // 110xxxxx 10xxxxxx
		return 2;
	if (c >> 4 == 14)   // 1110xxxx 10xxxxxx 10xxxxxx
		return 3;
	if (c >> 3 == 30)   // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		return 4;
	return 0;
}

int utf8strlen(char *s, int len)
{
	int i = 0, ulen = 0;
	while (i < len)
	{
		int trailing = utf8charlen((unsigned char)s[i]);
		if (!trailing)
			return ulen;
		trailing--, i++;
		if (trailing && (i >= len))
			return ulen;
		while (trailing)
		{
			if (i >= len || (unsigned char)s[i] >> 6 != 2)
				return ulen;
			trailing--;
			i++;
		}
		ulen++;
	}
	return ulen; // can be UTF8 (or pure ASCII, at least no non-UTF-8 8bit characters)
}

static int draw_thread(void *arg)
{
	struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
	unsigned char buf[sizeof(data->data) + 2 * DISPLAYWIDTH_MAX];
	int len = data->length;
	int off = 0;
	int saved = 0;
	int utf8len = utf8strlen(data->data, data->length);
	if ((panel_version.DisplayInfo == YWPANEL_FP_DISPTYPE_LED) && (len > 2) && (data->data[2] == '.' || data->data[2] == ','))
		saved = 1;
	if (utf8len - saved > YWPANEL_width)
	{
		memset(buf, ' ', sizeof(buf));
		off = YWPANEL_width - 1;
		memcpy(buf + off, data->data, len);
		len += off;
		utf8len += off;
		buf[len + YWPANEL_width] = 0;
	}
	else
	{
		memcpy(buf, data->data, len);
		buf[len] = 0;
	}
	draw_thread_status = DRAW_THREAD_STATUS_RUNNING;
	if (utf8len - saved > YWPANEL_width)
	{
		unsigned char *b = buf;
		int pos;
		for (pos = 0; pos < utf8len; pos++)
		{
			int i;
			char dot = 0;
			if (kthread_should_stop())
			{
				draw_thread_status = DRAW_THREAD_STATUS_STOPPED;
				return 0;
			}
			if ((panel_version.DisplayInfo == YWPANEL_FP_DISPTYPE_LED) && (b + 2 < buf + sizeof(buf)) && (b[2] == '.' || b[2] == ','))
				dot = b[2];
			if (dot)
				b[2] = ' ';
			YWPANEL_VFD_ShowString(b);
			if (dot)
				b[2] = dot;
			// sleep 200 ms
			for (i = 0; i < 5; i++)
			{
				if (kthread_should_stop())
				{
					draw_thread_status = DRAW_THREAD_STATUS_STOPPED;
					return 0;
				}
				msleep(40);
			}
			// advance to next UTF-8 character
			b += utf8charlen(*b);
		}
	}
	if (utf8len > 0)
		YWPANEL_VFD_ShowString(buf + off);
	else
		clear_display();
	draw_thread_status = DRAW_THREAD_STATUS_STOPPED;
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
			YWPANEL_VFD_SetLed(led, !led_state[led].state);
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

// Try "spark_fp -p 20 -l 2" ... --martii
static int spinner_thread(void *arg)
{
	int led = (int) arg;
	// enable DISK_S0, periodically enable S1, S2, S3 until stopped
	led_state[led].stop = 0;
	while (!kthread_should_stop())
	{
		if (!down_interruptible(&led_state[led].led_sem))
		{
			int i = 0;
			if (kthread_should_stop())
				break;
			while (!down_trylock(&led_state[led].led_sem));
			YWPANEL_VFD_ShowIcon(AOTOM_DISK_S0, LED_ON);
			while ((led_state[led].period > 0) && !kthread_should_stop())
			{
				int period = led_state[led].period;
				YWPANEL_VFD_ShowIcon(AOTOM_DISK_S1, i == 0);
				YWPANEL_VFD_ShowIcon(AOTOM_DISK_S2, i == 1);
				YWPANEL_VFD_ShowIcon(AOTOM_DISK_S3, i == 2);
				i++;
				i %= 3;
				while ((period > 0) && (led_state[led].period > 0) && !kthread_should_stop())
				{
					msleep(10);
					period -= 10;
				}
			}
			YWPANEL_VFD_ShowIcon(AOTOM_DISK_S0, LED_OFF);
			YWPANEL_VFD_ShowIcon(AOTOM_DISK_S1, LED_OFF);
			YWPANEL_VFD_ShowIcon(AOTOM_DISK_S2, LED_OFF);
			YWPANEL_VFD_ShowIcon(AOTOM_DISK_S3, LED_OFF);
		}
	}
	led_state[led].stop = 1;
	led_state[led].led_task = 0;
	return 0;
}

static struct vfd_ioctl_data last_draw_data;

static int run_draw_thread(struct vfd_ioctl_data *draw_data)
{
	if (down_interruptible(&draw_thread_sem))
		return -ERESTARTSYS;
	// return if there is already a draw task running for the same text
	if ((draw_thread_status != DRAW_THREAD_STATUS_STOPPED) && draw_task && (last_draw_data.length == draw_data->length) && !memcmp(&last_draw_data.data, draw_data->data, draw_data->length))
	{
		up(&draw_thread_sem);
		return 0;
	}
	memcpy(&last_draw_data, draw_data, sizeof(struct vfd_ioctl_data));
	// stop existing thread, if any
	if ((draw_thread_status != DRAW_THREAD_STATUS_STOPPED) && draw_task)
	{
		kthread_stop(draw_task);
		while (draw_thread_status != DRAW_THREAD_STATUS_STOPPED)
			msleep(1);
	}
	draw_thread_status = DRAW_THREAD_STATUS_INIT;
	draw_task = kthread_run(draw_thread, draw_data, "draw_thread");
	//wait until thread has copied the argument
	while (draw_thread_status == DRAW_THREAD_STATUS_INIT)
		msleep(1);
	up(&draw_thread_sem);
	return 0;
}

int aotomSetTime(char *time)
{
	int res = 0;
	dprintk(5, "%s >\n", __func__);
	dprintk(5, "%s time: %02d:%02d\n", __func__, time[2], time[3]);
	res = VFD_Show_Time(time[2], time[3]);
	YWPANEL_FP_ControlTimer(true);
	dprintk(5, "%s <\n", __func__);
	return res;
}

int vfd_init_func(void)
{
	dprintk(5, "%s >\n", __func__);
	printk("Fulan VFD module initializing\n");
	return 0;
}

static int aotomSetIcon(int which, int on)
{
	int  res = 0;
	dprintk(5, "%s > %d, %d\n", __func__, which, on);
	if (which < AOTOM_FIRST)   // sequential enumeration, starting at 1
	{
		which -= 1;
		which = ((which / 15) + 11) * 16 + (which % 15) + 1;
	}
	if (which < AOTOM_FIRST || which > AOTOM_LAST)
	{
		printk("VFD/AOTOM icon number out of range %d\n", which);
		return -EINVAL;
	}
	res = VFD_Show_Icon(which, on);
	dprintk(10, "%s <\n", __func__);
	return res;
}

/* export for later use in e2_proc */
//EXPORT_SYMBOL(aotomSetIcon);

static ssize_t AOTOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int res = 0;
	struct vfd_ioctl_data data;
	dprintk(5, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);
	kernel_buf = kmalloc(len, GFP_KERNEL);
	if (kernel_buf == NULL)
	{
		printk("%s returns no memory <\n", __func__);
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
	if (!led_state[led].led_task || (ms < 1 && led < LED_COUNT - 1))
		return;
	led_state[led].period = ms;
	up(&led_state[led].led_sem);
}

static int AOTOMdev_open(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk(5, "%s >\n", __func__);
	minor = MINOR(inode->i_rdev);
	dprintk(1, "open minor %d\n", minor);
	if (minor != FRONTPANEL_MINOR_VFD)
		return -ENOTSUPP;
	open_count++;
	dprintk(5, "%s <\n", __func__);
	return 0;
}

static int AOTOMdev_close(struct inode *inode, struct file *filp)
{
	int minor;
	dprintk(5, "%s >\n", __func__);
	minor = MINOR(inode->i_rdev);
	dprintk(1, "close minor %d\n", minor);
	if (open_count > 0)
		open_count--;
	dprintk(5, "%s <\n", __func__);
	return 0;
}

static struct aotom_ioctl_data aotom_data;
static struct vfd_ioctl_data vfd_data;

static int AOTOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
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
		case VFDGETSTBYKEY:
		case VFDSETSTBYKEY:
		case VFDGETBLUEKEY:
		case VFDSETBLUEKEY:
			if (copy_from_user(&aotom_data, (void *) arg, sizeof(aotom_data)))
				return -EFAULT;
	}
	switch (cmd)
	{
		case VFDSETMODE:
			mode = aotom_data.u.mode.compat;
			break;
		case VFDSETLED:
			if (aotom_data.u.led.led_nr > -1 && aotom_data.u.led.led_nr < led_count)
			{
				switch (aotom_data.u.led.on)
				{
					case LED_OFF:
					case LED_ON:
						if (aotom_data.u.led.led_nr < LED_COUNT)
						{
							res = YWPANEL_VFD_SetLed(aotom_data.u.led.led_nr, aotom_data.u.led.on);
							led_state[aotom_data.u.led.led_nr].state = aotom_data.u.led.on;
							break;
						}
					default:
						// led 0-1: toggle LED for <aotom_data.u.led.on * 10 ms>
						// led 2:   set spinner segment time to <aotom_data.u.led.on * 10 ms> and enable spinner
						if (aotom_data.u.led.led_nr < led_count)
						{
							flashLED(aotom_data.u.led.led_nr, aotom_data.u.led.on * 10);
							res = 0;
						}
				}
			}
			break;
		case VFDBRIGHTNESS:
			if (aotom_data.u.brightness.level < 0)
				aotom_data.u.brightness.level = 0;
			else if (aotom_data.u.brightness.level > 7)
				aotom_data.u.brightness.level = 7;
			res = YWPANEL_VFD_SetBrightness(aotom_data.u.brightness.level);
			break;
		case VFDICONDISPLAYONOFF:
			switch (panel_version.DisplayInfo)
			{
				case YWPANEL_FP_DISPTYPE_LED:
					switch (aotom_data.u.icon.icon_nr)
					{
						case 0: // ?!?
							res = YWPANEL_VFD_SetLed(LED_RED, aotom_data.u.icon.on);
							led_state[LED_RED].state = aotom_data.u.icon.on;
							break;
						case 35:
						case AOTOM_DOT2:
							res = YWPANEL_VFD_SetLed(LED_GREEN, aotom_data.u.icon.on);
							led_state[LED_GREEN].state = aotom_data.u.icon.on;
							break;
						case 46:
						case AOTOM_ALL:
							led_state[LED_RED].state = aotom_data.u.icon.on;
							led_state[LED_GREEN].state = aotom_data.u.icon.on;
							YWPANEL_VFD_SetLed(LED_RED, aotom_data.u.icon.on);
							res = YWPANEL_VFD_SetLed(LED_GREEN, aotom_data.u.icon.on);
							break;
					}
					break;
				default:
				{
					int icon_nr = aotom_data.u.icon.icon_nr;
					if (icon_nr & ~0xff)
					{
						icon_nr >>= 8;
						switch (icon_nr)
						{
							case 0x11:
								icon_nr = AOTOM_DOUBLESCREEN;
								break;
							case 0x13:
								icon_nr = AOTOM_CA;
								break;
							case 0x15:
								icon_nr = AOTOM_MP3;
								break;
							case 0x17:
								icon_nr = AOTOM_AC3;
								break;
							case 0x1A:
								icon_nr = AOTOM_PLAY_LOG;
								break;
							case 0x1e:
								icon_nr = AOTOM_REC1;
								break;
							case 38:
								// AOTOM_DISK_S3
								break; //cd part1
							case 39:
								// AOTOM_DISK_S2
								break; //cd part2
							case 40:
								// AOTOM_DISK_S1
								break; //cd part3
							case 41:
								// AOTOM_DISK_S0
								break; //cd part4
							default:
								icon_nr = -1; //no additional symbols at the moment
								break;
						}
					}
					switch (icon_nr)
					{
						case 46:
						case AOTOM_ALL:
							VFD_set_all_icons(aotom_data.u.icon.on);
							res = 0;
						case -1:
							break;
						default:
							res = aotomSetIcon(icon_nr, aotom_data.u.icon.on);
					}
				}
				mode = 0;
				break;
			}
			break;
		case VFDSETPOWERONTIME:
		{
			u32 uTime = 0;
			get_user(uTime, (int *) arg);
			YWPANEL_FP_SetPowerOnTime(uTime);
			res = 0;
			break;
		}
		case VFDPOWEROFF:
			clear_display();
			YWPANEL_FP_ControlTimer(true);
			YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_STANDBY);
			res = 0;
			break;
		case VFDSTANDBY:
		{
			u32 uTime = 0;
			get_user(uTime, (int *) arg);
			YWPANEL_FP_SetPowerOnTime(uTime);
			clear_display();
			YWPANEL_FP_ControlTimer(true);
			YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_STANDBY);
			res = 0;
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
			u32 uTime = 0;
			uTime = YWPANEL_FP_GetTime();
			//printk("uTime = %d\n", uTime);
			res = put_user(uTime, (int *) arg);
			break;
		}
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
		case VFDDISPLAYCLR:
			vfd_data.length = 0;
			res = run_draw_thread(&vfd_data);
			break;
		case VFDGETWAKEUPMODE:
		case VFDDISPLAYWRITEONOFF:
			res = 0;
			break;
		case VFDGETSTARTUPSTATE:
		{
			YWPANEL_STARTUPSTATE_t State;
			if (YWPANEL_FP_GetStartUpState(&State))
				res = put_user(State, (int *) arg);
			break;
		}
		case VFDSETLOOPSTATE:
		{
			YWPANEL_LOOPSTATE_t State = YWPANEL_LOOPSTATE_UNKNOWN;
			res = get_user(State, (int *)arg);
			if (!res)
				res = YWPANEL_FP_SetLoopState(State);
			break;
		}
		case VFDGETLOOPSTATE:
		{
			YWPANEL_LOOPSTATE_t State;
			if (YWPANEL_FP_GetLoopState(&State))
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
		case VFDGETBLUEKEY:
		case VFDGETSTBYKEY:
		{
			if (YWPANEL_FP_GetKey(cmd == VFDGETBLUEKEY, aotom_data.u.key.key_nr, &aotom_data.u.key.key))
				res = copy_to_user((void *) arg, &aotom_data, sizeof(aotom_data));
			break;
		}
		case VFDSETBLUEKEY:
		case VFDSETSTBYKEY:
			res = !YWPANEL_FP_SetKey(cmd == VFDSETBLUEKEY, aotom_data.u.key.key_nr, aotom_data.u.key.key);
			break;
		default:
			printk("VFD/AOTOM: unknown IOCTL 0x%x\n", cmd);
		case 0x5305:
		case 0x5401:
			mode = 0;
			break;
	}
	up(&write_sem);
	dprintk(5, "%s <\n", __func__);
	return res;
}

static struct file_operations vfd_fops =
{
	.owner = THIS_MODULE,
	.ioctl = AOTOMdev_ioctl,
	.write = AOTOMdev_write,
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
		button_value = YWPANEL_VFD_GetKeyValue();
		if (button_value != KEY_UNKNOWN)
		{
			dprintk(5, "got button: %X\n", button_value);
			flashLED(LED_GREEN, 100);
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

static int button_dev_init(void)
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

static void button_dev_exit(void)
{
	dprintk(5, "[BTN] unregistering button device\n");
	input_unregister_device(button_dev);
}

static int aotom_reboot_event(struct notifier_block *nb, unsigned long event, void *ptr)
{
	switch (event)
	{
		case SYS_POWER_OFF:
			YWPANEL_VFD_ShowString("POWEROFF");
			break;
		case SYS_HALT:
			YWPANEL_VFD_ShowString("HALT");
			break;
		default:
			YWPANEL_VFD_ShowString("REBOOT");
			return NOTIFY_DONE;
	}
	msleep(1000);
	clear_display();
	YWPANEL_FP_ControlTimer(true);
	YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_STANDBY);
	return NOTIFY_DONE;
};

static struct notifier_block aotom_reboot_block =
{
	.notifier_call = aotom_reboot_event,
	.priority = INT_MAX,
};

static int aotom_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	u32 uTime = 0;
	//printk("%s\n", __func__);
	uTime = YWPANEL_FP_GetTime();
	rtc_time_to_tm(uTime, tm);
	return 0;
}

static int tm2time(struct rtc_time *tm)
{
	return mktime(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static int aotom_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int res = 0;
	//printk("%s\n", __func__);
	u32 uTime = tm2time(tm);
	res = YWPANEL_FP_SetTime(uTime);
	YWPANEL_FP_ControlTimer(true);
	return res;
}

static int aotom_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *al)
{
	return 0;
}

static int aotom_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *al)
{
	u32 a_time = 0;
	if (al->enabled)
		a_time = tm2time(&al->time);
	printk(KERN_INFO "%s enabled:%d time: %d\n", __func__, al->enabled, a_time);
	YWPANEL_FP_SetPowerOnTime(a_time);
	return 0;
}

static const struct rtc_class_ops aotom_rtc_ops =
{
	.read_time  = aotom_rtc_read_time,
	.set_time   = aotom_rtc_set_time,
	.read_alarm = aotom_rtc_read_alarm,
	.set_alarm  = aotom_rtc_set_alarm
};

static int __devinit aotom_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	/* I have no idea where the pdev comes from, but it needs the can_wakeup = 1
	 * otherwise we don't get the wakealarm sysfs attribute... :-) */
	pdev->dev.power.can_wakeup = 1;
	rtc = rtc_device_register("aotom", &pdev->dev, &aotom_rtc_ops, THIS_MODULE);
	printk(KERN_DEBUG "%s %p\n", __func__, rtc);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);
	printk(KERN_DEBUG "%s 2\n", __func__);
	platform_set_drvdata(pdev, rtc);
	return 0;
}

static int __devexit aotom_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc = platform_get_drvdata(pdev);
	printk(KERN_DEBUG "%s %p\n", __func__, rtc);
	rtc_device_unregister(rtc);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver aotom_rtc_driver =
{
	.probe = aotom_rtc_probe,
	.remove = __devexit_p(aotom_rtc_remove),
	.driver = {
		.name   = RTC_NAME,
		.owner  = THIS_MODULE
	},
};

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
	sema_init(&write_sem, 1);
	sema_init(&draw_thread_sem, 1);
	for (i = 0; i < LED_COUNT; i++)
	{
		led_state[i].state = LED_OFF;
		led_state[i].period = 0;
		led_state[i].stop = 1;
		sema_init(&led_state[i].led_sem, 0);
		led_state[i].led_task = kthread_run(led_thread, (void *) i, "led_thread");
	}
	if (panel_version.DisplayInfo == YWPANEL_FP_DISPTYPE_VFD)
	{
		led_state[led_count].state = LED_OFF;
		led_state[led_count].period = 0;
		led_state[led_count].stop = 1;
		sema_init(&led_state[led_count].led_sem, 0);
		led_state[led_count].led_task = kthread_run(spinner_thread, (void *) led_count, "spinner thread");
		led_count++;
	}
	register_reboot_notifier(&aotom_reboot_block);
	i = platform_driver_register(&aotom_rtc_driver);
	if (i)
		printk(KERN_ERR "%s platform_driver_register failed: %d\n", __func__, i);
	else
		rtc_pdev = platform_device_register_simple(RTC_NAME, -1, NULL, 0);
	if (IS_ERR(rtc_pdev))
		printk(KERN_ERR "%s platform_device_register_simple failed: %ld\n", __func__, PTR_ERR(rtc_pdev));
	dprintk(5, "%s <\n", __func__);
	proc_create("vfdlen", 0, NULL, &vfdlen_fops);
	return 0;
}

static int led_thread_active(void)
{
	int i;
	for (i = 0; i < led_count; i++)
		if (!led_state[i].stop && led_state[i].led_task)
			return 0;
	return -1;
}

static void __exit aotom_cleanup_module(void)
{
	int i;
	unregister_reboot_notifier(&aotom_reboot_block);
	platform_driver_unregister(&aotom_rtc_driver);
	platform_set_drvdata(rtc_pdev, NULL);
	platform_device_unregister(rtc_pdev);
	if ((draw_thread_status != DRAW_THREAD_STATUS_STOPPED) && draw_task)
		kthread_stop(draw_task);
	for (i = 0; i < led_count; i++)
		if (!led_state[i].stop && led_state[i].led_task)
		{
			up(&led_state[i].led_sem);
			kthread_stop(led_state[i].led_task);
		}
	while ((draw_thread_status != DRAW_THREAD_STATUS_STOPPED) && led_thread_active())
		msleep(1);
	dprintk(5, "[BTN] unloading ...\n");
	button_dev_exit();
	unregister_chrdev(VFD_MAJOR, "VFD");
	YWPANEL_VFD_Term();
	remove_proc_entry("vfdlen", NULL);
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
