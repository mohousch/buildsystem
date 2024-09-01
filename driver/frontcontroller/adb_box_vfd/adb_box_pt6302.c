/* - PT6302 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

#//if defined (CONFIG_KERNELVERSION) /* ST Linux 2.3 */
#include <linux/stm/pio.h>
//#else
//#include <linux/stpio.h>
//#endif

#include "adb_box_pt6302.h"
#include "adb_box_table.h" 	// tablica znakow
#include "adb_box_pt6958.h"


static char adb_box_scp_access_char(struct scp_driver *scp, int dout)
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

static inline void adb_box_scp_write_char(struct scp_driver *scp, char data)
{
	stpio_set_pin(scp->scs, 0);
	adb_box_scp_access_char(scp, data);
	stpio_set_pin(scp->scs, 1);
};

static inline char adb_box_scp_read_char(struct scp_driver *scp)
{
	stpio_set_pin(scp->scs, 0);
	return adb_box_scp_access_char(scp, -1);
	stpio_set_pin(scp->scs, 1);
};

static void adb_box_scp_write_data(struct scp_driver *scp, char *data, int len)
{
	int i;
	stpio_set_pin(scp->scs, 0);
	for (i = 0; i < len; i++) adb_box_scp_access_char(scp, data[i]);
	stpio_set_pin(scp->scs, 1);
};

