#ifndef _123_micom
#define _123_micom
/*
 */

extern short paramDebug;

#define TAGDEBUG "[micom] "

#ifndef dprintk
#if 1
#define dprintk(level, x...)
#else
#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug >= level)) printk(TAGDEBUG x); \
	} while (0)
#endif
#endif

extern int micom_init_func(void);
extern void copyData(unsigned char *data, int len);
extern void getRCData(unsigned char *data, int *len);
void dumpValues(void);

extern int                  errorOccured;

extern struct file_operations vfd_fops;

typedef struct
{
	struct file      *fp;
	int               read;
	struct semaphore  sem;

} tFrontPanelOpen;

#define FRONTPANEL_MINOR_RC   1
#define LASTMINOR             2

extern tFrontPanelOpen FrontPanelOpen[LASTMINOR];

#define VFD_MAJOR           147

/* ioctl numbers ->hacky */
#define VFDBRIGHTNESS        0xc0425a03
#define VFDDRIVERINIT        0xc0425a08
#define VFDICONDISPLAYONOFF  0xc0425a0a
#define VFDDISPLAYWRITEONOFF 0xc0425a05
#define VFDDISPLAYCHARS      0xc0425a00

#define VFDGETVERSION        0xc0425af7
#define VFDLEDBRIGHTNESS     0xc0425af8
#define VFDGETWAKEUPMODE     0xc0425af9
#define VFDGETTIME           0xc0425afa
#define VFDSETTIME           0xc0425afb
#define VFDSTANDBY           0xc0425afc
#define VFDREBOOT            0xc0425afd

#define VFDSETLED            0xc0425afe //BlueLed
#define VFDSETMODE           0xc0425aff

#define VFDSETREDLED		0xc0425af6
#define VFDSETGREENLED		0xc0425af5
#define VFDDEEPSTANDBY		0xc0425a81

#define	NO_ICON	0x0000

struct set_brightness_s
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
	//int led_nr;
	int on;
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

/* this setups the mode temporarily (for one ioctl)
 * to the desired mode. currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s
{
	int compat; /* 0 = compatibility mode to vfd driver; 1 = micom mode */
};

struct micom_ioctl_data
{
	union
	{
		struct set_icon_s icon;
		struct set_led_s led;
		struct set_brightness_s brightness;
		struct set_mode_s mode;
		struct set_standby_s standby;
		struct set_time_s time;
	} u;
};

struct vfd_ioctl_data
{
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};



/* Vitamin stuff */

#define MC_MSG_KEYIN	0x01
#define VFD_MAX_LEN		11

#define IR_CUSTOM_HI	0x01
#define IR_CUSTOM_LO	0xFE
#define IR_POWER		0x5F
#define FRONT_POWER		0x00

#define MAX_WAIT		10

enum MICOM_CLA_E
{
	CLASS_BOOTCHECK			= 0x01,	/* loader only */
	CLASS_MCTX			= 0x02,
	CLASS_KEY			= 0x10,
	CLASS_IR			= 0x20,
	CLASS_CLOCK			= 0x30,
	CLASS_CLOCKHI			= 0x40,
	CLASS_STATE			= 0x50,
	CLASS_POWER			= 0x60,
	CLASS_RESET			= 0x70,
	CLASS_WDT			= 0x80,
	CLASS_MUTE			= 0x90,
	CLASS_STATE_REQ			= 0xA0,
	CLASS_TIME			= 0xB0,
	CLASS_DISPLAY			= 0xC0,
	CLASS_FIX_IR			= 0xD0,
	CLASS_BLOCK			= 0xE0,
	CLASS_ON_TIME			= 0xF0,
	CLASS_FIGURE			= 0xF1, /* VFD ICON */
	CLASS_FIGURE2			= 0xF2, /* VFD ICON */
	CLASS_LED			= 0xF3,
	CLASS_SENSOR			= 0xF4,
	CLASS_DISPLAY2			= 0xF5,
	CLASS_INTENSITY			= 0xF6, /* VFD BRIGHTNESS */
	CLASS_DISP_ALL			= 0xF7,
	CLASS_WRITEFONT			= 0xF8,
	CLASS_IR_LED			= 0xF9, /* control IR LED */
	CLASS_CIRCLE			= 0xFA,
	CLASS_BLUELED			= 0xFB,
	CLASS_ECHO			= 0xFF, /* for debug */
};

enum
{
	POWER_ON = 0,
	POWER_DOWN,
	POWER_SLEEP,
	POWER_RESET,
};

enum
{
	RESET_0 = 0,
	RESET_POWER_ON,
};





enum
{
	ICON_TV = 0x0,
	ICON_RADIO,
	ICON_REPEAT,
	ICON_CIRCLE,
	ICON_RECORD,
	ICON_POWER,
	ICON_DOLBY,
	ICON_MUTE,
	ICON_DOLLAR,
	ICON_LOCK,
	ICON_CLOCK,
	ICON_HD,
	ICON_DVD,
	ICON_MP3,
	ICON_NET,
	ICON_USB,
	ICON_MC,
};

#endif
