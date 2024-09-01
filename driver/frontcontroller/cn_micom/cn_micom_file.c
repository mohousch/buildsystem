/*
 * cn_micom_file.c
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
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>
#include <linux/syscalls.h>

#include "cn_micom.h"
#include "cn_micom_asc.h"
#include "cn_micom_utf.h"
#include "cn_micom_char.h"

extern void ack_sem_up(void);
extern int  ack_sem_down(void);
int mcom_WriteString(unsigned char *aBuf, int len);
extern void mcom_putc(unsigned char data);

struct semaphore     write_sem;

int errorOccured = 0;

static unsigned char ioctl_data[12];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
	int   length;
	char  data[128];
};

#define OW_TaskDelay(x)              msleep(x)

#if 1
#define PRN_LOG(msg, args...)        printk(msg, ##args)
#define PRN_DUMP(str, data, len)     mcom_Dump(str, data, len)
#else
#define PRN_LOG(msg, args...)
#define PRN_DUMP(str, data, len)
#endif

#define MAX_MCOM_MSG        30

#define MCU_COMM_PORT       UART_1
#define MCU_COMM_TIMEOUT    1000

#define _MCU_KEYIN          0xF1
#define _MCU_DISPLAY        0xED
#define _MCU_STANDBY        0xE5
#define _MCU_NOWTIME        0xEC
#define _MCU_WAKEUP         0xEA
#define _MCU_BOOT           0xEB
#define _MCU_TIME           0xFC
#define _MCU_RESPONSE       0xC5
#define _MCU_ICON           0xE1
#define _MCU_VERSION        0xDB
#define _MCU_REBOOT         0xE6
#define _MCU_PRIVATE        0xE7
#define _MCU_KEYCODE        0xE9

#define _TAG                0
#define _LEN                1
#define _VAL                2

typedef enum
{
	_VFD,
	_7SEG
} DISPLAYTYPE;

static unsigned int         _1SEC  = 1000;
static unsigned int         _100MS = 100;

static time_t               mcom_time_set_time;
static unsigned char        mcom_time[12];

static int                  mcom_settime_set_time_flag = 0;
static time_t               mcom_settime_set_time;
static time_t               mcom_settime;

static unsigned char        mcom_version[4];
static unsigned char        mcom_private[8]; // 0000 0000(Boot fron AC on), 0000 0001(Boot fron standby), 0000 0002(micom did not use this value, bug)

static char    *mcom_ascii_char_7seg[] =
{
	_7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    // 0 ~  7 (0x00 ~ 0x07)
	_7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    // 8 ~ 15 (0x08 ~ 0x0F)
	_7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    //16 ~ 23 (0x10 ~ 0x18)
	_7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    //24 ~ 31 (0x19 ~ 0x1F)
	_7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    //32 ~ 39 (0x20 ~ 0x27)
	_7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    //40 ~ 47 (0x28 ~ 0x2F)
	_7SEG_NUM_0,   _7SEG_NUM_1,   _7SEG_NUM_2,   _7SEG_NUM_3,   _7SEG_NUM_4,   _7SEG_NUM_5,   _7SEG_NUM_6,   _7SEG_NUM_7,
	_7SEG_NUM_8,   _7SEG_NUM_9,   _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,
	_7SEG_NULL,    _7SEG_CH_A,    _7SEG_CH_B,    _7SEG_CH_C,    _7SEG_CH_D,    _7SEG_CH_E,    _7SEG_CH_F,    _7SEG_CH_G,
	_7SEG_CH_H,    _7SEG_CH_I,    _7SEG_CH_J,    _7SEG_CH_K,    _7SEG_CH_L,    _7SEG_CH_M,    _7SEG_CH_N,    _7SEG_CH_O,
	_7SEG_CH_P,    _7SEG_CH_Q,    _7SEG_CH_R,    _7SEG_CH_S,    _7SEG_CH_T,    _7SEG_CH_U,    _7SEG_CH_V,    _7SEG_CH_W,
	_7SEG_CH_X,    _7SEG_CH_Y,    _7SEG_CH_Z,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,
	_7SEG_NULL,    _7SEG_CH_A,    _7SEG_CH_B,    _7SEG_CH_C,    _7SEG_CH_D,    _7SEG_CH_E,    _7SEG_CH_F,    _7SEG_CH_G,
	_7SEG_CH_H,    _7SEG_CH_I,    _7SEG_CH_J,    _7SEG_CH_K,    _7SEG_CH_L,    _7SEG_CH_M,    _7SEG_CH_N,    _7SEG_CH_O,
	_7SEG_CH_P,    _7SEG_CH_Q,    _7SEG_CH_R,    _7SEG_CH_S,    _7SEG_CH_T,    _7SEG_CH_U,    _7SEG_CH_V,    _7SEG_CH_W,
	_7SEG_CH_X,    _7SEG_CH_Y,    _7SEG_CH_Z,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL,    _7SEG_NULL
};

static char    *mcom_ascii_char_14seg[] =
{
	_14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    // 0 ~  7 (0x00 ~ 0x07)
	_14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    // 8 ~ 15 (0x08 ~ 0x0F)
	_14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    //16 ~ 23 (0x10 ~ 0x18)
	_14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    //24 ~ 31 (0x19 ~ 0x1F)
	_14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    //32 ~ 39 (0x20 ~ 0x27)
	_14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    //40 ~ 47 (0x28 ~ 0x2F)
	_14SEG_NUM_0,   _14SEG_NUM_1,   _14SEG_NUM_2,   _14SEG_NUM_3,   _14SEG_NUM_4,   _14SEG_NUM_5,   _14SEG_NUM_6,   _14SEG_NUM_7,
	_14SEG_NUM_8,   _14SEG_NUM_9,   _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,
	_14SEG_NULL,    _14SEG_CH_A,    _14SEG_CH_B,    _14SEG_CH_C,    _14SEG_CH_D,    _14SEG_CH_E,    _14SEG_CH_F,    _14SEG_CH_G,
	_14SEG_CH_H,    _14SEG_CH_I,    _14SEG_CH_J,    _14SEG_CH_K,    _14SEG_CH_L,    _14SEG_CH_M,    _14SEG_CH_N,    _14SEG_CH_O,
	_14SEG_CH_P,    _14SEG_CH_Q,    _14SEG_CH_R,    _14SEG_CH_S,    _14SEG_CH_T,    _14SEG_CH_U,    _14SEG_CH_V,    _14SEG_CH_W,
	_14SEG_CH_X,    _14SEG_CH_Y,    _14SEG_CH_Z,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,
	_14SEG_NULL,    _14SEG_CH_A,    _14SEG_CH_B,    _14SEG_CH_C,    _14SEG_CH_D,    _14SEG_CH_E,    _14SEG_CH_F,    _14SEG_CH_G,
	_14SEG_CH_H,    _14SEG_CH_I,    _14SEG_CH_J,    _14SEG_CH_K,    _14SEG_CH_L,    _14SEG_CH_M,    _14SEG_CH_N,    _14SEG_CH_O,
	_14SEG_CH_P,    _14SEG_CH_Q,    _14SEG_CH_R,    _14SEG_CH_S,    _14SEG_CH_T,    _14SEG_CH_U,    _14SEG_CH_V,    _14SEG_CH_W,
	_14SEG_CH_X,    _14SEG_CH_Y,    _14SEG_CH_Z,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL,    _14SEG_NULL
};

static struct saved_data_s lastdata;


static void mcom_Dump(char *string, unsigned char *v_pData, unsigned short v_szData)
{
	int i;
	char str[10], line[80], txt[25];
	unsigned char    *pData;

	if ((v_pData == NULL) || (v_szData == 0))
	{
		return;
	}

	printk("[ %s [%x]]\n", string, v_szData);

	i = 0;
	pData = v_pData;
	line[0] = '\0';
	while ((unsigned short)(pData - v_pData) < v_szData)
	{
		sprintf(str, "%02x ", *pData);
		strcat(line, str);
		txt[i] = ((*pData >= 0x20) && (*pData < 0x7F))  ? *pData : '.';
		i++;

		if (i == 16)
		{
			txt[i] = '\0';
			printk("%s %s\n", line, txt);
			i = 0;
			line[0] = '\0';
		}
		pData++;
	}
	if (i != 0)
	{
		txt[i] = '\0';
		do
			strcat(line, "   ");
		while (++i < 16);

		printk("%s %s\n\n", line, txt);
	}
}

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

int mcomWriteCommand(char *buffer, int len, int needAck)
{
	int i;

	dprintk(150, "%s >\n", __func__);

	for (i = 0; i < len; i++)
	{
#ifdef DIRECT_ASC
		serial_putc(buffer[i]);
#else
		mcom_putc(buffer[i]);
#endif
	}

	if (needAck)
		if (ack_sem_down())
			return -ERESTARTSYS;

	dprintk(150, "%s < \n", __func__);

	return 0;
}

static DISPLAYTYPE mcomGetDisplayType(void)
{
	if (mcom_version[0] == 4)
		return _7SEG;
	if (mcom_version[0] == 5)
		return _VFD;

	return _VFD;
}

static unsigned short mcomYMD2MJD(unsigned short Y, unsigned char M, unsigned char D)
{
	int     L;


	Y -= 1900;

	L = ((M == 1) || (M == 2)) ? 1 : 0 ;

	return (unsigned short)(14956 + D + ((Y - L) * 36525 / 100) + ((M + 1 + L * 12) * 306001 / 10000));
}

static void mcom_MJD2YMD(unsigned short usMJD, unsigned short *year, unsigned char *month, unsigned char *day)
{
	int		Y, M, D, K;

	Y = (usMJD * 100 - 1507820) / 36525;
	M = ((usMJD * 10000 - 149561000) - (Y * 36525 / 100) * 10000) / 306001;
	D = usMJD - 14956 - (Y * 36525 / 100) - (M * 306001 / 10000);

	K = ((M == 14) || (M == 15)) ? 1 : 0 ;

	Y = Y + K + 1900;
	M = M - 1 - K * 12;

	*year  = (unsigned short)Y;
	*month = (unsigned char)M;
	*day   = (unsigned char)D;
}

int mcomSetStandby(char *time)
{
	char    comm_buf[16];
	int     res = 0;
	int     showTime = 0;
	int     wakeup = 0;

	dprintk(100, "%s >\n", __func__);

	if (mcomGetDisplayType() == _VFD)
		res = mcom_WriteString("Bye bye ...", strlen("Bye bye ..."));
	else
		res = mcom_WriteString("WAIT", strlen("WAIT"));

	// send "STANDBY"
	comm_buf[_TAG] = _MCU_STANDBY;
	comm_buf[_LEN] = 5;
	comm_buf[_VAL + 0] = (showTime) ? 1 : 0 ;
	comm_buf[_VAL + 1] = 1;		// 24시간체계
	comm_buf[_VAL + 2] = (wakeup) ? 1 : 0 ;
	comm_buf[_VAL + 3] = 3;		// power on delay
	comm_buf[_VAL + 4] = 1; 		// extra data (private data)

	PRN_DUMP("@@@ SEND STANDBY @@@", comm_buf, 2 + comm_buf[_LEN]);

	res = mcomWriteCommand(comm_buf, 7, 0);

	if (res != 0)		goto _EXIT_MCOM_STANDBY;
	OW_TaskDelay(_100MS);

	// send "NOWTIME"
	comm_buf[_TAG] = _MCU_NOWTIME;
	comm_buf[_LEN] = 5;

	if (mcom_settime_set_time_flag)
	{
		unsigned short      mjd;
		unsigned short      year;
		unsigned char       month;
		unsigned char       day;
		time_t              curr_time;
		struct timespec     tp;

		tp = current_kernel_time();

		PRN_LOG("mcomSetTime: %d\n", tp.tv_sec);

		curr_time = mcom_settime + (tp.tv_sec - mcom_settime_set_time);

		mjd = (curr_time / (24 * 60 * 60)) + 40587;

		mcom_MJD2YMD(mjd, &year, &month, &day);

		comm_buf[_VAL + 0] = (year - 2000) & 0xff;
		comm_buf[_VAL + 1] = month;
		comm_buf[_VAL + 2] = day;
		comm_buf[_VAL + 3] = (curr_time / (24 * 60 * 60)) / (60 * 60);
		comm_buf[_VAL + 4] = ((curr_time / (24 * 60 * 60)) % (60 * 60)) / 60;

#if 0
		mjd      = (mcom_settime[0] << 8) | mcom_settime[1];
		time_sec = mcom_settime[2] * 60 * 60 + mcom_settime[3] * 60 + mcom_settime[4] +
			   (tp.tv_sec - mcom_settime_set_time);

		mjd          += time_sec / (24 * 60 * 60);
		curr_time_sec = time_sec % (24 * 60 * 60);

		mcom_MJD2YMD(mjd, &year, &month, &day);

		comm_buf[_VAL + 0] = (year - 2000) & 0xff;
		comm_buf[_VAL + 1] = month;
		comm_buf[_VAL + 2] = day;
		comm_buf[_VAL + 3] = curr_time_sec / (60 * 60);
		comm_buf[_VAL + 4] = (curr_time_sec % (60 * 60)) / 60;
#endif
	}
	else
	{

	}

	PRN_DUMP("@@@ SEND TIME @@@", comm_buf, 2 + comm_buf[_LEN]);

	res = mcomWriteCommand(comm_buf, 7, 0);
	if (res != 0)		goto _EXIT_MCOM_STANDBY;
	OW_TaskDelay(_100MS);

	if (wakeup)
	{
		/* TO BE DONE!! */
		// send "WAKEUP"
		comm_buf[_TAG] = _MCU_WAKEUP;

		comm_buf[_LEN] = 3;
