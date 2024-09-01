#ifndef _123_micom
#define _123_micom
/*
 */

extern short paramDebug;

#define TAGDEBUG "[micom] "

#ifndef dprintk
#define dprintk(level, x...) do { \
		if ((paramDebug) && (paramDebug >= level)) printk(TAGDEBUG x); \
	} while (0)
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

#define VFDSETLED            0xc0425afe
#define VFDSETMODE           0xc0425aff

#define VFDSETRCCODE         0xc0425af6

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
	int led_nr;
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

#if defined(UFS922) || defined(UFC960)
enum
{
	LED_AUX = 0x1,
	LED_LIST,
	LED_POWER,
	LED_TV_R,
	LED_VOL,
	LED_WHEEL
};

enum
{
	ICON_MIN = 0x0,
	ICON_USB = 0x1,
	ICON_HD,
	ICON_HDD,
	ICON_SCRAMBLED,
	ICON_BLUETOOTH,
	ICON_MP3,
	ICON_RADIO,
	ICON_DOLBY,
	ICON_EMAIL,
	ICON_MUTE,
	ICON_PLAY,
	ICON_PAUSE,
	ICON_FF,
	ICON_REW,
	ICON_REC,
	ICON_TIMER,
	ICON_MAX
};
#endif

#if defined(UFS922) || defined(UFS912) || defined(UFS913) || defined(UFC969) || defined(UFH969)
#define VFD_LENGTH 16
#define VFD_CHARSIZE 1
#elif defined(UFI510) || defined(UFC960)
#define VFD_LENGTH 12
#define VFD_CHARSIZE 2
#else
#define VFD_LENGTH 16
#define VFD_CHARSIZE 1
#endif

#endif
