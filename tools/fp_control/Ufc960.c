/*
 * Ufc960.c
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
#include <limits.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "global.h"
#include "Ufc960.h"

static int setText(Context_t *context, char *theText);
static int Clear(Context_t *context);
static int setIcon(Context_t *context, int which, int on);

/******************** constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
#define cEVENT_DEVICE "/dev/input/event0"

#define WAKEUPFILE "/var/wakeup"
#define WAS_TIMER_WAKEUP "/proc/stb/fp/was_timer_wakeup"

#define cMAXCharsUFC960 16

typedef struct
{
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tUFC960Private;

/* ******************* helper/misc functions ****************** */

static void setMode(int fd)
{
	struct micom_ioctl_data micom;
	micom.u.mode.compat = 1;

	if (ioctl(fd, VFDSETMODE, &micom) < 0)
	{
		perror("setMode: ");
	}
}

/* Calculate the time value which we can pass to
 * the micom fp. its a mjd time (mjd=modified
 * julian date). mjd is relativ to gmt so theGMTTime
 * must be in GMT/UTC.
 */
static void setMicomTime(time_t theGMTTime, char *destString)
{
	/* from u-boot micom */
	struct tm *now_tm;
	now_tm = gmtime(&theGMTTime);
	printf("Set Time (UTC): %02d:%02d:%02d %02d-%02d-%04d\n",
	       now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, now_tm->tm_mday, now_tm->tm_mon + 1, now_tm->tm_year + 1900);
	double mjd = modJulianDate(now_tm);
	int mjd_int = mjd;
	destString[0] = (mjd_int >> 8);
	destString[1] = (mjd_int & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
}

static unsigned long getMicomTime(char *micomTimeString)
{
	unsigned int mjd = ((micomTimeString[1] & 0xFF) * 256) + (micomTimeString[2] & 0xFF);
	unsigned long epoch = ((mjd - 40587) * 86400);
	unsigned int hour = micomTimeString[3] & 0xFF;
	unsigned int min = micomTimeString[4] & 0xFF;
	unsigned int sec = micomTimeString[5] & 0xFF;
	epoch += (hour * 3600 + min * 60 + sec);
	printf("MJD = %d epoch = %ld, time = %02d:%02d:%02d\n", mjd,
	       epoch, hour, min, sec);
	return epoch;
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	tUFC960Private *private = malloc(sizeof(tUFC960Private));
	int vFd;
	printf("%s\n", __func__);
	vFd = open(cVFD_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "cannot open %s\n", cVFD_DEVICE);
		perror("");
	}

	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tUFC960Private));
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
	struct micom_ioctl_data vData;
//	printf("%s\n", __func__);
	setMicomTime(*theGMTTime, vData.u.time.time);

	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("settime: ");
		return -1;
	}

	return 0;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{
	char fp_time[8];
	fprintf(stderr, "Waiting on current time from fp...\n");

	/* front controller time */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("gettime: ");
		return -1;
	}

	/* if we get the fp time */
	if (fp_time[0] != '\0')
	{
		fprintf(stderr, "success reading time from fp\n");
		/* current front controller time */
		*theGMTTime = (time_t) getMicomTime(fp_time);
	}
	else
	{
		fprintf(stderr, "error reading time from fp\n");
		*theGMTTime = 0;
	}

	return 0;
}