#if 1
		comm_buf[_VAL + 0] = 0xFF;
		comm_buf[_VAL + 1] = 0xFF;
		comm_buf[_VAL + 2] = 0xFF;
#else
		comm_buf[_VAL + 0] = (after_min >> 16) & 0xFF;
		comm_buf[_VAL + 1] = (after_min >>  8) & 0xFF;
		comm_buf[_VAL + 2] = (after_min >>  0) & 0xFF;
#endif

		PRN_DUMP("@@@ SEND WAKEUP @@@", comm_buf, 2 + comm_buf[_LEN]);

		res = mcomWriteCommand(comm_buf, 5, 0);
		if (res != 0)		goto _EXIT_MCOM_STANDBY;
		OW_TaskDelay(_100MS);

	}

	// send "PRIVATE"
	comm_buf[_TAG] = _MCU_PRIVATE;
	comm_buf[_LEN] = 8;
	comm_buf[_VAL + 0] = 0;
	comm_buf[_VAL + 1] = 1;
	comm_buf[_VAL + 2] = 2;
	comm_buf[_VAL + 3] = 3;
	comm_buf[_VAL + 4] = 4;
	comm_buf[_VAL + 5] = 5;
	comm_buf[_VAL + 6] = 6;
	comm_buf[_VAL + 7] = 7;

	memset(&comm_buf + _VAL, 1, 8);
	res = mcomWriteCommand(comm_buf, 10, 0);
	if (res != 0)		goto _EXIT_MCOM_STANDBY;
	OW_TaskDelay(_100MS);

