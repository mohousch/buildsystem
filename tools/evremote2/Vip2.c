/*
 * Vip2.c
 *
 * (c) 2010 duckbox project
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
#include "Vip2.h"

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120,
};

/* Edision argus-vip2 RCU */
static tButton cButtonsEdisionVip2[] =
{
	{"STANDBY"        , "f7", KEY_POWER},
	{"MUTE"           , "77", KEY_MUTE},
	{"V.FORMAT"       , "e7", KEY_V},
	{"AUX"            , "37", KEY_AUX},

	{"0BUTTON"        , "ff", KEY_0},
	{"1BUTTON"        , "7f", KEY_1},
	{"2BUTTON"        , "bf", KEY_2},
	{"3BUTTON"        , "3f", KEY_3},
	{"4BUTTON"        , "df", KEY_4},
	{"5BUTTON"        , "5f", KEY_5},
	{"6BUTTON"        , "9f", KEY_6},
	{"7BUTTON"        , "1f", KEY_7},
	{"8BUTTON"        , "ef", KEY_8},
	{"9BUTTON"        , "6f", KEY_9},

	{"BACK"           , "0f", KEY_BACK},
	{"INFO"           , "25", KEY_INFO}, //THIS IS WRONG SHOULD BE KEY_INFO
	{"AUDIO"          , "35", KEY_AUDIO},

	{"DOWN/P-"        , "a7", KEY_DOWN},
	{"UP/P+"          , "67", KEY_UP},
	{"RIGHT/V+"       , "c7", KEY_RIGHT},
	{"LEFT/V-"        , "27", KEY_LEFT},
	{"OK/LIST"        , "47", KEY_OK},
	{"MENU"           , "af", KEY_MENU},
	{"GUIDE"          , "4f", KEY_EPG},
	{"EXIT"           , "cf", KEY_HOME},
	{"FAV"            , "85", KEY_FAVORITES},

	{"RED"            , "3d", KEY_RED},
	{"GREEN"          , "fd", KEY_GREEN},
	{"YELLOW"         , "6d", KEY_YELLOW},
	{"BLUE"           , "8d", KEY_BLUE},

	{"REWIND"         , "65", KEY_REWIND},
	{"PAUSE"          , "87", KEY_PAUSE},
	{"PLAY"           , "57", KEY_PLAY},
	{"FASTFORWARD"    , "9d", KEY_FASTFORWARD},
	{"RECORD"         , "8f", KEY_RECORD},
	{"STOP"           , "d5", KEY_STOP},
	{"SLOWMOTION"     , "97", KEY_SLOW},
	{"ARCHIVE"        , "15", KEY_ARCHIVE},
	{"SAT"            , "b5", KEY_SAT},
	{"STEPBACK"       , "95", KEY_PREVIOUS},
	{"STEPFORWARD"    , "55", KEY_NEXT},
	{"MARK"           , "4f", KEY_EPG},
	{"TV/RADIO"       , "2f", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCH BUTTON
	{"USB"            , "a5", KEY_CLOSE},
	{"TIMER"          , "b7", KEY_TIME},
	{""               , ""  , KEY_NULL},
};
/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un  vAddr;

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

	memset(vBuffer, 0, 128);
	//wait for new command
	read(context->fd, vBuffer, cSize);

	//parse and send key event
	vData[0] = vBuffer[17];
	vData[1] = vBuffer[18];
	vData[2] = '\0';


	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';

	printf("[RCU] key: %s -> %s\n", vData, &vBuffer[0]);
	vCurrentCode = getInternalCode((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData);

	if (vCurrentCode != 0)
	{
		static int nextflag = 0;

		if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
		{
			nextflag++;
		}

		vCurrentCode += (nextflag << 16);
	}

	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{

	struct aotom_ioctl_data vfd_data;
	int ioctl_fd = -1;

	if (cOn)
	{
		ioctl_fd = open("/dev/vfd", O_RDONLY);
		vfd_data.u.icon.icon_nr = 35;
		vfd_data.u.icon.on = 1;
		ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
		close(ioctl_fd);
	}
	else
	{
		usleep(100000);
		ioctl_fd = open("/dev/vfd", O_RDONLY);
		vfd_data.u.icon.icon_nr = 35;
		vfd_data.u.icon.on = 0;
		ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
		close(ioctl_fd);
	}

	return 0;
}

RemoteControl_t Vip2_RC =
{
	"Vip2 RemoteControl",
	Vip2,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonsEdisionVip2,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};
