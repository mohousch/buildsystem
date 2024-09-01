#ifndef __AOTOM_MAIN_H__
#define __AOTOM_MAIN_H__

#ifndef __KERNEL__
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

#if 0
/* broken and unused */
typedef signed long s64;
typedef unsigned long u64;
#endif
#endif

#define VFD_MAJOR             147

#define LOG_OFF               0
#define LOG_ON                1
#define LED_RED               0
#define LED_GREEN             1
#define LASTLED               2

#define VFDBRIGHTNESS         0xc0425a03
#define VFDDRIVERINIT         0xc0425a08
#define VFDICONDISPLAYONOFF   0xc0425a0a
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDISPLAYCHARS       0xc0425a00

#define VFDGETVERSION         0xc0425af7
#define VFDGETSTARTUPSTATE    0xc0425af8
#define VFDGETWAKEUPMODE      0xc0425af9
#define VFDGETTIME            0xc0425afa
#define VFDSETTIME            0xc0425afb
#define VFDSTANDBY            0xc0425afc
#define VFDSETTIME2           0xc0425afd // seife, set 'complete' time...

#define VFDSETLED             0xc0425afe
#define VFDSETMODE            0xc0425aff
#define VFDDISPLAYCLR         0xc0425b00

#define	REMOTE_SLAVE_ADDRESS          0x40bd0000 /* slave address is 5 */
#define	REMOTE_SLAVE_ADDRESS_NEW      0xc03f0000 /* sz 2008-06-26 add new remote*/
#define	REMOTE_SLAVE_ADDRESS_EDISION1 0x22dd0000
#define	REMOTE_SLAVE_ADDRESS_EDISION2 0XCC330000
#define	REMOTE_SLAVE_ADDRESS_GOLDEN   0x48b70000 /* slave address is 5 */
#define REMOTE_TOPFIELD_MASK          0x4fb0000

#define YW_VFD_ENABLE
#define  INVALID_KEY    -1

#define	I2C_BUS_NUM      1
#define	I2C_BUS_ADD      (0x50>>1)  //this is important not 0x50

typedef unsigned int     YWOS_ClockMsec;

#define LOG_OFF     0
#define LOG_ON      1
#define YWPANEL_KEYBOARD

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

