/*
 * Ufs922.c
 *
 * (c) 2009 dagobert@teamducktales
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
#include <linux/input.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include "global.h"
#include "map.h"
#include "remotes.h"
#include "Ufs922.h"

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cUFS922CommandLen 8

typedef struct
{
	int          toggleFeedback;
	int          disableFeedback;
} tUFS922Private;

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120,
};

static tButton cButtonUFS922[] =
{
	{"MEDIA"          , "D5", KEY_MEDIA},
	{"ARCHIVE"        , "46", KEY_ARCHIVE},
	{"MENU"           , "54", KEY_MENU},
	{"RED"            , "6D", KEY_RED},
	{"GREEN"          , "6E", KEY_GREEN},
	{"YELLOW"         , "6F", KEY_YELLOW},
	{"BLUE"           , "70", KEY_BLUE},
	{"EXIT"           , "55", KEY_HOME},
	{"TEXT"           , "3C", KEY_TEXT},
//  {"EPG"            , "4C", KEY_EPG},
	{"EPG"            , "CC", KEY_EPG},
	{"REWIND"         , "21", KEY_REWIND},
	{"FASTFORWARD"    , "20", KEY_FASTFORWARD},
	{"PLAY"           , "38", KEY_PLAY},
	{"PAUSE"          , "39", KEY_PAUSE},
	{"RECORD"         , "37", KEY_RECORD},
	{"STOP"           , "31", KEY_STOP},
	{"STANDBY"        , "0C", KEY_POWER},
	{"MUTE"           , "0D", KEY_MUTE},
	{"CHANNELUP"      , "1E", KEY_PAGEUP},
	{"CHANNELDOWN"    , "1F", KEY_PAGEDOWN},
	{"VOLUMEUP"       , "10", KEY_VOLUMEUP},
	{"VOLUMEDOWN"     , "11", KEY_VOLUMEDOWN},
	{"INFO"           , "0F", KEY_INFO},
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
	{""               , ""  , KEY_NULL},
};

/* ***************** our fp button assignment **************** */

static tButton cButtonUFS922Frontpanel[] =
{
	{"FP_MENU"		, "80", KEY_MENU},
	{"FP_EXIT"		, "0D", KEY_HOME},
	/*	{"FP_AUX"		, 0x20, ???},
		{"FP_TV_R"		, 0x08, ???},
	*/
	{"FP_OK"		, "04", KEY_OK},
	{"FP_WHEEL_LEFT"	, "0F", KEY_UP},
	{"FP_WHEEL_RIGHT"	, "0E", KEY_DOWN},
	{""	                , ""  , KEY_NULL}
	/* is there no power key on frontpanel? */
};

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vFd;
	tUFS922Private *private = malloc(sizeof(tUFS922Private));

	((RemoteControl_t *)context->r)->private = private;

	vFd = open(rcDeviceName, O_RDWR);

	memset(private, 0, sizeof(tUFS922Private));

	if (argc >= 2)
		private->toggleFeedback = atoi(argv[1]);
	else
		private->toggleFeedback = 0;

	if (argc >= 3)
		private->disableFeedback = atoi(argv[2]);
	else
		private->disableFeedback = 0;

	printf("toggle %d, disable %d\n", private->toggleFeedback, private->disableFeedback);

	if (argc >= 4)
	{
		cLongKeyPressSupport.period = atoi(argv[3]);
	}

	if (argc >= 5)
	{
		cLongKeyPressSupport.delay = atoi(argv[4]);
	}

	printf("period %d, delay %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);

	if (private->toggleFeedback)
	{
		struct micom_ioctl_data vfd_data;
		int ioctl_fd = open("/dev/vfd", O_RDONLY);

		vfd_data.u.led.led_nr = 6;
		vfd_data.u.led.on = 1;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		close(ioctl_fd);
	}

	return vFd;
}


static int pRead(Context_t *context)
{
	unsigned char   vData[cUFS922CommandLen];
	eKeyType        vKeyType = RemoteControl;
	int             vCurrentCode = -1;

	/*    printf("%s >\n", __func__); */

	while (1)
	{
		int i;
		int n = read(context->fd, vData, cUFS922CommandLen);

		if (vData[0] == 0xD2)
			vKeyType = RemoteControl;
		else if (vData[0] == 0xD1)
			vKeyType = FrontPanel;
		else
			continue;

		if (vKeyType == RemoteControl)
		{
			vCurrentCode = getInternalCodeHex((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData[1]);
		}
		else
		{
			vCurrentCode = getInternalCodeHex((tButton *)((RemoteControl_t *)context->r)->Frontpanel, vData[1]);
		}

		if (vCurrentCode != 0)
		{
			unsigned int vNextKey = vData[4];
			vCurrentCode += (vNextKey << 16);
			break;
		}
	}

	/*    printf("%s <\n", __func__);*/

	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	int 	            ioctl_fd = -1;
	struct micom_ioctl_data vfd_data;
	tUFS922Private         *private = (tUFS922Private *)((RemoteControl_t *)context->r)->private;

	if (private->disableFeedback)
		return 0;

	if (cOn)
	{
		ioctl_fd = open("/dev/vfd", O_RDONLY);
		vfd_data.u.led.led_nr = 6;
		vfd_data.u.led.on = !private->toggleFeedback;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		close(ioctl_fd);
	}
	else
	{
		usleep(100000);
		ioctl_fd = open("/dev/vfd", O_RDONLY);
		vfd_data.u.led.led_nr = 6;
		vfd_data.u.led.on = private->toggleFeedback;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		close(ioctl_fd);
	}

	return 0;
}

static int pShutdown(Context_t *context)
{
	tUFS922Private         *private = (tUFS922Private *)((RemoteControl_t *)context->r)->private;

	close(context->fd);

	if (private->toggleFeedback)
	{
		struct micom_ioctl_data vfd_data;
		int ioctl_fd = open("/dev/vfd", O_RDONLY);

		vfd_data.u.led.led_nr = 6;
		vfd_data.u.led.on = 0;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		close(ioctl_fd);
	}

	free(private);

	return 0;
}

RemoteControl_t UFS922_RC =
{
	"Kathrein UFS922 RemoteControl",
	Ufs922,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonUFS922,
	cButtonUFS922Frontpanel,
	NULL,
	1,
	&cLongKeyPressSupport,
};
