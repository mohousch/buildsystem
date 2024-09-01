/* Based on work of various authors I don't recall right now. */
/* Konfetti is among them for sure                            */
/*                                                            */
/* PT6302 vfd driver for Homecast 5101                        */
/* - Still a work in progress                                 */
/* - Needs cleanup!                                           */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#include <linux/stpio.h>

#include "vfd.h"

static int debug  = 0;
static int delay  = 5;

#define DBG(fmt, args...) if ( debug ) printk(KERN_INFO "[vfd] :: " fmt "\n", ## args )
#define ERR(fmt, args...) printk(KERN_ERR "[vfd] :: " fmt "\n", ## args )

//
// hs5101 scp
//

#define HS5101_VFD_MAX_CHARS 12

#define HS5101_VFD_PIO_PORT_SCS 1
#define HS5101_VFD_PIO_PIN_SCS  0
#define HS5101_VFD_PIO_PORT_SCL 1
#define HS5101_VFD_PIO_PIN_SCL  1
#define HS5101_VFD_PIO_PORT_SDA 1
#define HS5101_VFD_PIO_PIN_SDA  2

struct scp_driver
{
	struct stpio_pin *scs;
	struct stpio_pin *scl;
	struct stpio_pin *sda;
};

static void hs5101_scp_free(struct scp_driver *scp)
{
	if (scp == NULL) return;
	if (scp->scs) stpio_set_pin(scp->scs, 1);

	if (scp->sda) stpio_free_pin(scp->sda);
	if (scp->scl) stpio_free_pin(scp->scl);
	if (scp->scs) stpio_free_pin(scp->scs);

	if (scp) kfree(scp);
	scp = NULL;

	//  DBG("removed hs5101 scp driver.");
};

static const char stpio_vfd_scs[] = "vfd_scs";
static const char stpio_vfd_scl[] = "vfd_sck";
static const char stpio_vfd_sda[] = "vfd_sda";

static struct scp_driver *hs5101_scp_init(void)
{
	struct scp_driver *scp = NULL;

	//  DBG("init hs5101 scp driver.");

	scp = (struct scp_driver *)kzalloc(sizeof(struct scp_driver), GFP_KERNEL);
	if (scp == NULL)
	{
		ERR("Unable to allocate scp driver struct. abort.");
		goto hs5101_scp_init_fail;
	}

	//  DBG("request stpio %d,%d,%s,%d", HS5101_VFD_PIO_PORT_SCS, HS5101_VFD_PIO_PIN_SCS, stpio_vfd_scs, STPIO_OUT);
	scp->scs = stpio_request_pin(HS5101_VFD_PIO_PORT_SCS, HS5101_VFD_PIO_PIN_SCS, stpio_vfd_scs, STPIO_OUT);

	if (scp->scs == NULL)
	{
		ERR("Request stpio scs failed. abort.");
		goto hs5101_scp_init_fail;
	}

	//  DBG("request stpio %d,%d,%s,%d", HS5101_VFD_PIO_PORT_SCL, HS5101_VFD_PIO_PIN_SCL, stpio_vfd_scl, STPIO_OUT);
	scp->scl = stpio_request_pin(HS5101_VFD_PIO_PORT_SCL, HS5101_VFD_PIO_PIN_SCL, stpio_vfd_scl, STPIO_OUT);

	if (scp->scl == NULL)
	{
		ERR("Request stpio scl failed. abort.");
		goto hs5101_scp_init_fail;
	}

	//  DBG("request stpio %d,%d,%s,%d", HS5101_VFD_PIO_PORT_SDA, HS5101_VFD_PIO_PIN_SDA, stpio_vfd_sda, STPIO_OUT );
	scp->sda = stpio_request_pin(HS5101_VFD_PIO_PORT_SDA, HS5101_VFD_PIO_PIN_SDA, stpio_vfd_sda, STPIO_OUT);

	if (scp->sda == NULL)
	{
		ERR("Request stpio sda failed. abort.");
		goto hs5101_scp_init_fail;
	}

	return scp;

hs5101_scp_init_fail:
	hs5101_scp_free(scp);

	return 0;
};