/* this changes the mode temporarily (for one ioctl)
 * to the desired mode. currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s
{
	int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

struct aotom_ioctl_data
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

enum
{
	POWER_KEY        = 88,

	TIME_SET_KEY     = 87,
	UHF_KEY          = 68,
	VFormat_KEY      = 67,
	MUTE_KEY         = 66,

	TVSAT_KEY        = 65,
	MUSIC_KEY        = 64,
	FIND_KEY         = 63,
	FAV_KEY          = 62,

	MENU_KEY         = 102, // HOME
	i_KEY            = 61,
	EPG_KEY          = 18,
	EXIT_KEY         = 48,	// B
	RECALL_KEY       = 30,
	RECORD_KEY       = 19,

	UP_KEY           = 103, // UP
	DOWN_KEY         = 108, // DOWN
	LEFT_KEY         = 105, // LEFT
	RIGHT_KEY        = 106, // RIGTHT
	SELECT_KEY       = 28, // ENTER

	PLAY_KEY         = 25,
	PAGE_UP_KEY      = 104, // P_UP
	PAUSE_KEY        = 22,
	PAGE_DOWN_KEY    = 109, // P_DOWN

	STOP_KEY         = 20,
	SLOW_MOTION_KEY  = 50,
	FAST_REWIND_KEY  = 33,
	FAST_FORWARD_KEY = 49,

	DOCMENT_KEY      = 32,
	SWITCH_PIC_KEY   = 17,
	PALY_MODE_KEY    = 24,
	USB_KEY          = 111,

	RADIO_KEY        = 110,
	SAT_KEY          = 15,
	F1_KEY           = 59,
	F2_KEY           = 60,

	RED_KEY          = 44, // Z
	GREEN_KEY        = 45, // X
	YELLOW_KEY       = 46, // C
	BLUE_KEY         = 47  // V
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

typedef enum
{
	REMOTE_OLD,
	REMOTE_NEW,
	REMOTE_TOPFIELD,
	REMOTE_EDISION1,
	REMOTE_EDISION2,
	REMOTE_GOLDEN,
	REMOTE_UNKNOWN
} REMOTE_TYPE;

typedef enum VFDMode_e
{
	VFDWRITEMODE,
	VFDREADMODE
} VFDMode_T;

typedef enum SegNum_e
{
	SEGNUM1 = 0,
	SEGNUM2
} SegNum_T;

typedef struct SegAddrVal_s
{
	u8 Segaddr1;
	u8 Segaddr2;
	u8 CurrValue1;
	u8 CurrValue2;
} SegAddrVal_T;

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

#define YWPANEL_FP_INFO_MAX_LENGTH		  (10)
#define YWPANEL_FP_DATA_MAX_LENGTH		  (38)

typedef struct YWPANEL_I2CData_s
{
	u8	writeBuffLen;
	u8	writeBuff[YWPANEL_FP_DATA_MAX_LENGTH];
	u8	readBuff[YWPANEL_FP_INFO_MAX_LENGTH];

} YWPANEL_I2CData_t;

typedef enum YWPANEL_DataType_e
{
	YWPANEL_DATATYPE_LBD = 0x01,
	YWPANEL_DATATYPE_LCD,
	YWPANEL_DATATYPE_LED,
	YWPANEL_DATATYPE_VFD,
	YWPANEL_DATATYPE_DVFD,
	YWPANEL_DATATYPE_SCANKEY,
	YWPANEL_DATATYPE_IRKEY,

	YWPANEL_DATATYPE_GETVERSION,
	YWPANEL_DATATYPE_GETVFDSTATE,
	YWPANEL_DATATYPE_SETVFDSTATE,
	YWPANEL_DATATYPE_GETCPUSTATE,
	YWPANEL_DATATYPE_SETCPUSTATE,

	YWPANEL_DATATYPE_GETSTBYKEY1,
	YWPANEL_DATATYPE_GETSTBYKEY2,
	YWPANEL_DATATYPE_GETSTBYKEY3,
	YWPANEL_DATATYPE_GETSTBYKEY4,
	YWPANEL_DATATYPE_GETSTBYKEY5,
	YWPANEL_DATATYPE_SETSTBYKEY1,
	YWPANEL_DATATYPE_SETSTBYKEY2,
	YWPANEL_DATATYPE_SETSTBYKEY3,
	YWPANEL_DATATYPE_SETSTBYKEY4,
	YWPANEL_DATATYPE_SETSTBYKEY5,
	YWPANEL_DATATYPE_GETBLUEKEY1,
	YWPANEL_DATATYPE_GETBLUEKEY2,
	YWPANEL_DATATYPE_GETBLUEKEY3,
	YWPANEL_DATATYPE_GETBLUEKEY4,
	YWPANEL_DATATYPE_GETBLUEKEY5,
	YWPANEL_DATATYPE_SETBLUEKEY1,
	YWPANEL_DATATYPE_SETBLUEKEY2,
	YWPANEL_DATATYPE_SETBLUEKEY3,
	YWPANEL_DATATYPE_SETBLUEKEY4,
	YWPANEL_DATATYPE_SETBLUEKEY5,

	YWPANEL_DATATYPE_GETIRCODE,
	YWPANEL_DATATYPE_SETIRCODE,

	YWPANEL_DATATYPE_GETENCRYPTMODE,
	YWPANEL_DATATYPE_SETENCRYPTMODE,
	YWPANEL_DATATYPE_GETENCRYPTKEY,
	YWPANEL_DATATYPE_SETENCRYPTKEY,

	YWPANEL_DATATYPE_GETVERIFYSTATE,
	YWPANEL_DATATYPE_SETVERIFYSTATE,

	YWPANEL_DATATYPE_GETTIME,
	YWPANEL_DATATYPE_SETTIME,
	YWPANEL_DATATYPE_CONTROLTIMER,

	YWPANEL_DATATYPE_SETPOWERONTIME,
	YWPANEL_DATATYPE_GETPOWERONTIME,

	YWPANEL_DATATYPE_GETVFDSTANDBYSTATE,
	YWPANEL_DATATYPE_SETVFDSTANDBYSTATE,

	YWPANEL_DATATYPE_GETPOWERONSTATE, /* 0x77 */
	YWPANEL_DATATYPE_SETPOWERONSTATE, /* 0x78 */
	YWPANEL_DATATYPE_GETSTARTUPSTATE, /* 0x79 */
	YWPANEL_DATATYPE_SETSTARTUPSTATE,
	YWPANEL_DATATYPE_GETLOOPSTATE,    /* 0x80 */
	YWPANEL_DATATYPE_SETLOOPSTATE,    /* 0x81 */

	YWPANEL_DATATYPE_NUM
} YWPANEL_DataType_t;

