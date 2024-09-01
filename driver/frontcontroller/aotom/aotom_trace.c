/*
 * aotom_trace.c
 *
 * (c) 2010 Spider-Team
 * (c) 2011 oSaoYa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <asm/string.h>

#include "aotom_trace.h"

int trace_level = TRACE_ERROR | TRACE_FATAL | TRACE_INFO;

int YWTRACE_Init(void)
{
	return 0;
}

int YWTRACE_Print(const unsigned int level, const char *format, ...)
{
	va_list args;
	int r = 0;
	if (trace_level & level)
	{
		va_start(args, format);
		if (level & (TRACE_ERROR | TRACE_FATAL))
		{
			printk("---------------");
		}
		r = vprintk(format, args);
		va_end(args);
	}
	return r;
}
