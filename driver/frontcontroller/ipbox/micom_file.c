/*
 * micom_file.c
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
#include "micom_utf.h"

extern const char *driver_version;
extern void ack_sem_up(void);
extern int  ack_sem_down(void);
int micomWriteString(unsigned char *aBuf, int len);
extern void micom_putc(unsigned char data);


struct semaphore     write_sem;

int errorOccured = 0;

static char ioctl_data[20];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
	int   length;
	char  data[128];
};

int currentDisplayTime = 0; //display text not time

#ifdef CUBEREVO
/* animation timer for Play Symbol on 9000er */
static struct timer_list playTimer;
#endif

/* version date of fp */
int micom_day, micom_month, micom_year;

/* to the fp */
#define VFD_GETA0                0xA0
#define VFD_GETWAKEUPSTATUS      0xA1
#define VFD_GETA2                0xA2
#define VFD_GETRAM               0xA3
#define VFD_GETDATETIME          0xA4
#define VFD_GETMICOM             0xA5
#define VFD_GETWAKEUP            0xA6 /* wakeup time */
#define VFD_SETWAKEUPDATE        0xC0
#define VFD_SETWAKEUPTIME        0xC1
#define VFD_SETDATETIME          0xC2
#define VFD_SETBRIGHTNESS        0xC3
#define VFD_SETVFDTIME           0xC4
#define VFD_SETLED               0xC5
#define VFD_SETLEDSLOW           0xC6
#define VFD_SETLEDFAST           0xC7
#define VFD_SETSHUTDOWN          0xC8
#define VFD_SETRAM               0xC9
#define VFD_SETTIME              0xCA
#define VFD_SETCB                0xCB
#define VFD_SETFANON             0xCC
#define VFD_SETFANOFF            0xCD
#define VFD_SETRFMODULATORON     0xCE
#define VFD_SETRFMODULATOROFF    0xCF
#define VFD_SETCHAR              0xD0
#define VFD_SETDISPLAYTEXT       0xD1
#define VFD_SETD2                0xD2
#define VFD_SETD3                0xD3
#define VFD_SETD4                0xD4
#define VFD_SETCLEARTEXT         0xD5
#define VFD_SETD6                0xD6
#define VFD_SETMODETIME          0xD7
#define VFD_SETSEGMENTI          0xD8
#define VFD_SETSEGMENTII         0xD9
#define VFD_SETCLEARSEGMENTS     0xDA

typedef struct _special_char
{
	unsigned char  ch;
	unsigned short value;
} special_char_t;

static struct saved_data_s lastdata;

/* number segments */
int front_seg_num = 14;

unsigned short *num2seg;
unsigned short *Char2seg;
unsigned short *LowerChar2seg;
static special_char_t  *special2seg;
static int special2seg_size = 4;

unsigned short num2seg_12dotmatrix[] =
{
	0x20,       // 0
	0x21,       // 1
	0x22,       // 2
	0x23,       // 3
	0x24,       // 4
	0x25,       // 5
	0x26,       // 6
	0x27,       // 7
	0x28,       // 8
	0x29,       // 9
};

unsigned short Char2seg_12dotmatrix[] =
{
	0x31,       // A
	0x32,       // B
	0x33,       // C
	0x34,       // D
	0x35,       // E
	0x36,       // F
	0x37,       // G
	0x38,       // H
	0x39,       // I
	0x3a,       // J
	0x3b,       // K
	0x3c,       // L
	0x3d,       // M
	0x3e,       // N
	0x3f,       // O
	0x40,       // P
	0x41,       // Q
	0x42,       // R
	0x43,       // S
	0x44,       // T
	0x45,       // U
	0x46,       // V
	0x47,       // W
	0x48,       // X
	0x49,       // Y
	0x4a,       // Z
};

unsigned short num2seg_14dotmatrix[] =
{
	0x20,       // 0
	0x21,       // 1
	0x22,       // 2
	0x23,       // 3
	0x24,       // 4
	0x25,       // 5
	0x26,       // 6
	0x27,       // 7
	0x28,       // 8
	0x29,       // 9
};

unsigned short Char2seg_14dotmatrix[] =
{
	0x31,       // A
	0x32,       // B
	0x33,       // C
	0x34,       // D
	0x35,       // E
	0x36,       // F
	0x37,       // G
	0x38,       // H
	0x39,       // I
	0x3a,       // J
	0x3b,       // K
	0x3c,       // L
	0x3d,       // M
	0x3e,       // N
	0x3f,       // O
	0x40,       // P
	0x41,       // Q
	0x42,       // R
	0x43,       // S
	0x44,       // T
	0x45,       // U
	0x46,       // V
	0x47,       // W
	0x48,       // X
	0x49,       // Y
	0x4a,       // Z
};

unsigned short LowerChar2seg_14dotmatrix[] =
{
	0x51,       // a
	0x52,       // b
	0x53,       // c
	0x54,       // d
	0x55,       // e
	0x56,       // f
	0x57,       // g
	0x58,       // h
	0x59,       // i
	0x5a,       // j
	0x5b,       // k
	0x5c,       // l
	0x5d,       // m
	0x5e,       // n
	0x5f,       // o
	0x60,       // p
	0x61,       // q
	0x62,       // r
	0x63,       // s
	0x64,       // t
	0x65,       // u
	0x66,       // v
	0x67,       // w
	0x68,       // x
	0x69,       // y
	0x6a,       // z
};

unsigned short num2seg_13grid[] =
{
	0x3123,     // 0
	0x0408,     // 1
	0x30c3,     // 2
	0x21c3,     // 3
	0x01e2,     // 4
	0x21e1,     // 5
	0x31e1,     // 6
	0x0123,     // 7
	0x31e3,     // 8
	0x21e3,     // 9
};

