/*
 * micom_ioctl.c
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
#include <linux/stm/pio.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>

#include "micom.h"
#include "micom_asc.h"
#include "../vfd/utf.h"

extern void ack_sem_up(void);
extern int  ack_sem_down(void);
extern void micom_putc(unsigned char data);

struct semaphore     write_sem;

int errorOccured = 0;

static char ioctl_data[8];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
	int   length;
	char  data[20];
};

static struct saved_data_s lastdata;

void write_sem_up(void)
{
	up(&write_sem);
}

int write_sem_down(void)
{
	return down_interruptible(&write_sem);
}

void copyData(unsigned char *data, int len)
{
	printk("%s len %d\n", __func__, len);
	memcpy(ioctl_data, data, len);
}

int micomWriteCommand(char command, char *buffer, int len, int needAck, int needLen)
{
	int i;

	dprintk(100, "%s >\n", __func__);

	if (len > 255)
		len = 255;

	/* Trzeba troche przyhamować  żeby nie zapychało seriala */
	udelay(1000);

	serial_putc(command);
	if (needLen == 1)
		serial_putc((char)len);

	for (i = 0; i < len; i++)
	{
		dprintk(90, "Put: %c (0x%02X)\n", buffer[i], buffer[i]);

		serial_putc(buffer[i]);

	}

	if (needAck)
	{
		if (ack_sem_down())
			return -ERESTARTSYS;
	}

	dprintk(100, "%s < \n", __func__);

	return 0;
}
static const unsigned short gVfdICONs[17][3] =
{

	/*0		ICON_TV,		*/	{ 0x2001,	NO_ICON,	NO_ICON },
	/*1		ICON_RADIO,    		*/	{ 0x1001,	NO_ICON,	NO_ICON },
	/*2	 	ICON_REPEAT,   		*/	{ 0x0801,	NO_ICON,	NO_ICON },
	/*3		ICON_CIRCLE		*/	{ 0x0501,	NO_ICON,	NO_ICON },
	/*4		ICON_RECORD,   		*/	{ 0x8000,	NO_ICON,	NO_ICON },
	/*5		ICON_POWER,    		*/	{ 0xC001,	NO_ICON,	NO_ICON },
	/*6		ICON_DOLBY,    		*/	{ 0x4000,	NO_ICON,	NO_ICON },
	/*7		ICON_MUTE,       	*/	{ 0x2000,	NO_ICON,	NO_ICON },
	/*8		ICON_DOLLAR,   		*/	{ 0x1000,	NO_ICON,	NO_ICON },
	/*9		ICON_LOCK,        	*/	{ 0x0800,	NO_ICON,	NO_ICON },
	/*a		ICON_CLOCK,		*/	{ 0x0400,	NO_ICON,	NO_ICON },
	/*b		ICON_HD,       		*/	{ 0x0200,	NO_ICON,	NO_ICON },
	/*c		ICON_DVD,        	*/	{ 0x0100,	NO_ICON,	NO_ICON },
	/*d		ICON_MP3,      		*/	{ 0x0080,	NO_ICON,	NO_ICON },
	/*e		ICON_NET,       	*/	{ 0x0040,	NO_ICON,	NO_ICON },
	/*f		ICON_USB,       	*/	{ 0x0020,	NO_ICON,	NO_ICON },
	/*10		ICON_MC,       		*/	{ 0x0010,	NO_ICON,	NO_ICON },

};

static unsigned short gVfdIconState   = 0; //figure
static unsigned short gVfdIconState2 = 0; //figure2
static unsigned short gVfdIconCircle  = 0; //circle
static unsigned char gcVfdIndex = 0;

