/*
    hotplug_pidfile.c

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

#include <stdarg.h>
#include <stdio.h>
#include "hotplug_pidfile.h"
#include "udev_sysdeps.h"

static void pidfile_set_name(char *filename, size_t size, const char *fmt, va_list ap)
{
	strlcpy(filename, "/var/run/", size);
	vsnprintf(&filename[strlen(filename)], size - strlen(filename), fmt, ap);
	strlcat(filename, ".pid", size);
}

int pidfile_read(pid_t *pid, const char *fmt, ...)
{
	char filename[FILENAME_MAX];
	va_list ap;
	FILE *f;
	int ret;

	va_start(ap, fmt);
	pidfile_set_name(filename, sizeof(filename), fmt, ap);
	va_end(ap);

	f = fopen(filename, "r");
	if (f == NULL)
	{
		perror(filename);
		return -1;
	}

	ret = fscanf(f, "%d\n", pid);
	fclose(f);

	return (ret == 1) ? 0 : -1;
}

int pidfile_write(pid_t pid, const char *fmt, ...)
{
	char filename[FILENAME_MAX];
	va_list ap;
	FILE *f;

	va_start(ap, fmt);
	pidfile_set_name(filename, sizeof(filename), fmt, ap);
	va_end(ap);

	f = fopen(filename, "w");
	if (f == NULL)
	{
		perror(filename);
		return -1;
	}

	fprintf(f, "%d\n", pid);
	fclose(f);

	return 0;
}

int pidfile_unlink(const char *fmt, ...)
{
	char filename[FILENAME_MAX];
	va_list ap;

	va_start(ap, fmt);
	pidfile_set_name(filename, sizeof(filename), fmt, ap);
	va_end(ap);

	return unlink(filename);
}

