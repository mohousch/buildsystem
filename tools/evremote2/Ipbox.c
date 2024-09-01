/*
 * Ipbox.c based on Hl101.c
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
#include "Ipbox.h"

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120,
};

/* Spider Box HL-101 RCU */
static tButton cButtonsSpideroxIpbox[] =
{
	{"POWER  "        , "ff", KEY_POWER},
	{"MUTE"           , "fd", KEY_MUTE},
	{"TV"             , "0f", KEY_TV},
	{"RADIO"          , "ad", KEY_RADIO},
	{"VIDEO"          , "f9", KEY_VIDEO},


	{"0BUTTON"        , "8f", KEY_0},
	{"1BUTTON"        , "3f", KEY_1},
	{"2BUTTON"        , "bf", KEY_2},
	{"3BUTTON"        , "7f", KEY_3},
	{"4BUTTON"        , "1f", KEY_4},
	{"5BUTTON"        , "9f", KEY_5},
	{"6BUTTON"        , "5f", KEY_6},
	{"7BUTTON"        , "2f", KEY_7},
	{"8BUTTON"        , "af", KEY_8},
	{"9BUTTON"        , "6f", KEY_9},


	{"INFO"           , "47", KEY_INFO},
	{"AUDIO"          , "29", KEY_AUDIO},
	{"SUBTITLE"       , "a9", KEY_SUBTITLE},
	{"TEXT"           , "d7", KEY_TEXT},
	{"PROG1"          , "e9", KEY_PROG1},
	{"PROG2"          , "b7", KEY_PROG2},
	{"ZOOM"           , "69", KEY_ZOOM},
	{"DIRECTION"      , "6d", KEY_DIRECTION},
	{"EMAIL"          , "97", KEY_EMAIL},
	{"HELP"           , "87", KEY_HELP},
	{"UNDO"           , "07", KEY_UNDO},

	{"DOWN"           , "27", KEY_DOWN},
	{"UP"             , "a7", KEY_UP},
	{"RIGHT"          , "4f", KEY_RIGHT},
	{"LEFT"           , "77", KEY_LEFT},
	{"OK"             , "ef", KEY_OK},
	{"MENU"           , "67", KEY_MENU},
	{"EPG"            , "79", KEY_EPG},
	{"EXIT"           , "df", KEY_HOME},
	{"PAGEUP"         , "39", KEY_PAGEUP},
	{"PAGEDOWN"       , "b9", KEY_PAGEDOWN},
	{"CHANNELUP"      , "f7", KEY_CHANNELUP},
	{"CHANNELDOWN"    , "59", KEY_CHANNELDOWN},
	{"VOLUMEUP"       , "99", KEY_VOLUMEUP},
	{"VOLUMEDOWN"     , "19", KEY_VOLUMEDOWN},

	{"RED"            , "f5", KEY_RED},
	{"GREEN"          , "d5", KEY_GREEN},
	{"YELLOW"         , "7d", KEY_YELLOW},
	{"BLUE"           , "75", KEY_BLUE},


	{"REWIND"         , "d9", KEY_REWIND},
	{"PAUSE"          , "b5", KEY_PAUSE},
	{"PLAY"           , "17", KEY_PLAY},
	{"FASTFORWARD"    , "89", KEY_FASTFORWARD},
	{"RECORD"         , "35", KEY_RECORD},
	{"STOP"           , "c7", KEY_STOP},
	{"ANGLE"          , "8d", KEY_ANGLE},
	{"REFRESH"        , "cd", KEY_REFRESH},

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

	struct micom_ioctl_data vfd_data;
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

RemoteControl_t Ipbox_RC =
{
	"Ipbox RemoteControl",
	Ipbox,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonsSpideroxIpbox,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};
