/**************************************************************************/
/* Name :   Front Panel Driver Module control tool                        */
/*                                                                        */
/* Author:  Gustav Gans                                                   */
/*                                                                        */
/* Licence: This file is subject to the terms and conditions of the       */
/*          GNU General Public License version 2.                         */
/*                                                                        */
/* 2008-06-10 V1.0  First release                                         */
/* 2008-07-19 V2.0  Added --typematicdelay, --typematicrate and           */
/*                    --keyemulationmode                                  */
/* 2009-01-06 V3.1  Added the --reboot command                            */
/*                  Synced version number with front panel driver module  */
/* 2009-02-08 V3.2  Added GMT offset handling                             */
/* 2009-01-06 V3.7  Added the --resend command                            */
/*                  Synced version number with front panel driver module  */
/* 2009-06-25 V4.0  Added the --allcaps switch                            */
/* 2009-06-29 V4.1  Added the following switches:                         */
/*                    --scrollmode                                        */
/*                    --scrollpause                                       */
/*                    --scrolldelay                                       */
/*                  tffpctl now saves some settings into a file.          */
/*                  --restoresettings                                     */
/* 2011-07-23 V4.2  Added lonkeypress option to keyemulation mode         */
/**************************************************************************/

#define VERSION "V4.2"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <tftypes.h>
#include "frontpanel.h"

#define _FRONTPANELGETTIME              0x00
#define _FRONTPANELSETTIME              0x01
#define _FRONTPANELSYNCSYSTIME          0x02
#define _FRONTPANELCLEARTIMER           0x03
#define _FRONTPANELSETTIMER             0x04
#define _FRONTPANELBRIGHTNESS           0x05
#define _FRONTPANELIRFILTER1            0x06
#define _FRONTPANELIRFILTER2            0x07
#define _FRONTPANELIRFILTER3            0x08
#define _FRONTPANELIRFILTER4            0x09
#define _FRONTPANELPOWEROFF             0x0a
#define _FRONTPANELBOOTREASON           0x0b
#define _FRONTPANELCLEAR                0x0c
#define _FRONTPANELTYPEMATICDELAY       0x0d
#define _FRONTPANELTYPEMATICRATE        0x0e
#define _FRONTPANELKEYEMULATION         0x0f
#define _FRONTPANELREBOOT               0x10
#define _FRONTPANELICON                 0x11
#define _HELP                           0x12
#define _FRONTPANELSYNCFPTIME           0x13
#define _FRONTPANELSETGMTOFFSET         0x14
#define _FRONTPANELRESEND               0x15
#define _FRONTPANELALLCAPS              0x16
#define _FRONTPANELSCROLLMODE           0x17
#define _FRONTPANELSCROLLPAUSE          0x18
#define _FRONTPANELSCROLLDELAY          0x19
#define _FRONTPANELRESTORSETTINGS       0x1a


#define SETTINGSFILE                    "/etc/tffpctl.conf"
#define SETTINGSVERSION                 1

typedef struct
{
	unsigned char Version;
	unsigned char Brightness;
	unsigned char IRFilter1;
	unsigned char IRFilter2;
	unsigned char IRFilter3;
	unsigned char IRFilter4;
	unsigned char AllCaps;
	unsigned char TypematicDelay;
	unsigned char TypematicRate;
	unsigned char KeyemulationMode;
	unsigned char ScrollMode;
	unsigned char ScrollPause;
	unsigned char ScrollDelay;
	//--------------- for future enhancements: settings version 1 ends here
} tSettings;

tSettings Settings;

