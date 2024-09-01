/**************************************************************************/
/* Name :   Topfield TF7700 Front Panel Driver Module                     */
/*          ./drivers/topfield/frontpanel.c                               */
/*                                                                        */
/* Author:  Gustav Gans                                                   */
/*                                                                        */
/* Descr.:  Controls the front panel clock, timers, keys and display      */
/*                                                                        */
/* Licence: This file is subject to the terms and conditions of the GNU   */
/* General Public License version 2.                                      */
/*                                                                        */
/* 2008-06-10 V1.0  First release                                         */
/* 2008-07-19 V2.0  Added the key buffer and the /dev/rc device           */
/*                  Added the TypematicDelay and TypematicRate            */
/*                  Added the KeyEmulationMode (0=TF7700, 1=UFS910)       */
/* 2008-07-28 V2.1  Adapted to Enigma2                                    */
/* 2008-08-21 V2.2  Added poll() operation                                */
/* 2008-12-31       introduced a thread for interrupt processing          */
/*                  added handling for the new VFD interface              */
/*                  added issuing a standby key to the application        */
/* 2009-01-05 V3.0  added suppression of multiple shutdown commands       */
/*                  replaced the module author                            */
/* 2009-01-06 V3.1  Added reboot control code                             */
/* 2009-02-08 V3.2  Added GMT offset handling                             */
/* 2009-03-19 V3.3  Added mapping of VFD codes 0-9 to CD segments         */
/* 2009-03-19 V3.4  Changed the standby button logic                      */
/*                  Added a 3 sec emergency shutdown                      */
/* 2009-03-19 V3.5  Changed mapping of VFD codes 0x20-0x29 to CD segments */
/*                  Fixed a break statement                               */
/* 2009-03-22 V3.6  Fixed a buffer overflow which garbled the VFD         */
/* 2009-03-28 V3.7  Added the --resend command                            */
/* 2009-03-30 V3.8  Fixed the issue with late acknowledge of shutdown     */
/*                  requests                                              */
/*                  Increased the forced shutdown delay to 5 seconds      */
/* 2009-04-10 V3.9  Added mapping of the keys |< and >| to VolumeDown     */
/*                  and VolumeUp                                          */
/* 2009-06-25 V4.0  Added support for UTF-8 strings                       */
/*                  Added the option to show all characters of the large  */
/*                    display in capital letters                          */
/* 2009-06-29 V4.1  Added the ability to scroll longer strings            */
/* 2009-07-01 V4.1a Trims trailing space to prevent the scrolling of text */
/*                    with less than 8 chars                              */
/* 2009-07-27 V4.2  Removed filtering of the power-off key.               */
/* 2009-08-08 V4.3  Added support for configuration in EEPROM             */
/* 2009-10-13 V4.4  Added typematic rate control for the power-off key    */
/* 2009-10-13 V4.5  Fixed error in function TranslateUTFString            */
/*                  In special casses the ending '\0' has been skipped    */
/* 2011-07-18 V4.6  Add quick hack for long key press                     */
/* 2011-07-23 V4.7  Add LKP emulation mode                                */
/**************************************************************************/

#define VERSION         "V4.7"
//#define DEBUG

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <stdarg.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/termios.h>
#include <linux/poll.h>
#include <linux/ctype.h>

#include "frontpanel.h"
#include "stb7109regs.h"
#include "VFDSegmentMap.h"
#include "tffp_config.h"

extern void create_proc_fp(void);
extern void remove_proc_fp(void);

#define LASTMINOR                 3
#define IOCTLMAGIC                0x3a

#define _FRONTPANELGETTIME        0x00
#define _FRONTPANELSETTIME        0x01
#define _FRONTPANELSYNCTIME       0x02
#define _FRONTPANELCLEARTIMER     0x03
#define _FRONTPANELSETTIMER       0x04
#define _FRONTPANELBRIGHTNESS     0x05
#define _FRONTPANELIRFILTER1      0x06
#define _FRONTPANELIRFILTER2      0x07
#define _FRONTPANELIRFILTER3      0x08
#define _FRONTPANELIRFILTER4      0x09
#define _FRONTPANELPOWEROFF       0x0a
#define _FRONTPANELBOOTREASON     0x0b
#define _FRONTPANELCLEAR          0x0c
#define _FRONTPANELTYPEMATICDELAY 0x0d
#define _FRONTPANELTYPEMATICRATE  0x0e
#define _FRONTPANELKEYEMULATION   0x0f
#define _FRONTPANELREBOOT         0x10
#define _FRONTPANELRESEND         0x13
#define _FRONTPANELALLCAPS        0x14
#define _FRONTPANELSCROLLMODE     0x15
#define _FRONTPANELICON           0x20

#define FPSHUTDOWN                0x20
#define FPPOWEROFF                0x21
#define FPSHUTDOWNACK             0x31
#define FPGETDISPLAYCONTROL       0x40
#define FPREQDATETIMENEW          0x30
#define FPDATETIMENEW             0x25
#define FPKEYPRESSFP              0x51
#define FPKEYPRESS                0x61
#define FPREQBOOTREASON           0x80
#define FPBOOTREASON              0x81
#define FPTIMERCLEAR              0x72
#define FPTIMERSET                0x84

#define VFDMAGIC                  0xc0425a00
#define VFDDISPLAYCHARS           0xc0425a00
#define VFDICONDISPLAYONOFF       0xc0425a0a

#define STASC1IRQ                 120
#define BUFFERSIZE                256     //must be 2 ^ n

#define KEYEMUTF7700              0
#define KEYEMUUFS910              1
#define KEYEMUTF7700LKP           2

typedef enum
{
	/* common icons */
	VFD_ICON_HD = 0x01,
	VFD_ICON_HDD,
	VFD_ICON_LOCK,
	VFD_ICON_BT,
	VFD_ICON_MP3,
	VFD_ICON_MUSIC,
	VFD_ICON_DD,
	VFD_ICON_MAIL,
	VFD_ICON_MUTE,
	VFD_ICON_PLAY,
	VFD_ICON_PAUSE,
	VFD_ICON_FF,
	VFD_ICON_FR,
	VFD_ICON_REC,
	VFD_ICON_CLOCK,

	/* additional TF7700 icons */
	VFD_ICON_CD1 = 0x20,
	VFD_ICON_CD2,
	VFD_ICON_CD3,
	VFD_ICON_CD4,
	VFD_ICON_CD5,
	VFD_ICON_CD6,
	VFD_ICON_CD7,
	VFD_ICON_CD8,
	VFD_ICON_CD9,
	VFD_ICON_CD10,
} VFD_ICON;

struct vfd_ioctl_data
{
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};

typedef struct
{
	struct file 	        *fp;
	struct semaphore      sem;
} tFrontPanelOpen;

static wait_queue_head_t	wq;

struct semaphore rx_int_sem;

static time_t gmtWakeupTime = 0;

static tFrontPanelOpen FrontPanelOpen [LASTMINOR + 1];     //remembers the file handle for all minor numbers

typedef enum
{
	VFD_7,
	VFD_14,
	VFD_17
} DISPLAYTYPE;

static byte                    DisplayBufferLED [ 4];
static byte                    DisplayBufferVFD [48], DisplayBufferVFDCurrent [48];
static byte                    RCVBuffer [BUFFERSIZE];
static int                     RCVBufferStart = 0, RCVBufferEnd = 0;
static char                    b[256];
static struct timer_list       timer;
static byte                    IconMask [] =  {0xff, 0xcf, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0x3f,
					       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					       0xff, 0xfe, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff,
					       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
					       0xff, 0xfc, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff
					      };
static byte                    IconBlinkMode[48][4];
static byte                    DefaultTypematicDelay = 3;
static byte                    DefaultTypematicRate = 1;
static byte                    KeyBuffer [256], KeyBufferStart = 0, KeyBufferEnd = 0;
static byte                    KeyEmulationMode = 1; // Kathrein mode by default
static struct termios          termAttributes;
static tTffpConfig             tffpConfig;

static void SendFPByte(byte Data)
{
	byte                  *ASC_3_TX_BUFF = (byte *)(ASC3BaseAddress + ASC_TX_BUFF);
	unsigned int          *ASC_3_INT_STA = (unsigned int *)(ASC3BaseAddress + ASC_INT_STA);
	dword                  Counter = 100000;

	while (((*ASC_3_INT_STA & ASC_INT_STA_THE) == 0) && --Counter)
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		// We are to fast, lets make a break
		udelay(0);
#endif
	}

	*ASC_3_TX_BUFF = Data;
}

static void SendFPString(byte *s)
{
	unsigned int          i;

#ifdef DEBUG
	if (s)
	{
		printk("FP: SendString(s=02 ");
		for (i = 0; i <= (s[0] & 0x0f); i++)
			printk("%2.2x ", s[i]);
		printk("03)\n");
	}
#endif

	if (s)
	{
		SendFPByte(2);
		SendFPByte(s[0]);
		for (i = 1; i <= (s[0] & 0x0f); i++)
			SendFPByte(s[i]);
		SendFPByte(3);
	}
}

static void SendFPData(byte Len, byte Data, ...)
{
	unsigned int          i;
	va_list               ap;
	byte                  DispData [20];

#ifdef DEBUG
	printk("FP: SendFPData(Len=%d,...)\n", Len);
#endif

	DispData [0] = Data;
	va_start(ap, Data);
	for (i = 1; i < Len; i++)
		DispData [i] = (byte) va_arg(ap, unsigned int);
	va_end(ap);
	SendFPString(DispData);
}

static inline byte GetBufferByte(int Offset)
{
	return RCVBuffer[(RCVBufferEnd + Offset) & (BUFFERSIZE - 1)];
}

