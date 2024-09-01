/*
 *   Copyright (C) redblue 2018
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include "oled_main.h"
#include "oled_freetype.h"
#include "oled_driver.h"

typedef struct
{
	char *arg;
	char *arg_long;
	char *arg_description;
} tArgs;

tArgs vArgs[] =
{
	{ "-b", " --setBrightness		", "Args: brightness\n\tSet oled brightness" },
	{ "-c", " --clear			", "Args: No argumens\n\tClear oled display" },
        { "-d", " --deepStandby		", "Args: No argumens\n\tEnter deep standby" },
	{ "-tu", " --setTextUp		", "Args: text\n\tSet text to oled in up" },
	{ "-tc", " --setTextCenter		", "Args: text\n\tSet text to oled in center" },
	{ "-td", " --setTextDown		", "Args: text\n\tSet text to oled in down" },
	{ "-tud", " --setTextUpDifferent	", "Args: text\n\tSet text to oled in up" },
	{ "-tcd", " --setTextCenterDifferent	", "Args: text\n\tSet text to oled in center" },
	{ "-tdd", " --setTextDownDifferent	", "Args: text\n\tSet text to oled in down" },
	{ NULL, NULL, NULL }
};

void usage(char *prg, char *cmd)
{
	int i;
	/* or printout a default usage */
	fprintf(stderr, "Oled control tool, version 1.00 (VU ARM)\n");
	fprintf(stderr, "General usage:\n\n");
	fprintf(stderr, "%s argument [optarg1] [optarg2]\n", prg);

	for (i = 0; ; i++)
	{
		if (vArgs[i].arg == NULL)
			break;
		if ((cmd == NULL) || (strcmp(cmd, vArgs[i].arg) == 0) || (strstr(vArgs[i].arg_long, cmd) != NULL))
			fprintf(stderr, "%s %s %s\n", vArgs[i].arg, vArgs[i].arg_long, vArgs[i].arg_description);
	}
	exit(1);
}

int main(int argc, char *argv[])
{
	driver_start(LCD_DEVICE, LCD_BIN_MODE, LCD_MY_BRIGHTNESS, LCD_MY_XRES, LCD_MY_YRES);
	init_freetype();
	int i;
	if (argc > 1)
	{
		i = 1;
		while (i < argc)
		{
			if ((strcmp(argv[i], "-b") == 0) || (strcmp(argv[i], "--setBrightness") == 0))
			{
				if (i + 1 <= argc)
				{
					int brightness;
					if (argv[i + 1] == NULL)
					{
						fprintf(stderr, "Missing brightness value\n");
						usage(argv[0], NULL);
					}
					brightness = atoi(argv[i + 1]);
					if (brightness < 0 || brightness > 10)
					{
						fprintf(stderr, "Brightness value out of range\n");
                                        	usage(argv[0], NULL);
					}
					/* set display brightness */
					lcd_brightness(brightness);
				}
				i += 1;
			}
			else if ((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--clear") == 0))
			{
				/* clear the display */
				lcd_clear();
			}
			else if ((strcmp(argv[i], "-d") == 0) || (strcmp(argv[i], "--deepStandby") == 0))
			{
				/* enter in deep standby */
				lcd_deepstandby();
			}
			else if ((strcmp(argv[i], "-tu") == 0) || (strcmp(argv[i], "--setTextUp") == 0))
			{
				if (i + 1 <= argc)
				{
					const char *text;
					if (argv[i + 1] == NULL)
					{
						fprintf(stderr, "Missing text value\n");
						usage(argv[0], NULL);
					}
					text = argv[i + 1];
					/* set display text */
					lcd_print_text_up(text, LCD_UP_COLOR, NULL, TEXT_ALIGN_CENTER);
					lcd_draw();
				}
				i += 1;
			}
			else if ((strcmp(argv[i], "-tc") == 0) || (strcmp(argv[i], "--setTextCenter") == 0))
			{
				if (i + 1 <= argc)
				{
					const char *text;
					if (argv[i + 1]== NULL)
					{
						fprintf(stderr, "Missing text value\n");
						usage(argv[0], NULL);
					}
					text = argv[i + 1];
					/* set display text */
					lcd_print_text_center(text, LCD_CENTER_COLOR, NULL, TEXT_ALIGN_CENTER);
					lcd_draw();
				}
				i += 1;
			}
			else if ((strcmp(argv[i], "-td") == 0) || (strcmp(argv[i], "--setTextDown") == 0))
			{
				if (i + 1 <= argc)
				{
					const char *text;
					if (argv[i + 1] == NULL)
					{
						fprintf(stderr, "Missing text value\n");
						usage(argv[0], NULL);
					}
					text = argv[i + 1];
					/* set display text */
					lcd_print_text_down(text, LCD_DOWN_COLOR, NULL, TEXT_ALIGN_CENTER);
					lcd_draw();
				}
				i += 1;
			}
			else if ((strcmp(argv[i], "-tud") == 0) || (strcmp(argv[i], "--setTextUpDifferent") == 0))
			{
				if (i + 1 <= argc)
				{
					const char *text;
					if (argv[i + 1] == NULL)
					{
						fprintf(stderr, "Missing text value\n");
						usage(argv[0], NULL);
					}
					text = argv[i + 1];
					/* set display text */
					lcd_print_text_up_different(text, LCD_UP_COLOR_DIFFERENT, NULL, TEXT_ALIGN_CENTER);
					lcd_draw();
				}
				i += 1;
			}
			else if ((strcmp(argv[i], "-tcd") == 0) || (strcmp(argv[i], "--setTextUpDifferent") == 0))
			{
				if (i + 1 <= argc)
				{
					const char *text;
					if (argv[i + 1] == NULL)
					{
						fprintf(stderr, "Missing text value\n");
						usage(argv[0], NULL);
					}
					text = argv[i + 1];
					/* set display text */
					lcd_print_text_center_different(text, LCD_CENTER_COLOR_DIFFERENT, NULL, TEXT_ALIGN_CENTER);
					lcd_draw();
				}
				i += 1;
			}
			else if ((strcmp(argv[i], "-tdd") == 0) || (strcmp(argv[i], "--setTextDownDifferent") == 0))
			{
				if (i + 1 <= argc)
				{
					const char *text;
					if (argv[i + 1] == NULL)
					{
						fprintf(stderr, "Missing text value\n");
						usage(argv[0], NULL);
					}
					text = argv[i + 1];
					/* set display text */
					lcd_print_text_down_different(text, LCD_DOWN_COLOR_DIFFERENT, NULL, TEXT_ALIGN_CENTER);
					lcd_draw();
				}
				i += 1;
			}
			else
			{
				usage(argv[0], NULL);
			}
			i++;
		}
	}
	else
	{
		usage(argv[0], NULL);
	}
	return 0;
}