int micomSetIcon(int which, int on)
{
// MemData = which  bOn = on

	unsigned char buffer[8] , i;
	unsigned short  icon_sel;
	int res = 0;

	if (on == 0 || on == 1) // in case ICON
	{
		if (which < 0 || which > 18)
			return -EINVAL;

		which = gVfdICONs[which][gcVfdIndex];


		if (which & 0x0001)
		{
			icon_sel = 2;

			if (which ==  0x0501)
				icon_sel = 3; //circle
		}
		else
		{
			icon_sel = 0;
		}

		if (on == 0)
		{
			if (!icon_sel)
				gVfdIconState  &= ~which;
			else if (icon_sel == 2)
				gVfdIconState2  &= ~which;
			else if (icon_sel == 3)
				gVfdIconCircle    = 0x4500;
		}
		else
		{
			if (!icon_sel)		gVfdIconState     |= which;
			else if (icon_sel == 2)	gVfdIconState2   |= which;
			else if (icon_sel == 3)     gVfdIconCircle     = 0x0500;

		}
	}
	/*else if(on == 3) // clear all ICON
	{
		gVfdIconState   = 0;
		gVfdIconState2 = 0;
		memset(buffer, 0, 8);


		//CLASS_FIGURE;
		buffer[0] = (unsigned char)((gVfdIconState >> 8) & 0xFF);
		buffer[1] = (unsigned char)(gVfdIconState & 0xFF);

		res = micomWriteCommand( CLASS_FIGURE, buffer, 7,0,0);
		mdelay(10);

		// CLASS_FIGURE2;
		buffer[0] = (unsigned char)((gVfdIconState2 >> 8) & 0xFF);

		res = micomWriteCommand( CLASS_FIGURE2, buffer, 7,0,0);
		mdelay(10);
		return res;

	}
	else
	{
		return;
	} */

	if (!icon_sel)
	{
		memset(buffer, 0, 8);
		//CLASS_FIGURE;
		buffer[0] = (unsigned char)((gVfdIconState >> 8) & 0xFF);
		buffer[1] = (unsigned char)(gVfdIconState & 0xFF);
		res = micomWriteCommand(CLASS_FIGURE, buffer, 7, 0, 0);

	}
	else if (icon_sel == 2)
	{
		memset(buffer, 0, 8);
		//CLASS_FIGURE2;
		buffer[0] = (unsigned char)((gVfdIconState2 >> 8) & 0xFF);
		res = micomWriteCommand(CLASS_FIGURE2, buffer, 7, 0, 0);
		mdelay(10);
	}
	else if (icon_sel == 3)
	{
		memset(buffer, 0, 8);
		//CLASS_CIRCLE;
		buffer[0] = (unsigned char)((gVfdIconCircle >> 8) & 0xFF);
		res = micomWriteCommand(CLASS_CIRCLE, buffer, 7, 0, 0);
		mdelay(10);
	}
	//else
	//{
	return res ;
	//}

}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetIcon);


//0 = Blue-LED on,  1 = Blue-LED off,
int micomSetBLUELED(int on)
{
	char buffer[8];
	int  res = 0;
	dprintk(100, "%s > %d\n", __func__,  on);

	memset(buffer, 0, 8);
	buffer[0] = on;


	res = micomWriteCommand(CLASS_BLUELED, buffer, 7, 1, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}


/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBLUELED);

