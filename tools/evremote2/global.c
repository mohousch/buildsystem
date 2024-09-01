/*
 * global.c
 *
 * (c) 2009 donald@teamducktales
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

#define DEVICENAME "TDT RC event driver"
char eventPath[] = "/dev/input/event0";

//Checks with event device is created by simubutton.ko
int getEventDevice()
{
	int     vFd             = -1;
	int     vIterator       = 0;
	bool    vDeviceFound    = false;
	char    vName[256]      = "Unknown";

	while (vIterator < 10 && vDeviceFound == false)
	{
		eventPath[16] = vIterator + 48;
		vIterator++;
		vFd = open(eventPath, O_RDONLY);

		if (vFd <= 0)
			continue;

		ioctl(vFd, EVIOCGNAME(sizeof(vName)), vName);
		printf("Input device name: \"%s\"\n", vName);

		if (!strcmp(DEVICENAME, vName))
			vDeviceFound = true;

		close(vFd);
	}

	return vDeviceFound;
}

// Translates internal remote control hex value to known linux input key value
int getInternalCode(tButton *cButtons, const char cCode[3])
{
	int         vLoop       = 0;

	for (vLoop = 0; cButtons[vLoop].KeyCode != KEY_NULL; vLoop++)
	{
		//printf("%20s - %2s - %3d\n", cButtons[vLoop].KeyName, cButtons[vLoop].KeyWord, cButtons[vLoop].KeyCode);
		if ((cButtons[vLoop].KeyWord[0] == cCode[0] || cButtons[vLoop].KeyWord[0] == (cCode[0] - 32)) &&
				(cButtons[vLoop].KeyWord[1] == cCode[1] || cButtons[vLoop].KeyWord[1] == (cCode[1] - 32)))
		{

			printf("KEY by code: %02X - %s\n", cButtons[vLoop].KeyCode, cButtons[vLoop].KeyName);
			return cButtons[vLoop].KeyCode;
		}
	}

	return 0;
}

// Translates lirc key name to known linux input key value
int getInternalCodeLircKeyName(tButton *cButtons, const char cCode[30])
{
	int vLoop = 0;

	for (vLoop = 0; cButtons[vLoop].KeyCode != KEY_NULL; vLoop++)
	{
		//printf("%20s - %2s - %3d\n", cButtons[vLoop].KeyName, cButtons[vLoop].KeyWord, cButtons[vLoop].KeyCode);
		if (strcmp(cCode, cButtons[vLoop].KeyName) == 0)
		{
			printf("KEY by name: %02X - %s\n", cButtons[vLoop].KeyCode, cButtons[vLoop].KeyName);
			return cButtons[vLoop].KeyCode;
		}
	}
	return 0;
}

int printKeyMap(tButton *cButtons)
{
	int vLoop = 0;

	for (vLoop = 0; cButtons[vLoop].KeyCode != KEY_NULL; vLoop++)
	{
		printf("%20s - %2s - %3d\n", cButtons[vLoop].KeyName, cButtons[vLoop].KeyWord, cButtons[vLoop].KeyCode);
	}
	return 0;
}

// Translates internal remote control hex value to known linux input key value
int getInternalCodeHex(tButton *cButtons, const unsigned char cCode)
{
	char vCode[3];

	sprintf(vCode, "%.2hhx", cCode);
	vCode[2] = '\0';

	return getInternalCode(cButtons, vCode);
}

static int tuxtxt_exit_count = 0;
static int sSockethandle = -1;
#define SocketName "/tmp/rc.socket"

int checkTuxTxt(const int cCode)
{
	int vTmpO;

	if ((vTmpO = open("/tmp/block.tmp", O_RDONLY)) >= 0)
	{
		close(vTmpO);

		//workaround if tuxtxt hangs
		if (cCode == KEY_HOME) //EXIT
		{
			tuxtxt_exit_count++;

			if (tuxtxt_exit_count > 1)
			{
				tuxtxt_exit_count = 0;
				system("killall tuxtxt; sleep 3; killall -9 tuxtxt; rm -f /tmp/block.tmp");
			}
		}

		fprintf(stderr, "Forwarding to Socket-> %u\n", sSockethandle);

		if (sSockethandle <= 0)
		{
			struct sockaddr_un vAddr;
			vAddr.sun_family = AF_UNIX;
			strcpy(vAddr.sun_path, SocketName);
			sSockethandle = socket(PF_UNIX, SOCK_STREAM, 0);

			if (sSockethandle <= 0)
			{
				fprintf(stderr, "No RemoteControlSocket attached!\n");
				return 0;
			};

			if (connect(sSockethandle, (struct sockaddr *)&vAddr, sizeof(vAddr)) != 0)
			{
				close(sSockethandle);
				sSockethandle = -1;
				fprintf(stderr, "connect failed!\n");
				return 0;
			}
		}

		if (sSockethandle > 0)
		{
			char *vTmpS = (char *) malloc(sizeof("00000000"));
			sprintf(vTmpS, "%08d", cCode);

			if (write(sSockethandle, (void *) vTmpS, sizeof("00000000")) <= 0)
				fprintf(stderr, "Error while forwarding!\n");
			free(vTmpS);
		}
		else
			fprintf(stderr, "Error while forwarding!\n");

		return 1;
	}

	tuxtxt_exit_count = 0;

	if (sSockethandle != -1)
	{
		close(sSockethandle);
		sSockethandle = -1;
	}

	return 0;
}

void sendInputEvent(const int cCode)
{
	sendInputEventT(INPUT_PRESS, cCode);
	sendInputEventT(INPUT_RELEASE, cCode);
}

void sendInputEventT(const unsigned int type, const int cCode)
{
	int                 vFd    = -1;
	struct input_event  vInev;
	int                 cSize  = sizeof(struct input_event);

	gettimeofday(&vInev.time, NULL);
	vInev.type             = 1;
	vInev.code             = cCode;

	vFd = open(eventPath, O_WRONLY);

	vInev.value             = type;
	write(vFd, &vInev, cSize);

	close(vFd);
}

void setInputEventRepeatRate(unsigned int delay, unsigned int period)
{
	int                 vFd    = -1;
	struct input_event  vInev;

	vFd = open(eventPath, O_WRONLY);

	vInev.type = EV_REP;
	vInev.code = REP_DELAY;
	vInev.value = delay;

	if (write(vFd, &vInev, sizeof(vInev)) == -1)
		perror("REP_DELAY");

	vInev.code = REP_PERIOD;
	vInev.value = period;

	if (write(vFd, &vInev, sizeof(vInev)) == -1)
		perror("REP_PERIOD");

	close(vFd);
}