static int adb_box_scp_read_data(struct scp_driver *scp, char *data, int len)
{
	int i;
	stpio_set_pin(scp->scs, 0);
	for (i = 0; i < len; i++) data[i] = adb_box_scp_access_char(scp, -1);
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


struct pt6302_driver
{
	struct scp_driver *scp;
};

#define pt6302_write_data( scp, data, len ) adb_box_scp_write_data( scp, data, len )
#define pt6302_write_char( scp, data )      adb_box_scp_write_char( scp, data )

static void pt6302_free(struct pt6302_driver *ptd);

static struct pt6302_driver *pt6302_init(struct scp_driver *scp)
{
	struct pt6302_driver *ptd = NULL;

	DBG("pt6302_init( scp = %p )", scp);

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


// zapis na VFD --------------------------------------------------------------------------------------------------------
static int pt6302_write_dcram(struct pt6302_driver *ptd, unsigned char addr, unsigned char *data, unsigned char len)
{
	pt6302_command_t cmd;
	uint8_t         *wdata;

	int              i = 0;
	int              j = 0;
	int		   znak = 0x00;
	char 		   led_txt[16];


	//printk("[fp_vfd]data:[%s], len:[%x]\n", data, len); // wyswietl na ekranie to co przychodzi

	if ((data[0] == 0x6c) && (data[1] == 0x33) && (data[2] == 0x64)) goto led_control;   //skok do kontroli led   ->   l3d X:X

	wdata = kmalloc(1 + len_vfd, GFP_KERNEL);

	if (wdata == NULL)
	{
		ERR("Unable to allocate write buffer of %d bytes.", len_vfd + 1);
		return -ENOMEM;
	}

	memset(wdata, 0x20, len_vfd + 1); // zapisanie bufora 0x20

	cmd.dcram.cmd  = PT6302_COMMAND_DCRAM_WRITE;
	cmd.dcram.addr = (addr & 0xf);
	wdata[0] = cmd.all;

	len = (len <= len_vfd) ? len : len_vfd;

	// czysty LFD
	for (i = 0; i < 16; i++)
	{
		led_txt[i] = " ";
	}

	// pomijanie znakow poza STANDARDOWA tablica
	for (i = 0; i < len; i++)
	{
		if (data[i] < 0x80)
		{
			wdata[len_vfd - j] = pt6302_adb_box_rom_table[data[i]]; 	// wybor z tablicy znakow dla VFD

			j++;
		}
		else
		{
			DBG("[fp_vfd]:[%x]\n", data[i]); // wyswietl na VFD i LFD znaki > 0x7f

			znak = 0x00;

			if ((data[i] == 0xc4) && (data[i + 1] == 0x85)) znak = 0x61;	//0xc4,0x85 ą
			if ((data[i] == 0xc4) && (data[i + 1] == 0x87)) znak = 0x63;	//0xc4,0x87 ć
			if ((data[i] == 0xc4) && (data[i + 1] == 0x99)) znak = 0x65;	//0xc4,0x99 ę
			if ((data[i] == 0xc5) && (data[i + 1] == 0x82)) znak = 0x6c;	//0xc5,0x82 ł
			if ((data[i] == 0xc5) && (data[i + 1] == 0x84)) znak = 0x6e;	//0xc3,0xb3 ń
			if ((data[i] == 0x87) && (data[i + 1] == 0xc5)) znak = 0x73; 	//0x87,0xc5 ś
			if ((data[i] == 0xc3) && (data[i + 1] == 0xb3)) znak = 0x6f;	//0xc3,0xb3 ó
			if ((data[i] == 0xc5) && (data[i + 1] == 0xbc)) znak = 0x7a;	//0xc5,0xbc ż
			if ((data[i] == 0xc5) && (data[i + 1] == 0xba)) znak = 0x7a;	//0xc5,0xba ź

			if (znak != 0x00)
			{
				wdata[len_vfd - j] = pt6302_adb_box_rom_table[znak]; 	// wybor z tablicy znakow
				j++;
			}
		}
	}

	pt6302_write_data(ptd->scp, wdata, len_vfd + 1);

// obsluga LFD
	for (i = 0; i < 16; i++)
	{
		led_txt[i] = wdata[len_vfd - i];
	}

	pt6958_display(led_txt);


	return 0;

led_control:

	DBG("[fp_led]:led:0x%x stan:0x%x", data[4], data[6]);

	if ((data[4] == 0x31) && (data[6] == 0x30)) pt6958_led_control(PT6958_CMD_ADDR_LED1, 0);
	if ((data[4] == 0x31) && (data[6] == 0x31)) pt6958_led_control(PT6958_CMD_ADDR_LED1, 1);
	if ((data[4] == 0x31) && (data[6] == 0x32)) pt6958_led_control(PT6958_CMD_ADDR_LED1, 2);
	if ((data[4] == 0x31) && (data[6] == 0x33)) pt6958_led_control(PT6958_CMD_ADDR_LED1, 3);

	if ((data[4] == 0x32) && (data[6] == 0x30)) pt6958_led_control(PT6958_CMD_ADDR_LED2, 0);
	if ((data[4] == 0x32) && (data[6] == 0x31)) pt6958_led_control(PT6958_CMD_ADDR_LED2, 1);

	if ((data[4] == 0x33) && (data[6] == 0x30)) pt6958_led_control(PT6958_CMD_ADDR_LED3, 0);
	if ((data[4] == 0x33) && (data[6] == 0x31)) pt6958_led_control(PT6958_CMD_ADDR_LED3, 1);

	if ((data[4] == 0x34) && (data[6] == 0x30)) pt6958_led_control(PT6958_CMD_ADDR_LED4, 0);
	if ((data[4] == 0x34) && (data[6] == 0x31)) pt6958_led_control(PT6958_CMD_ADDR_LED4, 1);

	return 0;

}

// ustawienie kontrastu
#define PT6302_DUTY_MIN    02
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

// ustawienie ilosci znakow
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

// ustawienie podswietlenia
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



	// jasnosc didek i lfd
//  pt6958_write(PT6958_CMD_DISPLAY_ON, NULL, 0);

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
	0x20, 0x20, 0x20, 0x20
};

static void pt6302_setup(struct pt6302_driver *pfd)
{
	int i;
	DBG("setup pt6302.");

	pt6302_set_ports(pfd, 1, 0);

	//pt6302_set_digits( pfd, PT6302_DIGITS_MAX );
	pt6302_set_digits(pfd, len_vfd);
	pt6302_set_brightness(pfd, PT6302_DUTY_MIN);
	pt6302_set_lights(pfd, PT6302_LIGHTS_NORMAL);

	DBG("setup pt6302 done.");

	/*  if (rec) {
	*/
	pt6302_write_dcram(pfd, 0x0, "       []       ", 16);   // powitanie
	pt6958_display("-  -");
	for (i = 0; i < 50; i++)
		udelay(2500);
	pt6302_write_dcram(pfd, 0x0, "      [  ]      ", 16);   // powitanie
	for (i = 0; i < 50; i++)
		udelay(2500);
	pt6302_write_dcram(pfd, 0x0, "     [ ** ]     ", 16);   // powitanie
	pt6958_display(" -- ");
	for (i = 0; i < 50; i++)
		udelay(2500);
	pt6302_write_dcram(pfd, 0x0, "    [ *  * ]    ", 16);   // powitanie
	for (i = 0; i < 50; i++)
		udelay(2500);
	pt6302_write_dcram(pfd, 0x0, "   [ *    * ]   ", 16);   // powitanie
	pt6958_display(" bm ");
	for (i = 0; i < 50; i++)
		udelay(1500);
	pt6302_write_dcram(pfd, 0x0, "  [ *  Bm  * ]  ", 16);   // powitanie
	for (i = 0; i < 50; i++)
		udelay(2500);
	pt6302_write_dcram(pfd, 0x0, " [ *  B4am  * ] ", 16);   // powitanie
	pt6958_display("b4tm");
	for (i = 0; i < 50; i++)
		udelay(2500);
	pt6302_write_dcram(pfd, 0x0, "[ *  B4Team  * ]", 16);   // powitanie
	pt6958_display("b4tm");
	for (i = 0; i < 150; i++)
		udelay(4000);

	pt6302_write_dcram(pfd, 0x0, "[ *= B4Team =* ]", 16);   // powitanie
	pt6958_display("05.03");

	/*
		}
		else
		{
		pt6958_display(led_txt);
		}
	*/

	for (i = 0; i < 150; i++)
		udelay(5000);

	pt6302_write_dcram(pfd, 0x0, "[              ]", 16);   // powitanie
	pt6958_display("    ");

//       pt6958_led_control(PT6958_CMD_ADDR_LED1, 2 );
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

