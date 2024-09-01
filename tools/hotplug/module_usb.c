/*
 * module_usb.c
 *
 * Loads a usb driver based on the usb hotplug environment variables.
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
#include "module_usb.h"
#include "udev.h"

int usb_add(void)
{
	char usb_string[256];
	char *product_env;
	char *type_env;
	char *interface_env;
	int error;
	unsigned int idVendor;
	unsigned int idProduct;
	unsigned int bcdDevice;
	unsigned int device_class;
	unsigned int device_subclass;
	unsigned int device_protocol;
	unsigned int interface_class;
	unsigned int interface_subclass;
	unsigned int interface_protocol;

	product_env = getenv("PRODUCT");
	type_env = getenv("TYPE");
	dbg("PRODUCT='%s', TYPE = '%s'", product_env, type_env);
	if ((product_env == NULL) ||
			(type_env == NULL))
	{
		dbg("missing an environment variable, aborting.");
		return 1;
	}
	strcpy(usb_string, "usb:");

	error = split_3values(product_env, 16, &idVendor, &idProduct, &bcdDevice);
	if (error)
		return error;
	error = split_3values(type_env, 10, &device_class, &device_subclass, &device_protocol);
	if (error)
		return error;

	sprintf(usb_string + strlen(usb_string), "v%04X", idVendor);
	sprintf(usb_string + strlen(usb_string), "p%04X", idProduct);
	sprintf(usb_string + strlen(usb_string), "dl%04X", bcdDevice);
	sprintf(usb_string + strlen(usb_string), "dh%04X", bcdDevice);
	sprintf(usb_string + strlen(usb_string), "dc%02X", (unsigned char)device_class);
	sprintf(usb_string + strlen(usb_string), "dsc%02X", (unsigned char)device_subclass);
	sprintf(usb_string + strlen(usb_string), "dp%02X", (unsigned char)device_protocol);

	/* we need to look at the interface too */
	interface_env = getenv("INTERFACE");
	if (interface_env == NULL)
	{
		/* no interface, use default values here. */
		sprintf(usb_string + strlen(usb_string), "ic*isc*ip*");
	}
	else
	{
		error = split_3values(interface_env, 10, &interface_class,
				      &interface_subclass, &interface_protocol);
		if (error)
			return error;
		sprintf(usb_string + strlen(usb_string), "ic%02X", (unsigned char)interface_class);
		sprintf(usb_string + strlen(usb_string), "isc%02X", (unsigned char)interface_subclass);
		sprintf(usb_string + strlen(usb_string), "ip%02X", (unsigned char)interface_protocol);

	}

	return modprobe(usb_string, true);
}

