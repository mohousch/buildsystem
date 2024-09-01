/*
 * Ufs910_14W.c
 *
 * (c) 2009 dagobert@teamducktales
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/******************** includes ************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <linux/fb.h>
#include <linux/stmfb.h>
#include <linux/input.h>

#include "global.h"

static int setText(Context_t *context, char *theText);
static int setIcon(Context_t *context, int which, int on);
static int setLed(Context_t *context, int which, int on);
static int Sleep(Context_t *context, time_t *wakeUpGMT);
static int setLight(Context_t *context, int on);

/******************** constants ************************ */
//#define cmdReboot "/sbin/reboot" /* does not currently work */
#define cmdReboot "init 6"
#define cmdHalt "/sbin/halt"

#define cVFD_DEVICE "/dev/vfd"
#define cCMDLINE "/proc/cmdline"

#define cMAXCharsUFS910 16

typedef struct
{
	int vfd;
	int fd_green;
	int fd_red;
	int fd_yellow;

	int nfs;
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tUFS910Private;

/* ******************* helper/misc functions ****************** */
/* donalds helper functions */

/* ----------------------------------------------------- */
struct termios old_io;
struct termios new_io;

int destroySerial()
{
	printf("%s\n", __func__);
	int fd = open("/dev/ttyAS0", O_RDWR);

	if ((tcgetattr(fd, &old_io)) == 0)
	{
		new_io = old_io;
		printf("Setting new flags\n");
		/* c_iflags ->input flags */
		new_io.c_iflag &= ~(IMAXBEL | BRKINT | ICRNL);
		/* c_lflags ->local flags*/
		new_io.c_lflag &= ~(ECHO | IEXTEN);
		/* c_oflags ->output flags*/
		new_io.c_oflag &= ~(ONLCR);
		/* c_cflags ->constant flags*/
		new_io.c_cflag = B19200;
		tcsetattr(fd, TCSANOW, &new_io);
	}
	else
		printf("Error setting raw mode.\n");

	close(fd);
	return 0;
}

/* ----------------------------------------------------- */
void avs_standby(int fd_avs, unsigned int mode)
{
	printf("%s %d\n", __func__, mode);

	if (!mode)
		write(fd_avs, "on", 2);
	else
		write(fd_avs, "off", 3);
}

/* ----------------------------------------------------- */
void net_standby(int mode)
{
	printf("%s\n", __func__);

	if (!mode)
		system("/sbin/ifconfig eth0 down");
	else if (mode)
		system("/sbin/ifconfig eth0 up");
}

/* ----------------------------------------------------- */
void hdmi_standby(int fd_hdmi, int mode)
{
	struct stmfbio_output_configuration outputConfig = {0};
	printf("%s %d\n", __func__, mode);
	outputConfig.outputid = 1;

	if (ioctl(fd_hdmi, STMFBIO_GET_OUTPUT_CONFIG, &outputConfig) < 0)
		perror("Getting current output configuration failed");

	outputConfig.caps = 0;
	outputConfig.activate = STMFBIO_ACTIVATE_IMMEDIATE;
	outputConfig.analogue_config = 0;
	outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;

	if (!mode)
	{
		outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_DISABLED;
	}
	else if (mode)
	{
		outputConfig.hdmi_config &= ~STMFBIO_OUTPUT_HDMI_DISABLED;
	}

	if (outputConfig.caps != STMFBIO_OUTPUT_CAPS_NONE)
	{
		if (ioctl(fd_hdmi, STMFBIO_SET_OUTPUT_CONFIG, &outputConfig) < 0)
			perror("setting output configuration failed");
	}
}

/* ----------------------------------------------------- */
int helloSerial()
{
	int fd = open("/dev/ttyAS0", O_RDWR);
	printf("%s\n", __func__);
	tcsetattr(fd, TCSANOW, &old_io);
	close(fd);
	return 0;
}
/* ----------------------------------------------------- */

void startPseudoStandby(Context_t *context, tUFS910Private *private)
{
	int id;
	int fd_avs = open("/proc/stb/avs/0/standby", O_RDWR);
	int fd_hdmi = open("/dev/fb0", O_RDWR);
	printf("%s\n", __func__);
	setLed(context, 1, 0);
	setLed(context, 2, 1);
	setLed(context, 3, 0);

	if (private->nfs == 0)
	{
		destroySerial();
		net_standby(0);
	}

	avs_standby(fd_avs, 0);
	hdmi_standby(fd_hdmi, 0);
	setText(context, "                ");

	for (id = 0x10; id < 0x20; id++)
		setIcon(context, id, 0);

	if (private->display == 0)
		setLight(context, 0);

	close(fd_hdmi);
	close(fd_avs);
}

void stopPseudoStandby(Context_t *context, tUFS910Private *private)
{
	if (private->display == 0)
		setLight(context, 1);

	setText(context, "                ");
	int id;

	for (id = 0x10; id < 0x20; id++)
		setIcon(context, id, 0);

	setLed(context, 1, 0);
	setLed(context, 2, 0);
	setLed(context, 3, 0);
	system(cmdReboot);
	/* deactivated, because box will hang and remote control not longer works */
#if 0
	int fd_avs = open("/proc/stb/avs/0/standby", O_RDWR);
	int fd_hdmi = open("/dev/fb0", O_RDWR);
	int id;
	printf("%s\n", __func__);

	if (private->display == 0)
		setLight(context, 1);

	setText(context, " ");

	for (id = 0x10; id < 0x20; id++)
		setIcon(context, id, 0);

	hdmi_standby(fd_hdmi, 1);
	avs_standby(fd_avs, 1);

	if (private->nfs == 0)
	{
		net_standby(1);
		helloSerial();
	}

	setLed(context, 1, 0);
	setLed(context, 2, 0);
	setLed(context, 3, 0);
	close(fd_hdmi);
	close(fd_avs);
#endif
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	char cmdLine[512];
	int vFd;
	tUFS910Private *private = malloc(sizeof(tUFS910Private));
//	printf("%s\n", __func__);
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tUFS910Private));
	vFd = open(cCMDLINE, O_RDWR);
	private->nfs = 0;

	if (read(vFd, cmdLine, 512) > 0)
	{
		if (strstr("nfsroot", cmdLine) != NULL)
			private->nfs = 1;
	}

	close(vFd);

	if (private->nfs)
		printf("mode = nfs\n");
	else
		printf("mode = non-nfs\n");

	vFd = open(cVFD_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}

	private->fd_green = open("/sys/class/leds/ufs910:green/brightness", O_WRONLY);
	private->fd_red = open("/sys/class/leds/ufs910:red/brightness", O_WRONLY);
	private->fd_yellow = open("/sys/class/leds/ufs910:orange/brightness", O_WRONLY);
	private->vfd = vFd;
	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement);
	return vFd;
}

