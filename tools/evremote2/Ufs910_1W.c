/*
 * Ufs910_1W.c
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

#define UFS910_1W_LONGKEY

#ifdef UFS910_1W_LONGKEY
static tLongKeyPressSupport cLongKeyPressSupport =
{
	20, 106,
};
#endif

static tButton cButtonsKathrein[] =
{
//    {"VFORMAT_FRONT"  , "4A", KEY_MENU}, // any idea?
	{"MENU"           , "54", KEY_MENU},
	{"MENU_FRONT"     , "49", KEY_MENU},
	{"RED"            , "6D", KEY_RED},
	{"GREEN"          , "6E", KEY_GREEN},
	{"YELLOW"         , "6F", KEY_YELLOW},
	{"BLUE"           , "70", KEY_BLUE},
	{"EXIT"           , "55", KEY_HOME},
	{"EXIT_FRONT"     , "4B", KEY_HOME},
	{"TEXT"           , "3C", KEY_TEXT},
	{"EPG"            , "4C", KEY_EPG},
	{"REWIND"         , "21", KEY_REWIND},
	{"FASTFORWARD"    , "20", KEY_FASTFORWARD},
	{"PLAY"           , "38", KEY_PLAY},
	{"PAUSE"          , "39", KEY_PAUSE},
	{"RECORD"         , "37", KEY_RECORD},
	{"STOP"           , "31", KEY_STOP},
	{"STANDBY"        , "0C", KEY_POWER},
	{"STANDBY_FRONT"  , "48", KEY_POWER},
	{"MUTE"           , "0D", KEY_MUTE},
	{"CHANNELUP"      , "1E", KEY_PAGEUP},
	{"CHANNELDOWN"    , "1F", KEY_PAGEDOWN},
	{"VOLUMEUP"       , "10", KEY_VOLUMEUP},
	{"VOLUMEDOWN"     , "11", KEY_VOLUMEDOWN},
	{"INFO"           , "0F", KEY_INFO},
	{"OPTIONS_FRONT"  , "47", KEY_HELP},
	{"OK"             , "5C", KEY_OK},
	{"UP"             , "58", KEY_UP},
	{"RIGHT"          , "5B", KEY_RIGHT},
	{"DOWN"           , "59", KEY_DOWN},
	{"LEFT"           , "5A", KEY_LEFT},
	{"0BUTTON"        , "00", KEY_0},
	{"1BUTTON"        , "01", KEY_1},
	{"2BUTTON"        , "02", KEY_2},
	{"3BUTTON"        , "03", KEY_3},
	{"4BUTTON"        , "04", KEY_4},
	{"5BUTTON"        , "05", KEY_5},
	{"6BUTTON"        , "06", KEY_6},
	{"7BUTTON"        , "07", KEY_7},
	{"8BUTTON"        , "08", KEY_8},
	{"9BUTTON"        , "09", KEY_9},

	{"LMENU"          , "D4", KEY_MENU},
	{"LRED"           , "ED", KEY_RED},
	{"LGREEN"         , "EE", KEY_GREEN},
	{"LYELLOW"        , "EF", KEY_YELLOW},
	{"LBLUE"          , "F0", KEY_BLUE},
	{"LEXIT"          , "D5", KEY_HOME},
	{"LTEXT"          , "BC", KEY_TEXT},
	{"LEPG"           , "CC", KEY_EPG},
	{"LREWIND"        , "A1", KEY_REWIND},
	{"LFASTFORWARD"   , "A0", KEY_FASTFORWARD},
	{"LPLAY"          , "B8", KEY_PLAY},
	{"LPAUSE"         , "B9", KEY_PAUSE},
	{"LRECORD"        , "B7", KEY_RECORD},
	{"LSTOP"          , "B1", KEY_STOP},
	{"LSTANDBY"       , "8C", KEY_POWER},
	{"LMUTE"          , "8D", KEY_MUTE},
	{"LCHANNELUP"     , "9E", KEY_PAGEUP},
	{"LCHANNELDOWN"   , "9F", KEY_PAGEDOWN},
	{"LVOLUMEUP"      , "90", KEY_VOLUMEUP},
	{"LVOLUMEDOWN"    , "91", KEY_VOLUMEDOWN},
	{"LINFO"          , "8F", KEY_HELP}, //THIS IS WRONG SHOULD BE KEY_INFO
	{"LOK"            , "DC", KEY_OK},
	{"LUP"            , "D8", KEY_UP},
	{"LRIGHT"         , "DB", KEY_RIGHT},
	{"LDOWN"          , "D9", KEY_DOWN},
	{"LLEFT"          , "DA", KEY_LEFT},
	{"L0BUTTON"       , "80", KEY_0},
	{"L1BUTTON"       , "81", KEY_1},
	{"L2BUTTON"       , "82", KEY_2},
	{"L3BUTTON"       , "83", KEY_3},
	{"L4BUTTON"       , "84", KEY_4},
	{"L5BUTTON"       , "85", KEY_5},
	{"L6BUTTON"       , "86", KEY_6},
	{"L7BUTTON"       , "87", KEY_7},
	{"L8BUTTON"       , "88", KEY_8},
	{"L9BUTTON"       , "89", KEY_9},

	{""               , ""  , KEY_NULL},
};


static int          vFd;


static int setTemFlagsKathrein(int fd)
{
	struct termios old_io;
	struct termios new_io;

	if ((tcgetattr(fd, &old_io)) == 0)
	{
		new_io = old_io;

		printf("setting new flags\n");
		/* c_iflags ->input flags */
		new_io.c_iflag &= ~(IMAXBEL | BRKINT | ICRNL);

		/* c_lflags ->local flags*/
		new_io.c_lflag &= ~(ECHO | IEXTEN);

		/* c_oflags ->output flags*/
		new_io.c_oflag &= ~(ONLCR);

		/* c_cflags ->constant flags*/
		new_io.c_cflag = B19200;

		tcsetattr(fd, TCSANOW, &new_io);

	}
	else
		printf("error set raw mode.\n");

	return 0;
}