_EXIT_MCOM_STANDBY:
	dprintk(100, "%s <\n", __func__);

	return res;
}

int mcomSetTime(time_t time)
{
	struct timespec     tp;

	tp = current_kernel_time();

	PRN_LOG("mcomSetTime: %d\n", tp.tv_sec);

	mcom_settime_set_time = tp.tv_sec;

	mcom_settime = time;

	mcom_settime_set_time_flag = 1;

	return 0;
}

int mcomGetTime(void)
{
	unsigned short      cur_mjd;
	unsigned int        cur_time_in_sec;
	unsigned int        time_in_sec;
	unsigned int        passed_time_in_sec;
	struct timespec     tp_now;

	cur_mjd = mcomYMD2MJD(mcom_time[0] + 2000, mcom_time[1], mcom_time[2]);

	time_in_sec = mcom_time[3] * 60 * 60 + mcom_time[4] * 60 +
		      ((mcom_time[5] << 16) | (mcom_time[6] << 8) | mcom_time[7]) * 60 +
		      (tp_now.tv_sec - mcom_time_set_time);

	cur_mjd        += time_in_sec / (24 * 60 * 60);
	cur_time_in_sec = time_in_sec % (24 * 60 * 60);

	ioctl_data[0] = cur_mjd >> 8;
	ioctl_data[1] = cur_mjd & 0xff;
	ioctl_data[2] = cur_time_in_sec / (60 * 60);
	ioctl_data[3] = (cur_time_in_sec % (60 * 60)) / 60;
	ioctl_data[4] = (cur_time_in_sec % (60 * 60)) % 60;

	return 0;
}

