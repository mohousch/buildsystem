/*
 * micom_ioctl.c
 *
 * (c) 2009 Dagobert@teamducktales
 * (c) 2010 Schischu & konfetti: Add irq handling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/termbits.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,17)
#include <linux/stm/pio.h>
#else
#include <linux/stpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/poll.h>

#include "micom.h"
#include "micom_asc.h"
#include "../vfd/utf.h"

#if defined(UFI510) || defined(UFC960)
// ROM_PT6315
unsigned int VFD_CHARTABLE[256] =
{
	0x0000,//0x00, icon play
	0x0000,//0x01, icon stop
	0x0000,//0x02, icon pause
	0x0000,//0x03, icon ff
	0x0000,//0x04, icon rewind
	0x0000,//0x05, rec
	0x0000,//0x06, arrow
	0x0000,//0x07, clock
	0x0000,//0x08, usb
	0x0000,//0x09, 1
	0x0000,//0x0a, 2
	0x0000,//0x0b, 16_9
	0x0000,//0x0c, dvb
	0x0000,//0x0d, hdd
	0x0000,//0x0e, link
	0x0000,//0x0f, audio

	0x0000,//0x10,video
	0x0000,//0x11,,dts
	0x0000,//0x12,dolby
	0x0000,//0x13,,reserved
	0x0000,//0x14,,reserved
	0x0000,//0x15,,reserved
	0x0000,//0x16,,reserved
	0x0000,//0x17,,reserved
	0x0000,//0x18,,reserved
	0x0000,//0x19,,reserved
	0x0000,//0x1a,,reserved
	0x0000,//0x1b,,reserved
	0x0000,//0x1c,,reserved
	0x0000,//0x1d,,reserved
	0x0000,//0x1e,,reserved
	0x0000,//0x1f, , reserved

	0x0000,//0x20, <space>
	0x4008,//0x21,!
	0x0000,//0x22,"
	0x0000,//0x23,#
	0xC7CB,//0x24,$
	0x0000,//0x25,%
	0x0000,//0x26,&
	0x2000,//0x27,'
	0x2010,//0x28,(
	0x1004,//0x29,)
	0x301C,//0x2a,*
	0x438A,//0x2b,+
	0x0004,//0x2c,,
	0x0380,//0x2d,-
	0x0000,//0x2e,.
	0x2004,//0x2f, /

	0x8C61,//0x30,0
	0x410A,//0x31,1
	0x8BA1,//0x32,2
	0x8BC1,//0x33,3
	0x0FC0,//0x34,4
	0x87C1,//0x35,5
	0x87E1,//0x36,6
	0x8840,//0x37,7
	0x8FE1,//0x38,8
	0x8FC1,//0x39,9
	0x4102,//0x3a,:
	0x4004,//0x3b,;
	0x2010,//0x3c,<
	0x0381,//0x3d,=
	0x1004,//0x3e,>
	0x8BA0,//0x3f, ?

	0x0000,//0x40,@
	0x8FE0,//0x41,A
	0xA5B1,//0x42,B
	0x8421,//0x43,C
	0x9945,//0x44,D
	0x87A1,//0x45,E
	0x85A0,//0x46,F
	0x8731,//0x47,G
	0x0FE0,//0x48,H
	0xC10B,//0x49,I
	0xA015,//0x4a,J
	0x25B0,//0x4b,K
	0x0421,//0x4c,L
	0x3d60,//0x4d,M
	0x1D70,//0x4e,N
	0x8c61,//0x4f,O

	0x8FA0,//0x50,P
	0x8FC0,//0x51,Q
	0xA5B0,//0x52,R
	0x87C1,//0x53,S
	0xC10A,//0x54,T
	0x0C61,//0x55,U
	0x2524,//0x56,V
	0x0D74,//0x57,W
	0x3114,//0x58,X
	0x3102,//0x59,Y
	0xA105,//0x5a,Z
	0x8421,//0x5b,[
	0x1010,//0x5c,backslash
	0x8841,//0x5d,]
	0x0014,//0x5e,^
	0x0001,//0x5f,_

	0x1000,//0x60,`
	0x8FE0,//0x61,a
	0xA5B1,//0x62,b
	0x8421,//0x63,c
	0x9945,//0x64,d
	0x87A1,//0x65,e
	0x85A0,//0x66,f
	0x8731,//0x67,g
	0x0FE0,//0x68,h
	0xC10B,//0x69,i
	0xA015,//0x6a,j
	0x25B0,//0x6b,k
	0x0421,//0x6c,l
	0x3d60,//0x6d,m
	0x1D70,//0x6e,n
	0x8c61,//0x6f,o

	0x8FA0,//0x70,p
	0x8FC0,//0x71,q
	0xA5B0,//0x72,r
	0x87C1,//0x73,s
	0xC10A,//0x74,t
	0x0C61,//0x75,u
	0x2524,//0x76,v
	0x0D74,//0x77,w
	0x3114,//0x78,x
	0x3102,//0x79,y
	0xA105,//0x7a,z
	0x2010,//0x7b,{
	0x410A,//0x7c,|
	0x1004,//0x7d,}
	0x0000,//0x7e,~
	0x0000,//0x7f, <DEL>--> all light on

	0x8FE0,//0x80,a -umulaut
	0x33E1,//0x81,o -umulaut
	0x3061,//0x82,u -umulaut
	0x8FE0,//0x83,A -umulaut
	0x33E1,//0x84,O -umulaut
	0x3061,//0x85,U -umulaut
	0xA5B1,//0x86,sz- estset
	0x0000,//0x87,reserved
	0x0000,//0x88,reserved
	0x0000,//0x89,reserved
	0x0000,//0x8a,reserved
	0x0000,//0x8b,reserved
	0x0000,//0x8c,reserved
	0x0000,//0x8d,reserved
	0x0000,//0x8e,reserved
	0x0000,//0x8f,	reserved

	0x0000,//0x90,reserved
	0x0000,//0x91,reserved
	0x0000,//0x92,reserved
	0x0000,//0x93,reserved
	0x0000,//0x94,reserved
	0x0000,//0x95,reserved
	0x0000,//0x96,reserved
	0x0000,//0x97,reserved
	0x0000,//0x98,reserved
	0x0000,//0x99,reserved
	0x0000,//0x9a,reserved
	0x0000,//0x9b,reserved
	0x0000,//0x9c,reservedr
	0x0000,//0x9d,reserved
	0x0000,//0x9e,reserved
	0x0000,//0x9f, reserved

	0x0000,//0xa0,reserved
	0x0000,//0xa1,reserved
	0x0000,//0xa2,reserved
	0x0000,//0xa3,reserved
	0x0000,//0xa4,reserved
	0x0000,//0xa5,reserved
	0x0000,//0xa6,reserved
	0x0000,//0xa7,reserved
	0x0000,//0xa8,reserved
	0x0000,//0xa9,reserved
	0x0000,//0xaa,reserved
	0x0000,//0xab,reserved
	0x0000,//0xac,eserved
	0x0000,//0xad,reserved
	0x0000,//0xae,reserved
	0x0000,//0xaf, reserved

	0x0000,//0xb0, reserved
	0x0000,//0xb1, reserved
	0x0000,//0xb2, reserved
	0x0000,//0xb3, reserved
	0x0000,//0xb4, reserved
	0x0000,//0xb5, reserved
	0x0000,//0xb6, reserved
	0x0000,//0xb7, reserved
	0x0000,//0xb8, reserved
	0x0000,//0xb9, reserved
	0x0000,//0xba, reserved
	0x0000,//0xbb, reserved
	0x0000,//0xbc,reserved
	0x0000,//0xbd, reserved
	0x0000,//0xbe, reserved
	0x0000,//0xbf, reserved

	0x0000,//0xc0, reserved
	0x0000,//0xc1, reserved
	0x0000,//0xc2, reserved
	0x0000,//0xc3, reserved
	0x0000,//0xc4, reserved
	0x0000,//0xc5, reserved
	0x0000,//0xc6, reserved
	0x0000,//0xc7, reserved
	0x0000,//0xc8, reserved
	0x0000,//0xc9, reserved
	0x0000,//0xca, reserved
	0x0000,//0xcb, reserved
	0x0000,//0xcc, reserved
	0x0000,//0xcd, reserved
	0x0000,//0xce, reserved
	0x0000,//0xcf, reserved

	0x0000,//0xd0, reserved
	0x0000,//0xd1, reserved
	0x0000,//0xd2, reserved
	0x0000,//0xd3, reserved
	0x0000,//0xd4, reserved
	0x0000,//0xd5, reserved
	0x0000,//0xd6, reserved
	0x0000,//0xd7, reserved
	0x0000,//0xd8, reserved
	0x0000,//0xd9, reserved
	0x0000,//0xda, reserved
	0x0000,//0xdb, reserved
	0x0000,//0xdc,reserved
	0x0000,//0xdd, reserved
	0x0000,//0xde, reserved
	0x0000,//0xdf, reserved

	0x0000,//0xe0, reserved
	0x0000,//0xe1, reserved
	0x0000,//0xe2, reserved
	0x0000,//0xe3, reserved
	0x0000,//0xe4, reserved
	0x0000,//0xe5, reserved
	0x0000,//0xe6, reserved
	0x0000,//0xe7, reserved
	0x0000,//0xe8, reserved
	0x0000,//0xe9, reserved
	0x0000,//0xea, reserved
	0x0000,//0xeb, reserved
	0x0000,//0xec,reserved
	0x0000,//0xed, reserved
	0x0000,//0xee, All on
	0x0000,//0xef, All off

	0x0001,//0xf0,	All on
	0x0003,//0xf1,	All Off
	0x0007,//0xf2,	reserved
	0x000f,//0xf3,	reserved
	0x001f,//0xf4,	reserved
	0x003f,//0xf5,	reserved
	0x007f,//0xf6,	reserved
	0x00ff,//0xf7,	reserved
	0x01ff,//0xf8,	reserved
	0x03ff,//0xf9,	reserved
	0x07ff,//0xfa,	reserved
	0x0fff,//0xfb,	reserved
	0x1fff,//0xfc, reserved
	0x3fff,//0xfd,	reserved
	0x7fff,//0xfe,	reserved
	0xffff,//0xff, reservedr


};
#else
// ROM_KATHREIN
unsigned char VFD_CHARTABLE[256] =
{
	0x2e,//0x00, icon play
	0x8f,//0x01, icon stop
	0xe4,//0x02, icon pause
	0xdd,//0x03, icon ff
	0xdc,//0x04, icon rewind
	0x10,//0x05,
	0x10,//0x06,
	0x10,//0x07,
	0x10,//0x08,
	0x10,//0x09,
	0x10,//0x0a,
	0x10,//0x0b,
	0x10,//0x0c,
	0x10,//0x0d,
	0x10,//0x0e,
	0x10,//0x0f,

	0x10,//0x10,,reserved
	0x11,//0x11,,reserved
	0x12,//0x12,,reserved
	0x13,//0x13,,reserved
	0x14,//0x14,,reserved
	0x15,//0x15,,reserved
	0x16,//0x16,,reserved
	0x17,//0x17,,reserved
	0x18,//0x18,,reserved
	0x19,//0x19,,reserved
	0x1a,//0x1a,,reserved
	0x1b,//0x1b,,reserved
	0x1c,//0x1c,,reservedr
	0x10,//0x1d,,reserved
	0x10,//0x1e,,reserved
	0x10,//0x1f, , reserved

	0x20,//0x20, <space>
	0x21,//0x21,!
	0x22,//0x22,"
	0x23,//0x23,#
	0x24,//0x24,$
	0x25,//0x25,%
	0x26,//0x26,&
	0x27,//0x27,'
	0x28,//0x28,(
	0x29,//0x29,)
	0x2a,//0x2a,*
	0x2b,//0x2b,+
	0x2c,//0x2c,,
	0x2d,//0x2d,-
	0x2e,//0x2e,.
	0x2f,//0x2f, /

	0x30,//0x30,0
	0x31,//0x31,1
	0x32,//0x32,2
	0x33,//0x33,3
	0x34,//0x34,4
	0x35,//0x35,5
	0x36,//0x36,6
	0x37,//0x37,7
	0x38,//0x38,8
	0x39,//0x39,9
	0x3a,//0x3a,:
	0x3b,//0x3b,;
	0x3c,//0x3c,<
	0x3d,//0x3d,=
	0x3e,//0x3e,>
	0x3f,//0x3f, ?

	0x40,//0x40,@
	0x41,//0x41,A
	0x42,//0x42,B
	0x43,//0x43,C
	0x44,//0x44,D
	0x45,//0x45,E
	0x46,//0x46,F
	0x47,//0x47,G
	0x48,//0x48,H
	0x49,//0x49,I
	0x4a,//0x4a,J
	0x4b,//0x4b,K
	0x4c,//0x4c,L
	0x4d,//0x4d,M
	0x4e,//0x4e,N
	0x4f,//0x4f,O _

	0x50,//0x50,P
	0x51,//0x51,Q
	0x52,//0x52,R
	0x53,//0x53,S
	0x54,//0x54,T
	0x55,//0x55,U
	0x56,//0x56,V  f
	0x57,//0x57,W
	0x58,//0x58,X
	0x59,//0x59,Y  i
	0x5a,//0x5a,Z  j
	0x5b,//0x5b,[
	0x5c,//0x5c,  l
	0x5d,//0x5d,]
	0x5e,//0x5e,^
	0x5f,//0x5f,

	0x60,//0x60,`
	0x61,//0x61,a
	0x62,//0x62,b  r
	0x63,//0x63,c
	0x64,//0x64,d  t
	0x65,//0x65,e
	0x66,//0x66,
	0x67,//0x67,g
	0x68,//0x68,h
	0x69,//0x69,
	0x6a,//0x6a,
	0x6b,//0x6b,k
	0x6c,//0x6c,  |
	0x6d,//0x6d,m
	0x6e,//0x6e,n
	0x6f,//0x6f, o

	0x70,//0x70,p
	0x71,//0x71,q
	0x72,//0x72,
	0x73,//0x73,s
	0x74,//0x74,
	0x75,//0x75,u
	0x76,//0x76,v
	0x77,//0x77,w
	0x78,//0x78,x
	0x79,//0x79,y
	0x7a,//0x7a,z
	0x7b,//0x7b,{
	0x7c,//0x7c,
	0x7d,//0x7d,}
	0x7e,//0x7e,~
	0x7f,//0x7f, <DEL>--> all light on

	0x84,//0x80,a -umulaut
	0x94,//0x81,o -umulaut
	0x81,//0x82,u -umulaut
	0x8e,//0x83,A -umulaut
	0x99,//0x84,O -umulaut
	0x9a,//0x85,U -umulaut
	0xb1,//0x86,©¬- estset
	0x10,//0x87,reserved
	0x10,//0x88,reserved
	0x10,//0x89,reserved
	0x10,//0x8a,reserved
	0x10,//0x8b,reserved
	0x10,//0x8c,reserved
	0x10,//0x8d,reserved
	0x10,//0x8e,reserved
	0x10,//0x8f,  reserved

	0x10,//0x90,reserved
	0x10,//0x91,reserved
	0x10,//0x92,reserved
	0x10,//0x93,reserved
	0x10,//0x94,reserved
	0x10,//0x95,reserved
	0x10,//0x96,reserved
	0x10,//0x97,reserved
	0x10,//0x98,reserved
	0x10,//0x99,reserved
	0x10,//0x9a,reserved
	0x10,//0x9b,reserved
	0x10,//0x9c,reservedr
	0x10,//0x9d,reserved
	0x10,//0x9e,reserved
	0x10,//0x9f, reserved

	0x10,//0xa0,reserved
	0x10,//0xa1,reserved
	0x10,//0xa2,reserved
	0x10,//0xa3,reserved
	0x10,//0xa4,reserved
	0x10,//0xa5,reserved
	0x10,//0xa6,reserved
	0x10,//0xa7,reserved
	0x10,//0xa8,reserved
	0x10,//0xa9,reserved
	0x10,//0xaa,reserved
	0x10,//0xab,reserved
	0x10,//0xac,eserved
	0x10,//0xad,reserved
	0x10,//0xae,reserved
	0x10,//0xaf, reserved

	0x10,//0xb0, reserved
	0x10,//0xb1, reserved
	0x10,//0xb2, reserved
	0x10,//0xb3, reserved
	0x10,//0xb4, reserved
	0x10,//0xb5, reserved
	0x10,//0xb6, reserved
	0x10,//0xb7, reserved
	0x10,//0xb8, reserved
	0x10,//0xb9, reserved
	0x10,//0xba, reserved
	0x10,//0xbb, reserved
	0x10,//0xbc,reserved
	0x10,//0xbd, reserved
	0x10,//0xbe, reserved
	0x10,//0xbf, reserved

	0x10,//0xc0, reserved
	0x10,//0xc1, reserved
	0x10,//0xc2, reserved
	0x10,//0xc3, reserved
	0x10,//0xc4, reserved
	0x10,//0xc5, reserved
	0x10,//0xc6, reserved
	0x10,//0xc7, reserved
	0x10,//0xc8, reserved
	0x10,//0xc9, reserved
	0x10,//0xca, reserved
	0x10,//0xcb, reserved
	0x10,//0xcc, reserved
	0x10,//0xcd, reserved
	0x10,//0xce, reserved
	0x10,//0xcf, reserved

	0x10,//0xd0, reserved
	0x10,//0xd1, reserved
	0x10,//0xd2, reserved
	0x10,//0xd3, reserved
	0x10,//0xd4, reserved
	0x10,//0xd5, reserved
	0x10,//0xd6, reserved
	0x10,//0xd7, reserved
	0x10,//0xd8, reserved
	0x10,//0xd9, reserved
	0x10,//0xda, reserved
	0x10,//0xdb, reserved
	0x10,//0xdc,reserved
	0x10,//0xdd, reserved
	0x10,//0xde, reserved
	0x10,//0xdf, reserved

	0x10,//0xe0, reserved
	0x10,//0xe1, reserved
	0x10,//0xe2, reserved
	0x10,//0xe3, reserved
	0x10,//0xe4, reserved
	0x10,//0xe5, reserved
	0x10,//0xe6, reserved
	0x10,//0xe7, reserved
	0x10,//0xe8, reserved
	0x10,//0xe9, reserved
	0x10,//0xea, reserved
	0x10,//0xeb, reserved
	0x10,//0xec,reserved
	0x10,//0xed, reserved
	0x10,//0xee, reserved
	0x10,//0xef, reserved

	0x10,//0xf0,  reserved
	0x10,//0xf1,  reserved
	0x10,//0xf2,  reserved
	0x10,//0xf3,  reserved
	0x10,//0xf4,  reserved
	0x10,//0xf5,  reserved
	0x10,//0xf6,  reserved
	0x10,//0xf7,  reserved
	0x10,//0xf8,  reserved
	0x10,//0xf9,  reserved
	0x10,//0xfa,  reserved
	0x10,//0xfb,  reserved
	0x10,//0xfc, reserved
	0x10,//0xfd,  reserved
	0x10,//0xfe,  reserved
	0x10,//0xff, reservedr
};
#endif

extern void ack_sem_up(void);
extern int  ack_sem_down(void);
extern void micom_putc(unsigned char data);

struct semaphore     write_sem;

int errorOccured = 0;

static char ioctl_data[8];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
	int   length;
	char  data[20];
};

static struct saved_data_s lastdata;

void write_sem_up(void)
{
	up(&write_sem);
}

int write_sem_down(void)
{
	return down_interruptible(&write_sem);
}

void copyData(unsigned char *data, int len)
{
	printk("%s len %d\n", __func__, len);
	memcpy(ioctl_data, data, len);
}

int micomWriteCommand(char command, char *buffer, int len, int needAck)
{
	int i;

	dprintk(100, "%s >\n", __func__);

#ifdef DIRECT_ASC
	serial_putc(command);
#else
	micom_putc(command);
#endif

	for (i = 0; i < len; i++)
	{
		dprintk(201, "Put: %c\n", buffer[i]);
		udelay(1);
#ifdef DIRECT_ASC
		serial_putc(buffer[i]);
#else
		micom_putc(buffer[i]);
#endif
	}

	if (needAck)
		if (ack_sem_down())
			return -ERESTARTSYS;

	dprintk(100, "%s < \n", __func__);

	return 0;
}

int micomSetRCcode(int code)
{
	char buffer[8];
	int  res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	buffer[0] = 0x02;
	buffer[1] = 0xff;
	buffer[2] = 0x80;
	buffer[3] = 0x48;
	buffer[4] = code & 0x07;	// RC code 1 to 4
	res = micomWriteCommand(0x55, buffer, 7, 0);

	memset(buffer, 0, 8);
	buffer[0] = 0x2;
	res = micomWriteCommand(0x3, buffer, 7, 0);
	msleep(10);
	return res;
}

int micomSetIcon(int which, int on)
{
	char buffer[8];
	int  res = 0;

	dprintk(100, "%s > %d, %d\n", __func__, which, on);
	if (which < 1 || which > 16)
	{
		printk("VFD/MICOM icon number out of range %d\n", which);
		return -EINVAL;
	}

	memset(buffer, 0, 8);
	buffer[0] = which;

	if (on == 1)
		res = micomWriteCommand(0x11, buffer, 7, 1);
	else
		res = micomWriteCommand(0x12, buffer, 7, 1);

	dprintk(100, "%s <\n", __func__);

	return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetIcon);

int micomSetLED(int which, int on)
{
	char buffer[8];
	int  res = 0;

	dprintk(100, "%s > %d, %d\n", __func__, which, on);

#if defined(UFS922)
	if (which < 1 || which > 6)
	{
		printk("VFD/MICOM led number out of range %d\n", which);
		return -EINVAL;
	}
#elif defined(UFS912) || defined(UFS913)
	/* 0x02 = green
	 * 0x03 = red
	 * 0x04 = left  (manually build-in)
	 * 0x05 = right (manually build-in)
	 */
	if (which < 0x02 || which > 0x05)
	{
		printk("VFD/MICOM led number out of range %d\n", which);
		return -EINVAL;
	}