static void VFDSendToDisplay(bool Force)
{
	byte                  DispBuffer [10];

#ifdef DEBUG
	//printk("FP: VFDSendToDisplay(Force=%s)\n", Force ? "true" : "false");
#endif

	DispBuffer[0] = 0x99;

	if (Force || memcmp(&DisplayBufferVFD[0x00], &DisplayBufferVFDCurrent[0x00], 8))
	{
		DispBuffer[1] = 0x00;
		memcpy(&DispBuffer[2], &DisplayBufferVFD[0x00], 8);
		SendFPString(DispBuffer);
	}

	if (Force || memcmp(&DisplayBufferVFD[0x08], &DisplayBufferVFDCurrent[0x08], 8))
	{
		DispBuffer[1] = 0x08;
		memcpy(&DispBuffer[2], &DisplayBufferVFD[0x08], 8);
		SendFPString(DispBuffer);
	}

	if (Force || memcmp(&DisplayBufferVFD[0x10], &DisplayBufferVFDCurrent[0x10], 8))
	{
		DispBuffer[1] = 0x10;
		memcpy(&DispBuffer[2], &DisplayBufferVFD[0x10], 8);
		SendFPString(DispBuffer);
	}

	if (Force || memcmp(&DisplayBufferVFD[0x18], &DisplayBufferVFDCurrent[0x18], 8))
	{
		DispBuffer[1] = 0x18;
		memcpy(&DispBuffer[2], &DisplayBufferVFD[0x18], 8);
		SendFPString(DispBuffer);
	}

	if (Force || memcmp(&DisplayBufferVFD[0x20], &DisplayBufferVFDCurrent[0x20], 8))
	{
		DispBuffer[1] = 0x20;
		memcpy(&DispBuffer[2], &DisplayBufferVFD[0x20], 8);
		SendFPString(DispBuffer);
	}

	if (Force || memcmp(&DisplayBufferVFD[0x28], &DisplayBufferVFDCurrent[0x28], 8))
	{
		DispBuffer[1] = 0x28;
		memcpy(&DispBuffer[2], &DisplayBufferVFD[0x28], 8);
		SendFPString(DispBuffer);
	}

	memcpy(DisplayBufferVFDCurrent, DisplayBufferVFD, sizeof(DisplayBufferVFD));
}

static void VFDClearBuffer(void)
{
#ifdef DEBUG
	printk("FP: VFDClearBuffer()\n");
#endif

	memset(DisplayBufferLED, 0, sizeof(DisplayBufferLED));
	memset(DisplayBufferVFD, 0, sizeof(DisplayBufferVFD));
	memset(DisplayBufferVFDCurrent, 0, sizeof(DisplayBufferVFDCurrent));
	SendFPData(2, 0x91, 0x00);
}

static void VFDInit(void)
{
#ifdef DEBUG
	printk("FP: VFDInit()\n");
#endif

	//Set bits 0 to 2 of PIO 5 for alternative functions (UART3)
	*(unsigned int *)(PIO5BaseAddress + PIO_CLR_PnC0) = 0x07;
	*(unsigned int *)(PIO5BaseAddress + PIO_CLR_PnC1) = 0x06;
	*(unsigned int *)(PIO5BaseAddress + PIO_SET_PnC1) = 0x01;
	*(unsigned int *)(PIO5BaseAddress + PIO_SET_PnC2) = 0x07;

	*(unsigned int *)(ASC3BaseAddress + ASC_INT_EN)   = 0x00000000;
	*(unsigned int *)(ASC3BaseAddress + ASC_CTRL)     = 0x00001589;
	*(unsigned int *)(ASC3BaseAddress + ASC_TIMEOUT)  = 0x00000010;
	*(unsigned int *)(ASC3BaseAddress + ASC_BAUDRATE) = 0x000000c9;
	*(unsigned int *)(ASC3BaseAddress + ASC_TX_RST)   = 0;
	*(unsigned int *)(ASC3BaseAddress + ASC_RX_RST)   = 0;
	VFDClearBuffer();
}

static unsigned int VFDTranslateSegments(byte Character, DISPLAYTYPE DisplayType)
{
	unsigned int          code = 0;

	switch (DisplayType)
	{
		case VFD_7:
			code = VFDSegmentMap7 [Character];
			break;
		case VFD_14:
			code = VFDSegmentMap14 [Character];
			break;
		case VFD_17:
			code = VFDSegmentMap17 [Character];
			break;
	}

	return code;
}

