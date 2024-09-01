/*
    bdpoll.c (based on addon-storage.c from hal-0.5.10)

    Poll storage devices for media changes

    Copyright (C) 2007 Andreas Oberritter
    Copyright (C) 2004 David Zeuthen, <david@fubar.dk>

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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mntent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>
#include <linux/cdrom.h>
#include "bdpoll.h"
#include "hotplug_devpath.h"
#include "hotplug_setenv.h"
#include "hotplug_socket.h"
#include "udev.h"

enum
{
	MEDIA_STATUS_UNKNOWN = 0,
	MEDIA_STATUS_GOT_MEDIA = 1,
	MEDIA_STATUS_NO_MEDIA = 2,
};

static int media_status = MEDIA_STATUS_NO_MEDIA;
static const int interval_in_seconds = 2;

static const char *bdpoll_vars[] =
{
	"DEVPATH",
	"X_E2_MEDIA_STATUS",
	NULL,
};

static void bdpoll_notify(const char devpath[])
{
	setenv("DEVPATH", devpath, 1);
	hotplug_setenv_bool("X_E2_MEDIA_STATUS", media_status == MEDIA_STATUS_GOT_MEDIA);
	hotplug_socket_send_env(bdpoll_vars);
}

static bool is_mounted(const char device_file[])
{
	FILE *f;
	bool rc;
	struct mntent mnt;
	struct mntent *mnte;
	char buf[512];

	rc = false;

	if ((f = setmntent("/etc/mtab", "r")) == NULL)
		return rc;

	while ((mnte = getmntent_r(f, &mnt, buf, sizeof(buf))) != NULL)
	{
		if (strcmp(device_file, mnt.mnt_fsname) == 0)
		{
			rc = true;
			break;
		}
	}

	endmntent(f);
	return rc;
}

static bool poll_for_media(const char device_file[], bool is_cdrom, bool support_media_changed)
{
	int fd;
	bool got_media = false;
	bool ret = false;

	if (is_cdrom)
	{
		int drive;

		fd = open(device_file, O_RDONLY | O_NONBLOCK | O_EXCL);
		if (fd < 0 && errno == EBUSY)
		{
			/* this means the disc is mounted or some other app,
			 * like a cd burner, has already opened O_EXCL */

			/* HOWEVER, when starting hald, a disc may be
			 * mounted; so check /etc/mtab to see if it
			 * actually is mounted. If it is we retry to open
			 * without O_EXCL
			 */
			if (!is_mounted(device_file))
				return false;
			fd = open(device_file, O_RDONLY | O_NONBLOCK);
		}
		if (fd < 0)
		{
			err("%s: %s", device_file, strerror(errno));
			return false;
		}

		/* Check if a disc is in the drive
		 *
		 * @todo Use MMC-2 API if applicable
		 */
		drive = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
		switch (drive)
		{
			case CDS_NO_INFO:
			case CDS_NO_DISC:
			case CDS_TRAY_OPEN:
			case CDS_DRIVE_NOT_READY:
				break;

			case CDS_DISC_OK:
				/* some CD-ROMs report CDS_DISK_OK even with an open
				 * tray; if media check has the same value two times in
				 * a row then this seems to be the case and we must not
				 * report that there is a media in it. */
				if (support_media_changed &&
						ioctl(fd, CDROM_MEDIA_CHANGED, CDSL_CURRENT) &&
						ioctl(fd, CDROM_MEDIA_CHANGED, CDSL_CURRENT))
				{
				}
				else
				{
					got_media = true;
				}
				break;

			case -1:
				err("%s: CDROM_DRIVE_STATUS: %s", device_file, strerror(errno));
				break;
		}

		close(fd);
	}
	else
	{
		fd = open(device_file, O_RDONLY);
		if ((fd < 0) && (errno == ENOMEDIUM))
		{
			got_media = false;
		}
		else if (fd >= 0)
		{
			got_media = true;
		}
		else
		{
			err("%s: %s", device_file, strerror(errno));
			return false;
		}
	}

	switch (media_status)
	{
		case MEDIA_STATUS_GOT_MEDIA:
			if (!got_media)
			{
				dbg("Media removal detected on %s\n", device_file);
				ret = true;
				/* have to this to trigger appropriate hotplug events */
				fd = open(device_file, O_RDONLY | O_NONBLOCK);
				if (fd >= 0)
				{
					ioctl(fd, BLKRRPART);
					close(fd);
				}
			}
			break;

		case MEDIA_STATUS_NO_MEDIA:
			if (got_media)
			{
				dbg("Media insertion detected on %s\n", device_file);
				ret = true;
			}
			break;
	}

	/* update our current status */
	if (got_media)
		media_status = MEDIA_STATUS_GOT_MEDIA;
	else
		media_status = MEDIA_STATUS_NO_MEDIA;

	return ret;
}

static void usage(const char argv0[])
{
	fprintf(stderr, "usage: %s <block device> [-c][-m]\n", argv0);
}

int bdpoll(int argc, char *argv[], char *envp[])
{
	char devnode[FILENAME_MAX];
	const char *devpath;
	bool is_cdrom = false;
	bool support_media_changed = false;
	int opt;

	while ((opt = getopt(argc, argv, "cm")) != -1)
	{
		switch (opt)
		{
			case 'c':
				is_cdrom = true;
				break;
			case 'm':
				support_media_changed = true;
				break;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	if (optind == argc)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	devpath = argv[optind];

	if (!hotplug_devpath_to_devnode(devpath, devnode, sizeof(devnode)))
	{
		err("could not parse devpath");
	}

	for (;;)
	{
		if (poll_for_media(devnode, is_cdrom, support_media_changed))
			bdpoll_notify(devpath);
		sleep(interval_in_seconds);
	}

	return EXIT_SUCCESS;
}

