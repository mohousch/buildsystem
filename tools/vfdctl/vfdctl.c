/*
*
*	vfdctl.c
*	by nebman
*	version v0.7.1
*
*	vfdctl is a little utility to control the display of the UFS910 receiver
*
*	thx to captaintrip for his code
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#define VFD_Display_Chars    0xc0425a00
#define VFDICONDISPLAYONOFF  0xc0425a0a
#define VFDICONGETSTATE      0xc0425a0b
#define VFDDISPLAYWRITEONOFF 0xc0425a05
#define VFDWRITECGRAM        0x40425a01
#define SLEEPTIME            200000
#define IN_CHAR              27
#define true 1
#define false 0

/*
*	Umlauts:
*		128 ä, 129 ö, 130 ü
*		131 Ä, 132 Ö, 133 Ü
*		134 ß
*
*	Special chars:
*		17 = | bis 21 = |||||
*		22 = 6 horizontal bars filled from bottom of a total of 7
*		.. 27 = 1 horizontal bars filled from bottom of a total of 7
*/

void setMessageToDisplay(char *);
void setMessageToDisplayEx(char *, int len);
void show_help();
void close_device_vfd();
void scrollText(char *text);
int iconOnOff(char *sym, unsigned char onoff);
void centeredText(char *text);
int writeCG(unsigned char adress, unsigned char pixeldata[5]);
void demoMode(void);
void sigfunc(int sig);
void inputRemote(char *text);
void refreshDisp();
void printState(int index);
int getIconIndex(char *icon);
void printBitmap(char *filename, int animationSpeed);
void playVfdx(char *filename);

