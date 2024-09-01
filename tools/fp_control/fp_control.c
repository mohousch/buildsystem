/*
 * fp_control.c
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

/*
 * added:
 * powerLed intensity adjustment
 * by zeroone
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "global.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/* software version of fp_control. please increase on every change */
static const char *sw_version = "1.04";

static eWakeupReason reason = 0;


typedef struct
{
	char *arg;
	char *arg_long;
	char *arg_description;
} tArgs;

tArgs vArgs[] =
{
	{
		"-e",  " --setTimer         ", "Args: No arguments or [time date] Format: HH:MM:SS dd-mm-YYYY \
		\n\tSet the most recent timer from e2 or neutrino to the frontcontroller and standby \
		\n\tSet the current frontcontroller wake-up time"
	},
	{ "-d",  " --shutdown         ", "Args: [time date] Format: HH:MM:SS dd-mm-YYYY\n\tMimics shutdown command. Shutdown receiver via fc at given time." },
	{ "-g",  " --getTime          ", "Args: No arguments\n\tReturn current set frontcontroller time" },
	{ "-gs", " --getTimeAndSet    ", "Args: No arguments\n\tSet system time to current frontcontroller time" },
	{ "-gw", " --getWakeupTime    ", "Args: No arguments\n\tReturn current wakeup time" },
	{ "-s",  " --setTime          ", "Args: time date Format: HH:MM:SS dd-mm-YYYY\n\tSet the current frontcontroller time" },
	{ "-gt", " --getTimer         ", "Args: No arguments\n\tGet the current frontcontroller wake-up time" },
	{ "-r",  " --reboot           ", "Args: time date Format: HH:MM:SS dd-mm-YYYY\n\tReboot receiver via fc at given time" },
	{ "-p",  " --sleep            ", "Args: time date Format: HH:MM:SS dd-mm-YYYY\n\tReboot receiver via fc at given time" },
	{ "-t",  " --settext          ", "Args: text\n\tSet text to frontpanel." },
	{ "-l",  " --setLed           ", "Args: led on\n\tSet a led on or off" },
	{ "-i",  " --setIcon          ", "Args: icon on\n\tSet an icon on or off" },
	{ "-b",  " --setBrightness    ", "Args: brightness\n\tSet display brightness" },
	{ "-P",  " --setPwrLed        ", "Args: 0..15\n\tSet PowerLed brightness" },
	{ "-w",  " --getWakeupReason  ", "Args: No arguments\n\tGet the wake-up reason" },
	{ "-L",  " --setLight         ", "Args: 0/1\n\tSet light" },
	{ "-c",  " --clear            ", "Args: No argumens\n\tClear display, all icons and leds off" },
	{ "-v",  " --version          ", "Args: No argumens\n\tGet version from fc" },
	{ "-sf", " --setFan           ", "Args: 0/1\n\tset fan on/off" },
	{ "-sr", " --setRF            ", "Args: 0/1\n\tset rf modulator on/off" },
	{ "-dt", " --display_timer    ", "Args: 0/1\n\tset display time on/off" },
	{ "-tm", " --time_mode        ", "Args: 0/1\n\ttoggle 12/24 hour mode" },
	{ NULL, NULL, NULL }
};

const char *wakeupreason[8] = { "unknown", "poweron", "standby", "timer", "powerswitch", "unknown", "unknown", "unknown" };

