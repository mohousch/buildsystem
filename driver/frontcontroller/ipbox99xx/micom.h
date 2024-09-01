/*
 */

#define VFD_MAJOR		147

#define MICOM_MAX_DISPLAY_LEN	14
#define VFD_MAX_DATA_LEN	(25-3)

/* ioctl numbers ->hacky */
#define	VFDDISPLAYCHARS 	0xc0425a00
#define VFDBRIGHTNESS		0xc0425a03
#define VFDDISPLAYWRITEONOFF	0xc0425a05
#define VFDDRIVERINIT		0xc0425a08
#define VFDICONDISPLAYONOFF	0xc0425a0a

#define VFDDISPLAYCHARSRAW	0xc0425a80 // sisyfos specific
#define VFDDEEPSTANDBY		0xc0425a81 // sisyfos specific
#define VFDDISPLAYCLEAR  	0xc0425a82 // sisyfos specific

#define VFDSETTIMERWAKEUP	0xc0425af9
#define VFDGETSTATUS		0xc0425afa
#define VFDSETTIME		0xc0425afb
#define VFDSTANDBY		0xc0425afc
#define VFDRESET		0xc0425afd

#define VFDSETLED		0xc0425afe

#define VFDSETMODE		0xc0425aff

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

struct set_wakeup_s
{
	char timer[5];
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
		struct set_wakeup_s wakeup;
	} u;
};

struct vfd_ioctl_data
{
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};
