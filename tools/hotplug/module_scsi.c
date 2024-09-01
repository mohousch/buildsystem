/*
 * module_scsi.c
 *
 * Loads a scsi driver based on the scsi hotplug environment variables.
 *
 * Copyright (C) 2005 Greg Kroah-Hartman <greg@kroah.com>
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "hotplug_util.h"
#include "module_scsi.h"
#include "udev.h"

int scsi_add(void)
{
	char scsi_file[256];
	char scsi_type[50];
	int type;
	char *devpath;
	char *module = NULL;
	int i;
	int fd;
	int len;
	int retval = 1;

	devpath = getenv("DEVPATH");
	if (!devpath)
	{
		dbg("missing DEVPATH environment variable, aborting.");
		goto exit;
	}

	snprintf(scsi_file, sizeof(scsi_file), "/sys%s/type", devpath);
	for (i = 0; i < 10; ++i)
	{
		struct stat stats;
		if (stat(scsi_file, &stats) == 0)
			break;
		sleep(1);
	}
	fd = open(scsi_file, O_RDONLY);
	if (fd < 0)
	{
		dbg("can't open file '%s'", scsi_file);
		goto exit;
	}
	len = read(fd, scsi_type, sizeof(scsi_type));
	if (len < 0)
	{
		dbg("can't open file '%s'", scsi_file);
		goto exit_close;
	}

	dbg("read '%s' from '%s'", scsi_type, scsi_file);
	type = atoi(scsi_type);
	dbg("type = %d", type);
	switch (type)
	{
		case 0:
		case 7:
			module = "sd_mod";
			break;
		case 1:
			module = "st";
			break;
		case 4:
		case 5:
			module = "sr_mod";
			break;
	}

	if (module)
		retval = modprobe(module, true);
	else
		retval = 0;

exit_close:
	close(fd);
exit:
	return retval;
}

