/*
	Modified by TangoCash to fit my needs (c) 2016
*/
/* $Id: spf87hd,v 0.0.1 2011/01/19 22:11:54

   Use only with the Samsung SPF-87H.
   Desgined and written for Firmware M-IR8HSBWW-1007.0 which is broken
   by design.
   Compile with make.

   This software acts as a deamon and has options.
   It automatically sends commands to the device if you turn mini Monitor
   Mode on. The commands sent by the deamon cause the device to stay in this
   Mode (which the named firmware lacks).
   The deaman can start e.g. your own scripts depending on the actions the
   deamon makes.
   I use spf87hd -n sp-off.sh -m sp-on.sh -u sascha.
   Try spf87hd --help.

   No software to send pictures to the device is encluded. Look for
   playusb at http://vdrportal.de/board/thread.php?postid=951818#post951818

   No commercial affiliation, warranty, copyright, party invitation,
   fitness or non-fitness, implied or otherwise, is claimed by this comment.
   Compile with make.

   This software is free of charge. It has no warranty, copyright, fitness or
   non-fitness, implied or otherwise, use it at own risk. It is deciated to
   the GPL License.

   You can find it at http://www.killerhippy.de

   Sascha Wüstemann
   2011-01-30 v.0.0.1 - Initial Version

   thanks to Grace Woo: http://web.media.mit.edu/~gracewoo/stuff/picframe/
   Vadim Zaliva: http://notbrainsurgery.livejournal.com/38622.html
   Christian Mehlis and René Kijewski:
   http://page.mi.fu-berlin.de/kijewski/solutions/ti3/atoi,grep,wc,cat.html
   and e.g. http://pronix.linuxdelta.de/
   and e.g. to http://www.signal11.us/oss/udev/
*/

#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <getopt.h>
#include <libusb-1.0/libusb.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define CHILD            0
#define ERROR           -1
#define LOGNAME         "spf87hd"
#define FACILITY        LOG_DAEMON
#define SPF87H_VID      0x04e8
#define SPF87H_PID1     0x2033
#define SPF87H_PID2     0x2034
#define PATTERN		"Vendor=04e8 ProdID=2034"
#define SYSBUS "/sys/bus/usb/devices/"

const char *uvid = "04e8";
const char *upid = "2034";
const char *appeared = "add";
const char *away = "remove";

enum opmode_t
{
	DEAMON,
	NODEAMON
} opmode;
int CMD_SENT = -1;

void show_help()
{
	printf("Initializes the SAMSUNG SPF-87H for Linux Monitor Mode when it is set to it.\n\n");
	printf("%s", LOGNAME);
	printf(" has these Options:\n\n"\
	       "\t-f, --foreground\t  Don't deamonize, stay at console.\n"\
	       "\t-n, --nomonitor 'command' Command (unchecked) to be executed after Mini-\n\t\t\t\t  Monitor function has been stopped.\n"\
	       "\t-m, --monitor 'command'\t  Command (unchecked) to be executed after Mini-\n\t\t\t\t  Monitor function has been started.\n"\
	       "\t-l, --lcd4linux Start/Stop LCD4linux with Mini-Monitor function.\n"\
	       "\t-h, --help\t\t  This text.\n\n");

}

void start_deamon()
{
	int i;
	pid_t pid;

	if ((pid = fork()) != CHILD) exit(-1);
	if (setsid() == ERROR)
	{
		fprintf(stderr, "%s can't become session leader!\n", LOGNAME);
		exit(-1);
	}
	if ((chdir("/") < 0))
	{
		fprintf(stderr, "%s can't chdir /!\n", LOGNAME);
		exit(-1);
	}
	umask(0);
	for (i = sysconf(_SC_OPEN_MAX); i > 0; i--)
		close(i);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	openlog(LOGNAME, LOG_PERROR | LOG_PID | LOG_CONS, FACILITY);
	syslog(FACILITY, "%s%s\n", LOGNAME, " started.");
}

