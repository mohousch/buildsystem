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

#ifndef _OLED_FREETYPE_H_
#define _OLED_FREETYPE_H_

#include <ft2build.h>
#include FT_FREETYPE_H

#define TEXT_ALIGN_LEFT 0
#define TEXT_ALIGN_CENTER 1
#define TEXT_ALIGN_RIGHT 2

#define MAX_GLYPHS 255

static FT_Library freetype_library;
static FT_Face freetype_face;
static FT_Face freetype_lcd_face;
static FT_Face freetype_symbols_face;
static FT_GlyphSlot freetype_slot;
static FT_GlyphSlot freetype_lcd_slot;
static FT_GlyphSlot freetype_symbols_slot;

int init_freetype();
void deinit_freetype();
int render_lcd_symbol(int code, int x, int y, int width, int color, int font_size, int align);
int render_lcd_text(const char* text, int x, int y, int width, int color, int font_size, int align);
int lcd_print_text_up(const char* text, int color, int font_size, int align);
int lcd_print_text_center(const char* text, int color, int font_size, int align);
int lcd_print_text_down(const char* text, int color, int font_size, int align);
int lcd_print_text_up_different(const char* text, int color, int font_size, int align);
int lcd_print_text_center_different(const char* text, int color, int font_size, int align);
int lcd_print_text_down_different(const char* text, int color, int font_size, int align);

#endif