//0 = Green-LED on,  1 = Green-LED off,
int micomSetGREENLED(int on)
{
	char buffer[8];
	int  res = 0;
	dprintk(100, "%s > %d\n", __func__,  on);

	memset(buffer, 0, 8);
	buffer[0] = 0xA0 + on;


	res = micomWriteCommand(CLASS_BLUELED, buffer, 7, 1, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetGREENLED);

//0 = RED-LED on,  1 = RED-LED off,
int micomSetREDLED(int on)
{
	char buffer[8];
	int  res = 0;
	dprintk(100, "%s > %d\n", __func__,  on);

	memset(buffer, 0, 8);
	buffer[0] = 0xB0 + on;


	res = micomWriteCommand(CLASS_BLUELED, buffer, 7, 1, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetREDLED);


int micomSetBrightness(int level)
{
	char buffer[8];
	int  res = 0;


	dprintk(5, "%s > %d\n", __func__, level);
	if (level < 0 || level > 7)
	{
		printk("VFD/MICOM brightness out of range %d\n", level);
		return -EINVAL;
	}

	memset(buffer, 0, 8);
	buffer[0] = level;

	res = micomWriteCommand(CLASS_INTENSITY, buffer, 7, 1, 0);

	dprintk(10, "%s <\n", __func__);

	return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBrightness);

int micomSetLedBrightness(int level)
{
	return 0;
}

EXPORT_SYMBOL(micomSetLedBrightness);

int micomInitialize(void)
{
	char buffer[8];
	int  res = 0;

	dprintk(5, "%s >\n", __func__);

	/* MICOM Tx On */
	memset(buffer, 0, 8);
	buffer[0] = 1;
	res = micomWriteCommand(CLASS_MCTX, buffer, 7, 1, 0);

	/* MICOM Tx Data Blocking Off */
	memset(buffer, 0, 8);
	buffer[0] = 0;
	res = micomWriteCommand(CLASS_BLOCK, buffer, 7, 1, 0);

	/* Mute request On */
	memset(buffer, 0, 8);
	buffer[0] = 1;
	res = micomWriteCommand(CLASS_MUTE, buffer, 7, 1, 0);

	/* Watchdog On */
	memset(buffer, 0, 8);
	buffer[0] = 1;
	res = micomWriteCommand(CLASS_WDT, buffer, 7, 1, 0);

	/* Send custom IR data */
	memset(buffer, 0, 8);
	buffer[0] = IR_CUSTOM_HI;
	buffer[1] = IR_CUSTOM_LO;
	buffer[2] = IR_POWER;
	buffer[3] = FRONT_POWER;
	buffer[4] = 0x01; //last power
	res = micomWriteCommand(CLASS_FIX_IR, buffer, 7, 1, 0);

	dprintk(10, "%s <\n", __func__);
	micomSetBrightness(5);
	micomSetBLUELED(0); //0   = Blue-LED on,  1   = Blue-LED off,
	return res;
}


int micomSetStandby(void)
{
	char       buffer[8];
	int      res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	buffer[0] = POWER_DOWN;
	res = micomWriteCommand(CLASS_POWER, buffer, 7, 1, 0);

	dprintk(10, "%s <\n", __func__);

	return res;
}

int micomSetDeepStandby(void)
{
	char 	   buffer[8];
	int      res = 0;

	memset(buffer, 0, 8);
	buffer[0] = POWER_DOWN;;

	res = micomWriteCommand(CLASS_POWER, buffer, 7, 1, 0);

	return res;
}


int micomSetTime(char *time)
{
	char buffer[5] = {0};
	int res = 0;
#if 0
	dprintk(5, "%s > \n", __func__);

	/* time: HH MM SS DD mm YY */

	memset(buffer, 0, 5);
	buffer[0] = time[3]; /* day */
	buffer[1] = time[0]; /* hour */
	buffer[2] = time[1]; /* minute */
	buffer[3] = time[2]; /* second */
	res = micomWriteCommand(CLASS_CLOCK, buffer, 4, 1, 0);

	memset(buffer, 0, 5);
	buffer[0] = 20; /* year high 20 */
	buffer[1] = time[5]; /* year low */
	buffer[2] = time[4]; /* month */
	res = micomWriteCommand(CLASS_CLOCKHI, buffer, 3, 1, 0);

	dprintk(10, "%s <\n", __func__);
#endif
	return res;
}

int micomGetVersion(void)
{
	char       buffer[8];
	int        res = 0;
#if 0
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	errorOccured   = 0;
	res = micomWriteCommand(0x05, buffer, 7, 1, 0);

	if (res < 0)
	{
		printk("%s < res %d\n", __func__, res);
		return res;
	}

	if (errorOccured)
	{
		memset(ioctl_data, 0, 8);
		printk("error\n");

		res = -ETIMEDOUT;
	}
	else
	{
		/* version received ->noop here */
		dprintk(1, "version received\n");
	}

	dprintk(10, "%s <\n", __func__);
#endif
	return res;
}

int micomGetTime(void)
{
	char       buffer[8];
	int        res = 0;
#if 0
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	errorOccured   = 0;
	res = micomWriteCommand(0x39, buffer, 7, 1, 0);

	if (res < 0)
	{
		printk("%s < res %d\n", __func__, res);
		return res;
	}

	if (errorOccured)
	{
		memset(ioctl_data, 0, 8);
		printk("error\n");

		res = -ETIMEDOUT;
	}
	else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
	}

	dprintk(10, "%s <\n", __func__);
#endif
	return res;
}

/* 0xc1 = rcu
 * 0xc2 = front
 * 0xc3 = time
 * 0xc4 = ac ???
 */
int micomGetWakeUpMode(void)
{
	char       buffer[8];
	int      res = 0;
#if 0
	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	errorOccured   = 0;
	res = micomWriteCommand(0x43, buffer, 7, 1, 0);

	if (res < 0)
	{
		printk("%s < res %d\n", __func__, res);
		return res;
	}

	if (errorOccured)
	{
		memset(ioctl_data, 0, 8);
		printk("error\n");

		res = -ETIMEDOUT;
	}
	else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
	}

	dprintk(10, "%s <\n", __func__);
#endif
	return res;
}

