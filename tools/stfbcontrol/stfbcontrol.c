/***********************************************************************
 *
 * File: stgfb/Linux/tests/stfbcontrol/stfbcontrol.c
 * Copyright (c) 2000, 2004, 2005 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 \***********************************************************************/

#include <sys/types.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <linux/fb.h>

/*
 * This test builds against the version of stmfb.h in this source tree, rather
 * than the one that is shipped as part of the kernel headers package for
 * consistency. Normal user applications should use <linux/stmfb.h>
 */
#include <linux/stmfb.h>

void usage(void)
{
	fprintf(stderr, "Usage: stfbcontrol [a num] [cc] [cs] [cr|cy] [fa|fy|fn] [hd|he] [hr|hy] [p|n] [rf|rv]\n");
	fprintf(stderr, "\ta num - Set framebuffer global alpha transparency (range 0-255)\n");
	fprintf(stderr, "\tcc    - Enable CVBS output\n");
	fprintf(stderr, "\tcr    - Enable component RGB outputs\n");
	fprintf(stderr, "\tcs    - Enable Y/C (S-Video) outputs\n");
	fprintf(stderr, "\tcy    - Enable component YUV outputs\n");
	fprintf(stderr, "\tfa    - Turn adaptive flicker filter on\n");
	fprintf(stderr, "\tfy    - Turn simple flicker filter on\n");
	fprintf(stderr, "\tfn    - Turn flicker filter off\n");
	fprintf(stderr, "\tg num - Set framebuffer colour gain (range 0-255 = 0-100%%)\n");
	fprintf(stderr, "\thd    - Disable HDMI output\n");
	fprintf(stderr, "\the    - Enable HDMI output\n");
	fprintf(stderr, "\thr    - Set HDMI to RGB output\n");
	fprintf(stderr, "\thy    - Set HDMI to YCrCb output\n");
	fprintf(stderr, "\tp     - Set framebuffer blend mode to assume premultiplied alpha\n");
	fprintf(stderr, "\tn     - Set framebuffer blend mode to assume non-premultiplied alpha\n");
	fprintf(stderr, "\trf    - Set framebuffer colour range to full RGB\n");
	fprintf(stderr, "\trv    - Set framebuffer colour range to standard video\n");
	exit(1);
}

/*
 * Simple program to control aspects of the ST framebuffer using IOCTL extensions
 * defined in stmfb.h
 */
