/*
 * module_ieee1394.c
 *
 * Loads an ieee1394 driver based on the ieee1394 hotplug environment variables.
 *
 * Copyright (C) 2005 Pozsár Balázs <pozsy@uhulinux.hu>
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
#include <stdio.h>
#include <stdlib.h>
#include "hotplug_util.h"
#include "module_ieee1394.h"
#include "udev.h"

int ieee1394_add(void)
{
	char ieee1394_string[256];
	char *vendor_env;
	char *model_env;
	char *specifier_env;
	char *version_env;
	int error;
	unsigned long vendor;
	unsigned long model;
	unsigned long specifier;
	unsigned long version;

	vendor_env = getenv("VENDOR_ID");
	model_env = getenv("MODEL_ID");
	specifier_env = getenv("SPECIFIER_ID");
	version_env = getenv("VERSION");
	dbg("VENDOR_ID='%s', MODEL_ID='%s' SPECIFIER_ID='%s' VERSION='%s'", vendor_env, model_env, specifier_env, version_env);
	if ((vendor_env == NULL) ||
			(model_env == NULL) ||
			(specifier_env == NULL) ||
			(version_env == NULL))
	{
		dbg("missing an environment variable, aborting.");
		return 1;
	}

	vendor    = strtoul(vendor_env, NULL, 16);
	model     = strtoul(model_env, NULL, 16);
	specifier = strtoul(specifier_env, NULL, 16);
	version   = strtoul(version_env, NULL, 16);

	strcpy(ieee1394_string, "ieee1394:");
	sprintf(ieee1394_string + strlen(ieee1394_string), "ven%08lX", vendor);
	sprintf(ieee1394_string + strlen(ieee1394_string), "mo%08lX", model);
	sprintf(ieee1394_string + strlen(ieee1394_string), "sp%08lX", specifier);
	sprintf(ieee1394_string + strlen(ieee1394_string), "ver%08lX", version);

	return modprobe(ieee1394_string, true);
}