#else
	/* 0x02 = green
	 * 0x03 = red
	 */
	if (which < 0x02 || which > 0x03)
	{
		printk("VFD/MICOM led number out of range %d\n", which);
		return -EINVAL;
	}
#endif

	memset(buffer, 0, 8);
	buffer[0] = which;

	if (on == 1)
		res = micomWriteCommand(0x06, buffer, 7, 1);
	else
		res = micomWriteCommand(0x22, buffer, 7, 1);

	dprintk(100, "%s <\n", __func__);

	return res;
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetLED);


int micomSetBrightness(int level)
{
	char buffer[8];
	int  res = 0;

	dprintk(5, "%s > %d\n", __func__, level);
	if (level < 1 || level > 5)
	{
		printk("VFD/MICOM brightness out of range %d\n", level);
		return -EINVAL;
	}

	memset(buffer, 0, 8);
	buffer[0] = level;

	res = micomWriteCommand(0x25, buffer, 7, 1);

	dprintk(10, "%s <\n", __func__);

	return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(micomSetBrightness);

int micomSetLedBrightness(int level)
{
	char buffer[8];
	int  res = 0;

	dprintk(5, "%s > %d\n", __func__, level);
	if (level < 0 || level > 0xff)
	{
		printk("VFD/MICOM button brightness out of range %d\n", level);
		return -EINVAL;
	}

	memset(buffer, 0, 8);
	buffer[0] = level;

	res = micomWriteCommand(0x07, buffer, 7, 1);

	dprintk(10, "%s <\n", __func__);

	return res;
}

EXPORT_SYMBOL(micomSetLedBrightness);

#if defined(UFS922) || defined(UFC960)
int micomSetModel(void)
{
	char buffer[8];
	int  res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);
	buffer[0] = 0x1;

	res = micomWriteCommand(0x3, buffer, 7, 0);

	dprintk(10, "%s <\n", __func__);

	return res;
}
#else
int micomInitialize(void)
{
	char buffer[8];
	int  res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	/* unknown command:
	 * modifications of most of the values leads to a
	 * not working fp which can only be fixed by switching
	 * off receive. value 0x46 can be modified and resetted.
	 * changing the values behind 0x46 has no effect for me.
	 */
	buffer[0] = 0x02;
	buffer[1] = 0xff;
	buffer[2] = 0x80;
	buffer[3] = 0x48;
	buffer[4] = 0x01;	// RC code 1 to 4

	res = micomWriteCommand(0x55, buffer, 7, 0);

	memset(buffer, 0, 8);
	buffer[0] = 0x2;

	res = micomWriteCommand(0x3, buffer, 7, 0);

	dprintk(10, "%s <\n", __func__);

	return res;
}
#endif


