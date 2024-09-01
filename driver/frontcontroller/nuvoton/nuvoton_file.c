/*
 * nuvoton_file.c
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
******************************************************************************
 *
 * Changes
 *
 * Date     By              Description
 * ----------------------------------------------------------------------------
 * 20131001 Audioniek       Added code for all icons on/off.
 * 20131004 Audioniek       Get wake up reason added.
 * 20131008 Audioniek       Added Octagon1008 (HS9510) missing icons.
 * 20131008 Audioniek       SetLED command, setPwrLED is now obsolete as it
 *                          is a subset of SetLED.
 * 20131008 Audioniek       Beginning of Octagon1008 lower case characters.
 * 20131015 Audioniek       VFDDISPLAYWRITEONOFF made functional on HDBOX.
 * 20131026 Audioniek       Octagon1008 lower case characters completed.
 * 20131126 Audioniek       Start of adding text scrolling.
 * 20131128 Audioniek       Text scroll on /dev/vfd working: text scrolls
 *                          once if text is longer than display size,
 * 20131224 Audioniek       except on HS7119, HS7810A & HS7819.
 * 20131205 Audioniek       Errors and doubles corrected in HDBOX icon table.
 * 20131220 Audioniek       Start of work on ATEVIO7500 (Fortis HS8200).
 * 20131224 Audioniek       nuvotonWriteString on HS7119/HS7810A/HS7819
 *                          completely rewritten including special handling
 *                          of periods and colons.
 * 20140210 Audioniek       Start of work on HS7119 & HS7819.
 * 20140403 Audioniek       SetLED for ATEVIO7500 added.
 * 20140415 Audioniek       ATEVIO7500 finished; yet to do: icons.
 * 20140426 Audioniek       HS7119/7810A/7819 work completed.
 * 20140916 Audioniek       Fixed compiler warnings with HS7119/7810A/7819.
 * 20140921 Audioniek       No special handling of periods and colons for
 *                          HS7119: receiver has not got these segments.
 * 20141010 Audioniek       Special handling of colons for HS7119 added:
 *                          DOES have a colon, but no periods.
 * 20141010 Audioniek       HS7119/HS7810A/HS7819 scroll texts longer than
 *                          4 once.
 * 20151231 Audioniek       HS7420/HS7429 added.
 * 20160103 Audioniek       Compiler warning fixed, references to ATEMIO530
 *                          removed.
 *
 *****************************************************************************/

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

#include "nuvoton.h"
#include "nuvoton_asc.h"
#include "nuvoton_utf.h"

#if defined(OCTAGON1008) \
 || defined(HS7420) \
 || defined(HS7429)
#define DISP_SIZE 8
#elif defined(FORTIS_HDBOX) \
 || defined(ATEVIO7500)
#define DISP_SIZE 12
#elif defined(HS7810A) \
 || defined(HS7119) \
 || defined(HS7819)
#define DISP_SIZE 4
#elif defined(HS7110)
#define DISP_SIZE 0
#endif

extern void ack_sem_up(void);
extern int  ack_sem_down(void);
int nuvotonWriteString(unsigned char *aBuf, int len);
extern void nuvoton_putc(unsigned char data);

struct semaphore write_sem;
int errorOccured = 0;
static char ioctl_data[8];

tFrontPanelOpen FrontPanelOpen [LASTMINOR];

struct saved_data_s
{
	int length;
	char data[128];
};

u8 regs[0xff];  // array with copy values of FP registers

#if defined(OCTAGON1008) || defined(HS7420) || defined(HS7429)
/* Note on adding HS742X: The front panel controller is almost
   equal to the one on the Octagon1008 (the board it is built
   on says HS-9510HD FRONT B-Type) but has a different VFD tube
   mounted on it.
   The VFD has eight 15 segment characters and four icons: a red
   dot and three colons between the main characters.
   Stock firmware does not control the icons; the red dot is used
   as remote feedback, a function internal to the front processor.
   After testing the conclusion was reached that the icons on the
   HS742X models cannot be controlled by the user.
*/

/* Dagobert: On octagon 1008 the "normal" fp setting does not work.
   it seems that with this command we set the segment elements
   direct (not sure here). I dont know how this works but ...

   Added by Audioniek:
   Command format for set icon (Octagon1008 only):
   SOP 0xc4 byte_pos byte_1 byte_2 byte_3 EOP
   byte pos is 0..8 (0=rightmost, 8 is collection of icons on left)
*/

/* character layout for positions 0..7:
       icon
    aaaaaaaaa
   fj   i   hb
   f j  i  h b
 p f  j i h  b
   f   jih   b
    llllgkkkk
   e   onm   c
 p e  o n m  c
   e o  n  m c
   eo   n   mc
    ddddddddd

bit = 1 -> segment on
bit = 0 -> segment off

icon on top is byte_1, bit 0

segment l is byte_2, bit 0 (  1)
segment c is byte_2, bit 1 (  2)
segment e is byte_2, bit 2 (  4)
segment m is byte_2, bit 3 (  8)
segment n is byte_2, bit 4 ( 16)
segment o is byte_2, bit 5 ( 32)
segment d is byte_2, bit 6 ( 64)
segment p is byte_2, bit 7 (128) (colon, only on positions 1, 3 & 5 and only on Octagon1008)

segment a is byte_3, bit 0 (  1)
segment h is byte_3, bit 1 (  2)
segment i is byte_3, bit 2 (  4)
segment j is byte_3, bit 3 (  8)
segment b is byte_3, bit 4 ( 16)
segment f is byte_3, bit 5 ( 32)
segment k is byte_3, bit 6 ( 64)
segment g is byte_3, bit 7 (128)

top icons on positions 0 .. 7: (Octagon1008 only, not on HS742X)
(0) ICON_DOLBY, ICON_DTS, ICON_VIDEO, ICON_AUDIO, ICON_LINK, ICON_HDD, ICON_DISC, ICON_DVB (7)

icons on position 8: (Octagon1008 only, not on HS742X)
ICON_DVD     is byte_1, bit 0 (  1)

ICON_TIMER   is byte_2, bit 0 (  1)
ICON_TIME    is byte_2, bit 1 (  2)
ICON_CARD    is byte_2, bit 2 (  4)
ICON_1       is byte_2, bit 3 (  8)
ICON_2       is byte_2, bit 4 ( 16)
ICON_KEY     is byte_2, bit 5 ( 32)
ICON_16_9    is byte_2, bit 6 ( 64)
ICON_USB     is byte_2, bit 7 (128)

ICON_CRYPTED is byte_3, bit 0 (  1)
ICON_PLAY    is byte_3, bit 1 (  2)
ICON_REWIND  is byte_3, bit 2 (  4)
ICON_PAUSE   is byte_3, bit 3 (  8)
ICON_FF      is byte_3, bit 4 ( 16)
none         is byte_3, bit 5 ( 32)
ICON_REC     is byte_3, bit 6 ( 64)
ICON_ARROW   is byte_3, bit 7 (128) (RC feedback)
*/
struct vfd_buffer vfdbuf[9];