static void VFDSetDisplayDigit(byte Character, DISPLAYTYPE DisplayType, byte Digit)
{
	switch (DisplayType)
	{
		case VFD_7:
		{
			if (Digit < 4) DisplayBufferLED [Digit] = VFDTranslateSegments(Character, DisplayType);
			break;
		}

		case VFD_14:
		{
			unsigned int s = VFDTranslateSegments(Character, VFD_14);

			/*  7  6  5  4  3  2  1  0
			//01                      1d  1e  1c  1l
			//02  1m  1k  1g1  1g2  1b  1f  1j  1h
			//03  1i  1a  2d  2e  2c  2l  2m  2k
			//04  2g1  2g2  2b  2f  2j  2h  2i  2a
			//05    3d  3e  3c  3l  3m  3k  3g1
			//06  3g2  3b  3f  3j  3h  3i  3a
			//07      4d  4e  4c  4l  4m  4k
			//08  4g1  4g2  4b  4f  4j  4h  4i  4a      */

			switch (Digit)
			{
				/*Source: m l   k j i h   g2 g1 f e   d c b a*/
				case 0:
				{
					DisplayBufferVFD[1] = (DisplayBufferVFD[1] & 0xf0) |
							      ((s >> 12) & 0x01) |
							      ((s >>  1) & 0x02) |
							      ((s >>  2) & 0x04) |
							      (s        & 0x08);

					DisplayBufferVFD[2] = ((s >>  8) & 0x01) |
							      ((s >>  9) & 0x02) |
							      ((s >>  3) & 0x04) |
							      ((s <<  2) & 0x08) |
							      ((s >>  3) & 0x10) |
							      ((s >>  1) & 0x20) |
							      ((s >>  5) & 0x40) |
							      ((s >>  6) & 0x80);

					DisplayBufferVFD[3] = (DisplayBufferVFD[3] & 0x3f) |
							      ((s <<  6) & 0x40) |
							      ((s >>  2) & 0x80);

					break;
				}

				case 1:
				{
					DisplayBufferVFD[3] = (DisplayBufferVFD[3] & 0xc0) |
							      ((s >> 11) & 0x01) |
							      ((s >> 12) & 0x02) |
							      ((s >> 10) & 0x04) |
							      ((s <<  1) & 0x08) |
							      (s        & 0x10) |
							      ((s <<  2) & 0x20);

					DisplayBufferVFD[4] = (s        & 0x01) |
							      ((s >>  8) & 0x02) |
							      ((s >>  6) & 0x04) |
							      ((s >>  7) & 0x08) |
							      ((s >>  1) & 0x10) |
							      ((s <<  4) & 0x20) |
							      ((s >>  1) & 0x40) |
							      ((s <<  1) & 0x80);

					break;
				}

				case 2:
				{
					DisplayBufferVFD[5] = (DisplayBufferVFD[5] & 0x80) |
							      ((s >>  6) & 0x01) |
							      ((s >> 10) & 0x02) |
							      ((s >> 11) & 0x04) |
							      ((s >>  9) & 0x08) |
							      ((s <<  2) & 0x10) |
							      ((s <<  1) & 0x20) |
							      ((s <<  3) & 0x40);

					DisplayBufferVFD[6] = (DisplayBufferVFD[6] & 0x01) |
							      ((s <<  1) & 0x02) |
							      ((s >>  7) & 0x04) |
							      ((s >>  5) & 0x08) |
							      ((s >>  6) & 0x10) |
							      (s        & 0x20) |
							      ((s <<  5) & 0x40) |
							      (s        & 0x80);

					break;
				}

				case 3:
				{
					DisplayBufferVFD[7] = (DisplayBufferVFD[7] & 0xc0) |
							      ((s >> 11) & 0x01) |
							      ((s >> 12) & 0x02) |
							      ((s >> 10) & 0x04) |
							      ((s <<  1) & 0x08) |
							      (s        & 0x10) |
							      ((s <<  2) & 0x20);

					DisplayBufferVFD[8] = (s        & 0x01) |
							      ((s >>  8) & 0x02) |
							      ((s >>  6) & 0x04) |
							      ((s >>  7) & 0x08) |
							      ((s >>  1) & 0x10) |
							      ((s <<  4) & 0x20) |
							      ((s >>  1) & 0x40) |
							      ((s <<  1) & 0x80);

					break;
				}

				default:
					;


			}

			break;
		}

		case VFD_17:
		{

			unsigned int s = VFDTranslateSegments(Character, VFD_17);

			/*Source: m   l k j i   h g3 g2 g1   f e d2 d1   c b a2 a1*/
			switch (Digit)
			{
				case 0:
				{
					DisplayBufferVFD[33] = (DisplayBufferVFD[33] & 0xd5) |
							       ((s >>  5) & 0x02) |
							       ((s >>  2) & 0x08) |
							       ((s <<  1) & 0x20);

					DisplayBufferVFD[34] = (DisplayBufferVFD[34] & 0x55) |
							       ((s >> 13) & 0x02) |
							       ((s >> 13) & 0x08) |
							       ((s >> 10) & 0x20) |
							       ((s <<  4) & 0x80);

					DisplayBufferVFD[35] = (DisplayBufferVFD[35] & 0x55) |
							       ((s >>  1) & 0x02) |
							       ((s >>  7) & 0x08) |
							       ((s >>  4) & 0x20) |
							       ((s >>  1) & 0x80);

					DisplayBufferVFD[36] = (DisplayBufferVFD[36] & 0x55) |
							       ((s >> 11) & 0x02) |
							       ((s >>  8) & 0x08) |
							       ((s >>  8) & 0x20) |
							       (s        & 0x80);

					DisplayBufferVFD[37] = (DisplayBufferVFD[37] & 0x55) |
							       ((s <<  4) & 0x20) |
							       ((s <<  7) & 0x80);
					break;
				}

				case 1:
				{
					DisplayBufferVFD[17] = (DisplayBufferVFD[17] & 0xd5) |
							       ((s >>  5) & 0x02) |
							       ((s >>  2) & 0x08) |
							       ((s <<  1) & 0x20);

					DisplayBufferVFD[18] = (DisplayBufferVFD[18] & 0x55) |
							       ((s >> 13) & 0x02) |
							       ((s >> 13) & 0x08) |
							       ((s >> 10) & 0x20) |
							       ((s <<  4) & 0x80);

					DisplayBufferVFD[19] = (DisplayBufferVFD[19] & 0x55) |
							       ((s >>  1) & 0x02) |
							       ((s >>  7) & 0x08) |
							       ((s >>  4) & 0x20) |
							       ((s >>  1) & 0x80);

					DisplayBufferVFD[20] = (DisplayBufferVFD[20] & 0x55) |
							       ((s >> 11) & 0x02) |
							       ((s >>  8) & 0x08) |
							       ((s >>  8) & 0x20) |
							       (s        & 0x80);

					DisplayBufferVFD[21] = (DisplayBufferVFD[21] & 0x55) |
							       ((s <<  4) & 0x20) |
							       ((s <<  7) & 0x80);
					break;
				}

				case 2:
				{
					DisplayBufferVFD[17] = (DisplayBufferVFD[17] & 0xea) |
							       ((s >>  6) & 0x01) |
							       ((s >>  3) & 0x04) |
							       (s        & 0x10);

					DisplayBufferVFD[18] = (DisplayBufferVFD[18] & 0xaa) |
							       ((s >> 14) & 0x01) |
							       ((s >> 14) & 0x04) |
							       ((s >> 11) & 0x10) |
							       ((s <<  3) & 0x40);

					DisplayBufferVFD[19] = (DisplayBufferVFD[19] & 0xaa) |
							       ((s >>  2) & 0x01) |
							       ((s >>  8) & 0x04) |
							       ((s >>  5) & 0x10) |
							       ((s >>  2) & 0x40);

					DisplayBufferVFD[20] = (DisplayBufferVFD[20] & 0xaa) |
							       ((s >> 12) & 0x01) |
							       ((s >>  9) & 0x04) |
							       ((s >>  9) & 0x10) |
							       ((s >>  1) & 0x40);

					DisplayBufferVFD[21] = (DisplayBufferVFD[21] & 0xaf) |
							       ((s <<  3) & 0x10) |
							       ((s <<  6) & 0x40);
					break;
				}

				case 3:
				{
					DisplayBufferVFD[37] = (DisplayBufferVFD[37] & 0xf5) |
							       ((s >>  4) & 0x02) |
							       ((s >>  1) & 0x08);

					DisplayBufferVFD[38] = (DisplayBufferVFD[38] & 0x55) |
							       ((s >> 15) & 0x02) |
							       ((s >> 12) & 0x08) |
							       ((s <<  2) & 0x20) |
							       ((s <<  1) & 0x80);

					DisplayBufferVFD[39] = (DisplayBufferVFD[39] & 0x55) |
							       ((s >>  9) & 0x02) |
							       ((s >>  6) & 0x08) |
							       ((s >>  3) & 0x20) |
							       ((s >>  7) & 0x80);

					DisplayBufferVFD[40] = (DisplayBufferVFD[40] & 0x55) |
							       ((s >> 10) & 0x02) |
							       ((s >> 10) & 0x08) |
							       ((s >>  2) & 0x20) |
							       ((s <<  5) & 0x80);

					DisplayBufferVFD[41] = (DisplayBufferVFD[41] & 0x57) |
							       ((s <<  2) & 0x08) |
							       ((s <<  5) & 0x20) |
							       ((s >>  5) & 0x80);
					break;
				}

				case 4:
				{
					DisplayBufferVFD[37] = (DisplayBufferVFD[37] & 0xfa) |
							       ((s >>  5) & 0x01) |
							       ((s >>  2) & 0x04);

					DisplayBufferVFD[38] = (DisplayBufferVFD[38] & 0xaa) |
							       ((s >> 16) & 0x01) |
							       ((s >> 13) & 0x04) |
							       ((s <<  1) & 0x10) |
							       (s        & 0x40);

					DisplayBufferVFD[39] = (DisplayBufferVFD[39] & 0xaa) |
							       ((s >> 10) & 0x01) |
							       ((s >>  7) & 0x04) |
							       ((s >>  4) & 0x10) |
							       ((s >>  8) & 0x40);

					DisplayBufferVFD[40] = (DisplayBufferVFD[40] & 0xaa) |
							       ((s >> 11) & 0x01) |
							       ((s >> 11) & 0x04) |
							       ((s >>  3) & 0x10) |
							       ((s <<  4) & 0x40);

					DisplayBufferVFD[41] = (DisplayBufferVFD[41] & 0xab) |
							       ((s <<  1) & 0x04) |
							       ((s <<  4) & 0x10) |
							       ((s >>  6) & 0x40);

					break;
				}

				case 5:
				{
					DisplayBufferVFD[21] = (DisplayBufferVFD[21] & 0xf5) |
							       ((s >>  4) & 0x02) |
							       ((s >>  1) & 0x08);

					DisplayBufferVFD[22] = (DisplayBufferVFD[22] & 0x55) |
							       ((s >> 15) & 0x02) |
							       ((s >> 12) & 0x08) |
							       ((s <<  2) & 0x20) |
							       ((s <<  1) & 0x80);

					DisplayBufferVFD[23] = (DisplayBufferVFD[23] & 0x55) |
							       ((s >>  9) & 0x02) |
							       ((s >>  6) & 0x08) |
							       ((s >>  3) & 0x20) |
							       ((s >>  7) & 0x80);

					DisplayBufferVFD[24] = (DisplayBufferVFD[24] & 0x55) |
							       ((s >> 10) & 0x02) |
							       ((s >> 10) & 0x08) |
							       ((s >>  2) & 0x20) |
							       ((s <<  5) & 0x80);

					DisplayBufferVFD[25] = (DisplayBufferVFD[25] & 0x57) |
							       ((s <<  2) & 0x08) |
							       ((s <<  5) & 0x20) |
							       ((s >>  5) & 0x80);
					break;
				}

				case 6:
				{
					DisplayBufferVFD[21] = (DisplayBufferVFD[21] & 0xfa) |
							       ((s >>  5) & 0x01) |
							       ((s >>  2) & 0x04);

					DisplayBufferVFD[22] = (DisplayBufferVFD[22] & 0xaa) |
							       ((s >> 16) & 0x01) |
							       ((s >> 13) & 0x04) |
							       ((s <<  1) & 0x10) |
							       (s        & 0x40);

					DisplayBufferVFD[23] = (DisplayBufferVFD[23] & 0xaa) |
							       ((s >> 10) & 0x01) |
							       ((s >>  7) & 0x04) |
							       ((s >>  4) & 0x10) |
							       ((s >>  8) & 0x40);

					DisplayBufferVFD[24] = (DisplayBufferVFD[24] & 0xaa) |
							       ((s >> 11) & 0x01) |
							       ((s >> 11) & 0x04) |
							       ((s >>  3) & 0x10) |
							       ((s <<  4) & 0x40);

					DisplayBufferVFD[25] = (DisplayBufferVFD[25] & 0xab) |
							       ((s <<  1) & 0x04) |
							       ((s <<  4) & 0x10) |
							       ((s >>  6) & 0x40);
					break;
				}

				case 7:
				{
					DisplayBufferVFD[33] = (DisplayBufferVFD[33] & 0xea) |
							       ((s >>  6) & 0x01) |
							       ((s >>  3) & 0x04) |
							       (s        & 0x10);

					DisplayBufferVFD[34] = (DisplayBufferVFD[34] & 0xaa) |
							       ((s >> 14) & 0x01) |
							       ((s >> 14) & 0x04) |
							       ((s >> 11) & 0x10) |
							       ((s <<  3) & 0x40);

					DisplayBufferVFD[35] = (DisplayBufferVFD[35] & 0xaa) |
							       ((s >>  2) & 0x01) |
							       ((s >>  8) & 0x04) |
							       ((s >>  5) & 0x10) |
							       ((s >>  2) & 0x40);

					DisplayBufferVFD[36] = (DisplayBufferVFD[36] & 0xaa) |
							       ((s >> 12) & 0x01) |
							       ((s >>  9) & 0x04) |
							       ((s >>  9) & 0x10) |
							       ((s >>  1) & 0x40);

					DisplayBufferVFD[37] = (DisplayBufferVFD[37] & 0xaf) |
							       ((s <<  3) & 0x10) |
							       ((s <<  6) & 0x40);

					break;
				}
			}

			break;
		}
	}
}

static void VFDSetDisplaySmallString(char *s)
{
	unsigned int          i;

#ifdef DEBUG
	printk("FP: VFDSetDisplaySmallString(s=%s)\n", s);
#endif

	if (s)
	{
		i = strlen(s);

		if (i > 0) VFDSetDisplayDigit(s[0], VFD_14, 0);
		else VFDSetDisplayDigit(' ', VFD_14, 0);
		if (i > 1) VFDSetDisplayDigit(s[1], VFD_14, 1);
		else VFDSetDisplayDigit(' ', VFD_14, 1);
		if (i > 2) VFDSetDisplayDigit(s[2], VFD_14, 2);
		else VFDSetDisplayDigit(' ', VFD_14, 2);
		if (i > 3) VFDSetDisplayDigit(s[3], VFD_14, 3);
		else VFDSetDisplayDigit(' ', VFD_14, 3);
	}
	VFDSendToDisplay(false);
}

static void VFDSetDisplayLargeString(char *s)
{
	unsigned int          i;
	char                 *c;

#ifdef DEBUG
	printk("FP: VFDSetDisplayLargeString(s=%s)\n", s);
#endif

	if (s)
	{
		if (tffpConfig.allCaps)
		{
			c = s;
			while (*c)
			{
				*c = toupper(*c);
				c++;
			}
		}

		i = strlen(s);

		if (i > 0) VFDSetDisplayDigit(s[0], VFD_17, 0);
		else VFDSetDisplayDigit(' ', VFD_17, 0);
		if (i > 1) VFDSetDisplayDigit(s[1], VFD_17, 1);
		else VFDSetDisplayDigit(' ', VFD_17, 1);
		if (i > 2) VFDSetDisplayDigit(s[2], VFD_17, 2);
		else VFDSetDisplayDigit(' ', VFD_17, 2);
		if (i > 3) VFDSetDisplayDigit(s[3], VFD_17, 3);
		else VFDSetDisplayDigit(' ', VFD_17, 3);
		if (i > 4) VFDSetDisplayDigit(s[4], VFD_17, 4);
		else VFDSetDisplayDigit(' ', VFD_17, 4);
		if (i > 5) VFDSetDisplayDigit(s[5], VFD_17, 5);
		else VFDSetDisplayDigit(' ', VFD_17, 5);
		if (i > 6) VFDSetDisplayDigit(s[6], VFD_17, 6);
		else VFDSetDisplayDigit(' ', VFD_17, 6);
		if (i > 7) VFDSetDisplayDigit(s[7], VFD_17, 7);
		else VFDSetDisplayDigit(' ', VFD_17, 7);
	}
	VFDSendToDisplay(false);
}

static void TranslateUTFString(unsigned char *s)
{
	//This routine modifies a string buffer, so that multibyte UTF strings are converted into single byte...
	//Characters, which don't fit into 8 bit are replaced by '*'

	unsigned char           *Source;
	unsigned char           *Dest;

	if (s && s[0])
	{
		Source = &s[0];
		Dest = &s[0];

		while (*Source)
		{
			if (*Source > 0xbf)
			{
				// Hit a Unicode character.
				// Translatable characters can be found between C280 and C3BF.
				if (*Source > 0xc3)
				{
					//Invalid character
					*Dest = '*';
					if ((*Source & 0xe0) == 0xc0)
						Source++;
					else if ((*Source & 0xf0) == 0xe0)
						Source++;
					if (*Source) Source++;
					else if ((*Source & 0xf8) == 0xf0)
						Source++;
					if (*Source) Source++;
					if (*Source) Source++;
				}
				else
				{
					*Dest = ((Source[0] & 0x03) << 6) | (Source[1] & 0x3f);
					Source++;
				}
			}
			else
				*Dest = *Source;
			if (*Source)
			{
				Source++;
				Dest++;
			}
		}
		*Dest = '\0';
	}
}