static char hs5101_scp_access_char(struct scp_driver *scp, int dout)
{
	uint8_t din   = 0;
	int     outen = (dout < 0) ? 0 : 1, i;

	for (i = 0; i < 8; i++)
	{
		if (outen) stpio_set_pin(scp->sda, (dout & 1) == 1);
		stpio_set_pin(scp->scl, 0);
		udelay(delay);
		stpio_set_pin(scp->scl, 1);
		udelay(delay);
		din  = (din >> 1) | (stpio_get_pin(scp->sda) > 0 ? 0x80 : 0);
		dout = dout >> 1;
	}
	return din;
};

static inline void hs5101_scp_write_char(struct scp_driver *scp, char data)
{
	stpio_set_pin(scp->scs, 0);
	hs5101_scp_access_char(scp, data);
	stpio_set_pin(scp->scs, 1);
};

static inline char hs5101_scp_read_char(struct scp_driver *scp)
{
	stpio_set_pin(scp->scs, 0);
	return hs5101_scp_access_char(scp, -1);
	stpio_set_pin(scp->scs, 1);
};

static void hs5101_scp_write_data(struct scp_driver *scp, char *data, int len)
{
	int i;
	stpio_set_pin(scp->scs, 0);
	for (i = 0; i < len; i++) hs5101_scp_access_char(scp, data[i]);
	stpio_set_pin(scp->scs, 1);
};

static int hs5101_scp_read_data(struct scp_driver *scp, char *data, int len)
{
	int i;
	stpio_set_pin(scp->scs, 0);
	for (i = 0; i < len; i++) data[i] = hs5101_scp_access_char(scp, -1);
	stpio_set_pin(scp->scs, 1);
	return len;
};

//
// pt6302
//

typedef union
{
	struct
	{
		uint8_t addr: 4, cmd: 4;
	} dcram;
	struct
	{
		uint8_t addr: 3, reserved: 1, cmd: 4;
	} cgram;
	struct
	{
		uint8_t addr: 4, cmd: 4;
	} adram;
	struct
	{
		uint8_t port1: 1, port2: 1, reserved: 2, cmd: 4;
	} port;
	struct
	{
		uint8_t duty: 3, reserved: 1, cmd: 4;
	} duty;
	struct
	{
		uint8_t digits: 3, reserved: 1, cmd: 4;
	} digits;
	struct
	{
		uint8_t onoff: 2, reserved: 2, cmd: 4;
	} lights;
	uint8_t all;
} pt6302_command_t;

#define PT6302_COMMAND_DCRAM_WRITE 1
#define PT6302_COMMAND_CGRAM_WRITE 2
#define PT6302_COMMAND_ADRAM_WRITE 3
#define PT6302_COMMAND_SET_PORTS   4
#define PT6302_COMMAND_SET_DUTY    5
#define PT6302_COMMAND_SET_DIGITS  6
#define PT6302_COMMAND_SET_LIGHTS  7
#define PT6302_COMMAND_TESTMODE    8