void Help(void)
{
	printf("Topfield TF7700HDPVR front panel controller " VERSION "\n\n");
	printf("usage: tffpctl [options]\n\n");

	printf("--gettime                    returns the current frontpanel time\n");
	printf("--settime <yyyymmddhhmmss>   set the front panel time\n");
	printf("--syncsystime                sets the system time from the FP time\n");
	printf("--syncfptime                 sets the FP time from the system time\n");
	printf("--setgmtoffset               set the GMT offset from the timezone\n");
	printf("--cleartimers                clears all front panel timers\n");
	printf("--settimer <yyyymmddhhmm>    sets a front panel timer\n");
	printf("--brightness <0 to 5>        adjusts the brightness of the front panel display\n");
	printf("--irfilter1 <0|1>            turns IR mode 1 on or off\n");
	printf("--irfilter2 <0|1>            turns IR mode 2 on or off\n");
	printf("--irfilter3 <0|1>            turns IR mode 3 on or off\n");
	printf("--irfilter4 <0|1>            turns IR mode 4 on or off\n");
	printf("--poweroff                   turns the power off\n");
	printf("--reboot                     initiates a hard reboot\n");
	printf("--bootreason                 returns the boot reason\n");
	printf("--cleardisplay               clears the front panel display\n");
	printf("--resend                     sends the whole display buffer to the VFD\n");
	printf("--allcaps <0|1>              show all chars of the big display in CAPS letters\n");
	printf("--icon <xxxxxxxx_xxxxxxxx_m> defines which icons to displayed\n");

	printf("--typematicdelay <0 to 255>  delay between the first and the repeated keys\n");
	printf("--typematicrate <0 to 255>   delay between the repeated key codes\n");
	printf("                             the typematic rate unit is about 1/9th of a second\n");
	printf("--keyemulationmode <0 to 2>  selects the emulation mode (0=TF7700, 1=UFS910, 2=TF7700LKP)\n");

	printf("--scrollmode <0 to 2>        selects a scroll mode (0=none, 1=once, 2=always)\n");
	printf("--scrollpause <1 to 255>     sets the pause between the first 8 chars and the following scroll (n*100ms)\n");
	printf("--scrolldelay <1 to 255>     sets the delay between 2 text shifts (n*100ms)\n");

	printf("--restoresettings            resends several settings to the frontpanel\n");

	printf("--help                       this screen\n");
}

void LoadSettings(void)
{
	FILE *fp;

	Settings.Version = 0;
	if ((fp = fopen(SETTINGSFILE, "rb")))
	{
		fread(&Settings, sizeof(Settings), 1, fp);
		fclose(fp);
	}

	switch (Settings.Version)
	{
		case 0:
		{
			//Loading failed, use defaults
			Settings.Brightness       =   3; //Medium
			Settings.IRFilter1        =   1; //Mode 1 and 4 active (my remote transmits on mode 4)
			Settings.IRFilter2        =   0;
			Settings.IRFilter3        =   0;
			Settings.IRFilter4        =   1;
			Settings.AllCaps          =   1; //Capital characters on
			Settings.TypematicDelay   =   3;
			Settings.TypematicRate    =   1;
			Settings.KeyemulationMode =   1; //Kathrein mode
			Settings.ScrollMode       =   1; //Scroll once
			Settings.ScrollPause      = 100; //1s pause until start of scroll
			Settings.ScrollDelay      =  10; //Scroll every 100ms
			//no break here
		}
	}
	Settings.Version = SETTINGSVERSION;
}

void SaveSettings(void)
{
	FILE *fp;

	if ((fp = fopen(SETTINGSFILE, "wb")))
	{
		fwrite(&Settings, sizeof(Settings), 1, fp);
		fclose(fp);
	}
	else
	{
		printf("Failed to open %s\n", SETTINGSFILE);
	}
}