/**********************************************************************************************/
/* Code for playing with the time on the FP                                                   */
/**********************************************************************************************/
char                              sdow[][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static frontpanel_ioctl_time      fptime;
int                               FPREQDATETIMENEWPending = 0;
static DECLARE_WAIT_QUEUE_HEAD(FPREQDATETIMENEW_queue);

frontpanel_ioctl_time   *VFDReqDateTime(void)
{
#ifdef DEBUG
	printk("FP: VFDReqDateTime()\n");
#endif

	FPREQDATETIMENEWPending = 1;
	SendFPData(1, FPREQDATETIMENEW);
	wait_event_interruptible_timeout(FPREQDATETIMENEW_queue, !FPREQDATETIMENEWPending, HZ);

	return &fptime;
}

static void InterpretDateTime(void)
{
	word                  w;

#ifdef DEBUG
	printk("FP: InterpretDateTime()\n");
#endif

	w = (GetBufferByte(2) << 8) | GetBufferByte(3);

	fptime.year  = (w >> 9) + 2000;
	fptime.month = (w >> 5) & 0x0f;
	fptime.day   = w & 0x1f;
	fptime.dow   = GetBufferByte(4) >> 5;
	sprintf(fptime.sdow, sdow[fptime.dow]);
	fptime.hour  = GetBufferByte(4) & 0x1f;
	fptime.min   = GetBufferByte(5);
	fptime.sec   = GetBufferByte(6);
	fptime.now = mktime(fptime.year, fptime.month, fptime.day, fptime.hour, fptime.min, fptime.sec);

	printk("FP: clock set to %d-%2.2d-%2.2d %s %2.2d:%2.2d:%2.2d\n", fptime.year, fptime.month, fptime.day, fptime.sdow, fptime.hour, fptime.min, fptime.sec);

	FPREQDATETIMENEWPending = 0;
	wake_up_interruptible(&FPREQDATETIMENEW_queue);
}

void VFDSetTime(frontpanel_ioctl_time *fptime)
{
	char                  s[6];
	word                  w;

#ifdef DEBUG
	printk("FP: VFDSetTime(*fptime=%p)\n", fptime);
#endif

	w = (((fptime->year - 2000) & 0x7f) << 9) | ((fptime->month & 0x0f) << 5) | (fptime->day & 0x1f);
	s[0] = FPDATETIMENEW;
	s[1] = w >> 8;
	s[2] = w & 0xff;
	s[3] = fptime->hour;
	s[4] = fptime->min;
	s[5] = fptime->sec;
	SendFPString(s);
}


/**********************************************************************************************/
/* Code for remote and frontpanel buttons                                                     */
/**********************************************************************************************/
unsigned char ufs910map[256] =
{
	0x58, 0x59, 0x5b, 0x5a, 0xff, 0xff, 0x39, 0xff, // 0x00 - 0x07
	0xff, 0xff, 0xff, 0xff, 0x0d, 0x6e, 0x6f, 0x70,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 0x10
	0x08, 0x09, 0x54, 0x4c, 0x55, 0x0f, 0xff, 0x5c,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x20
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x30
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x21, 0x38, 0x3c, // 0x40
	0x20, 0xff, 0x31, 0x37, 0xff, 0x6d, 0xff, 0xff,
	0x11, 0xff, 0x10, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x50
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x60
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x70
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static inline void AddKeyBuffer(byte Key)
{
	int tmp = (KeyBufferStart + 1) & 0xff;
#ifdef DEBUG
	printk("FP: AddKeyBuffer(Key=%x)\n", Key);
#endif

	/* check for buffer overflow, if it is the case then
	   discard the new character */
	if (tmp != KeyBufferEnd)
	{
		/* no overflow */
		KeyBuffer[KeyBufferStart] = Key;
		KeyBufferStart = tmp;
	}
}


static void AddKeyToBuffer(byte Key, bool FrontPanel)
{
	switch (KeyEmulationMode)
	{
		case KEYEMUTF7700:
		case KEYEMUTF7700LKP:
		{
			AddKeyBuffer(FrontPanel ? FPKEYPRESSFP : FPKEYPRESS);
			AddKeyBuffer(Key);
			wake_up_interruptible(&wq);
			break;
		}

		case KEYEMUUFS910:
		{
			if (ufs910map[Key] == 0xff)
				printk("FP: undefined UFS910 key (TF=0x%2.2x)\n", Key);
			else
			{
				AddKeyBuffer(ufs910map[Key]);
				wake_up_interruptible(&wq);
			}
			break;
		}
	}
}

static void InterpretKeyPresses(void)
{
	static byte           TypematicDelay = 0;
	static byte           TypematicRate = 0;
	byte                  Key;

	Key = GetBufferByte(2);
	if (KeyEmulationMode == KEYEMUTF7700LKP)
	{
		AddKeyToBuffer(Key, GetBufferByte(1) == FPKEYPRESSFP);
	}
	else
	{
		if ((Key & 0x80) == 0) //First burst
		{
			AddKeyToBuffer(Key, GetBufferByte(1) == FPKEYPRESSFP);
			TypematicDelay = DefaultTypematicDelay;
			TypematicRate  = DefaultTypematicRate;
		}
		else
		{
			if (TypematicDelay == 0)
			{
				if (TypematicRate == 0)
				{
					AddKeyToBuffer(Key & 0x7f, GetBufferByte(1) == FPKEYPRESSFP);
					TypematicRate = DefaultTypematicRate;
				}
				else TypematicRate--;
			}
			else TypematicDelay--;
		}
	}
}


/**********************************************************************************************/
/* Code for getting the boot reason from the FP                                               */
/*                                                                                            */
/* As the 'boot reason' doesn't change during the systems up time, it's requested only once   */
/*                                                                                            */
/**********************************************************************************************/
static frontpanel_ioctl_bootreason       fpbootreason;

static void VFDGetBootReason(void)
{
#ifdef DEBUG
	printk("FP: VFDGetBootReason()\n");
#endif

	SendFPData(1, FPREQBOOTREASON);
}

static void InterpretBootReason(void)
{
#ifdef DEBUG
	printk("FP: InterpretBootReason()\n");
#endif

	fpbootreason.reason = GetBufferByte(2);
}

int getBootReason()
{
	return fpbootreason.reason;
}

/**********************************************************************************************/
/* Code for turning off the power via the FP                                                   */
/**********************************************************************************************/
static void VFDShutdown(void)
{
#ifdef DEBUG
	printk("FP: VFDShutdown()\n");
#endif

	SendFPData(2, 0x21, 0x01);
}

static void VFDReboot(void)
{
#ifdef DEBUG
	printk("FP: VFDReboot()\n");
#endif

	SendFPData(2, 0x31, 0x06);
}



/**********************************************************************************************/
/* Code for settings the front panels VFD brightness                                          */
/**********************************************************************************************/
static void VFDBrightness(byte Brightness)
{
	byte                  BrightData[] = {0x00, 0x01, 0x02, 0x08, 0x10, 0x20};

#ifdef DEBUG
	printk("FP: VFDBrightness(Brightness=%d)\n", Brightness);
#endif

	if (Brightness < 6) SendFPData(3, 0xa2, 0x08, BrightData[Brightness]);
}


/**********************************************************************************************/
/* Code for working with timers                                                               */
/**********************************************************************************************/
static void VFDClearTimers(void)
{
#ifdef DEBUG
	printk("FP: VFDClearTimers()\n");
#endif

	SendFPData(3, FPTIMERCLEAR, 0xff, 0xff);
}

void VFDSetTimer(frontpanel_ioctl_time *fptime)
{
	char                  s[6];
	word                  w;

#ifdef DEBUG
	printk("FP: VFDSetTimer(pfptime=%p)\n", fptime);
#endif

	w = (((fptime->year - 2000) & 0x7f) << 9) | ((fptime->month & 0x0f) << 5) | (fptime->day & 0x1f);
	s[0] = FPTIMERSET;
	s[1] = w >> 8;
	s[2] = w & 0xff;
	s[3] = fptime->hour;
	s[4] = fptime->min;
	SendFPString(s);
}


/**********************************************************************************************/
/* Code for enabling and disabling IR remote modes                                            */
/**********************************************************************************************/
byte		        FPIRData [] = {0xa5, 0x00, 0x02, 0x34, 0x0a, 0x00,
				       0xa5, 0x01, 0x49, 0x00, 0x0a, 0x00,
				       0xa5, 0x02, 0x49, 0x99, 0x0a, 0x00,
				       0xa5, 0x03, 0x20, 0xdf, 0x0a, 0x00
			    };

static void VFDSetIRMode(byte Mode, byte OnOff)
{
#ifdef DEBUG
	printk("FP: VFDSetIRMode(Mode0%d, OnOff=%d)\n", Mode, OnOff);
#endif

	if (Mode < 4)
	{
		FPIRData [Mode * 6 + 1] = (FPIRData [Mode * 6 + 1] & 0x0f) | (OnOff << 4);
		FPIRData [Mode * 6 + 5] = (FPIRData [Mode * 6 + 1] + FPIRData [Mode * 6 + 2] + FPIRData [Mode * 6 + 3] + FPIRData [Mode * 6 + 4] - 1) & 0xff;
		SendFPString(&FPIRData [Mode * 6]);
	}
}


/**********************************************************************************************/
/* Code for showing the icons                                                                 */
/**********************************************************************************************/
static inline void SetIconBits(byte Reg, byte Bit, byte Mode)
{
	byte                  i, j;

	i = 1 << Bit;
	j = i ^ 0xff;
	IconBlinkMode[Reg][0] = (IconBlinkMode[Reg][0] & j) | ((Mode & 0x01) ? i : 0);
	IconBlinkMode[Reg][1] = (IconBlinkMode[Reg][1] & j) | ((Mode & 0x02) ? i : 0);
	IconBlinkMode[Reg][2] = (IconBlinkMode[Reg][2] & j) | ((Mode & 0x04) ? i : 0);
	IconBlinkMode[Reg][3] = (IconBlinkMode[Reg][3] & j) | ((Mode & 0x08) ? i : 0);
}

static void setDolbyIcon(int on)
{
	if (on)
		SetIconBits(43, 3, 0xf);
	else
		SetIconBits(43, 3, 0x0);
}

static void setHddIcon(int on)
{
	if (on)
	{
		SetIconBits(29, 4, 0xf);
		SetIconBits(28, 5, 0xf);
	}
	else
	{
		SetIconBits(29, 4, 0x0);
		SetIconBits(28, 5, 0x0);
	}
}

static void setMuteIcon(int on)
{
	if (on)
		SetIconBits(42, 0, 0xf);
	else
		SetIconBits(42, 0, 0x0);
}

static void setPlayIcon(int on)
{
	if (on)
		SetIconBits(42, 4, 0xf);
	else
		SetIconBits(42, 4, 0x0);
}

static void setPauseIcon(int on)
{
	if (on)
		SetIconBits(42, 1, 0xf);
	else
		SetIconBits(42, 1, 0x0);
}

static void setRecIcon(int on)
{
	if (on)
		SetIconBits(41, 1, 0xf);
	else
		SetIconBits(41, 1, 0x0);
}

static void setFwdIcon(int on)
{
	if (on)
		SetIconBits(42, 2, 0xf);
	else
		SetIconBits(42, 2, 0x0);
}

static void setRwdIcon(int on)
{
	if (on)
		SetIconBits(42, 6, 0xf);
	else
		SetIconBits(42, 6, 0x0);
}

static void setMusicIcon(int on)
{
	if (on)
		SetIconBits(27, 3, 0xf);
	else
		SetIconBits(27, 3, 0x0);
}

static void setMp3Icon(int on)
{
	if (on)
		SetIconBits(28, 7, 0xf);
	else
		SetIconBits(28, 7, 0x0);
}

static void setTimerIcon(int on)
{
	if (on)
		SetIconBits(7, 6, 0xf);
	else
		SetIconBits(7, 6, 0x0);
}

static void VFDSetIcons(dword Icons1, dword Icons2, byte BlinkMode)
{
#ifdef DEBUG
	printk("FP: VFDSetIcons(Icons1=0x%08x, Icons2=0x%08x, BlinkMode=%d)\n", (int)Icons1, (int)Icons2, BlinkMode);
#endif

	if (Icons1 & FPICON_AC3)          SetIconBits(27, 0, BlinkMode);
	if (Icons1 & FPICON_AM)           SetIconBits(7, 7, BlinkMode);
	if (Icons1 & FPICON_ATTN)         SetIconBits(43, 4, BlinkMode);
	if (Icons1 & FPICON_AUTOREWLEFT)  SetIconBits(43, 7, BlinkMode);
	if (Icons1 & FPICON_AUTOREWRIGHT) SetIconBits(43, 6, BlinkMode);
	if (Icons1 & FPICON_COLON)        SetIconBits(5, 7, BlinkMode);
	if (Icons1 & FPICON_DISH)         SetIconBits(27, 4, BlinkMode);
	if (Icons1 & FPICON_DOLBY)        SetIconBits(43, 3, BlinkMode);
	if (Icons1 & FPICON_DOLLAR)       SetIconBits(43, 5, BlinkMode);
	if (Icons1 & FPICON_FWD)          SetIconBits(42, 2, BlinkMode);
	if (Icons1 & FPICON_IRDOT)        SetIconBits(1, 4, BlinkMode);
	if (Icons1 & FPICON_MP3)          SetIconBits(28, 7, BlinkMode);
	if (Icons1 & FPICON_MUSIC)        SetIconBits(27, 3, BlinkMode);
	if (Icons1 & FPICON_MUTE)         SetIconBits(42, 0, BlinkMode);
	if (Icons1 & FPICON_NETWORK)      SetIconBits(43, 2, BlinkMode);
	if (Icons1 & FPICON_PAUSE)        SetIconBits(42, 1, BlinkMode);
	if (Icons1 & FPICON_PLAY)         SetIconBits(42, 4, BlinkMode);
	if (Icons1 & FPICON_PM)           SetIconBits(6, 0, BlinkMode);
	if (Icons1 & FPICON_POWER)        SetIconBits(1, 5, BlinkMode);
	if (Icons1 & FPICON_REC)          SetIconBits(41, 1, BlinkMode);
	if (Icons1 & FPICON_RWD)          SetIconBits(42, 6, BlinkMode);
	if (Icons1 & FPICON_TIMER)        SetIconBits(7, 6, BlinkMode);
	if (Icons1 & FPICON_TIMESHIFT)    SetIconBits(27, 1, BlinkMode);
	if (Icons1 & FPICON_TUNER1)       SetIconBits(41, 0, BlinkMode);
	if (Icons1 & FPICON_TUNER2)       SetIconBits(42, 7, BlinkMode);
	if (Icons1 & FPICON_TV)           SetIconBits(27, 2, BlinkMode);
	if (Icons1 & FPICON_xxx2)         SetIconBits(42, 3, BlinkMode);
	if (Icons1 & FPICON_xxx4)         SetIconBits(42, 5, BlinkMode);
	if (Icons2 & FPICON_CD1)          SetIconBits(26, 7, BlinkMode);
	if (Icons2 & FPICON_CD10)         SetIconBits(27, 6, BlinkMode);
	if (Icons2 & FPICON_CD11)         SetIconBits(27, 5, BlinkMode);
	if (Icons2 & FPICON_CD12)         SetIconBits(25, 0, BlinkMode);
	if (Icons2 & FPICON_CD2)          SetIconBits(26, 6, BlinkMode);
	if (Icons2 & FPICON_CD3)          SetIconBits(26, 5, BlinkMode);
	if (Icons2 & FPICON_CD4)          SetIconBits(26, 4, BlinkMode);
	if (Icons2 & FPICON_CD5)          SetIconBits(26, 3, BlinkMode);
	if (Icons2 & FPICON_CD6)          SetIconBits(26, 2, BlinkMode);
	if (Icons2 & FPICON_CD7)          SetIconBits(26, 1, BlinkMode);
	if (Icons2 & FPICON_CD8)          SetIconBits(26, 0, BlinkMode);
	if (Icons2 & FPICON_CD9)          SetIconBits(27, 7, BlinkMode);
	if (Icons2 & FPICON_CDCENTER)     SetIconBits(25, 1, BlinkMode);
	if (Icons2 & FPICON_HDD)          SetIconBits(29, 4, BlinkMode);
	if (Icons2 & FPICON_HDD1)         SetIconBits(29, 5, BlinkMode);
	if (Icons2 & FPICON_HDD2)         SetIconBits(29, 6, BlinkMode);
	if (Icons2 & FPICON_HDD3)         SetIconBits(29, 7, BlinkMode);
	if (Icons2 & FPICON_HDD4)         SetIconBits(28, 0, BlinkMode);
	if (Icons2 & FPICON_HDD5)         SetIconBits(28, 1, BlinkMode);
	if (Icons2 & FPICON_HDD6)         SetIconBits(28, 2, BlinkMode);
	if (Icons2 & FPICON_HDD7)         SetIconBits(28, 3, BlinkMode);
	if (Icons2 & FPICON_HDD8)         SetIconBits(28, 4, BlinkMode);
	if (Icons2 & FPICON_HDDFRAME)     SetIconBits(28, 5, BlinkMode);
	if (Icons2 & FPICON_HDDFULL)      SetIconBits(28, 6, BlinkMode);
}


/**********************************************************************************************/
/* Scrolling task                                                                             */
/**********************************************************************************************/
typedef enum
{
	SS_InitChars,
	SS_WaitPauseTimeout,
	SS_ScrollText,
	SS_WaitDelayTimeout,
	SS_ShortString
} eScrollState;

static frontpanel_ioctl_scrollmode        ScrollMode;
static int                                ScrollOnce = 1;
static eScrollState                       ScrollState = SS_InitChars;
static char                               Message[40], ScrollText[40];

static void ScrollTimer(void)
{
	static int                              CurrentDelay = 0;

	switch (ScrollState)
	{
		case SS_InitChars:
		{
			//Display the first 8 chars and set the delay counter
			//to the delay to the first scroll movement
			strncpy(ScrollText, Message, sizeof(ScrollText));
			ScrollText[sizeof(ScrollText) - 1] = '\0';
			VFDSetDisplayLargeString(ScrollText);
			CurrentDelay = ScrollMode.ScrollPause;

			if (strlen(Message) <= 8)
			{
				//No need to scroll
				ScrollState = SS_ShortString;
				break;
			}

			//Check if the scroll mode has changed in the meantime
			switch (ScrollMode.ScrollMode)
			{
				case 0:
					ScrollState = SS_ShortString;
					break;

				case 1:
					if (ScrollOnce)
					{
						ScrollState = SS_WaitPauseTimeout;
						ScrollOnce = 0;
					}
					else
						ScrollState = SS_ShortString;
					break;

				case 2:
					ScrollState = SS_WaitPauseTimeout;
					break;
			}
			break;
		}

		case SS_WaitPauseTimeout:
		{
			//Wait until the counter reaches zero, then start the scroll
			CurrentDelay--;
			if (CurrentDelay == 0) ScrollState = SS_ScrollText;
			break;
		}

		case SS_ScrollText:
		{
			char      *s, *d;

			if (ScrollText[0])
			{
				//Shift the text, display it and wait ScrollDelay
				d = &ScrollText[0];
				s = &ScrollText[1];
				do
				{
					*d = *s;
					s++;
					d++;
				}
				while (*d);
				VFDSetDisplayLargeString(ScrollText);

				CurrentDelay = ScrollMode.ScrollDelay;
				ScrollState = SS_WaitDelayTimeout;
			}
			else
			{
				//Text has completely fallen out on the left side of the VFD
				ScrollState = SS_InitChars;
			}
			break;
		}

		case SS_WaitDelayTimeout:
		{
			CurrentDelay--;
			if (CurrentDelay == 0) ScrollState = SS_ScrollText;
			break;
		}

		case SS_ShortString:
		{
			//Text fits into the 8 char display, no need to do anything
			break;
		}
	}
}

static void ShowText(char *text)
{
	int                   i;

	//Save the text and reset the scroll state
	memset(Message, 0, sizeof(Message));
	strncpy(Message, text, sizeof(Message) - 1);

	//Trim trailing spaces
	if (*Message)
	{
		i = strlen(Message) - 1;
		while ((i > 0) && Message[i] == ' ')
			i--;
		Message[i + 1] = '\0';
	}


	ScrollState = SS_InitChars;
	ScrollOnce = 1;
}


/**********************************************************************************************/
/* Other code                                                                                 */
/**********************************************************************************************/
#if 0
static void VFDGetControl(void)
{
#ifdef DEBUG
	printk("FP: VFDGetControl()\n");
#endif

	SendFPData(1, 0x40);
}
#endif

static int frontpanel_init_func(void)
{
#ifdef DEBUG
	printk("FP: frontpanel_init_func()\n");
#endif

	printk("FP: Topfield TF7700HDPVR front panel module %s initializing\n", VERSION);

	VFDInit();
	ShowText("LINUX");

	return 0;
}

static void FPCommandInterpreter(void)
{
	int                   Cmd, CmdLen, i;

#ifdef DEBUG
	printk("FP: FPCommandInterpreter()\n");
#endif

	Cmd    = GetBufferByte(1);
	CmdLen = Cmd & 0x0f;

	switch (Cmd)
	{
		case FPSHUTDOWN:
		{
			static byte           TypematicRate = 0;
#ifdef DEBUG
			printk("FP: FPSHUTDOWN\n");
#endif

			if (KeyEmulationMode == KEYEMUTF7700LKP)
			{
				AddKeyBuffer(FPKEYPRESSFP);
				AddKeyBuffer(0x0c);  //KEY_POWER_FAKE
				wake_up_interruptible(&wq);
			}
			else
			{
				if (TypematicRate < 1)
				{
					/* send standby key to the application */
					if (KeyEmulationMode == KEYEMUTF7700)
						AddKeyBuffer(FPKEYPRESSFP);

					AddKeyBuffer(0x0c);
					TypematicRate = DefaultTypematicRate;
					wake_up_interruptible(&wq);
				}
				else
					TypematicRate--;
			}

			break;
		}

		case FPDATETIMENEW:
		{
			InterpretDateTime();
			//update_persistent_clock(now);
			break;
		}

		case FPBOOTREASON:
		{
			InterpretBootReason();
			break;
		}

		case FPKEYPRESS:
		case FPKEYPRESSFP:
		{
			/* reset the bootreason to disable autoshutdown if a key was pressed */
			if (fpbootreason.reason)
			{
				fpbootreason.reason = 0;
				VFDSetIcons(FPICON_POWER, 0x0, 0x0); // Switch off power icon to indicate that no automatic shutdown for auto timers will be done
			}

			InterpretKeyPresses();
			break;
		}

		default:
		{
			b[0] = '\0';
			for (i = 1; i <= CmdLen + 1; i++)
				sprintf(&b[strlen(b)], "%2.2x ", GetBufferByte(i));
			//printk ("FP: unknown cmd %s\n", b);
		}
	}
}

int fpTask(void *dummy)
{
	int i, j, BuffSize, CmdLen;

	daemonize("frontpanel");

	allow_signal(SIGTERM);

	while (1)
	{
		int dataAvailable;

		if (down_interruptible(&rx_int_sem))
			break;

		dataAvailable = 1;

		while (dataAvailable)
		{
			//Check for a valid RCV buffer
			//Find the next 0x02
			i = RCVBufferEnd;
			while ((i != RCVBufferStart) && (RCVBuffer[i] != 2))
				i = (i + 1) & (BUFFERSIZE - 1);

			if (RCVBufferEnd != i)
			{
				//There is spurios data without the SOT (0x02). Ignore leading 0x00
				b[0] = '\0';
				j = RCVBufferEnd;
				while (j != i)
				{
					if (b[0] || RCVBuffer[j]) sprintf(&b[strlen(b)], "%2.2x ",  RCVBuffer[j]);
					j = (j + 1) & (BUFFERSIZE - 1);
				}
				if (b[0]) printk("FP: Invalid data %s\n", b);
				RCVBufferEnd = i;
			}

			//Reached end of buffer with locating a 0x02?
			if (RCVBufferEnd != RCVBufferStart)
			{
				//check if there are already enough bytes for a full command
				BuffSize = RCVBufferStart - RCVBufferEnd;
				if (BuffSize < 0) BuffSize = BuffSize + BUFFERSIZE;
				if (BuffSize > 2)
				{
					CmdLen = GetBufferByte(1) & 0x0f;
					if (BuffSize >= CmdLen + 3)
					{
						//Enough data to interpret command
						FPCommandInterpreter();
						RCVBufferEnd = (RCVBufferEnd + CmdLen + 3) & (BUFFERSIZE - 1);
					}
					else
					{
						dataAvailable = 0;
					}
				}
				else
				{
					dataAvailable = 0;
				}
			}
			else
			{
				dataAvailable = 0;
			}
		}
	}

	return 0;
}

/**********************************************************************************************/
/* Interrupt handler                                                                          */
/*                                                                                            */
/**********************************************************************************************/
static irqreturn_t FP_interrupt(int irq, void *dev_id)
{
	byte                 *ASC_3_RX_BUFF = (byte *)(ASC3BaseAddress + ASC_RX_BUFF);
	unsigned int         *ASC_3_INT_STA = (unsigned int *)(ASC3BaseAddress + ASC_INT_STA);
	int dataArrived = 0;
	static int msgStart = 0;

#ifdef DEBUG
	printk("FP: FP_interrupt(irq=%d, *dev_id=%p\n", irq, dev_id);
#endif

	// copy data into the ring buffer
	while (*ASC_3_INT_STA & ASC_INT_STA_RBF)
	{
		RCVBuffer [RCVBufferStart] = *ASC_3_RX_BUFF;
		if (RCVBuffer[RCVBufferStart] == 0x02)
			msgStart = 1;
		else
		{
			if (msgStart && (RCVBuffer[RCVBufferStart] == 0x20))
			{
				SendFPData(2, FPSHUTDOWNACK, 0x02);
				SendFPData(1, FPGETDISPLAYCONTROL);
			}
			msgStart = 0;
		}
		RCVBufferStart = (RCVBufferStart + 1) % BUFFERSIZE;

		dataArrived = 1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		// We are to fast, lets make a break
		udelay(0);
#endif

		if (RCVBufferStart == RCVBufferEnd)
			printk("FP: RCV buffer overflow!!!\n");
	}

	// signal the data arrival
	if (dataArrived)
		up(&rx_int_sem);

	return IRQ_HANDLED;
}

static void FP_Timer(void)
{
	static byte           DelayCounter = 0;
	int                   i;

	for (i = 0; i < 48; i++)
		DisplayBufferVFD[i] = (DisplayBufferVFD[i] & IconMask[i]) | IconBlinkMode[i][DelayCounter];

	VFDSendToDisplay(false);

	DelayCounter = (DelayCounter + 1) & 0x03;
}

//This is a 10ms timer. It is responsible to call ScrollTimer() every time
// and FP_Timer() every 250ms
static void HighRes_FP_Timer(dword arg)
{
	static int            Counter = 0;

	ScrollTimer();

	if (Counter > 24)
	{
		FP_Timer();
		Counter = 0;
	}

	else
		Counter++;

	/* fill the data for our timer function */
	init_timer(&timer);
	timer.data = 0;
	timer.function = HighRes_FP_Timer;
	timer.expires = jiffies + (HZ / 100);
	add_timer(&timer);
}


/**********************************************************************************************/
/* fops                                                                                       */
/*                                                                                            */
/**********************************************************************************************/
static ssize_t FrontPaneldev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char                 *kernel_buf = kmalloc(len, GFP_KERNEL);
	int                   i;

#ifdef DEBUG
	printk("FP: FrontPaneldev_write(*filp=%p, *buff=%p, len=%d, off=%d", filp, buff, (int)len, (int)*off);
#endif

	for (i = 0; i <= LASTMINOR; i++)
	{
		if (FrontPanelOpen[i].fp == filp)
		{

#ifdef DEBUG
			printk(", minor=%d)\n", i);
#endif

			if (kernel_buf == NULL) return -1;
			copy_from_user(kernel_buf, buff, len);
			kernel_buf[len - 1] = '\0';

			switch (i)
			{
				case FRONTPANEL_MINOR_FPC:        //IOCTL only
				case FRONTPANEL_MINOR_RC:         //read-only
					len = -1;
					break;

				case FRONTPANEL_MINOR_FPLARGE:
					TranslateUTFString(kernel_buf);

					//ShowText() replaces VFDSetDisplayLargeString() to incorporate scrolling
					ShowText(kernel_buf);
					//VFDSetDisplayLargeString (kernel_buf);
					break;

				case FRONTPANEL_MINOR_FPSMALL:
					VFDSetDisplaySmallString(kernel_buf);
					break;
			}
			kfree(kernel_buf);
			return len;
		}
	}
	kfree(kernel_buf);
	return 0;
}

static unsigned int FrontPaneldev_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &wq, wait);
	if (KeyBufferStart != KeyBufferEnd)
	{
		mask = POLLIN | POLLRDNORM;
	}

	return mask;
}

