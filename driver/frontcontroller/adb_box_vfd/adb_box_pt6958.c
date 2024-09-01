/* - PT6958 */

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>

//#if defined (CONFIG_KERNELVERSION) /* ST Linux 2.3 */
#include <linux/stm/pio.h>
//#else
//#include <linux/stpio.h>
//#endif

#include "pio.h"
#include "adb_box_pt6958.h"




// PT6958
//------------------------------------------------------------------------------
static void pt6958_start_write(void)
{
	FP_STB_BIT(LOW);
	//stpio_set_pin( button_ds,  0 );
	udelay(FP_TIMING_STBCLK);
	return;
}
//------------------------------------------------------------------------------
static void pt6958_finish_write(void)
{
	udelay(FP_TIMING_STBCLK);
	FP_STB_BIT(HIGH);
	//stpio_set_pin( button_ds,  1 );
	return;
}
//------------------------------------------------------------------------------

static int pt6958_write(unsigned char cmd, unsigned char *data, int data_len)
{
	int i, j;
	unsigned char mask_cmd = 1, mask_data = 1;

#ifdef DEBUG_PT6958
//	printf("[pt6958.write] <- CMD: %02X (%s)\n",cmd,pt6958_dbgCMDtype(cmd));
#endif


	pt6958_start_write();

	for (i = 0; i < 8; i++)
	{
		mask_cmd = cmd & (1 << i);
		FP_CLK_BIT(LOW);

		if (mask_cmd)
			FP_DATA_BIT(HIGH);
		else
			FP_DATA_BIT(LOW);

		udelay(FP_TIMING_PWCLK); // wait tCPW
		FP_CLK_BIT(HIGH);
		udelay(FP_TIMING_PWCLK); // wait tCPW
	}
	udelay(FP_TIMING_PWSTB);
	FP_CLK_BIT(HIGH);

	if ((data == NULL) || (data_len == 0))
	{
		if (cmd == PT6958_CMD_READ_KEY) FP_DATA_BIT(HIGH);  	// Zajebiscie wazne by byl w high !!!
		if (cmd != PT6958_CMD_READ_KEY) pt6958_finish_write();
		//printk("[pt6958.write] <- DATA[%d]: ",data_len);
		return;
	}

	/*
	 * dlugosc danych MAX 10
	 */
	if (data_len > 10)
		data_len = 10;

#ifdef DEBUG_PT6958
//	printf("[pt6958.write] <- DATA[%d]: ",data_len);
#endif

//	FP_STB_BIT(PIN_OFF);
//	udelay(FP_TIMING_STBCLK);

	for (j = 0; j < data_len; j++)
	{
#ifdef DEBUG_PT6958
//		printf("%02X ", data[j]);
#endif
		for (i = 0; i < 8; i++)
		{
			mask_data = data[j] & (1 << i);

			FP_CLK_BIT(LOW);

			if (mask_data)
				FP_DATA_BIT(HIGH);
			else
				FP_DATA_BIT(LOW);

			udelay(FP_TIMING_PWCLK); // wait tCPW
			FP_CLK_BIT(HIGH);
			udelay(FP_TIMING_PWCLK); // wait tCPW
		}
	}
	udelay(FP_TIMING_PWSTB);
	FP_CLK_BIT(HIGH);

	if (cmd != PT6958_CMD_READ_KEY) pt6958_finish_write();

#ifdef DEBUG_PT6958
//	printk("\n");
#endif
	return;
}

//------------------------------------------------------------------------------
/*
 * mapa przyciskow
 * PCB:
 * SW1 = POWER, SW2 = UP, SW3 = DOWN, SW4 = OK
 * SW5 = MENU, SW6 = LEFT, SW8 = RIGHT, SW9 = LIST
 *
 * K1 = POWER [SW 1], UP [SW 5]
 * K2 = MENU [SW 2], LEFT [SW 6]
 * K3 = RIGHT [SW 3], LIST [SW 7]
 * K4 = DOWN [SW 4], OK [SW 8]
 *
 * LSB first
 */

// nasza mapa
/*     SG1       SG2         SG3
K1- POW(SG1),  UP(SG2),    DOWN(SG3)
K2- OK(SG1),   EPG(SG2),   LEFT(SG3)
K3- RES(SG1),  RIGHT(SG2), REC(SG3)
*/