int micomSetStandby(char *time)
{
	char       buffer[8];
	int      res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	if (time[0] == '\0')
	{
		/* clear wakeup time */
		res = micomWriteCommand(0x33, buffer, 7, 1);
	}
	else
	{
		/* set wakeup time */

		memcpy(buffer, time, 5);
		res = micomWriteCommand(0x32, buffer, 7, 1);
	}

	if (res < 0)
	{
		printk("%s <res %d \n", __func__, res);
		return res;
	}

	memset(buffer, 0, 8);
	/* enter standby */
	res = micomWriteCommand(0x41, buffer, 7, 0);

	dprintk(10, "%s <\n", __func__);

	return res;
}

int micomSetTime(char *time)
{
	char       buffer[8];
	int      res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	memcpy(buffer, time, 5);
	res = micomWriteCommand(0x31, buffer, 7, 1);

	dprintk(10, "%s <\n", __func__);

	return res;
}

int micomGetVersion(void)
{
	char       buffer[8];
	int        res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	errorOccured   = 0;
	res = micomWriteCommand(0x05, buffer, 7, 1);

	if (res < 0)
	{
		printk("%s < res %d\n", __func__, res);
		return res;
	}

	if (errorOccured)
	{
		memset(ioctl_data, 0, 8);
		printk("error\n");

		res = -ETIMEDOUT;
	}
	else
	{
		/* version received ->noop here */
		dprintk(1, "version received\n");
	}

	dprintk(10, "%s <\n", __func__);

	return res;
}