void stop_deamon()
{
	char buf[30];
	syslog(FACILITY, "%s%s\n", LOGNAME, " received SIGTERM.");
	sprintf(buf, "/var/run/%s.pid", LOGNAME);
	syslog(FACILITY, "Removing %s.\n", buf);
	if ((remove(buf) == -1))
		syslog(FACILITY, "Can't remove %s\n", buf);
	syslog(FACILITY, "%s%s\n", LOGNAME, " stopped normally.");
	closelog();
	exit(0);
}

int find_spf()
{

	struct libusb_device_descriptor desc; //target device description
	struct libusb_device_handle *devh = NULL;
	libusb_device **devs;  //a list of usb devices
	libusb_device *dev; //target device reference
	int ret = libusb_init(NULL);
	if (ret < 0)
	{
		if (opmode == NODEAMON)
		{
			fprintf(stderr, "failed to initialize libusb\n");
			exit(1);
		}
		else
		{
			syslog(FACILITY, "failed to initialize libusb\n");
			exit(1);
		}
	}
	libusb_set_debug(NULL, 3); // set Debug Level
	ssize_t  cnt = libusb_get_device_list(NULL, &devs);
	if (cnt > 0)
	{
		int i = 0;
		while ((dev = devs[i++]) != NULL)
		{
			int ret = libusb_get_device_descriptor(dev, &desc);
			if (ret < 0)
			{
				if (opmode == NODEAMON)
					fprintf(stderr, "libusb failed to get device descriptor");
				else
					syslog(FACILITY, "libusb failed to get device descriptor");
				dev = NULL;
				break;
			}
			if (desc.idVendor == SPF87H_VID)
			{
				if (desc.idProduct == SPF87H_PID1)
				{
					if (opmode == NODEAMON)
						fprintf(stderr, "SPF87H in mass storage mode found\n");
					else
						syslog(FACILITY, "SPF87H in mass storage mode found\n");
					dev = NULL;
					break;
				}
				else if (desc.idProduct == SPF87H_PID2)
				{
					if (opmode == NODEAMON)
						fprintf(stderr, "SPF87H device in monitor mode found.\n");
					else
						syslog(FACILITY, "SPF87H device in monitor mode found.\n");
					break;
				}
				else
				{
					dev = NULL;
					break;
				}
			}
		}
		if (dev)
		{
			//found device
			if (CMD_SENT == -1)
			{
				ret = libusb_open(dev, &devh); // open the device handle
				if (ret != 0)
				{
					if (opmode == NODEAMON)
						fprintf(stderr, "Error opening device handle\n");
					else
						syslog(FACILITY, "Error opening device handle\n");
					libusb_free_device_list(devs, 1);
					libusb_exit(NULL);
					return -1;
				}
				ret = libusb_claim_interface(devh, 0);
				if (ret < 0) // claim first fail
				{
					ret = libusb_detach_kernel_driver(devh, 0);
					if (ret > 0) // claim 2nd try
					{
						ret = libusb_claim_interface(devh, 0);
						if (ret < 0) // claim 2nd fail
						{
							if (opmode == NODEAMON)
								fprintf(stderr, "Error claim_device\n");
							else
								syslog(FACILITY, "Error claim_device\n");
							libusb_free_device_list(devs, 1);
							libusb_exit(NULL);
							return -1;
						}
					}
					else
					{
						if (opmode == NODEAMON)
							fprintf(stderr, "Error detaching kernel driver from system\n");
						else
							syslog(FACILITY, "Error detaching kernel driver from system\n");
						return -1;
					}
				}
				if (opmode == NODEAMON)
					fprintf(stderr, "claimed device.\n");
				else
					syslog(FACILITY, "claimed device.\n");
				int ret;
				unsigned char data[0x00];
				// sequence 1/9
				ret = libusb_control_transfer(devh,
							      0xc0,
							      0x04,
							      0x0000,
							      0x0000,
							      data,
							      0x0001,
							      0);
				if (ret < 0)
				{
					if (opmode == NODEAMON)
						fprintf(stderr, "Seq 1: F0 error %d\n", ret);
					else
						syslog(FACILITY, "Seq 1: F0 error %d\n", ret);
				}
				if ((unsigned int) ret < sizeof(data))
				{
					if (opmode == NODEAMON)
						fprintf(stderr, "Seq 1: short read (%d)\n", ret);
					else
						syslog(FACILITY, "Seq 1: short read (%d)\n", ret);
				}

				// sequence 2/9 -8/9
				int i;
				for (i = 2; i < 9; i++)
				{
					ret = libusb_control_transfer(devh,
								      0xc0,
								      0x01,
								      0x0000,
								      0x0000,
								      data,
								      0x0002,
								      0);
					if (ret < 0)
					{
						if (opmode == NODEAMON)
							fprintf(stderr, "Seq %i: F0 error %d\n", i, ret);
						else
							syslog(FACILITY, "Seq %i: F0 error %d\n", i, ret);
					}
					if ((unsigned int) ret < sizeof(data))
					{
						if (opmode == NODEAMON)
							fprintf(stderr, "Seq %i: short read (%d)\n", i, ret);
						else
							syslog(FACILITY, "Seq %i: short read (%d)\n", i, ret);
					}
				}

				// sequence 9/9
				ret = libusb_control_transfer(devh,
							      0xc0,
							      0x06,
							      0x0000,
							      0x0000,
							      data,
							      0x0002,
							      0);
				if (ret < 0)
				{
					if (opmode == NODEAMON)
						fprintf(stderr, "Seq 1: F0 error %d\n", ret);
					else
						syslog(FACILITY, "Seq 1: F0 error %d\n", ret);
				}
				if ((unsigned int) ret < sizeof(data))
				{
					if (opmode == NODEAMON)
						fprintf(stderr, "Seq 1: short read (%d)\n", ret);
					else
						syslog(FACILITY, "Seq 1: short read (%d)\n", ret);
				}

				if (opmode == NODEAMON)
					fprintf(stderr, "Permanent monitor mode enabled.\n");
				else
					syslog(FACILITY, "Permanent monitor mode enabled.\n");

				CMD_SENT = 1;
				//sent_cmd
				libusb_release_interface(devh, 0);
			} // !CMD_SENT
		} //found device
	} //cnt > 0
	libusb_close(devh);
	libusb_exit(NULL);
	return CMD_SENT;
} // find_spf