unsigned short Char2seg_13grid[] =
{
	0x11e3,     // A
	0x25cb,     // B
	0x3021,     // C
	0x250b,     // D
	0x30e1,     // E
	0x10e1,     // F
	0x31a1,     // G
	0x11e2,     // H
	0x2409,     // I
	0x0809,     // J
	0x1264,     // K
	0x3020,     // L
	0x1136,     // M
	0x1332,     // N
	0x3123,     // O
	0x10e3,     // P
	0x3323,     // Q
	0x12e3,     // R
	0x21e1,     // S
	0x0409,     // T
	0x3122,     // U
	0x1824,     // V
	0x1b22,     // W
	0x0a14,     // X
	0x04e2,     // Y
	0x2805,     // Z
};

special_char_t special2seg_14dotmatrix[] =
{
	{':',     15},
	{' ',     16},
	{'!',     17},
	{'"',     18},
	{'#',     19},
	{'$',     20},
	{'%',     21},
	{'&',     22},
	{'´',     23},
	{'(',     24},
	{')',     25},
	{'*',     26},
	{'+',     27},
	{',',     28},
	{'-',     29},
	{'.',     30},
	{'/',     31},
	{'<',     44},
	{'=',     45},
	{'>',     46},
	{'?',     47},
	{'@',     48},
	{'[',     75},
	{']',     77},
	{'^',     78},
	{'_',     79},
	{'{',    107},
	{'}',    109},
	/* eurosymbol  {'',    116},*/
	{'§',    129},
	{'\'',   144},
};

special_char_t special2seg_13grid[] =
{
	{'-',   0x00c0},
	{'\'',  0x0004},
	{'.',   0x4000},
	{' ',   0x0000},
};

special_char_t special2seg_12dotmatrix[] =
{
	{'-',   0x1d},
	{'\'',  0x90},
	{'.',   0x1e},
	{' ',   0x10},
};

static unsigned short Char2seg_7seg[] =
{
	~0x01 & ~0x02 & ~0x04 & ~0x10 & ~0x20 & ~0x40,
	~0x04 & ~0x08 & ~0x10 & ~0x20 & ~0x40,
	~0x01 & ~0x08 & ~0x10 & ~0x20,
	~0x02 & ~0x04 & ~0x08 & ~0x10 & ~0x40,
	~0x01 & ~0x08 & ~0x10 & ~0x20 & ~0x40,
	~0x01 & ~0x10 & ~0x20 & ~0x40,
	~0x01 & ~0x04 & ~0x08 & ~0x10 & ~0x20,
	~0x04 & ~0x10 & ~0x20 & ~0x40,
	~0x04,
	~0x02 & ~0x04 & ~0x08 & ~0x10,
	~0x01 & ~0x04 & ~0x10 & ~0x20 & ~0x40,
	~0x08 & ~0x10 & ~0x20,
	~0x01 & ~0x02 & ~0x04 & ~0x10 & ~0x20,
	~0x04 & ~0x10 & ~0x40,
	~0x04 & ~0x08 & ~0x10 & ~0x40,
	~0x01 & ~0x02 & ~0x10 & ~0x20 & ~0x40,
	~0x01 & ~0x02 & ~0x08 & ~0x10 & ~0x20 & ~0x40,
	~0x10 & ~0x40,
	~0x04 & ~0x08 & ~0x20 & ~0x40,
	~0x08 & ~0x10 & ~0x20 & ~0x40,
	~0x04 & ~0x08 & ~0x10,
	~0x02 & ~0x04 & ~0x08 & ~0x10 & ~0x20,
	~0x02 & ~0x04 & ~0x08 & ~0x10 & ~0x20 & ~0x40,
	~0x02 & ~0x04 & ~0x10 & ~0x20 & ~0x40,
	~0x02 & ~0x04 & ~0x08 & ~0x20 & ~0x40,
	~0x01 & ~0x02 & ~0x08 & ~0x10,
};

static unsigned short num2seg_7seg[] =
{
	0xc0,		// 0
	0xf9,		// 1
	0xa4,		// 2
	0xb0,		// 3
	0x99,		// 4
	0x92,		// 5
	0x82,		// 6
	0xd8,		// 7
	0x80,		// 8
	0x98,		// 9
};

special_char_t  special2seg_7seg[] =
{
	{'-',	0xbf},
	{'_',	0xf7},
	{'.',	0x7f},
	{' ',	0xff},
};

enum
{
	ICON_MIN,             // 0x00
	ICON_STANDBY,
	ICON_SAT,
	ICON_REC,
	ICON_TIMESHIFT,
	ICON_TIMER,           // 0x05
	ICON_HD,
	ICON_USB,
	ICON_SCRAMBLED,
	ICON_DOLBY,
	ICON_MUTE,            // 0x0a
	ICON_TUNER1,
	ICON_TUNER2,
	ICON_MP3,
	ICON_REPEAT,
	ICON_Play,            // 0x0f
	ICON_TER,
	ICON_FILE,
	ICON_480i,
	ICON_480p,
	ICON_576i,            // 0x14
	ICON_576p,
	ICON_720p,
	ICON_1080i,
	ICON_1080p,
	ICON_Play_1,          // 0x19
	ICON_RADIO,
	ICON_TV,
	ICON_PAUSE,
	ICON_MAX
};



