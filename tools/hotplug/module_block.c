/*
    module_block.c

    Creates block device nodes using hotplug environment variables.

    Copyright (C) 2007 Andreas Oberritter
    Copyright (C) 2005 Greg Kroah-Hartman <greg@kroah.com>

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

#define _GNU_SOURCE	/* for getline() */
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "hotplug_basename.h"
#include "hotplug_devpath.h"
#include "hotplug_pidfile.h"
#include "hotplug_setenv.h"
#include "hotplug_socket.h"
#include "hotplug_timeout.h"
#include "module_block.h"
#include "udev.h"

static const char *block_vars[] =
{
	"ACTION",
	"DEVPATH",
	"PHYSDEVPATH",
	"PHYSDEVDRIVER",
	"X_E2_REMOVABLE",
	"X_E2_CDROM",
	NULL,
};

static int bdpoll_exec(const char devpath[], bool is_cdrom, bool support_media_changed)
{
	pid_t pid;
	char *argv[5];
	unsigned int i = 0;

	pid = fork();
	if (pid == -1)
	{
		perror("fork");
		return -1;
	}
	else if (pid == 0)
	{
		argv[i++] = "bdpoll";
		argv[i++] = (char *)devpath;
		if (is_cdrom)
			argv[i++] = "-c";
		if (support_media_changed)
			argv[i++] = "-m";
		argv[i++] = NULL;
		if (execvp(argv[0], argv) == -1)
			perror(argv[0]);
		return -1;
	}
	else
	{
		return pidfile_write(pid, "bdpoll.%s", hotplug_basename(devpath));
	}
}

static int bdpoll_kill(const char devpath[])
{
	struct timeout t;
	pid_t pid, wpid;

	if (pidfile_read(&pid, "bdpoll.%s", hotplug_basename(devpath)) == -1)
		return -1;

	if (kill(pid, SIGTERM) == -1)
	{
		perror("kill");
		return -1;
	}

	timeout_init(&t, 1000);
	do
	{
		wpid = waitpid(pid, NULL, WNOHANG);
		if (wpid == -1)
		{
			perror("waitpid");
			return -1;
		}
		if (wpid > 0)
			goto exit;
	}
	while (!timeout_exceeded(&t));

	if (kill(pid, SIGKILL) == -1)
	{
		perror("kill");
		return -1;
	}

exit:
	pidfile_unlink("bdpoll.%s", hotplug_basename(devpath));
	return 0;
}

static int do_mknod(const char *devnode, const char *major, const char *minor)
{
	dev_t dev = (atoi(major) << 8) | atoi(minor);

	return mknod(devnode, S_IFBLK | S_IRUSR | S_IWUSR, dev);
}

static long sysfs_attr_get_long(const char *devpath, const char *attr_name)
{
	char *value;

	value = sysfs_attr_get_value(devpath, attr_name);
	if (value == NULL)
	{
		errno = ERANGE;
		return LONG_MAX;
	}

	errno = 0;
	return strtol(value, NULL, 0);
}

#define GENHD_FL_REMOVABLE                      1
static bool dev_is_removable(const char *devpath)
{
	long attr;

	attr = sysfs_attr_get_long(devpath, "removable");
	if ((attr != LONG_MAX) || (errno != ERANGE))
		return (attr != 0);

	attr = sysfs_attr_get_long(devpath, "capability");
	if ((attr != LONG_MAX) || (errno != ERANGE))
		return (attr & GENHD_FL_REMOVABLE);

	return false;
}

#define GENHD_FL_MEDIA_CHANGE_NOTIFY            4
static bool dev_can_notify_media_change(const char *devpath)
{
	long attr;

	attr = sysfs_attr_get_long(devpath, "capability");
	if ((attr != LONG_MAX) || (errno != ERANGE))
		return (attr & GENHD_FL_MEDIA_CHANGE_NOTIFY);

	return false;
}

#define GENHD_FL_CD                             8
static bool dev_is_cdrom(const char *devpath)
{
	char pathname[FILENAME_MAX];
	bool ret = false;
	const char *str;
	char *buf = NULL;
	size_t n = 0;
	long attr;
	FILE *f;

	attr = sysfs_attr_get_long(devpath, "capability");
	if ((attr != LONG_MAX) || (errno != ERANGE))
		return (attr & GENHD_FL_CD);

	str = hotplug_basename(devpath);

	if (!strncmp(str, "sr", 2))
		return 1;

	if ((strlen(str) > 2) &&
			(str[0] == 'h') &&
			(str[1] == 'd'))
	{
		snprintf(pathname, sizeof(pathname), "/proc/ide/%s/media", str);

		f = fopen(pathname, "r");
		if (f == NULL)
		{
			dbg("can't open %s: %s", pathname, strerror(errno));
		}
		else
		{
			if (getline(&buf, &n, f) != -1)
			{
				if (buf != NULL)
				{
					if (n >= 5)
						ret = !strncmp(buf, "cdrom", 5);
					free(buf);
				}
			}
			fclose(f);
		}
	}

	return ret;
}

int block_add(void)
{
	char *devpath;
	const char *minor, *major;
	char devnode[FILENAME_MAX];
	bool is_removable;
	bool is_cdrom;
	bool support_media_changed;

	sysfs_init();

	/*
	 * DEVPATH=/block/sda
	 * DEVPATH=/block/sda/sda1
	 */
	devpath = getenv("DEVPATH");
	if (!devpath)
	{
		dbg("missing DEVPATH environment variable, aborting.");
		return EXIT_FAILURE;
	}

	minor = getenv("MINOR");
	if (!minor)
	{
		dbg("missing MINOR environment variable, aborting.");
		return EXIT_FAILURE;
	}

	major = getenv("MAJOR");
	if (!major)
	{
		dbg("missing MAJOR environment variable, aborting.");
		return EXIT_FAILURE;
	}

	if (!hotplug_devpath_to_devnode(devpath, devnode, sizeof(devnode)))
	{
		dbg("could not get device node.");
		return EXIT_FAILURE;
	}

	unlink(devnode);

	if (do_mknod(devnode, major, minor) == -1)
	{
		dbg("mknod: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	is_removable = dev_is_removable(devpath);
	is_cdrom = is_removable && dev_is_cdrom(devpath);
	support_media_changed = is_cdrom && dev_can_notify_media_change(devpath);

	if (is_removable)
	{
		if (bdpoll_exec(devpath, is_cdrom, support_media_changed) == -1)
			dbg("could not exec bdpoll");
	}

	hotplug_setenv_bool("X_E2_REMOVABLE", is_removable);
	hotplug_setenv_bool("X_E2_CDROM", is_cdrom);

	hotplug_socket_send_env(block_vars);

	return EXIT_SUCCESS;
}

int block_remove(void)
{
	char *devpath;
	char devnode[FILENAME_MAX];

	devpath = getenv("DEVPATH");
	if (!devpath)
	{
		dbg("missing DEVPATH environment variable, aborting.");
		return EXIT_FAILURE;
	}

	if (!hotplug_devpath_to_devnode(devpath, devnode, sizeof(devnode)))
	{
		dbg("could not get device node.");
		return EXIT_FAILURE;
	}

	unlink(devnode);

	if (bdpoll_kill(devpath) == -1)
		dbg("could not kill bdpoll");

	hotplug_socket_send_env(block_vars);

	return EXIT_SUCCESS;
}

