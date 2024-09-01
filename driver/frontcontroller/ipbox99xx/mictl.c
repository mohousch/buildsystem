#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include "micom.h"

#define DEV_CTL_NAME	"/dev/dbox/vfd"

int main(int argc, char **argv)
{
	int i, rv, fd;
	struct vfd_ioctl_data iodata;

	if ((fd = open(DEV_CTL_NAME, O_RDWR)) < 0)
	{
		printf("ERR: can't open controling device '%s'\n", DEV_CTL_NAME);
		return 1;
	}

#define DISPLAY_MSG1 "mictl(c) testing ..."
	write(fd, DISPLAY_MSG1, strlen(DISPLAY_MSG1));
	sleep(1);

	iodata.data[0] = 1; // icon idx
	iodata.data[4] = 1; // ON
	ioctl(fd, VFDICONDISPLAYONOFF, &iodata);

	strcpy(iodata.data, "ping pong holala ...");
	iodata.length = strlen(iodata.data);
	ioctl(fd, VFDDISPLAYCHARS, &iodata);
	sleep(1);

	//ioctl(fd, VFDREBOOT);

	//ioctl(fd, VFDSTANDBY);

	printf("Table show starts:\n");

	memset(iodata.data, 0, sizeof(iodata.data));
	for (i = 8; i < 256; i += 8)
	{
		int j;
		printf("   %02x -> ...\n", i);
		sprintf(iodata.data, "%02x: ", i);
		for (j = i; j < (i + 8); j++)
			sprintf(iodata.data + strlen(iodata.data), "%c", j);
		iodata.length = strlen(iodata.data);
#if 0
		ioctl(fd, VFDDISPLAYCHARSRAW, &iodata);
#else
		ioctl(fd, VFDDISPLAYCHARS, &iodata);
#endif
		sleep(10);
	}
}
