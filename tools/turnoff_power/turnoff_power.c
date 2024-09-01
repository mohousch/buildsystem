/*
	Power off for VU+

	Copyright (C) 2019 'redblue-pkt'

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>


#define DEEPSTANDBY 0x123456 // 0x34 0x56 0x12 to send

int g_oledFd = -1;
char g_oledDevice[] = "/dev/oled0";

int main(int argc, char **argv)
{
	g_oledFd = open(g_oledDevice, O_RDWR);
	
	if (ioctl(g_oledFd, DEEPSTANDBY) < 0)
		perror("DEEPSTANDBY");

	if (g_oledFd >= 0)
	{
		close(g_oledFd);
		g_oledFd = -1;
	} 
	else
		perror("Error: Oled not available!\n");
}

