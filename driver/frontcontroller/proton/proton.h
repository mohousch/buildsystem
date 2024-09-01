
#define VFD_MAJOR				147

#define VFDBRIGHTNESS         0xc0425a03
#define VFDDRIVERINIT         0xc0425a08
#define VFDICONDISPLAYONOFF   0xc0425a0a
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDISPLAYCHARS       0xc0425a00

#define VFDGETWAKEUPMODE      0xc0425af9
#define VFDGETTIME            0xc0425afa
#define VFDSETTIME            0xc0425afb
#define VFDSTANDBY            0xc0425afc

#define VFDSETLED             0xc0425afe
#define VFDSETMODE            0xc0425aff
#define VFDDISPLAYCLR		  0xc0425b00

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
	int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

struct proton_ioctl_data
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

typedef struct VFD_Format_s
{
	unsigned char LogNum;
	unsigned char LogSta;
} VFD_Format_T;

typedef struct VFD_Time_s
{
	unsigned char hour;
	unsigned char mint;
} VFD_Time_T;

typedef enum
{
	REMOTE_OLD = 0,
	REMOTE_NEW,
	REMOTE_TOPFIELD,
	REMOTE_UNKNOWN
} REMOTE_TYPE;

enum
{
	POWER_KEY 		= 88,

	TIME_SET_KEY 	= 87,
	UHF_KEY 		= 68,
	VFormat_KEY 	= 67,
	MUTE_KEY 		= 66,

	TVSAT_KEY 		= 65,
	MUSIC_KEY 		= 64,
	FIND_KEY 		= 63,
	FAV_KEY 		= 62,

	MENU_KEY 		= 102,	//HOME
	i_KEY 			= 61,
	EPG_KEY 		= 18,
	EXIT_KEY 		= 48,	//B
	RECALL_KEY 		= 30,
	RECORD_KEY 		= 19,

	UP_KEY 			= 103,	//UP
	DOWN_KEY		= 108,	//DOWN
	LEFT_KEY 		= 105,	//LEFT
	RIGHT_KEY		= 106,	//RIGTHT
	SELECT_KEY 		= 0x160,	//ENTER

	PLAY_KEY 		= 25,
	PAGE_UP_KEY 	= 104,	//P_UP
	PAUSE_KEY 		= 22,
	PAGE_DOWN_KEY 	= 109,	//P_DOWN

	STOP_KEY 		= 20,
	SLOW_MOTION_KEY = 50,
	FASTREWIND_KEY  = 33,
	FASTFORWARD_KEY = 49,

	DOCMENT_KEY 	= 32,
	SWITCH_PIC_KEY 	= 17,
	PALY_MODE_KEY 	= 24,
	USB_KEY 		= 111,

	RADIO_KEY 		= 110,
	SAT_KEY 		= 15,
	F1_KEY 			= 59,
	F2_KEY 			= 60,

	RED_KEY 		= 44,	//Z
	GREEN_KEY 		= 45,	//X
	YELLOW_KEY 		= 46,	//C
	BLUE_KEY 		= 47	//V
};

enum
{
	KEY_DIGIT0 = 11,
	KEY_DIGIT1 = 2,
	KEY_DIGIT2 = 3,
	KEY_DIGIT3 = 4,
	KEY_DIGIT4 = 5,
	KEY_DIGIT5 = 6,
	KEY_DIGIT6 = 7,
	KEY_DIGIT7 = 8,
	KEY_DIGIT8 = 9,
	KEY_DIGIT9 = 10
};


typedef enum LogNum_e
{
	/*----------------------------------11G-------------------------------------*/
	PLAY_FASTBACKWARD = 11 * 16 + 1,
	PLAY_HEAD,
	PLAY_LOG,
	PLAY_TAIL,
	PLAY_FASTFORWARD,
	PLAY_PAUSE,
	REC1,
	MUTE,
	CYCLE,
	DUBI,
	CA,
	CI,
	USB,
	DOUBLESCREEN,
	REC2,
	/*----------------------------------12G-------------------------------------*/
	HDD_A8 = 12 * 16 + 1,
	HDD_A7,
	HDD_A6,
	HDD_A5,
	HDD_A4,
	HDD_A3,
	HDD_FULL,
	HDD_A2,
	HDD_A1,
	MP3,
	AC3,
	TVMODE_LOG,
	AUDIO,
	ALERT,
	HDD_A9,
	/*----------------------------------13G-------------------------------------*/
	CLOCK_PM = 13 * 16 + 1,
	CLOCK_AM,
	CLOCK,
	TIME_SECOND,
	DOT2,
	STANDBY,
	TER,
	DISK_S3,
	DISK_S2,
	DISK_S1,
	DISK_S0,
	SAT,
	TIMESHIFT,
	DOT1,
	CAB,
	/*----------------------------------end-------------------------------------*/
	LogNum_Max
} LogNum_T;

#define BASE_VFD_PRIVATE 0x00

#define VFD_GetRevision         _IOWR('s',(BASE_VFD_PRIVATE+0),char*)
#define VFD_ShowLog             _IOWR('s',(BASE_VFD_PRIVATE+1),VFD_Format_T)
#define VFD_ShowTime            _IOWR('s',(BASE_VFD_PRIVATE+2),VFD_Time_T)
#define VFD_ShowStr             _IOWR('s',(BASE_VFD_PRIVATE+3),char*)
#define VFD_ClearTime           _IOWR('s',(BASE_VFD_PRIVATE+4),int)
