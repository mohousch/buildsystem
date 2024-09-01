/*
 * Hl101.c
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
#include "Hl101.h"

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120,
};

/* Spider Box HL-101 RCU */
static tButton cButtonsSpiderboxHL101[] =
{
	{"0BUTTON"	, "ff", KEY_0},
	{"1BUTTON"	, "7f", KEY_1},
	{"2BUTTON"	, "bf", KEY_2},
	{"3BUTTON"	, "3f", KEY_3},
	{"4BUTTON"	, "df", KEY_4},
	{"5BUTTON"	, "5f", KEY_5},
	{"6BUTTON"	, "9f", KEY_6},
	{"7BUTTON"	, "1f", KEY_7},
	{"8BUTTON"	, "ef", KEY_8},
	{"9BUTTON"	, "6f", KEY_9},
	{"STANDBY"	, "f7", KEY_POWER},
	{"TIMER"	, "b7", KEY_TIME},
	{"UHF"		, "d7", KEY_U},
	{"V.FORMAT"	, "e7", KEY_V},
	{"MUTE"		, "77", KEY_MUTE},
	{"TV/SAT"	, "37", KEY_AUX},
	{"TV/RADIO"	, "2f", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCH BUTTON
	{"FIND"		, "17", KEY_FIND},
	{"FAV"		, "85", KEY_FAVORITES},
	{"MENU"		, "af", KEY_MENU},
	{"INFO"		, "25", KEY_INFO}, //THIS IS WRONG SHOULD BE KEY_INFO
	{"GUIDE"	, "4f", KEY_EPG},
	{"EXIT"		, "cf", KEY_HOME},
	{"UP/P+"	, "67", KEY_UP},
	{"DOWN/P-"	, "a7", KEY_DOWN},
	{"LEFT/V-"	, "27", KEY_LEFT},
	{"RIGHT/V+"	, "c7", KEY_RIGHT},
	{"OK/LIST"	, "47", KEY_OK},
	{"BACK"		, "0f", KEY_BACK},
	{"RECORD"	, "8f", KEY_RECORD},
	{"PLAY"		, "57", KEY_PLAY},
	{"REWIND"	, "97", KEY_REWIND},
	{"PAUSE"	, "87", KEY_PAUSE},
	{"FASTFORWARD"	, "9d", KEY_FASTFORWARD},
	{"STOP"		, "d5", KEY_STOP},
	{"SLOWMOTION"	, "cd", KEY_SLOW},
	{"STEPBACK"	, "95", KEY_PREVIOUS},
	{"STEPFORWARD"	, "55", KEY_NEXT},
	{"ARCHIVE"	, "15", KEY_ARCHIVE},
	{"ZOOM"		, "e5", KEY_ZOOM},
	{"PLAYMODE"	, "65", KEY_PLAYER},
	{"USB"		, "a5", KEY_CLOSE},
	{"AUDIO"	, "35", KEY_AUDIO},
	{"SAT"		, "b5", KEY_SAT},
	{"F1"		, "07", KEY_F1},
	{"F2"		, "2d", KEY_F2},
	{"RED"		, "3d", KEY_RED},
	{"GREEN"	, "fd", KEY_GREEN},
	{"YELLOW"	, "6d", KEY_YELLOW},
	{"BLUE"		, "8d", KEY_BLUE},
	{""		, ""  , KEY_NULL},
};
/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un  vAddr;

static int pInit(Context_t *context, int argc, char *argv[])
{

	int vHandle;

	vAddr.sun_family = AF_UNIX;
	// in new lircd its moved to /var/run/lirc/lircd by default and need use key to run as old version

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

	struct proton_ioctl_data vfd_data;
	int ioctl_fd = -1;

	if (cOn)
	{
		vfd_data.u.icon.on = 1;
	}
	else
	{
		usleep(100000);
		vfd_data.u.icon.on = 0;
	}

	ioctl_fd = open("/dev/vfd", O_RDONLY);
	vfd_data.u.icon.icon_nr = 35;
	ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
	close(ioctl_fd);
	return 0;
}

RemoteControl_t Hl101_RC =
{
	"Hl101 RemoteControl",
	Hl101,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonsSpiderboxHL101,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};
