/*
 * global.c
 *
 * (c) 2009 dagobert/donald@teamducktales
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
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>

#include "global.h"

/* #define E2TIMERSXML "/usr/local/share/enigma2/timers.xml" */
#define E2TIMERSXML "/etc/enigma2/timers.xml"
#define E2WAKEUPTIME "/proc/stb/fp/wakeup_time"

#define NEUTRINO_TIMERS "/var/tuxbox/config/timerd.conf"

#define CONFIG "/etc/vdstandby.cfg"
char *sDisplayStd = "%a %d %H:%M:%S";

#define WAKEUPFILE "/var/wakeup"
#define WAS_TIMER_WAKEUP "/proc/stb/fp/was_timer_wakeup"

#define E2_WAKEUP_TIME_PROC

static Model_t *AvailableModels[] =
{
	&Ufs910_1W_model,
	&Ufs910_14W_model,
	&UFS922_model,
	&Fortis_model,
	&HL101_model,
	&VIP2_model,
	&Hs5101_model,
	&UFS912_model,
	&UFC960_model,
	&Spark_model,
	&Adb_Box_model,
	&Cuberevo_model,
	&CNBOX_model,
	NULL
};

#ifdef E2_WAKEUP_TIME_PROC
static time_t read_e2_timers(time_t curTime)
{
	char line[12];
	time_t recordTime = LONG_MAX;
	FILE *fd = fopen(E2WAKEUPTIME, "r");
	printf("Getting Enigma2 wakeup time");

	if (fd > 0)
	{
		fgets(line, 11, fd);
		sscanf(line, "%ld", &recordTime);

		if (recordTime <= curTime)
		{
			recordTime = LONG_MAX;
			printf(" - Got time, but in the past (ignoring)\n");
		}
		else
			printf(" - Done\n");

		fclose(fd);
	}
	else
		printf(" - Error reading %s\n", E2WAKEUPTIME);

	return recordTime;
}
#else
static time_t read_e2_timers(time_t curTime)
{
	char recordString[11];
	char line[1000];
	time_t recordTime = LONG_MAX;
	FILE *fd = fopen(E2TIMERSXML, "r");
	printf("Getting enigma2 wakeup time");

	if (fd > 0)
	{
		while (fgets(line, 999, fd) != NULL)
		{
			line[999] = '\0';

			if (!strncmp("<timer begin=\"", line, 14))
			{
				unsigned long int tmp = 0;
				strncpy(recordString, line + 14, 10);
				recordString[11] = '\0';
				tmp = atol(recordString);
				recordTime = (tmp < recordTime && tmp > curTime ? tmp : recordTime);
			}
		}

		printf(" - Done\n");
		fclose(fd);
	}
	else
	{
		printf(" - Error reading %s\n", E2TIMERSXML);
	}
	return recordTime;
}
#endif

static time_t read_neutrino_timers(time_t curTime)
{
	char line[1000];
	time_t recordTime = LONG_MAX;
	FILE *fd = fopen(NEUTRINO_TIMERS, "r");
	printf("Getting neutrino wakeup time");

	if (fd > 0)
	{
		printf("Opening %s\n", NEUTRINO_TIMERS);
		while (fgets(line, 999, fd) != NULL)
		{
			line[999] = '\0';

			if (strstr(line, "ALARM_TIME_") != NULL)
			{
				time_t tmp = 0;
				char *str;
				str = strstr(line, "=");

				if (str != NULL)
				{
					tmp = atol(str + 1);
					recordTime = (tmp < recordTime && tmp > curTime ? tmp : recordTime);
				}
			}
		}

		printf(" - Done\n");
		fclose(fd);
	}
	else
		printf(" - Error reading %s\n", NEUTRINO_TIMERS);

	if (recordTime != LONG_MAX)
	{
		int wakeupDecrement = 5 * 60;
		int platzhalter;
		char * dummy;
		checkConfig(&platzhalter, &platzhalter, &dummy, &wakeupDecrement);
		recordTime -= wakeupDecrement;
	}

	return recordTime;
}

// Write the wakeup time to a file to allow detection of wakeup cause if fp does not support
static void write_wakeup_file(time_t wakeupTime)
{
	FILE *wakeupFile;
	wakeupFile = fopen(WAKEUPFILE, "w");

	if (wakeupFile)
	{
		fprintf(wakeupFile, "%ld", wakeupTime);
		fclose(wakeupFile);
		system("sync");
	}
}

static time_t read_wakeup_file()
{
	time_t wakeupTime = LONG_MAX;
	FILE *wakeupFile;
	wakeupFile = fopen(WAKEUPFILE, "r");

	if (wakeupFile)
	{
		fscanf(wakeupFile, "%ld", &wakeupTime);
		fclose(wakeupFile);
	}

	return wakeupTime;
}

#define FIVE_MIN 300