struct iconToInternal
{
	char *name;
	u16 icon;
	u8 codemsb;
	u8 codelsb;
	u8 segment;
} micomIcons[] =
{
	/*--------------------- SetIcon -------  msb   lsb   segment -----*/
	{ "ICON_STANDBY"  , ICON_STANDBY   , 0x03, 0x00, 1},
	{ "ICON_SAT"      , ICON_SAT       , 0x02, 0x00, 1},
	{ "ICON_REC"      , ICON_REC       , 0x00, 0x00, 0},
	{ "ICON_TIMESHIFT", ICON_TIMESHIFT , 0x01, 0x01, 0},
	{ "ICON_TIMESHIFT", ICON_TIMESHIFT , 0x01, 0x02, 0},
	{ "ICON_TIMER"    , ICON_TIMER     , 0x01, 0x03, 0},
	{ "ICON_HD"       , ICON_HD        , 0x01, 0x04, 0},
	{ "ICON_USB"      , ICON_USB       , 0x01, 0x05, 0},
	{ "ICON_SCRAMBLED", ICON_SCRAMBLED , 0x01, 0x06, 0}, //locked not scrambled
	{ "ICON_DOLBY"    , ICON_DOLBY     , 0x01, 0x07, 0},
	{ "ICON_MUTE"     , ICON_MUTE      , 0x01, 0x08, 0},
	{ "ICON_TUNER1"   , ICON_TUNER1    , 0x01, 0x09, 0},
	{ "ICON_TUNER2"   , ICON_TUNER2    , 0x01, 0x0a, 0},
	{ "ICON_MP3"      , ICON_MP3       , 0x01, 0x0b, 0},
	{ "ICON_REPEAT"   , ICON_REPEAT    , 0x01, 0x0c, 0},
	{ "ICON_Play"     , ICON_Play      , 0x00, 0x00, 1},
	{ "ICON_Play_1"   , ICON_Play_1    , 0x01, 0x04, 1},
	{ "ICON_TER"      , ICON_TER       , 0x02, 0x01, 1},
	{ "ICON_FILE"     , ICON_FILE      , 0x02, 0x02, 1},
	{ "ICON_480i"     , ICON_480i      , 0x06, 0x04, 1},
	{ "ICON_480i"     , ICON_480i      , 0x06, 0x03, 1},
	{ "ICON_480p"     , ICON_480p      , 0x06, 0x04, 1},
	{ "ICON_480p"     , ICON_480p      , 0x06, 0x02, 1},
	{ "ICON_576i"     , ICON_576i      , 0x06, 0x01, 1},
	{ "ICON_576i"     , ICON_576i      , 0x06, 0x00, 1},
	{ "ICON_576p"     , ICON_576p      , 0x06, 0x01, 1},
	{ "ICON_576p"     , ICON_576p      , 0x05, 0x04, 1},
	{ "ICON_720p"     , ICON_720p      , 0x05, 0x03, 1},
	{ "ICON_1080i"    , ICON_1080i     , 0x05, 0x02, 1},
	{ "ICON_1080p"    , ICON_1080p     , 0x05, 0x01, 1},
	{ "ICON_RADIO"    , ICON_RADIO     , 0x02, 0x04, 1},
	{ "ICON_TV"       , ICON_TV        , 0x02, 0x03, 1}
};

struct iconToInternal micom_14seg_Icons[] =
{
	/*--------------------- SetIcon -------  msb   lsb   segment -----*/
	{ "ICON_TIMER"     , ICON_TIMER     , 0x03, 0x00, 1},
	{ "ICON_REC"       , ICON_REC       , 0x02, 0x00, 1},
	{ "ICON_HD"        , ICON_HD        , 0x02, 0x04, 1},
	{ "ICON_Play"      , ICON_Play      , 0x02, 0x01, 1},
	{ "ICON_PAUSE"     , ICON_PAUSE     , 0x02, 0x02, 1},
	{ "ICON_DOLBY"     , ICON_DOLBY     , 0x02, 0x03, 1},
	{ "ICON_TIMESHIFT" , ICON_TIMESHIFT , 0x03, 0x01, 1},
};

#ifdef CUBEREVO
int micomWriteCommand(char *buffer, int len, int needAck);

#define cNumberSymbols      8
#define ANIMATION_INTERVAL  msecs_to_jiffies(500)

static int current_symbol = 0;
static int animationDie = 0;

struct iconToInternal playIcons[cNumberSymbols] =
{
	{ "ICON_Play"     , ICON_Play      , 0x00, 0x00, 1},
	{ "ICON_Play"     , ICON_Play      , 0x00, 0x02, 1},
	{ "ICON_Play"     , ICON_Play      , 0x01, 0x01, 1},
	{ "ICON_Play"     , ICON_Play      , 0x01, 0x03, 1},
	{ "ICON_Play"     , ICON_Play      , 0x01, 0x04, 1},
	{ "ICON_Play"     , ICON_Play      , 0x01, 0x02, 1},
	{ "ICON_Play"     , ICON_Play      , 0x00, 0x03, 1},
	{ "ICON_Play"     , ICON_Play      , 0x00, 0x01, 1},
};

static void animated_play(unsigned long data)
{
	char buffer[5];
	int i;

	current_symbol = (current_symbol + 1) % cNumberSymbols;

	for (i = 0; i < cNumberSymbols; i++)
	{
		memset(buffer, 0, 5);
		if ((i == current_symbol) && (animationDie == 0))
		{
			buffer[0] = VFD_SETSEGMENTI + playIcons[i].segment;
			buffer[1] = 0x01;
			buffer[2] = playIcons[i].codelsb;
			buffer[3] = playIcons[i].codemsb;
			micomWriteCommand(buffer, 5, 0);
		}
		else
		{
			buffer[0] = VFD_SETSEGMENTI + playIcons[i].segment;
			buffer[1] = 0x00;
			buffer[2] = playIcons[i].codelsb;
			buffer[3] = playIcons[i].codemsb;
			micomWriteCommand(buffer, 5, 0);
		}
	}

	if (animationDie == 0)
	{
		/* reschedule the timer */
		playTimer.expires = jiffies + ANIMATION_INTERVAL;
		add_timer(&playTimer);
	}
}
#endif

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

int micomWriteCommand(char *buffer, int len, int needAck)
{
	int i;

	dprintk(150, "%s >\n", __func__);

	for (i = 0; i < len; i++)
	{
#ifdef DIRECT_ASC
		serial_putc(buffer[i]);
#else
		micom_putc(buffer[i]);
#endif
	}

	if (needAck)
		if (ack_sem_down())
			return -ERESTARTSYS;

	dprintk(150, "%s < \n", __func__);

	return 0;
}

