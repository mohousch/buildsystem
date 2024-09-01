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

/*
 * Based on https://github.com/oe-alliance/openmultiboot
 *
 * Based on https://github.com/Duckbox-Developers/apps/blob/master/tools/fp_control/fp_control.c
 */

/*
 * How to use?
 *
 * Example:
 *
 * oled -tud BlaBlaBla -tdd BlaBlaBla
 *
 * -------------
 *   BlaBlaBla
 *
 *   BlaBlaBla
 * -------------
 */

/*
 * to fix compiler warnings, dbox header
 */

#ifndef _MAIN_OLED_H_
#define _MAIN_OLED_H_

#include "lcd-ks0713.h"

#define LCD_UP_X 0.5 // 50% of display width
#define LCD_UP_Y 0.30 // like the X axis (same margin)
#define LCD_UP_SIZE 0.10 // 10% of display width
#define LCD_UP_COLOR 0xffffffff

#define LCD_CENTER_X 0.5 // 50% of display width
#define LCD_CENTER_Y 0.55 // 55% of display width (keep proportion with x axis)
#define LCD_CENTER_SIZE 0.10 // 10% of display width
#define LCD_CENTER_COLOR 0xffffffff

#define LCD_DOWN_X 0.5 // 50% of display width
#define LCD_DOWN_Y 0.80 // 80% like the X axis (same margin)
#define LCD_DOWN_SIZE 0.10 // 10% of display width
#define LCD_DOWN_COLOR 0xffffffff

#define LCD_UP_X_DIFFERENT 0.5 // 50% of display width
#define LCD_UP_Y_DIFFERENT 0.35 // 35% like the X axis (same margin)
#define LCD_UP_SIZE_DIFFERENT 0.13 // 13% of display width
#define LCD_UP_COLOR_DIFFERENT 0xffffffff

#define LCD_CENTER_X_DIFFERENT 0.5 // 50% of display width
#define LCD_CENTER_Y_DIFFERENT 0.55 // 55% of display width (keep proportion with x axis)
#define LCD_CENTER_SIZE_DIFFERENT 0.15 // 15% of display width
#define LCD_CENTER_COLOR_DIFFERENT 0xffffffff

#define LCD_DOWN_X_DIFFERENT 0.5 // 50% of display width
#define LCD_DOWN_Y_DIFFERENT 0.75 // 75% like the X axis (same margin)
#define LCD_DOWN_SIZE_DIFFERENT 0.13 // 13% of display width
#define LCD_DOWN_COLOR_DIFFERENT 0xffffffff

#define LCD_DEVICE "/dev/oled0"

#define LCD_MY_BRIGHTNESS 5 // 0 = use last brightnes (0-10)
#define LCD_MY_XRES 0 // 0 = get it from driver
#define LCD_MY_YRES 0 // 0 = get it from driver
#define LCD_ASC_MODE LCD_MODE_ASC
#define LCD_BIN_MODE LCD_MODE_BIN

#endif
