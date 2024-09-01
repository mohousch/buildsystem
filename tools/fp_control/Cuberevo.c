/*
 * Cuberevo.c
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
#include "Cuberevo.h"

static int setText(Context_t *context, char *theText);
static int Clear(Context_t *context);
static int setIcon(Context_t *context, int which, int on);
static int getVersion(Context_t *context, int *version);
static int setDisplayTime(Context_t *context, int on);

/******************** constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
#define cEVENT_DEVICE "/dev/input/event0"

#define cMAXCharsCuberevo 14 /* 14seg ->rest is filtered by driver */

//#define USE_FP_UTC

typedef struct
{
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tCUBEREVOPrivate;

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
 * the micom fp.
 */
static void setMicomTime(time_t theGMTTime, char *destString, int seconds)
{
	struct tm *now_tm;
	char tmpString[13];
#ifdef USE_FP_UTC
	now_tm = gmtime(&theGMTTime);
#else
	now_tm = localtime(&theGMTTime);
#endif

	if (seconds)
	{
		sprintf(tmpString, "%02d%02d%02d%02d%02d%02d",
			now_tm->tm_year - 100, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);
		strncpy(destString, tmpString, 12);
	}
	else
	{
		sprintf(tmpString, "%02d%02d%02d%02d%02d",
			now_tm->tm_year - 100, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min);
		strncpy(destString, tmpString, 10);
	}
}

static time_t getMicomTime(char *micomTimeString)
{
	char convertTime[128];
	unsigned int year, month, day;
	unsigned int hour, min, sec;
	struct tm the_tm;
	time_t convertedTime;
	sprintf(convertTime, "%02x %02x %02x %02x %02x %02x\n",
		micomTimeString[0], micomTimeString[1],
		micomTimeString[2], micomTimeString[3],
		micomTimeString[4], micomTimeString[5]);
	sscanf(convertTime, "%d %d %d %d %d %d", &sec, &min, &hour, &day, &month, &year);
	the_tm.tm_year = year + 100;
	the_tm.tm_mon  = month - 1;
	the_tm.tm_mday = day;
	the_tm.tm_hour = hour;
	the_tm.tm_min  = min;
	the_tm.tm_sec  = sec;
	the_tm.tm_isdst = -1;
	convertedTime = mktime(&the_tm); //16:00:00
#ifdef USE_FP_UTC
#else
// We have to convert the localstring to UTC
	{
		struct tm *wrong_tz_time_utc_tm;
		struct tm *wrong_tz_time_local_tm;
		wrong_tz_time_utc_tm = gmtime(&convertedTime); //16:00:00
		wrong_tz_time_local_tm = localtime(&convertedTime); //18:00:00
		convertedTime -= difftime(mktime(&wrong_tz_time_local_tm), mktime(&wrong_tz_time_utc_tm)); // -> local = 16:00:00 / utc = 14:00:00
	}
#endif
	return convertedTime;
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	tCUBEREVOPrivate *private = malloc(sizeof(tCUBEREVOPrivate));
	int vFd;
	printf("%s\n", __func__);
	vFd = open(cVFD_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}

	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tCUBEREVOPrivate));
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
	printf("%s ->\n", __func__);
	setMicomTime(*theGMTTime, vData.u.time.time, 1);
	fprintf(stderr, "Setting current Fp Time to: %s (mtime)\n", vData.u.time.time);
#if 1

	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("setTime: ");
		printf("%s <- -1\n", __func__);
		return -1;
	}

#endif
	printf("%s <- 0\n", __func__);
	return 0;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{
	struct micom_ioctl_data vData;
	printf("%s ->\n", __func__);
	fprintf(stderr, "Getting current fp time...\n");
#if 1

	/* front controller time */
	if (ioctl(context->fd, VFDGETTIME, &vData) < 0)
	{
		perror("getTime: ");
		printf("%s <- -1\n", __func__);
		return -1;
	}

#else
	strncpy(vData.u.get_time.time, "111017182540", 12);
#endif
	fprintf(stderr, "Got current fp Time %s (mtime)\n", vData.u.get_time.time);
	/* current front controller time */
	*theGMTTime = (time_t) getMicomTime(vData.u.get_time.time);
	printf("%s <- 0\n", __func__);
	return 0;
}

static int getWakeupTime(Context_t *context, time_t *theGMTTime)
{
	struct micom_ioctl_data vData;
	fprintf(stderr, "Waiting for current wakeup-time from fp...\n");

	/* front controller wake-up time */
	if (ioctl(context->fd, VFDGETWAKEUPTIME, &vData) < 0)
	{
		perror("getWakeupTime: ");
		return -1;
	}

	/* current front controller time */
	*theGMTTime = (time_t)getMicomTime(vData.u.wakeup_time.time);
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
	tCUBEREVOPrivate *private = (tCUBEREVOPrivate *)((Model_t *)context->m)->private;
	printf("%s ->\n", __func__);
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

	if ((wakeupTime == 0) || (wakeupTime == LONG_MAX))
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
		setMicomTime(wakeupTime, vData.u.standby.time, 0);
		fprintf(stderr, "Setting planned fp wakeup time to = %s (mtime)\n",
			vData.u.standby.time);
	}

	fprintf(stderr, "Entering deep standby, goodbye...\n");
	fflush(stdout);
	fflush(stderr);
	sleep(2);

	if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
	{
		perror("standby: ");
		printf("%s <- -1\n", __func__);
		return -1;
	}

	printf("%s <- 0\n", __func__);
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
	int retval, i, rd, version;
	struct tm *ts;
	char output[cMAXCharsCuberevo + 1];
	struct input_event ev[64];
	tCUBEREVOPrivate *private = (tCUBEREVOPrivate *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);
	vFd = open(cEVENT_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "cannot open %s\n", cEVENT_DEVICE);
		perror("");
		return -1;
	}

	Clear(context); /* clear display */
	setIcon(context, 1, 1); /* show standby icon */
	getVersion(context, &version);

	/* 4char vfd's ? */
	if ((version == 3) && (private->display))
	{
		/* yes: then enable fp time */
		setDisplayTime(context, 1);
	}
	else
	{
		setDisplayTime(context, 0);
	}

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

		if ((private->display) && (version != 3))
		{
			/* show soft time with user format */
			strftime(output, cMAXCharsCuberevo + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}

	Clear(context); /* clear display */
	setIcon(context, 1, 0); /* unshow standby icon */

	if (version == 3)
		setDisplayTime(context, 0);

	return 0;
}