struct chars
{
	u8 s1;
	u8 s2;
	u8 c;
} octagon_chars[] =
{
	{0x00, 0x00, ' '},
	{0x10, 0x84, '!'}, //0x21
	{0x00, 0x30, '"'},
	{0x00, 0x00, '#'},
	{0x43, 0xe1, '$'},
	{0x00, 0x00, '%'},
	{0x00, 0x00, '&'},
	{0x00, 0x02, 0x27},
	{0x08, 0x82, '('},
	{0x20, 0x88, ')'},
	{0x39, 0xce, '*'},
	{0x11, 0xc4, '+'},
	{0x21, 0x00, ','},
	{0x01, 0xc0, '-'},
	{0x04, 0x00, '.'},
	{0x20, 0x82, '/'},
	{0x46, 0x31, '0'},
	{0x02, 0x12, '1'},
	{0x60, 0xd1, '2'},
	{0x43, 0xd1, '3'},
	{0x11, 0xe4, '4'},
	{0x43, 0xe1, '5'},
	{0x47, 0xe1, '6'},
	{0x20, 0x83, '7'},
	{0x47, 0xf1, '8'},
	{0x43, 0xf1, '9'},
	{0x08, 0x82, '<'},
	{0x41, 0xc0, '='},
	{0x20, 0x88, '>'},
	{0x10, 0x83, '?'},
	{0x45, 0xf1, '@'},
	{0x07, 0xf1, 'A'},
	{0x52, 0xd5, 'B'},
	{0x44, 0x21, 'C'},
	{0x52, 0x95, 'D'},
	{0x45, 0xe1, 'E'},
	{0x05, 0xe1, 'F'},
	{0x46, 0x61, 'G'},
	{0x07, 0xf0, 'H'},
	{0x10, 0x84, 'I'},
	{0x46, 0x10, 'J'},
	{0x0d, 0xA2, 'K'},
	{0x44, 0x20, 'L'},
	{0x06, 0xba, 'M'},
	{0x0e, 0xb8, 'N'},
	{0x46, 0x31, 'O'},
	{0x05, 0xf1, 'P'},
	{0x4e, 0x31, 'Q'},
	{0x0d, 0xf1, 'R'},
	{0x43, 0xe1, 'S'},
	{0x10, 0x85, 'T'},
	{0x46, 0x30, 'U'},
	{0x24, 0xa2, 'V'},
	{0x2e, 0xb0, 'W'},
	{0x28, 0x8a, 'X'},
	{0x10, 0x8a, 'Y'},
	{0x60, 0x83, 'Z'},
	{0x44, 0x21, '['},
	{0x08, 0x88, 0x5C},
	{0x42, 0x11, ']'},
	{0x00, 0x31, '^'},
	{0x40, 0x00, '_'},
	{0x00, 0x08, '`'},
	{0x62, 0xc0, 'a'}, // {0x62, 0xd1, 'a'},
	{0x4d, 0xa0, 'b'},
	{0x45, 0xc0, 'c'}, // {0x44, 0x21, 'c'},
	{0x62, 0xd0, 'd'},
	{0x45, 0xa3, 'e'},
	{0x05, 0xa1, 'f'},
	{0x42, 0xd9, 'g'},
	{0x07, 0xe0, 'h'},
	{0x10, 0x80, 'i'}, // {0x10, 0x84, 'i'},
	{0x42, 0x10, 'j'}, // {0x46, 0x10, 'j'},
	{0x18, 0xc4, 'k'},
	{0x10, 0x84, 'l'},
	{0x17, 0xc0, 'm'}, // {0x16, 0xb5, 'm'},
	{0x07, 0xc0, 'n'}, // {0x06, 0x31, 'n'},
	{0x47, 0xc0, 'o'}, // {0x46, 0x31, 'o'},
	{0x05, 0xa3, 'p'},
	{0x02, 0xd9, 'q'},
	{0x05, 0xc0, 'r'}, // {0x04, 0x21, 'r'},
	{0x43, 0xe1, 's'},
	{0x45, 0xa0, 't'},
	{0x46, 0x00, 'u'}, // {0x46, 0x30, 'u'},
	{0x24, 0x00, 'v'}, // {0x24, 0xa2, 'v'},
	{0x56, 0x00, 'w'}, // {0x56, 0xb0, 'w'},
	{0x28, 0x8a, 'x'},
	{0x42, 0xd8, 'y'},
	{0x60, 0x83, 'z'},
	{0x44, 0x21, '{'},
	{0x10, 0x84, '|'},
	{0x42, 0x11, '}'},
	{0x01, 0xc0, '~'},
	{0xff, 0xff, 0x7f} //0x7f
};

#if defined(OCTAGON1008)
enum //HS9510 icon numbers and their names, HS742X models do not have user controllable icons)
{
	ICON_MIN,     // 00
	ICON_DOLBY,   // 01
	ICON_DTS,     // 02
	ICON_VIDEO,   // 03
	ICON_AUDIO,   // 04
	ICON_LINK,    // 05
	ICON_HDD,     // 06
	ICON_DISC,    // 07
	ICON_DVB,     // 08
	ICON_DVD,     // 09
	ICON_TIMER,   // 10
	ICON_TIME,    // 11
	ICON_CARD,    // 12
	ICON_1,       // 13
	ICON_2,       // 14
	ICON_KEY,     // 15
	ICON_16_9,    // 16
	ICON_USB,     // 17
	ICON_CRYPTED, // 18
	ICON_PLAY,    // 19
	ICON_REWIND,  // 20
	ICON_PAUSE,   // 21
	ICON_FF,      // 22
	ICON_NONE,    // 23
	ICON_REC,     // 24
	ICON_ARROW,   // 25
	ICON_COLON1,  // 26
	ICON_COLON2,  // 27
	ICON_COLON3,  // 28
	ICON_MAX      // 29
};

/*
top icons on positions 0 .. 7: (0=rightmost)
(0) ICON_DOLBY, ICON_DTS, ICON_VIDEO, ICON_AUDIO, ICON_LINK, ICON_HDD, ICON_DISC, ICON_DVB (7)

icons on position 8:
ICON_DVD     is byte_1, bit 0 (  1)

ICON_TIMER   is byte_2, bit 0 (  1)
ICON_TIME    is byte_2, bit 1 (  2)
ICON_CARD    is byte_2, bit 2 (  4)
ICON_1       is byte_2, bit 3 (  8)
ICON_2       is byte_2, bit 4 ( 16)
ICON_KEY     is byte_2, bit 5 ( 32)
ICON_16_9    is byte_2, bit 6 ( 64)
ICON_USB     is byte_2, bit 7 (128)

ICON_CRYPTED is byte_3, bit 0 (  1)
ICON_PLAY    is byte_3, bit 1 (  2)
ICON_REWIND  is byte_3, bit 2 (  4)
ICON_PAUSE   is byte_3, bit 3 (  8)
ICON_FF      is byte_3, bit 4 ( 16)
none         is byte_3, bit 5 ( 32)
ICON_REC     is byte_3, bit 6 ( 64)
ICON_ARROW   is byte_3, bit 7 (128) (RC feedback)
*/
struct iconToInternal
{
	char *name;
	u16 icon;
	u8 pos;    //display position (0=rightmost)
	u8 mask1;  //bitmask for on/off,(byte 1)
	u8 mask2;  //bitmask for on/off,(byte 2)
	u8 mask3;  //bitmask for on/off,(byte 3)
} nuvotonIcons[] =
{
	/*- Name -------- icon -------- pos - mask1 mask2 mask3 -----*/
	{ "ICON_DOLBY"  , ICON_DOLBY  , 0x00, 0x01, 0x00, 0x00}, // 01
	{ "ICON_DTS"    , ICON_DTS    , 0x01, 0x01, 0x00, 0x00}, // 02
	{ "ICON_VIDEO"  , ICON_VIDEO  , 0x02, 0x01, 0x00, 0x00}, // 03
	{ "ICON_AUDIO"  , ICON_AUDIO  , 0x03, 0x01, 0x00, 0x00}, // 04
	{ "ICON_LINK"   , ICON_LINK   , 0x04, 0x01, 0x00, 0x00}, // 05
	{ "ICON_HDD"    , ICON_HDD    , 0x05, 0x01, 0x00, 0x00}, // 06
	{ "ICON_DISC"   , ICON_DISC   , 0x06, 0x01, 0x00, 0x00}, // 07
	{ "ICON_DVB"    , ICON_DVB    , 0x07, 0x01, 0x00, 0x00}, // 08
	{ "ICON_DVD"    , ICON_DVD    , 0x08, 0x01, 0x00, 0x00}, // 09
	{ "ICON_TIMER"  , ICON_TIMER  , 0x08, 0x00, 0x01, 0x00}, // 10
	{ "ICON_TIME"   , ICON_TIME   , 0x08, 0x00, 0x02, 0x00}, // 11
	{ "ICON_CARD"   , ICON_CARD   , 0x08, 0x00, 0x04, 0x00}, // 12
	{ "ICON_1"      , ICON_1      , 0x08, 0x00, 0x08, 0x00}, // 13
	{ "ICON_2"      , ICON_2      , 0x08, 0x00, 0x10, 0x00}, // 14
	{ "ICON_KEY"    , ICON_KEY    , 0x08, 0x00, 0x20, 0x00}, // 15
	{ "ICON_16_9"   , ICON_16_9   , 0x08, 0x00, 0x40, 0x00}, // 16
	{ "ICON_USB"    , ICON_USB    , 0x08, 0x00, 0x80, 0x00}, // 17
	{ "ICON_CRYPTED", ICON_CRYPTED, 0x08, 0x00, 0x00, 0x01}, // 18
	{ "ICON_PLAY"   , ICON_PLAY   , 0x08, 0x00, 0x00, 0x02}, // 19
	{ "ICON_REWIND" , ICON_REWIND , 0x08, 0x00, 0x00, 0x04}, // 20
	{ "ICON_PAUSE"  , ICON_PAUSE  , 0x08, 0x00, 0x00, 0x08}, // 21
	{ "ICON_FF"     , ICON_FF     , 0x08, 0x00, 0x00, 0x10}, // 22
	{ "ICON_NONE"   , ICON_NONE   , 0x08, 0x00, 0x00, 0x20}, // 23
	{ "ICON_REC"    , ICON_REC    , 0x08, 0x00, 0x00, 0x40}, // 24
	{ "ICON_ARROW"  , ICON_ARROW  , 0x08, 0x00, 0x00, 0x80}, // 25
	{ "ICON_COLON1" , ICON_COLON1 , 0x01, 0x00, 0x80, 0x00}, // 26
	{ "ICON_COLON2" , ICON_COLON2 , 0x03, 0x00, 0x80, 0x00}, // 27
	{ "ICON_COLON3" , ICON_COLON3 , 0x05, 0x00, 0x80, 0x00}, // 28
};
#endif //Octagon1008 only (icon definitions)
#endif //Octagon1008, HS7420 & HS7429 (character stuff)