int micomReboot(void)
{
	char       buffer[8];
	int      res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	buffer[0] = 0;
	buffer[1] = POWER_RESET;
	res = micomWriteCommand(CLASS_POWER, buffer, 7, 1, 0);

	dprintk(10, "%s <\n", __func__);

	return res;
}


int micomWriteString(unsigned char *aBuf, int len)
{
	/*Wyswietlacz jest 11 znakowy aby nie skrolowalo
	  ustawiamy bufor na 11 znakow */
	unsigned char bBuf[11];
	int i = 0;
	int j = 0;
	int res = 0;

	dprintk(100, "%s >\n", __func__);
	/* Czyscimy bufor */
	memset(bBuf, ' ', 11);
	/* Kopiujemy tylko 11 bajt */
	memcpy(bBuf, aBuf, (len > 11) ? 11 : len);

	res = micomWriteCommand(CLASS_DISPLAY2, bBuf, 11, 1, 1);

	dprintk(100, "%s <\n", __func__);

	return res;
}

int micom_init_func(void)
{
	int vLoop;

	dprintk(5, "%s >\n", __func__);

	sema_init(&write_sem, 1);

	printk("Vitamin HD5000 VFD/MICOM module initializing\n");

	micomInitialize();

	micomSetBrightness(4);
	//micomSetLedBrightness(0x80);//0x50
	micomWriteString(" Vitamin HD ", strlen(" Vitamin HD "));

#if 0
	for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++)
		micomSetIcon(vLoop, 0);
#endif
	dprintk(10, "%s <\n", __func__);

	return 0;
}

static ssize_t MICOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int minor, vLoop, res = 0;

	dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	if (len == 0)
		return len;

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

	dprintk(70, "minor = %d\n", minor);

	/* dont write to the remote control */
	if (minor == FRONTPANEL_MINOR_RC)
		return -EOPNOTSUPP;

	kernel_buf = kmalloc(len, GFP_KERNEL);

	if (kernel_buf == NULL)
	{
		printk("%s return no mem<\n", __func__);
		return -ENOMEM;
	}

	copy_from_user(kernel_buf, buff, len);

	if (write_sem_down())
		return -ERESTARTSYS;

	/* Dagobert: echo add a \n which will be counted as a char
	 */
	if (kernel_buf[len - 1] == '\n')
		res = micomWriteString(kernel_buf, len - 1);
	else
		res = micomWriteString(kernel_buf, len);

	kfree(kernel_buf);

	write_sem_up();

	dprintk(100, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
		return res;
	else
		return len;
}

static ssize_t MICOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;
	dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

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

	dprintk(100, "minor = %d\n", minor);

	if (minor == FRONTPANEL_MINOR_RC)
	{
		int           size = 0;
		unsigned char data[20];

		memset(data, 0, 20);

		getRCData(data, &size);

		if (size > 0)
		{
			if (down_interruptible(&FrontPanelOpen[minor].sem))
				return -ERESTARTSYS;

			copy_to_user(buff, data, size);

			up(&FrontPanelOpen[minor].sem);

			dprintk(150, "%s < %d\n", __func__, size);
			return size;
		}

		dumpValues();

		return 0;
	}

	/* copy the current display string to the user */
	if (down_interruptible(&FrontPanelOpen[minor].sem))
	{
		printk("%s return erestartsys<\n", __func__);
		return -ERESTARTSYS;
	}

	if (FrontPanelOpen[minor].read == lastdata.length)
	{
		FrontPanelOpen[minor].read = 0;

		up(&FrontPanelOpen[minor].sem);
		printk("%s return 0<\n", __func__);
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

	dprintk(100, "%s < (len %d)\n", __func__, len);
	return len;
}