static ssize_t FrontPaneldev_read(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char  kernel_buff [16];
	int   i, j;

#ifdef DEBUG
	printk("FP: FrontPaneldev_read(*filp=%p, *buff=%p, len=%d, off=%d", filp, buff, (int)len, (int)*off);
#endif

	for (i = 0; i <= LASTMINOR; i++)
	{
		if (FrontPanelOpen[i].fp == filp)
		{
#ifdef DEBUG
			printk(", minor=%d)\n", i);
#endif

			switch (i)
			{
				case FRONTPANEL_MINOR_FPC:        //IOCTL only
				case FRONTPANEL_MINOR_FPLARGE:    //write-only
				case FRONTPANEL_MINOR_FPSMALL:    //write-only
					return -EUSERS;

				case FRONTPANEL_MINOR_RC:
				{
					byte  BytesPerKey = 2;

					while (KeyBufferStart == KeyBufferEnd)
					{
						if (wait_event_interruptible(wq, (KeyBufferStart != KeyBufferEnd)))
							return -ERESTARTSYS;
					}

					switch (KeyEmulationMode)
					{
						case KEYEMUTF7700:
						case KEYEMUTF7700LKP:
							BytesPerKey = 2;
							for (j = 0; j < BytesPerKey; j++)
							{
								kernel_buff[j] = KeyBuffer[KeyBufferEnd];
								KeyBufferEnd = (KeyBufferEnd + 1) & 0xff;
							}
							break;

						case KEYEMUUFS910:
							BytesPerKey = 2;
							sprintf(kernel_buff, "%02x", KeyBuffer[KeyBufferEnd]);
							KeyBufferEnd = (KeyBufferEnd + 1) & 0xff;
							break;
					}

					if (down_interruptible(&FrontPanelOpen[i].sem))
					{
#ifdef DEBUG
						printk("FP: FrontPaneldev_read = -ERESTARTSYS\n");
#endif
						return -ERESTARTSYS;
					}
					copy_to_user(buff, kernel_buff, BytesPerKey);
					up(&FrontPanelOpen[i].sem);

#ifdef DEBUG
					printk("FP: FrontPaneldev_read = %d\n", BytesPerKey);
#endif

					return BytesPerKey;
				}
			}

#ifdef DEBUG
			printk("FP: FrontPaneldev_read = 0\n");
#endif

			return 0;
		}
	}

#ifdef DEBUG
	printk("FP: FrontPaneldev_read = -EUSERS\n");
#endif

	return -EUSERS;
}