int main(int argc, char **argv)
{
	int file_fp;

	if ((file_fp = open("/dev/fpc", O_RDWR)) < 0)
	{
		if ((file_fp = open("/dev/.static/dev/fpc", O_RDWR)) < 0)
		{
			printf("FP: Error - can't open /dev/fpc\n");
			return 0;
		}
	}

	if (argc < 2)
	{
		Help();
		return 0;
	}

	LoadSettings();

	while (1)
	{
		int c;
		int option_index = 0;
		static struct option long_options[] =
		{
			{"gettime",         no_argument      , 0, 0},
			{"settime",         required_argument, 0, 0},
			{"syncsystime",     no_argument      , 0, 0},

			{"cleartimers",     no_argument      , 0, 0},
			{"settimer",        required_argument, 0, 0},

			{"brightness",      required_argument, 0, 0},
			{"irfilter1",       required_argument, 0, 0},
			{"irfilter2",       required_argument, 0, 0},
			{"irfilter3",       required_argument, 0, 0},
			{"irfilter4",       required_argument, 0, 0},
			{"poweroff",        no_argument      , 0, 0},
			{"bootreason",      no_argument      , 0, 0},
			{"cleardisplay",    no_argument      , 0, 0},

			{"typematicdelay",  required_argument, 0, 0},
			{"typematicrate",   required_argument, 0, 0},
			{"keyemulationmode", required_argument, 0, 0},
			{"reboot",          no_argument      , 0, 0},
			{"icon",            required_argument, 0, 0},

			{"help",            no_argument      , 0, 0},

			{"syncfptime",      no_argument      , 0, 0},
			{"setgmtoffset",    no_argument      , 0, 0},

			{"resend",          no_argument      , 0, 0},
			{"allcaps",         required_argument, 0, 0},

			{"scrollmode",      required_argument, 0, 0},
			{"scrollpause",     required_argument, 0, 0},
			{"scrolldelay",     required_argument, 0, 0},

			{"restoresettings", no_argument      , 0, 0},

			{NULL,      0, 0, 0}
		};

		c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1) break;

		switch (c)
		{
			case 0:
			{
				switch (option_index)
				{
					case _FRONTPANELGETTIME:
					{
						frontpanel_ioctl_time fpdata;

						ioctl(file_fp, FRONTPANELGETTIME, &fpdata);
						printf("%d-%2.2d-%2.2d %s %2.2d:%2.2d:%2.2d\n", fpdata.year, fpdata.month, fpdata.day, fpdata.sdow, fpdata.hour, fpdata.min, fpdata.sec);
						break;
					}
					case _FRONTPANELSETTIME:
					{
						frontpanel_ioctl_time fpdata;
						char                  s[8];

						if (strlen(optarg) == 14)
						{
							//yyyymmddhhmmss
							strncpy(s, &optarg[ 0], 4);
							s[4] = '\0';
							fpdata.year  = atoi(s);
							strncpy(s, &optarg[ 4], 2);
							s[2] = '\0';
							fpdata.month = atoi(s);
							strncpy(s, &optarg[ 6], 2);
							s[2] = '\0';
							fpdata.day   = atoi(s);
							strncpy(s, &optarg[ 8], 2);
							s[2] = '\0';
							fpdata.hour  = atoi(s);
							strncpy(s, &optarg[10], 2);
							s[2] = '\0';
							fpdata.min   = atoi(s);
							strncpy(s, &optarg[12], 2);
							s[2] = '\0';
							fpdata.sec   = atoi(s);
							ioctl(file_fp, FRONTPANELSETTIME, &fpdata);
						}
						else
						{
							printf("Invalid settime parameter\n");
						}
						break;
					}
					case _FRONTPANELSYNCSYSTIME:
					{
						ioctl(file_fp, FRONTPANELSYNCSYSTIME, NULL);
						break;
					}
					case _FRONTPANELCLEARTIMER:
					{
						ioctl(file_fp, FRONTPANELCLEARTIMER, NULL);
						break;
					}
					case _FRONTPANELSETTIMER:
					{
						frontpanel_ioctl_time fpdata;
						char                  s[8];

						if (strlen(optarg) == 12)
						{
							//yyyymmddhhmm
							strncpy(s, &optarg[ 0], 4);
							s[4] = '\0';
							fpdata.year  = atoi(s);
							strncpy(s, &optarg[ 4], 2);
							s[2] = '\0';
							fpdata.month = atoi(s);
							strncpy(s, &optarg[ 6], 2);
							s[2] = '\0';
							fpdata.day   = atoi(s);
							strncpy(s, &optarg[ 8], 2);
							s[2] = '\0';
							fpdata.hour  = atoi(s);
							strncpy(s, &optarg[10], 2);
							s[2] = '\0';
							fpdata.min   = atoi(s);
							ioctl(file_fp, FRONTPANELSETTIMER, &fpdata);
						}
						else
						{
							printf("Invalid settimer parameter\n");
						}
						break;
					}
					case _FRONTPANELBRIGHTNESS:
					{
						frontpanel_ioctl_brightness fpdata;

						fpdata.bright = atoi(optarg);
						if (fpdata.bright > 5) fpdata.bright = 5;
						Settings.Brightness = fpdata.bright;
						SaveSettings();
						ioctl(file_fp, FRONTPANELBRIGHTNESS, &fpdata);
						break;
					}
					case _FRONTPANELIRFILTER1:
					{
						frontpanel_ioctl_irfilter fpdata;

						fpdata.onoff = atoi(optarg);
						if (fpdata.onoff > 1) fpdata.onoff = 1;
						Settings.IRFilter1 = fpdata.onoff;
						SaveSettings();
						ioctl(file_fp, FRONTPANELIRFILTER1, &fpdata);
						break;
					}
					case _FRONTPANELIRFILTER2:
					{
						frontpanel_ioctl_irfilter fpdata;

						fpdata.onoff = atoi(optarg);
						if (fpdata.onoff > 1) fpdata.onoff = 1;
						Settings.IRFilter2 = fpdata.onoff;
						SaveSettings();
						ioctl(file_fp, FRONTPANELIRFILTER2, &fpdata);
						break;
					}
					case _FRONTPANELIRFILTER3:
					{
						frontpanel_ioctl_irfilter fpdata;

						fpdata.onoff = atoi(optarg);
						if (fpdata.onoff > 1) fpdata.onoff = 1;
						Settings.IRFilter3 = fpdata.onoff;
						SaveSettings();
						ioctl(file_fp, FRONTPANELIRFILTER3, &fpdata);
						break;
					}
					case _FRONTPANELIRFILTER4:
					{
						frontpanel_ioctl_irfilter fpdata;

						fpdata.onoff = atoi(optarg);
						if (fpdata.onoff > 1) fpdata.onoff = 1;
						Settings.IRFilter4 = fpdata.onoff;
						SaveSettings();
						ioctl(file_fp, FRONTPANELIRFILTER4, &fpdata);
						break;
					}
					case _FRONTPANELPOWEROFF:
					{
						ioctl(file_fp, FRONTPANELPOWEROFF, NULL);
						break;
					}
					case _FRONTPANELREBOOT:
					{
						ioctl(file_fp, FRONTPANELREBOOT, NULL);
						break;
					}
					case _FRONTPANELBOOTREASON:
					{
						frontpanel_ioctl_bootreason fpdata;

						ioctl(file_fp, FRONTPANELBOOTREASON, &fpdata);
						printf("%d\n", fpdata.reason);
						break;
					}
					case _FRONTPANELCLEAR:
					{
						ioctl(file_fp, FRONTPANELCLEAR, NULL);
						break;
					}
					case _FRONTPANELICON:
					{
						frontpanel_ioctl_icons fpdata;
						char                          s[12];

						if (strlen(optarg) == 19)
						{
							//xxxxxxxx_xxxxxxxx_m
							strncpy(s, &optarg[ 0], 8);
							s[8] = '\0';
							fpdata.Icons1    = strtol(s, NULL, 16);
							strncpy(s, &optarg[ 9], 8);
							s[8] = '\0';
							fpdata.Icons2    = strtol(s, NULL, 16);
							strncpy(s, &optarg[18], 1);
							s[1] = '\0';
							fpdata.BlinkMode = strtol(s, NULL, 16) & 0x0f;
							ioctl(file_fp, FRONTPANELICON, &fpdata);
						}
						else
						{
							printf("Invalid icon parameter\n");
						}
						break;
					}
					case _FRONTPANELTYPEMATICDELAY:
					{
						frontpanel_ioctl_typematicdelay fpdata;

						fpdata.TypematicDelay = atoi(optarg);
						Settings.TypematicDelay = fpdata.TypematicDelay;
						SaveSettings();
						ioctl(file_fp, FRONTPANELTYPEMATICDELAY, &fpdata);
						break;
					}
					case _FRONTPANELTYPEMATICRATE:
					{
						frontpanel_ioctl_typematicrate fpdata;

						fpdata.TypematicRate = atoi(optarg);
						Settings.TypematicRate = fpdata.TypematicRate;
						SaveSettings();
						ioctl(file_fp, FRONTPANELTYPEMATICRATE, &fpdata);
						break;
					}
					case _FRONTPANELKEYEMULATION:
					{
						frontpanel_ioctl_keyemulation fpdata;

						fpdata.KeyEmulation = atoi(optarg);
						Settings.KeyemulationMode = fpdata.KeyEmulation;
						SaveSettings();
						ioctl(file_fp, FRONTPANELKEYEMULATION, &fpdata);
						break;
					}
					case _FRONTPANELSYNCFPTIME:
					{
						ioctl(file_fp, FRONTPANELSYNCFPTIME, NULL);
						break;
					}
					case _FRONTPANELSETGMTOFFSET:
					{
						time_t t = time(0);
						struct tm *pTime = localtime(&t);
						ioctl(file_fp, FRONTPANELSETGMTOFFSET, pTime->tm_gmtoff);
						break;
					}
					case _FRONTPANELRESEND:
					{
						ioctl(file_fp, FRONTPANELRESEND, NULL);
						break;
					}
					case _FRONTPANELALLCAPS:
					{
						frontpanel_ioctl_allcaps fpdata;

						fpdata.AllCaps = atoi(optarg);
						if (fpdata.AllCaps > 1) fpdata.AllCaps = 1;
						Settings.AllCaps = fpdata.AllCaps;
						SaveSettings();
						ioctl(file_fp, FRONTPANELALLCAPS, &fpdata);
						break;
					}
					case _FRONTPANELSCROLLMODE:
					{
						frontpanel_ioctl_scrollmode fpdata;

						fpdata.ScrollMode = atoi(optarg);
						if (fpdata.ScrollMode > 2) fpdata.ScrollMode = 2;
						Settings.ScrollMode = fpdata.ScrollMode;
						fpdata.ScrollPause = Settings.ScrollPause;
						fpdata.ScrollDelay = Settings.ScrollDelay;
						SaveSettings();
						ioctl(file_fp, FRONTPANELSCROLLMODE, &fpdata);
						break;
					}
					case _FRONTPANELSCROLLPAUSE:
					{
						frontpanel_ioctl_scrollmode fpdata;

						fpdata.ScrollPause = atoi(optarg);
						Settings.ScrollPause = fpdata.ScrollPause;
						fpdata.ScrollMode = Settings.ScrollMode;
						fpdata.ScrollDelay = Settings.ScrollDelay;
						SaveSettings();
						ioctl(file_fp, FRONTPANELSCROLLMODE, &fpdata);
						break;
					}
					case _FRONTPANELSCROLLDELAY:
					{
						frontpanel_ioctl_scrollmode fpdata;

						fpdata.ScrollDelay = atoi(optarg);
						Settings.ScrollDelay = fpdata.ScrollDelay;
						fpdata.ScrollMode = Settings.ScrollMode;
						fpdata.ScrollPause = Settings.ScrollPause;
						SaveSettings();
						ioctl(file_fp, FRONTPANELSCROLLMODE, &fpdata);
						break;
					}
					case _FRONTPANELRESTORSETTINGS:
					{
						frontpanel_ioctl_brightness     Brightness;
						frontpanel_ioctl_irfilter       IRFilter;
						frontpanel_ioctl_typematicdelay TypematicDelay;
						frontpanel_ioctl_typematicrate  TypematicRate;
						frontpanel_ioctl_keyemulation   KeyemulationMode;
						frontpanel_ioctl_allcaps        AllCaps;
						frontpanel_ioctl_scrollmode     ScrollMode;

						//Resend all settings to the front panel
						Brightness.bright = Settings.Brightness;
						ioctl(file_fp, FRONTPANELBRIGHTNESS, &Brightness);

						IRFilter.onoff = Settings.IRFilter1;
						ioctl(file_fp, FRONTPANELIRFILTER1, &IRFilter);

						IRFilter.onoff = Settings.IRFilter2;
						ioctl(file_fp, FRONTPANELIRFILTER2, &IRFilter);

						IRFilter.onoff = Settings.IRFilter3;
						ioctl(file_fp, FRONTPANELIRFILTER3, &IRFilter);

						IRFilter.onoff = Settings.IRFilter4;
						ioctl(file_fp, FRONTPANELIRFILTER4, &IRFilter);

						TypematicDelay.TypematicDelay = Settings.TypematicDelay;
						ioctl(file_fp, FRONTPANELTYPEMATICDELAY, &TypematicDelay);

						TypematicRate.TypematicRate = Settings.TypematicRate;
						ioctl(file_fp, FRONTPANELTYPEMATICRATE, &TypematicRate);

						KeyemulationMode.KeyEmulation = Settings.KeyemulationMode;
						ioctl(file_fp, FRONTPANELKEYEMULATION, &KeyemulationMode);

						AllCaps.AllCaps = Settings.AllCaps;
						ioctl(file_fp, FRONTPANELALLCAPS, &AllCaps);

						ScrollMode.ScrollMode   = Settings.ScrollMode;
						ScrollMode.ScrollPause  = Settings.ScrollPause;
						ScrollMode.ScrollDelay  = Settings.ScrollDelay;
						ioctl(file_fp, FRONTPANELSCROLLMODE, &ScrollMode);
						break;
					}
					case _HELP:
					{
						Help();
						break;
					}
				}
				break;
			}
			case '?':
			{
				break;
			}
			default:
			{
				printf("?? getopt returned character code 0%x ??\n", c);
			}
		}
	}

	if (optind < argc)
	{
		printf("non-option ARGV-elements: ");
		while (optind < argc)
		{
			printf("%s ", argv[optind++]);
		}
		printf("\n");
	}
	close(file_fp);

	return 0;
}