unsigned char pt6958_read_key(void)
{
	int i;
	unsigned char keycode = 0, pin_state = 0;



	for (i = 0; i < 8; i++)
	{
		FP_CLK_BIT(LOW);

		pin_state = FP_DATA;

		if (pin_state)
			keycode |= (1 << i);

		udelay(FP_TIMING_PWCLK); // wait tCPW
		FP_CLK_BIT(HIGH);
		udelay(FP_TIMING_PWCLK); // wait tCPW

	}



#ifdef DEBUG_PT6958
//	printk("[pt6958.read_key] -> KEY = %02X\n",keycode);
#endif
	return keycode;
}
//------------------------------------------------------------------------------
/*
 *  SG9 + GR4 - wykrzyknik
 *  SG9 + GR3 - malpa
 *  SG9 + GR2 - zegar
 *  SG9 + GR1 - power 1
 * SG10 + GR1 - power 2
 * power = 00 (off), 01(red), 02(green), 03(orange)
 *
 * FF = LEAVE AS IS
 * 0 = OFF
 * 1 = ON / power RED
 * 2 = power GREEN
 * 3 = power ORANGE
 */

/*
#define PT6958_CMD_ADDR_LED1	0xC1 //power LED
#define PT6958_CMD_ADDR_LED2	0xC3 //zegar LED
#define PT6958_CMD_ADDR_LED3	0xC5 //malpa LED
#define PT6958_CMD_ADDR_LED4	0xC7 //wykrzyknik LED
*/

static int pt6958_led_control(unsigned char led, unsigned char set)
{
	unsigned char data[1] = {0};

	//printk("[vfd_led] led:[0x%x], set:[0x%x]\n", led, set); // wyswietl na ekranie to co na VFD

	if (set != LED_LEAVE)
	{
		data[0] = set;
		pt6958_write(led, data, 1);
	}
	return;
}
//------------------------------------------------------------------------------

static int pt_6958_set_icon(unsigned char addr, unsigned char *data, unsigned char len)
{

//  printk("[VFD-LED]adres:[%x], data:[%x], len:[%x]\n", addr, data, len); // wyswietl na ekranie to co na VFD


	return 0;
}
//------------------------------------------------------------------------------



/*
 * tablica konwersji CHAR -> CODE
 */
static const pt6958_char_table_t pt6958_char_table_data[] =
{
	// znaki specjalne
	{ ' ', PT6958_LED_EMPTY, },
	{ '-', PT6958_LED_DASH, },
	{ '_', PT6958_LED_UNDERSCORE, },
	{ '~', PT6958_LED_UPPERSCORE, },
	{ '*', PT6958_LED_ASTERISK, },
	// cyfry
	{ '0', PT6958_LED_0, },
	{ '1', PT6958_LED_1, },
	{ '2', PT6958_LED_2, },
	{ '3', PT6958_LED_3, },
	{ '4', PT6958_LED_4, },
	{ '5', PT6958_LED_5, },
	{ '6', PT6958_LED_6, },
	{ '7', PT6958_LED_7, },
	{ '8', PT6958_LED_8, },
	{ '9', PT6958_LED_9, },
	// male litery
	{ 'a', PT6958_LED_A, },
	{ 'b', PT6958_LED_B, },
	{ 'c', PT6958_LED_C, },
	{ 'd', PT6958_LED_D, },
	{ 'e', PT6958_LED_E, },
	{ 'f', PT6958_LED_F, },
	{ 'g', PT6958_LED_G, },
	{ 'h', PT6958_LED_H, },
	{ 'i', PT6958_LED_I, },
	{ 'j', PT6958_LED_J, },
	{ 'k', PT6958_LED_K, },
	{ 'l', PT6958_LED_L, },
	{ 'm', PT6958_LED_M, },
	{ 'n', PT6958_LED_N, },
	{ 'o', PT6958_LED_O, },
	{ 'p', PT6958_LED_P, },
	{ 'q', PT6958_LED_Q, },
	{ 'r', PT6958_LED_R, },
	{ 's', PT6958_LED_S, },
	{ 't', PT6958_LED_T, },
	{ 'u', PT6958_LED_U, },
	{ 'v', PT6958_LED_V, },
	{ 'w', PT6958_LED_W, },
	{ 'x', PT6958_LED_X, },
	{ 'y', PT6958_LED_Y, },
	{ 'z', PT6958_LED_Z, },
	// duze litery
	{ 'A', PT6958_LED_A, },
	{ 'B', PT6958_LED_B, },
	{ 'C', PT6958_LED_C, },
	{ 'D', PT6958_LED_D, },
	{ 'E', PT6958_LED_E, },
	{ 'F', PT6958_LED_F, },
	{ 'G', PT6958_LED_G, },
	{ 'H', PT6958_LED_H, },
	{ 'I', PT6958_LED_I, },
	{ 'J', PT6958_LED_J, },
	{ 'K', PT6958_LED_K, },
	{ 'L', PT6958_LED_L, },
	{ 'M', PT6958_LED_M, },
	{ 'N', PT6958_LED_N, },
	{ 'O', PT6958_LED_O, },
	{ 'P', PT6958_LED_P, },
	{ 'Q', PT6958_LED_Q, },
	{ 'R', PT6958_LED_R, },
	{ 'S', PT6958_LED_S, },
	{ 'T', PT6958_LED_T, },
	{ 'U', PT6958_LED_U, },
	{ 'V', PT6958_LED_V, },
	{ 'W', PT6958_LED_W, },
	{ 'X', PT6958_LED_X, },
	{ 'Y', PT6958_LED_Y, },
	{ 'Z', PT6958_LED_Z, },
	// pozostale
	{ '!', PT6958_LED_EMPTY, },
	{ '"', PT6958_LED_EMPTY, },
	{ '$', PT6958_LED_EMPTY, },
	{ '%', PT6958_LED_EMPTY, },
	{ '&', PT6958_LED_EMPTY, },
//	{ ''', PT6958_LED_EMPTY, },
	{ '(', PT6958_LED_EMPTY, },
	{ ')', PT6958_LED_EMPTY, },
	{ '+', PT6958_LED_EMPTY, },
	{ ',', PT6958_LED_DOT, },
	{ '.', PT6958_LED_DOT, },
	{ '/', PT6958_LED_EMPTY, },
	{ ':', PT6958_LED_DOT, },
	{ ';', PT6958_LED_EMPTY, },
	{ '<', PT6958_LED_EMPTY, },
	{ '=', PT6958_LED_EMPTY, },
	{ '>', PT6958_LED_EMPTY, },
	{ '?', PT6958_LED_EMPTY, },
	{ '[', PT6958_LED_EMPTY, },
	{ ']', PT6958_LED_EMPTY, },
	{ '^', PT6958_LED_EMPTY, },
	{ '{', PT6958_LED_EMPTY, },
	{ '|', PT6958_LED_EMPTY, },
	{ '}', PT6958_LED_EMPTY, },

};
//------------------------------------------------------------------------------
/*
 * szuka w tablicy znaku CHAR i zamienia go na KOD
 */
