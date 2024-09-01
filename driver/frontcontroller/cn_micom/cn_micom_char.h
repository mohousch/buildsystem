/**
 * \file				cn_micom_char.h
 *
 * \brief				character definitions of Crenova Micoms
 *
 * \author				W.Y.Yi
 *
 * \section histroy		History
 *						- 2009.03.06. start
 *
 */
#ifndef __CN_MICOM_CHAR_H__	// {
#	define __CN_MICOM_CHAR_H__


/******************************************************************************/
// INCLUDE
/******************************************************************************/


/******************************************************************************/
// DEFINE
/******************************************************************************/

/*******************************************************************************
		<<VFD>>
Model 		: FLEXLINE, ECOLINE
Front Micom : Version 3.xx.xx

        0
   -----------
 |\     |     /|
 | \7   |8   / |
5|  \   |   /9 | 1
 |   \  |  /   |
 | 6  \ | /  10|
   ---- * ----    Middle point is 13
 |    / | \    |
 | 12/  |  \11 |
4|  /   |   \  | 2
 | /    |8   \ |
 |/     |     \|
   -----------
        3

Data format :(7|6|5|4  3|2|1|0) (x|x|13|12  11|10|9|8)
You have to keep 0 to "x"

*******************************************************************************/

/******************************************************************************
		<<7-Segment>>
Model : MINILINE
Front Micom : Version 4.xx.xx

      0
  ----------
 |          |
 |          |
5|          | 1
 |          |
 |     6    |
  ----------
 |          |
 |          |
4|          | 2
 |          |
 |          |
  ----------
      3

Data Fotmat : (x|6|5|4  3|2|1|0)
You have to keep 0 to "x"

*******************************************************************************/


#define _7SEG_NULL		"\x00"
#define _7SEG_DASH		"\x40"
#define _7SEG_NUM_0		"\x3F"
#define _7SEG_NUM_1		"\x06"
#define _7SEG_NUM_2		"\x5B"
#define _7SEG_NUM_3		"\x4F"
#define _7SEG_NUM_4		"\x66"
#define _7SEG_NUM_5		"\x6D"
#define _7SEG_NUM_6		"\x7D"
#define _7SEG_NUM_7		"\x07"
#define _7SEG_NUM_8		"\x7F"
#define _7SEG_NUM_9		"\x6F"
#define _7SEG_CH_A		"\x5F"
#define _7SEG_CH_B		"\x0F"
#define _7SEG_CH_C		"\x39"
#define _7SEG_CH_D		"\x5E"
#define _7SEG_CH_E		"\x79"
#define _7SEG_CH_F		"\x71"
#define _7SEG_CH_G		"\x3D"
#define _7SEG_CH_H		"\x76"
#define _7SEG_CH_I		"\x06"
#define _7SEG_CH_J		"\x1E"
#define _7SEG_CH_K		"\x70"
#define _7SEG_CH_L		"\x38"
#define _7SEG_CH_M		"\x55"
#define _7SEG_CH_N		"\x54"
#define _7SEG_CH_O		"\x5C"
#define _7SEG_CH_P		"\x73"
#define _7SEG_CH_Q		"\x3F"
#define _7SEG_CH_R		"\x77"
#define _7SEG_CH_S		"\x6D"
#define _7SEG_CH_T		"\x01"
#define _7SEG_CH_U		"\x1C"
#define _7SEG_CH_V		"\x30"
#define _7SEG_CH_W		"\x6A"
#define _7SEG_CH_X		"\x80"
#define _7SEG_CH_Y		"\x6E"
#define _7SEG_CH_Z		"\x5B"