#ifdef HAVE_SPARK7162_HARDWARE
#define VFD_LEN    8
struct set_mode_s
{
	int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

struct set_brightness_s
{
	int level;
};

struct set_icon_s
{
	int icon_nr;
	int on;
};

struct set_led_s
{
	int led_nr;
	int on;
};

/* time must be given as follows:
 * time[0] & time[1] = mjd ???
 * time[2] = hour
 * time[3] = min
 * time[4] = sec
 */
struct set_standby_s
{
	char time[5];
};

struct set_time_s
{
	char time[5];
};

struct aotom_ioctl_data
{
	union
	{
		struct set_icon_s icon;
		struct set_led_s led;
		struct set_brightness_s brightness;
		struct set_mode_s mode;
		struct set_standby_s standby;
		struct set_time_s time;
	} u;
};

const char *icons[46] =
{
	"fr", "plr", "play", "plf", "ff", "pause", "rec", "mute", "cycle", "dd", "lock", "ci", "usb", "hd", "rec2", "hd8", "hd7", /* 17 */
	"hd6", "hd5", "hd4", "hd3", "hdfull", "hd2", "hd1", "mp3", "ac3", "tvl", "music", "alert", "hdd", "clockpm", "clockam", /* 32 */
	"clock", "mail", "bt", "stby", "ter", "disk3", "disk2", "disk1", "disk0", "sat", "ts", "dot1", "cab", "all"
};           /* 46 */

static struct aotom_ioctl_data aotom_data;
static char textOff = 100;
#else
#define VFD_LEN   16
const char *icons[16] = {"usb", "hd", "hdd", "lock", "bt", "mp3", "music", "dd", "mail", "mute", "play", "pause", "ff", "fr", "rec", "clock"};
#endif

const char *states[3] = {"off", "on", "not inited"};

int file_vfd;
char verbose = false;

#define MAX_INPUT 200
char outbuffer[16];
char input[MAX_INPUT];
int position = 0;
int offset = 0;

int main(int argc, char **argv)
{
	// check command line argument count
	if (argc == 1)
	{
		show_help();
		return EXIT_FAILURE;
	}

	// function to catch SIGINT
	signal(SIGINT, sigfunc);

	// open display file
	if ((file_vfd = open("/dev/vfd", O_RDWR)) == -1)
	{
		printf("vfdctl: could not open vfd-device!\n");
		return EXIT_FAILURE;
	}

	int i;
	unsigned char centerText = false;
	char *output = 0;
	int animationSpeed = 500000;	// default animation to 1/2 second

	for (i = 1; i < argc; i++)
	{
		char *cmd = argv[i];

		if (strcmp(cmd, "-c") == 0)
		{
			centerText = true;
			if (verbose) printf("option centered text active\n");
		}
		else if (strcmp(cmd, "-v") == 0)
		{
			verbose = true;
#ifndef HAVE_SPARK7162_HARDWARE
		}
		else if (strcmp(cmd, "-s") == 0)
		{
			if (argc > 2)
			{
				i++;
				animationSpeed = atoi(argv[i]) * 1000;
				continue;
			}
			else
			{
				fprintf(stderr, "vfdctl: please specify animationspeed in milliseconds as 2nd argument\n");
				break;
			}
		}
		else if (strcmp(cmd, "-b") == 0)
		{
			if (argc > 2)
			{
				printBitmap(argv[i + 1], animationSpeed);
			}
			else
			{
				fprintf(stderr, "vfdctl: please specify bitmap file as 2nd argument\n");
			}
			break;
		}
		else if (strcmp(cmd, "-x") == 0)
		{
			if (argc > 2)
			{
				playVfdx(argv[i + 1]);
			}
			else
			{
				fprintf(stderr, "vfdctl: please specify .vfdx file as 2nd argument\n");
			}
			break;
#endif
		}
		else if (cmd[0] == '+')
		{
			iconOnOff(cmd + 1, true);
		}
		else if (cmd[0] == '-')
		{
			iconOnOff(cmd + 1, false);
#ifdef HAVE_SPARK7162_HARDWARE
		}
		else if (strcmp(cmd, "texton") == 0)
		{
			textOff = 101;
			ioctl(file_vfd, VFDDISPLAYWRITEONOFF, &textOff);
		}
		else if (strcmp(cmd, "textoff") == 0)
		{
			textOff = 100;
			ioctl(file_vfd, VFDDISPLAYWRITEONOFF, &textOff);
#else
		}
		else if (strcmp(cmd, "demomode") == 0)
		{
			printf("vfdctl: starting demomode\n");
			demoMode();
			break;
		}
		else if (strcmp(cmd, "input") == 0)
		{
			fprintf(stderr, "vfdctl: starting remote control input mode\n");
			if (argc == 3)
			{
				inputRemote(argv[2]);
			}
			else
			{
				inputRemote("");
			}
			break;
		}
		else if (strcmp(cmd, "iconstate") == 0)
		{
			if (argc == 3)
			{
				printState(getIconIndex(argv[2]));
			}
			else
			{
				printState(-1);
			}
			break;
#endif
		}
		else
		{
			output = cmd;
		}

	}

	if (output)
	{
		if (strlen(output) > VFD_LEN)
			scrollText(output); // scroll text if >VFD_LEN
		else
		{
			if (centerText == true)
			{
				centeredText(output);
			}
			else
			{
				setMessageToDisplay(output);
			}
		}
	}
	if (verbose) printf("closing vfd device\n");
	close_device_vfd();

	return EXIT_SUCCESS;
}

int getIconIndex(char *icon)
{
	int i;

#ifdef HAVE_SPARK7162_HARDWARE
	for (i = 0; i < 46; i++)
	{
		if (strcmp(icon, icons[i]) == 0)
		{
			return i + 1;
		}
	}
#else
	for (i = 0; i < 16; i++)
	{
		if (strcmp(icon, icons[i]) == 0)
		{
			return i;
		}
	}
#endif
	printf("Icon %s does not exist!\n", icon);
	exit(-1);
}

// CODE captaintrip
void setMessageToDisplay(char *str)
{
	int i;

	struct ioctl_data
	{
		unsigned char start;
		unsigned char data[64];
		unsigned char length;
	}
	writedisp_data;

	memset(writedisp_data.data, ' ', VFD_LEN);

	i = strlen(str);
	if (i > VFD_LEN) i = VFD_LEN;
	memcpy(writedisp_data.data, str, i);

	writedisp_data.start = 0;
	writedisp_data.length = VFD_LEN;

	ioctl(file_vfd, VFD_Display_Chars, &writedisp_data);
}

void close_device_vfd()
{
	if (file_vfd != -1)
		close(file_vfd);
}

void show_help()
{
#ifdef HAVE_SPARK7162_HARDWARE
	printf("vfdctl v0.7.1 Spark7162 - usage:\n\
\tvfdctl [[-c] text] [+sym] [-sym] ...\n\
\t-c\tcentered output\n\
\tto set symbols use e.g. +usb or -usb\n\
\tavailable symbols are fr,plr,play,plf,ff,pause,rec,mute,cycle,dd,lock,ci,usb,hd,rec2,hd8,hd7,hd6,hd5,hd4,hd3,hdfull,hd2,hd1\n\
\t,mp3,ac3,tvl,music,alert,hdd,clockpm,clockam,clock,mail,bt,stby,ter,disk3,disk2,disk1,disk0,sat,ts,dot1,cab,all\n\
\tspecial modes are: \n\
\ttexton\tactivate text showing on VFD\n\
\ttextoff\tdeactivate text showing on VFD\n");
#else
	printf("vfdctl v0.7.1 - usage:\n\
\tvfdctl [[-c] text] [-s speed] [-b file] [+sym] [-sym] ...\n\
\t-c\tcentered output\n\
\t-s\tset animation speed in milliseconds for -b output\n\
\t-b\toutput content of bitmap file\n\
\t-x\tplay vfdx file (animated)\n\
\tto set symbols use e.g. +usb or -usb\n\
\tavailable symbols are usb,hd,hdd,lock,bt,mp3,music,dd,mail,mute,play,pause,ff,fr,rec,clock\n\
\tspecial modes are: \n\
\tvfdctl demomode to start demo loop\n\
\tvfdctl input [text] for remote control input mode\n\
\tvfdctl iconstate [iconname] to get the current icon state\n");
#endif
}

void centeredText(char *text)
{
	if (verbose) printf("centering text\n");

	int ws = 0; // needed whitespace for centering
	if (strlen(text) < VFD_LEN)
		ws = (VFD_LEN - strlen(text)) / 2;
	else
		ws = 0;

	char *textout = malloc(VFD_LEN);
	memset(textout, ' ', VFD_LEN);
	memcpy(textout + ws, text, VFD_LEN - ws);
	setMessageToDisplay(textout);
	free(textout);
}

void scrollText(char *text)
{
	int i, len = strlen(text);
	char *out = malloc(VFD_LEN);

	for (i = 0; i <= (len - VFD_LEN); i++) // scroll text till end
	{
		memset(out, ' ', VFD_LEN);
		memcpy(out, text + i, VFD_LEN);
		setMessageToDisplay(out);
		usleep(SLEEPTIME);
	}
	for (i = 1; i < VFD_LEN; i++)   // scroll text with whitespaces from right
	{
		memset(out, ' ', VFD_LEN);
		memcpy(out, text + len + i - VFD_LEN, VFD_LEN - i);
		setMessageToDisplay(out);
		usleep(SLEEPTIME);
	}

	memcpy(out, text, VFD_LEN); // display first 16 chars after scrolling
	setMessageToDisplay(out);
	free(out);
}

int iconOnOff(char *sym, unsigned char onoff)
{
	char icon = getIconIndex(sym);
#ifdef HAVE_SPARK7162_HARDWARE
	aotom_data.u.icon.icon_nr = icon;
	aotom_data.u.icon.on = onoff;
	ioctl(file_vfd, VFDICONDISPLAYONOFF, &aotom_data);
#else
	struct
	{
		unsigned char start;
		unsigned char data[64];
		unsigned char length;
	} data;

	data.start = 0x00;
	data.data[0] = icon;
	data.data[4] = onoff;
	data.length = 5;
	ioctl(file_vfd, VFDICONDISPLAYONOFF, &data);
#endif

	if (verbose)
		printf("set icon %s(%x) %d \n", sym, icon, onoff);
	return 0;
}

int writeCG(unsigned char adress, unsigned char pixeldata[5])
{
	struct
	{
		unsigned char start;
		unsigned char data[64];
		unsigned char length;
	} data;

	data.start = adress & 0x07;
	data.data[0] = pixeldata[0];
	data.data[1] = pixeldata[1];
	data.data[2] = pixeldata[2];
	data.data[3] = pixeldata[3];
	data.data[4] = pixeldata[4];
	data.length = 5;
	return ioctl(file_vfd, VFDWRITECGRAM, &data);
}

void demoMode(void)
{
	char man1[5] = {0x02, 0x64, 0x1D, 0x64, 0x02};
	char man2[5] = {0x04, 0x64, 0x1D, 0x64, 0x04};
	char u[5] = {0x00, 0x1C, 0x20, 0x20, 0x3E};
	char f[5] = {0x00, 0x3E, 0x0A, 0x0A, 0x00};
	char s[5] = {0x20, 0x20, 0x2C, 0x12, 0x02};
	char neun[5] = {0x20, 0x20, 0x2E, 0x2A, 0x3E};
	char eins[5] = {0x00, 0x00, 0x3E, 0x00, 0x00};
	char nul[5] = {0x1C, 0x22, 0x22, 0x1C, 0x00};

	char *writechars[7] = {man1, u, f, s, neun, eins, nul };
	char *writechars_ani[7] = {man2, u, f, s, neun, eins, nul };

	int i;
	for (i = 0; i < 7; i++)
	{
		writeCG(i, (unsigned char *)(writechars + i));
	}

	char test[9] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00};
	setMessageToDisplay(test);