int micomSetLED(int on)
{
	char buffer[5];
	int res = 0;

	dprintk(100, "%s > %d\n", __func__, on);

	memset(buffer, 0, 5);

	/*
	xxxxxxx0 ->off
	xxxxxxx1 ->on
	LED_ON 0x1
	LED_OFF ~ 0x1

	xxxxxx0x ->slow blink off
	xxxxxx1x ->slow blink on

	SLOW_ON 0x2
	SLOW_OFF ~0x2

	xxxxx0xx ->fast blink off
	xxxxx1xx ->fast blink on

	FAST_ON 0x4
	FAST_OFF ~0x4
	*/
	if (on & 0x1)
	{
		buffer[0] = VFD_SETLED;
		buffer[1] = 0x01;

		res = micomWriteCommand(buffer, 5, 0);

		if (on & 0x2)
		{
			buffer[0] = VFD_SETLEDSLOW;
			buffer[1] = 0x81;           /* continues blink mode, last state will be on */

			res = micomWriteCommand(buffer, 5, 0);
		}

		if (on & 0x4)
		{
			buffer[0] = VFD_SETLEDFAST;
			buffer[1] = 0x81;           /* continues blink mode, last state will be on */

			res = micomWriteCommand(buffer, 5, 0);
		}
	}
	else
	{
		buffer[0] = VFD_SETLED;
		buffer[1] = 0x00;

		res = micomWriteCommand(buffer, 5, 0);

		buffer[0] = VFD_SETLEDFAST;
		buffer[1] = 0x00;

		res = micomWriteCommand(buffer, 5, 0);

		buffer[0] = VFD_SETLEDSLOW;
		buffer[1] = 0x00;

		res = micomWriteCommand(buffer, 5, 0);
	}

	dprintk(100, "%s (%d) <\n", __func__, res);

	return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetLED);

int micomSetFan(int on)
{
	char buffer[5];
	int res = 0;

	dprintk(100, "%s > %d\n", __func__, on);

	memset(buffer, 0, 5);

	/* on:
	 * 0 = off
	 * 1 = on
	 */
	if (on == 1)
	{
		buffer[0] = VFD_SETFANON;
	}
	else
	{
		buffer[0] = VFD_SETFANOFF;
	}

	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s (%d) <\n", __func__, res);

	return res;
}

int micomSetTimeMode(int twentyFour)
{
	char buffer[5];
	int res = 0;

	dprintk(100, "%s > %d\n", __func__, twentyFour);

	/* clear display */
	memset(buffer, 0, 5);
	buffer[0] = VFD_SETCLEARTEXT;
	res = micomWriteCommand(buffer, 5, 0);
	buffer[0] = VFD_SETDISPLAYTEXT;
	res = micomWriteCommand(buffer, 5, 0);

	memset(buffer, 0, 5);
	buffer[0] = VFD_SETMODETIME;
	buffer[1] = twentyFour & 0x1;

	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s (%d) <\n", __func__, res);

	return res;
}

int micomSetDisplayTime(int on)
{
	char buffer[5];
	int res = 0;

	dprintk(100, "%s > %d\n", __func__, on);

	/* clear display */
	memset(buffer, 0, 5);
	buffer[0] = VFD_SETCLEARTEXT;
	res = micomWriteCommand(buffer, 5, 0);
	buffer[0] = VFD_SETDISPLAYTEXT;
	res = micomWriteCommand(buffer, 5, 0);

	/* show time */
	memset(buffer, 0, 5);
	buffer[0] = VFD_SETVFDTIME;
	buffer[1] = on;

	res = micomWriteCommand(buffer, 5, 0);

	currentDisplayTime = on;

	dprintk(100, "%s (%d) <\n", __func__, res);

	return res;
}

int micomSetRF(int on)
{
	char buffer[5];
	int res = 0;

	dprintk(100, "%s > %d\n", __func__, on);

	memset(buffer, 0, 5);

	/* on:
	 * 0 = off
	 * 1 = on
	 */
	if (on == 1)
	{
		buffer[0] = VFD_SETRFMODULATORON;
	}
	else
	{
		buffer[0] = VFD_SETRFMODULATOROFF;
	}

	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s (%d) <\n", __func__, res);

	return res;
}

int micomSetBrightness(int level)
{
	char buffer[5];
	int res = 0;

	dprintk(100, "%s > %d\n", __func__, level);

	if (level < 0 || level > 7)
	{
		printk("VFD/Micom brightness out of range %d\n", level);
		return -EINVAL;
	}

	memset(buffer, 0, 5);

	buffer[0] = VFD_SETBRIGHTNESS;
	buffer[1] = level & 0x07;

	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBrightness);

