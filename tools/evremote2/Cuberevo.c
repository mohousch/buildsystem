/*
 * Cuberevo.c
 *
 * (c) 2011 konfetti
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
#include <sys/ioctl.h>

#include "global.h"
#include "map.h"
#include "remotes.h"
#include "Cuberevo.h"

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cCuberevoCommandLen 2

/* ***************** our key assignment **************** */

static int version = 0;

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 140,
};

static tButton cButtonCuberevo[] =
{
	{"KEY_F1"         , "00", KEY_F1},
	{"KEY_F2"         , "01", KEY_F2},
	{"KEY_F3"         , "02", KEY_F3},
	{"INFO"           , "03", KEY_INFO},
	{"RADIO"          , "04", KEY_RADIO},
	{"PREVIOUS"       , "05", KEY_PREVIOUS},
	{"EPG"            , "06", KEY_EPG},
	{"RECORD"         , "07", KEY_RECORD},
	{"FAVORITES"      , "08", KEY_FAVORITES},
	{"MEDIA"          , "09", KEY_MEDIA},   //fixme
	{"STANDBY"        , "0A", KEY_POWER},
	{"KEY_F5"         , "0B", KEY_F5},
	{"MUTE"           , "0C", KEY_MUTE},
	{"ARCHIVE"        , "0D", KEY_ARCHIVE}, //fixme
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
	{"UP"             , "1A", KEY_UP},
	{"DOWN"           , "1B", KEY_DOWN},
	{"RIGHT"          , "1C", KEY_RIGHT},
	{"LEFT"           , "1D", KEY_LEFT},
	{"REWIND"         , "1E", KEY_REWIND},
	{"OK"             , "1F", KEY_OK},
	{"PLAY"           , "20", KEY_PLAY},
	{"FASTFORWARD"    , "21", KEY_FASTFORWARD},
	{"STOP"           , "22", KEY_STOP},
	{"SLOW"           , "23", KEY_SLOW},
	{"AGAIN"          , "24", KEY_AGAIN},
	{"PAUSE"          , "25", KEY_PAUSE},
	{"MENU"           , "26", KEY_MENU},
	{"EXIT"           , "27", KEY_HOME},
	{"KEY_F7"         , "28", KEY_F7},
	{"KEY_BOOMARKS"   , "29", KEY_BOOKMARKS},
	{"RED"            , "2A", KEY_RED},
	{"BLUE"           , "2B", KEY_BLUE},
	{"KEY_F8"         , "2C", KEY_F8},
	{"GREEN"          , "2D", KEY_GREEN},
	{"YELLOW"         , "2E", KEY_YELLOW},
	{"AUDIO"          , "2F", KEY_AUDIO},
	{"SUBTITLE"       , "30", KEY_SUBTITLE},
	{"TEXT"           , "31", KEY_TEXT},
	{"CHANNELUP"      , "32", KEY_CHANNELUP},
	{"CHANNELDOWN"    , "33", KEY_CHANNELDOWN},
	{"VOLUMEUP"       , "34", KEY_VOLUMEUP},
	{"VOLUMEDOWN"     , "35", KEY_VOLUMEDOWN},
	{"WWW"            , "36", KEY_WWW},
	{""               , ""  , KEY_NULL},
};

/* ***************** our fp button assignment **************** */

typedef struct
{
	char          *KeyName;
	unsigned short code;
	int            KeyCode;
} key_table_t;

key_table_t front_keymap_13grid[] =
{
	{ "STANDBY", 0x1000,              KEY_POWER         },  /* front power */
	{ "LEFT"   , 0x0002,              KEY_LEFT          },  /* front left */
	{ "RIGHT"  , 0x0004,              KEY_RIGHT         },  /* front right */
	{ "UP"     , 0x4000,              KEY_UP            },  /* front up */
	{ "FILE"   , 0x2000,              KEY_MEDIA         },  /* front file */
	{ "DOWN"   , 0x0040,              KEY_DOWN          },  /* front down */
	{ "OK"     , 0x0020,              KEY_OK            },  /* front ok */
	{ "HOME"   , 0x0010,              KEY_HOME          },  /* front back */
	{ "MENU"   , 0x0001,              KEY_MENU          },  /* front menu */
	{ "RELEASE", 0xFFFF,              KEY_NULL          },  /* release */
	{ ""       , 0x0000,              KEY_NULL          },
};

key_table_t front_keymap_7seg[] =
{
	{ "STANDBY"  , 0x0001,              KEY_POWER         },  /* front power */
	{ "MENU"     , 0x0002,              KEY_MENU          },  /* front menu  */
	{ "EXIT"     , 0x0004,              KEY_HOME          },  /* front exit  */
	{ "OK"       , 0x0010,              KEY_OK            },  /* front ok    */
	{ "LEFT"     , 0x0020,              KEY_LEFT          },  /* front left  */
	{ "RIGHT"    , 0x0040,              KEY_RIGHT         },  /* front right */
	{ "UP"       , 0x0080,              KEY_UP            },  /* front up    */
	{ "DOWN"     , 0x0100,              KEY_DOWN          },  /* front down  */
	{ ""         , 0x0000,              KEY_NULL          },
};

