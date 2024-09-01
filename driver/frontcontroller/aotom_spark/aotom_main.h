#ifndef __AOTOM_MAIN_H__
#define __AOTOM_MAIN_H__

#define LED_OFF               0
#define LED_ON                1
#define LOG_OFF               LED_OFF // deprecated
#define LOG_ON                LED_ON  // deprecated
#define LED_RED               0
#define LED_GREEN             1
#define LED_COUNT             2
#define LED_SPINNER           2

#define VFDBRIGHTNESS         0xc0425a03
#define VFDDRIVERINIT         0xc0425a08
#define VFDICONDISPLAYONOFF   0xc0425a0a
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDISPLAYCHARS       0xc0425a00

#define VFDGETBLUEKEY         0xc0425af1
#define VFDSETBLUEKEY         0xc0425af2
#define VFDGETSTBYKEY         0xc0425af3
#define VFDSETSTBYKEY         0xc0425af4
#define VFDPOWEROFF           0xc0425af5
#define VFDSETPOWERONTIME     0xc0425af6
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
#define VFDGETLOOPSTATE       0xc0425b01
#define VFDSETLOOPSTATE       0xc0425b02

#define INVALID_KEY           -1

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

struct set_key_s
{
	int key_nr;
	unsigned int key;
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
		struct set_key_s key;
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
	/*----------------------------------11G-------------------------------------*/
	AOTOM_PLAY_FASTBACKWARD = 11 * 16 + 1,
	AOTOM_FIRST = AOTOM_PLAY_FASTBACKWARD,
	AOTOM_PLAY_PREV,
	AOTOM_PLAY_HEAD = AOTOM_PLAY_PREV,  // deprecated
	AOTOM_PLAY,
	AOTOM_PLAY_LOG = AOTOM_PLAY,        // deprecated
	AOTOM_NEXT,
	AOTOM_PLAY_TAIL = AOTOM_NEXT,       // deprecated
	AOTOM_PLAY_FASTFORWARD,
	AOTOM_PLAY_PAUSE,
	AOTOM_REC1,
	AOTOM_MUTE,
	AOTOM_LOOP,
	AOTOM_CYCLE = AOTOM_LOOP,           // deprecated
	AOTOM_DOLBYDIGITAL,
	AOTOM_DUBI = AOTOM_DOLBYDIGITAL,    // deprecated
	AOTOM_CA,
	AOTOM_CI,
	AOTOM_USB,
	AOTOM_DOUBLESCREEN,
	AOTOM_REC2,
	/*----------------------------------12G-------------------------------------*/
	AOTOM_HDD_A8 = 12 * 16 + 1,
	AOTOM_HDD_A7,
	AOTOM_HDD_A6,
	AOTOM_HDD_A5,
	AOTOM_HDD_A4,
	AOTOM_HDD_A3,
	AOTOM_HDD_FULL,
	AOTOM_HDD_A2,
	AOTOM_HDD_A1,
	AOTOM_MP3,
	AOTOM_AC3,
	AOTOM_TV,
	AOTOM_TVMODE_LOG = AOTOM_TV,        // deprecated
	AOTOM_AUDIO,
	AOTOM_ALERT,
	AOTOM_HDD_FRAME,
	AOTOM_HDD_A9 = AOTOM_HDD_FRAME,     // deprecated
	/*----------------------------------13G-------------------------------------*/
	AOTOM_CLOCK_PM = 13 * 16 + 1,
	AOTOM_CLOCK_AM,
	AOTOM_CLOCK,
	AOTOM_TIME_SECOND,
	AOTOM_DOT2,
	AOTOM_STANDBY,
	AOTOM_TERRESTRIAL,
	AOTOM_TER = AOTOM_TERRESTRIAL,      // deprecated
	AOTOM_DISK_S3,
	AOTOM_DISK_S2,
	AOTOM_DISK_S1,
	AOTOM_DISK_CIRCLE,
	AOTOM_DISK_S0 = AOTOM_DISK_CIRCLE,  // deprecated
	AOTOM_SATELLITE,
	AOTOM_SAT = AOTOM_SATELLITE,        // deprecated
	AOTOM_TIMESHIFT,
	AOTOM_DOT1,
	AOTOM_CABLE,
	AOTOM_CAB = AOTOM_CABLE,            // deprecated
	AOTOM_LAST = AOTOM_CABLE,
	/*----------------------------------end-------------------------------------*/
	AOTOM_ALL
};