int mcomGetWakeUpMode(void)
{
	if (mcom_version[0] == 4)            // 4.x.x
	{
		ioctl_data[0] = 0;
	}
	else if (mcom_version[0] == 5)        // 5.x.x
	{
		ioctl_data[0] = 0;
	}
	else
	{
		ioctl_data[0] = 0;
	}

	return 0;
}

int mcom_WriteString(unsigned char *aBuf, int len)
{
	unsigned char bBuf[128];
	int i = 0;
	int j = 0;
	int payload_len;
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(bBuf, ' ', 128);

	for (i = 0, payload_len = 0; i < len; i++)
	{
		// workaround start
		if (aBuf[i] > 0x7f)
			continue;
		// workaround end

		if (mcomGetDisplayType() == _VFD)    // VFD display
		{
			memcpy(bBuf + _VAL + payload_len, mcom_ascii_char_14seg[aBuf[i]], 2);
			payload_len += 2;
		}
		else if (mcomGetDisplayType() == _7SEG)    // 4 digit 7-Seg display
		{
			memcpy(bBuf + _VAL + payload_len, mcom_ascii_char_7seg[aBuf[i]], 1);
			payload_len += 1;
		}
		else
		{
			// Unknown
		}
	}

	/* start of command write */
	bBuf[0] = _MCU_DISPLAY;
	bBuf[1] = payload_len;

	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 128);
	lastdata.length = len;

	dprintk(70, "len %d\n", len);

	res = mcomWriteCommand(bBuf, payload_len + 2, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}