static const uint8_t pt6302_007_rom_table[256] =
{
	0x2e,//0x00, RAM0
	0x8f,//0x01, RAM1
	0xe4,//0x02, RAM2
	0xdd,//0x03, RAM3
	0xdc,//0x04, RAM4
	0x10,//0x05, RAM5
	0x10,//0x06, RAM6
	0x10,//0x07, RAM7
	0x10,//0x08,
	0x10,//0x09,
	0x10,//0x0a,
	0x10,//0x0b,
	0x10,//0x0c,
	0x10,//0x0d,
	0x10,//0x0e,
	0x10,//0x0f,

	0x10,//0x10, reserved
	0x11,//0x11, reserved
	0x12,//0x12, reserved
	0x13,//0x13, reserved
	0x14,//0x14, reserved
	0x15,//0x15, reserved
	0x16,//0x16, reserved
	0x17,//0x17, reserved
	0x18,//0x18, reserved
	0x19,//0x19, reserved
	0x1a,//0x1a, reserved
	0x1b,//0x1b, reserved
	0x1c,//0x1c, reserved
	0x10,//0x1d, reserved
	0x10,//0x1e, reserved
	0x10,//0x1f, reserved

	0x20,//0x20, <space>
	0x21,//0x21, !
	0x22,//0x22, "
	0x23,//0x23, #
	0x24,//0x24, $
	0x25,//0x25, %
	0x26,//0x26, &
	0x27,//0x27, '
	0x28,//0x28, (
	0x29,//0x29, )
	0x2a,//0x2a, *
	0x2b,//0x2b, +
	0x2c,//0x2c, ,
	0x2d,//0x2d, -
	0x2e,//0x2e, .
	0x2f,//0x2f, /

	0x30,//0x30, 0
	0x31,//0x31, 1
	0x32,//0x32, 2
	0x33,//0x33, 3
	0x34,//0x34, 4
	0x35,//0x35, 5
	0x36,//0x36, 6
	0x37,//0x37, 7
	0x38,//0x38, 8
	0x39,//0x39, 9
	0x3a,//0x3a, :
	0x3b,//0x3b, ;
	0x3c,//0x3c, <
	0x3d,//0x3d, =
	0x3e,//0x3e, >
	0x3f,//0x3f, ?

	0x40,//0x40, @
	0x41,//0x41, A
	0x42,//0x42, B
	0x43,//0x43, C
	0x44,//0x44, D
	0x45,//0x45, E
	0x46,//0x46, F
	0x47,//0x47, G
	0x48,//0x48, H
	0x49,//0x49, I
	0x4a,//0x4a, J
	0x4b,//0x4b, K
	0x4c,//0x4c, L
	0x4d,//0x4d, M
	0x4e,//0x4e, N
	0x4f,//0x4f, O

	0x50,//0x50, P
	0x51,//0x51, Q
	0x52,//0x52, R
	0x53,//0x53, S
	0x54,//0x54, T
	0x55,//0x55, U
	0x56,//0x56, V
	0x57,//0x57, W
	0x58,//0x58, X
	0x59,//0x59, Y
	0x5a,//0x5a, Z
	0x5b,//0x5b, [
	0x5c,//0x5c, <BACKSLASH>
	0x5d,//0x5d, ]
	0x5e,//0x5e, ^
	0x5f,//0x5f, _

	0x60,//0x60, `
	0x61,//0x61, a
	0x62,//0x62, b
	0x63,//0x63, c
	0x64,//0x64, d
	0x65,//0x65, e
	0x66,//0x66, f
	0x67,//0x67, g
	0x68,//0x68, h
	0x69,//0x69, i
	0x6a,//0x6a, j
	0x6b,//0x6b, k
	0x6c,//0x6c, l
	0x6d,//0x6d, m
	0x6e,//0x6e, n
	0x6f,//0x6f, o

	0x70,//0x70, p
	0x71,//0x71, q
	0x72,//0x72, r
	0x73,//0x73, s
	0x74,//0x74, t
	0x75,//0x75, u
	0x76,//0x76, v
	0x77,//0x77, w
	0x78,//0x78, x
	0x79,//0x79, y
	0x7a,//0x7a, z
	0x7b,//0x7b, {
	0x7c,//0x7c, |
	0x7d,//0x7d, }
	0x7e,//0x7e, ~
	0x7f,//0x7f, <DEL>

	0x84,//0x80, adiaeresis
	0x94,//0x81, odiaeresis
	0x81,//0x82, udiaeresis
	0x8e,//0x83, Adiaeresis
	0x99,//0x84, Odiaeresis
	0x9a,//0x85, Udiaeresis
	0xb1,//0x86, ssharp
	0x10,//0x87, reserved
	0x10,//0x88, reserved
	0x10,//0x89, reserved
	0x10,//0x8a, reserved
	0x10,//0x8b, reserved
	0x10,//0x8c, reserved
	0x10,//0x8d, reserved
	0x10,//0x8e, reserved
	0x10,//0x8f, reserved

	0x10,//0x90, reserved
	0x10,//0x91, reserved
	0x10,//0x92, reserved
	0x10,//0x93, reserved
	0x10,//0x94, reserved
	0x10,//0x95, reserved
	0x10,//0x96, reserved
	0x10,//0x97, reserved
	0x10,//0x98, reserved
	0x10,//0x99, reserved
	0x10,//0x9a, reserved
	0x10,//0x9b, reserved
	0x10,//0x9c, reserved
	0x10,//0x9d, reserved
	0x10,//0x9e, reserved

	0x10,//0xa0, reserved
	0x10,//0xa1, reserved
	0x10,//0xa2, reserved
	0x10,//0xa3, reserved
	0x10,//0xa4, reserved
	0x10,//0xa5, reserved
	0x10,//0xa6, reserved
	0x10,//0xa7, reserved
	0x10,//0xa8, reserved
	0x10,//0xa9, reserved
	0x10,//0xaa, reserved
	0x10,//0xab, reserved
	0x10,//0xac, reserved
	0x10,//0xad, reserved
	0x10,//0xae, reserved
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
	0x10,//0xbc, reserved
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
	0x10,//0xdc, reserved
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
	0x10,//0xec, reserved
	0x10,//0xed, reserved
	0x10,//0xee, reserved
	0x10,//0xef, reserved

	0x10,//0xf0, reserved
	0x10,//0xf1, reserved
	0x10,//0xf2, reserved
	0x10,//0xf3, reserved
	0x10,//0xf4, reserved
	0x10,//0xf5, reserved
	0x10,//0xf6, reserved
	0x10,//0xf7, reserved
	0x10,//0xf8, reserved
	0x10,//0xf9, reserved
	0x10,//0xfa, reserved
	0x10,//0xfb, reserved
	0x10,//0xfc, reserved
	0x10,//0xfd, reserved
	0x10,//0xfe, reserved
	0x00 //0xff, reserved
};

