/*
 * Tf7700.c
 *
 * (c) 2009 teamducktales
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

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 110,
};

static tButton cButtonsTopfield7700HDPVR[] =
{
	{"STANDBY"        , "0A", KEY_POWER}, //This is the real power key, but sometimes we only get 0x0C FP
	{"MUTE"           , "0C", KEY_MUTE},

	{"V.FORMAT"       , "42", KEY_ZOOM},
	{"A/R"            , "43", KEY_SCREEN},
	{"AUX"            , "08", KEY_AUX},

	{"0BUTTON"        , "10", KEY_0},
	{"1BUTTON"        , "11", KEY_1},
	{"2BUTTON"        , "12", KEY_2},
	{"3BUTTON"        , "13", KEY_3},
	{"4BUTTON"        , "14", KEY_4},
	{"5BUTTON"        , "15", KEY_5},
	{"6BUTTON"        , "16", KEY_6},
	{"7BUTTON"        , "17", KEY_7},
	{"8BUTTON"        , "18", KEY_8},
	{"9BUTTON"        , "19", KEY_9},

	{"BACK"           , "1E", KEY_BACK},
	{"INFO"           , "1D", KEY_HELP}, //THIS IS WRONG SHOULD BE KEY_INFO
	{"AUDIO"          , "05", KEY_LANGUAGE},
	{"SUBTITLE"       , "07", KEY_SUBTITLE},
	{"TEXT"           , "47", KEY_TEXT},

	{"DOWN/P-"        , "01", KEY_DOWN},
	{"UP/P+"          , "00", KEY_UP},
	{"RIGHT/V+"       , "02", KEY_RIGHT},
	{"LEFT/V-"        , "03", KEY_LEFT},
	{"OK/LIST"        , "1F", KEY_OK},
	{"MENU"           , "1A", KEY_MENU},
	{"GUIDE"          , "1B", KEY_EPG},
	{"EXIT"           , "1C", KEY_HOME},
	{"FAV"            , "09", KEY_FAVORITES},

	{"RED"            , "4D", KEY_RED},
	{"GREEN"          , "0D", KEY_GREEN},
	{"YELLOW"         , "0E", KEY_YELLOW},
	{"BLUE"           , "0F", KEY_BLUE},

	{"REWIND"         , "45", KEY_REWIND},
	{"PAUSE"          , "06", KEY_PAUSE},
	{"PLAY"           , "46", KEY_PLAY},
	{"FASTFORWARD"    , "48", KEY_FASTFORWARD},
	{"RECORD"         , "4B", KEY_RECORD},
	{"STOP"           , "4A", KEY_STOP},
	{"SLOWMOTION"     , "49", KEY_SLOW},
	{"RECORDLIST"     , "51", KEY_LIST},
	{"SAT"            , "5E", KEY_SAT},
	{"STEPBACK"       , "50", KEY_PREVIOUS},
	{"STEPFORWARD"    , "52", KEY_NEXT},
	{"MARK"           , "4C", KEY_OPTION},
	{"TV/RADIO"       , "04", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCH BUTTON
	{"USB"            , "40", KEY_ARCHIVE},
	{"TIMER"          , "44", KEY_TIME},
	{""               , ""  , KEY_NULL},
};

/* ***************** our fp button assignment **************** */

static tButton cButtonsTopfield7700HDPVRFrontpanel[] =
{
	{"CHANNELDOWN"    , "02", KEY_PAGEDOWN},
	{"CHANNELUP"      , "03", KEY_PAGEUP},

	{"VOLUMEDOWN"     , "01", KEY_VOLUMEDOWN},
	{"VOLUMEUP"       , "06", KEY_VOLUMEUP},

	{"STANDBY"        , "0C", KEY_POWER}, // This is the fake power call
	{""               , ""  , KEY_NULL},
};


static int pInit(Context_t *context, int argc, char *argv[])
{

	int vFd              = open("/dev/rc", O_RDWR);

	if (argc >= 2)
	{
		cLongKeyPressSupport.period = atoi(argv[1]);
	}

	if (argc >= 3)
	{
		cLongKeyPressSupport.delay = atoi(argv[2]);
	}

	printf("period %d, delay %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);

	return vFd;
}

static int pShutdown(Context_t *context)
{

	close(context->fd);

	return 0;
}

static int gNextKey = 0;

static int pRead(Context_t *context)
{

	unsigned char vData[3];
	const int     cSize            = 3;
	eKeyType      vKeyType         = RemoteControl;
	int           vCurrentCode     = -1;
	unsigned char vIsBurst         = 0;
	//WORKAROUND: Power does not have burst flag
	static unsigned char sWasLastKeyPower = 0;

	// wait for new command
	read(context->fd, vData, cSize);

	//printf("---> 0x%02X 0x%02X\n", vData[0], vData[1]);
	if (vData[0] == 0x61)
		vKeyType = RemoteControl;
	else if (vData[0] == 0x51)
		vKeyType = FrontPanel;
	else //Control Sequence
		return -1;

	vIsBurst = ((vData[1] & 0x80) == 0x80) ? 1 : 0;
	vData[1] = vData[1] & 0x7f;

	if (vKeyType == RemoteControl)
		vCurrentCode = getInternalCodeHex((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData[1]);
	else
		vCurrentCode = getInternalCodeHex((tButton *)((RemoteControl_t *)context->r)->Frontpanel, vData[1]);

	// We have a problem here, the power key has no burst flag, so a quick hack would be to always
	// say its burst. this is not noce and hopefully nobody will notice
	if ((vKeyType == FrontPanel) && (vData[1] == 0x0C) && sWasLastKeyPower)
		vIsBurst = 1;

	sWasLastKeyPower = ((vKeyType == FrontPanel) && (vData[1] == 0x0C)) ? 1 : 0;

	//printf("vIsBurst=%d Key=0x%02x\n", vIsBurst, vCurrentCode);

	if (vIsBurst == 0) // new key
	{
		gNextKey++;
		gNextKey %= 20;
	}

	vCurrentCode += (gNextKey << 16);

	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{

	//Notification is handeld by the frontpanel
	return 0;
}

RemoteControl_t Tf7700_RC =
{
	"Tf7700 RemoteControl",
	Tf7700,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonsTopfield7700HDPVR,
	cButtonsTopfield7700HDPVRFrontpanel,
	NULL,
	1,
	&cLongKeyPressSupport,
};
