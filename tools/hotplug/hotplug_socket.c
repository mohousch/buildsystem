/*
    hotplug_socket.c

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

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "udev.h"

void hotplug_socket_send_env(const char *vars[])
{
	struct sockaddr_un addr;
	const char *var;
	int s;

	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, "/tmp/hotplug.socket");

	if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1)
	{
		err("socket: %s", strerror(errno));
		return;
	}

	if (connect(s, (const struct sockaddr *)&addr, SUN_LEN(&addr)) == -1)
	{
		err("connect: %s", strerror(errno));
		goto exit;
	}

	while (*vars != NULL)
	{
		if ((var = getenv(*vars)))
		{
			write(s, *vars, strlen(*vars));
			write(s, "=", 1);
			write(s, var, strlen(var) + 1);
		}
		vars++;
	}
exit:
	close(s);
}