// Important: system has to have a valid current time
// This is a little bit tricky, we can only detect if the time is valid
// and this check happens +-5min arround the timer
int getWakeupReasonPseudo(eWakeupReason *reason)
{
	time_t curTime = 0;
	time_t wakeupTime = LONG_MAX;
	printf("getWakeupReasonPseudo: IMPORTANT: Valid Linux System Time is mandatory\n");
	time(&curTime);
	wakeupTime = read_wakeup_file();

	if ((curTime - FIVE_MIN) < wakeupTime && (curTime + FIVE_MIN) > wakeupTime)
	{
		*reason = TIMER;
	}
	else
	{
		*reason = NONE;
	}
	return 0;
}

int syncWasTimerWakeup(eWakeupReason reason)
{
	FILE *wasTimerWakeupProc = fopen(WAS_TIMER_WAKEUP, "w");

	if (wasTimerWakeupProc == NULL)
	{
		fprintf(stderr, "setWakeupReason failed to open %s\n", WAS_TIMER_WAKEUP);
		return -1;
	}

	if (reason == TIMER)
	{
		fwrite("1\n", 2, 1, wasTimerWakeupProc);
	}
	else
	{
		fwrite("0\n", 2, 1, wasTimerWakeupProc);
	}
	fclose(wasTimerWakeupProc);
	return 0;
}

// This function tries to read the wakeup time from enigma2 and neutrino
// The wakeup time will also be written to file for wakeupcause detection
// If no wakeup time can be found LONG_MAX will be returned
time_t read_timers_utc(time_t curTime)
{
	time_t wakeupTime = LONG_MAX;
	wakeupTime = read_e2_timers(curTime);

	if (wakeupTime == LONG_MAX)
	{
		wakeupTime = read_neutrino_timers(curTime);
	}
	write_wakeup_file(wakeupTime);
	return wakeupTime;
}

// This function returns a time in the future
time_t read_fake_timer_utc(time_t curTime)
{
	struct tm tsWake;
	struct tm *ts;
	time_t wakeupTime = LONG_MAX;
	ts = gmtime(&curTime);
	tsWake.tm_hour = ts->tm_hour;
	tsWake.tm_min = ts->tm_min;
	tsWake.tm_sec = ts->tm_sec;
	tsWake.tm_mday = ts->tm_mday;
	tsWake.tm_mon = ts->tm_mon;
	tsWake.tm_year = ts->tm_year + 1;
	wakeupTime = mktime(&tsWake);
	return wakeupTime;
}
/* ******************************************** */

double modJulianDate(struct tm *theTime)
{
	double date;
	int month;
	int day;
	int year;
	year = theTime->tm_year + 1900;
	month = theTime->tm_mon + 1;
	day = theTime->tm_mday;
	date = day - 32076 +
	       1461 * (year + 4800 + (month - 14) / 12) / 4 +
	       367 * (month - 2 - (month - 14) / 12 * 12) / 12 -
	       3 * ((year + 4900 + (month - 14) / 12) / 100) / 4;
	date += (theTime->tm_hour + 12.0) / 24.0;
	date += (theTime->tm_min) / 1440.0;
	date += (theTime->tm_sec) / 86400.0;
	date -= 2400000.5;
	return date;
}

/* ********************************************** */

int searchModel(Context_t *context, eBoxType type)
{
	int i;

	for (i = 0; AvailableModels[i] != NULL; i++)
		if (AvailableModels[i]->Type == type)
		{
			context->m = AvailableModels[i];
			return 0;
		}

	return -1;
}

int checkConfig(int *display, int *display_custom, char **timeFormat, int *wakeup)
{
	const int MAX = 100;
	char buffer[MAX];
	*display = 0;
	*display_custom = 0;
	*timeFormat = "Unknown";
	*wakeup = 5 * 60;
	FILE *fd_config = fopen(CONFIG, "r");

	if (fd_config == NULL)
	{
		printf("Config file (%s) not found,\nusing standard config:", CONFIG);
		printf("Config:\nDisplay:%d, DISPLAYCUSTOM = %d, CUSTOM = %s, WAKEUPDECREMENT %d\n", *display, *display_custom, *timeFormat, *wakeup);
		return -1;
	}

	while (fgets(buffer, MAX, fd_config))
	{
		if (!strncmp("DISPLAY=", buffer, 8))
		{
			char *option = &buffer[8];

			if (!strncmp("TRUE", option, 2))
				*display = 1;
		}
		else if (!strncmp("DISPLAYCUSTOM=", buffer, 14))
		{
			char *option = &buffer[14];
			*display_custom = 1;
			*timeFormat = strdup(option);
		}
		else if (!strncmp("WAKEUPDECREMENT=", buffer, 16))
		{
			char *option = &buffer[16];
			*wakeup = atoi(option);
		}
	}

	if (*timeFormat == NULL)
		*timeFormat = sDisplayStd;

	printf("configs: DISPLAY = %d, DISPLAYCUSTOM = %d, CUSTOM = %s, WAKEUPDECREMENT %d\n", *display, *display_custom, *timeFormat, *wakeup);
	fclose(fd_config);
	return 0;
}
