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
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include "oled_driver.h"
#include "oled_main.h"
#include "segoe_font.h"
#include "lcddot_font.h"
#include "icomoon_font.h"
#include "oled_freetype.h"

#define vumodel "solo4k" //FIX ME

int init_freetype()
{
	if (FT_Init_FreeType(&freetype_library) != 0)
	{
		printf("%s: cannot init freetype\n", __FUNCTION__);
		return -1;
	}
	if (strcmp(vumodel,"duo2"))
	{
		if (FT_New_Memory_Face(freetype_library, (const FT_Byte*)segoe_font, segoe_font_length, 0, &freetype_face) != 0)
		{
			printf("%s: cannot open base font\n", __FUNCTION__);
			return -1;
		}
		if (FT_New_Memory_Face(freetype_library, (const FT_Byte*)segoe_font, segoe_font_length, 0, &freetype_lcd_face) != 0)
		{
			printf("%s: cannot open base font\n", __FUNCTION__);
			return -1;
		}
	}
	else
	{
		if (FT_New_Memory_Face(freetype_library, (const FT_Byte*)segoe_font, segoe_font_length, 0, &freetype_face) != 0)
		{
			printf("%s: cannot open base font\n", __FUNCTION__);
			return -1;
		}
		if (FT_New_Memory_Face(freetype_library, (const FT_Byte*)lcddot_font, lcddot_font_length, 0, &freetype_lcd_face) != 0)
		{
			printf("%s: cannot open base font\n", __FUNCTION__);
			return -1;
		}
	}
	if (FT_New_Memory_Face(freetype_library, (const FT_Byte*)icomoon_font, icomoon_font_length, 0, &freetype_symbols_face) != 0)
	{
		printf("%s: cannot open symbols font\n", __FUNCTION__);
		return -1;
	}
	freetype_slot = freetype_face->glyph;
	freetype_lcd_slot = freetype_lcd_face->glyph;
	freetype_symbols_slot = freetype_symbols_face->glyph;
	return 0;
}

void deinit_freetype()
{
	FT_Done_Face(freetype_face);
	FT_Done_Face(freetype_lcd_face);
	FT_Done_Face(freetype_symbols_face);
	FT_Done_FreeType(freetype_library);
}

int render_lcd_symbol(int code, int x, int y, int width, int color, int font_size, int align)
{
	if (FT_Set_Char_Size(freetype_symbols_face, font_size * 64, 0, 100, 0))
	{
		printf("%s: cannot set font size\n", __FUNCTION__);
		return -1;
	}

	if (FT_Load_Char(freetype_symbols_face, code, FT_LOAD_RENDER) != 0)
		return -1;

	int offset = 0;
	if (align == TEXT_ALIGN_CENTER)
		offset = (width - freetype_symbols_slot->bitmap.width) / 2;
	else if (align == TEXT_ALIGN_RIGHT)
		offset = width - freetype_symbols_slot->bitmap.width;

	lcd_draw_character(&freetype_symbols_slot->bitmap, offset + x, y, color);

	return 0;
}

int render_lcd_text(const char* text, int x, int y, int width, int color, int font_size, int align)
{
	int i, pen_x, pen_y;
	int num_chars = strlen(text);
	FT_Bitmap bitmaps[MAX_GLYPHS];
	FT_Vector pos[MAX_GLYPHS];

	if (num_chars > MAX_GLYPHS)
		num_chars = MAX_GLYPHS;

	pen_x = x;
	pen_y = y;

	if (strcmp(vumodel,"duo2"))
	{
		if (FT_Set_Char_Size(freetype_lcd_face, font_size * 64, 0, 100, 0))
		{
			printf("%s: cannot set font size\n", __FUNCTION__);
			return -1;
		}
	}
	else
	{
		if (FT_Set_Pixel_Sizes(freetype_lcd_face, 16, 16))
		{
			printf("%s: cannot set font size\n", __FUNCTION__);
			return -1;
		}
	}
	for(i = 0; i < num_chars; i++) {
		if (FT_Load_Char(freetype_lcd_face, text[i], FT_LOAD_RENDER) != 0)
			continue;

		FT_Bitmap_New(&bitmaps[i]);
		FT_Bitmap_Copy(freetype_library, &freetype_lcd_slot->bitmap, &bitmaps[i]);
		pos[i].x = pen_x + freetype_lcd_slot->bitmap_left;
		pos[i].y = pen_y - freetype_lcd_slot->bitmap_top;
		pen_x += freetype_lcd_slot->advance.x >> 6;
	}
	int text_width = (pos[num_chars - 1].x + bitmaps[num_chars - 1].width) - pos[0].x;
	int offset = 0;
	if (align == TEXT_ALIGN_CENTER)
		offset = (width - text_width) / 2;
	else if (align == TEXT_ALIGN_RIGHT)
		offset = width - text_width;
	for(i = 0; i < num_chars; i++)
		lcd_draw_character(&bitmaps[i], offset + pos[i].x, pos[i].y, color);
	return 0;
}