int micomSetIcon(int which, int on)
{
	char buffer[5];
	int  vLoop, res = 0;

#if !defined(CUBEREVO_250HD)
	dprintk(100, "%s > %d, %d\n", __func__, which, on);

	if (which < 1 || which >= ICON_MAX)
	{
		printk("VFD/Nuvoton icon number out of range %d\n", which);
		return -EINVAL;
	}

	if (front_seg_num == 13) //no icons
		return res;

	memset(buffer, 0, 5);

	if (front_seg_num == 14) //only six icons
	{
		for (vLoop = 0; vLoop < ARRAY_SIZE(micom_14seg_Icons); vLoop++)
		{
			if ((which & 0xff) == micom_14seg_Icons[vLoop].icon)
			{
				buffer[0] = VFD_SETSEGMENTI + micom_14seg_Icons[vLoop].segment;
				buffer[1] = on;
				buffer[2] = micom_14seg_Icons[vLoop].codelsb;
				buffer[3] = micom_14seg_Icons[vLoop].codemsb;
				res = micomWriteCommand(buffer, 5, 0);

				break;
			}
		}
	}
	else
	{
#ifdef CUBEREVO
		if ((which == ICON_Play) && (front_seg_num == 12) && (on))
		{
			/* display circle */
			buffer[0] = VFD_SETSEGMENTII;
			buffer[1] = 0x01;
			buffer[2] = 0x04;
			buffer[3] = 0x00;
			micomWriteCommand(buffer, 5, 0);

			/* display play symbol */
			buffer[0] = VFD_SETSEGMENTII;
			buffer[1] = 0x01;
			buffer[2] = 0x00;
			buffer[3] = 0x01;
			micomWriteCommand(buffer, 5, 0);

			current_symbol = 0;
			animationDie = 0;
			playTimer.expires = jiffies + ANIMATION_INTERVAL;
			add_timer(&playTimer);
		}
		else if ((which == ICON_Play) && (front_seg_num == 12) && (!on))
		{
			/* undisplay circle */
			buffer[0] = VFD_SETSEGMENTII;
			buffer[1] = 0x00;
			buffer[2] = 0x04;
			buffer[3] = 0x00;
			micomWriteCommand(buffer, 5, 0);

			/* undisplay play symbol */
			buffer[0] = VFD_SETSEGMENTII;
			buffer[1] = 0x00;
			buffer[2] = 0x00;
			buffer[3] = 0x01;
			micomWriteCommand(buffer, 5, 0);

			animationDie = 1;
		}
		else
#endif
			for (vLoop = 0; vLoop < ARRAY_SIZE(micomIcons); vLoop++)
			{
				if ((which & 0xff) == micomIcons[vLoop].icon)
				{
					buffer[0] = VFD_SETSEGMENTI + micomIcons[vLoop].segment;
					buffer[1] = on;
					buffer[2] = micomIcons[vLoop].codelsb;
					buffer[3] = micomIcons[vLoop].codemsb;
					res = micomWriteCommand(buffer, 5, 0);

					/* dont break here because there may be multiple segments */
				}
			}
	}
	dprintk(100, "%s <\n", __func__);
#endif

	return res;
}

/* expected format: YYMMDDhhmm */
int micomSetStandby(char *time)
{
	char     buffer[5];
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

	res = micomWriteString("Bye bye ...", strlen("Bye bye ..."));

	/* set wakeup time */
	memset(buffer, 0, 5);

	if (time[0] == '\0')
	{
		dprintk(1, "clear wakeup time\n");
		buffer[0] = VFD_SETWAKEUPDATE;
		buffer[1] = 0x00; //year
		buffer[2] = 0x00; //month
		buffer[3] = 0x00; //day

		res = micomWriteCommand(buffer, 5, 0);

		memset(buffer, 0, 5);

		buffer[0] = VFD_SETWAKEUPTIME;
		buffer[1] = 0x00; //hour
		buffer[2] = 0x00; //minute
		buffer[3] = 0; /* on/off */
		res = micomWriteCommand(buffer, 5, 0);
	}
	else
	{
		dprintk(1, "set wakeup time\n");
		buffer[0] = VFD_SETWAKEUPDATE;
		buffer[1] = ((time[0] - '0') << 4) | (time[1] - '0'); //year
		buffer[2] = ((time[2] - '0') << 4) | (time[3] - '0'); //month
		buffer[3] = ((time[4] - '0') << 4) | (time[5] - '0'); //day

		res = micomWriteCommand(buffer, 5, 0);

		memset(buffer, 0, 5);

		buffer[0] = VFD_SETWAKEUPTIME;
		buffer[1] = ((time[6] - '0') << 4) | (time[7] - '0'); //hour
		buffer[2] = ((time[8] - '0') << 4) | (time[9] - '0'); //minute
		buffer[3] = 1; /* on/off */
		res = micomWriteCommand(buffer, 5, 0);
	}

	/* now power off */
	memset(buffer, 0, 5);

	buffer[0] = VFD_SETSHUTDOWN;
	/* 0 = shutdown
	 * 1 = reboot
	 * 2 = warmoff
	 * 3 = warmon
	 */
	buffer[1] = 0;

	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}

int micomReboot(void)
{
	char     buffer[5];
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

	res = micomWriteString("Bye bye ...", strlen("Bye bye ..."));

	memset(buffer, 0, 5);

	buffer[0] = VFD_SETSHUTDOWN;
	/* 0 = shutdown
	 * 1 = reboot
	 * 2 = warmoff
	 * 3 = warmon
	 */
	buffer[1] = 1;

	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}

/* expected format: YYMMDDhhmmss */
int micomSetTime(char *time)
{
	char       buffer[5];
	int        res = 0;

	dprintk(100, "%s >\n", __func__);

	/* set wakeup time */
	memset(buffer, 0, 5);

	dprintk(1, "date: %c %c %c %c %c %c\n", time[0], time[1], time[2], time[3], time[4], time[5]);
	dprintk(1, "time: %c %c %c %c %c %c\n", time[6], time[7], time[8], time[9], time[10], time[11]);

	buffer[0] = VFD_SETDATETIME;
	buffer[1] = ((time[0] - '0') << 4) | (time[1] - '0'); //year
	buffer[2] = ((time[2] - '0') << 4) | (time[3] - '0'); //month
	buffer[3] = ((time[4] - '0') << 4) | (time[5] - '0'); //day

	res = micomWriteCommand(buffer, 5, 0);

	memset(buffer, 0, 5);

	buffer[0] = VFD_SETTIME;
	buffer[1] = ((time[6] - '0') << 4) | (time[7] - '0'); //hour
	buffer[2] = ((time[8] - '0') << 4) | (time[9] - '0'); //minute
	buffer[3] = ((time[10] - '0') << 4) | (time[11] - '0'); //second

	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}

