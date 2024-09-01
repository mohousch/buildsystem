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

#ifndef _OLED_DRIVER_H_
#define _OLED_DRIVER_H_

#include <stdint.h>

#include <ft2build.h>
#include FT_FREETYPE_H

typedef enum { false = 0, true = !false } bool;
static int fd = -1, bpp = 0, xres = 0, yres = 0, stride = 0;
static unsigned char * lcd_buffer = NULL;

#define RED(x)   (x >> 16) & 0xff;
#define GREEN(x) (x >> 8) & 0xff;
#define BLUE(x) x & 0xff;

#define LCD_XRES "/proc/stb/lcd/xres"
#define LCD_YRES "/proc/stb/lcd/yres"
#define LCD_BPP "/proc/stb/lcd/bpp"
#define LCD_BRIGHTNESS "/proc/stb/lcd/oled_brightness"
#define FP_BRIGHTNESS "/proc/stb/fp/oled_brightness"

int lcd_open(const char *dev, int mode, int x_res, int y_res);
int lcd_setmode(int mode);
int lcd_brightness(int brightness);
void lcd_setpixel(int x, int y, uint32_t data);
void lcd_draw();
int lcd_clear();
int lcd_get_xres();
int lcd_get_yres();
int lcd_close();
int driver_close();
int driver_start(const char *dev, int mode, int _brightness, int x_res, int y_res);
void lcd_draw_character(FT_Bitmap* bitmap, FT_Int x, FT_Int y, int color);
void lcd_write_text(const char* text);
int lcd_ioctl(const char *io_ctl);
int lcd_deepstandby();

#endif