int micomGetTime(void)
{
	char       buffer[8];
	int        res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	errorOccured   = 0;
	res = micomWriteCommand(0x39, buffer, 7, 1);

	if (res < 0)
	{
		printk("%s < res %d\n", __func__, res);
		return res;
	}

	if (errorOccured)
	{
		memset(ioctl_data, 0, 8);
		printk("error\n");

		res = -ETIMEDOUT;
	}
	else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
	}

	dprintk(10, "%s <\n", __func__);

	return res;
}

/* 0xc1 = rcu
 * 0xc2 = front
 * 0xc3 = time
 * 0xc4 = ac ???
 */
int micomGetWakeUpMode(void)
{
	char       buffer[8];
	int      res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	errorOccured   = 0;
	res = micomWriteCommand(0x43, buffer, 7, 1);

	if (res < 0)
	{
		printk("%s < res %d\n", __func__, res);
		return res;
	}

	if (errorOccured)
	{
		memset(ioctl_data, 0, 8);
		printk("error\n");

		res = -ETIMEDOUT;
	}
	else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
	}

	dprintk(10, "%s <\n", __func__);

	return res;
}

int micomReboot(void)
{
	char       buffer[8];
	int      res = 0;

	dprintk(5, "%s >\n", __func__);

	memset(buffer, 0, 8);

	res = micomWriteCommand(0x46, buffer, 7, 0);

	dprintk(10, "%s <\n", __func__);

	return res;
}


