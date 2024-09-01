/*
 * Copyright (c) donald@teamducktales
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include <linux/dvb/dmx.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void usage(char *name)
{
	printf("usage:\n\n");
	printf("\t%s path\n", name);
	printf("\t%s -p path\n", name);

	exit(1);
}

ssize_t write_all(int fd, const void *buf, size_t count)
{
	int retval;
	char *ptr = (char *)buf;
	size_t handledcount = 0;
	while (handledcount < count)
	{
		retval = write(fd, &ptr[handledcount], count - handledcount);

		if (retval == 0) return -1;
		if (retval < 0)
		{
			if (errno == EINTR) continue;
			return retval;
		}
		handledcount += retval;
	}
	return handledcount;
}

int showiframe(char *path, bool progress)
{

	int m_video_clip_fd;
	int f;

	printf("showSinglePic %s\n", path);

	f = open(path, O_RDONLY);

	if (f)
	{
		struct stat s;
		fstat(f, &s);

		m_video_clip_fd = open("/dev/dvb/adapter0/video0", O_WRONLY);
		if (ioctl(m_video_clip_fd, VIDEO_SET_FORMAT, VIDEO_FORMAT_16_9) < 0)
			printf("VIDEO_SET_FORMAT failed (%m)\n");

		if (m_video_clip_fd >= 0)
		{
			bool seq_end_avail = false;
			size_t pos = 0;
			unsigned char pes_header[] = { 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x80, 0x00, 0x00 };
			unsigned char seq_end[] = { 0x00, 0x00, 0x01, 0xB7 };
			unsigned char iframe[s.st_size];
			unsigned char stuffing[8192];
			memset(stuffing, 0, 8192);
			read(f, iframe, s.st_size);
			ioctl(m_video_clip_fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_MEMORY);
			printf("VIDEO_SELECT_SOURCE MEMORY (%m)\n");
			ioctl(m_video_clip_fd, VIDEO_PLAY);
			printf("VIDEO_PLAY (%m)\n");
			ioctl(m_video_clip_fd, VIDEO_CONTINUE);
			printf("VIDEO_CONTINUE: (%m)\n");
			ioctl(m_video_clip_fd, VIDEO_CLEAR_BUFFER);
			printf("VIDEO_CLEAR_BUFFER: (%m)\n");
			while (pos <= (s.st_size - 4) && !(seq_end_avail = (!iframe[pos] && !iframe[pos + 1] && iframe[pos + 2] == 1 && iframe[pos + 3] == 0xB7)))
				++pos;

			if ((iframe[3] >> 4) != 0xE) // no pes header
			{
				write_all(m_video_clip_fd, pes_header, sizeof(pes_header));
			}
			else
			{
				iframe[4] = iframe[5] = 0x00;
			}
			write_all(m_video_clip_fd, iframe, s.st_size);
			if (!seq_end_avail)
				write_all(m_video_clip_fd, seq_end, sizeof(seq_end));
			write_all(m_video_clip_fd, stuffing, 8192);

			bool end = false;
			char progress_ch [4];

			while (!end)
			{
				sleep(1);

				if (progress)
				{
					int progress_fd = open("/proc/progress", O_RDONLY);
					read(progress_fd, progress_ch, 4);
					close(progress_fd);

					/* Dagobert: Sporadically the video device is not freed before e2 will access it (on ufs922,
					 * maybe on other too?).
					 * In that case no TV Picture is available until reboot. So I end showiframe
					 * a little bit earlier.
					 * Another solution, and I think a better one, is to kill showiframe from e2
					 * if it is running, or wait until it stops. I think the sleep (1) here can be the problem.
					 * Under some circumstance this is enough time for e2 to try to open the video device itself.
					 */
					progress_ch[3] = '\0';
					if (atoi(progress_ch) >= 98)
						end = true;

				}
			}

			printf("********** end showiframe\n");

			ioctl(m_video_clip_fd, VIDEO_SELECT_SOURCE, VIDEO_SOURCE_DEMUX);
			printf("VIDEO_SELECT_SOURCE DEMUX (%m)\n");
			close(m_video_clip_fd);

		}
		close(f);
	}
	else
	{
		printf("could not open %s", path);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 3)
	{
		if (strcmp(argv[1], "-p") == 0)
		{
			showiframe(argv[2], true);
		}
	}
	else if (argc == 2)
	{
		showiframe(argv[1], false);
	}
	else
		usage(argv[0]);

	return 0;
}