typedef struct YWPANEL_LBDData_s
{
	u8 value;
} YWPANEL_LBDData_t;

typedef struct YWPANEL_LEDData_s
{
	u8 led1;
	u8 led2;
	u8 led3;
	u8 led4;
} YWPANEL_LEDData_t;

typedef struct YWPANEL_LCDData_s
{
	u8 value;
} YWPANEL_LCDData_t;

typedef enum YWPANEL_VFDDataType_e
{
	YWPANEL_VFD_SETTING = 0x1,
	YWPANEL_VFD_DISPLAY,
	YWPANEL_VFD_READKEY,
	YWPANEL_VFD_DISPLAYSTRING
} YWPANEL_VFDDataType_t;

typedef struct YWPANEL_VFDData_s
{
	YWPANEL_VFDDataType_t  type; /*1- setting  2- display 3- readscankey  4-displaystring*/

	u8 setValue;   //if type == YWPANEL_VFD_SETTING
	u8 address[16];  //if type == YWPANEL_VFD_DISPLAY
	u8 DisplayValue[16];
	u8 key;  //if type == YWPANEL_VFD_READKEY

} YWPANEL_VFDData_t;

typedef enum YWPANEL_DVFDDataType_e
{
	YWPANEL_DVFD_SETTING = 0x1,
	YWPANEL_DVFD_DISPLAY,
	YWPANEL_DVFD_DISPLAYSTRING,
	YWPANEL_DVFD_DISPLAYSYNC,
	YWPANEL_DVFD_SETTIMEMODE,
	YWPANEL_DVFD_GETTIMEMODE,
} YWPANEL_DVFDDataType_t;

typedef struct YWPANEL_DVFDData_s
{
	YWPANEL_DVFDDataType_t  type;

	U8						setValue;

	U8						ulen;
	U8						address[16];
	U8						DisplayValue[16][5];
} YWPANEL_DVFDData_t;

typedef struct YWPANEL_IRKEY_s
{
	u32 customCode;
	u32 dataCode;
} YWPANEL_IRKEY_t;

typedef struct YWPANEL_SCANKEY_s
{
	u32 keyValue;
} YWPANEL_SCANKEY_t;

typedef struct YWPANEL_StandByKey_s
{
	u32 key;
} YWPANEL_StandByKey_t;

typedef enum YWPANEL_IRCODE_e
{
	YWPANEL_IRCODE_NONE,
	YWPANEL_IRCODE_NEC = 0x01,
	YWPANEL_IRCODE_RC5,
	YWPANEL_IRCODE_RC6,
	YWPANEL_IRCODE_PILIPS
} YWPANEL_IRCODE_T;

typedef struct YWPANEL_IRCode_s
{
	YWPANEL_IRCODE_T code;
} YWPANEL_IRCode_t;

typedef enum YWPANEL_ENCRYPEMODE_e
{
	YWPANEL_ENCRYPEMODE_NONE = 0x00,
	YWPANEL_ENCRYPEMODE_ODDBIT,
	YWPANEL_ENCRYPEMODE_EVENBIT,
	YWPANEL_ENCRYPEMODE_RAMDONBIT
} YWPANEL_ENCRYPEMODE_T;

typedef struct YWPANEL_EncryptMode_s
{
	YWPANEL_ENCRYPEMODE_T	 mode;
} YWPANEL_EncryptMode_t;

typedef struct YWPANEL_EncryptKey_s
{
	u32 key;
} YWPANEL_EncryptKey_t;