int micomWriteString(unsigned char *aBuf, int len)
{
	unsigned char bBuf[32];
	int i = 0;
	int j = 0;
	int res = 0;

	dprintk(100, "%s >\n", __func__);

//utf8: if (len > 16 || len < 0)
//  {
//      printk("VFD String Length value is over! %d\n", len);
//      len = 16;
//  }

	memset(bBuf, 0, 32);

	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 20);
	lastdata.length = len;

	while ((i < len) && (j < VFD_LENGTH * VFD_CHARSIZE))
	{
		if (aBuf[i] < 0x80)
		{
			bBuf[j] = VFD_CHARTABLE[aBuf[i]] & 0xff;
#if VFD_CHARSIZE > 1
			bBuf[j + 1] = (VFD_CHARTABLE[aBuf[i]] >> 8) & 0xff;
#endif
		}
		else if (aBuf[i] < 0xE0)
		{
			switch (aBuf[i])
			{
				case 0xc2:
					UTF_Char_Table = UTF_C2;
					break;
				case 0xc3:
					UTF_Char_Table = UTF_C3;
					break;
				case 0xc4:
					UTF_Char_Table = UTF_C4;
					break;
				case 0xc5:
					UTF_Char_Table = UTF_C5;
					break;
				case 0xd0:
					UTF_Char_Table = UTF_D0;
					break;
				case 0xd1:
					UTF_Char_Table = UTF_D1;
					break;
				default:
					UTF_Char_Table = NULL;
			}
			i++;
			if (UTF_Char_Table)
			{
				bBuf[j] = UTF_Char_Table[aBuf[i] & 0x3f];
#if VFD_CHARSIZE > 1
				int newVal = 0;
				switch (bBuf[j])
				{
					case 0x84: //0x80,a -umulaut
						newVal = VFD_CHARTABLE[0x80];
						break;
					case 0x94: //0x81,o -umulaut
						newVal = VFD_CHARTABLE[0x81];
						break;
					case 0x81: //0x82,u -umulaut
						newVal = VFD_CHARTABLE[0x82];
						break;
					case 0x8e: //0x83,A -umulaut
						newVal = VFD_CHARTABLE[0x83];
						break;
					case 0x99: //0x84,O -umulaut
						newVal = VFD_CHARTABLE[0x84];
						break;
					case 0x9a: //0x85,U -umulaut
						newVal = VFD_CHARTABLE[0x85];
						break;
					case 0xb1: //0x86,sz- estset
						newVal = VFD_CHARTABLE[0x86];
						break;
				}

				if (newVal)
				{
					bBuf[j] = newVal & 0xff;
					bBuf[j + 1] = (newVal >> 8) & 0xff;
				}
#endif
			}
			else
			{
				sprintf(&bBuf[j], "%02x", aBuf[i - 1]);
				j += 2;
				bBuf[j] = (aBuf[i] & 0x3f) | 0x40;
			}
		}
		else
		{
			if (aBuf[i] < 0xF0)
				i += 2;
			else if (aBuf[i] < 0xF8)
				i += 3;
			else if (aBuf[i] < 0xFC)
				i += 4;
			else
				i += 5;
			bBuf[j] = 0x20;
		}
		i++;
		j = j + VFD_CHARSIZE;
	}

	while ((i < VFD_LENGTH) && (j < VFD_LENGTH * VFD_CHARSIZE))
	{
		bBuf[j] = VFD_CHARTABLE[' '] & 0xff;
#if VFD_CHARSIZE > 1
		bBuf[j + 1] = (VFD_CHARTABLE[' '] >> 8) & 0xff;
#endif
		i++;
		j = j + VFD_CHARSIZE;
	}

	res = micomWriteCommand(0x21, bBuf, VFD_LENGTH * VFD_CHARSIZE, 1);

	dprintk(100, "%s <\n", __func__);

	return res;
}