static int FrontPaneldev_open(struct inode *Inode, struct file *filp)
{
	int                   minor;

	minor = MINOR(Inode->i_rdev);

#ifdef DEBUG
	printk("FP: FrontPaneldev_open(minor=%d, *filp=%p, *oldfp=%p\n", minor, filp, FrontPanelOpen[minor].fp);
#endif

	if (FrontPanelOpen[minor].fp != NULL)
	{
#ifdef DEBUG
		printk("FP: FrontPaneldev_open = -EUSERS\n");
#endif
		return -EUSERS;
	}
	FrontPanelOpen[minor].fp = filp;

	return 0;
}

static int FrontPaneldev_close(struct inode *Inode, struct file *filp)
{
	int                   minor;

	minor = MINOR(Inode->i_rdev);

#ifdef DEBUG
	printk("FP: FrontPaneldev_close(minor=%d, *filp=%p, *oldfp=%p\n", minor, filp, FrontPanelOpen[minor].fp);
#endif

	if (FrontPanelOpen[minor].fp == NULL) return -EUSERS;
	FrontPanelOpen[minor].fp = NULL;
	return 0;
}

#define LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year) (LEAPYEAR(year) ? 366 : 365)
static const int _ytab[2][12] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static frontpanel_ioctl_time *gmtime(register const time_t time)
{
	static frontpanel_ioctl_time fptime;
	register unsigned long dayclock, dayno;
	int year = 1970;

	dayclock = (unsigned long)time % 86400;
	dayno = (unsigned long)time / 86400;

	fptime.sec = dayclock % 60;
	fptime.min = (dayclock % 3600) / 60;
	fptime.hour = dayclock / 3600;
	fptime.dow = (dayno + 4) % 7;       /* day 0 was a thursday */
	while (dayno >= YEARSIZE(year))
	{
		dayno -= YEARSIZE(year);
		year++;
	}
	fptime.year = year;
	fptime.month = 0;
	while (dayno >= _ytab[LEAPYEAR(year)][fptime.month])
	{
		dayno -= _ytab[LEAPYEAR(year)][fptime.month];
		fptime.month++;
	}
	fptime.day = dayno + 1;
	fptime.month++;

	return &fptime;
}