static unsigned char mcom_Checksum(unsigned char *payload, int len)
{
	int                    n;
	unsigned char        checksum = 0;


	for (n = 0; n < len; n++)
		checksum ^= payload[n];

	return checksum;
}

static int mcom_SendResponse(char *buf, int needack)
{
	unsigned char    response[3];
	int                res = 0;

	response[_TAG] = _MCU_RESPONSE;
	response[_LEN] = 1;
	response[_VAL] = mcom_Checksum(buf, 2/*tag+len*/ + buf[_LEN]);

	if (needack)
	{
		errorOccured = 0;
		res = mcomWriteCommand((char *)response, 3, 1);

		if (errorOccured == 1)
		{
			/* error */
			memset(ioctl_data, 0, 8);
			printk("error\n");

			res = -ETIMEDOUT;
		}

		return res;
	}
	else
	{
		mcomWriteCommand((char *)response, 3, 0);

		return 0;
	}
}

int mcom_init_func(void)
{
	unsigned char        comm_buf[20];
	unsigned char        n, k;
	unsigned char        checksum;
	int                    res;
	int                    bootSendCount;
#if !defined(SUPPORT_MINILINE)
	int                    seq_count = -1;
#endif


	dprintk(100, "%s >\n", __func__);

	sema_init(&write_sem, 1);

MCOM_RECOVER :


#if 0
#if !defined(SUPPORT_MINILINE)
	seq_count++;
	if (seq_count > 0)
	{
		mcom_FlushData();
		if ((seq_count % 10) == 0)
		{
			mcom_SetupRetryDisplay(seq_count);
			OW_TaskDelay(_1SEC * 2);
		}
	}
#endif
#endif
	bootSendCount = 0;
	while (1)
	{
		// send "BOOT"
		comm_buf[_TAG] = _MCU_BOOT;
		comm_buf[_LEN] = n = 1;
		comm_buf[_VAL] = 0x01;    // dummy

		errorOccured = 0;
		PRN_DUMP("## SEND:BOOT ##", comm_buf, 2/*tag+len*/ + n);
		res = mcomWriteCommand(comm_buf, 3, 1);

		if (errorOccured == 1)
		{
			/* error */
			memset(ioctl_data, 0, 8);
			printk("error\n");

			OW_TaskDelay(_100MS);
			goto MCOM_RECOVER;
		}
		else
		{
			/* time received ->noop here */
			dprintk(1, "time received\n");
			PRN_LOG("received\n");
		}

		// recv "VERSION" or "TIME"
		PRN_DUMP("## RECV:VERSION or TIME ", ioctl_data, 2/*tag+len*/ + ioctl_data[_LEN]);

		if (ioctl_data[_TAG] == _MCU_VERSION)
		{
			memcpy(mcom_version, ioctl_data + _VAL, ioctl_data[_LEN]);
			PRN_LOG("_MCU_VERSION %02X:%02X:%02X:%02X\n", mcom_version[0], mcom_version[1], mcom_version[2], mcom_version[3]);
			break;
		}
		else if (ioctl_data[_TAG] == _MCU_TIME)
		{
			struct timespec     tp;

			tp = current_kernel_time();

			mcom_time_set_time = tp.tv_sec;
			memcpy(mcom_time, ioctl_data + _VAL, ioctl_data[_LEN]);

			PRN_LOG("_MCU_TIME: %d\n", mcom_time_set_time);

			break;
		}

		PRN_LOG("tag = %02x\n", ioctl_data[_TAG]);

		if (bootSendCount++ == 3)
			goto MCOM_RECOVER;

		OW_TaskDelay(_100MS);
	}

	// send "RESPONSE" and recv "TIME" or "VERSION
	res = mcom_SendResponse(ioctl_data, 1);
	if (res)
	{
		OW_TaskDelay(_100MS);
		goto MCOM_RECOVER;
	}

	PRN_DUMP("## RECV:TIME or VERSION ", ioctl_data, ioctl_data[_LEN] + 2);
	if (ioctl_data[_TAG] == _MCU_TIME)
	{
		struct timespec     tp;

		tp = current_kernel_time();

		mcom_time_set_time = tp.tv_sec;
		memcpy(mcom_time, ioctl_data + _VAL, ioctl_data[_LEN]);

		PRN_LOG("_MCU_TIME: %d\n", mcom_time_set_time);
	}
	else if (ioctl_data[_TAG] == _MCU_VERSION)
	{
		memcpy(mcom_version, ioctl_data + _VAL, ioctl_data[_LEN]);
	}
	else
	{
		OW_TaskDelay(_100MS);
		goto MCOM_RECOVER;
	}

//    PRN_LOG("mcutype[%d]\n", MCOM_GetMcuType());
	if (1)   //(MCOM_GetMcuType() > _MCU_TYPE_1)
	{
		// recv "PRIVATE"
		res = mcom_SendResponse(ioctl_data, 1);
		if (res)
		{
			OW_TaskDelay(_100MS);
			goto MCOM_RECOVER;
		}

		PRN_DUMP("## RECV:PRIVATE ##", ioctl_data, ioctl_data[_LEN] + 2);
		memcpy(mcom_private, ioctl_data + _VAL, ioctl_data[_LEN]);

		mcom_SendResponse(ioctl_data, 0);

		OW_TaskDelay(_100MS);
		for (k = 0; k < 3; k++)
		{
			// send "KEYCODE"
			comm_buf[_TAG] = _MCU_KEYCODE;
			comm_buf[_LEN] = n = 4;
			comm_buf[_VAL + 0] = 0x04;
			comm_buf[_VAL + 1] = 0xF3;
			comm_buf[_VAL + 2] = 0x5F;
			comm_buf[_VAL + 3] = 0xA0;

			checksum = mcom_Checksum(comm_buf, 6);

			errorOccured = 0;
			PRN_DUMP("## SEND:KEYCODE ##", comm_buf, 2/*tag+len*/ + n);
			res = mcomWriteCommand(comm_buf, 6, 1);

			if (errorOccured == 1)
			{
				/* error */
				memset(ioctl_data, 0, 8);
				printk("error\n");

				OW_TaskDelay(_100MS);
				goto MCOM_RECOVER;
			}

			if (res == 0)
			{
				if (checksum == ioctl_data[_VAL])
					break;
			}
		}

		if (k >= 3)
		{
			OW_TaskDelay(_100MS);
			goto MCOM_RECOVER;
		}

	}

	dprintk(100, "%s <\n", __func__);

	return 0;
}