int micom_init_func(void)
{
#if defined(UFS922) || defined(UFC960)
	int vLoop;
#endif
	dprintk(5, "%s >\n", __func__);

	sema_init(&write_sem, 1);

#if defined(UFS922) || defined(UFC960)
	printk("Kathrein UFS922 VFD/MICOM module initializing\n");
	micomSetModel();
#else
	printk("Kathrein UFS912/913 VFD/MICOM module initializing\n");
	micomInitialize();
#endif
	msleep(10);
	micomSetBrightness(1);

	msleep(10);
	micomSetLedBrightness(0x50);

	msleep(10);
#if VFD_LENGTH < 16
	micomWriteString(" T D T  ", strlen(" T D T  "));
#else
	micomWriteString(" Team Ducktales ", strlen(" Team Ducktales "));
#endif
	msleep(10);

#if defined(UFS922) || defined(UFC960)
	for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++)
		micomSetIcon(vLoop, 0);
#endif
	dprintk(10, "%s <\n", __func__);

	return 0;
}

static ssize_t MICOMdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int minor, vLoop, res = 0;

	dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	if (len == 0)
		return len;

	minor = -1;
	for (vLoop = 0; vLoop < LASTMINOR; vLoop++)
	{
		if (FrontPanelOpen[vLoop].fp == filp)
		{
			minor = vLoop;
		}
	}

	if (minor == -1)
	{
		printk("Error Bad Minor\n");
		return -1; //FIXME
	}

	dprintk(70, "minor = %d\n", minor);

	/* dont write to the remote control */
	if (minor == FRONTPANEL_MINOR_RC)
		return -EOPNOTSUPP;

	kernel_buf = kmalloc(len, GFP_KERNEL);

	if (kernel_buf == NULL)
	{
		printk("%s return no mem<\n", __func__);
		return -ENOMEM;
	}

	copy_from_user(kernel_buf, buff, len);

	if (write_sem_down())
		return -ERESTARTSYS;

	/* Dagobert: echo add a \n which will be counted as a char
	 */
	if (kernel_buf[len - 1] == '\n')
		res = micomWriteString(kernel_buf, len - 1);
	else
		res = micomWriteString(kernel_buf, len);

	kfree(kernel_buf);

	write_sem_up();

	dprintk(100, "%s < res %d len %d\n", __func__, res, len);

	if (res < 0)
		return res;
	else
		return len;
}