static int setTimer(Context_t *context, time_t *theGMTTime)
{
	struct micom_ioctl_data vData;
	time_t curTime = 0;
	time_t curTimeFp = 0;
	time_t wakeupTime = 0;
	struct tm *ts;
	struct tm *tsFp;
	struct tm *tsWakeupTime;
//	tUFC960Private *private = (tUFC960Private *)((Model_t *)context->m)->private;
//	printf("%s ->\n", __func__);
	// Get current Frontpanel time
	getTime(context, &curTimeFp);
	tsFp = gmtime(&curTimeFp);
	fprintf(stderr, "Current Fp Time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
		tsFp->tm_hour, tsFp->tm_min, tsFp->tm_sec,
		tsFp->tm_mday, tsFp->tm_mon + 1, tsFp->tm_year + 1900);
	// Get current Linux time
	time(&curTime);
	ts = gmtime(&curTime);
	fprintf(stderr, "Current Linux Time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
		ts->tm_hour, ts->tm_min, ts->tm_sec,
		ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	// Set current Linux time as new current Frontpanel time
	setTime(context, &curTime);

	if (theGMTTime == NULL)
		wakeupTime = read_timers_utc(curTime);
	else
		wakeupTime = *theGMTTime;

	if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX))
	{
		/* clear timer */
		vData.u.standby.time[0] = '\0';
	}
	else
	{
		// Print wakeup time
		tsWakeupTime = gmtime(&wakeupTime);
		fprintf(stderr, "Planned Wakeup Time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
			tsWakeupTime->tm_hour, tsWakeupTime->tm_min, tsWakeupTime->tm_sec,
			tsWakeupTime->tm_mday, tsWakeupTime->tm_mon + 1, tsWakeupTime->tm_year + 1900);
		setMicomTime(wakeupTime, vData.u.standby.time);
		fprintf(stderr, "Setting Planned Fp Wakeup Time to = %02X%02X %d %d %d (mtime)\n",
			vData.u.standby.time[0], vData.u.standby.time[1], vData.u.standby.time[2],
			vData.u.standby.time[3], vData.u.standby.time[4]);
	}

	fprintf(stderr, "Entering DeepStandby. Goodbye...\n");
	fflush(stdout);
	fflush(stderr);
	sleep(2);

	if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
	{
		perror("standby: ");
		return -1;
	}

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

		/*printf("curTime = %d, shutdown %d\n", curTime, *shutdownTimeGMT);*/

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
	struct micom_ioctl_data vData;

	while (1)
	{
		time(&curTime);

		if (curTime >= *rebootTimeGMT)
		{
			if (ioctl(context->fd, VFDREBOOT, &vData) < 0)
			{
				perror("reboot: ");
				return -1;
			}
		}

		usleep(100000);
	}

	return 0;
}

static int Sleep(Context_t *context, time_t *wakeUpGMT)
{
	time_t curTime;
	int sleep = 1;
	int vFd;
	fd_set rfds;
	struct timeval tv;
	int retval, i, rd;
	struct tm *ts;
	char output[cMAXCharsUFC960 + 1];
	struct input_event ev[64];
	tUFC960Private *private = (tUFC960Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);
	vFd = open(cEVENT_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cEVENT_DEVICE);
		perror("");
		return -1;
	}

//	printf("%s 1\n", __func__);
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
			FD_SET(vFd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			retval = select(vFd + 1, &rfds, NULL, NULL, &tv);

			if (retval > 0)
			{
				rd = read(vFd, ev, sizeof(struct input_event) * 64);

				if (rd < (int) sizeof(struct input_event))
				{
					continue;
				}

				for (i = 0; i < rd / sizeof(struct input_event); i++)
				{
					if (ev[i].type == EV_SYN)
					{
					}
					else if (ev[i].type == EV_MSC && (ev[i].code == MSC_RAW || ev[i].code == MSC_SCAN))
					{
					}
					else
					{
						if (ev[i].code == 116)
							sleep = 0;
					}
				}
			}
		}

		if (private->display)
		{
			strftime(output, cMAXCharsUFC960 + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}

	return 0;
}

static int setText(Context_t *context, char *theText)
{
	char vHelp[128];
	strncpy(vHelp, theText, cMAXCharsUFC960);
	vHelp[cMAXCharsUFC960] = '\0';
	/* printf("%s, %d\n", vHelp, strlen(vHelp));*/
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}

static int setLed(Context_t *context, int which, int on)
{
	struct micom_ioctl_data vData;
	vData.u.led.led_nr = which;
	vData.u.led.on = on;
	setMode(context->fd);

	if (ioctl(context->fd, VFDSETLED, &vData) < 0)
	{
		perror("setLed: ");
		return -1;
	}

	return 0;
}

