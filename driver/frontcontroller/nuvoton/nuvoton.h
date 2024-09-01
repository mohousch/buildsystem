/*
 * nuvoton.h
 *
 * Frontpanel driver for Fortis HDBOX and Octagon 1008
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
 *
 ****************************************************************************************
 *
 * Changes
 *
 * Date     By              Description
 * --------------------------------------------------------------------------------------
 * 20130929 Audioniek
 *
 ****************************************************************************************/

#ifndef _123_nuvoton
#define _123_nuvoton

extern short paramDebug;
#define TAGDEBUG "[nuvoton] "

#ifndef dprintk
#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug >= level)) printk(TAGDEBUG x); \
	} while (0)
#endif

extern int nuvoton_init_func(void);
extern void copyData(unsigned char *data, int len);
extern void getRCData(unsigned char *data, int *len);
void dumpValues(void);

extern int errorOccured;
extern struct file_operations vfd_fops;

/****************************************************************************************/

typedef struct
{
	struct file     *fp;
	int              read;
	struct semaphore sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC   1
#define LASTMINOR             2

extern tFrontPanelOpen FrontPanelOpen[LASTMINOR];

#define VFD_MAJOR             147
#define SOP                   0x02
#define EOP                   0x03

/* ioctl numbers ->hacky */
#define VFDDISPLAYCHARS       0xc0425a00
#define VFDBRIGHTNESS         0xc0425a03
//#define VFDPWRLED             0xc0425a04 /* added by zeroone, also used in fp_control/global.h ; set PowerLed Brightness on HDBOX*/
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDRIVERINIT         0xc0425a08
#define VFDICONDISPLAYONOFF   0xc0425a0a

#define VFDTEST               0xc0425af0 // added by audioniek
#define VFDGETBLUEKEY         0xc0425af1 // unused, used by other boxes
#define VFDSETBLUEKEY         0xc0425af2 // unused, used by other boxes
#define VFDGETSTBYKEY         0xc0425af3 // unused, used by other boxes
#define VFDSETSTBYKEY         0xc0425af4 // unused, used by other boxes
#define VFDPOWEROFF           0xc0425af5 // unused, used by other boxes
#define VFDSETPOWERONTIME     0xc0425af6 // unused, used by other boxes
#define VFDGETVERSION         0xc0425af7 // unused, used by other boxes
#define VFDGETSTARTUPSTATE    0xc0425af8 // unused, used by other boxes
#define VFDGETWAKEUPMODE      0xc0425af9
#define VFDGETTIME            0xc0425afa
#define VFDSETTIME            0xc0425afb
#define VFDSTANDBY            0xc0425afc
//
#define VFDSETTIME2           0xc0425afd // unused, used by other boxes
#define VFDSETLED             0xc0425afe
#define VFDSETMODE            0xc0425aff
#define VFDDISPLAYCLR         0xc0425b00 // unused, used by other boxes
#define VFDGETLOOPSTATE       0xc0425b01 // unused, used by other boxes
#define VFDSETLOOPSTATE       0xc0425b02 // unused, used by other boxes
#define VFDGETWAKEUPTIME      0xc0425b03 // added by audioniek, unused, used by other boxes
#define VFDSETTIMEFORMAT      0xc0425b04 // added by audioniek, unused, used by other boxes

struct set_test_s
{
	char test[19];
};

struct set_brightness_s
{
	int level;
};

struct set_pwrled_s
{
	int level;
};

struct set_icon_s
{
	int icon_nr;
	int on;
};

struct set_led_s
{
	int led_nr;
	int level;
};

struct set_light_s
{
	int onoff;
};

/* time must be given as follows:
 * time[0] & time[1] = mjd ???
 * time[2] = hour
 * time[3] = min
 * time[4] = sec
 */
struct set_standby_s
{
	char time[5];
};

struct set_time_s
{
	char time[5];
};

struct set_timeformat_s
{
	int format;
};

/* This will set the mode temporarily (for one ioctl)
 * to the desired mode. Currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s
{
	int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

struct nuvoton_ioctl_data
{
	union
	{
		struct set_test_s test;
		struct set_icon_s icon;
		struct set_led_s led;
		struct set_brightness_s brightness;
		struct set_pwrled_s pwrled;
		struct set_light_s light;
		struct set_mode_s mode;
		struct set_standby_s standby;
		struct set_time_s time;
		struct set_timeformat_s timeformat;
	} u;
};

struct vfd_ioctl_data
{
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};

/****************************************************************************************/

#if defined(OCTAGON1008) \
 || defined(HS7420) \
 || defined(HS7429)
struct vfd_buffer
{
	u8 buf0;
	u8 buf1;
	u8 buf2;
};
#endif

#endif //_123_nuvoton
