/*
 * Fortis.c
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
#include <linux/input.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include "global.h"
#include "map.h"
#include "remotes.h"
#include "Fortis.h"

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cFortisDataLen    128

typedef struct
{
} tFortisPrivate;

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 140,
};

static tButton cButtonFortis[] =
{
	{"MENU"           , "04", KEY_MENU},
	{"RED"            , "4B", KEY_RED},
	{"GREEN"          , "4A", KEY_GREEN},
	{"YELLOW"         , "49", KEY_YELLOW},
	{"BLUE"           , "48", KEY_BLUE},
	{"EXIT"           , "1C", KEY_HOME},
	{"TEXT"           , "0D", KEY_TEXT},
	{"EPG"            , "08", KEY_EPG},
	{"REWIND"         , "58", KEY_REWIND},
	{"FASTFORWARD"    , "5C", KEY_FASTFORWARD},
	{"PLAY"           , "55", KEY_PLAY},
	{"PAUSE"          , "07", KEY_PAUSE},
	{"RECORD"         , "56", KEY_RECORD},
	{"STOP"           , "54", KEY_STOP},
	{"STANDBY"        , "0A", KEY_POWER},
	{"MUTE"           , "0C", KEY_MUTE},
	{"CHANNELUP"      , "5E", KEY_PAGEUP},
	{"CHANNELDOWN"    , "5F", KEY_PAGEDOWN},
	{"VOLUMEUP"       , "4E", KEY_VOLUMEUP},
	{"VOLUMEDOWN"     , "4F", KEY_VOLUMEDOWN},
	{"INFO"           , "06", KEY_INFO},
	{"OK"             , "1F", KEY_OK},
	{"UP"             , "00", KEY_UP},
	{"RIGHT"          , "02", KEY_RIGHT},
	{"DOWN"           , "01", KEY_DOWN},
	{"LEFT"           , "03", KEY_LEFT},
	{"RECALL"         , "09", KEY_HELP},
	{"ZOOM"           , "0B", KEY_ZOOM},
	{"VFORMAT"        , "0E", KEY_ANGLE},
	{"RESOLUTION"     , "0F", KEY_SCREEN},
	{"TVRADIO"        , "1A", KEY_MODE},
	{"SLEEP"          , "1E", KEY_SLOW},
	{"OPEN"           , "40", KEY_DIRECTORY},
	{"FAV"            , "41", KEY_FAVORITES},
	{"CHECK"          , "42", KEY_SELECT},
	{"UPUP"           , "43", KEY_TEEN},
	{"DOWNDOWN"       , "44", KEY_TWEN},
	{"NEXT"           , "4C", KEY_NEXT},
	{"LAST"           , "50", KEY_PREVIOUS},
	{"PIP"            , "51", KEY_OPTION},
	{"SWAP"           , "52", KEY_GOTO},
	{"LIST"           , "53", KEY_SUBTITLE},
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
	{""               , ""  , KEY_NULL}
};

/* ***************** our fp button assignment **************** */

static tButton cButtonFortisFrontpanel[] =
{
	{"STANDBY"        , "00", KEY_POWER},
	{"OK"             , "06", KEY_OK},
	{"MENU"           , "05", KEY_MENU},
	{"VOLUMEUP"       , "03", KEY_VOLUMEUP},
	{"VOLUMEDOWN"     , "04", KEY_VOLUMEDOWN},
	{"CHANNELUP"      , "01", KEY_PAGEUP},
	{"CHANNELDOWN"    , "02", KEY_PAGEDOWN},
	{""               , ""  , KEY_NULL}
};

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vFd;
	tFortisPrivate *private = malloc(sizeof(tFortisPrivate));

	((RemoteControl_t *)context->r)->private = private;

	memset(private, 0, sizeof(tFortisPrivate));

	vFd = open(rcDeviceName, O_RDWR);

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


static int pRead(Context_t *context)
{
	unsigned char   vData[cFortisDataLen];
	eKeyType        vKeyType = RemoteControl;
	int             vCurrentCode = -1;
	static   int    vNextKey = 0;
	static   char   vOldButton = 0;

	while (1)
	{
#if 0
		int vLoop;
		int n =
#endif
			read(context->fd, vData, cFortisDataLen);

#if 0
		printf("(len %d): ", n);

		for (vLoop = 0; vLoop < n; vLoop++)
			printf("0x%02X ", vData[vLoop]);

		printf("\n");
#endif

		if ((vData[2] != 0x51) && (vData[2] != 0x63) && (vData[2] != 0x80))
			continue;

		if (vData[2] == 0x63)
			vKeyType = RemoteControl;
		else if (vData[2] == 0x51)
			vKeyType = FrontPanel;
		else
			continue;

		if (vKeyType == RemoteControl)
		{
			vCurrentCode = getInternalCodeHex((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData[5] & ~0x80);

			if (vCurrentCode != 0)
			{
				vNextKey = (vData[5] & (0x80 == 0 ? vNextKey + 1 : vNextKey)) % 0x100;

				/* printf("nextFlag %d\n", vNextKey);*/

				vCurrentCode += (vNextKey << 16);
				break;
			}
		}
		else
		{
			vCurrentCode = getInternalCodeHex((tButton *)((RemoteControl_t *)context->r)->Frontpanel, vData[3]);

			if (vCurrentCode != 0)
			{
				vNextKey = (vOldButton != vData[3] ? vNextKey + 1 : vNextKey) % 0x100;

				/* printf("nextFlag %d\n", vNextKey);*/

				vCurrentCode += (vNextKey << 16);
				break;
			}
		}
	} /* for later use we make a dummy while loop here */

	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	/* noop: is handled from fp itself */
	return 0;
}

static int pShutdown(Context_t *context)
{
	tFortisPrivate *private = (tFortisPrivate *)((RemoteControl_t *)context->r)->private;

	close(context->fd);
	free(private);

	return 0;
}

RemoteControl_t Fortis_RC =
{
	"Fortis RemoteControl",
	Fortis,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonFortis,
	cButtonFortisFrontpanel,
	NULL,
	1,
	&cLongKeyPressSupport,
};
