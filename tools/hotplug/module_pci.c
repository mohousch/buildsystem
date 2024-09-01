/*
 * module_pci.c
 *
 * Loads a pci driver based on the pci hotplug environment variables.
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
#include <stdio.h>
#include <stdlib.h>
#include "hotplug_util.h"
#include "module_pci.h"
#include "udev.h"

int pci_add(void)
{
	char pci_string[256];
	char *class_env;
	char *id_env;
	char *subsys_env;
	int error;
	unsigned int vendor;
	unsigned int device;
	unsigned int subvendor;
	unsigned int subdevice;
	unsigned int class;
	unsigned char baseclass, subclass, interface;

	id_env = getenv("PCI_ID");
	subsys_env = getenv("PCI_SUBSYS_ID");
	class_env = getenv("PCI_CLASS");
	if ((id_env == NULL) ||
			(subsys_env == NULL) ||
			(class_env == NULL))
	{
		dbg("missing an environment variable, aborting.");
		return 1;
	}

	error = split_2values(id_env, 16, &vendor, &device);
	if (error)
		return error;
	error = split_2values(subsys_env, 16, &subvendor, &subdevice);
	if (error)
		return error;
	class = strtoul(class_env, NULL, 16);

	baseclass = (unsigned char)(class >> 16);
	subclass = (unsigned char)(class >> 8);
	interface = (unsigned char)class;

	strcpy(pci_string, "pci:");
	sprintf(pci_string + strlen(pci_string), "v%08X", vendor);
	sprintf(pci_string + strlen(pci_string), "d%08X", device);
	sprintf(pci_string + strlen(pci_string), "sv%08X", subvendor);
	sprintf(pci_string + strlen(pci_string), "sd%08X", subdevice);
	sprintf(pci_string + strlen(pci_string), "bc%02X", baseclass);
	sprintf(pci_string + strlen(pci_string), "sc%02X", subclass);
	sprintf(pci_string + strlen(pci_string), "i%02X", interface);

	return modprobe(pci_string, true);
}