#ifdef __KERNEL__

#define VFD_MAJOR       147
#define I2C_BUS_NUM     1
#define I2C_BUS_ADD     (0x50>>1)  //this is important not 0x50
typedef unsigned int    YWOS_ClockMsec;

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

#if 0
typedef struct VFD_Format_s
{
	unsigned char LogNum;
	unsigned char LogSta;
} VFD_Format_T;
#endif

typedef struct VFD_Time_s
{
	unsigned char hour;
	unsigned char mint;
} VFD_Time_T;

#define YWPANEL_FP_INFO_MAX_LENGTH        (10)
#define YWPANEL_FP_DATA_MAX_LENGTH        (38)

typedef struct YWPANEL_I2CData_s
{
	u8  writeBuffLen;
	u8  writeBuff[YWPANEL_FP_DATA_MAX_LENGTH];
	u8  readBuff[YWPANEL_FP_INFO_MAX_LENGTH];

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
	YWPANEL_DATATYPE_GETCPUSTATE,
	YWPANEL_DATATYPE_SETCPUSTATE,
	YWPANEL_DATATYPE_GETVFDSTATE,
	YWPANEL_DATATYPE_SETVFDSTATE,

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

	YWPANEL_DATATYPE_GETPOWERONSTATE,
	YWPANEL_DATATYPE_SETPOWERONSTATE,
	YWPANEL_DATATYPE_GETSTARTUPSTATE,
	YWPANEL_DATATYPE_SETSTARTUPSTATE,
	YWPANEL_DATATYPE_GETLOOPSTATE,
	YWPANEL_DATATYPE_SETLOOPSTATE,

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
	YWPANEL_DVFDDataType_t type;
	u8 setValue;
	u8 ulen;
	u8 address[16];
	u8 DisplayValue[16][5];
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
	YWPANEL_ENCRYPEMODE_T    mode;
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

#if 0
typedef struct YWVFD_Format_s
{
	u8 LogNum;
	u8 LogSta;
} YWVFD_Format_T;
#endif

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
	YWPANEL_DataType_t  dataType;

	union
	{
		YWPANEL_Version_t           version;
		YWPANEL_LBDData_t           lbdData;
		YWPANEL_LEDData_t           ledData;
		YWPANEL_LCDData_t           lcdData;
		YWPANEL_VFDData_t           vfdData;
		YWPANEL_DVFDData_t          dvfdData;
		YWPANEL_IRKEY_t             IrkeyData;
		YWPANEL_SCANKEY_t           ScanKeyData;
		YWPANEL_CpuState_t          CpuState;
		YWPANEL_StandByKey_t        stbyKey;
		YWPANEL_IRCode_t            irCode;
		YWPANEL_EncryptMode_t       EncryptMode;
		YWPANEL_EncryptKey_t        EncryptKey;
		YWPANEL_VerifyState_t       verifyState;
		YWPANEL_Time_t              time;
		YWPANEL_ControlTimer_t      TimeState;
		YWPANEL_VfdStandbyState_T   VfdStandbyState;
		YWPANEL_BlueKey_t           BlueKey;
		YWPANEL_PowerOnState_t      PowerOnState;
		YWPANEL_StartUpState_t      StartUpState;
		YWPANEL_LoopState_t         LoopState;
	} data;

	int ack;

} YWPANEL_FPData_t;

#define BASE_VFD_PRIVATE 0x00