struct pt6302_driver
{
	struct scp_driver *scp;
};

#define pt6302_write_data( scp, data, len ) hs5101_scp_write_data( scp, data, len )
#define pt6302_write_char( scp, data )      hs5101_scp_write_char( scp, data )

static void pt6302_free(struct pt6302_driver *ptd);

static struct pt6302_driver *pt6302_init(struct scp_driver *scp)
{
	struct pt6302_driver *ptd = NULL;

	//  DBG("pt6302_init( scp = %p )", scp );

	if (scp == NULL)
	{
		ERR("Failed to access NULL scp driver. abort.");
		return NULL;
	}

	ptd = (struct pt6302_driver *)kzalloc(sizeof(struct pt6302_driver), GFP_KERNEL);
	if (ptd == NULL)
	{
		ERR("Unable to allocate pt6302 driver struct. abort.");
		goto pt6302_init_fail;
	}

	ptd->scp = scp;

	return ptd;

pt6302_init_fail:
	pt6302_free(ptd);
	return NULL;
}

static int pt6302_write_dcram(struct pt6302_driver *ptd, unsigned char addr, unsigned char *data, unsigned char len)
{
	pt6302_command_t cmd;
	uint8_t         *wdata;
	int              i = 0;

	wdata = kmalloc(1 + len, GFP_KERNEL);
	if (wdata == NULL)
	{
		ERR("Unable to allocate write buffer of %d bytes.", len + 1);
		return -ENOMEM;
	}

	cmd.dcram.cmd  = PT6302_COMMAND_DCRAM_WRITE;
	cmd.dcram.addr = (addr & 0xf);

	wdata[0] = cmd.all;
	for (i = 0; i < len; i++)
	{
		wdata[i + 1] = pt6302_007_rom_table[data[i]];
	}

	pt6302_write_data(ptd->scp, wdata, len + 1);

	return 0;
}

#define PT6302_DUTY_MIN    0
#define PT6302_DUTY_MAX    7

static void pt6302_set_brightness(struct pt6302_driver *ptd, int level)
{
	pt6302_command_t cmd;

	if (level < PT6302_DUTY_MIN) level = PT6302_DUTY_MIN;
	if (level > PT6302_DUTY_MAX) level = PT6302_DUTY_MAX;

	cmd.duty.cmd  = PT6302_COMMAND_SET_DUTY;
	cmd.duty.duty = level;

	pt6302_write_char(ptd->scp, cmd.all);
}

#define PT6302_DIGITS_MIN    9
#define PT6302_DIGITS_MAX    16
#define PT6302_DIGITS_OFFSET 8

static void pt6302_set_digits(struct pt6302_driver *ptd, int num)
{
	pt6302_command_t cmd;

	if (num < PT6302_DIGITS_MIN) num = PT6302_DIGITS_MIN;
	if (num > PT6302_DIGITS_MAX) num = PT6302_DIGITS_MAX;

	num = (num == PT6302_DIGITS_MAX) ? 0 : (num - PT6302_DIGITS_OFFSET);

	cmd.digits.cmd    = PT6302_COMMAND_SET_DIGITS;
	cmd.digits.digits = num;

	pt6302_write_char(ptd->scp, cmd.all);
}

