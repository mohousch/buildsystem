/*
	Thanks to rhabarber1848 tuxbox dbox2 cvs!!


	This Application shows "typed" Image Informations (known from dbox2) in a text console over the framebuffer (on television)
	CONFIG_FRAMEBUFFER_CONSOLE have to be enabled!!

	It will look like this:


                    ---------- Image Information ----------

                    Image Version   : 5.1.5
                    Image Type      : Release

                    Creation Date   : 02.04.2010
                    Creation Time   : 8:18
                    Creator         : git-developer
                    Image Name      : Neutrino for Duckbox -jffs2/ext2
                    Homepage        : http://gitorious.org/open-duckbox-project-sh4

                    ---------- Network Settings -----------

                    Network State   : disabled
                    DHCP State      : -- unknown --
                    Root-Server     : intern
                    IP Address      : 192.168.1.8
                    Subnet Mask     : 255.255.255.0
                    Broadcast       : 192.168.1.255
                    Gateway         : 192.168.1.1
                    Nameserver      : 192.168.178.1

                    Linux Version 2.6.17.14_stm22_0041, gcc Version
                    Built with the computer smog@redhat.desk

                                Loading Neutrino ....


	For "typing simulation" (delay between characters) you have to start gitVCInfo with -d!
	smogm 2010
*/

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//!!you have to set CONFIG_FRAMEBUFFER_CONSOLE in kernel!! thx bpanther
#define CONSOLE "/dev/tty1"
//where to find the version information (/etc/.version??):
#define VERSION_FILE "/.version"
//more information about your box:
#define VERSION_FILE2 "/proc/version"
//network informations:
#define INTERFACES_FILE "/etc/network/interfaces"
#define NAMENSSERVER_FILE "/etc/resolv.conf"
//what is mounted into you filesystem, and how:
#define MOUNTS_FILE "/proc/mounts"


#define BUFFERSIZE 255
#define BIGBUFFERSIZE 2000
#define MAXOSD 2
#define CHARDELAY 20000

enum {gVERSION, gTYPE, gDATE, gTIME, gCREATOR, gNAME, gWWW, gNETW, gDHCP, gROOT, gIP, gNETM, gBROAD, gGATEWAY, gDNS, gHEADLINE,
      gUNKNOWN, gENABLED, gDISABLED, gINTERN, gLINUX, gGCC, gUPC, gLOAD
     };

const char *info[][MAXOSD] =
{
	{ "Image Version   :"	, "Image Version   :" },
	{ "Image Typ       :"	, "Image Type      :" },
	{ "Datum           :"	, "Creation Date   :" },
	{ "Uhrzeit         :"	, "Creation Time   :" },
	{ "Erstellt von    :"	, "Creator         :" },
	{ "Image Name      :"	, "Image Name      :" },
	{ "Homepage        :"	, "Homepage        :" },
	{ "Netzwerk Status :"	, "Network State   :" },
	{ "DHCP Status     :"	, "DHCP State      :" },
	{ "Root-Server     :"	, "Root-Server     :" },
	{ "IP Adresse      :"	, "IP Address      :" },
	{ "Netzmaske       :"	, "Subnet Mask     :" },
	{ "Broadcast       :"	, "Broadcast       :" },
	{ "Gateway         :"	, "Gateway         :" },
	{ "Nameserver      :"	, "Nameserver      :" },
	{ "-------- Netzwerkeinstellungen --------"	, "---------- Network Settings -----------" },
	{ "-- unbekannt --"		, "-- unknown --" },
	{ "aktiviert"	, "enabled"		},
	{ "deaktiviert"	, "disabled"	},
	{ "intern"	, "intern"	},
	{ "Linux Version"	, "Linux Version"	},
	{ "gcc Version"	, "gcc Version"	},
	{ "Erstellt mit dem Computer"	, "Built with the computer"	},
	{ "Lade"		, "Loading"		}
};