void vfdSetGmtWakeupTime(time_t time)
{
	frontpanel_ioctl_time *pTime;

	/* store the wakeup time */
	gmtWakeupTime = time;

	/* apply GMT offset */
	time += tffpConfig.gmtOffset;

	pTime = gmtime(time);

	VFDSetTimer(pTime);
	printk("\nWakeup time: %d:%02d:%02d %d.%d.%d\n", pTime->hour, pTime->min,
	       pTime->sec, pTime->day, pTime->month, pTime->year);
}

void vfdSetGmtTime(time_t time)
{
	frontpanel_ioctl_time *pTime;

	/* apply GMT offset */
	time += tffpConfig.gmtOffset;

	pTime = gmtime(time);

	VFDSetTime(pTime);
}

time_t vfdGetGmtTime()
{
	frontpanel_ioctl_time *pTime = VFDReqDateTime();
	time_t time;

	/* convert to seconds since 1970 */
	time = mktime(pTime->year, pTime->month, pTime->day,
		      pTime->hour, pTime->min, pTime->sec);

	/* apply the GMT offset */
	time -= tffpConfig.gmtOffset;

	return time;
}

static int FrontPaneldev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, dword arg)
{
	int                   minor;
	int                   ret = 0;

	minor = MINOR(Inode->i_rdev);

#ifdef DEBUG
	printk("FP: FrontPaneldev_ioctl(minor=%d, *File=%p, cmd=0x%x, arg=0x%x\n", minor, File, cmd, (int)arg);
#endif

	//Quick hack to ignore any tcsetattr

	if (cmd == TCGETS)
	{
		copy_to_user(arg, &termAttributes, sizeof(struct termios));
		return 0;
	}

	if (cmd == 0x5404)
	{
		copy_from_user(&termAttributes, arg, sizeof(struct termios));
		return 0;
	}

	if (FrontPanelOpen[minor].fp == NULL || minor != FRONTPANEL_MINOR_FPC) return -EUSERS;

	if ((cmd & 0xffffff00) == VFDMAGIC)
	{
		struct vfd_ioctl_data vfdData;

		copy_from_user(&vfdData, arg, sizeof(vfdData));

		switch (cmd)
		{
			case VFDDISPLAYCHARS:
				if (vfdData.length >= (sizeof(vfdData.data) - 1))
					vfdData.length = sizeof(vfdData.data) - 1;
				vfdData.data[vfdData.length] = 0;
				TranslateUTFString(vfdData.data);

				//ShowText() replaces VFDSetDisplayLargeString() to incorporate scrolling
				ShowText(vfdData.data);
				//VFDSetDisplayLargeString(vfdData.data);
				break;

			case VFDICONDISPLAYONOFF:
				//printk("FP: VFDICONDISPLAYONOFF %x:%x\n", vfdData.data[0], vfdData.data[4]);
				switch (vfdData.data[0])
				{
					/* VFD driver of enigma2 issues codes during the start
					   phase, map them to the CD segments to indicate progress */
					case VFD_ICON_CD1:
						SetIconBits(26, 7, vfdData.data[4] ? 0xf : 0);
						SetIconBits(26, 6, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD2:
						SetIconBits(26, 5, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD3:
						SetIconBits(26, 4, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD4:
						SetIconBits(26, 3, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD5:
						SetIconBits(26, 2, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD6:
						SetIconBits(26, 1, vfdData.data[4] ? 0xf : 0);
						SetIconBits(26, 0, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD7:
						SetIconBits(27, 7, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD8:
						SetIconBits(27, 6, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD9:
						SetIconBits(27, 5, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_CD10:
						SetIconBits(25, 0, vfdData.data[4] ? 0xf : 0);
						break;
					case VFD_ICON_HDD:
						setHddIcon(vfdData.data[4]);
						break;
					case VFD_ICON_MP3:
						setMp3Icon(vfdData.data[4]);
						break;
					case VFD_ICON_MUSIC:
						setMusicIcon(vfdData.data[4]);
						break;
					case VFD_ICON_DD:
						setDolbyIcon(vfdData.data[4]);
						break;
					case VFD_ICON_MUTE:
						setMuteIcon(vfdData.data[4]);
						break;
					case VFD_ICON_PLAY:
						setPlayIcon(vfdData.data[4]);
						break;
					case VFD_ICON_PAUSE:
						setPauseIcon(vfdData.data[4]);
						break;
					case VFD_ICON_FF:
						setFwdIcon(vfdData.data[4]);
						break;
					case VFD_ICON_FR:
						setRwdIcon(vfdData.data[4]);
						break;
					case VFD_ICON_REC:
						setRecIcon(vfdData.data[4]);
						break;
					case VFD_ICON_CLOCK:
						setTimerIcon(vfdData.data[4]);
						break;
					//case VFD_ICON_HD:
					//case VFD_ICON_LOCK:
					//case VFD_ICON_MAIL:
					//case VFD_ICON_BT:
					default:
						return 0;
				}
				break;
			default:
				//printk("FP: VFD cmd %x\n", cmd);
				break;
		}

		return 0;
	}
	else if (((cmd >> 8) & 0xff) != IOCTLMAGIC)
	{
		printk("FP: invalid IOCTL magic 0x%x\n", cmd);
		return -ENOTTY;
	}


	switch (_IOC_NR(cmd))
	{
		case _FRONTPANELGETTIME:
		{
			VFDReqDateTime();
			copy_to_user(arg, &fptime, sizeof(frontpanel_ioctl_time));
			break;
		}

		case _FRONTPANELSETTIME:
		{
			copy_from_user(&fptime, arg, sizeof(frontpanel_ioctl_time));
			VFDSetTime(&fptime);
			VFDReqDateTime();
			break;
		}

		case _IOC_NR(FRONTPANELSYNCFPTIME):
		{
			/* set the FP time to the current system time */
			struct timeval tv;
			do_gettimeofday(&tv);
			vfdSetGmtTime(tv.tv_sec);
			break;
		}

		case _IOC_NR(FRONTPANELSETGMTOFFSET):
		{
			struct timeval tv;
			do_gettimeofday(&tv);
			/* update the GMT offset if necessary, accept the GMT offset
			   only if the system time is later than 01/01/2004 */
			if ((tffpConfig.gmtOffset != arg) && (tv.tv_sec > 1072224000))
			{
				time_t newTime;

				if (tffpConfig.gmtOffset == -1)
				{
					/* no valid GMT offset, set the new value and update from
					   system time */
					newTime = tv.tv_sec;
				}
				else
				{
					/* GMT offset valid, adjust the FP time */
					VFDReqDateTime();
					newTime = fptime.now - tffpConfig.gmtOffset;
				}

				tffpConfig.gmtOffset = arg;

				/* adjust the FP time */
				vfdSetGmtTime(newTime);

				writeTffpConfig(&tffpConfig);

				printk("New GMT offset is %d\n", tffpConfig.gmtOffset);

				/* update the wakeup time if set */
				if (gmtWakeupTime)
					vfdSetGmtWakeupTime(gmtWakeupTime);
			}

			break;
		}

		case _IOC_NR(FRONTPANELSYNCSYSTIME):
		{
			VFDReqDateTime();
			/* set the system time to the curernt FP time */
			{
				struct timespec ts;
				ts.tv_sec = fptime.now - tffpConfig.gmtOffset;
				ts.tv_nsec = 0;
				do_settimeofday(&ts);
			}

			break;
		}

		case _FRONTPANELCLEARTIMER:
		{
			VFDClearTimers();
			break;
		}

		case _FRONTPANELSETTIMER:
		{
			copy_from_user(&fptime, arg, sizeof(frontpanel_ioctl_time));
			VFDSetTimer(&fptime);
			break;
		}

		case _FRONTPANELIRFILTER1:
		{
			frontpanel_ioctl_irfilter      fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_irfilter));
				if (tffpConfig.irFilter1 != fpdata.onoff)
				{
					tffpConfig.irFilter1 = fpdata.onoff;
					writeTffpConfig(&tffpConfig);
				}
				VFDSetIRMode(0, fpdata.onoff);
			}
			break;
		}

		case _FRONTPANELIRFILTER2:
		{
			frontpanel_ioctl_irfilter      fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_irfilter));
				if (tffpConfig.irFilter2 != fpdata.onoff)
				{
					tffpConfig.irFilter2 = fpdata.onoff;
					writeTffpConfig(&tffpConfig);
				}
				VFDSetIRMode(1, fpdata.onoff);
			}
			break;
		}

		case _FRONTPANELIRFILTER3:
		{
			frontpanel_ioctl_irfilter      fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_irfilter));
				if (tffpConfig.irFilter3 != fpdata.onoff)
				{
					tffpConfig.irFilter3 = fpdata.onoff;
					writeTffpConfig(&tffpConfig);
				}
				VFDSetIRMode(2, fpdata.onoff);
			}
			break;
		}

		case _FRONTPANELIRFILTER4:
		{
			frontpanel_ioctl_irfilter      fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_irfilter));
				if (tffpConfig.irFilter4 != fpdata.onoff)
				{
					tffpConfig.irFilter4 = fpdata.onoff;
					writeTffpConfig(&tffpConfig);
				}
				VFDSetIRMode(3, fpdata.onoff);
			}
			break;
		}

		case _FRONTPANELBRIGHTNESS:
		{
			frontpanel_ioctl_brightness    *fpdata;

			if (arg)
			{
				fpdata = (frontpanel_ioctl_brightness *)arg;
				if (tffpConfig.brightness != fpdata->bright)
				{
					tffpConfig.brightness = fpdata->bright;
					writeTffpConfig(&tffpConfig);
				}
				VFDBrightness(fpdata->bright);
			}
			break;
		}

		case _FRONTPANELBOOTREASON:
		{
			//The reason value has already been cached upon module startup
			copy_to_user(arg, &fpbootreason, sizeof(frontpanel_ioctl_bootreason));
			break;
		}

		case _FRONTPANELPOWEROFF:
		{
			VFDShutdown();
			break;
		}

		case _FRONTPANELCLEAR:
		{
			VFDClearBuffer();
			memset(IconBlinkMode, 0, sizeof(IconBlinkMode));
			break;
		}

		case _FRONTPANELTYPEMATICDELAY:
		{
			frontpanel_ioctl_typematicdelay   fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_typematicdelay));
				if (tffpConfig.typematicDelay != fpdata.TypematicDelay)
				{
					tffpConfig.typematicDelay = fpdata.TypematicDelay;
					writeTffpConfig(&tffpConfig);
				}
				DefaultTypematicDelay = fpdata.TypematicDelay;
			}
			break;
		}

		case _FRONTPANELTYPEMATICRATE:
		{
			frontpanel_ioctl_typematicrate    fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_typematicrate));
				if (tffpConfig.typematicRate != fpdata.TypematicRate)
				{
					tffpConfig.typematicRate = fpdata.TypematicRate;
					writeTffpConfig(&tffpConfig);
				}
				DefaultTypematicRate = fpdata.TypematicRate;
			}
			break;
		}

		case _FRONTPANELKEYEMULATION:
		{
			frontpanel_ioctl_keyemulation     fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_keyemulation));
				KeyEmulationMode = fpdata.KeyEmulation;
				KeyBufferStart =  KeyBufferEnd = 0;
			}
			break;
		}

		case _FRONTPANELREBOOT:
		{
			VFDReboot();
			break;
		}

		case _FRONTPANELRESEND:
		{
			VFDSendToDisplay(true);
			break;
		}

		case _FRONTPANELALLCAPS:
		{
			frontpanel_ioctl_allcaps     fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_allcaps));
				if (tffpConfig.allCaps != fpdata.AllCaps)
				{
					tffpConfig.allCaps = fpdata.AllCaps;
					writeTffpConfig(&tffpConfig);
				}
			}
			break;
		}

		case _FRONTPANELSCROLLMODE:
		{
			if (arg)
			{
				copy_from_user(&ScrollMode, arg, sizeof(frontpanel_ioctl_scrollmode));
				if (ScrollMode.ScrollPause == 0) ScrollMode.ScrollPause = 1;
				if (ScrollMode.ScrollDelay == 0) ScrollMode.ScrollDelay = 1;

				if ((ScrollMode.ScrollMode != tffpConfig.scrollMode) ||
						(ScrollMode.ScrollDelay != tffpConfig.scrollDelay) ||
						(ScrollMode.ScrollPause != tffpConfig.scrollPause))
				{
					tffpConfig.scrollMode = ScrollMode.ScrollMode;
					tffpConfig.scrollDelay = ScrollMode.ScrollDelay;
					tffpConfig.scrollPause = ScrollMode.ScrollPause;
					writeTffpConfig(&tffpConfig);
				}

				ScrollOnce = 1;
				ScrollState = SS_InitChars;
			}
			break;
		}

		case _FRONTPANELICON:
		{
			frontpanel_ioctl_icons      fpdata;

			if (arg)
			{
				copy_from_user(&fpdata, arg, sizeof(frontpanel_ioctl_icons));
				VFDSetIcons(fpdata.Icons1, fpdata.Icons2, fpdata.BlinkMode);
			}
			break;
		}

		default:
			printk("FP: unknown IOCTL 0x%x(0x%08x)\n", cmd, (int)arg);
			ret = -ENOTTY;
	}

	return ret;
}


