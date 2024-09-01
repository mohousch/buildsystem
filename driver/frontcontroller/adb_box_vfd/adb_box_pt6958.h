#ifndef _PT6958_H_
#define _PT6958_H_

#define ADB_BOX_BUTTON_UP    0x01
#define ADB_BOX_BUTTON_DOWN  0x02
#define ADB_BOX_BUTTON_LEFT  0x04
#define ADB_BOX_BUTTON_RIGHT 0x08
#define ADB_BOX_BUTTON_POWER 0x10
#define ADB_BOX_BUTTON_MENU  0x20
#define ADB_BOX_BUTTON_EXIT  0x40

#define BUTTON_DO_PORT  2
#define BUTTON_DO_PIN   2

#define BUTTON_DS_PORT  1
#define BUTTON_DS_PIN   6


//#define BUTTON_RESET_PORT  3
//#define BUTTON_RESET_PIN   2


static char                    *button_driver_name = "pt6958 frontpanel buttons";
static struct workqueue_struct *button_wq;
static struct input_dev        *button_dev;
static struct stpio_pin        *button_do = NULL;
static struct stpio_pin        *button_ds = NULL;
static struct stpio_pin        *button_reset = NULL;

static int                      button_polling = 1;
unsigned char 			key_group1 = 0, key_group2 = 0;
static unsigned int  		key_button = 0;

/*
void ReadKey(void);
void WriteData(unsigned char Value);
unsigned char ReadData(void);
*/

/*
 * zmienne i definicje przeniesione z uboot
*/


//------------------------------------------------------------------------------
#define LOW				0
#define HIGH				1
//------------------------------------------------------------------------------
#define FP_CLK_SETUP			SET_PIO_PIN(PIO_PORT(4),0, STPIO_OUT); //BIDIR?
#define FP_DATA_SETUP			SET_PIO_PIN(PIO_PORT(4),1, STPIO_BIDIR);
#define FP_STB_SETUP			SET_PIO_PIN(PIO_PORT(1),6, STPIO_OUT);
#define FP_CSB_SETUP			SET_PIO_PIN(PIO_PORT(1),2, STPIO_OUT);
#define FP_KEY_SETUP			SET_PIO_PIN(PIO_PORT(2),2, STPIO_IN);
//------------------------------------------------------------------------------
#define FP_CLK_BIT(x)			STPIO_SET_PIN(PIO_PORT(4),0, x)
#define FP_DATA_BIT(x)			STPIO_SET_PIN(PIO_PORT(4),1, x)
#define FP_STB_BIT(x)			STPIO_SET_PIN(PIO_PORT(1),6, x)
#define FP_CSB_BIT(x)			STPIO_SET_PIN(PIO_PORT(1),2, x)
//------------------------------------------------------------------------------
#define FP_GETKEY			STPIO_GET_PIN(PIO_PORT(2),2) //4 ?
#define FP_DATA				STPIO_GET_PIN(PIO_PORT(4),1)
//------------------------------------------------------------------------------

#define FP_TIMING_PWCLK			1 			// clock pulse with is 1us 
#define FP_TIMING_STBCLK		1 		// clock strobe time 2us
#define FP_TIMING_CSBCLK		FP_TIMING_STBCLK 	// same as STBCLK
#define FP_TIMING_CLKSTB		1 			// strobe clock time 2us
#define FP_TIMING_PWSTB			1	 		// strobe pulse width 2us
#define FP_TIMING_TWAIT			1 			// waiting time 2us between ReadCMD & ReadDATA (KEY)
#define FP_TIMING_TDOFF			1			// 300ns delay between data bytes
//------------------------------------------------------------------------------

//extern void adb_box_fp_init(void);


//------------------------------------------------------------------------------
typedef struct
{
	char znak; 					//znak oryginalny
	unsigned char kod; 			//kod znaku
} pt6958_char_table_t;
//------------------------------------------------------------------------------
#define PT6958_TABLE_LEN 		(sizeof(pt6958_char_table_data)/sizeof(pt6958_char_table_t))
#define PT6958_MAX_CHARS		4
/**********************************************************************
 * Internal RAM Size : 14 bytes
 *
 *  1 - Data Setting Command
 *  1 - Address Setting Command
 * 10 - Display Data
 *  1 - Display Control Command
 **********************************************************************/
#define PT6958_RAM_SIZE 		(1+1+10+1)
/**********************************************************************
 * CMD 40: Data Setting Command
 * bits:   0100 dcba
 *
 * ba - Data Write & Read Mode Settings:
 * 00: Write Data to Display Mode
 * 10: Read Key Data
 *
 *  c - Address Incremental Mode Settings (Display Mode):
 *  0: Increment Address after Data has been Written
 *  1: Fixed Address
 *
 *  d - Mode Settings
 *  0: Normal Operation Mode
 *  1: Test Mode
 **********************************************************************/
