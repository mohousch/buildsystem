/*
 * Spark.c
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

#include "global.h"
#include "Spark.h"

static int Spark_setText(Context_t *context, char *theText);

/******************** constants ************************ */

#define cVFD_DEVICE "/dev/vfd"

#define cMAXCharsSpark 63

typedef struct
{
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tSparkPrivate;

/* ******************* helper/misc functions ****************** */

/* Calculate the time value which we can pass to
 * the aotom fp. its a mjd time (mjd=modified
 * julian date). mjd is relative to gmt so theGMTTime
 * must be in GMT/UTC.
 */
void Spark_setAotomTime(time_t theGMTTime, char *destString)
{
	/* from u-boot aotom */
	struct tm *now_tm;
	now_tm = localtime(&theGMTTime);
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

unsigned long Spark_getAotomTime(char *aotomTimeString)
{
	unsigned int mjd = ((aotomTimeString[1] & 0xFF) * 256) + (aotomTimeString[2] & 0xFF);
	unsigned long epoch = ((mjd - 40587) * 86400);
	unsigned int hour = aotomTimeString[3] & 0xFF;
	unsigned int min = aotomTimeString[4] & 0xFF;
	unsigned int sec = aotomTimeString[5] & 0xFF;
	epoch += (hour * 3600 + min * 60 + sec);
	printf("MJD = %d epoch = %ld, time = %02d:%02d:%02d\n", mjd, epoch, hour, min, sec);
	return epoch;
}

/* ******************* driver functions ****************** */

static int Spark_init(Context_t *context)
{
	tSparkPrivate *private = malloc(sizeof(tSparkPrivate));
	int vFd;
//	printf("%s\n", __func__);
	vFd = open(cVFD_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}

	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tSparkPrivate));
	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement);
	return vFd;
}

static int Spark_usage(Context_t *context, char *prg_name)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int Spark_setTime(Context_t *context, time_t *theGMTTime)
{
	struct aotom_ioctl_data vData;
	printf("%s\n", __func__);
	Spark_setAotomTime(*theGMTTime, vData.u.time.time);

	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("settime: ");
		return -1;
	}

	return 0;
}

static int Spark_getTime(Context_t *context, time_t *theGMTTime)
{
	time_t iTime;
	fprintf(stderr, "waiting on current time from fp ...\n");

	/* front controller time */
	if (ioctl(context->fd, VFDGETTIME, &iTime) < 0)
	{
		perror("gettime: ");
		return -1;
	}

	/* if we get the fp time */
	if (iTime != '\0')
	{
		fprintf(stderr, "success reading time from fp\n");
		/* current front controller time */
		*theGMTTime = iTime;
	}
	else
	{
		fprintf(stderr, "error reading time from fp\n");
		*theGMTTime = 0;
	}

	return 0;
}

static int Spark_setTimer(Context_t *context, time_t *theGMTTime)
{
	struct aotom_ioctl_data vData;
	time_t curTime;
	time_t wakeupTime;
	struct tm *ts;
//	tSparkPrivate *private = (tSparkPrivate *)((Model_t *)context->m)->private;
	time(&curTime);
	ts = localtime(&curTime);
	fprintf(stderr, "Current Time: %02d:%02d:%02d %02d-%02d-%04d\n", ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);

	if (theGMTTime == NULL)
		wakeupTime = read_timers_utc(curTime);
	else
		wakeupTime = *theGMTTime;

	if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX))
	{
		/* nothing to do for e2 */
		fprintf(stderr, "no e2 timer found clearing fp wakeup time ... good bye ...\n");
		vData.u.standby.time[0] = '\0';

		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("standby: ");
			return -1;
		}
	}
	else
	{
		unsigned long diff;
		time_t iTime;
		fprintf(stderr, "waiting on current time from fp ...\n");

		/* get front controller time */
		if (ioctl(context->fd, VFDGETTIME, &iTime) < 0)
		{
			perror("Get current fp time");
			return -1;
		}

		/* difference from now to wake up */
		diff = (unsigned long int) wakeupTime - curTime;

		if (iTime != '\0')
		{
			fprintf(stderr, "success reading time from fp\n");
			/* use current front controller time */
			curTime = iTime;
		}
		else
		{
			fprintf(stderr, "error reading time ... assuming localtime\n");
			/* noop current time already set */
		}

		wakeupTime = curTime + diff;
		Spark_setAotomTime(wakeupTime, vData.u.standby.time);

		if (ioctl(context->fd, VFDSTANDBY, &wakeupTime) < 0)
		{
			perror("Shut down until wake up time");
			return -1;
		}
	}

	return 0;
}