	//animation
	while (1)
	{
		usleep(750000);
		writeCG(0, (unsigned char *)writechars_ani + 0);
		setMessageToDisplay(test);

		usleep(750000);
		writeCG(0, (unsigned char *)writechars + 0);
		setMessageToDisplay(test);
	}
}

void sigfunc(int sig)
{
	if (sig != SIGINT)
		return;
	else
	{
		exit(-1);
	}
}

void appendToOutput(char c)
{
	//fprintf(stderr,"< appendToOutput\n");
	if (c == 0) return;

	input[position] = c;
	if (position == MAX_INPUT)
		return;
	position++;

	refreshDisp();
	//fprintf(stderr,"appendToOutput >\n");
}

void refreshDisp()
{
	memset(&outbuffer[offset], ' ', 16 - offset);
	if (position > 15 - offset)
	{
		memcpy(&outbuffer[offset], &input[position - 15 + offset], 15 - offset);
		outbuffer[15] = IN_CHAR;
	}
	else
	{
		memcpy(&outbuffer[offset], &input, position);
		outbuffer[position + offset] = IN_CHAR;
	}
	setMessageToDisplay(outbuffer);
}

char decodeToChar(char in, char in2)
{
	//fprintf(stderr,"< decodeToChar\n");

	if (in == 48 && in2 >= 48 && in2 <= 57)
	{
		return '0' + in2 - 48;
	}
	else if (in == 48 && in2 == 68)
	{
		return '.';
	}
	return 0;
}