static int usage(Context_t *context, char *prg_name)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int setTimer(Context_t *context, time_t *theGMTTime)
{
	time_t curTime;
	struct tm *ts;
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);
	time(&curTime);
	ts = localtime(&curTime);
	fprintf(stderr, "Current Time: %02d:%02d:%02d %02d-%02d-%04d\n",
		ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	startPseudoStandby(context, private);

	if (theGMTTime == NULL)
		private->wakeupTime = read_timers_utc(curTime);
	else
		private->wakeupTime = *theGMTTime;

	Sleep(context, &private->wakeupTime);
	stopPseudoStandby(context, private);
	return 0;
}

static int getTimer(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int shutdown(Context_t *context, time_t *shutdownTimeGMT)
{
	time_t curTime;
	printf("%s\n", __func__);

	/* shutdown immediately */
	if (*shutdownTimeGMT == -1)
		system(cmdHalt);

	while (1)
	{
		time(&curTime);

		if (curTime >= *shutdownTimeGMT)
		{
			system(cmdHalt);
		}

		usleep(100000);
	}

	return -1;
}

static int reboot(Context_t *context, time_t *rebootTimeGMT)
{
	time_t curTime;

//	printf("%s\n", __func__);
	while (1)
	{
		time(&curTime);

		if (curTime >= *rebootTimeGMT)
		{
			system(cmdReboot);
		}

		usleep(100000);
	}

	return 0;
}

static int Sleep(Context_t *context, time_t *wakeUpGMT)
{
	time_t curTime;
	int sleep = 1;
	int vFd;
	fd_set rfds;
	struct timeval tv;
	int retval, len, i;
	struct tm *ts;
	char output[cMAXCharsUFS910 + 1];
	struct input_event data[64];
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);
	output[cMAXCharsUFS910] = '\0';
	vFd = open("/dev/input/event0", O_RDONLY);

	if (vFd < 0)
	{
		perror("event0");
		return -1;
	}

	while (sleep)
	{
		time(&curTime);
		ts = localtime(&curTime);

		if (curTime >= *wakeUpGMT)
		{
			sleep = 0;
		}
		else
		{
			FD_ZERO(&rfds);
			FD_SET(vFd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			retval = select(vFd + 1, &rfds, NULL, NULL, &tv);

			if (retval > 0)
			{
				len = read(vFd, data, sizeof(struct input_event) * 64);

				for (i = 0; i < len / sizeof(struct input_event); i++)
				{
					if (data[i].type == EV_SYN)
					{
						/* noop */
					}
					else if (data[i].type == EV_MSC && (data[i].code == MSC_RAW || data[i].code == MSC_SCAN))
					{
						/* noop */
					}
					else
					{
						if (data[i].code == 116)
							sleep = 0;
					}
				}
			}
		}

		if (private->display)
		{
			strftime(output, cMAXCharsUFS910 + 1, private->timeFormat, ts);
			write(private->vfd, &output, sizeof(output));
		}
	}

	return 0;
}

static int setText(Context_t *context, char *theText)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	memset(data.data, ' ', 63);
	memcpy(data.data, theText, strlen(theText));
	data.start = 0;
	data.length = strlen(theText);

	if (ioctl(private->vfd, VFDDISPLAYCHARS, &data) < 0)
	{
		perror("setText: ");
		return -1;
	}

	return 0;
}