typedef enum YWPANEL_VERIFYSTATE_e
{
	YWPANEL_VERIFYSTATE_NONE = 0x00,
	YWPANEL_VERIFYSTATE_CRC16 ,
	YWPANEL_VERIFYSTATE_CRC32 ,
	YWPANEL_VERIFYSTATE_CHECKSUM
} YWPANEL_VERIFYSTATE_T;

typedef struct YWPANEL_VerifyState_s
{
	YWPANEL_VERIFYSTATE_T state;
} YWPANEL_VerifyState_t;

typedef struct YWPANEL_Time_s
{
	u32 second;
} YWPANEL_Time_t;

typedef struct YWPANEL_ControlTimer_s
{
	int startFlag; // 0 - stop  1-start
} YWPANEL_ControlTimer_t;

typedef struct YWPANEL_VfdStandbyState_s
{
	int On; // 0 - off  1-on
} YWPANEL_VfdStandbyState_T;

typedef struct YWPANEL_BlueKey_s
{
	u32 key;
} YWPANEL_BlueKey_t;

typedef struct YWVFD_Format_s
{
	u8 LogNum;
	u8 LogSta;
} YWVFD_Format_T;

typedef struct YWVFD_Time_s
{
	u8 hour;
	u8 mint;
} YWVFD_Time_T;

typedef enum YWPANEL_CPUSTATE_s
{
	YWPANEL_CPUSTATE_UNKNOWN,
	YWPANEL_CPUSTATE_RUNNING = 0x01,
	YWPANEL_CPUSTATE_STANDBY
} YWPANEL_CPUSTATE_t;

typedef enum YWPANEL_VFDSTATE_e
{
	YWPANEL_VFDSTATE_UNKNOWN,
	YWPANEL_VFDSTATE_STANDBYOFF = 0x01,
	YWPANEL_VFDSTATE_STANDBYON
} YWPANEL_VFDSTATE_t;

typedef enum YWPANEL_POWERONSTATE_e
{
	YWPANEL_POWERONSTATE_UNKNOWN,
	YWPANEL_POWERONSTATE_RUNNING = 0x01,
	YWPANEL_POWERONSTATE_CHECKPOWERBIT
} YWPANEL_POWERONSTATE_t;

typedef enum YWPANEL_LBDStatus_e
{
	YWPANEL_LBD_STATUS_OFF,
	YWPANEL_LBD_STATUS_ON,
	YWPANEL_LBD_STATUS_FL
} YWPANEL_LBDStatus_T;

typedef enum YWPANEL_STARTUPSTATE_e
{
	YWPANEL_STARTUPSTATE_UNKNOWN,
	YWPANEL_STARTUPSTATE_ELECTRIFY = 0x01,
	YWPANEL_STARTUPSTATE_STANDBY,
	YWPANEL_STARTUPSTATE_TIMER
} YWPANEL_STARTUPSTATE_t;

typedef enum YWPANEL_LOOPSTATE_e
{
	YWPANEL_LOOPSTATE_UNKNOWN,
	YWPANEL_LOOPSTATE_LOOPOFF = 0x01,
	YWPANEL_LOOPSTATE_LOOPON
} YWPANEL_LOOPSTATE_t;

typedef struct YWPANEL_CpuState_s
{
	YWPANEL_CPUSTATE_t state;
} YWPANEL_CpuState_t;

typedef struct YWVFD_FuncKey_s
{
	u8 key_index;
	u32 key_value;
} YWVFD_FuncKey_T;

typedef enum YWVFD_TYPE_s
{
	YWVFD_UNKNOWN,
	YWVFD_COMMON,
	YWVFD_STAND_BY
} YWVFD_TYPE_t;

typedef struct YWVFD_INFO_s
{
	YWVFD_TYPE_t vfd_type;
} YWVFD_INFO_t;

typedef struct YWPANEL_PowerOnState_s
{
	YWPANEL_POWERONSTATE_t state;
} YWPANEL_PowerOnState_t;

typedef struct YWPANEL_StartUpState_s
{
	YWPANEL_STARTUPSTATE_t State;
} YWPANEL_StartUpState_t;

typedef struct YWPANEL_LoopState_s
{
	YWPANEL_LOOPSTATE_t state;
} YWPANEL_LoopState_t;