static int pInit(Context_t *context, int argc, char *argv[])
{

	vFd              = open("/dev/ttyAS1", O_RDWR);
	setTemFlagsKathrein(vFd);

	return vFd;
}

static int pShutdown(Context_t *context)
{

	close(vFd);

	return 0;
}

#ifndef UFS910_1W_LONGKEY
static int pRead(Context_t *context)
{

	char         vData[3];
	const int    cSize             = 3;
	int          vCurrentCode      = -1;

	//wait for new command
	read(vFd, vData, cSize);

	//parse and send key event
	vData[2] = '\0';

	vCurrentCode = getInternalCode((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData);

	return vCurrentCode;
}
#else

static int gNextKey = 0;

static int pRead(Context_t *context)
{

	char         vData[3];
	const int    cSize             = 3;
	int          vCurrentCode      = -1;

	//wait for new command
	read(vFd, vData, cSize);

	//parse and send key event
	vData[2] = '\0';

	vCurrentCode = getInternalCode((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData);

	if ((vCurrentCode & 0x80) == 0) // new key
	{
		gNextKey++;
		gNextKey %= 20;
	}

	vCurrentCode += (gNextKey << 16);

	return vCurrentCode;
}
#endif

static int pNotification(Context_t *context, const int cOn)
{

	if (cOn)
		write(vFd, "1\n1\n1\n1\n", 8);
	else
	{
		usleep(50000);
		write(vFd, "A\nA\nA\nA\n", 8);
	}

	return 0;
}

RemoteControl_t Ufs910_1W_RC =
{
	"Ufs910 1Watt RemoteControl",
	Ufs910_1W,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonsKathrein,
	NULL,
	NULL,
#ifndef UFS910_1W_LONGKEY
	0,
	NULL,
#else
	1,
	&cLongKeyPressSupport,
#endif
};
