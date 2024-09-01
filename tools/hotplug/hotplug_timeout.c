/*
    hotplug_timeout.c

    Copyright (C) 2007 Andreas Oberritter

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License 2.0 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <sys/time.h>
#include <time.h>
#include "hotplug_timeout.h"

static unsigned long timeout_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void timeout_init(struct timeout *t, unsigned long ms)
{
	t->val = timeout_ms() + ms;
}

int timeout_exceeded(struct timeout *t)
{
	return !((long)timeout_ms() - (long)t->val < 0);
}

