/*
 *   avs_core.h - audio/video switch core driver - Kathrein UFS-910
 *
 *   written by captaintrip - 19.Nov 2007
 *
 *   mainly based on avs_core.c from Gillem gillem@berlios.de / Tuxbox-Project
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#define I2C_DRIVERID_AVS	1
#define AVS_MINOR		221
 
/* Un-/Mute */
#define AVS_MUTE		1
#define AVS_UNMUTE		0
 
/* IOCTL */
#define AVSIOSET   	 	0x1000
#define AVSIOGET	 	0x2000
#define AVSIOSVSW1   (1|AVSIOSET)
#define AVSIOSVSW2   (2|AVSIOSET)
#define AVSIOSVSW3   (3|AVSIOSET)
#define AVSIOSASW1   (4|AVSIOSET)
#define AVSIOSASW2   (5|AVSIOSET)
#define AVSIOSASW3   (6|AVSIOSET)
#define AVSIOSVOL    (7|AVSIOSET)
#define AVSIOSMUTE   (8|AVSIOSET)
#define AVSIOSFBLK   (9|AVSIOSET)
#define AVSIOSFNC    (10|AVSIOSET)
#define AVSIOSYCM    (11|AVSIOSET)
#define AVSIOSZCD    (12|AVSIOSET)
#define AVSIOSLOG1   (13|AVSIOSET)
#define AVSIOSLOG2   (14|AVSIOSET)
#define AVSIOSLOG3   (15|AVSIOSET)
#define AVSIOSLOG4   (16|AVSIOSET)
#define AVSIOGVSW1   (17|AVSIOGET)
#define AVSIOGVSW2   (18|AVSIOGET)
#define AVSIOGVSW3   (19|AVSIOGET)
#define AVSIOGASW1   (20|AVSIOGET)
#define AVSIOGASW2   (21|AVSIOGET)
#define AVSIOGASW3   (22|AVSIOGET)
#define AVSIOGVOL    (23|AVSIOGET)
#define AVSIOGMUTE   (24|AVSIOGET)
#define AVSIOGFBLK   (25|AVSIOGET)
#define AVSIOGFNC    (26|AVSIOGET)
#define AVSIOGYCM    (27|AVSIOGET)
#define AVSIOGZCD    (28|AVSIOGET)
#define AVSIOGLOG1   (29|AVSIOGET)
#define AVSIOGLOG2   (30|AVSIOGET)
#define AVSIOGLOG3   (31|AVSIOGET)
#define AVSIOGLOG4   (32|AVSIOGET)
#define AVSIOGSTATUS (33|AVSIOGET)
#define AVSIOGTYPE   (34|AVSIOGET)
#define AVSIOSSCARTPIN8	(35|AVSIOSET)
#define AVSIOGSCARTPIN8	(36|AVSIOGET)
#define AVSIOSTANDBY    (99|AVSIOSET)
 
#define SAAIOSENC          4 /* set encoder (pal/ntsc)             */
#define SAAIOSMODE         5 /* set mode (rgb/fbas/svideo/component) */
#define SAAIOSWSS         10 /* set wide screen signaling data */
#define SAAIOSSRCSEL      11 /* source selection(enc/scart) */
 
#define SAA_MODE_RGB       0
#define SAA_MODE_FBAS      1
#define SAA_MODE_SVIDEO    2
#define SAA_MODE_COMPONENT 3
 
#define SAA_NTSC           0
#define SAA_PAL            1
#define SAA_PAL_M          2
 
#define SAA_WSS_43F        0
#define SAA_WSS_169F       7
#define SAA_WSS_OFF        8

#define SAA_SRC_ENC        0
#define SAA_SRC_SCART      1

#define AVS_DEBUG          0


#define dprintk(fmt...) \
	do { \
		if (debug) printk (fmt); \
	} while (0)
