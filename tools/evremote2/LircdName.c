/*
 * LircdName.c
 *
 * (c) 2014 duckbox project
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
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include "global.h"
#include "map.h"
#include "remotes.h"

#define REPEATDELAY 130 // ms
#define REPEATFREQ 20 // ms

static tLongKeyPressSupport cLongKeyPressSupport =
{
	REPEATDELAY, REPEATFREQ /*delay, period*/
};

static long long GetNow(void)
{
#define MIN_RESOLUTION 1 // ms
	static bool initialized = false;
	static bool monotonic = false;
	struct timespec tp;

	if (!initialized)
	{
		// check if monotonic timer is available and provides enough accurate resolution:
		if (clock_getres(CLOCK_MONOTONIC, &tp) == 0)
		{
			//long Resolution = tp.tv_nsec;
			// require a minimum resolution:
			if (tp.tv_sec == 0 && tp.tv_nsec <= MIN_RESOLUTION * 1000000)
			{
				if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
				{
					monotonic = true;
				}
			}
		}

		initialized = true;
	}

	if (monotonic)
	{
		if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
			return (long long)(tp.tv_sec) * 1000 + tp.tv_nsec / 1000000;

		monotonic = false;
		// fall back to gettimeofday()
	}

	struct timeval t;

	if (gettimeofday(&t, NULL) == 0)
		return (long long)(t.tv_sec) * 1000 + t.tv_usec / 1000;

	return 0;
}