#define PT6958_CMD_WRITE_INC	0x40
#define PT6958_CMD_READ_KEY	0x42
#define PT6958_CMD_WRITE_FIXED	0x44
#define PT6958_CMD_MODE_NORMAL	0x40
#define PT6958_CMD_MODE_TEST	0x48
/**********************************************************************
 * CMD C0: Address Setting Command
 * bits:   1100 dcba
 *
 * 0,2,4,6 = segment 1,2,3,4
 * 1,3,5,7 = power, clock, mail, warning LED
 * 8,9     = not connected
 *
 * dcba - Address 0x00 to 0x09
 **********************************************************************/
#define PT6958_CMD_ADDR_SET	0xC0 // 0xC0 to 0xC9
//------------------------------------------------------------------------------
#define PT6958_CMD_ADDR_DISP1	0xC0 // 8.:.. 
#define PT6958_CMD_ADDR_DISP2	0xC2 // .8:..  
#define PT6958_CMD_ADDR_DISP3	0xC4 // ..:8. 
#define PT6958_CMD_ADDR_DISP4	0xC6 // ..:.8
//------------------------------------------------------------------------------
#define PT6958_CMD_ADDR_LED1	0xC1 //power LED
#define PT6958_CMD_ADDR_LED2	0xC3 //zegar LED#define PT6958_LED_LARGE_A		(0x77|0x80)

#define PT6958_CMD_ADDR_LED3	0xC5 //malpa LED
#define PT6958_CMD_ADDR_LED4	0xC7 //wykrzyknik LED
//------------------------------------------------------------------------------
#define LED_LEAVE		0xFF
#define LED_OFF			0
#define LED_ON			1
#define LED_RED 		1
#define LED_GREEN 		2
#define LED_ORANGE		3
//------------------------------------------------------------------------------
#define FP_DISP1		PT6958_CMD_ADDR_DISP1
#define FP_DISP2		PT6958_CMD_ADDR_DISP2
#define FP_DISP3		PT6958_CMD_ADDR_DISP3
#define FP_DISP4		PT6958_CMD_ADDR_DISP4
//------------------------------------------------------------------------------
#define FP_LED_POWER		PT6958_CMD_ADDR_LED1
#define FP_LED_CLOCK		PT6958_CMD_ADDR_LED2
#define FP_LED_MAIL		PT6958_CMD_ADDR_LED3
#define FP_LED_WARN		PT6958_CMD_ADDR_LED4
/**********************************************************************
 * CMD 80: Display Control Command
 * bits:   1000 dcba
 *
 * cba - Dimming Quantity Settings:
 * 000: Pulse width = 1/16
 * 001: Pulse width = 2/16
 * 010: Pulse width = 4/16
 * 011: Pulse width = 10/16
 * 100: Pulse width = 11/16
 * 101: Pulse width = 12/16
 * 110: Pulse width = 13/16
 * 111: Pulse width = 14/16
 *
 *   d - Display Settings
 *   0: Display Off (Key Scan Continues)
 *   1: Display On
 **********************************************************************/
#define PT6958_CMD_DISPLAY_OFF		0x80
#define PT6958_CMD_DISPLAY_OFF_DIM(x)	(0x80+x) //0x80-0x87
#define PT6958_CMD_DISPLAY_ON		0x8E //0x8E org adb_box
#define PT6958_CMD_DISPLAY_ON_DIM(x)	(0x88+x) //0x88-0x8F
/*********************************************************************
 * Definition of LED segment
 *
 *      MSB  LSB
 *  bit 76543210
 *
 *      -- 0
 *  5  |  |  1
 *    6 --
 *  4  |  |  2
 *      -- 3
 *
 **********************************************************************/
#define PT6958_LED_A		(0x77|0x00)	//A
#define PT6958_LED_B		(0x7C|0x00)	//b
#define PT6958_LED_C		(0x58|0x00)	//c
#define PT6958_LED_D		(0x5E|0x00)	//d
#define PT6958_LED_E		(0x79|0x00)	//E
#define PT6958_LED_F		(0x71|0x00)	//F
#define PT6958_LED_G		(0x6F|0x00)	//g
#define PT6958_LED_H		(0x74|0x00)	//h
#define PT6958_LED_I		(0x30|0x00)	//i
#define PT6958_LED_J		(0x0e|0x00)	//J
#define PT6958_LED_K		(0x76|0x00) 	//K
#define PT6958_LED_L		(0x38|0x00)	//L
#define PT6958_LED_M		(0x37|0x00)	//M
#define PT6958_LED_N		(0x54|0x00)	//n
#define PT6958_LED_O		(0x5C|0x00)	//o
#define PT6958_LED_P		(0x73|0x00)	//P
#define PT6958_LED_Q		(0x67|0x00)	//q
#define PT6958_LED_R		(0x50|0x00)	//r
#define PT6958_LED_S		(0x6D|0x00)	//S
#define PT6958_LED_T		(0x78|0x00)	//t
#define PT6958_LED_U		(0x1C|0x00)	//u
#define PT6958_LED_V		(0x3C|0x00)	//V 
#define PT6958_LED_W		(0x3E|0x00)	//W
#define PT6958_LED_X		(0x76|0x80)	//X 
#define PT6958_LED_Y		(0x66|0x00)	//Y
#define PT6958_LED_Z		(0x5B|0x00)	//Z

