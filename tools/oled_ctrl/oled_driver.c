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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "lcd-ks0713.h"
#include "oled_driver.h"

int lcd_read_value(const char *filename)
{
	int value = 0;
	FILE *fd = fopen(filename, "r");
	if (fd) {
		int tmp;
		if (fscanf(fd, "%x", &tmp) == 1)
			value = tmp;
		fclose(fd);
	}
	else
	{
		value = -1;
	}
	return value;
}

int lcd_open(const char *dev, int mode, int x_res, int y_res)
{
	fd = open(dev, O_RDWR);
	if (fd == -1)
	{
		printf("%s: cannot open lcd device\n", __FUNCTION__);
		return -1;
	}
	if (lcd_setmode(mode) < 0)
	{
		return -1;
	}
	if (x_res == 0)
	{
		xres = lcd_read_value(LCD_XRES);
	}
	else
	{
		xres = x_res;
	}
	if (xres == 0)
	{
		printf("%s: cannot read lcd xres resolution\n", __FUNCTION__);
		return -1;
	}
	if (y_res == 0)
	{
		yres = lcd_read_value(LCD_YRES);
	}
	else
	{
		yres = y_res;
	}
	if (yres == 0)
	{
		printf("%s: cannot read lcd yres resolution\n", __FUNCTION__);
	}
	bpp = lcd_read_value(LCD_BPP);
	if (bpp == 0)
	{
		printf("%s: cannot read lcd bpp\n", __FUNCTION__);
		return -1;
	}
	stride = xres * (bpp / 8);
	return 0;
}

int lcd_setmode(int mode)
{
	int tmp;
	if (mode == 0)
	{
		tmp = LCD_MODE_BIN;
	}
	else if (mode == 1)
	{
		tmp = LCD_MODE_ASC;
	}
	else
	{
		printf("%s: failed to read lcd mode\n", __FUNCTION__);
		return -1;
	}
	if (ioctl(fd, LCD_IOCTL_ASC_MODE, &tmp))
	{
		printf("%s: failed to set lcd mode\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

int lcd_brightness(int brightness)
{
	int value = 0;
	switch(brightness) {
		case 0:
			value = 0;
			break;
		case 1:
			value = 25;
			break;
		case 2:
			value = 51;
			break;
		case 3:
			value = 76;
			break;
		case 4:
			value = 102;
			break;
		case 5:
			value = 127;
			break;
		case 6:
			value = 153;
			break;
		case 7:
			value = 178;
			break;
		case 8:
			value = 204;
			break;
		case 9:
			value = 229;
			break;
		case 10:
			value = 255;
			break;
	}

	FILE *f = fopen(LCD_BRIGHTNESS, "w");
	if (!f)
		f = fopen(FP_BRIGHTNESS, "w");
	if (f)
	{
		if (fprintf(f, "%d", value) == 0)
			printf("%s: write %s failed!! (%m)\n", __FUNCTION__, LCD_BRIGHTNESS);
		fclose(f);
	}
	return 0;
}

void lcd_setpixel(int x, int y, uint32_t data)
{
	if (x >= xres || y >= yres)
		return;

	uint32_t red, green, blue;
	blue = (data & 0x000000FF) >> 0;
	green = (data & 0x0000FF00) >> 8;
	red = (data & 0x00FF0000) >> 16;

	int location = (y * xres + x) * 4;

	lcd_buffer[location+0]=blue;
	lcd_buffer[location+1]=green;
	lcd_buffer[location+2]=red;
	lcd_buffer[location+3]=0xff;
}

void lcd_draw()
{
	write(fd, lcd_buffer, yres * stride);
}

int lcd_clear()
{
	if (ioctl(fd,LCD_IOCTL_CLEAR) < 0)
	{
		printf("%s: cannot clear lcd device\n", __FUNCTION__);
		return -1;
	}
	lcd_draw();
	return 0;
}

int lcd_get_xres()
{
	return xres;
}

int lcd_get_yres()
{
	return yres;
}

int lcd_close()
{
	if (lcd_buffer)
	{
		free(lcd_buffer);
		lcd_buffer = 0;
	}
	if (-1 != fd)
	{
		close(fd);
		fd=-1;
	}
	return 0;
}

int driver_close()
{
	if (lcd_close() < 0);
	{
		printf("%s: failed to close lcd\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

int driver_start(const char *dev, int mode, int user_brightness, int x_res, int y_res)
{
	if (lcd_open(dev, mode, x_res, y_res) < 0)
	{
		return -1;
	}
	lcd_buffer = (unsigned char *)malloc(yres * stride);
	if (lcd_buffer)
		memset(lcd_buffer, 0, yres * stride);

	if (lcd_buffer == NULL) {
		printf("%s: lcd_buffer could not be allocated: malloc() failed\n", __FUNCTION__);
		return -1;
	}
        int tmp;
        tmp = lcd_read_value(LCD_BRIGHTNESS);
        if (tmp < 0)
                tmp = lcd_read_value(FP_BRIGHTNESS);
	if (tmp == 0)
	{
		if (lcd_brightness(user_brightness) < 0)
		{
			printf("%s: failed to set brightness\n", __FUNCTION__);
			return -1;
		}
	}
	lcd_clear();
	return 0;
}

void lcd_draw_character(FT_Bitmap* bitmap, FT_Int x, FT_Int y, int color)
{
	int i, j, z = 0;
	long int location = 0;
	unsigned char red = RED(color);
	unsigned char green = GREEN(color);
	unsigned char blue = BLUE(color);

	red = (red >> 3) & 0x1f;
	green = (green >> 3) & 0x1f;
	blue = (blue >> 3) & 0x1f;

	for (i = y; i < y + bitmap->rows; i++) {
		for (j = x; j < x + bitmap->width; j++) {
			if (i < 0 || j < 0 || i > yres || j > xres) {
				z++;
				continue;
			}
			if (bitmap->buffer[z] != 0x00) {
				location = (j * (bpp / 8)) +
					(i * stride);

				if (bpp == 32) {
					lcd_buffer[location] = RED(color);
					lcd_buffer[location + 1] = GREEN(color);
					lcd_buffer[location + 2] = BLUE(color) ;
					lcd_buffer[location + 3] = 0xff;
				} else {
					lcd_buffer[location] = red << 3 | green >> 2;
					lcd_buffer[location + 1] = green << 6 | blue << 1;
				}
			}
			// vusolo4k needs alpha channel
			if (bpp == 32)
				lcd_buffer[location + 3] = 0xff;
			z++;
		}
	}
}

void lcd_write_text(const char* text)
{
	if(fd < 0)
		return;

	write(fd, text, strlen(text));
}

int lcd_ioctl(const char *io_ctl)
{
        if (ioctl(fd, io_ctl) < 0)
        {
                printf("%s: command %s failed\n", __FUNCTION__, io_ctl);
                return -1;
        }
        lcd_draw();
        return 0;
}

int lcd_deepstandby()
{
#define LCD_DEEPSTANDBY 0x123456 // 0x34 0x56 0x12 to send
	if (ioctl(fd, LCD_DEEPSTANDBY) < 0)
	{
		printf("%s: command deep standby failed\n", __FUNCTION__);
		return -1;
	}
	return 0;
}
