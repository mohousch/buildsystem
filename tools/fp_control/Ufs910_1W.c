/*
 * Ufs910_1W.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/******************** includes ************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <limits.h>
#include <sys/ioctl.h>

#include "global.h"

static int setText(Context_t *context, char *theText);

/******************** constants ************************ */
#define cTTY_DEVICE "/dev/ttyAS1"
#define cVFD_DEVICE "/dev/vfd"

#define cMAXCharsUFS910 16

//#define cmdReboot "/sbin/reboot" /* does not currently work */
#define cmdReboot "init 6"

typedef struct
{
	int vfd;

	int display;
	int display_custom;
	char *timeFormat;
	time_t wakeupTime;
	int wakeupDecrement;
} tUFS910Private;

typedef struct
{
	char *ledOn;
	char *ledOff;
} tUFS910Leds;

#define cGreen  1
#define cRed    2
#define cYellow 3

tUFS910Leds led[] =
{
	{/* cGreen ,*/ "1", "A" },
	{/* cRed   ,*/ "2", "B" },
	{/* cYellow,*/ "3", "C" }
};

/* ******************* helper/misc functions ****************** */

static int setTemFlagsKathrein(int fd)
{
	struct termios old_io;
	struct termios new_io;

	if ((tcgetattr(fd, &old_io)) == 0)
	{
		new_io = old_io;
		printf("Setting new flags\n");
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
		printf("Error setting raw mode.\n");

	return 0;
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	int vFd;
	tUFS910Private *private = malloc(sizeof(tUFS910Private));
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tUFS910Private));
	vFd = open(cTTY_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "cannot open %s\n", cTTY_DEVICE);
		perror("");
	}

	setTemFlagsKathrein(vFd);
	private->vfd = open(cVFD_DEVICE, O_RDWR);

	if (private->vfd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}

	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement);
	return vFd;
}

static int usage(Context_t *context, char *prg_name)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int setTimer(Context_t *context, time_t *theGMTTime)
{
	time_t curTime;
	time_t wakeupTime;
	struct tm *ts;
	unsigned long int diffTm;
	unsigned char uTime0, uTime1, uTime2, uTime3;
	char cTime[6];
//	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	time(&curTime);
	ts = localtime(&curTime);
	fprintf(stderr, "Current Time: %02d:%02d:%02d %02d-%02d-%04d\n",
		ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);

	if (theGMTTime == NULL)
		wakeupTime = read_timers_utc(curTime);
	else
		wakeupTime = *theGMTTime;

	if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX))
		wakeupTime = read_fake_timer_utc(curTime);

	if (curTime > wakeupTime)
	{
		printf("System time wrong -> Reboot\n");
		diffTm = 5;
	}
	else
		diffTm = (unsigned long int) wakeupTime - curTime;

	printf("Time difference: %ld\n", diffTm);
	uTime0 = diffTm % 256;
	uTime1 = (diffTm / 256) % 256;
	uTime2 = ((diffTm / 256) / 256) % 256;
	uTime3 = (((diffTm / 256) / 256) / 256) % 256;
	printf("%03d %03d %03d %03d\n", uTime3, uTime2, uTime1, uTime0);
	cTime[0] = 'Q';
	cTime[1] = uTime3;
	cTime[2] = uTime2;
	cTime[3] = uTime1;
	cTime[4] = uTime0;

	printf("Goodbye\n");

	sleep(1);

	/* SWITCH ON RED LED */
	write(context->fd, "2" , 1);
	usleep(1000);
	write(context->fd, &cTime[0], 1);
	usleep(1000);
	write(context->fd, &cTime[1], 1);
	usleep(1000);
	write(context->fd, &cTime[2], 1);
	usleep(1000);
	write(context->fd, &cTime[3], 1);
	usleep(1000);
	write(context->fd, &cTime[4], 1);
	return 0;
}