static ssize_t MICOMdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;
	dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	minor = -1;
	for (vLoop = 0; vLoop < LASTMINOR; vLoop++)
	{
		if (FrontPanelOpen[vLoop].fp == filp)
		{
			minor = vLoop;
		}
	}

	if (minor == -1)
	{
		printk("Error Bad Minor\n");
		return -EUSERS;
	}

	dprintk(100, "minor = %d\n", minor);

	if (minor == FRONTPANEL_MINOR_RC)
	{
		int           size = 0;
		unsigned char data[20];

		memset(data, 0, 20);

		getRCData(data, &size);

		if (size > 0)
		{
			if (down_interruptible(&FrontPanelOpen[minor].sem))
				return -ERESTARTSYS;

			copy_to_user(buff, data, size);

			up(&FrontPanelOpen[minor].sem);

			dprintk(150, "%s < %d\n", __func__, size);
			return size;
		}

		dumpValues();

		return 0;
	}

	/* copy the current display string to the user */
	if (down_interruptible(&FrontPanelOpen[minor].sem))
	{
		printk("%s return erestartsys<\n", __func__);
		return -ERESTARTSYS;
	}

	if (FrontPanelOpen[minor].read == lastdata.length)
	{
		FrontPanelOpen[minor].read = 0;

		up(&FrontPanelOpen[minor].sem);
		printk("%s return 0<\n", __func__);
		return 0;
	}

	if (len > lastdata.length)
		len = lastdata.length;

	/* fixme: needs revision because of utf8! */
	if (len > 16)
		len = 16;

	FrontPanelOpen[minor].read = len;
	copy_to_user(buff, lastdata.data, len);

	up(&FrontPanelOpen[minor].sem);

	dprintk(100, "%s < (len %d)\n", __func__, len);
	return len;
}

