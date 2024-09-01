#ifndef GLOBAL_H_
#define GLOBAL_H_

#ifndef bool
#define bool unsigned char
#define true 1
#define false 0
#endif

#define VFDGETTIME           0xc0425afa
#define VFDSETTIME           0xc0425afb
#define VFDSTANDBY           0xc0425afc
#define VFDREBOOT            0xc0425afd
#define VFDSETLED            0xc0425afe
#define VFDICONDISPLAYONOFF  0xc0425a0a
#define VFDDISPLAYCHARS      0xc0425a00
#define VFDBRIGHTNESS        0xc0425a03
#define VFDPWRLED            0xc0425a04
#define VFDDISPLAYWRITEONOFF 0xc0425a05
#define VFDDISPLAYCLR        0xc0425b00
/* ufs912, 922, hdbox ->unset compat mode */
#define VFDSETMODE           0xc0425aff
/*spark*/
#define VFDGETSTARTUPSTATE   0xc0425af8

/* ufs912 */
#define VFDGETVERSION        0xc0425af7
#define VFDLEDBRIGHTNESS     0xc0425af8
#define VFDGETWAKEUPMODE     0xc0425af9

struct vfd_ioctl_data
{
	unsigned char start;
	unsigned char data[64];
	unsigned char length;
};

typedef enum {NONE, POWERON, STANDBY, TIMER, POWERSWITCH, UNK1, UNK2, UNK3} eWakeupReason;

typedef enum {Unknown, Ufs910_1W, Ufs910_14W, Ufs922, Ufc960, Tf7700, Hl101, Vip2, Fortis, Hs5101, Ufs912, Spark, Cuberevo, Adb_Box, CNBox} eBoxType;

typedef struct Context_s
{
	/* Model_t */
	void *m; /* instance data */
	int fd; /* filedescriptor of fd */

} Context_t;

typedef struct Model_s
{
		char *Name;
		eBoxType Type;
		int (* Init)(Context_t *context);
		int (* Clear)(Context_t *context);
		int (* Usage)(Context_t *context, char *prg_name);
		int (* SetTime)(Context_t *context, time_t *theGMTTime);
		int (* GetTime)(Context_t *context, time_t *theGMTTime);
		int (* SetTimer)(Context_t *context, time_t *theGMTTime);
		int (* GetTimer)(Context_t *context, time_t *theGMTTime);
		int (* Shutdown)(Context_t *context, time_t *shutdownTimeGMT);
		int (* Reboot)(Context_t *context, time_t *rebootTimeGMT);
		int (* Sleep)(Context_t *context, time_t *wakeUpGMT);
		int (* SetText)(Context_t *context, char *theText);
		int (* SetLed)(Context_t *context, int which, int on);
		int (* SetIcon)(Context_t *context, int which, int on);
		int (* SetBrightness)(Context_t *context, int brightness);
		int (* SetPwrLed)(Context_t *context, int pwrled);
		int (* GetWakeupReason)(Context_t *context, eWakeupReason *reason);
		int (* SetLight)(Context_t *context, int on);
		int (* Exit)(Context_t *context);
		int (* SetLedBrightness)(Context_t *context, int brightness);
		int (* GetVersion)(Context_t *context, int *version);
		int (* SetRF)(Context_t *context, int on);
		int (* SetFan)(Context_t *context, int on);
		int (* GetWakeupTime)(Context_t *context, time_t *theGMTTime);
		int (* SetDisplayTime)(Context_t *context, int on);
		int (* SetTimeMode)(Context_t *context, int twentyFour);
		void *private;
} Model_t;

extern Model_t Ufs910_1W_model;
extern Model_t Ufs910_14W_model;
extern Model_t UFS912_model;
extern Model_t UFS922_model;
extern Model_t UFC960_model;
extern Model_t Fortis_model;
extern Model_t HL101_model;
extern Model_t VIP2_model;
extern Model_t Hs5101_model;
extern Model_t Spark_model;
extern Model_t Adb_Box_model;
extern Model_t Cuberevo_model;
extern Model_t CNBOX_model;

double modJulianDate(struct tm *theTime);
time_t read_timers_utc(time_t curTime);
time_t read_fake_timer_utc(time_t curTime);
int searchModel(Context_t *context, eBoxType type);
int checkConfig(int *display, int *display_custom, char **timeFormat, int *wakeup);

int getWakeupReasonPseudo(eWakeupReason *reason);
int syncWasTimerWakeup(eWakeupReason reason);

#endif