void inputRemote(char *text)
{
	//fprintf(stderr, "< inputRemote(%s)\n", text);

	char *out = malloc(16);
	memset(out, 0x10, 16);
	memset(input, 0, MAX_INPUT);
	memset(outbuffer, ' ', 16);

	if (text != (void *)0)
	{
		offset = strlen(text);
		if (offset > 10) offset = 10;
		memcpy(outbuffer, text, offset);
	}

	outbuffer[offset] = IN_CHAR;

	FILE *fd = fopen("/dev/ttyAS1", "r");
	if (fd)
	{
		fprintf(stderr, "opened remote control\n");
	}

	setMessageToDisplay(outbuffer);
	int in, in2, in3;
	char dec_in;

	while ((in = fgetc(fd)) > 0)
	{
		in2 = fgetc(fd);
		in3 = fgetc(fd);

		if (in3 == 10)
		{
			//fprintf(stderr,"we have input: %d %d\n", in, in2);
			if (in == 53 && in2 == 53)
			{
				break;
			}
			else if (in == 53 && in2 == 67)
			{
				char *out = (char *)malloc(position + 1);
				memcpy(out, &input, position);
				*(out + position) = 0x00;
				printf("%s", out);
				break;
			}
			else if (in == 53 && in2 == 65)
			{
				//fprintf(stderr,"del\n");
				position--;
				if (position < 0) position = 0;
				refreshDisp();
			}
			dec_in = decodeToChar(in, in2);
			appendToOutput(dec_in);
		}
	}

	memset(&outbuffer, ' ', 16);
	setMessageToDisplay(outbuffer);
	fprintf(stderr, "bye\n");
	fclose(fd);
}