static int setLed(Context_t *context, int which, int on)
{
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);

	if (which == 1)
		write(private->fd_green, on == 0 ? "0" : "1", 1);
	else if (which == 2)
		write(private->fd_red, on == 0 ? "0" : "1", 1);
	else if (which == 3)
		write(private->fd_yellow, on == 0 ? "0" : "1", 1);
	else
		return -1;

	return 0;
}

static int setIcon(Context_t *context, int which, int on)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);
	memset(data.data, ' ', 63);
	data.start = 0;
	data.length = 5;
	data.data[0] = which & 0x0f;
	data.data[4] = on;

	if (ioctl(private->vfd, VFDICONDISPLAYONOFF, &data) < 0)
	{
		perror("setIcon: ");
		return -1;
	}

	return 0;
}

static int setBrightness(Context_t *context, int brightness)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);

	if (brightness < 0 || brightness > 7)
		return -1;

	memset(data.data, ' ', 63);
	data.start = brightness & 0x07;
	data.length = 0;

	if (ioctl(private->vfd, VFDBRIGHTNESS, &data) < 0)
	{
		perror("setBrightness: ");
		return -1;
	}

	return 0;
}

static int setPwrLed(Context_t *context, int brightness)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int setLight(Context_t *context, int on)
{
	struct vfd_ioctl_data data;
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);
	memset(&data, 0, sizeof(struct vfd_ioctl_data));

	if (on)
		data.start = 0x01;
	else
		data.start = 0x00;

	data.length = 0;

	if (ioctl(private->vfd, VFDDISPLAYWRITEONOFF, &data) < 0)
	{
		perror("setLight: ");
		return -1;
	}

	return 0;
}

static int Exit(Context_t *context)
{
	tUFS910Private *private = (tUFS910Private *)((Model_t *)context->m)->private;
	printf("%s\n", __func__);

	if (context->fd > 0)
		close(context->fd);

	if (private->vfd > 0)
		close(private->vfd);

	close(private->fd_green);
	close(private->fd_red);
	close(private->fd_yellow);
	free(private);
	return 1;
}

static int Clear(Context_t *context)
{
	int i;
	setText(context, "                ");
	setBrightness(context, 7);

	for (i = 1; i <= 3 ; i++)
		setLed(context, i, 0);

	for (i = 1; i <= 16 ; i++)
		setIcon(context, i, 0);

	return 0;
}

Model_t Ufs910_14W_model =
{
	.Name                      = "Kathrein UFS910 14W frontpanel control utility",
	.Type                      = Ufs910_14W,
	.Init                      = init,
	.Clear                     = Clear,
	.Usage                     = usage,
	.SetTime                   = setTime,
	.GetTime                   = getTime,
	.SetTimer                  = setTimer,
	.GetTimer                  = getTimer,
	.Shutdown                  = shutdown,
	.Reboot                    = reboot,
	.Sleep                     = Sleep,
	.SetText                   = setText,
	.SetLed                    = setLed,
	.SetIcon                   = setIcon,
	.SetBrightness             = setBrightness,
	.SetPwrLed                 = setPwrLed,
	.SetLight                  = setLight,
	.Exit                      = Exit,
	.SetLedBrightness          = NULL,
	.SetRF                     = NULL,
	.SetFan                    = NULL,
	.private                   = NULL
};