/* Key is recognized by name of it in lircd.conf */
static tButton cButtons_LircdName[] =
{

	{"KEY_OK"           , "=>", KEY_OK},
	{"KEY_UP"           , "=>", KEY_UP},
	{"KEY_DOWN"         , "=>", KEY_DOWN},
	{"KEY_RIGHT"        , "=>", KEY_RIGHT},
	{"KEY_LEFT"         , "=>", KEY_LEFT},

	{"KEY_RED"          , "=>", KEY_RED},
	{"KEY_GREEN"        , "=>", KEY_GREEN},
	{"KEY_YELLOW"       , "=>", KEY_YELLOW},
	{"KEY_BLUE"         , "=>", KEY_BLUE},
	{"KEY_POWER"        , "=>", KEY_POWER},

	{"KEY_VOLUMEUP"     , "=>", KEY_VOLUMEUP},
	{"KEY_VOLUMEDOWN"   , "=>", KEY_VOLUMEDOWN},
	{"KEY_MUTE"         , "=>", KEY_MUTE},
	{"KEY_PAGEUP"       , "=>", KEY_PAGEUP},
	{"KEY_PAGEDOWN"     , "=>", KEY_PAGEDOWN},

	{"KEY_MENU"         , "=>", KEY_MENU},
	{"KEY_HOME"         , "=>", KEY_HOME},
	{"KEY_OPTION"       , "=>", KEY_OPTION},
	{"KEY_EPG"          , "=>", KEY_EPG},
	{"KEY_GOTO"         , "=>", KEY_GOTO},

	{"KEY_PROGRAM"      , "=>", KEY_PROGRAM},
	{"KEY_TEXT"         , "=>", KEY_TEXT},
	{"KEY_HELP"         , "=>", KEY_HELP},
	{"KEY_LIST"         , "=>", KEY_LIST},
	{"KEY_MEDIA"        , "=>", KEY_MEDIA},

	{"KEY_1"            , "=>", KEY_1},
	{"KEY_2"            , "=>", KEY_2},
	{"KEY_3"            , "=>", KEY_3},
	{"KEY_4"            , "=>", KEY_4},
	{"KEY_5"            , "=>", KEY_5},

	{"KEY_6"            , "=>", KEY_6},
	{"KEY_7"            , "=>", KEY_7},
	{"KEY_8"            , "=>", KEY_8},
	{"KEY_9"            , "=>", KEY_9},
	{"KEY_0"            , "=>", KEY_0},

	{"KEY_PVR"          , "=>", KEY_PVR},
	{"KEY_PLAY"         , "=>", KEY_PLAY},
	{"KEY_PAUSE"        , "=>", KEY_PAUSE},
	{"KEY_RECORD"       , "=>", KEY_RECORD},
	{"KEY_STOP"         , "=>", KEY_STOP},
	{"KEY_FASTFORWARD"  , "=>", KEY_FASTFORWARD},
	{"KEY_REWIND"       , "=>", KEY_REWIND},

	{"KEY_MODE"         , "=>", KEY_MODE},
	{"KEY_SUBTITLE"     , "=>", KEY_SUBTITLE},
	{"KEY_V"            , "=>", KEY_V},
	{"KEY_AUX"          , "=>", KEY_AUX},
	{"KEY_TIME"         , "=>", KEY_TIME},

	{"KEY_TV2"          , "=>", KEY_TV2},
	{"KEY_BACK"         , "=>", KEY_BACK},
	{"KEY_FIND"         , "=>", KEY_FIND},
	{"KEY_ARCHIVE"      , "=>", KEY_ARCHIVE},
	{"KEY_INFO"         , "=>", KEY_INFO},

	{"KEY_FAVORITES"    , "=>", KEY_FAVORITES},
	{"KEY_SAT"          , "=>", KEY_SAT},
	{"KEY_PREVIOUS"     , "=>", KEY_PREVIOUS},
	{"KEY_NEXT"         , "=>", KEY_NEXT},
	{"KEY_F"            , "=>", KEY_F},

	{"KEY_SLOW"         , "=>", KEY_SLOW},
	{"KEY_P"            , "=>", KEY_P},
	{"KEY_CLOSE"        , "=>", KEY_CLOSE},
	{"KEY_T"            , "=>", KEY_T},
	{"KEY_F1"           , "=>", KEY_F1},

	{"KEY_F2"           , "=>", KEY_F2},
	{"KEY_F3"           , "=>", KEY_F3},
	{"KEY_SELECT"       , "=>", KEY_SELECT},
	{"KEY_POWER2"       , "=>", KEY_POWER2},
	{"KEY_CLEAR"        , "=>", KEY_CLEAR},

	{"KEY_VENDOR"       , "=>", KEY_VENDOR},
	{"KEY_CHANNEL"      , "=>", KEY_CHANNEL},
	{"KEY_MHP"          , "=>", KEY_MHP},
	{"KEY_LANGUAGE"     , "=>", KEY_LANGUAGE},
	{"KEY_TITLE"        , "=>", KEY_TITLE},

	{"KEY_ANGLE"        , "=>", KEY_ANGLE},
	{"KEY_ZOOM"         , "=>", KEY_ZOOM},
	{"KEY_KEYBOARD"     , "=>", KEY_KEYBOARD},
	{"KEY_SCREEN"       , "=>", KEY_SCREEN},
	{"KEY_PC"           , "=>", KEY_PC},

	{"KEY_TV"           , "=>", KEY_TV},
	{"KEY_VCR"          , "=>", KEY_VCR},
	{"KEY_VCR2"         , "=>", KEY_VCR2},
	{"KEY_SAT2"         , "=>", KEY_SAT2},
	{"KEY_CD"           , "=>", KEY_CD},

	{"KEY_TAPE"         , "=>", KEY_TAPE},
	{"KEY_RADIO"        , "=>", KEY_RADIO},
	{"KEY_TUNER"        , "=>", KEY_TUNER},
	{"KEY_PLAYER"       , "=>", KEY_PLAYER},
	{"KEY_DVD"          , "=>", KEY_DVD},

	{"KEY_MP3"          , "=>", KEY_MP3},
	{"KEY_AUDIO"        , "=>", KEY_AUDIO},
	{"KEY_VIDEO"        , "=>", KEY_VIDEO},
	{"KEY_DIRECTORY"    , "=>", KEY_DIRECTORY},
	{"KEY_MEMO"         , "=>", KEY_MEMO},

	{"KEY_CALENDAR"     , "=>", KEY_CALENDAR},
	{"KEY_CHANNELUP"    , "=>", KEY_CHANNELUP},
	{"KEY_CHANNELDOWN"  , "=>", KEY_CHANNELDOWN},
	{"KEY_FIRST"        , "=>", KEY_FIRST},
	{"KEY_LAST"         , "=>", KEY_LAST},

	{"KEY_AB"           , "=>", KEY_AB},
	{"KEY_RESTART"      , "=>", KEY_RESTART},
	{"KEY_SHUFFLE"      , "=>", KEY_SHUFFLE},
	{"KEY_DIGITS"       , "=>", KEY_DIGITS},
	{"KEY_TEEN"         , "=>", KEY_TEEN},

	{"KEY_TWEN"         , "=>", KEY_TWEN},
	{"KEY_BREAK"        , "=>", KEY_BREAK},
	{"KEY_PLAYPAUSE"    , "=>", KEY_PLAYPAUSE},
	{"KEY_EXIT"         , "=>", KEY_EXIT},
	{"KEY_SLEEP"        , "=>", KEY_SLEEP},
	{"KEY_OPEN"         , "=>", KEY_OPEN},
  {"KEY_POWERON"      , "=>", KEY_POWERON},
  {"KEY_POWEROFF"     , "=>", KEY_POWEROFF},
  {"KEY_STANDBYON"    , "=>", KEY_STANDBYON},
  {"KEY_STANDBYOFF"   , "=>", KEY_STANDBYOFF},
	{""                 , ""  , KEY_NULL},
};
/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un  vAddr;