key_table_t front_keymap_12dotmatrix[] =
{
	{ "STANDBY", (1 << 0),        KEY_POWER         }, /* front power */
	{ "MENU"   , (1 << 1),        KEY_MENU          }, /* front menu */
	{ "HOME"   , (1 << 2),        KEY_HOME          }, /* front back */
	{ "FILE"   , (1 << 3),        KEY_MEDIA         }, /* front file */
	{ "OK"     , (1 << 4),        KEY_OK            }, /* front ok */
	{ "LEFT"   , (1 << 5),        KEY_LEFT          }, /* front left */
	{ "RIGHT"  , (1 << 6),        KEY_RIGHT         }, /* front right */
	{ "UP"     , (1 << 7),        KEY_UP            }, /* front up */
	{ "DOWN"   , (1 << 8),        KEY_DOWN          }, /* front down */
	{ "RELEASE", 0xFFFF,        KEY_NULL          },  /* front release */
	{ ""       , 0x0000,        KEY_NULL          },
};

int getCuberevoCode(key_table_t *cKeys, unsigned short code)
{
	int vLoop = 0;

	for (vLoop = 0; cKeys[vLoop].KeyCode != KEY_NULL; vLoop++)
	{
		if (cKeys[vLoop].code == code)
		{
			printf("%02X - %s\n", cKeys[vLoop].KeyCode, cKeys[vLoop].KeyName);
			return cKeys[vLoop].KeyCode;
		}
	}

	return 0;
}

static int pInit(Context_t *context, int argc, char *argv[])
{
	struct micom_ioctl_data micom;
	int vFd;
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

	if (ioctl(vFd, VFDGETVERSION, &micom) < 0)
	{
		perror("getVersion: ");
	}

	printf("micom version = %d\n", micom.u.version.version);
	version = micom.u.version.version;

	return vFd;
}


static int pRead(Context_t *context)
{
	unsigned char   vData[cCuberevoCommandLen];
	eKeyType        vKeyType = RemoteControl;
	static   int    vNextKey = 0;
	static   int    lastCode = 0;
	int             vCurrentCode = -1;

	//printf("%s >\n", __func__);

	while (1)
	{
		read(context->fd, vData, cCuberevoCommandLen);

		if (vData[0] == 0xe0)
			vKeyType = RemoteControl;
		else if (vData[0] == 0xE1)
			vKeyType = FrontPanel;
		else if (vData[0] == 0xE2)
			vKeyType = FrontPanel;
		else
			continue;

		printf("data[0] 0x%2x, data[1] 0x%2x\n", vData[0], vData[1]);

		if (vKeyType == RemoteControl)
		{
			if (vData[1] != 0xff)
			{
				vCurrentCode =
					getInternalCodeHex((tButton *)((RemoteControl_t *)context->r)->RemoteControl, vData[1]);
			}
			else
			{
				vCurrentCode = lastCode = 0;
			}

			if (vCurrentCode != 0)
			{
				vNextKey = ((vCurrentCode != lastCode) ? vNextKey + 1 : vNextKey) % 0x100;
				lastCode = vCurrentCode;

//               		printf("nextFlag %d\n", vNextKey);

				vCurrentCode += (vNextKey << 16);

				break;
			}
		}
		else
		{
			static int front_key;

			if (vData[0] == 0xE1)
			{
				front_key = vData[1] << 8;
				vCurrentCode = 0;
			}
			else
			{
				front_key |= vData[1];

//				printf("front_key = 0x%04x\n", front_key);

				/* 12 dot, 12 and 14 segs */
				if ((version == 0) || (version == 2))
					vCurrentCode = getCuberevoCode(front_keymap_12dotmatrix, front_key);
				else if (version == 3)
					vCurrentCode = getCuberevoCode(front_keymap_7seg, front_key);
				else
					vCurrentCode = getCuberevoCode(front_keymap_13grid, front_key);

				if (vCurrentCode != 0)
				{
					vNextKey = ((vCurrentCode != lastCode) ? vNextKey + 1 : vNextKey) % 0x100;
					lastCode = vCurrentCode;

					//printf("nextFlag %d\n", vNextKey);

					vCurrentCode += (vNextKey << 16);

					break;
				}

//               		if (vCurrentCode == 0xffff) /* key release */
//                  			vCurrentCode = 0x0000;
			}
		}
	}

	printf("%s < %08X\n", __func__, vCurrentCode);

	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	return 0;
}

static int pShutdown(Context_t *context)
{

	close(context->fd);

	return 0;
}

RemoteControl_t Cuberevo_RC =
{
	"Cuberevo RemoteControl",
	Cuberevo,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonCuberevo,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};