#define _14SEG_NULL		"\x00\x00"
#define _14SEG_DASH		"\x40\x24"
#define _14SEG_NUM_0	"\x3F\x32"
#define _14SEG_NUM_1	"\x06\x00"
#define _14SEG_NUM_2	"\x5B\x24"
#define _14SEG_NUM_3	"\x4F\x24"
#define _14SEG_NUM_4	"\x66\x24"
#define _14SEG_NUM_5	"\x69\x28"
#define _14SEG_NUM_6	"\x7D\x24"
#define _14SEG_NUM_7	"\x07\x00"
#define _14SEG_NUM_8	"\x7F\x24"
#define _14SEG_NUM_9	"\x6F\x24"
#define _14SEG_CH_A		"\x77\x24"
#define _14SEG_CH_B		"\x0F\x25"
#define _14SEG_CH_C		"\x39\x00"
#define _14SEG_CH_D		"\x0F\x21"
#define _14SEG_CH_E		"\x79\x24"
#define _14SEG_CH_F		"\x71\x24"
#define _14SEG_CH_G		"\x3D\x24"
#define _14SEG_CH_H		"\x76\x24"
#define _14SEG_CH_I		"\x09\x21"
#define _14SEG_CH_J		"\x1E\x00"
#define _14SEG_CH_K		"\x70\x2A"
#define _14SEG_CH_L		"\x38\x00"
#define _14SEG_CH_M		"\xB6\x22"
#define _14SEG_CH_N		"\xB6\x28"
#define _14SEG_CH_O		"\x3F\x00"
#define _14SEG_CH_P		"\x73\x24"
#define _14SEG_CH_Q		"\x3F\x08"
#define _14SEG_CH_R		"\x73\x2C"
#define _14SEG_CH_S		"\x6D\x24"
#define _14SEG_CH_T		"\x01\x21"
#define _14SEG_CH_U		"\x3E\x00"
#define _14SEG_CH_V		"\x30\x32"
#define _14SEG_CH_W		"\x36\x38"
#define _14SEG_CH_X		"\x80\x3A"
#define _14SEG_CH_Y		"\x6E\x24"
#define _14SEG_CH_Z		"\x09\x32"

//러시아어 이름은 로마자 표기 임 (ref wiki)
#if defined(_BUYER_SATCOM_RUSSIA) || defined(_BUYER_SOGNO_GERMANY)
#define _14SEG_RUS_a 		"\x77\x24"
#define _14SEG_RUS_b 		"\x7d\x24"
#define _14SEG_RUS_v 		"\x79\x2A"
#define _14SEG_RUS_g 		"\x31\x00"
#define _14SEG_RUS_d 		"\x0F\x21"
#define _14SEG_RUS_ye 		"\x79\x24"
#define _14SEG_RUS_yo 		"\x79\x24"
#define _14SEG_RUS_zh 		"\xC0\x3F"
#define _14SEG_RUS_z 		"\x09\x2A"
#define _14SEG_RUS_i 		"\x36\x32"
#define _14SEG_RUS_ekra 	"\x36\x32"
#define _14SEG_RUS_k 		"\x70\x2A"
#define _14SEG_RUS_el 		"\x06\x32"
#define _14SEG_RUS_m 		"\xB6\x22"
#define _14SEG_RUS_n 		"\x76\x24"
#define _14SEG_RUS_o 		"\x3F\x00"
#define _14SEG_RUS_p 		"\x37\x00"
#define _14SEG_RUS_r 		"\x73\x24"
#define _14SEG_RUS_s 		"\x39\x00"
#define _14SEG_RUS_t 		"\x01\x21"
#define _14SEG_RUS_u 		"\x6E\x24"
#define _14SEG_RUS_f 		"\x3F\x21"
#define _14SEG_RUS_kh 		"\x80\x3A"
#define _14SEG_RUS_ts 		"\x38\x21"
#define _14SEG_RUS_ch 		"\x66\x24"
#define _14SEG_RUS_sh 		"\x3E\x21"
#define _14SEG_RUS_shch 	"\x3E\x21"
#define _14SEG_RUS_tvyor 	"\x0D\x25"
#define _14SEG_RUS_y 		"\x7E\x21"
#define _14SEG_RUS_myah 	"\x0C\x25"
#define _14SEG_RUS_eobeo 	"\x0F\x24"
#define _14SEG_RUS_yu 		"\x76\x2A"
#define _14SEG_RUS_ya		"\x27\x34"

#endif // END of #if defined(_BUYER_SATCOM_RUSSIA)

#define _7SEG_NULL  "\x00"
#define _14SEG_SPECIAL_1 "\x10\x00"
#define _14SEG_SPECIAL_2 "\x20\x00"
#define _14SEG_SPECIAL_3 "\x01\x00"
#define _14SEG_SPECIAL_4 "\x02\x00"
#define _14SEG_SPECIAL_5 "\x04\x00"
#define _14SEG_SPECIAL_6 "\x08\x00"
#define _14SEG_SPECIAL_7 "\x40\x24"


/******************************************************************************/
// MACRO
/******************************************************************************/


/******************************************************************************/
// GLOBAL VARIABLES
/******************************************************************************/

/******************************************************************************/
// FUNCTION PROTOTYPES
/******************************************************************************/

#endif	// __CN_MICOM_CHAR_H__
/******************************************************************************/
// END OF FILE
/******************************************************************************/
