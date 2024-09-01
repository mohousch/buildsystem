/*
 * Homecast5101.c
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

static tButton cButtonsHomecast5101[] =
{
	{"MENU"           , "5F", KEY_MENU},
	{"RED"            , "7F", KEY_RED},
	{"GREEN"          , "BF", KEY_GREEN},
	{"YELLOW"         , "3F", KEY_YELLOW},
	{"BLUE"           , "DF", KEY_BLUE},
	{"EXIT"           , "6D", KEY_HOME},
	{"EPG"            , "9F", KEY_EPG},
	{"STANDBY"        , "A7", KEY_POWER},
	{"MUTE"           , "8F", KEY_MUTE},
	{"CHANNELUP"      , "0F", KEY_PAGEUP},
	{"CHANNELDOWN"    , "CF", KEY_PAGEDOWN},
	{"VOLUMEUP"       , "2F", KEY_VOLUMEUP},
	{"VOLUMEDOWN"     , "4F", KEY_VOLUMEDOWN},
	{"INFO"           , "1F", KEY_HELP}, //THIS IS WRONG SHOULD BE KEY_INFO
	{"OK"             , "37", KEY_OK},
	{"BACK"           , "B7", KEY_BACK},
	{"UP"             , "77", KEY_UP},
	{"RIGHT"          , "D7", KEY_RIGHT},
	{"DOWN"           , "F7", KEY_DOWN},
	{"LEFT"           , "57", KEY_LEFT},
	{"0BUTTON"        , "8D", KEY_0},
	{"1BUTTON"        , "7D", KEY_1},
	{"2BUTTON"        , "BD", KEY_2},
	{"3BUTTON"        , "3D", KEY_3},
	{"4BUTTON"        , "9D", KEY_4},
	{"5BUTTON"        , "5D", KEY_5},
	{"6BUTTON"        , "DD", KEY_6},
	{"7BUTTON"        , "87", KEY_7},
	{"8BUTTON"        , "67", KEY_8},
	{"9BUTTON"        , "E7", KEY_9},
	{"TV_RADIO"       , "C7", KEY_LANGUAGE},
	{"TV_STB"         , "47", KEY_ZOOM},
	{""               , ""  , KEY_NULL},
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un  vAddr;
//static int                 vPanel;

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	vAddr.sun_family = AF_UNIX;
	strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
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
	char                vBuffer[128];
	char                vData[10];
	const int           cSize         = 128;
	int                 vCurrentCode  = -1;

	//wait for new command
	read(context->fd, vBuffer, cSize);

	//parse and send key event
	vData[0] = vBuffer[17];
	vData[1] = vBuffer[18];
	vData[2] = '\0';

	//prell, we could detect a long press here if we want
	if (atoi(vData) % 3 != 0)
		return -1;

	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';

	vCurrentCode = getInternalCode((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData);

	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	return 0;
}

RemoteControl_t Hs5101_RC =
{
	"Homecast5101 RemoteControl",
	Hs5101,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonsHomecast5101,
	NULL,
	NULL,
	0,
	NULL,
};
