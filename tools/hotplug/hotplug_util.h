/*
 * hotplug_util.h
 *
 * A version of /sbin/hotplug that is not a script.
 *
 * Why?  Think initrd :)
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

#ifndef HOTPLUG_UTIL_H
#define HOTPLUG_UTIL_H

#include <stdbool.h>

#define ADD_STRING	"add"
#define REMOVE_STRING	"remove"

int split_3values(const char *string, int base, unsigned int *value1, unsigned int *value2, unsigned int *value3);
int split_2values(const char *string, int base, unsigned int *value1, unsigned int *value2);
int modprobe(const char *module_name, bool insert);

#endif