#if defined(HS7119) || defined(HS7810A) || defined(HS7819)
static int _7seg_fonts[] =
	/* character layout:

	    aaaaaaaaa
	   f         b
	   f         b
	   f         b  i
	   f         b
	    ggggggggg
	   e         c
	   e         c  i
	   e         c
	   e         c
	    ddddddddd  h

	segment a is bit 0 (  1)
	segment b is bit 1 (  2)
	segment c is bit 2 (  4)
	segment d is bit 3 (  8)
	segment e is bit 4 ( 16, 0x10)
	segment f is bit 5 ( 32, 0x20)
	segment g is bit 6 ( 64, 0x40)
	segment h is bit 7 (128, 0x80, positions 2, 3 & 4, cmd byte 2, 3 & 5, not on HS7119)
	segment i is bit 7 (128, 0x80, position 3 only, cmd byte 4)
	NOTE: period on 1st position cannot be controlled */

{
//	' '   '!'    '"'   '#'   '$'   '%'   '&'  '
	0x00, 0x06, 0x22, 0x5c, 0x6d, 0x52, 0x7d, 0x02,
//	'('   ')'   '*'   '+'   ','   '-'   '.'   '/'
	0x39, 0x0f, 0x76, 0x46, 0x0c, 0x40, 0x08, 0x52,
//	'0'   '1'   '2'   '3'   '4'   '5'   '6'   '7'
	0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,
//	'8'   '9'   ':'   ';'   '<'   '='   '>'   '?'
	0x7f, 0x6f, 0x10, 0x0c, 0x58, 0x48, 0x4c, 0x53,
//	'@'   'A'   'B'   'C'   'D'   'E'   'F'   'G'
	0x7b, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x3d,
//	'H'   'I'   'J'   'K'   'L'   'M'   'N'   'O'
	0x76, 0x06, 0x1e, 0x7a, 0x38, 0x55, 0x37, 0x3f,
//	'P'   'Q'   'R'   'S'   'T'   'U'   'V'   'W'
	0x73, 0x67, 0x77, 0x6d, 0x78, 0x3e, 0x3e, 0x7e,
//	'X'   'Y'   'Z'   '['   '\'   ']'   '^'   '_'
	0x76, 0x66, 0x5b, 0x39, 0x64, 0x0f, 0x23, 0x08,
//	'`'   'a'   'b'   'c'   'd'   'e'   'f'   'g'
	0x20, 0x5f, 0x7c, 0x58, 0x5e, 0x7b, 0x71, 0x6f,
//	'h'   'i'   'j'   'k'   'l'   'm'   'n'  'o'
	0x74, 0x04, 0x0e, 0x78, 0x38, 0x55, 0x54, 0x5c,
//	'p'   'q'   'r'   's'   't'   'u'   'v'   'w'
	0x73, 0x67, 0x50, 0x6d, 0x78, 0x1c, 0x1c, 0x1c,
//	'x'   'y'   'z'   '{'   '|'   '}'   '~'   DEL
	0x76, 0x6e, 0x5b, 0x39, 0x06, 0x0f, 0x40, 0x5c
};
#endif


#if defined(FORTIS_HDBOX)
enum
{
	ICON_MIN,       // 00
	ICON_STANDBY,   // 01
	ICON_SAT,       // 02
	ICON_REC,       // 03
	ICON_TIMESHIFT, // 04
	ICON_TIMER,     // 05
	ICON_HD,        // 06
	ICON_USB,       // 07
	ICON_SCRAMBLED, // 08
	ICON_DOLBY,     // 09
	ICON_MUTE,      // 10
	ICON_TUNER1,    // 11
	ICON_TUNER2,    // 12
	ICON_MP3,       // 13
	ICON_REPEAT,    // 14
	ICON_PLAY,      // 15
	ICON_Circ0,     // 16
	ICON_Circ1,     // 17
	ICON_Circ2,     // 18
	ICON_Circ3,     // 19
	ICON_Circ4,     // 20
	ICON_Circ5,     // 21
	ICON_Circ6,     // 22
	ICON_Circ7,     // 23
	ICON_Circ8,     // 24
	ICON_FILE,      // 25
	ICON_TER,       // 26
	ICON_480i,      // 27
	ICON_480p,      // 28
	ICON_576i,      // 29
	ICON_576p,      // 30
	ICON_720p,      // 31
	ICON_1080i,     // 32
	ICON_1080p,     // 33
	ICON_COLON1,    // 34
	ICON_COLON2,    // 35
	ICON_COLON3,    // 36
	ICON_COLON4,    // 37
	ICON_TV,        // 38
	ICON_RADIO,     // 39
	ICON_MAX        // 40
};

struct iconToInternal
/* TODO: Icons 1 through 16 work on all models, but others only on early production ones */
{
	char *name;
	u16 icon;
	u8 internalCode1; //FP register number
	u8 SymbolData1;   //bitmask for on/off
	u8 internalCode2; //FP register number #2 for double icons, 0xff=single icon
	u8 SymbolData2;   //bitmask #2 for on/off for double icons
} nuvotonIcons[] =
{
	/*- Name ------------ icon --------- code1 data1 code2 data2 -----*/
	{ "ICON_STANDBY"  , ICON_STANDBY   , 0x20, 0x08, 0xff, 0x00}, // 01
	{ "ICON_SAT"      , ICON_SAT       , 0x20, 0x04, 0xff, 0x00}, // 02
	{ "ICON_REC"      , ICON_REC       , 0x30, 0x01, 0xff, 0x00}, // 03
	{ "ICON_TIMESHIFT", ICON_TIMESHIFT , 0x31, 0x02, 0x32, 0x02}, // 04
	{ "ICON_TIMER"    , ICON_TIMER     , 0x33, 0x02, 0xff, 0x00}, // 05
	{ "ICON_HD"       , ICON_HD        , 0x34, 0x02, 0xff, 0x00}, // 06
	{ "ICON_USB"      , ICON_USB       , 0x35, 0x02, 0xff, 0x00}, // 07
	{ "ICON_SCRAMBLED", ICON_SCRAMBLED , 0x36, 0x02, 0xff, 0x00}, // 08
	{ "ICON_DOLBY"    , ICON_DOLBY     , 0x37, 0x02, 0xff, 0x00}, // 09
	{ "ICON_MUTE"     , ICON_MUTE      , 0x38, 0x02, 0xff, 0x00}, // 10
	{ "ICON_TUNER1"   , ICON_TUNER1    , 0x39, 0x02, 0xff, 0x00}, // 11
	{ "ICON_TUNER2"   , ICON_TUNER2    , 0x3a, 0x02, 0xff, 0x00}, // 12
	{ "ICON_MP3"      , ICON_MP3       , 0x3b, 0x02, 0xff, 0x00}, // 13
	{ "ICON_REPEAT"   , ICON_REPEAT    , 0x3c, 0x02, 0xff, 0x00}, // 14
	{ "ICON_PLAY"     , ICON_PLAY      , 0x20, 0x02, 0xff, 0x00}, // 15 play in circle
	{ "ICON_Circ0"    , ICON_Circ0     , 0x24, 0x01, 0xff, 0x00}, // 16 inner circle
	{ "ICON_Circ1"    , ICON_Circ1     , 0x20, 0x01, 0xff, 0x00}, // 17 circle, segment 1
	{ "ICON_Circ2"    , ICON_Circ2     , 0x22, 0x01, 0xff, 0x00}, // 18 circle, segment 2
	{ "ICON_Circ3"    , ICON_Circ3     , 0x21, 0x02, 0xff, 0x00}, // 19 circle, segment 3
	{ "ICON_Circ4"    , ICON_Circ4     , 0x23, 0x02, 0xff, 0x00}, // 20 circle, segment 4
	{ "ICON_Circ5"    , ICON_Circ5     , 0x24, 0x02, 0xff, 0x00}, // 21 circle, segment 5
	{ "ICON_Circ6"    , ICON_Circ6     , 0x22, 0x02, 0xff, 0x00}, // 22 circle, segment 6
	{ "ICON_Circ7"    , ICON_Circ7     , 0x23, 0x01, 0xff, 0x00}, // 23 circle, segment 7
	{ "ICON_Circ8"    , ICON_Circ8     , 0x21, 0x01, 0xff, 0x00}, // 24 circle, segment 8
	{ "ICON_FILE"     , ICON_FILE      , 0x22, 0x04, 0xff, 0x00}, // 25
	{ "ICON_TER"      , ICON_TER       , 0x21, 0x04, 0xff, 0x00}, // 26
	{ "ICON_480i"     , ICON_480i      , 0x24, 0x40, 0x23, 0x40}, // 27
	{ "ICON_480p"     , ICON_480p      , 0x24, 0x40, 0x22, 0x40}, // 28
	{ "ICON_576i"     , ICON_576i      , 0x21, 0x40, 0x20, 0x40}, // 29
	{ "ICON_576p"     , ICON_576p      , 0x21, 0x40, 0x24, 0x20}, // 30
	{ "ICON_720p"     , ICON_720p      , 0x23, 0x20, 0xff, 0x00}, // 31
	{ "ICON_1080i"    , ICON_1080i     , 0x22, 0x20, 0xff, 0x00}, // 32
	{ "ICON_1080p"    , ICON_1080p     , 0x21, 0x20, 0xff, 0x00}, // 33
	{ "ICON_COLON1"   , ICON_COLON1    , 0x36, 0x01, 0xff, 0x00}, // 34 ":" left between text
	{ "ICON_COLON2"   , ICON_COLON2    , 0x39, 0x01, 0xff, 0x00}, // 35 ":" middle between text
	{ "ICON_COLON3"   , ICON_COLON3    , 0x3a, 0x01, 0xff, 0x00}, // 36 ":" right between text
	{ "ICON_COLON4"   , ICON_COLON4    , 0x3b, 0x01, 0xff, 0x00}, // 37 ":" right between text (dimmer)
	{ "ICON_TV"       , ICON_TV        , 0x23, 0x04, 0xff, 0x00}, // 38
	{ "ICON_RADIO"    , ICON_RADIO     , 0x24, 0x04, 0xff, 0x00}  // 39
};
#endif