int micomClearIcons(void)
{
	char     buffer[5];
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(buffer, 0, 5);

	buffer[0] = VFD_SETCLEARSEGMENTS;
	res = micomWriteCommand(buffer, 5, 0);

	dprintk(100, "%s <\n", __func__);

	return res;
}

int micomGetTime(char *time)
{
	char     buffer[5];
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(buffer, 0, 5);

	buffer[0] = VFD_GETDATETIME;

	errorOccured = 0;
	res = micomWriteCommand(buffer, 5, 1);

	if (errorOccured == 1)
	{
		/* error */
		memset(ioctl_data, 0, 20);
		printk("error\n");
		res = -ETIMEDOUT;
	}
	else
	{
		dprintk(100, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n", ioctl_data[0], ioctl_data[1], ioctl_data[2], ioctl_data[2], ioctl_data[4], ioctl_data[5]);

		memcpy(time, ioctl_data, 12);

		dprintk(1, "time received\n");
	}

	dprintk(100, "%s <\n", __func__);

	return res;
}

int micomGetWakeupTime(char *time)
{
	char     buffer[5];
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(buffer, 0, 5);

	buffer[0] = VFD_GETWAKEUP;

	errorOccured = 0;
	res = micomWriteCommand(buffer, 5, 1);

	if (errorOccured == 1)
	{
		/* error */
		memset(ioctl_data, 0, 20);
		printk("error\n");
		res = -ETIMEDOUT;
	}
	else
	{
		dprintk(100, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n", ioctl_data[0], ioctl_data[1], ioctl_data[2], ioctl_data[2], ioctl_data[4], ioctl_data[5]);

		memcpy(time, ioctl_data, 12);

		dprintk(1, "time received\n");
	}

	dprintk(100, "%s <\n", __func__);

	return res;
}

/* get date (version) of micom controller */
int micomGetMicom(void)
{
	char     buffer[5];
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

#if defined(CUBEREVO_MINI) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_3000HD) || defined(CUBEREVO_2000HD) || defined(CUBEREVO_250HD) /* fixme: not sure if true for MINI2 & CUBEREVO250HD!!! */
	micom_year  = 2008;
	micom_month = 4;
#else
	memset(buffer, 0, 5);

	buffer[0] = VFD_GETMICOM;

	errorOccured = 0;
	res = micomWriteCommand(buffer, 5, 1);

	if (errorOccured == 1)
	{
		/* error */
		memset(ioctl_data, 0, 20);
		printk("error\n");
		res = -ETIMEDOUT;
	}
	else
	{
		char  convertDate[128];

		dprintk(100, "0x%02x 0x%02x 0x%02x\n", ioctl_data[0], ioctl_data[1], ioctl_data[2]);

		sprintf(convertDate, "%02x %02x %02x\n",
			ioctl_data[0], ioctl_data[1], ioctl_data[2]);

		sscanf(convertDate, "%d %d %d", &micom_year, &micom_month, &micom_day);

		micom_year  += 2000;
	}

#endif

	dprintk(1, "myTime= %02d.%02d.%04d\n", micom_day, micom_month, micom_year);

#ifdef CUBEREVO_250HD

	front_seg_num    = 4;
	num2seg          = num2seg_7seg;
	Char2seg         = Char2seg_7seg;
	LowerChar2seg    = NULL;
	special2seg      = special2seg_7seg;
	special2seg_size = ARRAY_SIZE(special2seg_7seg);

#else

#if defined(CUBEREVO)
	if ((micom_year == 2008) && (micom_month == 3))
#elif defined(CUBEREVO_MINI) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_3000HD) || defined(CUBEREVO_2000HD) /* fixme: not sure if true for MINI2 !!! */
	if ((micom_year == 2008) && (micom_month == 4))
#endif
	{
#if defined(CUBEREVO)
		front_seg_num = 12;
		num2seg = num2seg_12dotmatrix;
		Char2seg = Char2seg_12dotmatrix;
		LowerChar2seg = LowerChar2seg_14dotmatrix;
		special2seg = special2seg_14dotmatrix;
		special2seg_size = ARRAY_SIZE(special2seg_14dotmatrix);
#elif defined(CUBEREVO_MINI) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_3000HD) || defined(CUBEREVO_2000HD)/* fixme: not sure if true for MINI2 */
		front_seg_num = 14;
		num2seg = num2seg_14dotmatrix;
		Char2seg = Char2seg_14dotmatrix;
		LowerChar2seg = LowerChar2seg_14dotmatrix;
		special2seg = special2seg_14dotmatrix;
		special2seg_size = ARRAY_SIZE(special2seg_14dotmatrix);
#endif
	}
	else
	{
		/* 13 grid */
		front_seg_num = 13;

		num2seg = num2seg_13grid;
		Char2seg = Char2seg_13grid;
		LowerChar2seg = NULL;
		special2seg = special2seg_13grid;
		special2seg_size = ARRAY_SIZE(special2seg_13grid);
	}

#endif /* 250HD */

	dprintk(1, "%s front_seg_num %d \n", __func__, front_seg_num);

	dprintk(100, "%s < \n", __func__);

	return res;
}

int micomGetWakeUpMode(char *mode)
{
	char     buffer[5];
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(buffer, 0, 5);

	buffer[0] = VFD_GETWAKEUPSTATUS;

	errorOccured = 0;
	res = micomWriteCommand(buffer, 5, 1);

	if (errorOccured == 1)
	{
		/* error */
		memset(ioctl_data, 0, 20);
		printk("error\n");
		res = -ETIMEDOUT;
	}
	else
	{
		dprintk(100, "0x%02x\n", ioctl_data[0]);

		*mode = ioctl_data[0] & 0xff;
	}

	dprintk(100, "%s <\n", __func__);

	return res;
}

inline char toupper(const char c)
{
	if ((c >= 'a') && (c <= 'z'))
		return (c - 0x20);
	return c;
}

inline int trimTrainlingBlanks(char *txt, int len)
{
	int i;

	for (i = len - 1; i > 0 && txt[i] == ' '; i--, len--)
		txt[i] = '\0';
	return len;
}

int micomWriteString(unsigned char *aBuf, int len)
{
	char                buffer[5];
	unsigned char       bBuf[128];
	int                 res = 0, i, j;
	int                 pos = 0;
	unsigned char       space;

	dprintk(50, "%s >\n", __func__);

	if (currentDisplayTime == 1)
	{
		printk("display in time mode ->ignoring display text\n");
		return 0;
	}

	memset(bBuf, 0, 100);

	memset(buffer, 0, 5);
	buffer[0] = VFD_SETCLEARTEXT;
	res = micomWriteCommand(buffer, 5, 0);

	len = trimTrainlingBlanks(aBuf, len);
	pos = front_seg_num - len;

	if (pos < 0)
		pos = 0;

	pos /= 2;

	for (i = 0; i < pos; i++)
		bBuf[i] = ' ';

	for (j = 0 ; j < len && pos < front_seg_num; pos++, i++, j++)
		bBuf[i] = aBuf[j];

	for (; pos < front_seg_num; pos++, i++)
		bBuf[i] = ' ';

	len = front_seg_num;

	/* none printable chars will be replaced by space */
	for (j = 0; j < special2seg_size; j++)
	{
		if (special2seg[j].ch == ' ')
			break;
	}
	space = special2seg[j].value;

	/* set text char by char */
	for (i = 0; i < len; i++)
	{
		unsigned short data;
		unsigned char ch;

		memset(buffer, 0, 5);

		buffer[0] = VFD_SETCHAR;
		buffer[1] = i & 0xff; /* position */

		ch = bBuf[i];
		switch (ch)
		{
			case 'A' ... 'Z':
				ch -= 'A' - 'a';
				data = Char2seg[ch - 'a'];
				break;
			case 'a' ... 'z':
				if (LowerChar2seg == NULL)
					data = Char2seg[ch - 'a'];
				else
					data = LowerChar2seg[ch - 'a'];
				break;
			case '0' ... '9':
				data = num2seg[ch - '0'];
				break;
			default:
				for (j = 0; j < special2seg_size; j++)
				{
					if (special2seg[j].ch == ch)
						break;
				}

				if (j < special2seg_size)
				{
					data = special2seg[j].value;
					break;
				}
				else
				{
					printk("%s ignore unprintable char \'%c\'\n", __func__, ch);
					data = space;
				}
				break;
		}


		dprintk(150, "%s data 0x%x \n", __func__, data);

		buffer[2] = data & 0xff;
		buffer[3] = (data >> 8) & 0xff;
		res = micomWriteCommand(buffer, 5, 0);
	}

	memset(buffer, 0, 5);
	buffer[0] = VFD_SETDISPLAYTEXT;
	res = micomWriteCommand(buffer, 5, 0);

	dprintk(51, "%s <\n", __func__);

	return res;
}

int micom_init_func(void)
{
	int  res = 0;
	int  vLoop;

	dprintk(100, "%s >\n", __func__);

	sema_init(&write_sem, 1);

#if defined(CUBEREVO_MINI)
	printk("Cuberevo MINI VFD initializing (V%s)\n", driver_version);
#elif defined(CUBEREVO_MINI2)
	printk("Cuberevo MINI2 VFD initializing (V%s)\n", driver_version);
#elif defined(CUBEREVO_250HD)
	printk("Cuberevo 250 HD VFD initializing (V%s)\n", driver_version);
#elif defined(CUBEREVO)
	printk("Cuberevo 9000 HD VFD initializing (V%s)\n", driver_version);
#elif defined(CUBEREVO_2000HD)
	printk("Cuberevo 2000 HD VFD initializing (V%s)\n", driver_version);
#elif defined(CUBEREVO_3000HD)
	printk("Cuberevo 3000 HD VFD initializing (V%s)\n", driver_version);
#else
	printk("Cuberevo UNKNOWN VFD initializing (V%s)\n", driver_version);
#endif

	micomGetMicom();
	res |= micomSetFan(0);
	res |= micomSetLED(3 /* on and slow blink mode */);
	res |= micomSetBrightness(7);
	res |= micomSetTimeMode(1); //24h mode
	res |= micomSetDisplayTime(0);   //mode = display text
	res |= micomWriteString("T.Ducktales", strlen("T.Ducktales"));

	/* disable all icons at startup */
#if !defined(CUBEREVO_250HD)
	if (front_seg_num != 13)
		for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++)
			micomSetIcon(vLoop, 0);
#endif

#ifdef CUBEREVO
	if (front_seg_num == 12)
	{
		init_timer(&playTimer);
		playTimer.function = animated_play;
		playTimer.data = 0;
	}
#endif

	dprintk(100, "%s <\n", __func__);

	return 0;
}

//#define TEST_COMMANDS
//#define TEST_CHARSET
#if !defined(TEST_CHARSET) && !defined(TEST_COMMANDS)
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

	dprintk(70, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
		return res;
	else
		return len;
}
#elif defined(TEST_CHARSET)
static ssize_t MICOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int minor, vLoop, res = 0;
	char *myString = kmalloc(len + 1, GFP_KERNEL);
	int value;
	char                buffer[5];

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

	strncpy(myString, kernel_buf, len);
	myString[len] = '\0';

	printk("%s\n", myString);
	sscanf(myString, "%d", &value);
	printk("0x%x\n", value);

	memset(buffer, 0, 5);
	buffer[0] = VFD_SETCLEARTEXT;
	res = micomWriteCommand(buffer, 5, 0);

	memset(buffer, 0, 5);
	buffer[0] = VFD_SETCHAR;
	buffer[1] = 1;
	buffer[2] = value & 0xff;
	buffer[3] = (value >> 8) & 0xff;
	res = micomWriteCommand(buffer, 5, 0);

	kfree(kernel_buf);

	memset(buffer, 0, 5);
	buffer[0] = VFD_SETDISPLAYTEXT;
	res = micomWriteCommand(buffer, 5, 0);

	write_sem_up();

	dprintk(70, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
		return res;
	else
		return len;
}
#else
static ssize_t MICOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int minor, vLoop, res = 0;
	char *myString = kmalloc(len + 1, GFP_KERNEL);
	int value, arg1, arg2, arg3;
	char                buffer[5];
	int i;

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

	strncpy(myString, kernel_buf, len);
	myString[len] = '\0';

	printk("str: %s\n", myString);
	i = sscanf(myString, "%x %x %x %x", &value, &arg1, &arg2, &arg3);
	printk("val (%d): 0x%x 0x%x 0x%x 0x%x\n", i, value, arg1, arg2, arg3);

	if ((value != 0xd2) && (value != 0xd3) && (value != 0xd4) && (value != 0xd6) &&
			(value != 0xCB) && (value != 0xa0) && (value != 0xa2))
	{
		write_sem_up();
		return len;
	}

//#define bimmel
#ifdef bimmel
	for (i = 0; i <= 255; i++)
	{
		printk("0x%x ->0x%x\n", value, i);

		memset(buffer, 0, 5);
		buffer[0] = value;
		buffer[1] = 1;
		buffer[2] = i;
		buffer[3] = 0;
		res = micomWriteCommand(buffer, 5, 0);
		msleep(125);
	}
#else
	{
		memset(buffer, 0, 5);
		buffer[0] = value;
		buffer[1] = arg1;
		buffer[2] = arg2;
		buffer[3] = arg3;
		res = micomWriteCommand(buffer, 5, 0);
	}

#endif
	kfree(kernel_buf);

	write_sem_up();

	dprintk(70, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
		return res;
	else
		return len;
}
#endif

static ssize_t MICOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;

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

static int MICOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	static int mode = 0;
	struct micom_ioctl_data *micom = (struct micom_ioctl_data *)arg;
	int res = 0;

	dprintk(100, "%s > 0x%.8x\n", __func__, cmd);

	if (down_interruptible(&write_sem))
		return -ERESTARTSYS;

	switch (cmd)
	{
		case VFDSETMODE:
			mode = micom->u.mode.compat;
			break;
		case VFDSETLED:
			res = micomSetLED(micom->u.led.on);
			break;
		case VFDBRIGHTNESS:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				res = micomSetBrightness(data->start);
			}
			else
			{
				res = micomSetBrightness(micom->u.brightness.level);
			}
			mode = 0;
			break;
		case VFDDRIVERINIT:
			res = micom_init_func();
			mode = 0;
			break;
		case VFDICONDISPLAYONOFF:
			if (micom_year > 2007)
			{
				if (mode == 0)
				{
					struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
					int icon_nr = (data->data[0] & 0xf) + 1;
					int on = data->data[4];
					res = micomSetIcon(icon_nr, on);
				}
				else
				{
					res = micomSetIcon(micom->u.icon.icon_nr, micom->u.icon.on);
				}
			}
			else
				res = 0;
			mode = 0;
			break;
		case VFDSTANDBY:
			res = micomSetStandby(micom->u.standby.time);
			break;
		case VFDREBOOT:
			res = micomReboot();
			break;
		case VFDSETTIME:
			if (micom->u.time.time != 0)
				res = micomSetTime(micom->u.time.time);
			break;
		case VFDSETFAN:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				int on = data->start;
				res = micomSetFan(on);
			}
			else
			{
				res = micomSetFan(micom->u.fan.on);
			}
			break;
		case VFDSETRF:
			res = micomSetRF(micom->u.rf.on);
			break;
		case VFDSETTIMEMODE:
			res = micomSetTimeMode(micom->u.time_mode.twentyFour);
			break;
		case VFDSETDISPLAYTIME:
			res = micomSetDisplayTime(micom->u.display_time.on);
			break;
		case VFDGETTIME:
			res = micomGetTime(micom->u.get_time.time);
			break;
		case VFDGETWAKEUPTIME:
			res = micomGetWakeupTime(micom->u.wakeup_time.time);
			break;
		case VFDGETWAKEUPMODE:
			res = micomGetWakeUpMode(&micom->u.status.status);
			break;
		case VFDDISPLAYCHARS:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				res = micomWriteString(data->data, data->length);
			}
			else
			{
				//not suppoerted
			}
			mode = 0;
			break;
		case VFDDISPLAYWRITEONOFF:
		{
			char buffer[5];
			struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
			int on = data->start;

			if (on)
				break;

			/* clear text */
			memset(buffer, 0, 5);
			buffer[0] = VFD_SETCLEARTEXT;
			res = micomWriteCommand(buffer, 5, 0);

			memset(buffer, 0, 5);
			buffer[0] = VFD_SETDISPLAYTEXT;
			res = micomWriteCommand(buffer, 5, 0);

			/* and fall through to clear icons */
		}
		case VFDCLEARICONS:
			res = 0;
			if (micom_year > 2007)
				res = micomClearIcons();
			break;
		case VFDGETVERSION:
			printk("VFDGETVERSION: front_seg_num %d\n", front_seg_num);
			if (front_seg_num == 12)
				micom->u.version.version = 0;
			else if (front_seg_num == 13)
				micom->u.version.version = 1;
			else if (front_seg_num == 14)
				micom->u.version.version = 2;
			else if (front_seg_num == 4)
				micom->u.version.version = 3;

			printk("VFDGETVERSION: version %d\n", micom->u.version.version);
			break;
		case 0x5305:
			break;
		default:
			printk("VFD/Micom: unknown IOCTL 0x%x\n", cmd);
			mode = 0;
			break;
	}
	up(&write_sem);
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