static int Spark_getTimer(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int Spark_shutdown(Context_t *context, time_t *shutdownTimeGMT)
{
	time_t curTime;

	/* shutdown immediate */
	if (*shutdownTimeGMT == -1)
		return (Spark_setTimer(context, NULL));

	while (1)
	{
		time(&curTime);

		/*printf("curTime = %d, shutdown %d\n", curTime, *shutdownTimeGMT);*/

		if (curTime >= *shutdownTimeGMT)
		{
			/* set most recent e2 timer and bye bye */
			return (Spark_setTimer(context, NULL));
		}

		usleep(100000);
	}

	return -1;
}

static int Spark_reboot(Context_t *context, time_t *rebootTimeGMT)
{
	time_t curTime;
	struct aotom_ioctl_data vData;

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

static int Spark_Sleep(Context_t *context, time_t *wakeUpGMT)
{
#if 0
	time_t curTime;
	int sleep = 1;
	int vFd;
	fd_set rfds;
	struct timeval tv;
	int retval;
	struct tm *ts;
	char output[cMAXCharsSpark + 1];
	tSparkPrivate *private = (tSparkPrivate *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);
	vFd = open(cRC_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "cannot open %s\n", cRC_DEVICE);
		perror("");
		return -1;
	}

	printf("%s 1\n", __func__);

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
				sleep = 0;
			}
		}

		if (private->display)
		{
			strftime(output, cMAXCharsSpark + 1, private->timeFormat, ts);
			Spark_setText(context, output);
		}
	}

#endif
	return 0;
}

static int Spark_setText(Context_t *context, char *theText)
{
	char vHelp[128];
	strncpy(vHelp, theText, cMAXCharsSpark);
	vHelp[cMAXCharsSpark] = '\0';
	/* printf("%s, %d\n", vHelp, strlen(vHelp));*/
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}

static int Spark_setLed(Context_t *context, int which, int on)
{
	struct aotom_ioctl_data vData;
	vData.u.led.led_nr = which;
	vData.u.led.on = on;

	if (ioctl(context->fd, VFDSETLED, &vData) < 0)
	{
		perror("Setled");
		return -1;
	}

	return 0;
}

static int Spark_setIcon(Context_t *context, int which, int on)
{
	struct aotom_ioctl_data vData;
	vData.u.icon.icon_nr = which;
	vData.u.icon.on = on;

	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("Set icon");
		return -1;
	}

	return 0;
}

static int Spark_setBrightness(Context_t *context, int brightness)
{
	struct aotom_ioctl_data vData;

	if (brightness < 0 || brightness > 7)
		return -1;

	vData.u.brightness.level = brightness;
	printf("%d\n", context->fd);

	if (ioctl(context->fd, VFDBRIGHTNESS, &vData) < 0)
	{
		perror("Set brightness");
		return -1;
	}

	return 0;
}

static int Spark_setPwrLed(Context_t *context, int brightness)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}


static int Spark_setLight(Context_t *context, int on)
{
	if (on)
		Spark_setBrightness(context, 7);
	else
		Spark_setBrightness(context, 0);

	return 0;
}

static int Spark_Exit(Context_t *context)
{
	tSparkPrivate *private = (tSparkPrivate *)
				 ((Model_t *)context->m)->private;

	if (context->fd > 0)
		close(context->fd);

	free(private);

	return 1;
}

static int Spark_Clear(Context_t *context)
{
	struct aotom_ioctl_data vData;

	if (ioctl(context->fd, VFDDISPLAYCLR, &vData) < 0)
	{
		perror("Clear");
		return -1;
	}

	return 0;
}

static int Spark_getWakeupReason(Context_t *context, eWakeupReason *reason)
{
	if (ioctl(context->fd, VFDGETSTARTUPSTATE, reason) < 0)
	{
		perror("Get wakeup reason");
		return -1;
	}
	printf("reason = 0x%x\n", *reason);

	return 0;
}

static int Spark_setDisplayTime(Context_t *context, int on)
{
	if (on == 1)
	{
		time_t theGMTTime = time(NULL);
		struct tm *gmt;
		gmt = localtime(&theGMTTime);

		if (gmt->tm_year == 100)
		{
			fprintf(stderr, "RTC Time not set.\n");
		}
		else
		{
			fprintf(stderr, "Setting Clock to current time: %02d:%02d:%02d %02d-%02d-%04d\n",
				gmt->tm_hour, gmt->tm_min, gmt->tm_sec, gmt->tm_mday, gmt->tm_mon + 1, gmt->tm_year + 1900);
			theGMTTime += gmt->tm_gmtoff;

			if (ioctl(context->fd, VFDREBOOT, &theGMTTime) < 0)
			{
				perror("settime: ");
				return -1;
			}
		}
	}
	else
	{
		Spark_Clear(context);
	}

	return 0;
}

Model_t Spark_model =
{
	.Name                      = "Edision Spark frontpanel control utility",
	.Type                      = Spark,
	.Init                      = Spark_init,
	.Clear                     = Spark_Clear,
	.Usage                     = Spark_usage,
	.SetTime                   = Spark_setTime,
	.GetTime                   = Spark_getTime,
	.SetTimer                  = Spark_setTimer,
	.GetTimer                  = Spark_getTimer,
	.SetDisplayTime            = Spark_setDisplayTime,
	.Shutdown                  = Spark_shutdown,
	.Reboot                    = Spark_reboot,
	.Sleep                     = Spark_Sleep,
	.SetText                   = Spark_setText,
	.SetLed                    = Spark_setLed,
	.SetIcon                   = Spark_setIcon,
	.SetBrightness             = Spark_setBrightness,
	.GetWakeupReason           = Spark_getWakeupReason,
	.SetPwrLed                 = Spark_setPwrLed,
	.SetLight                  = Spark_setLight,
	.Exit                      = Spark_Exit,
	.SetLedBrightness          = NULL,
	.GetVersion                = NULL,
	.SetRF                     = NULL,
	.SetFan                    = NULL,
	.private                   = NULL
};