/*      command name             Opc     L  parms                             returns       */
#define cCommandGetMJD           0x10 // 01 -                                 5 bytes: MJDh MJDl hrs mins secs
#define cCommandSetTimeFormat    0x11 // 02 1:format byte                     nothing
#define cCommandSetMJD           0x15 // 06 1:mjdH 2:mjdl 3:hrs 4:mins 5:secs nothing

#define cCommandSetPowerOff      0x20 // not used in nuvoton

#define cCommandPowerOffReplay   0x31 // 02 1:1=power off
#define cCommandSetBootOn        0x40 // 01 -
#define cCommandSetBootOnExt     0x41 // not used in nuvoton

#define cCommandSetWakeupTime    0x72 // 03 1:hrs 2:mins                      nothing
#define cCommandSetWakeupMJD     0x74 // 05 1:MJDh 2:MJDl 3:hrs 4:mins        nothing

#define cCommandGetPowerOnSrc    0x80 // 01                                   1 byte: wake up reason (2= from deep sleep)
//TODO: returns error on HS742X

#define cCommandSetLed           0x93 // 04 1:LED# 2:level 3:08               nothing

#define cCommandGetIrCode        0xa0
#define cCommandGetIrCodeExt     0xa1
#define cCommandSetIrCode        0xa5 // 06 (HS8200 only?)

#define cCommandGetPort          0xb2
#define cCommandSetPort          0xb3

#if defined(OCTAGON1008)
#define cCommandSetIcon          0xc4 // 05 1:position 2:data 3:data 4:data
#else
#define cCommandSetIconI         0xc2 // 03 1:registernumber 2:bitmask for on/off     /* 0xc2 0xc7 0xcb 0xcc */ /* display cgram */
#define cCommandSetIconII        0xc7 // not used in nuvoton
#endif

#if defined(OCTAGON1008) || defined(HS7420) || defined(HS7429)
#define cCommandSetVFD           0xc4 //same as cCommandSetIcon
#elif defined(HS7110) || defined(HS7119) || defined(HS7810A) || defined(HS7819)
#define cCommandSetVFD           0x24 // 05 1:data pos1 2:data pos2 3:data pos3 4:data pos4
#else
#define cCommandSetVFD           0xce // 14 1:0x11 2-13 characters /* 0xc0 */
#endif

#define cCommandSetVFDBrightness 0xd2 // 03 1:0 2:level

#if defined(ATEVIO7500) || defined(HS7110) || defined(HS7119) || defined(HS7810A) || defined(HS7819) || defined(HS7420) || defined(HS7429)
#define cCommandGetFrontInfo     0xd0 // not used in nuvoton
#else
#define cCommandGetFrontInfo     0xe0 // not used in nuvoton
#endif

static struct saved_data_s lastdata;

/****************************************************************************************/

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

int nuvotonWriteCommand(char *buffer, int len, int needAck)
{
	int i;

	dprintk(150, "%s >\n", __func__);

	for (i = 0; i < len; i++)
	{
		udelay(1);
#ifdef DIRECT_ASC
		serial_putc(buffer[i]);
#else
		nuvoton_putc(buffer[i]);
#endif
	}

	if (needAck)
	{
		if (ack_sem_down())
		{
			return -ERESTARTSYS;
		}
	}
	dprintk(150, "%s < \n", __func__);
	return 0;
}

#if defined (OCTAGON1008)
int nuvotonSetIcon(int which, int on)
{
	char buffer[7];
	int  res = 0;

	dprintk(100, "%s > %d, %d\n", __func__, which, on);

	if (which < ICON_MIN + 1 || which > ICON_MAX - 1)
	{
		printk("[nuvoton] Icon number %d out of range (1..%d)\n", which, ICON_MAX - 1);
		return -EINVAL;
	}

	dprintk(5, "Set icon %s (number %d) to %d\n", nuvotonIcons[which].name, which, on);
	which--;
	printk("VFD/Nuvoton icon number %d\n", which);
	memset(buffer, 0, 7);

	buffer[0] = SOP;
	buffer[1] = cCommandSetIcon;
	buffer[2] = nuvotonIcons[which].pos;

	//icons on positions 0 ..7 and ICON_DVD
	if ((nuvotonIcons[which].mask2 == 0) && (nuvotonIcons[which].mask3 == 0))
	{
		if (on)
		{
			vfdbuf[nuvotonIcons[which].pos].buf0 = buffer[3] = vfdbuf[nuvotonIcons[which].pos].buf0 | 0x01;
		}
		else
		{
			vfdbuf[nuvotonIcons[which].pos].buf0 = buffer[3] = vfdbuf[nuvotonIcons[which].pos].buf0 & 0xfe;
		}
		buffer[4] = vfdbuf[nuvotonIcons[which].pos].buf1;
		buffer[5] = vfdbuf[nuvotonIcons[which].pos].buf2;
	}
	else if (nuvotonIcons[which].mask1 == 0)
	{
		//icons on position 8, and colons
		buffer[3] = vfdbuf[nuvotonIcons[which].pos].buf0;
		if (on)
		{
			vfdbuf[nuvotonIcons[which].pos].buf1 = buffer[4] = vfdbuf[nuvotonIcons[which].pos].buf1 | nuvotonIcons[which].mask2;
			vfdbuf[nuvotonIcons[which].pos].buf2 = buffer[5] = vfdbuf[nuvotonIcons[which].pos].buf2 | nuvotonIcons[which].mask3;
		}
		else
		{
			vfdbuf[nuvotonIcons[which].pos].buf1 = buffer[4] = vfdbuf[nuvotonIcons[which].pos].buf1 & ~nuvotonIcons[which].mask2;
			vfdbuf[nuvotonIcons[which].pos].buf2 = buffer[5] = vfdbuf[nuvotonIcons[which].pos].buf2 & ~nuvotonIcons[which].mask3;
		}
	}
	buffer[6] = EOP;

	res = nuvotonWriteCommand(buffer, 7, 0);

	dprintk(100, "%s <\n", __func__);
	return res;
}

#elif defined(FORTIS_HDBOX)
int nuvotonSetIcon(int which, int on)  //works for icons 1 - 16 only on later production models
{
	char buffer[5];
	u8 internalCode1, SymbolData1, internalCode2, SymbolData2;
	int  vLoop, res = 0;

	dprintk(100, "%s > %d, %d\n", __func__, which, on);

	if (which < ICON_MIN + 1 || which > ICON_MAX - 1)
	{
		printk("[nuvoton] Icon number %d out of range (1-%d)\n", which, ICON_MAX - 1);
		return -EINVAL;
	}

	internalCode1 = 0xff;
	internalCode2 = 0xff;
	SymbolData1 = 0x00;
	SymbolData2 = 0x00;

	for (vLoop = 0; vLoop < ARRAY_SIZE(nuvotonIcons); vLoop++)
	{
		if ((which & 0xff) == nuvotonIcons[vLoop].icon) //look for icon number
		{
			internalCode1 = nuvotonIcons[vLoop].internalCode1; //if found
			internalCode2 = nuvotonIcons[vLoop].internalCode2; //get data
			SymbolData1 = nuvotonIcons[vLoop].SymbolData1;
			SymbolData2 = nuvotonIcons[vLoop].SymbolData2;
			if (internalCode1 == 0xff) // if code for unknown or not found
			{
				printk("%s: Unknown or unsupported icon %d ->%s\n", __func__, which, nuvotonIcons[vLoop].name);
				return -EINVAL;
			}

			memset(buffer, 0, 6);  //else clear buffer

			buffer[0] = SOP;        //fill
			buffer[1] = cCommandSetIconI;   //buffer
			buffer[2] = internalCode1;  //with

			if (on)
			{
				regs[internalCode1] = buffer[3] = regs[internalCode1] | SymbolData1;    //buffer3 is old register plus bitmask
			}
			else
			{
				regs[internalCode1] = buffer[3] = regs[internalCode1] & ~SymbolData1;   //buffer3 is old register minus bitmask
			}
			buffer[4] = EOP;    //data
			dprintk(50, "[nuvoton] Set icon number %d to %d\n", which, on);
			res = nuvotonWriteCommand(buffer, 5, 0);  //write to FP

			/* e.g. timeshift is made up of two parts */
			if (internalCode2 != 0xff)  // if multi part icon
			{
				memset(buffer, 0, 6);   //repeat process for 2nd part
				buffer[0] = SOP;
				buffer[1] = cCommandSetIconI;
				buffer[2] = internalCode2;
				if (on)
				{
					regs[internalCode2] = buffer[3] = regs[internalCode2] | SymbolData2;
				}
				else
				{
					regs[internalCode2] = buffer[3] = regs[internalCode2] & ~SymbolData2;
				}
				buffer[4] = EOP;

				res = nuvotonWriteCommand(buffer, 5, 0);
			}
		}
	}

	dprintk(100, "%s <\n", __func__);
	return res;
}
#elif defined (ATEVIO7500)
int nuvotonSetIcon(int which, int on)
{
	return 0;
}
#else // HS7110, HS7119, HS7420, HS7429, HS7810A & HS7819
int nuvotonSetIcon(int which, int on)
{
	return 0;
}
#endif

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetIcon);