static int setIcon(Context_t *context, int which, int on)
{
	struct micom_ioctl_data vData;
	vData.u.icon.icon_nr = which;
	vData.u.icon.on = on;
	setMode(context->fd);

	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("setIcon: ");
		return -1;
	}

	return 0;
}

static int setBrightness(Context_t *context, int brightness)
{
	struct micom_ioctl_data vData;

	if (brightness < 0 || brightness > 5)
		return -1;

	vData.u.brightness.level = brightness;
	printf("%d\n", context->fd);
	setMode(context->fd);

	if (ioctl(context->fd, VFDBRIGHTNESS, &vData) < 0)
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
	if (on)
		setBrightness(context, 5);
	else
		setBrightness(context, 0);

	return 0;
}

/* fixme: not sure if this really works for ufc960 ->must be checked */
static int getWakeupReason(Context_t *context, int *reason)
{
	char mode[8];
	fprintf(stderr, "waiting on wakeupmode from fp ...\n");

	/* front controller time */
	if (ioctl(context->fd, VFDGETWAKEUPMODE, &mode) < 0)
	{
		perror("getWakeupReason: ");
		return -1;
	}

	/* if we get the fp time */
	if (mode[0] != '\0')
	{
		fprintf(stderr, "success reading wakeupdmode from fp\n");
		*reason = mode[1] && 0xff;
		printf("reason = 0x%x\n", *reason);
	}
	else
	{
		fprintf(stderr, "error reading wakeupmode from fp\n");
		*reason = 0;
	}

	return 0;
}

/* FIXME: not sure if this really works for ufc960 ->must be checked */
static int getVersion(Context_t *context, int *version)
{
	char strVersion[8];
	fprintf(stderr, "Waiting on version from fp ...\n");

	/* front controller time */
	if (ioctl(context->fd, VFDGETVERSION, &strVersion) < 0)
	{
		perror("getVersion: ");
		return -1;
	}

	/* if we get the version info */
	if (strVersion[0] != '\0')
	{
		fprintf(stderr, "Success reading version from fp\n");
		*version = strVersion[1] * 10 | strVersion[2];
		printf("Version = %d\n", *version);
	}
	else
	{
		fprintf(stderr, "error reading version from fp\n");
		*version = 0;
	}

	return 0;
}

static int Exit(Context_t *context)
{
	tUFC960Private *private = (tUFC960Private *)((Model_t *)context->m)->private;

	if (context->fd > 0)
		close(context->fd);

	free(private);
	exit(1);
}

static int Clear(Context_t *context)
{
	int i;
	setText(context, "                ");
	setBrightness(context, 5);

	for (i = 1; i <= 6 ; i++)
		setLed(context, i, 0);

	for (i = 1; i <= 16 ; i++)
		setIcon(context, i, 0);

	return 0;
}

static int setLedBrightness(Context_t *context, int brightness)
{
	struct micom_ioctl_data vData;

	if (brightness < 0 || brightness > 0xff)
		return -1;

	vData.u.brightness.level = brightness;
	setMode(context->fd);
	printf("%d\n", context->fd);

	if (ioctl(context->fd, VFDLEDBRIGHTNESS, &vData) < 0)
	{
		perror("setLedBrightness: ");
		return -1;
	}

	return 0;
}

Model_t UFC960_model =
{
	.Name                      = "Kathrein UFC960 frontpanel control utility",
	.Type                      = Ufc960,
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
//	.GetWakeupReason           = getWakeupReason, //TODO: CHECK IF WORKING
	.SetLight                  = setLight,
	.Exit                      = Exit,
	.SetLedBrightness          = setLedBrightness,
	.GetVersion                = getVersion,
	.SetRF                     = NULL,
	.SetFan                    = NULL,
	.private                   = NULL
};