void printState(int index)
{
	struct
	{
		unsigned char start;
		unsigned char data[64];
		unsigned char length;
	} data;

	memset(&data.data[0], 0, 64);
	ioctl(file_vfd, VFDICONGETSTATE, &data);

	data.data[16] = 0x00;
	if (data.data[17] != 0x1F)
	{
		printf("Failed to get icon state! Kernel not patched?\n");
		return;
	}

	if (index == -1)
	{
		int i;
		for (i = 0; i < 16; i++)
		{
			printf("%s: %s\n", icons[i], states[data.data[i]]);
		}
	}
	else
	{
		printf("%s: %s\n", icons[index], states[data.data[index]]);
	}
}

void setMessageToDisplayEx(char *str, int len)
{
	struct ioctl_data
	{
		unsigned char start;
		unsigned char data[64];
		unsigned char length;
	}
	writedisp_data;

	memset(writedisp_data.data, ' ', 16);

	if (len > 16) len = 16;
	memcpy(writedisp_data.data, str, len);

	writedisp_data.start = 0;
	writedisp_data.length = 16;

	ioctl(file_vfd, VFD_Display_Chars, &writedisp_data);
}

void printBitmap(char *filename, int animationTime)
{
	FILE *fd;
	int i, x;
	unsigned char *buf;
	unsigned char *tx;

	buf = malloc(35);
	tx = malloc(17);

	fd = fopen(filename, "r");
	if (fd)
	{
		fread(buf, 35, 1, fd);		// read character bitmaps
		x = 0;
		for (i = 0; i < 35; i += 5)
		{
			writeCG(x++, &buf[i]);
		}

		while (fread(tx, 17, 1, fd) > 0)	// read string to display
		{
			setMessageToDisplayEx((char *)tx, 16);
			usleep(animationTime);
		}

		fclose(fd);

	}
	else
	{
		fprintf(stderr, "cannot open file\n");
	}
	free(tx);
	free(buf);
}

void playVfdx(char *filename)
{
	FILE *fd;
	int i, x;
	unsigned char *buf;
	char endless = 0;
	char currentCharSet = 255;

#pragma pack(1)
	struct
	{
		char charSet;
		char text[16];
		unsigned short sleepTime;
	} line;
#pragma pack()

	buf = malloc(10 * 35);
	fd = fopen(filename, "r");
	if (fd)
	{
		fread(&endless, 1, 1, fd);		// read character bitmaps
		fread(buf, 10 * 35, 1, fd);		// read character bitmaps

		for (;;)
		{
			fseek(fd, 351L, SEEK_SET);

			while (fread(&line, sizeof(line), 1, fd) > 0)  		// read string to display
			{
				if (line.charSet != currentCharSet)
				{
					currentCharSet = line.charSet;
					x = 0;
					int start = line.charSet * 35;
					for (i = start; i < start + 35; i += 5)
					{
						writeCG(x++, &buf[i]);
					}
				}
				setMessageToDisplayEx(line.text, 16);
				usleep(line.sleepTime * 1000);
			}

			if (endless == 0)
			{
				break;
			}
		}

		fclose(fd);

	}
	else
	{
		fprintf(stderr, "cannot open file\n");
	}

	free(buf);
}