int main(int argc, char **argv)
{
	int fbfd;
	int alpha              = 255;
	int gain               = 255;
	int cgms               = 0;

	struct stmfbio_var_screeninfo_ex varEx = {0};
	struct stmfbio_output_configuration outputConfig = {0};

	if (argc < 2)
	{
		usage();
	}

	argc--;
	argv++;

	if ((fbfd  = open("/dev/fb0",   O_RDWR)) < 0)
	{
		perror("Unable to open framebuffer");
		exit(1);
	}

	varEx.layerid  = 0;
	varEx.caps     = 0;
	varEx.activate = STMFBIO_ACTIVATE_IMMEDIATE;

	outputConfig.outputid = 1;
	if (ioctl(fbfd, STMFBIO_GET_OUTPUT_CONFIG, &outputConfig) < 0)
		perror("Getting current output configuration failed");

	outputConfig.caps = 0;
	outputConfig.activate = STMFBIO_ACTIVATE_IMMEDIATE;
	outputConfig.analogue_config = 0;

	while (argc > 0)
	{
		switch (**argv)
		{
			case 'a':
			{
				argc--;
				argv++;
				if (argc <= 0)
				{
					fprintf(stderr, "Missing alpha value\n");
					usage();
				}

				alpha = atoi(*argv);
				if (alpha < 0 || alpha > 255)
				{
					fprintf(stderr, "Alpha value out of range\n");
					usage();
				}

				varEx.caps |= STMFBIO_VAR_CAPS_OPACITY;
				varEx.opacity = alpha;

				break;
			}
			case 'c':
			{
				outputConfig.caps |= STMFBIO_OUTPUT_CAPS_ANALOGUE_CONFIG;
				switch ((*argv)[1])
				{
					case 'c':
						outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_CVBS;
						break;
					case 'r':
						outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_RGB;
						break;
					case 'y':
						outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_YPrPb;
						break;
					case 's':
						outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_YC;
						break;
					default:
						usage();
				}
				break;
			}
			case 'f':
			{
				varEx.caps |= STMFBIO_VAR_CAPS_FLICKER_FILTER;
				switch ((*argv)[1])
				{
					case 'a':
						varEx.ff_state = STMFBIO_FF_ADAPTIVE;
						break;
					case 'y':
						varEx.ff_state = STMFBIO_FF_SIMPLE;
						break;
					case 'n':
						varEx.ff_state = STMFBIO_FF_OFF;
						break;
					default:
						usage();
				}
				break;
			}
			case 'g':
			{
				argc--;
				argv++;
				if (argc <= 0)
				{
					fprintf(stderr, "Missing gain value\n");
					usage();
				}

				gain = atoi(*argv);
				if (gain < 0 || gain > 255)
				{
					fprintf(stderr, "gain value out of range\n");
					usage();
				}

				varEx.caps |= STMFBIO_VAR_CAPS_GAIN;
				varEx.gain  = gain;

				break;
			}
			case 'h':
			{
				outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;

				switch ((*argv)[1])
				{
					case 'r':
						outputConfig.hdmi_config &= ~STMFBIO_OUTPUT_HDMI_YUV;
						break;
					case 'y':
						outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_YUV;
						break;
					case 'd':
						outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_DISABLED;
						break;
					case 'e':
						outputConfig.hdmi_config &= ~STMFBIO_OUTPUT_HDMI_DISABLED;
						break;
					default:
						usage();
				}
				break;
			}
			case 'p':
			{
				varEx.caps |= STMFBIO_VAR_CAPS_PREMULTIPLIED;
				varEx.premultiplied_alpha = 1;
				break;
			}
			case 'n':
			{
				varEx.caps |= STMFBIO_VAR_CAPS_PREMULTIPLIED;
				varEx.premultiplied_alpha = 0;
				break;
			}
			case 'r':
			{
				switch ((*argv)[1])
				{
					case 'f':
					{
						varEx.caps |= STBFBIO_VAR_CAPS_RESCALE_COLOUR_TO_VIDEO_RANGE;
						varEx.rescale_colour_to_video_range = 0;
						break;
					}
					case 'v':
					{
						varEx.caps |= STBFBIO_VAR_CAPS_RESCALE_COLOUR_TO_VIDEO_RANGE;
						varEx.rescale_colour_to_video_range = 1;
						break;
					}
					default:
						usage();
				}
				break;
			}
			case 'x':
			{
				argc--;
				argv++;
				if (argc <= 0)
				{
					fprintf(stderr, "Missing cgms value\n");
					usage();
				}

				cgms = atoi(*argv);
				/*
				 * Note: This IOCTL is an _IO _not_ an _IOW, hence takes the argument
				 * value directly, not via a pointer.
				 */
				ioctl(fbfd, STMFBIO_SET_CGMS_ANALOG, cgms);

				break;
			}
			default:
			{
				fprintf(stderr, "Unknown option '%s'\n", *argv);
				usage();
			}
		}

		argc--;
		argv++;
	}

	if (varEx.caps != STMFBIO_VAR_CAPS_NONE)
	{
		if (ioctl(fbfd, STMFBIO_SET_VAR_SCREENINFO_EX, &varEx) < 0)
			perror("setting extended screen info failed");
	}

	if (outputConfig.caps != STMFBIO_OUTPUT_CAPS_NONE)
	{
		if (ioctl(fbfd, STMFBIO_SET_OUTPUT_CONFIG, &outputConfig) < 0)
			perror("setting output configuration failed");
	}

	return 0;
}