int main(int argc, char **argv)
{
	switch (fork())
	{
		case -1:
			perror("[gitVCInfo] fork");
			return -1;
		case 0:
			break;
		default:
			return 0;
	}

	if (setsid() < 0)
	{
		perror("[gitVCInfo] setsid");
		return 1;
	}

	unsigned int id = 1;
	int opt = -2;
	char buf[BUFFERSIZE] = "";
	int release_type = -1;
	int imageversion = 0;
	int imagesubver = 0;
	char imagesubver2[BUFFERSIZE] = "0";
	int year = 9999;
	int month = 99;
	int day = 99;
	int hour = 99;
	int minute = 99;
	bool delay = false;
	int dhcp = 0;
	int nic_on = 0;
	char *imagetyp = "squashfs";
	char linuxversion[24] = "";
	char gccversion[50] = "";
	char userpc[24];
	char ladename[BUFFERSIZE] = "System";
	char creator[BUFFERSIZE];
	char imagename[BUFFERSIZE];
	char homepage[BUFFERSIZE];
	char root[BUFFERSIZE];
	char address[BUFFERSIZE];
	char broadcast[BUFFERSIZE];
	char netmask[BUFFERSIZE];
	char nameserver[BUFFERSIZE];
	char gateway[BUFFERSIZE];
	char null[BUFFERSIZE] = "";
	char versioninfo[20];
	char git_revision[] = "$Revision: 1.7 $";
	sscanf(git_revision, "%*s %s", versioninfo);

	while ((opt = getopt(argc, argv, "hgdn:")) != -1)
	{
		switch (opt)
		{
			case 'h':
				if (argc < 3)
				{
					printf("gitVCInfo - bootinfo on screen, v%s\n", versioninfo);
					printf("Usage: gitVCInfo [-d] [-g] [-n name] [-h]\n");
					printf("\nPossible options:\n");
					printf("\t-h\t\tprint this usage information\n");
					printf("\t-g\t\tprint bootinfo in german\n");
					printf("\t-d\t\tdelay on\n");
					printf("\t-n name\t\tspecial output (e.g. -n Neutrino)\n");
					exit(0);
				}
				break;
			case 'g':
				id = 0;
				break;
			case 'd':
				delay = true;
				break;
			case 'n':
				strcpy(ladename, optarg);
				break;
			default:
				break;
		}
	}

	strcpy(creator, info[gUNKNOWN][id]);
	strcpy(imagename, info[gUNKNOWN][id]);
	strcpy(homepage, info[gUNKNOWN][id]);
	strcpy(root, info[gINTERN][id]);
	strcpy(address, info[gUNKNOWN][id]);
	strcpy(broadcast, info[gUNKNOWN][id]);
	strcpy(netmask, info[gUNKNOWN][id]);
	strcpy(nameserver, info[gUNKNOWN][id]);
	strcpy(gateway, info[gUNKNOWN][id]);

	FILE *fv1 = fopen(VERSION_FILE, "r");
	if (fv1)
	{
		while (fgets(buf, BUFFERSIZE, fv1))
		{
			sscanf(buf, "version=%1d%1d%1d%1s%4d%2d%2d%2d%2d",
			       &release_type, &imageversion, &imagesubver, (char *) &imagesubver2,
			       &year, &month, &day, &hour, &minute);
			sscanf(buf, "creator=%[^\n]", (char *) &creator);
			sscanf(buf, "imagename=%[^\n]", (char *) &imagename);
			sscanf(buf, "homepage=%[^\n]", (char *) &homepage);
		}
		fclose(fv1);
	}

	FILE *fv2 = fopen(INTERFACES_FILE, "r");
	if (fv2)
	{
		while (fgets(buf, BUFFERSIZE, fv2))
		{
			if (nic_on == 0)
			{
				if (sscanf(buf, "auto eth%[0]", (char *) &null))
				{
					nic_on = 1;
				}
			}
			if (sscanf(buf, "iface eth0 inet stati%[c]", (char *) &null))
			{
				dhcp = 1;
			}
			else if (sscanf(buf, "iface eth0 inet dhc%[p]", (char *) &null))
			{
				dhcp = 2;
			}
		}
		fclose(fv2);
	}

	FILE *fv3 = fopen(NAMENSSERVER_FILE, "r");
	if (fv3)
	{
		while (fgets(buf, BUFFERSIZE, fv3))
		{
			sscanf(buf, "nameserver %[^\n]", (char *) &nameserver);
		}
		fclose(fv3);
	}

	FILE *fv4 = popen("/sbin/ifconfig eth0", "r");
	if (fv4)
	{
		while (fgets(buf, BUFFERSIZE, fv4))
		{
			sscanf(buf, " inet addr:%s  Bcast:%s  Mask:%[^\n]", (char *) &address, (char *) &broadcast, (char *) &netmask);
		}
		fclose(fv4);
	}

	FILE *fv5 = popen("/sbin/route -n", "r");
	if (fv5)
	{
		fscanf(fv5, "%*[^\n]\n%*[^\n]\n%*[^\n]\n");
		while (fgets(buf, BUFFERSIZE, fv5))
		{
			sscanf(buf, "%s %[0-9.]", (char *) &null, (char *) &gateway);
		}
		fclose(fv5);
	}

	FILE *fv6 = fopen(MOUNTS_FILE, "r"); //Root-Server IP ermitteln, falls nfsboot
	if (fv6)
	{
		while (fgets(buf, BUFFERSIZE, fv6) != NULL)
		{
			sscanf(buf, "/dev/root / nfs rw,v2,rsize=4096,wsize=4096,hard,udp,nolock,addr=%s", (char *) &root);
		}
		fclose(fv6);
	}

	FILE *fv7 = fopen(VERSION_FILE2, "r"); //Versionsdatei (/proc/version) auswerten
	if (fv7)
	{
		while (fgets(buf, BUFFERSIZE, fv2) != NULL)
		{
			sscanf(buf, "Linux version %s (%s) (%s) ", (char *) &linuxversion, (char *) &userpc, (char *) &gccversion/*, (char *) &gccversion, (char *) &gccversion*/);
			//Linux version 2.6.17.14_stm22_0041 (BrechREiZ@rhel) (gcc-Version 4.1.1 (STMicroelectronics/Linux Base 4.1.1-23)) #1 PREEMPT Sun Mar 28 20:58:08 CEST
		}
		fclose(fv7);
	}

	FILE *fv8 = fopen(VERSION_FILE, "a"); //Versionsdatei (/.version) beschreibbar, dann jffs2/ext2...
	if (fv8)
	{
		fclose(fv8);
		imagetyp = "jffs2/ext2";
	}

	char message2[BUFFERSIZE];
	strcpy(message2, "");
	if (delay)
		sprintf(message2, "%s %s .... ", info[gLOAD][id], ladename);

	char message[BIGBUFFERSIZE];
	strcpy(message, "");
	sprintf(message,
		"\n\n\n\n"
		"\t\t    ---------- Image Information ----------\n\n"
		"\t\t    %s %d.%d.%s\n"						//Image Version
		"\t\t    %s %s\n\n"						//Image Typ
		"\t\t    %s %02d.%02d.%d\n"					//Date
		"\t\t    %s %d:%02d\n"						//Time
		"\t\t    %s %s\n"						//Creator
		"\t\t    %s %s-%s\n"						//Image Name
		"\t\t    %s %s\n\n"						//Homepage
		"\t\t    %s\n\n"
		"\t\t    %s %s\n"						//Network state
		"\t\t    %s %s\n"						//DHCP state
		"\t\t    %s %s\n"						//Root-Server
		"\t\t    %s %s\n"						//IP Adress
		"\t\t    %s %s\n"						//Subnet
		"\t\t    %s %s\n"						//Broadcast
		"\t\t    %s %s\n"						//Gateway
		"\t\t    %s %s\n\n"						//Nameserver
		"\t\t    %s %s, %s %s\n "					//Linux Version, gcc Version
		"\t\t    %s %s\n\n"						//User, PC
		"\t\t\t\t%s",
		info[gVERSION][id], imageversion, imagesubver, imagesubver2,
		info[gTYPE][id], release_type == 0 ? "Release"
		: release_type == 1 ? "Snapshot"
		: release_type == 2 ? "Intern"
		:	info[gUNKNOWN][id],
		info[gDATE][id], day, month, year,
		info[gTIME][id], hour, minute,
		info[gCREATOR][id], creator,
		info[gNAME][id], imagename, imagetyp,
		info[gWWW][id], homepage,
		info[gHEADLINE][id],
		info[gNETW][id], nic_on == 0 ? info[gDISABLED][id] : nic_on == 1 ? info[gENABLED][id] : info[gUNKNOWN][id],
		info[gDHCP][id], dhcp == 1 ? info[gDISABLED][id] : dhcp == 2 ? info[gENABLED][id] : info[gUNKNOWN][id],
		info[gROOT][id], root,
		info[gIP][id], address,
		info[gNETM][id], netmask,
		info[gBROAD][id], broadcast,
		info[gGATEWAY][id], gateway,
		info[gDNS][id], nameserver,
		info[gLINUX][id], linuxversion,
		info[gGCC][id], gccversion,
		info[gUPC][id], userpc,
		message2);

	FILE *fb = fopen(CONSOLE, "w");
	if (fb == 0)
	{
		perror("[gitVCInfo] fopen");
		exit(1);
	}

	if (!delay)
	{
		for (unsigned int i = 0; i < strlen(message); i++)
		{
			fputc(message[i], fb);
			fputc(message[i], stdout);
			fflush(fb);
			fflush(stdout);
		}
	}
	else
	{
		sprintf(message2, "%s %s .... ", info[gLOAD][id], ladename);
		for (unsigned int i = 0; i < strlen(message); i++)
		{
			fputc(message[i], fb);
			fputc(message[i], stdout);
			fflush(fb);
			fflush(stdout);
			usleep(CHARDELAY);
		}
	}
	fputc('\n', fb);
	fputc('\n', stdout);
	fclose(fb);
	return 0;
}