int nuvotonSetLED(int which, int level)
{
	char buffer[6];
	int res = -1;

	dprintk(100, "%s > %d, %d\n", __func__, which, level);

#if defined(OCTAGON1008)
#define MAX_LED 1
#define MAX_BRIGHT 7
#elif defined(FORTIS_HDBOX) || defined(ATEVIO7500) // New Nuvoton                    
#define MAX_LED 256  // LED number is a bit mask: bit 0 (  1) = red power
	//                           bit 1 (  2) = blue power
	//                           bit 2 (  4) = -
	//                           bit 3 (  8) = -
	//                           bit 4 ( 16) = cross left
	//                           bit 5 ( 32) = cross bottom
	//                           bit 6 ( 64) = cross top
	//                           bit 7 (128) = cross right
	//                           (0xf2 = full cross plus blue)
	// TODO: Buggy on ATEVIO7500, fp seems to have a life of its own
#define MAX_BRIGHT 31
#elif defined(HS7420) || defined(HS7429)
	// LED number is a bit mask:
	// bit 0 = standby (red),
	// bit 1 = logo (not on all models),
#define MAX_LED 2 // must be power of 2
#define MAX_BRIGHT 7
#elif defined(HS7810A) || defined(HS7819)
	// LED number is a bit mask:
	// bit 0 = standby (red),
	// bit 1 = logo,
	// bit 2(?) = RC feedback (green) seems to be not controllable (off when red is on)
#define MAX_LED 4 // must be power of 2
#define MAX_BRIGHT 7
#elif defined(HS7110) || defined(HS7119)
	// LED number is a bit mask:
	// bit 0 = standby (red),
	// bit 1(?) = RC feedback (green) seems to be not controllable (off when red is on)
#define MAX_LED 2 // must be power of 2
#define MAX_BRIGHT 7
#endif

#if defined(HS7110)
	printk("[nuvoton] Function %s is not supported on HS7110 (yet)\n", __func__);
	return -EINVAL;
#else
	if (which < 1 || which > MAX_LED)
	{
		printk("[nuvoton] LED number %d out of range (1-%d)\n", which, MAX_LED);
		return -EINVAL;
	}

	if (level < 0 || level > MAX_BRIGHT)
	{
		printk("[nuvoton] LED brightness level %d out of range (0-%d)\n", level, MAX_BRIGHT);
		return -EINVAL;
	}

	memset(buffer, 0, 6);

	buffer[0] = SOP;
	buffer[1] = cCommandSetLed;
	buffer[2] = which;
	buffer[3] = level;
	buffer[4] = 0x08; //what is this?
	buffer[5] = EOP;

	res = nuvotonWriteCommand(buffer, 6, 0);

	dprintk(100, "%s <\n", __func__);
	return res;
#endif
}

/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetLED);

#if !defined(HS7110) \
 && !defined(HS7119) \
 && !defined(HS7810A) \
 && !defined(HS7819)
int nuvotonSetBrightness(int level)
{
	char buffer[5];
	int res = -1;

	dprintk(100, "%s > %d\n", __func__, level);
	if (level < 0 || level > 7)
	{
		printk("[nuvoton] VFD brightness level %d out of range (0-%d)\n", level, 7);
		return -EINVAL;
	}

	memset(buffer, 0, 6);

	buffer[0] = SOP;
	buffer[1] = cCommandSetVFDBrightness;
	buffer[2] = 0x00;
	buffer[3] = level;
	buffer[4] = EOP;

	res = nuvotonWriteCommand(buffer, 5, 0);

	dprintk(100, "%s <\n", __func__);
	return res;
}
/* export for later use in e2_proc */
EXPORT_SYMBOL(nuvotonSetBrightness);
#endif

int nuvotonSetStandby(char *time)
{
	char     buffer[8];
	char     power_off[] = {SOP, cCommandPowerOffReplay, 0x01, EOP};
	int      res = 0;

	dprintk(100, "%s >\n", __func__);

//	res = nuvotonWriteString("Bye bye ...", strlen("Bye bye ..."));

	/* set wakeup time */
	memset(buffer, 0, 8);

	buffer[0] = SOP;
	buffer[1] = cCommandSetWakeupMJD;

	memcpy(buffer + 2, time, 4); /* only 4 because we do not have seconds here */
	buffer[6] = EOP;

	res = nuvotonWriteCommand(buffer, 7, 0);

	/* now power off */
	res = nuvotonWriteCommand(power_off, sizeof(power_off), 0);

	dprintk(100, "%s <\n", __func__);
	return res;
}

int nuvotonSetTime(char *time)
{
	char buffer[8];
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(buffer, 0, 8);

	buffer[0] = SOP;
	buffer[1] = cCommandSetMJD;

	memcpy(buffer + 2, time, 5);
	buffer[7] = EOP;

	res = nuvotonWriteCommand(buffer, 8, 0);

	dprintk(100, "%s <\n", __func__);
	return res;
}

int nuvotonGetTime(void)
{
	char buffer[3];
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(buffer, 0, 3);

	buffer[0] = SOP;
	buffer[1] = cCommandGetMJD;
	buffer[2] = EOP;

	errorOccured = 0;
	res = nuvotonWriteCommand(buffer, 3, 1);

	if (errorOccured == 1)
	{
		/* error */
		memset(ioctl_data, 0, 8);
		printk("error\n");
		res = -ETIMEDOUT;
	}
	else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
		dprintk(20, "myTime= 0x%02x - 0x%02x - 0x%02x - 0x%02x - 0x%02x\n", ioctl_data[0], ioctl_data[1],
			ioctl_data[2], ioctl_data[3], ioctl_data[4]);
	}

	dprintk(100, "%s <\n", __func__);
	return res;
}

int nuvotonGetWakeUpMode(void)
{
	char buffer[3];
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(buffer, 0, 3);

	buffer[0] = SOP;
	buffer[1] = cCommandGetPowerOnSrc;
	buffer[2] = EOP;

	errorOccured = 0;
	res = nuvotonWriteCommand(buffer, 3, 1);

	if (errorOccured == 1)
	{
		/* error */
		memset(ioctl_data, 0, 8);
		printk("error\n");

		res = -ETIMEDOUT;
	}
	else
	{
		/* time received ->noop here */
		dprintk(1, "time received\n");
	}

	dprintk(100, "%s <\n", __func__);
	return res;
}

int nuvotonSetTimeFormat(char format)
{
	char buffer[4];
	int  res = -1;

	dprintk(100, "%s >\n", __func__);

	buffer[0] = SOP;
	buffer[1] = cCommandSetTimeFormat;
	buffer[2] = format;
	buffer[3] = EOP;

//	dprintk(20, "Time mode command= 0x%02x (SOP) 0x%02x 0x%02x 0x%02x (EOP)\n", buffer[0], buffer[1], buffer[2], buffer[3]);
	res = nuvotonWriteCommand(buffer, 4, 0);

	dprintk(100, "%s <\n", __func__);
	return res;
}

int nuvotonSetDisplayOnOff(char level)
{
	int  res = 0;
#if defined(OCTAGON1008) //|| defined(HS7420) || defined(HS7429)
	char buffer[5];

	dprintk(100, "%s >\n", __func__);
	if (level != 0)
	{
		level = 7;
	}

	memset(buffer, 0, 5);
	buffer[0] = SOP;
	buffer[1] = cCommandSetVFDBrightness;
	buffer[2] = 0x00;
	buffer[3] = level;
	buffer[4] = EOP;

	res = nuvotonWriteCommand(buffer, 5, 0);
#elif defined(FORTIS_HDBOX)
	char buffer[5];

	dprintk(100, "%s >\n", __func__);
	if (level != 0)
	{
		level = 1;
	}
	memset(buffer, 0, 5);
	buffer[0] = SOP;
	buffer[1] = cCommandSetIconI;
	buffer[2] = 0x70;
	buffer[3] = level;
	buffer[4] = EOP;

	res = nuvotonWriteCommand(buffer, 5, 0);
#elif defined(ATEVIO7500) \
 || defined(HS7420) \
 || defined(HS7810A) \
 || defined(HS7119) \
 || defined(HS7429) \
 || defined(HS7819)
	dprintk(100, "%s >\n", __func__);

	if (level == 0)
	{
		res |= nuvotonWriteString("            ", DISP_SIZE);
	}
	else
	{
		res |= nuvotonWriteString(lastdata.data, lastdata.length);

	}
#endif
	dprintk(100, "%s <\n", __func__);
	return res;
}