/**********************************************************************************************/
/* Driver Interface                                                                           */
/*                                                                                            */
/**********************************************************************************************/
static struct file_operations frontpanel_fops =
{
	.owner      = THIS_MODULE,
	.open       = FrontPaneldev_open,
	.ioctl      = FrontPaneldev_ioctl,
	.read       = (void *)FrontPaneldev_read,
	.poll       = (void *)FrontPaneldev_poll,
	.write      = FrontPaneldev_write,
	.release    = FrontPaneldev_close
};

static int __init frontpanel_init_module(void)
{
	unsigned int         *ASC_3_INT_EN = (unsigned int *)(ASC3BaseAddress + ASC_INT_EN);
	unsigned int         *ASC_3_CTRL   = (unsigned int *)(ASC3BaseAddress + ASC_CTRL);
	int                   i;

	//Disable all ASC 3 interrupts
	*ASC_3_INT_EN = *ASC_3_INT_EN & ~0x000001ff;

	if (register_chrdev(FRONTPANEL_MAJOR, "FrontPanel", &frontpanel_fops))
		printk("FP: unable to get major %d\n", FRONTPANEL_MAJOR);
	else
		frontpanel_init_func();

	//Initialize the variables
	memset(FrontPanelOpen, 0, (LASTMINOR + 1) * sizeof(tFrontPanelOpen));
	memset(IconBlinkMode, 0, sizeof(IconBlinkMode));
	ScrollMode.ScrollMode  =   1; //scroll once
	ScrollMode.ScrollPause = 100; //wait 1s until start of scroll
	ScrollMode.ScrollDelay =  10; //scroll every 100ms
	Message[0] = '\0';

	// read and apply the settings
	readTffpConfig(&tffpConfig);
	DefaultTypematicRate = tffpConfig.typematicRate;
	DefaultTypematicDelay = tffpConfig.typematicDelay;
	ScrollMode.ScrollMode  = tffpConfig.scrollMode;
	ScrollMode.ScrollPause = tffpConfig.scrollPause;
	ScrollMode.ScrollDelay = tffpConfig.scrollDelay;
	/* transmit applicable settings to the VFD */
	VFDSetIRMode(0, tffpConfig.irFilter1);
	VFDSetIRMode(1, tffpConfig.irFilter2);
	VFDSetIRMode(2, tffpConfig.irFilter3);
	VFDSetIRMode(3, tffpConfig.irFilter4);
	VFDBrightness(tffpConfig.brightness);

	for (i = 0; i <= LASTMINOR; i++)
		sema_init(&FrontPanelOpen[i].sem, 1);

	sema_init(&rx_int_sem, 0);

	kernel_thread(fpTask, NULL, 0);

	init_waitqueue_head(&wq);

	//Enable the FIFO
	*ASC_3_CTRL = *ASC_3_CTRL | ASC_CTRL_FIFO_EN;

	printk("Stored GMT offset is %d\n", tffpConfig.gmtOffset);

	//Link the ASC 3 interupt to our handler and enable the receive buffer IE flag
	i = request_irq(STASC1IRQ, (void *)FP_interrupt, 0, "FP_serial", NULL);
	if (!i)
	{
		struct timespec ts = {0, 0};
		*ASC_3_INT_EN = *ASC_3_INT_EN | 0x00000001;

		if (tffpConfig.gmtOffset != -1)
		{
			/* GMT offset valid */
			/* retrieve the time from the FP */
			VFDReqDateTime();
			ts.tv_sec = fptime.now - tffpConfig.gmtOffset;
		}
		/* else: set 1970/01/01 to enforce getting the time
		   from transponder */

		/* set the system time */
		do_settimeofday(&ts);

		VFDGetBootReason();
		VFDClearTimers();

		/* get the VFD control */
		SendFPData(1, 0x40);
		/* uknown code sequence */
		SendFPData(2, 0x51, 0x5a);
		SendFPData(5, 0x54, 0xff, 0xff, 0x00, 0x00);
		/* set power status (?) */
		SendFPData(2, 0x41, 0x88);
		/* set time format to 24h */
		SendFPData(2, 0x11, 0x81);
	}
	else printk("FP: Can't get irq\n");

	memset(&termAttributes, 0, sizeof(struct termios));

	//Start the timer
	HighRes_FP_Timer(0);

	create_proc_fp();

	return 0;
}

static void __exit frontpanel_cleanup_module(void)
{
	unsigned int         *ASC_3_INT_EN = (unsigned int *)(ASC3BaseAddress + ASC_INT_EN);

	*ASC_3_INT_EN = *ASC_3_INT_EN & ~0x000001ff;
	free_irq(STASC1IRQ, NULL);
	del_timer(&timer);
	VFDClearBuffer();
	unregister_chrdev(FRONTPANEL_MAJOR, "FrontPanel");
	printk("FP: Topfield TF7700HDPVR front panel module unloading\n");

	remove_proc_fp();

}


module_init(frontpanel_init_module);
module_exit(frontpanel_cleanup_module);

MODULE_DESCRIPTION("FrontPanel module for Topfield TF7700HDPVR " VERSION);
MODULE_AUTHOR("Gustav Gans");
MODULE_LICENSE("GPL");