static int getTimer(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int shutdown(Context_t *context, time_t *shutdownTimeGMT)
{
	time_t curTime;

	/* shutdown immediately */
	if (*shutdownTimeGMT == -1)
		return (setTimer(context, NULL));

	while (1)
	{
		time(&curTime);

		if (curTime >= *shutdownTimeGMT)
		{
			/* set most recent e2 timer and bye bye */
			return (setTimer(context, NULL));
		}

		usleep(100000);
	}

	return -1;
}

static int reboot(Context_t *context, time_t *rebootTimeGMT)
{
	time_t curTime;

	while (1)
	{
		time(&curTime);

		if (curTime >= *rebootTimeGMT)
		{
			system(cmdReboot);
		}

		usleep(100000);
	}

	return 0;
}

static int Sleep(Context_t *context, time_t *wakeUpGMT)
{
	time_t curTime;
	int sleep = 1;
	fd_set rfds;
	struct timeval tv;
	int retval;
	struct tm *ts;
	char output[cMAXCharsUFS910 + 1];
	tUFS910Private *private = (tUFS910Private *)
				  ((Model_t *)context->m)->private;

	while (sleep)
	{
		time(&curTime);
		ts = localtime(&curTime);

		if (curTime >= *wakeUpGMT)
		{
			sleep = 0;
		}
		else
		{
			FD_ZERO(&rfds);
			FD_SET(context->fd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			retval = select(context->fd + 1, &rfds, NULL, NULL, &tv);

			if (retval > 0)
			{
				sleep = 0;
			}
		}

		if (private->display)
		{
			strftime(output, cMAXCharsUFS910 + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}

	return 0;
}

static int setText(Context_t *context, char *theText)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)
				  ((Model_t *)context->m)->private;
	memset(data.data, ' ', 63);
	memcpy(data.data, theText, strlen(theText));
	data.start = 0;
	data.length = strlen(theText);

	if (ioctl(private->vfd, VFDDISPLAYCHARS, &data) < 0)
	{
		perror("setText: ");
		return -1;
	}

	return 0;
}

static int setLed(Context_t *context, int which, int on)
{
	if (which < cGreen || which > cYellow)
		return -1;

	if (on)
		write(context->fd, led[which - 1].ledOn, 1);
	else
		write(context->fd, led[which - 1].ledOff, 1);

	return 0;
}

static int setIcon(Context_t *context, int which, int on)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)
				  ((Model_t *)context->m)->private;
	memset(data.data, ' ', 63);
	data.start = 0;
	data.length = 5;
	data.data[0] = which & 0x0f;
	data.data[4] = on;

	if (ioctl(private->vfd, VFDICONDISPLAYONOFF, &data) < 0)
	{
		perror("setIcon: ");
		return -1;
	}

	return 0;
}

static int setBrightness(Context_t *context, int brightness)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)
				  ((Model_t *)context->m)->private;

	if (brightness < 0 || brightness > 7)
		return -1;

	memset(data.data, ' ', 63);
	data.start = brightness & 0x07;
	data.length = 0;

	if (ioctl(private->vfd, VFDBRIGHTNESS, &data) < 0)
	{
		perror("setBrightness: ");
		return -1;
	}

	return 0;
}

static int setPwrLed(Context_t *context, int brightness)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int setLight(Context_t *context, int on)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	memset(&data, 0, sizeof(struct vfd_ioctl_data));

	if (on)
		data.start = 0x01;
	else
		data.start = 0x00;

	data.length = 0;

	if (ioctl(private->vfd, VFDDISPLAYWRITEONOFF, &data) < 0)
	{
		perror("setLight: ");
		return -1;
	}

	return 0;
}

static int Exit(Context_t *context)
{
	tUFS910Private *private = (tUFS910Private *)
				  ((Model_t *)context->m)->private;

	if (private->vfd > 0)
		close(private->vfd);

	if (context->fd > 0)
		close(context->fd);

	free(private);
	return 1;
}

static int Clear(Context_t *context)
{
	int i;
	setText(context, "                ");
	setBrightness(context, 7);

	for (i = 1; i <= 3 ; i++)
		setLed(context, i, 0);

	for (i = 1; i <= 16 ; i++)
		setIcon(context, i, 0);

	return 0;
}

Model_t Ufs910_1W_model =
{
	.Name                      = "Kathrein UFS910 1W frontpanel control utility",
	.Type                      = Ufs910_1W,
	.Init                      = init,
	.Clear                     = Clear,
	.Usage                     = usage,
	.SetTime                   = setTime,
	.GetTime                   = getTime,
	.SetTimer                  = setTimer,
	.GetTimer                  = getTimer,
	.Shutdown                  = shutdown,
	.Reboot                    = reboot,
	.Sleep                     = Sleep,
	.SetText                   = setText,
	.SetLed                    = setLed,
	.SetIcon                   = setIcon,
	.SetBrightness             = setBrightness,
	.SetPwrLed                 = setPwrLed,
	.SetLight                  = setLight,
	.Exit                      = Exit,
	.SetLedBrightness          = NULL,
	.SetRF                     = NULL,
	.SetFan                    = NULL,
	.private                   = NULL
};