static int LastKeyCode = -1;
static char LastKeyName[30];
static long long LastKeyPressedTime;
static int LircdBtnDelay = REPEATDELAY;
static int KeyPowerCounter = 0;
static int BlinkingIcon = -1;

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	vAddr.sun_family = AF_UNIX;

	if (argc >= 4)
		LircdBtnDelay = (atoi(argv[3]) == 0) ? REPEATDELAY : atoi(argv[3]);
	if (argc >= 5)
	{
		BlinkingIcon = (atoi(argv[4]) == 0) ? 0 : atoi(argv[4]);
		printf("[LircdName RCU] init delay %d ms, blinking ICON %i\n", LircdBtnDelay, BlinkingIcon);
	}
	else
		printf("[LircdName RCU] init delay %d ms\n", LircdBtnDelay);

	// in new lircd its moved to /var/run/lirc/lircd by default and need use key to run as old version
	if (access("/var/run/lirc/lircd", F_OK) == 0)
		strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
	else
	{
		strcpy(vAddr.sun_path, "/dev/lircd");
	}

	vHandle = socket(AF_UNIX, SOCK_STREAM, 0);

	if (vHandle == -1)
	{
		perror("socket");
		return -1;
	}

	if (connect(vHandle, (struct sockaddr *)&vAddr, sizeof(vAddr)) == -1)
	{
		perror("connect");
		return -1;
	}

	return vHandle;
}

static int pShutdown(Context_t *context)
{

	close(context->fd);

	return 0;
}