// #define VFD_GetRevision  _IOWR('s',(BASE_VFD_PRIVATE+0),char*)
// #define VFD_ShowLog          _IOWR('s',(BASE_VFD_PRIVATE+1),YWVFD_Format_T)
#define VFD_ShowTime        _IOWR('s',(BASE_VFD_PRIVATE+2),YWVFD_Time_T)
#define VFD_ShowStr         _IOWR('s',(BASE_VFD_PRIVATE+3),char*)
#define VFD_ClearTime       _IOWR('s',(BASE_VFD_PRIVATE+4),int)
#define VFD_SetBright       _IOWR('s',(BASE_VFD_PRIVATE+5),int)
#define VFD_GetCPUState     _IOWR('s',(BASE_VFD_PRIVATE+6),YWPANEL_CPUSTATE_t)
#define VFD_SetCPUState     _IOWR('s',(BASE_VFD_PRIVATE+7),YWPANEL_CPUSTATE_t)
#define VFD_GetStartUpState _IOWR('s',(BASE_VFD_PRIVATE+8),YWPANEL_STARTUPSTATE_t)
#define VFD_GetVFDState     _IOWR('s',(BASE_VFD_PRIVATE+9),YWPANEL_VFDSTATE_t)
#define VFD_SetVFDState     _IOWR('s',(BASE_VFD_PRIVATE+10),YWPANEL_VFDSTATE_t)
#define VFD_GetPOWERONState _IOWR('s',(BASE_VFD_PRIVATE+11),YWPANEL_POWERONSTATE_t)
#define VFD_SetPOWERONState _IOWR('s',(BASE_VFD_PRIVATE+12),YWPANEL_POWERONSTATE_t)
#define VFD_GetTime         _IOWR('s',(BASE_VFD_PRIVATE+13),u32)
#define VFD_SetTime         _IOWR('s',(BASE_VFD_PRIVATE+14),u32)
#define VFD_ControlTime     _IOWR('s',(BASE_VFD_PRIVATE+15),int)
#define VFD_GetStandByKey   _IOWR('s',(BASE_VFD_PRIVATE+16),YWVFD_FuncKey_T)
#define VFD_SetStandByKey   _IOWR('s',(BASE_VFD_PRIVATE+17),YWVFD_FuncKey_T)
#define VFD_GetBlueKey      _IOWR('s',(BASE_VFD_PRIVATE+18),YWVFD_FuncKey_T)
#define VFD_SetBlueKey      _IOWR('s',(BASE_VFD_PRIVATE+19),YWVFD_FuncKey_T)
#define VFD_GetPowerOnTime  _IOWR('s',(BASE_VFD_PRIVATE+20),u32)
#define VFD_SetPowerOnTime  _IOWR('s',(BASE_VFD_PRIVATE+21),u32)
#define VFD_ControlLBD      _IOWR('s',(BASE_VFD_PRIVATE+22),YWPANEL_LBDStatus_T)

int YWPANEL_VFD_Init(void);
extern int (*YWPANEL_VFD_Term)(void);
extern int (*YWPANEL_VFD_Initialize)(void);
extern int (*YWPANEL_VFD_ShowIcon)(int, int);
extern int (*YWPANEL_VFD_ShowTime)(u8 hh, u8 mm);
extern int (*YWPANEL_VFD_ShowTimeOff)(void);
extern int (*YWPANEL_VFD_SetBrightness)(int);
extern u8(*YWPANEL_VFD_ScanKeyboard)(void);
extern int (*YWPANEL_VFD_ShowString)(char *);

extern int YWPANEL_width;

YWPANEL_VFDSTATE_t YWPANEL_FP_GetVFDStatus(void);
int YWPANEL_FP_SetVFDStatus(YWPANEL_VFDSTATE_t state);
YWPANEL_CPUSTATE_t YWPANEL_FP_GetCpuStatus(void);
int YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_t state);
int YWPANEL_FP_ControlTimer(int on);
YWPANEL_POWERONSTATE_t YWPANEL_FP_GetPowerOnStatus(void);
int YWPANEL_FP_SetPowerOnStatus(YWPANEL_POWERONSTATE_t state);
u32 YWPANEL_FP_GetTime(void);
int YWPANEL_FP_SetTime(u32 value);
int YWPANEL_FP_GetKey(int blue, int key_nr, u32 *k);
int YWPANEL_FP_SetKey(int blue, int key_nr, u32 k);
int YWPANEL_FP_GetStartUpState(YWPANEL_STARTUPSTATE_t *State);
int YWPANEL_FP_GetVersion(YWPANEL_Version_t *version);
int  YWPANEL_FP_GetLoopState(YWPANEL_LOOPSTATE_t *state);
int  YWPANEL_FP_SetLoopState(YWPANEL_LOOPSTATE_t state);

int YWPANEL_FP_SetPowerOnTime(u32 Value);
u32 YWPANEL_FP_GetPowerOnTime(void);
int YWPANEL_VFD_GetKeyValue(void);
int YWPANEL_VFD_SetLed(int which, int on);

int utf8strlen(char *s, int len);
int utf8charlen(unsigned char c);

int vfdlen_show(struct seq_file *m, void *v);
int vfdlen_open(struct inode *inode, struct  file *file);

#endif /* __KERNEL__ */
#endif /* __AOTOM_MAIN_H__ */

// vim:ts=4
