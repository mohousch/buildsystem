/*
    hotplug.c

    Copyright (C) 2007 Andreas Oberritter
    Copyright (C) 2004,2005 Greg Kroah-Hartman <greg@kroah.com>
    Copyright (C) 2004,2007 Kay Sievers <kay@vrfy.org>

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
#include <stdio.h>
#include <stdlib.h>
#include "bdpoll.h"
#include "hotplug_basename.h"
#include "hotplug_socket.h"
#include "hotplug_util.h"
#include "module_block.h"
#include "module_firmware.h"
#include "module_ieee1394.h"
#include "module_pci.h"
#include "module_scsi.h"
#include "module_usb.h"
#include "udev.h"

struct command
{
	const char *name;
	int (*cmd)(int argc, char *argv[], char *envp[]);
};

struct subsys
{
	const char *name;
	int (*add)(void);
	int (*remove)(void);
};

static struct subsys subsystems[] =
{
	{
		.name = "block",
		.add = block_add,
		.remove = block_remove,
	}, {
		.name = "firmware",
		.add = firmware_add,
	}, {
		.name = "ieee1394",
		.add = ieee1394_add,
	}, {
		.name = "pci",
		.add = pci_add,
	}, {
		.name = "scsi",
		.add = scsi_add,
	}, {
		.name = "usb",
		.add = usb_add,
	},
};

#if defined(USE_LOG)
void log_message(int level, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsyslog(level, format, args);
	va_end(args);
}
#endif

static void redirect_io(void)
{
#if !defined(DEBUG)
	int fd;

	fd = open("/dev/null", O_RDWR);
	if (fd == -1)
	{
		perror("/dev/null");
		return;
	}

	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);
#endif
}

static int hotplug(int argc, char *argv[], char *envp[])
{
	const char *action;
	const char *modalias;
	struct subsys *s;
	unsigned int i;
	const char *sysname;

	redirect_io();

	/* dbg("starting hotplug version %s", UDEV_VERSION); */

	if (argc < 2)
	{
		err("hotplug expects a parameter, aborting.");
		return EXIT_FAILURE;
	}
	sysname = argv[1];

	action = getenv("ACTION");
	if (action == NULL)
	{
		err("missing ACTION environment variable, aborting.");
		return EXIT_FAILURE;
	}

	modalias = getenv("MODALIAS");
	if (modalias != NULL)
	{
		if (!strcmp(ADD_STRING, action))
			modprobe(modalias, true);
		else if (!strcmp(REMOVE_STRING, action))
			modprobe(modalias, false);
	}

	for (i = 0; i < sizeof(subsystems) / sizeof(subsystems[0]); i++)
	{
		s = &subsystems[i];
		if (strcmp(s->name, sysname))
			continue;
		if (!strcmp(ADD_STRING, action) && s->add)
		{
			return s->add();
		}
		else if (!strcmp(REMOVE_STRING, action) && s->remove)
		{
			return s->remove();
		}
		else
		{
			dbg("we do not handle %s for %s", action, sysname);
			return EXIT_SUCCESS;
		}
	}

	return EXIT_FAILURE;
}

static const struct command cmds[] =
{
	{
		.name = "bdpoll",
		.cmd = bdpoll,
	},
	{
		.name = "hotplug",
		.cmd = hotplug,
	},
#if defined(UDEVMONITOR)
	{
		.name = "udevmonitor",
		.cmd = udevmonitor,
	},
#endif
#if defined(UDEVTRIGGER)
	{
		.name = "udevtrigger",
		.cmd = udevtrigger,
	},
#endif
};

int main(int argc, char *argv[], char *envp[])
{
	const char *command;
	const struct command *cmd;
	int ret = EXIT_FAILURE;

	/* get binary or symlink name */
	command = hotplug_basename(argv[0]);

	logging_init(command);

	for (cmd = cmds; cmd->name != NULL; cmd++)
	{
		if (strcmp(cmd->name, command) == 0)
		{
			ret = cmd->cmd(argc, argv, envp);
			break;
		}
	}

	logging_close();
	return ret;
}