int MICOMdev_open(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(100, "%s >\n", __func__);

	/* needed! otherwise a racecondition can occur */
	if (down_interruptible(&write_sem))
		return -ERESTARTSYS;

	minor = MINOR(inode->i_rdev);

	dprintk(70, "open minor %d\n", minor);

	if (FrontPanelOpen[minor].fp != NULL)
	{
		printk("EUSER\n");
		up(&write_sem);
		return -EUSERS;
	}
	FrontPanelOpen[minor].fp = filp;
	FrontPanelOpen[minor].read = 0;

	up(&write_sem);

	dprintk(100, "%s <\n", __func__);

	return 0;

}

int MICOMdev_close(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(100, "%s >\n", __func__);

	minor = MINOR(inode->i_rdev);

	dprintk(70, "close minor %d\n", minor);

	if (FrontPanelOpen[minor].fp == NULL)
	{
		printk("EUSER\n");
		return -EUSERS;
	}

	FrontPanelOpen[minor].fp = NULL;
	FrontPanelOpen[minor].read = 0;

	dprintk(100, "%s <\n", __func__);
	return 0;
}

static int MICOMdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	static int mode = 0;
	struct micom_ioctl_data *micom = (struct micom_ioctl_data *)arg;
	int res = 0;

	dprintk(100, "%s > 0x%.8x\n", __func__, cmd);

	if (write_sem_down())
		return -ERESTARTSYS;

	switch (cmd)
	{
		case VFDSETMODE:
			mode = micom->u.mode.compat;
			break;
		case VFDSETLED:
			res = micomSetLED(micom->u.led.led_nr, micom->u.led.on);
			break;
		case VFDBRIGHTNESS:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				int level = data->start_address;

				/* scale level from 0 - 7 to a range from 1 - 5 where 5 is off */
				level = 7 - level;

				level = ((level * 100) / 7 * 5) / 100 + 1;

				res = micomSetBrightness(level);
			}
			else
			{
				res = micomSetBrightness(micom->u.brightness.level);
			}
			mode = 0;
			break;
		case VFDLEDBRIGHTNESS:
			if (mode == 0)
			{
			}
			else
			{
				res = micomSetLedBrightness(micom->u.brightness.level);
			}
			mode = 0;
			break;
		case VFDDRIVERINIT:
			res = micom_init_func();
			mode = 0;
			break;
		case VFDICONDISPLAYONOFF:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				int icon_nr = (data->data[0] & 0xf) + 1;
				int on = data->data[4];

				res = micomSetIcon(icon_nr, on);
			}
			else
			{
				res = micomSetIcon(micom->u.icon.icon_nr, micom->u.icon.on);
			}

			mode = 0;
			break;
		case VFDSTANDBY:
			res = micomSetStandby(micom->u.standby.time);
			break;
		case VFDREBOOT:
			res = micomReboot();
			break;
		case VFDSETTIME:
			if (micom->u.time.time != 0)
				res = micomSetTime(micom->u.time.time);
			break;
		case VFDGETTIME:
			res = micomGetTime();
			copy_to_user((void *)arg, &ioctl_data, 8);
			break;
		case VFDGETVERSION:
			res = micomGetVersion();
			copy_to_user((void *)arg, &ioctl_data, 8);
			break;
		case VFDGETWAKEUPMODE:
			res = micomGetWakeUpMode();
			copy_to_user((void *)arg, &ioctl_data, 8);
			break;
		case VFDDISPLAYCHARS:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;

				res = micomWriteString(data->data, data->length);
			}
			else
			{
				//not supported
			}

			mode = 0;

			break;
		case VFDSETRCCODE:
			if (mode == 0)
			{
				struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
				int rc_code = data->data[0];
				if (rc_code > 0 && rc_code < 5)
					res = micomSetRCcode(rc_code);
			}
			else
			{
				//not supported
			}
			mode = 0;
			break;
		case VFDDISPLAYWRITEONOFF:
			/* ->alles abschalten ? VFD_Display_Write_On_Off */
			printk("VFDDISPLAYWRITEONOFF ->not yet implemented\n");
			break;
		case 0x5305:
			break;
		default:
			printk("VFD/MICOM: unknown IOCTL 0x%x\n", cmd);

			mode = 0;
			break;
	}

	write_sem_up();

	dprintk(100, "%s <\n", __func__);
	return res;
}

struct file_operations vfd_fops =
{
	.owner = THIS_MODULE,
	.ioctl = MICOMdev_ioctl,
	.write = MICOMdev_write,
	.read  = MICOMdev_read,
	.open  = MICOMdev_open,
	.release  = MICOMdev_close
};