//nuvotonWriteString
#if defined(HS7810A) || defined(HS7819) // 4 character 7-segment LED with colon and periods
int nuvotonWriteString(unsigned char *aBuf, int len)
{
	int i, j, res;
	int buflen, strlen;
	int dot_count = 0;
	int colon_count = 0;
	unsigned char cmd_buf[7], bBuf[5];

	dprintk(100, "%s > %d\n", __func__, len);

	memset(cmd_buf, 0, 7);
	buflen = len;
	bBuf[0] = aBuf[0];
	bBuf[1] = aBuf[1];
	i = 2;
	j = 2;

	while ((j <= 4) && (i < buflen))
	{
		if (aBuf[i] == '.')
		{
			if (i == 2)
			{
				cmd_buf[i] = 0x80;
			}
			else
			{
				if (j >= 2)
				{
					cmd_buf[i + 1 - colon_count] = 0x80;
				}
			}
			dot_count++;
		}
		else
		{
			if ((aBuf[i] == ':') && (j == 2))
			{
				cmd_buf[3] = 0x80;
				colon_count++;
			}
			else
			{
				bBuf[j] = aBuf[i];
				j++;
			}
		}
		i++;
	}

	strlen = buflen - dot_count - colon_count;

	if (strlen > 4)
	{
		strlen = 4;
	}

	j = 2;
	for (i = 0; i < 4; i++)
	{
		cmd_buf[j] |= _7seg_fonts[(bBuf[i] - 0x20)];
		j++;
	}

	if (strlen < 4)
	{
		for (i = strlen; i < 4; i++)
		{
			cmd_buf[i + 2] &= 0x80;
		}
	}

	cmd_buf[0] = SOP;
	cmd_buf[1] = cCommandSetVFD;
	cmd_buf[6] = EOP;
	res = nuvotonWriteCommand(cmd_buf, 7, 0);

	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 128);
	lastdata.length = len;

	dprintk(100, "%s <\n", __func__);
	return res;
}
#elif defined(HS7119) // 4 character 7-segment LED with colon
int nuvotonWriteString(unsigned char *aBuf, int len)
{
	int i, res;
	int buflen;
	unsigned char cmd_buf[7], bBuf[5];

	dprintk(10, "%s > %d\n", __func__, len);

	memset(cmd_buf, 0, 7);
	buflen = len;
	bBuf[0] = aBuf[0];
	bBuf[1] = aBuf[1];

	if (aBuf[2] == ':') // if 3rd character is a colon
	{
		cmd_buf[3] = 0x80; // switch colon on
		bBuf[2] = aBuf[3]; // shift rest of buffer
		bBuf[3] = aBuf[4]; // forward
	}
	else
	{
		bBuf[2] = aBuf[2];
		bBuf[3] = aBuf[3];
	}

	if (buflen > 4)
	{
		buflen = 4;
	}

	for (i = 0; i < buflen; i++)
	{
		cmd_buf[i + 2] |= _7seg_fonts[(bBuf[i] - 0x20)];
	}

	cmd_buf[0] = SOP;
	cmd_buf[1] = cCommandSetVFD;
	cmd_buf[6] = EOP;
	res = nuvotonWriteCommand(cmd_buf, 7, 0);

	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 128);
	lastdata.length = len;

	dprintk(10, "%s <\n", __func__);
	return res;
}
#elif defined(OCTAGON1008) || defined(HS7420) || defined(HS7429) // 8 character 15-segment VFD (Octagon1008 with icons)
int nuvotonWriteString(unsigned char *aBuf, int len)
{
	unsigned char bBuf[7];
	int i = 0, max = 0;
	int j = 0;
	int res = 0;

	dprintk(100, "%s > %d\n", __func__, len);
	max = (len > 8) ? 8 : len;
	for (i = max; i < 8; i++)
	{
		aBuf[i] = ' '; //pad string with spaces at end
	}
	// now display 8 characters
	for (i = 0; i < 8 ; i++)
	{
		bBuf[0] = SOP;
		bBuf[1] = cCommandSetVFD;  //(equals cCommandSetIcon)
		bBuf[2] = 7 - i; /* position: 0x00 = right */
		bBuf[3] = vfdbuf[7 - i].buf0;
		bBuf[6] = EOP;

		for (j = 0; j < ARRAY_SIZE(octagon_chars); j++)
		{
			if (octagon_chars[j].c == aBuf[i]) //look up character
			{
				bBuf[4] = octagon_chars[j].s1 | (vfdbuf[7 - i].buf1 & 0x80); //preserve colon icon state
				vfdbuf[7 - i].buf1 = bBuf[4];
				bBuf[5] = octagon_chars[j].s2;
				vfdbuf[7 - i].buf2 = bBuf[5];
				res |= nuvotonWriteCommand(bBuf, 7, 0);
				break;
			}
		}
	}
	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 8);
	lastdata.length = len;

	dprintk(100, "%s <\n", __func__);
	return res;
}
#elif defined(FORTIS_HDBOX) // 12 character dot matrix VFD with colons and icons
int nuvotonWriteString(unsigned char *aBuf, int len)
{
	unsigned char bBuf[12];
	int i = 0;
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(bBuf, ' ', 12);

	if (len > 12) //if string is longer than display
	{
		len = 12; //display 1st 12 characters
	}
	else
	{
		for (i = len; i < 12; i++)
		{
			aBuf[i] = ' '; //pad string with spaces
		}
	}

	for (i = 0; i < 12; i++)
	{
		bBuf[0] = SOP;
		bBuf[1] = cCommandSetIconI;
		bBuf[2] = 17 + i;
		bBuf[3] = aBuf[i];
		bBuf[4] = EOP;
		res = nuvotonWriteCommand(bBuf, 5, 0);
	}
	dprintk(100, "%s <\n", __func__);
	return res;
}
#elif defined(ATEVIO7500) // 13 character dot matrix VFD without colons or icons, leftmost character not used
int nuvotonWriteString(unsigned char *aBuf, int len)
{
	unsigned char bBuf[128];
	int i = 0;
	int j = 0;
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(bBuf, ' ', 128);

	/* start of command write */
	bBuf[0] = SOP;
	bBuf[1] = cCommandSetVFD;
	bBuf[2] = 0x11;

	/* save last string written to fp */
	memcpy(&lastdata.data, aBuf, 128);
	lastdata.length = len;

	dprintk(70, "len %d\n", len);

	while ((i < len) && (j < 12))
	{
		if (aBuf[i] < 0x80) //if normal ASCII
		{
			bBuf[j + 3] = aBuf[i]; //stuff buffer from offset 3 on
		}
		else if (aBuf[i] < 0xE0)
		{
			switch (aBuf[i])
			{
				case 0xc2:
				{
					UTF_Char_Table = UTF_C2;
					break;
				}
				case 0xc3:
				{
					UTF_Char_Table = UTF_C3;
					break;
				}
				case 0xc4:
				{
					UTF_Char_Table = UTF_C4;
					break;
				}
				case 0xc5:
				{
					UTF_Char_Table = UTF_C5;
					break;
				}
				case 0xd0:
				{
					UTF_Char_Table = UTF_D0;
					break;
				}
				case 0xd1:
				{
					UTF_Char_Table = UTF_D1;
					break;
				}
				default:
				{
					UTF_Char_Table = NULL;
				}
			}
			i++;
			if (UTF_Char_Table)
			{
				bBuf[j + 3] = UTF_Char_Table[aBuf[i] & 0x3f];
			}
			else
			{
				sprintf(&bBuf[j + 3], "%02x", aBuf[i - 1]);
				j += 2;
				bBuf[j + 3] = (aBuf[i] & 0x3f) | 0x40;
			}
		}
		else
		{
			if (aBuf[i] < 0xF0)
			{
				i += 2;
			}
			else if (aBuf[i] < 0xF8)
			{
				i += 3;
			}
			else if (aBuf[i] < 0xFC)
			{
				i += 4;
			}
			else
			{
				i += 5;
			}
			bBuf[j + 3] = 0x20; //else put a space
		}
		i++;
		j++;
	}
	/* end of command write, string must be padded with spaces */
	bBuf[15] = 0x20;
	bBuf[16] = EOP;

	res = nuvotonWriteCommand(bBuf, 17, 0);

	dprintk(100, "%s <\n", __func__);
	return res;
}
#else // not HS7119, HS7420, HS7429, HS7810A, HS7819, OCTAGON1008, FORTIS_HDBOX or ATEVIO7500 -> HS7110
int nuvotonWriteString(unsigned char *aBuf, int len)
{
	dprintk(100, "%s >\n", __func__);
	dprintk(100, "%s <\n", __func__);
	return -EFAULT;
}
#endif

