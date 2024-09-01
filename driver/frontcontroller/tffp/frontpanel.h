#ifndef FRONTPANEL_H
#define FRONTPANEL_H
/*
  This device driver uses the following device entries:

  /dev/fpc      c 62 0      # FP control (read/write, for clock- and timer operations, shutdown etc.)
  /dev/rc       c 62 1      # Remote Control (read-only)
  /dev/fplarge  c 62 2      # Direct access to the 8 character display (write-only)
  /dev/fpsmall  c 62 3      # Direct access to the 4 character display (write-only)
 */

#include "tftypes.h"

#define FRONTPANEL_MAJOR		62    /* experimental major number */
#define FRONTPANEL_MINOR_FPC            0
#define FRONTPANEL_MINOR_RC             1
#define FRONTPANEL_MINOR_FPLARGE        2
#define FRONTPANEL_MINOR_FPSMALL        3

/* RWSS SSSS SSSS SSSS KKKK KKKK NNNN NNNN
   R = read
   W = write
   S = parameter size
   K = ioctl magic = 0x3a
   N = command number
*/

#define FRONTPANELGETTIME               0x40003a00 | (sizeof(frontpanel_ioctl_time) << 16)
#define FRONTPANELSETTIME               0x40003a01
#define FRONTPANELSYNCSYSTIME           0x40003a02
#define FRONTPANELCLEARTIMER            0x40003a03
#define FRONTPANELSETTIMER              0x40003a04
#define FRONTPANELBRIGHTNESS            0x40003a05 | (sizeof(frontpanel_ioctl_brightness) << 16)
#define FRONTPANELIRFILTER1             0x40003a06
#define FRONTPANELIRFILTER2             0x40003a07
#define FRONTPANELIRFILTER3             0x40003a08
#define FRONTPANELIRFILTER4             0x40003a09
#define FRONTPANELPOWEROFF              0x40003a0a
#define FRONTPANELBOOTREASON            0x40003a0b | (sizeof(frontpanel_ioctl_bootreason) << 16)
#define FRONTPANELCLEAR                 0x40003a0c
#define FRONTPANELTYPEMATICDELAY        0x40003a0d | (sizeof(frontpanel_ioctl_typematicdelay) << 16)
#define FRONTPANELTYPEMATICRATE         0x40003a0e | (sizeof(frontpanel_ioctl_typematicrate) << 16)
#define FRONTPANELKEYEMULATION          0x40003a0f | (sizeof(frontpanel_ioctl_keyemulation) << 16)
#define FRONTPANELREBOOT                0x40003a10
#define FRONTPANELSYNCFPTIME            0x40003a11
#define FRONTPANELSETGMTOFFSET          0x40003a12
#define FRONTPANELRESEND                0x40003a13
#define FRONTPANELALLCAPS               0x40003a14 | (sizeof(frontpanel_ioctl_allcaps) << 16)
#define FRONTPANELSCROLLMODE            0x40003a15 | (sizeof(frontpanel_ioctl_scrollmode) << 16)
#define FRONTPANELICON                  0x40003a20 | (sizeof(frontpanel_ioctl_icons) << 16)

//The following icons belong to block 1
#define FPICON_IRDOT                    0x00000001
#define FPICON_POWER                    0x00000002
#define FPICON_COLON                    0x00000004
#define FPICON_AM                       0x00000008
#define FPICON_PM                       0x00000010
#define FPICON_TIMER                    0x00000020
#define FPICON_AC3                      0x00000040
#define FPICON_TIMESHIFT                0x00000080
#define FPICON_TV                       0x00000100
#define FPICON_MUSIC                    0x00000200
#define FPICON_DISH                     0x00000400
#define FPICON_MP3                      0x00000800
#define FPICON_TUNER1                   0x00001000
#define FPICON_TUNER2                   0x00002000
#define FPICON_REC                      0x00004000
#define FPICON_MUTE                     0x00008000
#define FPICON_PAUSE                    0x00010000
#define FPICON_FWD                      0x00020000
#define FPICON_RWD                      0x00040000
#define FPICON_xxx2                     0x00080000
#define FPICON_PLAY                     0x00100000
#define FPICON_xxx4                     0x00200000
#define FPICON_NETWORK                  0x00400000
#define FPICON_DOLBY                    0x00800000
#define FPICON_ATTN                     0x01000000
#define FPICON_DOLLAR                   0x02000000
#define FPICON_AUTOREWRIGHT             0x04000000
#define FPICON_AUTOREWLEFT              0x08000000

//The following icons belong to block 2
#define FPICON_CDCENTER                 0x00000001
#define FPICON_CD1                      0x00000002
#define FPICON_CD2                      0x00000004
#define FPICON_CD3                      0x00000008
#define FPICON_CD4                      0x00000010
#define FPICON_CD5                      0x00000020
#define FPICON_CD6                      0x00000040
#define FPICON_CD7                      0x00000080
#define FPICON_CD8                      0x00000100
#define FPICON_CD9                      0x00000200
#define FPICON_CD10                     0x00000400
#define FPICON_CD11                     0x00000800
#define FPICON_CD12                     0x00001000

#define FPICON_HDD                      0x00002000
#define FPICON_HDDFRAME                 0x00004000
#define FPICON_HDD1                     0x00008000
#define FPICON_HDD2                     0x00010000
#define FPICON_HDD3                     0x00020000
#define FPICON_HDD4                     0x00040000
#define FPICON_HDD5                     0x00080000
#define FPICON_HDD6                     0x00100000
#define FPICON_HDD7                     0x00200000
#define FPICON_HDD8                     0x00400000
#define FPICON_HDDFULL                  0x00800000



typedef struct
{
	word    year;
	word    month;
	word    day;
	word    dow;
	char    sdow[4];
	word    hour;
	word    min;
	word    sec;
	dword   now;
} frontpanel_ioctl_time;

typedef struct
{
	byte    bright;
} frontpanel_ioctl_brightness;

typedef struct
{
	byte    reason;
} frontpanel_ioctl_bootreason;

typedef struct
{
	byte    onoff;
} frontpanel_ioctl_irfilter;

typedef struct
{
	dword   Icons1;
	dword   Icons2;
	byte    BlinkMode;
} frontpanel_ioctl_icons;

typedef struct
{
	byte    TypematicDelay;
} frontpanel_ioctl_typematicdelay;

typedef struct
{
	byte    TypematicRate;
} frontpanel_ioctl_typematicrate;

typedef struct
{
	byte    KeyEmulation;
} frontpanel_ioctl_keyemulation;

typedef struct
{
	byte    AllCaps;
} frontpanel_ioctl_allcaps;

typedef struct
{
	byte    ScrollMode;
	byte    ScrollPause;
	byte    ScrollDelay;
} frontpanel_ioctl_scrollmode;


extern void vfdSetGmtWakeupTime(time_t time);
extern void vfdSetGmtTime(time_t time);
extern time_t vfdGetGmtTime(void);
extern int getBootReason(void);

#endif