typedef enum YWPANEL_LBDType_e
{
	YWPANEL_LBD_TYPE_POWER  = (1 << 0),
	YWPANEL_LBD_TYPE_SIGNAL = (1 << 1),
	YWPANEL_LBD_TYPE_MAIL   = (1 << 2),
	YWPANEL_LBD_TYPE_AUDIO  = (1 << 3)
} YWPANEL_LBDType_T;

typedef enum YWPAN_FP_MCUTYPE_E
{
	YWPANEL_FP_MCUTYPE_UNKNOWN = 0x00,
	YWPANEL_FP_MCUTYPE_AVR_ATTING48, // AVR MCU
	YWPANEL_FP_MCUTYPE_AVR_ATTING88,
	YWPAN_FP_MCUTYPE_MAX
} YWPAN_FP_MCUTYPE_T;

typedef enum YWPANEL_FP_DispType_e
{
	YWPANEL_FP_DISPTYPE_UNKNOWN = 0x00,
	YWPANEL_FP_DISPTYPE_VFD = (1 << 0),
	YWPANEL_FP_DISPTYPE_LCD = (1 << 1),
	YWPANEL_FP_DISPTYPE_DVFD = (3),
	YWPANEL_FP_DISPTYPE_LED = (1 << 2),
	YWPANEL_FP_DISPTYPE_LBD = (1 << 3)
} YWPANEL_FP_DispType_t;

typedef struct YWPANEL_Version_s
{
	YWPAN_FP_MCUTYPE_T CpuType;
	u8 DisplayInfo;
	u8 scankeyNum;
	u8 swMajorVersion;
	u8 swSubVersion;
} YWPANEL_Version_t;

typedef struct YWPANEL_FPData_s
{
	YWPANEL_DataType_t	dataType;
	union
	{
		YWPANEL_Version_t			version;
		YWPANEL_LBDData_t			lbdData;
		YWPANEL_LEDData_t			ledData;
		YWPANEL_LCDData_t			lcdData;
		YWPANEL_VFDData_t			vfdData;
		YWPANEL_DVFDData_t			dvfdData;
		YWPANEL_IRKEY_t 			IrkeyData;
		YWPANEL_SCANKEY_t			ScanKeyData;
		YWPANEL_CpuState_t			CpuState;
		YWPANEL_StandByKey_t		stbyKey;
		YWPANEL_IRCode_t			irCode;
		YWPANEL_EncryptMode_t		EncryptMode;
		YWPANEL_EncryptKey_t		EncryptKey;
		YWPANEL_VerifyState_t		verifyState;
		YWPANEL_Time_t				time;
		YWPANEL_ControlTimer_t		TimeState;
		YWPANEL_VfdStandbyState_T	VfdStandbyState;
		YWPANEL_BlueKey_t			BlueKey;
		YWPANEL_PowerOnState_t		PowerOnState;
		YWPANEL_StartUpState_t		StartUpState;
		YWPANEL_LoopState_t 		LoopState;
	} data;
	int ack;
} YWPANEL_FPData_t;

#define BASE_VFD_PRIVATE 0x00