#if !defined(ATEVIO7500)
int nuvoton_init_func(void)
{
	char standby_disable[] = {SOP, cCommandPowerOffReplay, 0x02, EOP};
	char init1[] = {SOP, cCommandSetBootOn, EOP};
	char init2[] = {SOP, cCommandSetTimeFormat, 0x81, EOP};       //set 24h clock format ?
	char init3[] = {SOP, cCommandSetWakeupTime, 0xff, 0xff, EOP}; /* delete/invalidate wakeup time ? */
	char init4[] = {SOP, cCommandSetLed, 0x01, 0x00, 0x08, EOP};  //power LED (red) off
#if defined(FORTIS_HDBOX)
	char init5[] = {SOP, cCommandSetLed, 0xf2, 0x0a, 0x00, EOP};  //blue LED plus cross brightness 10
#elif defined(HS7810A) || defined(HS7819) || defined(HS7420) || defined(HS7429)
	char init5[] = {SOP, cCommandSetLed, 0x02, 0x03, 0x00, EOP};  //logo brightness 3
#else //HS7110, HS7119, HS7420 & HS7429
	char init5[] = {SOP, cCommandSetLed, 0xff, 0x00, 0x00, EOP};  //all LEDs off (=green on)
#endif
	int  vLoop;
	int  res = 0;

	dprintk(100, "%s >\n", __func__);

	sema_init(&write_sem, 1);
#if defined(OCTAGON1008)
	printk("Fortis HS9510");
#elif defined(FORTIS_HDBOX)
	printk("Fortis FS9000/9200");
#elif defined(HS7110)
	printk("Fortis HS7110");
#elif defined(HS7119)
	printk("Fortis HS7119");
#elif defined(HS7420)
	printk("Fortis HS7420");
#elif defined(HS7429)
	printk("Fortis HS7429");
#elif defined(HS7810A)
	printk("Fortis HS7810");
#elif defined(HS7819)
	printk("Fortis HS7819");
#else
	printk("Fortis");
#endif
	printk(" VFD/Nuvoton module initializing.\n");
	/* must be called before standby_disable */
	res = nuvotonWriteCommand(init1, sizeof(init1), 0);  //SetBootOn

	/* setup: frontpanel should not power down the receiver if standby is selected */
	res = nuvotonWriteCommand(standby_disable, sizeof(standby_disable), 0);

	res |= nuvotonWriteCommand(init2, sizeof(init2), 0);  //SetTimeFormat
	res |= nuvotonWriteCommand(init3, sizeof(init3), 0);  //Clear wakeuptime
	res |= nuvotonWriteCommand(init4, sizeof(init4), 0);  //Standby LED off
	res |= nuvotonWriteCommand(init5, sizeof(init5), 0);  //Set power LED brightness 10

	for (vLoop = 0x00; vLoop < 0xff; vLoop++)
	{
		regs[vLoop] = 0x00;  //initialize local shadow registers
	}
#if !defined(HS7810A) && !defined(HS7819) && !defined(HS7110) && !defined(HS7119) //VFD models
	res |= nuvotonSetBrightness(7);
	res |= nuvotonWriteString("T.-Ducktales", strlen("T.-Ducktales"));
#elif defined(FORTIS_HDBOX) || defined(OCTAGON1008) //models with icons
	for (vLoop = ICON_MIN + 1; vLoop < ICON_MAX; vLoop++)
	{
		res |= nuvotonSetIcon(vLoop, 0); //switch all icons off
	}
#elif defined(HS7119) || defined(HS7810A) || defined(HS7819) //LED models
	res |= nuvotonWriteString("----", 4); //HS7810A, HS7819 & HS7119: show 4 dashes
#endif
	dprintk(100, "%s <\n", __func__);
	return 0;
}
#else // Code for HS8200 (Atevio 7500)
int nuvoton_init_func(void)
{
	char standby_disable[] = {SOP, cCommandPowerOffReplay, 0x02, EOP};

	char init1[] = {SOP, cCommandSetIconI, 0x10, 0x00, EOP};      /* display cgram (what is this? does not work?) */
	char init2[] = {SOP, cCommandSetIrCode, 0x01, 0x02, 0xf9, 0x10, 0x0b, EOP};

	char init3[] = {SOP, cCommandSetTimeFormat, 0x81, EOP};       //set 24h mode?
	char init4[] = {SOP, cCommandSetWakeupTime, 0xff, 0xff, EOP}; /* delete/invalidate wakeup time ? */

	int  res = 0;

	dprintk(100, "%s >\n", __func__);

	sema_init(&write_sem, 1);

	printk("Fortis HS8200 VFD/Nuvoton module initializing\n");

	/* must be called before standby_disable */
	res = nuvotonWriteCommand(init1, sizeof(init1), 0);   //display cgram

	res = nuvotonWriteCommand(init2, sizeof(init2), 0);   //set IR command set

	/* setup: frontpanel should not power down the receiver if standby is selected */
	res = nuvotonWriteCommand(standby_disable, sizeof(standby_disable), 0);

	res |= nuvotonWriteCommand(init3, sizeof(init3), 0);  //set time format
	res |= nuvotonWriteCommand(init4, sizeof(init4), 0);  //clear wakeup time
	res |= nuvotonSetBrightness(7);

	res |= nuvotonWriteString("T.-Ducktales", strlen("T.-Ducktales"));

	dprintk(100, "%s <\n", __func__);
	return 0;
}
#endif

//code for writing to /dev/vfd
static void clear_display(void)
{
	unsigned char bBuf[12];
	int res = 0;

	dprintk(100, "%s >\n", __func__);

	memset(bBuf, ' ', 12);
	res = nuvotonWriteString(bBuf, DISP_SIZE);
}

static ssize_t NUVOTONdev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	char *kernel_buf;
	int minor, vLoop, res = 0;
	int llen;
	int pos;
	int offset = 0;
	char buf[64];

	dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	if ((len == 0) || (DISP_SIZE == 0))
	{
		return len;
	}

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

	/* do not write to the remote control */
	if (minor == FRONTPANEL_MINOR_RC)
	{
		return -EOPNOTSUPP;
	}
	kernel_buf = kmalloc(len, GFP_KERNEL);

	if (kernel_buf == NULL)
	{
		printk("%s return no mem<\n", __func__);
		return -ENOMEM;
	}

	copy_from_user(kernel_buf, buff, len);

	if (write_sem_down())
	{
		return -ERESTARTSYS;
	}

	llen = len;

#if defined(HS7119) || defined(HS7810A) || defined(HS7819)
	if (kernel_buf[2] == ':')
	{
		llen--; // correct scroll when 2nd char is a colon
	}
#endif
	if (llen >= 64) //do not display more than 64 characters
	{
		llen = 64;
	}

	/* Dagobert: echo add a \n which will not be counted as a char */
	/* Audioniek: Does not add a \n but compensates string length as not to count it */
	if (kernel_buf[len - 1] == '\n')
	{
		llen--;
	}

	if (llen <= DISP_SIZE) //no scroll
	{
		res = nuvotonWriteString(kernel_buf, llen);
	}
	else //scroll, display string is longer than display length
	{
		if (llen > DISP_SIZE)
		{
			memset(buf, ' ', sizeof(buf));
			offset = DISP_SIZE - 1;
			memcpy(buf + offset, kernel_buf, llen);
			llen += offset;
			buf[llen + DISP_SIZE] = 0;
		}
		else
		{
			memcpy(buf, kernel_buf, llen);
			buf[llen] = 0;
		}

		if (llen > DISP_SIZE)
		{
			char *b = buf;
			for (pos = 0; pos < llen; pos++)
			{
				res = nuvotonWriteString(b + pos, DISP_SIZE);
				// sleep 300 ms
				msleep(300);
			}
		}

		clear_display();

		if (llen > 0)
		{
			res = nuvotonWriteString(buf + offset, DISP_SIZE);
		}
	}
	kfree(kernel_buf);

	write_sem_up();

	dprintk(70, "%s < res=%d len=%d\n", __func__, res, len);

	if (res < 0)
	{
		return res;
	}
	else
	{
		return len;
	}
}

