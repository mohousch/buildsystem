/*
 * example to control lnb power
 * 1.1 compile driver
 * 2.1 insmod /lib/modules/lnb.ko
 * 2.2 mknod /dev/lnb c 149 0
 * 3.1 /var/bin/lnb 0 "set votage off"
 * 3.2 /var/bin/lnb 13 "set votage 13"
 * 3.3 /var/bin/lnb 18 "set votage 18"
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <linux/types.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include <sys/select.h>
#include <sys/time.h>

#define LNB_VOLTAGE_OFF 0x2b0010
#define LNB_VOLTAGE_VER 0x2b0011
#define LNB_VOLTAGE_HOR 0x2b0012

int main(int argc, char *argv[])
{
	int voltage = atoi(argv[1]);
	int fd, ret = -1;
	int val = LNB_VOLTAGE_OFF;
	char *devname = "/dev/lnb";
	if ((fd = open(devname, O_RDWR)) < 0)
	{
		fprintf(stderr, "Could not open %s\n", devname);
		perror(devname);
		exit(1);
	}
	printf("voltage: %d\n", voltage);
	switch (voltage)
	{
		case 13:
			val = LNB_VOLTAGE_VER;
			break;
		case 18:
			val = LNB_VOLTAGE_HOR;
			break;
	}
	ret = ioctl(fd, val, 0);
	close(fd);
	exit(ret);
}