/*
 * 0 1 2 3 4 5 6 6 7 8 9
 */


#define PT6958_LED_0_dot		(0x3F|0x80)
#define PT6958_LED_1_dot		(0x06|0x80)
#define PT6958_LED_2_dot		(0x5B|0x80)
#define PT6958_LED_3_dot		(0x4F|0x80)
#define PT6958_LED_4_dot		(0x66|0x80)
#define PT6958_LED_5_dot		(0x6D|0x80)
#define PT6958_LED_6_dot		(0x7D|0x80)
#define PT6958_LED_7_dot		(0x27|0x80)
#define PT6958_LED_8_dot		(0x7F|0x80)
#define PT6958_LED_9_dot		(0x6F|0x80)


#define PT6958_LED_0			(0x3F|0x00)
#define PT6958_LED_1			(0x06|0x00)
#define PT6958_LED_2			(0x5B|0x00)
#define PT6958_LED_3			(0x4F|0x00)
#define PT6958_LED_4			(0x66|0x00)
#define PT6958_LED_5			(0x6D|0x00)
#define PT6958_LED_6			(0x7D|0x00)
#define PT6958_LED_7			(0x27|0x00)
#define PT6958_LED_8			(0x7F|0x00)
#define PT6958_LED_9			(0x6F|0x00)



/*
 * A B C E F H L I O P S T U
 */
#define PT6958_LED_LARGE_A		(0x77|0x80)
#define PT6958_LED_LARGE_B		PT6958_LED_8
#define PT6958_LED_LARGE_C		(0x39|0x80)
#define PT6958_LED_LARGE_E		(0x79|0x80)
#define PT6958_LED_LARGE_F		(0x71|0x80)
#define PT6958_LED_LARGE_H		(0x76|0x80)
#define PT6958_LED_LARGE_I		(0x30|0x80)
#define PT6958_LED_LARGE_L		(0x38|0x80)
#define PT6958_LED_LARGE_O		PT6958_LED_0
#define PT6958_LED_LARGE_P		(0x73|0x80)
#define PT6958_LED_LARGE_S		(0x6D|0x80)
#define PT6958_LED_LARGE_T		(0x31|0x80)
#define PT6958_LED_LARGE_U		(0x3E|0x80)
/*
 * b c d i g h n o r t
 */
#define PT6958_LED_SMALL_B		(0x7C|0x80)
#define PT6958_LED_SMALL_C		(0x58|0x80)
#define PT6958_LED_SMALL_D		(0x5E|0x80)
#define PT6958_LED_SMALL_I		(0x10|0x80)
#define PT6958_LED_SMALL_G		(0x6F|0x80)
#define PT6958_LED_SMALL_H		(0x74|0x80)
#define PT6958_LED_SMALL_N		(0x54|0x80)
#define PT6958_LED_SMALL_O		(0x5C|0x80)
#define PT6958_LED_SMALL_R		(0x50|0x80)
#define PT6958_LED_SMALL_T		(0x78|0x80)
/*
 * - _ ~ *
 */
#define PT6958_LED_DASH			(0x40|0x00)
#define PT6958_LED_UNDERSCORE		(0x08|0x00)
#define PT6958_LED_UPPERSCORE		(0x01|0x00)
#define PT6958_LED_EMPTY		(0x00|0x00)
#define PT6958_LED_ASTERISK		(0x63|0x00)
/*
 * .
 */
#define PT6958_LED_DOT			(0x00|0x80)

/*
 * mapa przyciskow
 * PCB:
 * SW1 = POWER, SW2 = UP, SW3 = DOWN, SW4 = OK
 * SW5 = MENU, SW6 = LEFT, SW8 = RIGHT, SW9 = LIST
 *
 * K1 = POWER [SW 1], 	UP [SW 5]
 * K2 = MENU [SW 2], 	LEFT [SW 6]
 * K3 = RIGHT [SW 3], 	LIST [SW 7]
 * K4 = DOWN [SW 4], 	OK [SW 8]
 */



//void pt6958_pow_off(void);
static int pt6958_led_control(unsigned char led, unsigned char set);
unsigned char pt6958_read_key(void);
void pt6958_display(char *str);



#endif