//code for read from /dev/vfd
static ssize_t NUVOTONdev_read(struct file *filp, char __user *buff, size_t len, loff_t *off)
{
	int minor, vLoop;

	dprintk(100, "%s > (len %d, offs %d)\n", __func__, len, (int) *off);

	if (len == 0)
	{
		return len;
	}

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
		int size = 0;
		unsigned char data[20];

		memset(data, 0, 20);

		getRCData(data, &size);

		if (size > 0)
		{
			if (down_interruptible(&FrontPanelOpen[minor].sem))
			{
				return -ERESTARTSYS;
			}

			copy_to_user(buff, data, size);
			up(&FrontPanelOpen[minor].sem);

			dprintk(100, "%s < %d\n", __func__, size);
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
	{
		len = lastdata.length;
	}

	/* fixme: needs revision because of utf8! */
	if (len > 16)
	{
		len = 16;
	}

	FrontPanelOpen[minor].read = len;
	copy_to_user(buff, lastdata.data, len);
	up(&FrontPanelOpen[minor].sem);
	dprintk(100, "%s < (len %d)\n", __func__, len);
	return len;
}

int NUVOTONdev_open(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(100, "%s >\n", __func__);

	/* needed! otherwise a racecondition can occur */
	if (down_interruptible(&write_sem))
	{
		return -ERESTARTSYS;
	}

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

int NUVOTONdev_close(struct inode *inode, struct file *filp)
{
	int minor;

	dprintk(100, "%s >\n", __func__);
	minor = MINOR(inode->i_rdev);
	dprintk(20, "close minor %d\n", minor);

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

static int NUVOTONdev_ioctl(struct inode *Inode, struct file *File, unsigned int cmd, unsigned long arg)
{
	static int mode = 0;
#if defined(FORTIS_HDBOX) || defined(OCTAGON1008)
	int icon_nr, on, i;
#endif
	struct nuvoton_ioctl_data *nuvoton = (struct nuvoton_ioctl_data *) arg;
	struct vfd_ioctl_data *data = (struct vfd_ioctl_data *) arg;
	int res = 0;

	dprintk(100, "%s > 0x%.8x\n", __func__, cmd);

	if (down_interruptible(&write_sem))
	{
		return -ERESTARTSYS;
	}

	switch (cmd)
	{
		case VFDSETMODE:
		{
			mode = nuvoton->u.mode.compat;
			break;
		}
		case VFDSETLED:
		{
			res = nuvotonSetLED(nuvoton->u.led.led_nr, nuvoton->u.led.level);
			break;
		}
		case VFDBRIGHTNESS:
		{
#if defined(FORTIS_HDBOX) \
|| defined(OCTAGON1008) \
|| defined(ATEVIO7500) \
|| defined(HS7420) \
|| defined(HS7429)
			if (mode == 0)
			{
				res = nuvotonSetBrightness(data->start_address);
			}
			else
			{
				if (nuvoton->u.brightness.level < 0)
				{
					nuvoton->u.brightness.level = 0;
				}
				else if (nuvoton->u.brightness.level > 7)
				{
					nuvoton->u.brightness.level = 7;
				}
				res = nuvotonSetBrightness(nuvoton->u.brightness.level);
			}
#endif //all other than Octagon1008, FORTIS_HDBOX, ATEVIO7500, HS7420 or HS7429
			mode = 0;
			break;
		}
		case VFDDRIVERINIT:
		{
			res = nuvoton_init_func();
			mode = 0;
			break;
		}
		case VFDICONDISPLAYONOFF:
		{
#if defined(FORTIS_HDBOX)
			if (mode == 0) //vfd mode
			{
				icon_nr = data->data[0];
				on = data->data[4];
				dprintk(5, "%s Set icon %d to %d (mode 0)\n", __func__, icon_nr, on);

				switch (icon_nr)
				{
					case 0x13:  //crypted
					{
						icon_nr = ICON_SCRAMBLED;
						break;
					}
					case 0x17:  //dolby
					{
						icon_nr = ICON_DOLBY;
						break;
					}
					case 0x15:  //MP3
					{
						icon_nr = ICON_MP3;
						break;
					}
					case 17:    //HD
					{
						icon_nr = ICON_HD;
						break;
					}
					case 30:    //record
					{
						icon_nr = ICON_REC;
						break;
					}
					case 26:    //seekable (play)
					{
						icon_nr = ICON_PLAY;
						break;
					}
					default:
					{
						break;
					}
				}
				if (on != 0)
				{
					on = 1;
				}
				if (icon_nr != ICON_MAX)
				{
					res = nuvotonSetIcon(icon_nr, on);
				}
				else
				{
					for (i = ICON_MIN + 1; i < ICON_MAX; i++)
					{
						res = nuvotonSetIcon(i, on);
					}
				}
			}
			else
			{
				//compatible mode
				dprintk(5, "%s Set icon %d to %d (mode 1)\n", __func__, nuvoton->u.icon.icon_nr, nuvoton->u.icon.on);
				if (nuvoton->u.icon.icon_nr != ICON_MAX)
				{
					res = nuvotonSetIcon(nuvoton->u.icon.icon_nr, nuvoton->u.icon.on);
				}
				else
				{
					for (i = ICON_MIN + 1; i <  ICON_MAX; i++)
					{
						res = nuvotonSetIcon(i, nuvoton->u.icon.on);
					}
				}
			}
#elif defined(OCTAGON1008)
			if (mode == 0) //vfd mode
			{
				icon_nr = data->data[0];
				on = data->data[4];
				dprintk(5, "%s Set icon %d to %d (mode 0)\n", __func__, icon_nr, on);

				switch (icon_nr)
				{
					case 0x13:  //crypted
					{
						icon_nr = ICON_CRYPTED;
						break;
					}
					case 0x17:  //dolby
					{
						icon_nr = ICON_DOLBY;
						break;
					}
					case 26:    //seekable (play)
					{
						icon_nr = ICON_PLAY;
						break;
					}
					case 30:    //record
					{
						icon_nr = ICON_REC;
						break;
					}
					default:
					{
						break;
					}
				}
				if (on != 0)
				{
					on = 1;
				}
				if (icon_nr != ICON_MAX)
				{
					res = nuvotonSetIcon(icon_nr, on);
				}
				else
				{
					for (i = ICON_MIN + 1; i < ICON_MAX; i++)
					{
						res = nuvotonSetIcon(i, on);
					}
				}
			}
			else
			{
				//compatible mode
				dprintk(5, "%s Set icon %d to %d (mode 1)\n", __func__, nuvoton->u.icon.icon_nr, nuvoton->u.icon.on);
				if (nuvoton->u.icon.icon_nr != ICON_MAX)
				{
					res = nuvotonSetIcon(nuvoton->u.icon.icon_nr, nuvoton->u.icon.on);
				}
				else
				{
					for (i = ICON_MIN + 1; i < ICON_MAX; i++)
					{
						res = nuvotonSetIcon(i, nuvoton->u.icon.on);
					}
				}
			}
#elif defined(ATEVIO7500)
			printk("Set icon not supported on HS8200.\n");
#endif //all other than Octagon1008, FORTIS_HDBOX and Atevio7500
			mode = 0;
			res = 0;
			break;
		}
		case VFDSTANDBY:
		{
			clear_display();
			dprintk(5, "Set standby mode, wake up time: (MJD= %d) - %02d:%02d:%02d (UTC)\n", (nuvoton->u.standby.time[0] & 0xFF) * 256 + (nuvoton->u.standby.time[1] & 0xFF), nuvoton->u.standby.time[2], nuvoton->u.standby.time[3], nuvoton->u.standby.time[4]);
			res = nuvotonSetStandby(nuvoton->u.standby.time);
			break;
		}
		case VFDSETTIME:
		{
			if (nuvoton->u.time.time != 0)
			{
				dprintk(5, "Set frontpanel time to (MJD=) %d - %02d:%02d:%02d (UTC)\n", (nuvoton->u.time.time[0] & 0xFF) * 256 + (nuvoton->u.time.time[1] & 0xFF), nuvoton->u.time.time[2], nuvoton->u.time.time[3], nuvoton->u.time.time[4]);
				res = nuvotonSetTime(nuvoton->u.time.time);
			}
			break;
		}
		case VFDGETTIME:
		{
			res = nuvotonGetTime();
			dprintk(5, "Get frontpanel time: (MJD=) %d - %02d:%02d:%02d (UTC)\n", (ioctl_data[0] & 0xFF) * 256 + (ioctl_data[1] & 0xFF), ioctl_data[2], ioctl_data[3], ioctl_data[4]);
			copy_to_user((void *)arg, &ioctl_data, 5);
			break;
		}
		case VFDGETWAKEUPMODE:
		{
			res = nuvotonGetWakeUpMode();
			copy_to_user((void *)arg, &ioctl_data, 1);
			break;
		}
		case VFDDISPLAYCHARS:
		{
#if !defined (HS7110)
			if (mode == 0)
			{
				dprintk(5, "Write string (mode 0): %s (length = %d)\n", data->data, data->length);
				res = nuvotonWriteString(data->data, data->length);
			}
			else
			{
				dprintk(5, "Write string (mode 1): not supported!\n");
			}
#endif
			mode = 0;
			break;
		}
		case VFDDISPLAYWRITEONOFF:
		{
			res = nuvotonSetDisplayOnOff(nuvoton->u.light.onoff);
			mode = 0;  //go back to vfd mode
			break;
		}
		case VFDSETTIMEFORMAT:
		{
//			if (nuvoton->u.timeformat.format != 0)
//			{
//				nuvoton->u.timeformat.format = 1;
//			}
			dprintk(5, "Set time format to: %d\n", nuvoton->u.timeformat.format);
			res = nuvotonSetTimeFormat(nuvoton->u.timeformat.format);
			mode = 0; //go back to vfd mode
			break;
		}
		case 0x5305:
		{
			mode = 0; //go back to vfd mode
			break;
		}
		case 0x5401:
		case VFDGETBLUEKEY:
		case VFDSETBLUEKEY:
		case VFDGETSTBYKEY:
		case VFDSETSTBYKEY:
		case VFDPOWEROFF:
		case VFDSETPOWERONTIME:
		case VFDGETVERSION:
		case VFDGETSTARTUPSTATE:
		case VFDSETTIME2:
		case VFDDISPLAYCLR:
		case VFDGETLOOPSTATE:
		case VFDSETLOOPSTATE:
		case VFDGETWAKEUPTIME:
		{
			printk("[nuvoton] Unsupported IOCTL 0x%x for this receiver.\n", cmd);
			mode = 0;
			break;
		}
		default:
		{
			printk("[nuvoton] Unknown IOCTL 0x%x.\n", cmd);
			mode = 0;
			break;
		}
	}
	up(&write_sem);
	dprintk(100, "%s <\n", __func__);
	return res;
}

struct file_operations vfd_fops =
{
	.owner   = THIS_MODULE,
	.ioctl   = NUVOTONdev_ioctl,
	.write   = NUVOTONdev_write,
	.read    = NUVOTONdev_read,
	.open    = NUVOTONdev_open,
	.release = NUVOTONdev_close
};