#define PT6302_LIGHTS_NORMAL 0
#define PT6302_LIGHTS_OFF    1
#define PT6302_LIGHTS_ON     3

static void pt6302_set_lights(struct pt6302_driver *ptd, int onoff)
{
	pt6302_command_t cmd;

	if (onoff < PT6302_LIGHTS_NORMAL || onoff > PT6302_LIGHTS_ON)
		onoff = PT6302_LIGHTS_ON;

	cmd.lights.cmd   = PT6302_COMMAND_SET_LIGHTS;
	cmd.lights.onoff = onoff;

	pt6302_write_char(ptd->scp, cmd.all);
}

static void pt6302_set_ports(struct pt6302_driver *ptd, int port1, int port2)
{
	pt6302_command_t cmd;

	cmd.port.cmd   = PT6302_COMMAND_SET_PORTS;
	cmd.port.port1 = (port1) ? 1 : 0;
	cmd.port.port2 = (port2) ? 1 : 0;

	pt6302_write_char(ptd->scp, cmd.all);
}

static uint8_t PT6302_CLEAR_DATA[] =
{
	0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20,
	0x00, 0x00, 0x00, 0x00
};

static void pt6302_setup(struct pt6302_driver *pfd)
{
	int i;
	//  DBG("setup pt6302.");

	pt6302_set_ports(pfd, 1, 0);

	pt6302_set_digits(pfd, PT6302_DIGITS_MAX);
	pt6302_set_brightness(pfd, PT6302_DUTY_MIN);

	pt6302_set_lights(pfd, PT6302_LIGHTS_NORMAL);

	//  DBG("setup pt6302 done.");

	pt6302_write_dcram(pfd, 0x0, ">|PT-6302|<     ", 16);
	for (i = 0; i < 100; i++)
		udelay(10000);
	pt6302_write_dcram(pfd, 0x0, PT6302_CLEAR_DATA, 16);
}

static void pt6302_free(struct pt6302_driver *ptd)
{
	if (ptd == NULL) return;

	pt6302_set_lights(ptd, PT6302_LIGHTS_OFF);
	pt6302_set_brightness(ptd, PT6302_DUTY_MAX);
	pt6302_set_digits(ptd, PT6302_DIGITS_MIN);
	pt6302_set_ports(ptd, 0, 0);

	ptd->scp = NULL;
}

//
// VFD
//

struct vfd_driver
{
	struct scp_driver    *scp;
	struct pt6302_driver *ctrl;
	struct semaphore      sem;
	int                   opencount;
};

static struct vfd_driver vfd;

//
//

#define VFDIOC_DCRAMWRITE		0xc0425a00
#define VFDIOC_BRIGHTNESS		0xc0425a03
#define VFDIOC_DISPLAYWRITEONOFF	0xc0425a05
#define VFDIOC_DRIVERINIT		0xc0425a08
#define VFDIOC_ICONDISPLAYONOFF		0xc0425a0a

static int vfd_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct vfd_ioctl_data vfddata;

	switch (cmd)
	{
		case VFDIOC_DCRAMWRITE:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			return pt6302_write_dcram(vfd.ctrl, vfddata.address, vfddata.data, vfddata.length);
			break;
		case VFDIOC_BRIGHTNESS:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			pt6302_set_brightness(vfd.ctrl, vfddata.address);
			break;
		case VFDIOC_DISPLAYWRITEONOFF:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			pt6302_set_lights(vfd.ctrl, vfddata.address);
			break;
		case VFDIOC_ICONDISPLAYONOFF:
			copy_from_user(&vfddata, (void *)arg, sizeof(struct vfd_ioctl_data));
			//    vfd_set_icon( data );
			break;
		case VFDIOC_DRIVERINIT:
			pt6302_setup(vfd.ctrl);
			break;
		default:
			ERR("[vfd] unknown ioctl %08x", cmd);
			break;
	}
	return 0;
}