int grepOnce(FILE *stream, const char *needle)
{
	do
	{
		char data[64 * 1024];
		if (!fgets(data, sizeof(data), stream))
		{
			return 0;
		}
		if (strstr(data, needle))
		{
			return 1;
		}
	}
	while (!feof(stream));
	return 0;
}

int cmd_start(const char *argment)
{
	if (system(NULL) == 0)
	{
		fprintf(stderr, "Command process impossible, cannot start %s\n", argment);
		return 0;
	}
	if (argment)
	{
		char cmd[80] = "";
		sprintf(cmd, "%s", argment);
		if ((system(cmd)) == -1)
		{
			if (opmode == NODEAMON)
				fprintf(stderr, "Error at command: %s\n", cmd);
			else
				syslog(FACILITY, "Error at command: %s\n", cmd);
		}
		return 1;
	}
	else
		return 0;
}

int filter(const struct dirent *dir)
{
// fnmatch uses file globbing not regular expressions!
// The following glob sorts out usb? and *:1.0 which aren't devices,
// see http://www.linux-usb.org/FAQ.html#i6
	return !fnmatch("[0-9]*[!0]", dir->d_name, 0);
}

int main(int argc, char **argv)
{
	opmode = DEAMON;
	static const char *monitor_cmd = NULL;
	static const char *desktop_cmd = NULL;
	static struct dirent **namelist;
	static struct option long_options[] =
	{
		{"foreground", no_argument,       0, 'f'},
		{"monitor",    required_argument, 0, 'm'},
		{"nomonitor",  required_argument, 0, 'n'},
		{"lcd4linux",  no_argument,	      0, 'l'},
		{"help",       no_argument,       0, 'h'}
	};
	int option_index = 0;
	int opt = getopt_long(argc, argv, "fm:n:lu:h", long_options, &option_index);

// Check the command options
	while (opt != -1)
	{
		switch (opt)
		{
			case 'f':
				opmode = NODEAMON;
				// puts ("option -f");
				break;
			case 'l':
				monitor_cmd = "lcd4linux";
				desktop_cmd = "killall -9 lcd4linux";
				break;
			case 'm':
				monitor_cmd = optarg;
				// printf ("option -m with value `%s'\n", optarg);
				break;
			case 'n':
				desktop_cmd = optarg;
				// printf ("option -n with value `%s'\n", optarg);
				break;
			case 'h':
				// puts ("option -h");
				show_help();
				exit(0);
				break;
			default:
				// puts ("default option\n");
				abort();
		}
		opt = getopt_long(argc, argv, "fm:n:lu:h", long_options, &option_index);
	}
	if (optind < argc)
	{
		printf("non-option elements detected: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		putchar('\n');
		exit(1);
	}
	if (opmode == DEAMON)
	{
		// easy signal handling
		typedef void signalfunc(int);
		signalfunc *signal(int signr, signalfunc * sighandler);
		signal(SIGHUP, SIG_IGN); // Hangup detected on controlling terminal
		signal(SIGINT, SIG_IGN); // str-c
		signal(SIGWINCH, SIG_IGN); // Window resize signal (4.3BSD, Sun)
		signal(SIGTERM, stop_deamon); // kill -15, kill default
		start_deamon();
		// we need a pid file
		char buf[80];
		sprintf(buf, "/var/run/%s.pid", LOGNAME);
		FILE *pidfile;
		if ((pidfile = fopen(buf, "w")) == NULL)
		{
			syslog(FACILITY, "Error opening pid file at %s\n", buf);
			syslog(FACILITY, "spf87hd stopped\n");
			exit(1);
		}
		else
		{
			fprintf(pidfile, "%d", getpid());
			fclose(pidfile);
		}
	}

// Everything prepared, now run the main, "never" ending loop
	while (1)
	{
		FILE *stream;
		int ret = 0;
		int i;

		if ((stream = fopen("/proc/bus/usb/devices", "r")) == NULL)
		{
			i = scandir(SYSBUS, &namelist, filter, NULL);
			if (i < 0)
			{
				if (opmode == NODEAMON)
					fprintf(stderr, "Error reading SYSBUS");
				else
					syslog(FACILITY, "Error reading SYSBUS");
				stop_deamon();
			}
			else
			{
				while (i--)
				{
					char buf[80] = "";
					strcpy(buf, SYSBUS);
					strcat(buf, namelist[i]->d_name);
					strcat(buf, "/idVendor");
					if ((stream = fopen(buf, "r")) != NULL)
					{
						if (grepOnce(stream, uvid))
						{
							fclose(stream);
							strcpy(buf, SYSBUS);
							strcat(buf, namelist[i]->d_name);
							strcat(buf, "/idProduct");
							if ((stream = fopen(buf, "r")) != NULL)
							{
								if (grepOnce(stream, upid))
								{
									fclose(stream);
									ret = 1;
								}
							}
							else
								fclose(stream);
						}
						else
							fclose(stream);
					}
					free(namelist[i]);
				}
				free(namelist);
			}
		}
		else
		{
			if (stream != NULL)
			{
				ret = grepOnce(stream, PATTERN);
				fclose(stream);
			}
		}
		if (ret)
		{

			if (CMD_SENT == -1)
			{
				CMD_SENT = find_spf();
				if (CMD_SENT == 1)
					if (monitor_cmd)
						if ((cmd_start(monitor_cmd)) == 1)
						{
							if (opmode == NODEAMON)
								printf("Monitor cmd started.\n");
							else
								syslog(FACILITY, "Monitor cmd started.\n");
						}
			}
		}
		else
		{
			if (CMD_SENT == 1)
			{
				if (opmode == NODEAMON)
					fprintf(stderr, "SPF-87H monitor device gone.\n");
				else
					syslog(FACILITY, "SPF-87H monitor device gone.\n");
				if (desktop_cmd)
				{
					if ((cmd_start(desktop_cmd)) == 1)
					{
						if (opmode == NODEAMON)
							printf("No-Monitor cmd started.\n");
						else
							syslog(FACILITY, "No-Monitor cmd started.\n");
					}
				}
			}
			CMD_SENT = -1;
		}
		sleep(1);
	} //while deamon
	syslog(FACILITY, "%s %s\n", LOGNAME, " Argh, this should not happen!");
	syslog(FACILITY, "%s %s\n", LOGNAME, " Died abnormally!");
	closelog();
	exit(1);
} //main