int MICOMdev_open(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(100, "%s >\n", __func__);

	/* needed! otherwise a racecondition can occur */
	if (down_interruptible(&write_sem))
		return -ERESTARTSYS;

	minor = MINOR(inode->i_rdev);

	dprintk(70, "open minor %d\n", minor);

	if (FrontPanelOpen[minor].fp != NULL)
	{
		printk("EUSER\n");
		up(&write_sem);
		return -EUSERS;
	}
	FrontPanelOpen[minor].fp = filp;
	FrontPanelOpen[minor].read = 0;

	up(&write_sem);

	dprintk(100, "%s <\n", __func__);

	return 0;

}

int MICOMdev_close(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(100, "%s >\n", __func__);

	minor = MINOR(inode->i_rdev);

	dprintk(70, "close minor %d\n", minor);

	if (FrontPanelOpen[minor].fp == NULL)
	{
		printk("EUSER\n");
		return -EUSERS;
	}

	FrontPanelOpen[minor].fp = NULL;
	FrontPanelOpen[minor].read = 0;

	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int MICOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{

	struct micom_ioctl_data *micom = (struct micom_ioctl_data *)arg;
	int res = 0;

	dprintk(100, "%s > 0x%.8x\n", __func__, cmd);

	if (write_sem_down())
		return -ERESTARTSYS;


	switch (cmd)
	{
		case VFDSETMODE:
		{

			break;
		}
		case VFDSETLED:
		{

			res = micomSetBLUELED(micom->u.led.on);

			break;
		}

		case VFDSETREDLED:
		{

			res = micomSetREDLED(micom->u.led.on);

			break;
		}

		case VFDSETGREENLED:
		{

			res = micomSetGREENLED(micom->u.led.on);

			break;
		}
		case VFDBRIGHTNESS:
		{

			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
			int level = data->start_address;

			/* scale level from 0 - 7 to a range from 1 - 5 where 5 is off */
			//level = 7 - level;
			//level =  ((level * 100) / 7 * 5) / 100 + 1;
			res = micomSetBrightness(level);
			break;
		}
		case VFDLEDBRIGHTNESS:
		{

			break;
		}
		case VFDDRIVERINIT:
		{

			res = micom_init_func();
			break;
		}
		case VFDICONDISPLAYONOFF:
		{


			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
			int icon_nr = (data->data[0] & 0xf) + 1;
			int on = data->data[4];

			res = micomSetIcon(icon_nr, on);

			break;
		}
		case VFDSTANDBY:
		{
#if 0
			res = micomSetStandby();
#endif
			break;
		}
		case VFDDEEPSTANDBY:
		{
			res = micomSetDeepStandby();
			break;

		}
		case VFDREBOOT:
		{
#if 0
			res = micomReboot();
			break;
#endif
		}
		case VFDSETTIME:
		{
			if (micom->u.time.time != 0)
				res = micomSetTime(micom->u.time.time);

			break;
		}
		case VFDGETTIME:
		{
#if 0
			res = micomGetTime();
			copy_to_user(arg, &ioctl_data, 8);
#endif
			break;
		}
		case VFDGETVERSION:
		{
#if 0
			res = micomGetVersion();
			copy_to_user(arg, &ioctl_data, 8);
#endif
			break;
		}
		case VFDGETWAKEUPMODE:
		{
#if 0
			res = micomGetWakeUpMode();
			copy_to_user(arg, &ioctl_data, 8);
#endif
			break;
		}
		case VFDDISPLAYCHARS:
		{
			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;

			res = micomWriteString(data->data, data->length);

			break;
		}
		case VFDDISPLAYWRITEONOFF:
		{
			break;
		}
		case 0x5305:
			break;

		default:
		{
			printk("VFD/MICOM: unknown IOCTL 0x%x\n", cmd);

			break;
		}
	}


	printk("VFD/MICOM: IOCTL 0x%08X\n", cmd);

	write_sem_up();

	dprintk(100, "%s <\n", __func__);
	return res;
}

struct file_operations vfd_fops =
{
	.owner = THIS_MODULE,
	.ioctl = MICOMdev_ioctl,
	.write = MICOMdev_write,
	.read  = MICOMdev_read,
	.open  = MICOMdev_open,
	.release  = MICOMdev_close
};