static ssize_t vfd_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	unsigned char *kbuf;
	size_t         wlen;

	//  DBG("write : len = %d (%d), off = %d.\n", len, HS5101_VFD_MAX_CHARS, (int)*off);

	if (len == 0) return len;

	kbuf = kmalloc(len, GFP_KERNEL);
	if (kbuf == NULL)
	{
		ERR("Unable to allocate kernel write buffer.");
		return -ENOMEM;
	}

	copy_from_user(kbuf, buf, len);

	wlen = len;
	if (kbuf[len - 1] == '\n')
	{
		kbuf[len - 1] = '\0';
		wlen--;
	}
	if (wlen > HS5101_VFD_MAX_CHARS) wlen = HS5101_VFD_MAX_CHARS;

	//  DBG("write : len = %d, wlen = %d, kbuf = '%s'.\n", len, wlen, kbuf);

	pt6302_write_dcram(vfd.ctrl, 0, kbuf, wlen);

#if 0
	if (wlen <= HS5101_VFD_MAX_CHARS)
	{
		pt6302_write_dcram(vfd.ctrl, 0, kbuf, wlen);
	}
	else
	{
		pos = 0;
		pt6302_write_dcram(vfd.ctrl, 0, kbuf + pos, HS5101_VFD_MAX_CHARS);
	}
#endif

	return len;
}

static ssize_t vfd_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	return len;
}

static int vfd_open(struct inode *inode, struct file *file)
{
	if (down_interruptible(&(vfd.sem)))
	{
		//    DBG("interrupted while waiting for sema.");
		return -ERESTARTSYS;
	}

	if (vfd.opencount > 0)
	{
		//    DBG("device already opened.");
		up(&(vfd.sem));
		return -EUSERS;
	}

	vfd.opencount++;
	up(&(vfd.sem));

	pt6302_set_lights(vfd.ctrl, PT6302_LIGHTS_ON);
	udelay(1000);
	pt6302_set_lights(vfd.ctrl, PT6302_LIGHTS_NORMAL);
	return 0;
}

static int vfd_close(struct inode *inode, struct file *file)
{
	//  pt6302_set_lights( vfd.ctrl, PT6302_LIGHTS_OFF );
	vfd.opencount = 0;
	return 0;
}

//
//

static struct file_operations vfd_fops =
{
	.owner   = THIS_MODULE,
	.ioctl   = vfd_ioctl,
	.write   = vfd_write,
	.read    = vfd_read,
	.open    = vfd_open,
	.release = vfd_close
};

static void __exit vfd_module_exit(void)
{
	unregister_chrdev(VFD_MAJOR, "vfd");
	if (vfd.ctrl) pt6302_free(vfd.ctrl);
	if (vfd.scp) hs5101_scp_free(vfd.scp);
	vfd.ctrl = NULL;
	vfd.scp  = NULL;
}

static int __init vfd_module_init(void)
{
	//  DBG("Homecast 5101 vfd driver init.");

	vfd.scp  = NULL;
	vfd.ctrl = NULL;

	//  DBG("probe for scp driver.");
	vfd.scp = hs5101_scp_init();
	if (vfd.scp == NULL)
	{
		ERR("unable to init scp driver. abort.");
		goto vfd_init_fail;
	}

	//  DBG("probe for ctrl driver.");
	vfd.ctrl = pt6302_init(vfd.scp);
	if (vfd.ctrl == NULL)
	{
		ERR("unable to init ctrl driver. abort.");
		goto vfd_init_fail;
	}

	//  DBG("register character device %d.", VFD_MAJOR );
	if (register_chrdev(VFD_MAJOR, "vfd", &vfd_fops))
	{
		ERR("register major %d failed", VFD_MAJOR);
		goto vfd_init_fail;
	}

	sema_init(&(vfd.sem), 1);
	vfd.opencount = 0;

	pt6302_setup(vfd.ctrl);

	return 0;

vfd_init_fail:
	vfd_module_exit();
	return -EIO;
}

module_init(vfd_module_init);
module_exit(vfd_module_exit);

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "debug");

module_param(delay, int, 0644);
MODULE_PARM_DESC(delay, "scp delay");

MODULE_DESCRIPTION("pt6302 vfd driver");
MODULE_AUTHOR("corev");
MODULE_LICENSE("GPL");