static unsigned char pt6958_translate_lookup(char in)
{
	int i;
	unsigned char out;
	pt6958_char_table_t *table = (pt6958_char_table_t *)pt6958_char_table_data;

	for (i = 0; i < PT6958_TABLE_LEN; i++)
	{
		if (table->znak == in)
		{
			out = table->kod;
			break;
		}
		table++;
	}
	if (i == PT6958_TABLE_LEN) out = PT6958_LED_UNDERSCORE; //nieznany znak wyswietl jako _
	return out;
}
//------------------------------------------------------------------------------
/*
 * funkcja ktora tlumaczy znaki CHAR na KODY znakow
 */

static void pt6958_translate(char *string, unsigned char *out)
{
	int i, j = 0;
	unsigned char data;

	for (i = 0; i < PT6958_MAX_CHARS + PT6958_MAX_CHARS; i++)
		out[i] = pt6958_translate_lookup(string[i]);

	// konwersja kropki
	for (i = 0; i < PT6958_MAX_CHARS + PT6958_MAX_CHARS; i++)
	{
		data = out[i];

		if (data == PT6958_LED_DOT)
		{
			if (j > 0)
			{
				out[j - 1] = (out[i - 1] | 0x80);	//dodanie DO do poprzedniego znaku
				j--;				//cofniecie pozycji
			}
			else
			{
				out[j] = (out[i] | 0x80);		//dodanie jako poprzedniego znaku
			}
		}
		else
		{
			out[j] = data;
		}
		j++;

	}

	return;
}



//------------------------------------------------------------------------------
/*
 * pokazuje 4 znaki na wyswietlaczu LED
 */
void pt6958_display(char *str)
{
	unsigned char strcodes[PT6958_MAX_CHARS] = {0};
	unsigned char data[1] = {0};

//	printk("[pt6958.display] Text: %s\n",str);

	pt6958_translate(str, strcodes);

	data[0] = strcodes[0];
	pt6958_write(FP_DISP1, data, 1);

	data[0] = strcodes[1];
	pt6958_write(FP_DISP2, data, 1);

	data[0] = strcodes[2];
	pt6958_write(FP_DISP3, data, 1);

	data[0] = strcodes[3];
	pt6958_write(FP_DISP4, data, 1);

	return;
}