// #define VFD_GetRevision         _IOWR('s',(BASE_VFD_PRIVATE+0),char*)
#define VFD_ShowLog             _IOWR('s',(BASE_VFD_PRIVATE+1),YWVFD_Format_T)
#define VFD_ShowTime            _IOWR('s',(BASE_VFD_PRIVATE+2),YWVFD_Time_T)
#define VFD_ShowStr             _IOWR('s',(BASE_VFD_PRIVATE+3),char*)
#define VFD_ClearTime           _IOWR('s',(BASE_VFD_PRIVATE+4),int)
#define VFD_SetBright           _IOWR('s',(BASE_VFD_PRIVATE+5),int)
#define VFD_GetCPUState         _IOWR('s',(BASE_VFD_PRIVATE+6),YWPANEL_CPUSTATE_t)
#define VFD_SetCPUState         _IOWR('s',(BASE_VFD_PRIVATE+7),YWPANEL_CPUSTATE_t)
#define VFD_GetStartUpState     _IOWR('s',(BASE_VFD_PRIVATE+8),YWPANEL_STARTUPSTATE_t)
#define VFD_GetVFDState         _IOWR('s',(BASE_VFD_PRIVATE+9),YWPANEL_VFDSTATE_t)
#define VFD_SetVFDState         _IOWR('s',(BASE_VFD_PRIVATE+10),YWPANEL_VFDSTATE_t)
#define VFD_GetPOWERONState     _IOWR('s',(BASE_VFD_PRIVATE+11),YWPANEL_POWERONSTATE_t)
#define VFD_SetPOWERONState     _IOWR('s',(BASE_VFD_PRIVATE+12),YWPANEL_POWERONSTATE_t)
#define VFD_GetTime             _IOWR('s',(BASE_VFD_PRIVATE+13),u32)
#define VFD_SetTime             _IOWR('s',(BASE_VFD_PRIVATE+14),u32)
#define VFD_ControlTime         _IOWR('s',(BASE_VFD_PRIVATE+15),int)
#define VFD_GetStandByKey       _IOWR('s',(BASE_VFD_PRIVATE+16),YWVFD_FuncKey_T)
#define VFD_SetStandByKey       _IOWR('s',(BASE_VFD_PRIVATE+17),YWVFD_FuncKey_T)
#define VFD_GetBlueKey          _IOWR('s',(BASE_VFD_PRIVATE+18),YWVFD_FuncKey_T)
#define VFD_SetBlueKey          _IOWR('s',(BASE_VFD_PRIVATE+19),YWVFD_FuncKey_T)
#define VFD_GetPowerOnTime      _IOWR('s',(BASE_VFD_PRIVATE+20),u32)
#define VFD_SetPowerOnTime      _IOWR('s',(BASE_VFD_PRIVATE+21),u32)
#define VFD_ControlLBD          _IOWR('s',(BASE_VFD_PRIVATE+22),YWPANEL_LBDStatus_T)

int YWPANEL_VFD_Init(void);
extern int (*YWPANEL_VFD_Term)(void);
extern int (*YWPANEL_VFD_Initialize)(void);
extern int (*YWPANEL_VFD_ShowIco)(LogNum_T, int);
extern int (*YWPANEL_VFD_ShowTime)(u8 hh, u8 mm);
extern int (*YWPANEL_VFD_ShowTimeOff)(void);
extern int (*YWPANEL_VFD_SetBrightness)(int);
extern u8(*YWPANEL_VFD_ScanKeyboard)(void);
extern int (*YWPANEL_VFD_ShowString)(char *);

extern int YWPANEL_width;

//int YWPANEL_VFD_GetRevision(char * version);
YWPANEL_VFDSTATE_t YWPANEL_FP_GetVFDStatus(void);
int YWPANEL_FP_SetVFDStatus(YWPANEL_VFDSTATE_t state);
YWPANEL_CPUSTATE_t YWPANEL_FP_GetCpuStatus(void);
int YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_t state);
int YWPANEL_FP_ControlTimer(int on);
YWPANEL_POWERONSTATE_t YWPANEL_FP_GetPowerOnStatus(void);
int YWPANEL_FP_SetPowerOnStatus(YWPANEL_POWERONSTATE_t state);
u32 YWPANEL_FP_GetTime(void);
int YWPANEL_FP_SetTime(u32 value);
u32 YWPANEL_FP_GetStandByKey(u8 index);
int YWPANEL_FP_SetStandByKey(u8 index, u8 key);
u32 YWPANEL_FP_GetBlueKey(u8 index);
int YWPANEL_FP_SetBlueKey(u8 index, u8 key);
int YWPANEL_LBD_SetStatus(YWPANEL_LBDStatus_T  LBDStatus);
int YWPANEL_FP_GetStartUpState(YWPANEL_STARTUPSTATE_t *State);
int YWPANEL_FP_GetVersion(YWPANEL_Version_t *version);

//u32  YWPANEL_FP_GetIRKey(void);
int YWPANEL_FP_SetPowerOnTime(u32 Value);
u32 YWPANEL_FP_GetPowerOnTime(void);
int YWPANEL_VFD_GetKeyValue(void);
int YWPANEL_VFD_SetLed(int which, int on);

#endif /* __AOTOM_MAIN_H__ */

// vim:ts=4