static ssize_t MCOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int minor, vLoop, res = 0;

	dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);
	//printk("%s > (len %d, offs %d)\n", __func__, len, (int) *off);

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
		res = mcom_WriteString(kernel_buf, len - 1);
	else
		res = mcom_WriteString(kernel_buf, len);

	kfree(kernel_buf);

	write_sem_up();

	dprintk(70, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
		return res;
	else
		return len;
}

static ssize_t MCOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;

	//printk("%s > (len %d, offs %d)\n", __func__, len, (int) *off);
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
		return -EUSERS;
	}

	dprintk(100, "minor = %d\n", minor);
	//printk("minor = %d\n", minor);

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

			dprintk(100, "%s < %d\n", __func__, size);
			return size;
		}

		//printk("size = %d\n", size);
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

int MCOMdev_open(struct inode *inode, struct file *filp)
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

int MCOMdev_close(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(100, "%s >\n", __func__);

	minor = MINOR(inode->i_rdev);

	dprintk(20, "close minor %d\n", minor);

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

static int MCOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	struct cn_micom_ioctl_data *mcom = (struct cn_micom_ioctl_data *)arg;
	int res = 0;

	dprintk(100, "%s > 0x%.8x\n", __func__, cmd);
	printk("%s > 0x%.8x\n", __func__, cmd);

	if (down_interruptible(&write_sem))
		return -ERESTARTSYS;

	switch (cmd)
	{
		case VFDDRIVERINIT:
			res = mcom_init_func();
			break;
		case VFDSTANDBY:
			res = mcomSetStandby(mcom->u.standby.time);
			break;
		case VFDSETTIME:
			if (mcom->u.time.localTime != 0)
				res = mcomSetTime(mcom->u.time.localTime);
			break;
		case VFDGETTIME:
			res = mcomGetTime();
			copy_to_user(arg, &ioctl_data, 5);
			break;
		case VFDGETWAKEUPMODE:
			res = mcomGetWakeUpMode();
			copy_to_user(arg, &ioctl_data, 1);
			break;
		case VFDDISPLAYCHARS:
		{
			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
			res = mcom_WriteString(data->data, data->length);
		}
		break;
		case VFDDISPLAYWRITEONOFF:
			/* ->alles abschalten ? VFD_Display_Write_On_Off */
			printk("VFDDISPLAYWRITEONOFF ->not yet implemented\n");
			break;
		default:
			printk("VFD/CNMicom: unknown IOCTL 0x%x\n", cmd);
			break;
	}
	up(&write_sem);
	dprintk(100, "%s <\n", __func__);
	return res;
}

struct file_operations vfd_fops =
{
	.owner = THIS_MODULE,
	.ioctl = MCOMdev_ioctl,
	.write = MCOMdev_write,
	.read  = MCOMdev_read,
	.open  = MCOMdev_open,
	.release  = MCOMdev_close
};
