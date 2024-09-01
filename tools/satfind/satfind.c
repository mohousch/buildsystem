/*
 * SatFind
 * Changed for SH4 by BPanther (https://forum.mbremer.de)
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

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>

struct signal
{
	uint32_t ber;
	uint16_t snr;
	uint16_t strength;
	fe_status_t status;
};

int get_signal(struct signal *signal_data, int fe_fd)
{
	if (ioctl(fe_fd, FE_READ_BER, &signal_data->ber) < 0)
	{
		fprintf(stderr, "frontend ioctl - Can't read BER: %d\n", errno);
		return -1;
	}
	if (ioctl(fe_fd, FE_READ_SNR, &signal_data->snr) < 0)
	{
		fprintf(stderr, "frontend ioctl - Can't read SNR: %d\n", errno);
		return -1;
	}
	if (ioctl(fe_fd, FE_READ_SIGNAL_STRENGTH, &signal_data->strength) < 0)
	{
		fprintf(stderr, "frontend ioctl - Can't read Signal Strength: %d\n", errno);
		return -1;
	}
	if (ioctl(fe_fd, FE_READ_STATUS, &signal_data->status) < 0)
	{
		fprintf(stderr, "frontend ioctl - Can't read Status: %d\n", errno);
		return -1;
	}
	return 0;
}

int signal_changed(struct signal *a, struct signal *b)
{
	return ((a->ber != b->ber) ||
		(a->snr != b->snr) ||
		(a->strength != b->strength) ||
		(a->status != b->status));
}

void get_network_name_from_nit(char *network_name, unsigned char *nit, int len)
{
	unsigned char *ptr = nit + 10;
	if (len <= 24)
	{
		network_name[0] = 0;
		return;
	}
	while ((ptr - (nit + 10) < (((nit[8] & 0x0F) << 8) | nit[9])) && (ptr < nit + len))
	{
		if (ptr[0] == 0x40)
		{
			if (ptr[1] > 30)
				ptr[1] = 30;
			memmove(network_name, ptr + 2, ptr[1]);
			network_name[ptr[1]] = 0;
			return;
		}
		else
			ptr += ptr[1] + 2;
	}
	network_name[0] = 0;
}

int main(int argc, char **argv)
{
	struct dvb_frontend_parameters feparams;
	struct dvb_frontend_info info;
	char *DMX = "/dev/dvb/adapter0/demux0";
	char *FE = "/dev/dvb/adapter0/frontend0";
	char *fe_type = "DVB-S";
	int tune = 0;
	int nocolor = 0;
	int usevfd = 0;
	int fe_fd, dmx_fd;
	fd_set rfds;
	int result;
	struct timeval tv;
	struct signal signal_quality, old_signal;
	struct dmx_sct_filter_params flt;
	unsigned char buf[1024];
	char network_name[31], old_name[31];
	int x;
	for (x = 1; x < argc; x++)
	{
		if ((!strcmp(argv[x], "--tune")))
		{
			tune = 1;
		}
		else if ((!strcmp(argv[x], "--nocolor")))
		{
			nocolor = 1;
		}
		else if ((!strcmp(argv[x], "--usevfd")))
		{
			usevfd = 1;
		}
		else if ((!strcmp(argv[x], "--demux")))
		{
			x++;
			DMX = argv[x];
		}
		else if ((!strcmp(argv[x], "--frontend")))
		{
			x++;
			FE = argv[x];
		}
		else
		{
			printf("Usage: satfind [--tune] [--nocolor] [--usevfd] [--demux <device>] [--frontend <device>]\n\n--tune : tune to 12051 V 27500 3/4 (only for DVB-S and if no GUI is running)\n--nocolor : output without color\n--usevfd : show BER/SNR/SIG at vfd device\n--demux <device> : use alternative demux device (default: /dev/dvb/adapter0/demux0)\n--frontend <device>: use alternative frontend device (default: /dev/dvb/adapter0/frontend0)\n\n");
			return 0;
		}
	}
	if ((dmx_fd = open(DMX, O_RDWR)) < 0)
	{
		perror("Can't open Demux!");
		return 1;
	}
	if ((fe_fd = open(FE, O_RDONLY)) < 0)
	{
		perror("Can't open Tuner!");
		return 1;
	}
	if (ioctl(fe_fd, FE_GET_INFO, &info) < 0)
	{
		fprintf(stderr, "frontend ioctl - Can't read frontend info: %d\n", errno);
		return -1;
	}
	if (info.type == FE_QPSK) fe_type = "DVB-S";
	else if (info.type == FE_QAM) fe_type = "DVB-C";
	else if (info.type == FE_OFDM) fe_type = "DVB-T";
	else fe_type = "DVB-?";
	if (info.type != FE_QPSK && tune)
	{
		printf("\033[01;31m--tune only for DVB-S available.\033[00m\n");
		return -1;
	}
	memset(&old_signal, 0, sizeof(old_signal));
	/* initialize demux to get the NIT */
	memset(&flt, 0, sizeof(flt));
	flt.pid = 0x10;
	flt.filter.filter[0] = 0x40;
	flt.filter.mask[0] = 0xFF;
	flt.timeout = 10000;
	flt.flags = DMX_IMMEDIATE_START;
	if (ioctl(dmx_fd, DMX_SET_FILTER, &flt) < 0)
	{
		perror("DMX_SET_FILTER");
		return -1;
	}
	/* main stuff here */
	network_name[0] = 0;
	old_name[0] = 0;
	FD_ZERO(&rfds);
	FD_SET(dmx_fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 10000;
	if (tune)
	{
		/* TP 82 (ProSiebenSat.1 Media AG) on ASTRA 1H */
		feparams.frequency = 1880000;
		feparams.inversion = 0;
		feparams.u.qpsk.symbol_rate = 27500000;
		feparams.u.qpsk.fec_inner = FEC_3_4;
	}
	while (1)
	{
		if ((result = select(dmx_fd + 1, &rfds, NULL, NULL, &tv)) > 0)
		{
			if (FD_ISSET(dmx_fd, &rfds))
			{
				/* got data */
				if ((result = read(dmx_fd, buf, sizeof(buf))) > 0)
					get_network_name_from_nit(network_name, buf, result);
				/* zero or less read - we have to restart the DMX afaik*/
				else
				{
					printf("result: %d\n", result);
					ioctl(dmx_fd, DMX_STOP, 0);
					ioctl(dmx_fd, DMX_START, 0);
					int i;
					for (i = 0; i < sizeof(network_name); i++)
						network_name[i] = 0;
				}
				/* new networkname != "" */
				if ((memcmp(network_name, old_name, sizeof(network_name)) != 0) && (network_name[0] != 0))
				{
					int count;
					for (count = strlen(network_name); count <= 10; count++)
						network_name[count] = 0x20;
					network_name[count] = 0;
					memmove(old_name, network_name, sizeof(old_name));
				}
			}
			else
				printf("that should never happen...\n");
		}
		FD_ZERO(&rfds);
		FD_SET(dmx_fd, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		if (tune)
		{
			/* TUNE and wait till a possibly LOCK is done */
			ioctl(fe_fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
			ioctl(fe_fd, FE_SET_FRONTEND, &feparams);
			ioctl(fe_fd, FE_SET_TONE, SEC_TONE_ON);
			usleep(250);
		}
		get_signal(&signal_quality, fe_fd);
		if (!signal_changed(&signal_quality, &old_signal))
			continue;
		char network_name_fin[31];
		if (network_name[0] != 0)
		{
			snprintf(network_name_fin, sizeof(network_name_fin), "%s", network_name);
		}
		else
		{
			snprintf(network_name_fin, sizeof(network_name_fin), "unknown");
		}
		if (nocolor)
			printf("Network (%s): %s, BER: %u (%u%%), SNR: %u (%u%%), SIG: %u (%u%%) - [%c%c]\n", fe_type, network_name_fin, signal_quality.ber, (signal_quality.ber / 655), signal_quality.snr, (signal_quality.snr / 655), signal_quality.strength, (signal_quality.strength / 655), signal_quality.status & FE_HAS_SIGNAL ? 'S' : ' ', signal_quality.status & FE_HAS_LOCK ? 'L' : ' ');
		else
			printf("\033[01;33mNetwork (%s): %s\033[00m \033[01;31mBER: %u (%u%%)\033[00m \033[01;34mSNR: %u (%u%%)\033[00m \033[01;32mSIG: %u (%u%%)\033[00m - \033[01;36m[%c%c]\033[00m\n", fe_type, network_name_fin, signal_quality.ber, (signal_quality.ber / 655), signal_quality.snr, (signal_quality.snr / 655), signal_quality.strength, (signal_quality.strength / 655), signal_quality.status & FE_HAS_SIGNAL ? 'S' : ' ', signal_quality.status & FE_HAS_LOCK ? 'L' : ' ');
		if (usevfd)
		{
			usleep(250);
#ifdef __sh__
			const char* VFD = "/dev/vfd";
#else
			const char* VFD = "/dev/dbox/oled0";
#endif
			FILE *out = fopen(VFD, "w");
			if (!out)
			{
				printf("unable to write to %s\n", VFD);
			}
			else
			{
				fprintf(out, "%u/%u/%u", (signal_quality.ber / 655), (signal_quality.snr / 655), (signal_quality.strength / 655));
				fclose(out);
			}
		}
	}
	/* close devices */
	close(dmx_fd);
	close(fe_fd);
	return 0;
}