void usage(Context_t *context, char *prg, char *cmd)
{
	/* let the model print out what it can handle in reality */
	if ((((Model_t *)context->m)->Usage == NULL) || (((Model_t *)context->m)->Usage(context, prg) < 0))
	{
		int i;
		/* or printout a default usage */
		fprintf(stderr, "General usage:\n\n");
		fprintf(stderr, "%s argument [optarg1] [optarg2]\n", prg);

		for (i = 0; ; i++)
		{
			if (vArgs[i].arg == NULL)
				break;

			if ((cmd == NULL) || (strcmp(cmd, vArgs[i].arg) == 0) || (strstr(vArgs[i].arg_long, cmd) != NULL))
				fprintf(stderr, "%s %s %s\n", vArgs[i].arg, vArgs[i].arg_long, vArgs[i].arg_description);
		}
	}

	if (((Model_t *)context->m)->Exit)
		((Model_t *)context->m)->Exit(context);

	exit(1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/* FIXME: check if this function is correct and delivers gmt time */
void getTimeFromArg(char *timeStr, char *dateStr, time_t *theGMTTime)
{
	struct tm theTime;
//	printf("%s\n", __func__);
	sscanf(timeStr, "%d:%d:%d", &theTime.tm_hour, &theTime.tm_min, &theTime.tm_sec);
	sscanf(dateStr, "%d-%d-%d", &theTime.tm_mday, &theTime.tm_mon, &theTime.tm_year);
	theTime.tm_year -= 1900;
	theTime.tm_mon = theTime.tm_mon - 1;
	theTime.tm_isdst = -1; /* say mktime that we do not know */
	/* FIXME: hmm this is not a gmt or, isn't it? */
	*theGMTTime = mktime(&theTime);
	/* new_time = localtime(&dummy);*/
//	printf("%s <\n", __func__);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void processCommand(Context_t *context, int argc, char *argv[])
{
	int i;

	if (((Model_t *)context->m)->Init)
		context->fd = ((Model_t *)context->m)->Init(context);

	if (argc > 1)
	{
		i = 1;

		while (i < argc)
		{
			if ((strcmp(argv[i], "-e") == 0) || (strcmp(argv[i], "--setTimer") == 0))
			{
				if (argc == 4)
				{
					time_t theGMTTime;
					getTimeFromArg(argv[i + 1], argv[i + 2], &theGMTTime);

					/* set the frontcontroller timer from args */
					if (((Model_t *)context->m)->SetTimer)((Model_t *)context->m)->SetTimer(context, &theGMTTime);

					i += 2;
				}
				else if (argc == 2)
				{
					/* setup timer; wake-up time will be read */
					if (((Model_t *)context->m)->SetTimer)
						((Model_t *)context->m)->SetTimer(context, NULL);
				}
				else
					usage(context, argv[0], argv[1]);
			}
			else if ((strcmp(argv[i], "-g") == 0) || (strcmp(argv[i], "--getTime") == 0))
			{
				time_t theGMTTime;

				/* get the frontcontroller time */
				if (((Model_t *)context->m)->GetTime)
				{
					if (((Model_t *)context->m)->GetTime(context, &theGMTTime) == 0)
					{
						struct tm *gmt = gmtime(&theGMTTime);
						fprintf(stderr, "Current Time: %02d:%02d:%02d %02d-%02d-%04d\n",
							gmt->tm_hour, gmt->tm_min, gmt->tm_sec, gmt->tm_mday, gmt->tm_mon + 1, gmt->tm_year + 1900);
					}
				}
			}
			else if ((strcmp(argv[i], "-gs") == 0) || (strcmp(argv[i], "--getTimeAndSet") == 0))
			{
				time_t theGMTTime;

				/* get the frontcontroller time */
				if (((Model_t *)context->m)->GetTime)
				{
					if (((Model_t *)context->m)->GetTime(context, &theGMTTime) == 0)
					{
						struct tm *gmt = gmtime(&theGMTTime);
#if 0
						struct timeval tv;
						time_t allsec;
						allsec = mktime(gmt);
						tv.tv_sec = allsec;
						settimeofday(&tv, 0); // only works on spark, so we make a system-call later
#endif
						fprintf(stderr, "Setting RTC to current frontpanel time: %02d:%02d:%02d %02d-%02d-%04d\n",
							gmt->tm_hour, gmt->tm_min, gmt->tm_sec, gmt->tm_mday, gmt->tm_mon + 1, gmt->tm_year + 1900);
						char cmd[50];
						sprintf(cmd, "date -s %04d.%02d.%02d-%02d:%02d:%02d\n", gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
						system(cmd);
					}
				}
			}
			else if ((strcmp(argv[i], "-gw") == 0) || (strcmp(argv[i], "--getWakeupTime") == 0))
			{
				time_t theGMTTime;

				/* get the frontcontroller wakeup time */
				if (((Model_t *)context->m)->GetWakeupTime)
				{
					if (((Model_t *)context->m)->GetWakeupTime(context, &theGMTTime) == 0)
					{
						struct tm *gmt = gmtime(&theGMTTime);
						fprintf(stderr, "Wakeup Time: %02d:%02d:%02d %02d-%02d-%04d\n",
							gmt->tm_hour, gmt->tm_min, gmt->tm_sec, gmt->tm_mday, gmt->tm_mon + 1, gmt->tm_year + 1900);
					}
				}
			}
			else if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--setTime") == 0))
			{
				time_t theGMTTime;

				if (argc == 4)
				{
					getTimeFromArg(argv[i + 1], argv[i + 2], &theGMTTime);

					/* set the frontcontroller time */
					if (((Model_t *)context->m)->SetTime)
						((Model_t *)context->m)->SetTime(context, &theGMTTime);
				}
				else
					usage(context, argv[0], argv[1]);

				i += 2;
			}
			else if ((strcmp(argv[i], "-gt") == 0) || (strcmp(argv[i], "--getTimer") == 0))
			{
				time_t theGMTTime;

				/* get the current timer value from frontcontroller */
				if (((Model_t *)context->m)->GetTimer)
				{
					if (((Model_t *)context->m)->GetTimer(context, &theGMTTime) == 0)
					{
						struct tm *gmt = gmtime(&theGMTTime);
						fprintf(stderr, "Current Timer: %02d:%02d:%02d %02d-%02d-%04d\n",
							gmt->tm_hour, gmt->tm_min, gmt->tm_sec, gmt->tm_mday, gmt->tm_mon + 1, gmt->tm_year + 1900);
					}
				}
			}
			else if ((strcmp(argv[i], "-d") == 0) || (strcmp(argv[i], "--shutDown") == 0))
			{
				time_t theGMTTime;

				if (argc == 4)
				{
					getTimeFromArg(argv[i + 1], argv[i + 2], &theGMTTime);

					/* shutdown at the given time */
					if (((Model_t *)context->m)->Shutdown)
						((Model_t *)context->m)->Shutdown(context, &theGMTTime);
				}
				else if (argc == 2)
				{
					theGMTTime = -1;

					/* shutdown immediately */
					if (((Model_t *)context->m)->Shutdown)
						((Model_t *)context->m)->Shutdown(context, &theGMTTime);
				}
				else
					usage(context, argv[0], argv[1]);

				i += 2;
			}
			else if ((strcmp(argv[i], "-r") == 0) || (strcmp(argv[i], "--reboot") == 0))
			{
				time_t theGMTTime;

				if (argc == 4)
				{
					getTimeFromArg(argv[i + 1], argv[i + 2], &theGMTTime);

					/* reboot at the given time */
					if (((Model_t *)context->m)->Reboot)
						((Model_t *)context->m)->Reboot(context, &theGMTTime);
				}
				else
					usage(context, argv[0], argv[1]);

				i += 2;
			}
			else if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "--sleep") == 0))
			{
				time_t theGMTTime;

				if (argc == 4)
				{
					getTimeFromArg(argv[i + 1], argv[i + 2], &theGMTTime);

					/* sleep for a while, or wake-up on another reason (rc ...) */
					if (((Model_t *)context->m)->Sleep)
						((Model_t *)context->m)->Sleep(context, &theGMTTime);
				}
				else
					usage(context, argv[0], argv[1]);

				i += 2;
			}
			else if ((strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "--settext") == 0))
			{
				if (i + 1 <= argc)

					/* set display text */
					if (((Model_t *)context->m)->SetText)
						((Model_t *)context->m)->SetText(context, argv[i + 1]);

				i += 1;
			}
			else if ((strcmp(argv[i], "-l") == 0) || (strcmp(argv[i], "--setLed") == 0))
			{
				if (i + 2 <= argc)
				{
					int which, on;
					which = atoi(argv[i + 1]);
					on = atoi(argv[i + 2]);
					i += 2;

					/* set display led */
					if (((Model_t *)context->m)->SetLed)
						((Model_t *)context->m)->SetLed(context, which, on);
				}

				i += 2;
			}
			else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--setIcon") == 0))
			{
				if (i + 2 <= argc)
				{
					int which, on;
					which = atoi(argv[i + 1]);
					on = atoi(argv[i + 2]);

					/* set display icon */
					if (((Model_t *)context->m)->SetIcon)
						((Model_t *)context->m)->SetIcon(context, which, on);
				}

				i += 2;
			}
			else if ((strcmp(argv[i], "-b") == 0) || (strcmp(argv[i], "--setBrightness") == 0))
			{
				if (i + 1 <= argc)
				{
					int brightness;
					brightness = atoi(argv[i + 1]);

					/* set display brightness */
					if (((Model_t *)context->m)->SetBrightness)
						((Model_t *)context->m)->SetBrightness(context, brightness);
				}

				i += 1;
			}
			/* added by zeroone; set PowerLed Brightness on HDBOX*/
			// BEGIN SetPwrLed
			else if ((strcmp(argv[i], "-P") == 0) || (strcmp(argv[i], "--setPwrLed") == 0))
			{
				if (i + 1 <= argc)
				{
					int brightness;
					brightness = atoi(argv[i + 1]);

					/* set PwrLed Brightness icon */
					if (((Model_t *)context->m)->SetPwrLed)
						((Model_t *)context->m)->SetPwrLed(context, brightness);
				}

				i += 1;
			}
			// END SetPwrLed
			else if ((strcmp(argv[i], "-w") == 0) || (strcmp(argv[i], "--getWakeupReason") == 0))
			{
				int ret = -1;

				if (((Model_t *)context->m)->GetWakeupReason)
					ret = ((Model_t *)context->m)->GetWakeupReason(context, &reason);
				else
					ret = getWakeupReasonPseudo(&reason);

				if (ret == 0)
				{
					printf("Wakeup reason = %d (%s)\n\n", reason & 0x07, wakeupreason[reason & 0x07]);
					syncWasTimerWakeup(reason);
				}
			}
			else if ((strcmp(argv[i], "-L") == 0) || (strcmp(argv[i], "--setLight") == 0))
			{
				if (i + 1 < argc)
				{
					int on;
					on = atoi(argv[i + 1]);

					/* set brightness on/off */
					if (((Model_t *)context->m)->SetLight)
						((Model_t *)context->m)->SetLight(context, on);
				}

				i += 1;
			}
			else if ((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--clear") == 0))
			{
				/* clear the display */
				if (((Model_t *)context->m)->Clear)
					((Model_t *)context->m)->Clear(context);
			}
			else if ((strcmp(argv[i], "-led") == 0) || (strcmp(argv[i], "--setLedBrightness") == 0))
			{
				if (i + 1 <= argc)
				{
					/* set LED brightness */
					int brightness;
					brightness = atoi(argv[i + 1]);

					if (((Model_t *)context->m)->SetLedBrightness)
						((Model_t *)context->m)->SetLedBrightness(context, brightness);
				}

				i += 1;
			}
			else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0))
			{
				int version;

				/* get version */
				if (((Model_t *)context->m)->GetVersion)
					((Model_t *)context->m)->GetVersion(context, &version);
			}
			else if ((strcmp(argv[i], "-sf") == 0) || (strcmp(argv[i], "--setFan") == 0))
			{
				if (i + 1 <= argc)
				{
					int on;
					on = atoi(argv[i + 1]);

					/* set fan on/off */
					if (((Model_t *)context->m)->SetFan)
						((Model_t *)context->m)->SetFan(context, on);
				}

				i += 1;
			}
			else if ((strcmp(argv[i], "-sr") == 0) || (strcmp(argv[i], "--setRF") == 0))
			{
				if (i + 1 <= argc)
				{
					int on;
					on = atoi(argv[i + 1]);

					/* set rf on/off */
					if (((Model_t *)context->m)->SetRF)
						((Model_t *)context->m)->SetRF(context, on);
				}

				i += 2;
			}
			else if ((strcmp(argv[i], "-dt") == 0) || (strcmp(argv[i], "--display_time") == 0))
			{
				if (i + 1 <= argc)
				{
					int on;
					on = atoi(argv[i + 1]);

					/* set display time */
					if (((Model_t *)context->m)->SetDisplayTime)
						((Model_t *)context->m)->SetDisplayTime(context, on);
				}

				i += 2;
			}
			else if ((strcmp(argv[i], "-tm") == 0) || (strcmp(argv[i], "--time_mode") == 0))
			{
				if (i + 1 <= argc)
				{
					int twentyFour;
					twentyFour = atoi(argv[i + 1]);

					/* set 12/24 hour mode */
					if (((Model_t *)context->m)->SetTimeMode)
						((Model_t *)context->m)->SetTimeMode(context, twentyFour);
				}

				i += 2;
			}
			else
			{
				usage(context, argv[0], NULL);
			}

			i++;
		}
	}
	else
	{
		usage(context, argv[0], NULL);
	}

	if (((Model_t *)context->m)->Exit)
		((Model_t *)context->m)->Exit(context);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int getKathreinUfs910BoxType()
{
	char vType;
	int vFdBox = open("/proc/boxtype", O_RDONLY);
	read(vFdBox, &vType, 1);
	close(vFdBox);
	return vType == '0' ? 0 : vType == '1' || vType == '3' ? 1 : -1;
}

int getModel()
{
	int vFd = -1;
	const int cSize = 128;
	char vName[129] = "Unknown";
	int vLen = -1;
	eBoxType vBoxType = Unknown;
	vFd = open("/proc/stb/info/model", O_RDONLY);
	vLen = read(vFd, vName, cSize);
	close(vFd);

	if (vLen > 0)
	{
		vName[vLen - 1] = '\0';
		printf("Model: %s\n", vName);

		if (!strncasecmp(vName, "ufs910", 6))
		{
			switch (getKathreinUfs910BoxType())
			{
				case 0:
					vBoxType = Ufs910_1W;
					break;

				case 1:
					vBoxType = Ufs910_14W;
					break;

				default:
					vBoxType = Unknown;
					break;
			}
		}
		else if (!strncasecmp(vName, "ufs922", 6))
			vBoxType = Ufs922;
		else if (!strncasecmp(vName, "ufc960", 6))
			vBoxType = Ufc960;
		else if (!strncasecmp(vName, "tf7700hdpvr", 11))
			vBoxType = Tf7700;
		else if (!strncasecmp(vName, "hl101", 5))
			vBoxType = Hl101;
		else if (!strncasecmp(vName, "vip1-v2", 7))
			vBoxType = Vip2;
		else if (!strncasecmp(vName, "vip2-v1", 7))
			vBoxType = Vip2;
		else if (!strncasecmp(vName, "hdbox", 5))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "octagon1008", 11))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "atevio7500", 10))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "hs7110", 6))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "hs7810a", 7))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "hs7119", 6))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "hs7819", 6))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "atemio520", 9))
			vBoxType = CNBox;
		else if (!strncasecmp(vName, "atemio530", 9))
			vBoxType = CNBox;
		else if (!strncasecmp(vName, "hs5101", 6))
			vBoxType = Hs5101;
		else if (!strncasecmp(vName, "octagon1008", 11))
			vBoxType = Fortis;
		else if (!strncasecmp(vName, "ufs912", 6))
			vBoxType = Ufs912;
		else if (!strncasecmp(vName, "ufs913", 6))
			vBoxType = Ufs912;
		else if (!strncasecmp(vName, "spark", 5))
			vBoxType = Spark;
		else if (!strncasecmp(vName, "spark7162", 9))
			vBoxType = Spark;
		else if ((!strncasecmp(vName, "adb_box", 7)) ||
				(!strncasecmp(vName, "sagemcom88", 10)) ||
				(!strncasecmp(vName, "esi_88", 6)) ||
				(!strncasecmp(vName, "esi88", 5)) ||
				(!strncasecmp(vName, "dsi87", 5)))
			vBoxType = Adb_Box;
		else if ((!strncasecmp(vName, "cuberevo", 8)) ||
				(!strncasecmp(vName, "cuberevo-mini", 13)) ||
				(!strncasecmp(vName, "cuberevo-mini2", 14)) ||
				(!strncasecmp(vName, "cuberevo-mini-fta", 17)) ||
				(!strncasecmp(vName, "cuberevo-250hd", 14)) ||
				(!strncasecmp(vName, "cuberevo-2000hd", 15)) ||
				(!strncasecmp(vName, "cuberevo-9500hd", 15)) ||
				(!strncasecmp(vName, "cuberevo-3000hd", 14)))
			vBoxType = Cuberevo;
		else
			vBoxType = Unknown;
	}

	printf("vBoxType: %d\n", vBoxType);

	return vBoxType;
}

int main(int argc, char *argv[])
{
	eBoxType vBoxType = Unknown;
	Context_t context;

	printf("%s: SW Version %s\n", argv[0], sw_version);

	vBoxType = getModel();

	if (searchModel(&context, vBoxType) != 0)
	{
		printf("Model not found\n");
		return -1;
	}

	printf("Selected Model: %s\n", ((Model_t *)context.m)->Name);

	processCommand(&context, argc, argv);

	exit(reason & 0x07);
}