static int pRead(Context_t *context)
{
	char vBuffer[128];
	char vData[3];
	const int cSize = 128;
	int vCurrentCode = -1;
	char *buffer;
	char KeyName[30]; 	//For flexibility we use Lircd keys names
	int LastKeyNameChar;	//for long detection on RCU sending different codes for short/long
	int count;
	tButton *cButtons = cButtons_LircdName;

	long long LastTime;

	memset(vBuffer, 0, 128);

	//wait for new command
	read(context->fd, vBuffer, cSize);

	if (sscanf(vBuffer, "%*x %x %29s", &count, KeyName) != 2)  // '29' in '%29s' is LIRC_KEY_BUF-1!
	{
		printf("[LircdName RCU] Warning: unparseable lirc command: %s\n", vBuffer);
		return -1;
	}

	//some RCUs send different codes for single click and long push. This breakes e2 LONG detection, because lircd counter starts from beginning
	//workarround is to define names for long codes ending '&' in lircd.conf and using this marker to copunt data correctly
	LastKeyNameChar = strlen(KeyName) - 1 ;
	if (KeyName[LastKeyNameChar] == 0x26) //&
	{
		//printf("[LircdName RCU] LONG detected\n");
		count += 1;
		KeyName[LastKeyNameChar] = 0;
	}

	vCurrentCode = getInternalCodeLircKeyName(cButtons, KeyName);

	if (vCurrentCode != 0)
	{
		static int nextflag = 0;
		if (count == 0)
		{
			//Emergency reboot after 5xPOWER, we count only presses within 2 seconds, not lONGs
			if (!strncasecmp(LastKeyName, "KEY_POWER", 9) && !strncasecmp(KeyName, "KEY_POWER", 9)  && (GetNow() - LastKeyPressedTime < 2000))
			{
				KeyPowerCounter += 1;
				printf("[LircdName RCU] KEY_POWER pressed %d time(s)\n", KeyPowerCounter);
				if (KeyPowerCounter >= 5)
				{
					printf("[LircdName RCU] EMERGENCY REBOOT !!!\n");
					fflush(stdout);
					system("init 6");
					sleep(4);
					reboot(LINUX_REBOOT_CMD_RESTART);
					return -1;
				}
			}
			else
				KeyPowerCounter = 0;
			//time checking
			if ((LastKeyCode == vCurrentCode) && (GetNow() - LastKeyPressedTime < LircdBtnDelay))   // (diffMilli(LastKeyPressedTime, CurrKeyPressedTime) <= REPEATDELAY) )
			{
				printf("[LircdName RCU] skiping next press of same key coming in too fast %lld ms\n", GetNow() - LastKeyPressedTime);
				return -1;
			}
			else if (GetNow() - LastKeyPressedTime < LircdBtnDelay)
			{
				printf("[LircdName RCU] skiping different keys coming in too fast %lld ms\n", GetNow() - LastKeyPressedTime);
				return -1;
			}
			else
			{
				printf("[RCU LircdName] new KeyName: '%s', after %lld ms, LastKey: '%s', count: %i -> %s\n", KeyName, GetNow() - LastKeyPressedTime, LastKeyName, count, &vBuffer[0]);
			}
			nextflag++;

		}
		else
			printf("[RCU LircdName] same KeyName: '%s', after %lld ms, LastKey: '%s', count: %i -> %s\n", KeyName, GetNow() - LastKeyPressedTime, LastKeyName, count, &vBuffer[0]);

		LastKeyCode = vCurrentCode;
		LastKeyPressedTime = GetNow();
		strcpy(LastKeyName, KeyName);

		vCurrentCode += (nextflag << 16);
	}
	else
		printf("[RCU LircdName] unknown key -> %s\n", &vBuffer[0]);

	return vCurrentCode;


}

static int pNotification(Context_t *context, const int cOn)
{
	if ((BlinkingIcon == -1)) // && (cOn == 1))
	{
		printf("[LircdName RCU] << end\n");
		return 0;
	}

	//printf("[LircdName RCU] ICON %i %i\n", BlinkingIcon, cOn);

	int file_vfd = -1;
	char icon = BlinkingIcon;


	struct
	{
		unsigned char start;
		unsigned char data[64];
		unsigned char length;
	} data;

	struct
	{
		int icon_nr;
		int on;
	} vfd_icon;

	data.start = 0x00;
	data.data[0] = icon;
	data.data[4] = cOn;
	data.length = 5;

	vfd_icon.icon_nr = icon;
	vfd_icon.on = cOn;

	if ((file_vfd = open("/dev/vfd", O_RDWR)) == -1)
	{
		printf("[LircdName]: could not open vfd-device!\n");
	}
	else
	{
		ioctl(file_vfd, 0xc0425a0a, &data); //0xc0425a0a = VFDICONDISPLAYONOFF
		ioctl(file_vfd, 0xc0425a0a, &vfd_icon); //0xc0425a0a = VFDICONDISPLAYONOFF
		close(file_vfd);
	}
	return 0;
}

RemoteControl_t LircdName_RC =
{
	"LircdName Universal RemoteControl v.1.2",
	LircdName,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtons_LircdName,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};