int lcd_print_text_up(const char* text, int color, int font_size, int align)
{
	int up_x = lcd_get_xres() * LCD_UP_X;
	int up_y = lcd_get_yres() * LCD_UP_Y;
	int tmp;
	if (font_size == NULL)
	{
		tmp = lcd_get_xres() * LCD_UP_SIZE;
	}
	else
	{
		tmp = font_size;
	}
	render_lcd_text(text, up_x, up_y, 0, LCD_UP_COLOR, tmp, align);
}

int lcd_print_text_center(const char* text, int color, int font_size, int align)
{
	int center_x = lcd_get_xres() * LCD_CENTER_X;
	int center_y = lcd_get_yres() * LCD_CENTER_Y;
	int tmp;
	if (font_size == NULL)
	{
		tmp = lcd_get_xres() * LCD_CENTER_SIZE;
	}
	else
	{
		tmp = font_size;
	}
	render_lcd_text(text, center_x, center_y, 0, LCD_CENTER_COLOR, tmp, align);
}

int lcd_print_text_down(const char* text, int color, int font_size, int align)
{
	int down_x = lcd_get_xres() * LCD_DOWN_X;
	int down_y = lcd_get_yres() * LCD_DOWN_Y;
	int tmp;
	if (font_size == NULL)
	{
		tmp = lcd_get_xres() * LCD_DOWN_SIZE;
	}
	else
	{
		tmp = font_size;
	}
	render_lcd_text(text, down_x, down_y, 0, LCD_DOWN_COLOR, tmp, align);
}

int lcd_print_text_up_different(const char* text, int color, int font_size, int align)
{
	int up_x_different = lcd_get_xres() * LCD_UP_X_DIFFERENT;
	int up_y_different = lcd_get_yres() * LCD_UP_Y_DIFFERENT;
	int tmp;
	if (font_size == NULL)
	{
		tmp = lcd_get_xres() * LCD_UP_SIZE_DIFFERENT;
	}
	else
	{
		tmp = font_size;
	}
	render_lcd_text(text, up_x_different, up_y_different, 0, LCD_UP_COLOR_DIFFERENT, tmp, align);
}

int lcd_print_text_center_different(const char* text, int color, int font_size, int align)
{
	int center_x_different = lcd_get_xres() * LCD_CENTER_X_DIFFERENT;
	int center_y_different = lcd_get_yres() * LCD_CENTER_Y_DIFFERENT;
	int tmp;
	if (font_size == NULL)
	{
		tmp = lcd_get_xres() * LCD_CENTER_SIZE_DIFFERENT;
	}
	else
	{
		tmp = font_size;
	}
	render_lcd_text(text, center_x_different, center_y_different, 0, LCD_CENTER_COLOR_DIFFERENT, tmp, align);
}

int lcd_print_text_down_different(const char* text, int color, int font_size, int align)
{
	int down_x_different = lcd_get_xres() * LCD_DOWN_X_DIFFERENT;
	int down_y_different = lcd_get_yres() * LCD_DOWN_Y_DIFFERENT;
	int tmp;
	if (font_size == NULL)
	{
		tmp = lcd_get_xres() * LCD_DOWN_SIZE_DIFFERENT;
	}
	else
	{
		tmp = font_size;
	}
	render_lcd_text(text, down_x_different, down_y_different, 0, LCD_DOWN_COLOR_DIFFERENT, tmp, align);
}
