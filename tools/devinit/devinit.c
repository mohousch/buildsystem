/*  $Id: devinit.c,v 1.2 2012/04/13 12:15:01 rhabarber1848 Exp $

	Helper app to populate the initial /dev directory with the required
	devices when they don't exist yet

	Call this instead of init to set up everything for the real init
	(using the init-commandline option of the linux kernel)

	Copyright (c) 2005, Carsten Juttner
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	* The name of the author may not be used to endorse or promote products
	  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
	THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static void exec_init(char **argv, char **envp)
{
	char *const init_name = "init";
	char *const sh_name = "sh";

	argv[0] = init_name;
	execve("/sbin/init", argv, envp);
	execve("/etc/init", argv, envp);
	execve("/bin/init", argv, envp);

	argv[0] = sh_name;
	execve("/bin/sh", argv, envp);

	/* we should never get here... */
	perror("unable to exec init");
}

int main(int argc, char **argv, char **envp)
{
	char *const console_dev = "/dev/console";
	char *const null_dev = "/dev/null";
	char *const ttyAS0_dev = "/dev/ttyAS0";
	char *const vfd_dev = "/dev/vfd";

	if (getpid() == 1)
	{
		int fh;
		mode_t old = umask(0000);

		if (access(console_dev, F_OK))
		{
			mknod(console_dev, S_IFCHR | 0600, makedev(5, 1));

			fh = open(console_dev, O_RDWR);
			if (fh)  /* descriptor 0 is already open?!? */
			{
				close(fh);
			}
			else     /* create stdout/stderr for init */
			{
				(void)dup(0); /* 1 */
				(void)dup(0); /* 2 */
			}
		}

		if (access(null_dev, F_OK))
			mknod(null_dev, S_IFCHR | 0666, makedev(1, 3));

		if (access(ttyAS0_dev, F_OK))
			mknod(ttyAS0_dev, S_IFCHR | 0660, makedev(204, 40));

		if (access(vfd_dev, F_OK))
			mknod(vfd_dev, S_IFCHR | 0660, makedev(147, 0));

		umask(old);

		/* finally become the real init */
		exec_init(argv, envp);
	}
	else
	{
		fprintf(stderr, "This program is supposed to be run by the kernel.\n");
	}

	return 0;
}