static int setText(Context_t *context, char *theText)
{
	char vHelp[128];
	strncpy(vHelp, theText, cMAXCharsCuberevo);
	vHelp[cMAXCharsCuberevo] = '\0';
	/* printf("%s, %d\n", vHelp, strlen(vHelp)); */
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

static int setRFModulator(Context_t *context, int on)
{
	struct micom_ioctl_data vData;
	vData.u.rf.on = on;
	setMode(context->fd);

	if (ioctl(context->fd, VFDSETRF, &vData) < 0)
	{
		perror("setRFModulator: ");
		return -1;
	}

	return 0;
}

static int setDisplayTime(Context_t *context, int on)
{
	struct micom_ioctl_data vData;
	vData.u.display_time.on = on;
	setMode(context->fd);

	if (ioctl(context->fd, VFDSETDISPLAYTIME, &vData) < 0)
	{
		perror("setDisplayTime: ");
		return -1;
	}

	return 0;
}

static int setFan(Context_t *context, int on)
{
	struct micom_ioctl_data vData;
	vData.u.fan.on = on;
	setMode(context->fd);

	if (ioctl(context->fd, VFDSETFAN, &vData) < 0)
	{
		perror("setFan: ");
		return -1;
	}

	return 0;
}

static int setTimeMode(Context_t *context, int twentyFour)
{
	struct micom_ioctl_data vData;
	vData.u.time_mode.twentyFour = twentyFour;
	setMode(context->fd);

	if (ioctl(context->fd, VFDSETTIMEMODE, &vData) < 0)
	{
		perror("setTimeMode: ");
		return -1;
	}

	return 0;
}

static int setBrightness(Context_t *context, int brightness)
{
	struct micom_ioctl_data vData;

	if (brightness < 0 || brightness > 7)
		return -1;

	vData.u.brightness.level = brightness;
	setMode(context->fd);
	printf("%d\n", context->fd);

	if (ioctl(context->fd, VFDBRIGHTNESS, &vData) < 0)
	{
		perror("setBrightness: ");
		return -1;
	}

	return 0;
}

static int setLight(Context_t *context, int on)
{
	if (on)
		setBrightness(context, 7);
	else
		setBrightness(context, 0);

	return 0;
}

/* Attention: this is not the wakeup reason as for other
 * boxes (poweron, timer and son on) this is:
 * 0x00 ->timer off
 * 0x02 ->timer on
 */
static int getWakeupReason(Context_t *context, eWakeupReason *reason)
{
	struct micom_ioctl_data vData;
	fprintf(stderr, "Waiting for wakeupmode from fp...\n");

	/* front controller data */
	if (ioctl(context->fd, VFDGETWAKEUPMODE, &vData) < 0)
	{
		perror("getWakeupReason: ");
		return -1;
	}

	if ((vData.u.status.status & 0xff) == 0x02)
		*reason = TIMER;
	else
		*reason = NONE;

	printf("Reason = 0x%x\n", *reason);
	return 0;
}

static int getVersion(Context_t *context, int *version)
{
	struct micom_ioctl_data micom;
	fprintf(stderr, "waiting on version from fp ...\n");

	/* front controller time */
	if (ioctl(context->fd, VFDGETVERSION, &micom) < 0)
	{
		perror("getVersion: ");
		return -1;
	}

	*version = micom.u.version.version;
	printf("micom version = %d\n", micom.u.version.version);
	return 0;
}

static int Exit(Context_t *context)
{
	tCUBEREVOPrivate *private = (tCUBEREVOPrivate *)((Model_t *)context->m)->private;

	if (context->fd > 0)
		close(context->fd);

	free(private);
	exit(1);
}

static int Clear(Context_t *context)
{
	struct vfd_ioctl_data data;
	data.start = 0;

	if (ioctl(context->fd, VFDDISPLAYWRITEONOFF, &data) < 0)
	{
		perror("Clear: ");
		return -1;
	}

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

Model_t Cuberevo_model =
{
	.Name                      = "CUBEREVO frontpanel control utility",
	.Type                      = Cuberevo,
	.Init                      = init,
	.Clear                     = Clear,
	.Usage                     = NULL,
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
	.SetPwrLed                 = NULL,
	.GetWakeupReason           = getWakeupReason,
	.SetLight                  = setLight,
	.Exit                      = Exit,
	.SetLedBrightness          = setLedBrightness,
	.GetVersion                = getVersion,
	.SetRF                     = setRFModulator,
	.SetFan                    = setFan,
	.GetWakeupTime             = getWakeupTime,
	.SetDisplayTime            = setDisplayTime,
	.SetTimeMode               = setTimeMode,
	.private                   = NULL
};
