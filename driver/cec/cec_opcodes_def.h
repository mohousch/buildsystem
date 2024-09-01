/*
 *
 * (c) 2010 konfetti, schischu
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

#ifndef CEC_OPCODES_DEF_H_
#define CEC_OPCODES_DEF_H_

// http://www.cec-o-matic.com/

#define BROADCAST 0xf
#define SCREEN 0x0

#define ABORT_REASON_UNRECOGNIZED_OPCODE    0x00
#define ABORT_REASON_NOT_IN_CORRECT_MODE    0x01
#define ABORT_REASON_CANNOT_PROVIDE_SOURCE  0x02
#define ABORT_REASON_INVALID_OPERAND        0x03
#define ABORT_REASON_REFUSED                0x04

#define ANALOGUE_BROADCAST_TYPE_CABLE       0x00
#define ANALOGUE_BROADCAST_TYPE_SATELLITE   0x01
#define ANALOGUE_BROADCAST_TYPE_TERRESTRIAL 0x02

#define AUDIO_RATE_RATE_CONTROL_OFF  0x00
#define AUDIO_RATE_WRC_STANDARD_RATE 0x01
#define AUDIO_RATE_WRC_FAST_RATE     0x02
#define AUDIO_RATE_WRC_SLOW_RATE     0x03
#define AUDIO_RATE_NRC_STANDARD_RATE 0x04
#define AUDIO_RATE_NRC_FAST_RATE     0x05
#define AUDIO_RATE_NRC_SLOW_RATE     0x06

#define AUDIO_STATUS_AUDIO_MUTE_STATUS   0x08
#define AUDIO_STATUS_AUDIO_VOLUME_STATUS 0x07

#define AUDIO_MUTE_STATUS_AUDIO_MUTE_OFF 0x00
#define AUDIO_MUTE_STATUS_AUDIO_MUTE_ON  0x01

#define CEC_VERSION_V11   0x00
#define CEC_VERSION_V12   0x01
#define CEC_VERSION_V12A  0x02
#define CEC_VERSION_V13   0x03
#define CEC_VERSION_V13A  0x04
#define CEC_VERSION_V14   0x05

#define DECK_CONTROL_MODE_SKIP_FORWARD 0x01
#define DECK_CONTROL_MODE_SKIP_REVERSE 0x02
#define DECK_CONTROL_MODE_STOP         0x03
#define DECK_CONTROL_MODE_EJECT        0x04

#define DECK_INFO_PLAY 0x11
#define DECK_INFO_RECORD 0x12
#define DECK_INFO_PLAY_REVERSE 0x13
#define DECK_INFO_STILL 0x14
#define DECK_INFO_SLOW 0x15
#define DECK_INFO_SLOW_REVERSE 0x16
#define DECK_INFO_FAST_FORWARD 0x17
#define DECK_INFO_FAST_REVERSE 0x18
#define DECK_INFO_NO_MEDIA 0x19
#define DECK_INFO_STOP 0x1A
#define DECK_INFO_SKIP_FORWARD 0x1B
#define DECK_INFO_SKIP_REVERSE 0x1C
#define DECK_INFO_INDEX_SEARCH_FORWARD 0x1D
#define DECK_INFO_INDEX_SEARCH_REVERSE 0x1E
#define DECK_INFO_OTHER_STATUS 0x1F

#define DEVICE_TYPE_TV     0x00
#define DEVICE_TYPE_REC    0x01
#define DEVICE_TYPE_STB    0x03
#define DEVICE_TYPE_DVD    0x04
#define DEVICE_TYPE_AUDIO  0x05

#define DEVICE_TYPE_REC1   0x01
#define DEVICE_TYPE_REC2   0x02
#define DEVICE_TYPE_STB1   0x03
#define DEVICE_TYPE_DVD1   0x04
#define DEVICE_TYPE_AUDIO  0x05
#define DEVICE_TYPE_STB2   0x06
#define DEVICE_TYPE_STB3   0x07
#define DEVICE_TYPE_DVD2   0x08
#define DEVICE_TYPE_REC3   0x09
#define DEVICE_TYPE_STB4   0x0A
#define DEVICE_TYPE_DVD3   0x0B
#define DEVICE_TYPE_RES1    0x0C
#define DEVICE_TYPE_RES2    0x0D
#define DEVICE_TYPE_FREEUSE 0x0E
#define DEVICE_TYPE_UNREG   0x0F

#define DISPLAY_CONTROL_DEFAULT_TIME    0x00
#define DISPLAY_CONTROL_UNTIL_CLEARED   0x01
#define DISPLAY_CONTROL_CLEAR           0x02
#define DISPLAY_CONTROL_RESERVED        0x03

#define MENU_REQUEST_TYPE_ACTIVATE      0x00
#define MENU_REQUEST_TYPE_DEACTIVATE    0x01
#define MENU_REQUEST_TYPE_QUERY         0x02

#define MENU_STATE_ACTIVATE             0x00
#define MENU_STATE_DEACTIVATE           0x01

#define PHYSICAL_ADDRESS_MASK  0xFFFF

#define POWER_STATUS_ON                 0x00
#define POWER_STATUS_STANDBY            0x01
#define POWER_STATUS_GOING_TO_ON        0x02
#define POWER_STATUS_GOING_TO_STANDBY   0x03

#define STATUS_REQUEST_ON   0x01
#define STATUS_REQUEST_OFF  0x02
#define STATUS_REQUEST_ONCE 0x03

#define USER_CONTROL_CODE_SELECT        0x00
#define USER_CONTROL_CODE_UP            0x01
#define USER_CONTROL_CODE_DOWN          0x02
#define USER_CONTROL_CODE_LEFT          0x03
#define USER_CONTROL_CODE_RIGHT         0x04
#define USER_CONTROL_CODE_RIGHTUP       0x05
#define USER_CONTROL_CODE_RIGHTDOWN     0x06
#define USER_CONTROL_CODE_LEFTUP        0x07
#define USER_CONTROL_CODE_LEFTDOWN      0x08

#define USER_CONTROL_CODE_ROOT_MENU     0x09
#define USER_CONTROL_CODE_SETUP_MENU    0x0A
#define USER_CONTROL_CODE_CONTENTS_MENU 0x0B

#define USER_CONTROL_CODE_EXIT          0x0D
#define USER_CONTROL_CODE_NUMBERS_0     0x20
#define USER_CONTROL_CODE_NUMBERS_1     0x21
#define USER_CONTROL_CODE_NUMBERS_2     0x22
#define USER_CONTROL_CODE_NUMBERS_3     0x23
#define USER_CONTROL_CODE_NUMBERS_4     0x24
#define USER_CONTROL_CODE_NUMBERS_5     0x25
#define USER_CONTROL_CODE_NUMBERS_6     0x26
#define USER_CONTROL_CODE_NUMBERS_7     0x27
#define USER_CONTROL_CODE_NUMBERS_8     0x28
#define USER_CONTROL_CODE_NUMBERS_9     0x29

#define USER_CONTROL_CODE_PAGEUP        0x30
#define USER_CONTROL_CODE_PAGEDOWN      0x31

#define USER_CONTROL_CODE_PREV_CHANNEL  0x32
#define USER_CONTROL_CODE_INFO          0x35
#define USER_CONTROL_CODE_EPG           0x53

#define USER_CONTROL_CODE_F1_BLUE       0x71
#define USER_CONTROL_CODE_F2_RED        0x72
#define USER_CONTROL_CODE_F3_GREEN      0x73
#define USER_CONTROL_CODE_F4_YELLOW     0x74
#define USER_CONTROL_CODE_F5            0x75

#define USER_CONTROL_CODE_PLAY          0x44
#define USER_CONTROL_CODE_STOP          0x45
#define USER_CONTROL_CODE_PAUSE         0x46
#define USER_CONTROL_CODE_RECORD        0x47
#define USER_CONTROL_CODE_REWIND        0x48
#define USER_CONTROL_CODE_FASTFORWARD   0x49

#define USER_CONTROL_CODE_NEXT          0x4B
#define USER_CONTROL_CODE_LAST          0x4C

#define USER_CONTROL_CODE_FUNCTION_PLAY               0x60
#define USER_CONTROL_CODE_FUNCTION_PAUSEPLAY          0x61
#define USER_CONTROL_CODE_FUNCTION_RECORD             0x62
#define USER_CONTROL_CODE_FUNCTION_PAUSERECORD        0x63
#define USER_CONTROL_CODE_FUNCTION_STOP               0x64
#define USER_CONTROL_CODE_FUNCTION_MUTE               0x65
#define USER_CONTROL_CODE_FUNCTION_RESTORE            0x66
#define USER_CONTROL_CODE_FUNCTION_TUNE               0x67
#define USER_CONTROL_CODE_FUNCTION_SELECT_MEDIA       0x68
#define USER_CONTROL_CODE_FUNCTION_SELECT_AV_INPUT    0x69
#define USER_CONTROL_CODE_FUNCTION_SELECT_AUDIO_INPUT 0x6A
#define USER_CONTROL_CODE_FUNCTION_POWER_TOGGLE       0x6B
#define USER_CONTROL_CODE_FUNCTION_POWER_OFF          0x6C
#define USER_CONTROL_CODE_FUNCTION_POWER_ON           0x6D

// LG Vendor Specific

#define LG_DEVICE_TYPE_DVD               0x02
#define LG_DEVICE_TYPE_STB               0x03
#define LG_DEVICE_TYPE_HDD               0x04
#define LG_DEVICE_TYPE_BR                0x05
#define LG_DEVICE_TYPE_HOME_THEATRE      0x06

#define LG_REQUEST_DEVICE_TYPE           0x01
#define LG_REQUEST_REMOTE_CONTROL_LEVEL  0x0B
#define LG_REQUEST_VENDOR_COMMAND1       0x04
#define LG_REQUEST_VENDOR_COMMAND2       0xA0
#define LG_REQUEST_VENDOR_COMMAND3       0x03

// end LG Vendor Specific

typedef unsigned char  tAudioRate;
typedef unsigned char  tAbortReason;
typedef unsigned char  tDeviceType;
typedef unsigned char  tDisplayControl;
typedef unsigned char  tMenuRequestType;
typedef unsigned char  tMenuState;
typedef unsigned short tPhysicalAddress;
typedef unsigned char  tPowerStatus;
typedef unsigned char  tUiCommand;

#define FEATURE_ABORT           0x00
struct sFEATURE_ABORT
{
	unsigned char FeatureOpcode;
	tAbortReason AbortReason;
};

#define ABORT_MESSAGE           0xFF
struct sABORT_MESSAGE
{
};

#define IMAGE_VIEW_ON           0x04
struct sIMAGE_VIEW_ON
{
};

#define RECORD_ON               0x09
struct sRECORD_ON
{
	unsigned char RecordSource; //todo
};

#define RECORD_STATUS           0x0A
struct sRECORD_STATUS
{
	unsigned char RecordStatusInfo; //todo
};

#define RECORD_OFF              0x0B
struct sRECORD_OFF
{
};

#define TEXT_VIEW_ON            0x0D
struct sTEXT_VIEW_ON
{
};

#define RECORD_TV_SCREEN        0x0F
struct sRECORD_TV_SCREEN
{
};

#define GIVE_DECK_STATUS        0x1A
struct sGIVE_DECK_STATUS
{
	unsigned char StatusRequest; //todo
};

#define DECK_STATUS             0x1B
struct sDECK_STATUS
{
	unsigned char DeckInfo; //todo
};

#define SET_MENU_LANGUAGE       0x32
struct sSET_MENU_LANGUAGE
{
	char Language[3];
};

#define STANDBY                 0x36
struct sSTANDBY
{
};

#define USER_CONTROL_PRESSED    0x44
// Directly
struct sUSER_CONTROL_PRESSED
{
	tUiCommand UiCommand;
};

#define USER_CONTROL_RELEASED   0x45
// Directly
struct sUSER_CONTROL_RELEASED
{
};

#define GIVE_OSD_NAME           0x46
struct sGIVE_OSD_NAME
{
};

#define SET_OSD_NAME            0x47
struct sSET_OSD_NAME
{
	char OsdName[8];
};

#define SET_OSD_STRING          0x64
struct sSET_OSD_STRING
{
	tDisplayControl DisplayControl;
	char OsdString[13];
};

#define ROUTING_CHANGE          0x80
struct sROUTING_CHANGE
{
	tPhysicalAddress  OriginalAddress;
	tPhysicalAddress  NewAddress;
};

#define ROUTING_INFORMATION     0x81
struct sROUTING_INFORMATION
{
	tPhysicalAddress  PhysicalAddress;
};

#define ACTIVE_SOURCE           0x82
// Broadcast
struct sACTIVE_SOURCE
{
	tPhysicalAddress  PhysicalAddress;
};

#define GIVE_PHYSICAL_ADDRESS   0x83
struct sGIVE_PHYSICAL_ADDRESS
{
};

#define REPORT_PHYSICAL_ADDRESS 0x84
struct sREPORT_PHYSICAL_ADDRESS   // 2f 84 20 00 03
{
	tPhysicalAddress  PhysicalAddress;
	tDeviceType       DeviceType;
};

#define REQUEST_ACTIVE_SOURCE   0x85
struct sREQUEST_ACTIVE_SOURCE
{
};

#define SET_STREAM_PATH         0x86
// Broadcast
// -> ACTIVE_SOURCE
struct sSET_STREAM_PATH
{
	tPhysicalAddress  PhysicalAddress;
};

#define DEVICE_VENDOR_ID        0x87

//LG:          00 E0 91
//PANASONIC:   00 02 45
//PIONEER:     00 E0 36
//SAMSUNG:     00 00 F0
//SONY BRAVIA: 08 00 46

struct sDEVICE_VENDOR_ID
{
	unsigned char  VendorId[3];
};

#define VENDOR_COMMAND          0x89
struct sVENDOR_COMMAND
{
	unsigned char  VendorSpecificData[14];
};

#define VENDOR_REMOTE_BUTTON_DOWN   0x8A
struct sVENDOR_REMOTE_BUTTON_DOWN
{
	unsigned char VendorSpecificRCCode;
};

#define GIVE_DEVICE_VENDOR_ID   0x8C
struct sGIVE_DEVICE_VENDOR_ID
{
	unsigned char VendorId[3];
	unsigned char VendorSpecificData[14];
};

#define MENU_REQUEST   0x8D
// Directly
struct sMENU_REQUEST
{
	tMenuRequestType MenuRequestType;
};

#define MENU_STATUS   0x8E
// Directly
struct sMENU_STATUS
{
	tMenuState MenuState;
};

#define GIVE_DEVICE_POWER_STATUS    0x8F
// Directly
// -> REPORT_POWER_STATUS
struct sGIVE_DEVICE_POWER_STATUS
{
};

#define REPORT_POWER_STATUS     0x90
// Directly
struct sREPORT_POWER_STATUS
{
	tPowerStatus PowerStatus;
};

#define GIVE_MENU_LANGUAGE      0x91
struct sGIVE_MENU_LANGUAGE
{
};

#define SET_AUDIO_RATE      0x9a
struct sSET_AUDIO_RATE
{
	tAudioRate AudioRate;
};

#define CEC_VERSION      0x9e
struct sCEC_VERSION
{
};

#define GET_CEC_VERSION      0x9f
struct sGET_CEC_VERSION
{
};

#define VENDOR_COMMAND_WITH_ID          0xa0
struct sVENDOR_COMMAND_WITH_ID
{
	//unsigned char  VendorSpecificData[14];
};

#endif
