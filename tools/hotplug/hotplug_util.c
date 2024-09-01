/*
 * hotplug_util.c
 *
 * Simple utility functions needed by some of the subsystems.
 *
 * Copyright (C) 2001,2005 Greg Kroah-Hartman <greg@kroah.com>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stddef.h>	/* for NULL */
#include <string.h>
#include <stdlib.h>	/* for exit() */
#include <unistd.h>
#include "hotplug_util.h"
#include "udev.h"

/**
 * split_2values
 *
 * takes a string of format "xxxx:yyyy" and figures out the
 * values for xxxx and yyyy
 *
 */
int split_2values(const char *string, int base, unsigned int *value1, unsigned int *value2)
{
	char buffer[200];
	char *temp1;
	const char *temp2;

	dbg("string = %s", string);

	if (string == NULL)
		return -1;
	/* dietLibc doesn't have strnlen yet :( */
	/* if (strnlen (string, sizeof(buffer)) >= sizeof(buffer)) */
	if (strlen(string) >= sizeof(buffer))
		return -1;

	/* pick up the first number */
	temp1 = &buffer[0];
	temp2 = string;
	while (1)
	{
		if (*temp2 == 0x00)
			break;
		if (*temp2 == ':')
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value1 = strtoul(buffer, NULL, base);
	dbg("buffer = %s", &buffer[0]);
	dbg("value1 = %d", *value1);

	if (*temp2 == 0x00)
	{
		/* string is ended, not good */
		return -1;
	}

	/* get the second number */
	temp1 = &buffer[0];
	++temp2;
	while (1)
	{
		if (*temp2 == 0x00)
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value2 = strtoul(buffer, NULL, base);
	dbg("buffer = %s", &buffer[0]);
	dbg("value2 = %d", *value2);

	return 0;
}


/**
 * split_3values
 *
 * takes a string of format "xxxx/yyyy/zzzz" and figures out the
 * values for xxxx, yyyy, and zzzz
 *
 */
int split_3values(const char *string, int base, unsigned int *value1, unsigned int *value2, unsigned int *value3)
{
	char buffer[200];
	char *temp1;
	const char *temp2;

	dbg("string = %s", string);

	if (string == NULL)
		return -1;
	/* dietLibc doesn't have strnlen yet :( */
	/* if (strnlen (string, sizeof(buffer)) >= sizeof(buffer)) */
	if (strlen(string) >= sizeof(buffer))
		return -1;

	/* pick up the first number */
	temp1 = &buffer[0];
	temp2 = string;
	while (1)
	{
		if (*temp2 == 0x00)
			break;
		if (*temp2 == '/')
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value1 = strtoul(buffer, NULL, base);
	dbg("buffer = %s", &buffer[0]);
	dbg("value1 = %d", *value1);

	if (*temp2 == 0x00)
	{
		/* string is ended, not good */
		return -1;
	}

	/* get the second number */
	temp1 = &buffer[0];
	++temp2;
	while (1)
	{
		if (*temp2 == 0x00)
			break;
		if (*temp2 == '/')
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value2 = strtoul(buffer, NULL, base);
	dbg("buffer = %s", &buffer[0]);
	dbg("value2 = %d", *value2);

	if (*temp2 == 0x00)
	{
		/* string is ended, not good */
		return -1;
	}

	/* get the third number */
	temp1 = &buffer[0];
	++temp2;
	while (1)
	{
		if (*temp2 == 0x00)
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value3 = strtoul(buffer, NULL, base);
	dbg("buffer = %s", &buffer[0]);
	dbg("value3 = %d", *value3);

	return 0;
}

int modprobe(const char *module_name, bool insert)
{
	unsigned int i = 0;
	char *argv[4];

	argv[i++] = "/sbin/modprobe";
	if (!insert)
		argv[i++] = "-r";
	argv[i++] = (char *)module_name;
	argv[i++] = NULL;
	dbg("%sloading module %s", insert ? "" : "un", module_name);
	switch (fork())
	{
		case 0:
			/* we are the child, so lets run the program */
			execv("/sbin/modprobe", argv);
			exit(0);
			break;
		case (-1):
			dbg("fork failed.");
			break;
		default:
			break;
	}
	return 0;
}

