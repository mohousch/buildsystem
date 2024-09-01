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


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/platform_device.h>

#include <asm/system.h>
#include <asm/io.h>

#include "cec_worker.h"
#include "cec_opcodes.h"
#include "cec_opcodes_def.h"
#include "cec_internal.h"
#include "cec_proc.h"
#include "cec_rc.h"
#include "cec_dev.h"
#include "cec_debug.h"

extern int activemode;

extern long stmhdmiio_get_cec_address(unsigned int *arg);

static unsigned char isFirstKiss = 0;

static unsigned char logicalDeviceTypeChoicesIndex = 0;


static const unsigned char logicalDeviceTypeChoices[][2] =
{
	{ 1 << DEVICE_TYPE_DVD, DEVICE_TYPE_DVD1 },
	{ 1 << DEVICE_TYPE_DVD, DEVICE_TYPE_DVD2 },
	{ 1 << DEVICE_TYPE_DVD, DEVICE_TYPE_DVD3 },
	{ 1 << DEVICE_TYPE_STB, DEVICE_TYPE_STB1 },
	{ 1 << DEVICE_TYPE_STB, DEVICE_TYPE_STB2 },
	{ 1 << DEVICE_TYPE_STB, DEVICE_TYPE_STB3 },
	{ 1 << DEVICE_TYPE_REC, DEVICE_TYPE_REC1 },
	{ 1 << DEVICE_TYPE_REC, DEVICE_TYPE_REC2 },
	{ 0xFF, DEVICE_TYPE_UNREG }
};

static unsigned char logicalDeviceType = DEVICE_TYPE_DVD1;
extern char *deviceName;
extern unsigned char deviceType;

static unsigned short ActiveSource = 0x0000;

////////////////////////////////////

unsigned char getIsFirstKiss(void)
{
	return isFirstKiss;
}

void setIsFirstKiss(unsigned char kiss)
{
	isFirstKiss = kiss;
	if (isFirstKiss == 0)
	{
		if (activemode)
			sendReportPhysicalAddress();
	}
}

unsigned char getLogicalDeviceType(void)
{
	return logicalDeviceType;
}

unsigned char getDeviceType(void)
{
	return deviceType;
}

unsigned short getPhysicalAddress(void)
{
	unsigned int value = 0;
	stmhdmiio_get_cec_address(&value);

	return value & 0xffff;
}

//=================

unsigned short getActiveSource(void)
{
	return ActiveSource;
}

void setActiveSource(unsigned short addr)
{
	dprintk(2, "FROM: %04x TO: %04x\n", ActiveSource, addr);
	if (ActiveSource != addr)
	{
		ActiveSource = addr;
		setUpdatedActiveSource();
	}
}

//-----------------------------------------

void parseMessage(unsigned char src, unsigned char dst, unsigned int len, unsigned char buf[])
{
#define NAME_SIZE 100
	char name[NAME_SIZE];
	unsigned char responseBuffer[SEND_BUF_SIZE];

	memset(name, 0, NAME_SIZE);
	memset(responseBuffer, 0, SEND_BUF_SIZE);

	switch (buf[0])
	{
		case FEATURE_ABORT:
			strcpy(name, "FEATURE_ABORT");
			strcat(name, ": ");
			switch (buf[2])
			{
				case ABORT_REASON_UNRECOGNIZED_OPCODE:
					strcat(name, "UNRECOGNIZED_OPCODE");
					break;
				case ABORT_REASON_NOT_IN_CORRECT_MODE:
					strcat(name, "NOT_IN_CORRECT_MODE");
					break;
				case ABORT_REASON_CANNOT_PROVIDE_SOURCE:
					strcat(name, "CANNOT_PROVIDE_SOURCE");
					break;
				case ABORT_REASON_INVALID_OPERAND:
					strcat(name, "INVALID_OPERAND");
					break;
				case ABORT_REASON_REFUSED:
					strcat(name, "REFUSED");
					break;
				default:
					break;
			}
			break;

		case ABORT_MESSAGE:
			strcpy(name, "ABORT_MESSAGE");
			break;

		case IMAGE_VIEW_ON:
			strcpy(name, "IMAGE_VIEW_ON");
			break;

		case RECORD_ON:
			strcpy(name, "RECORD_ON");
			break;

		case RECORD_STATUS:
			strcpy(name, "RECORD_STATUS");
			break;

		case RECORD_OFF:
			strcpy(name, "RECORD_OFF");
			break;

		case TEXT_VIEW_ON:
			strcpy(name, "TEXT_VIEW_ON");
			break;

		case RECORD_TV_SCREEN:
			strcpy(name, "RECORD_TV_SCREEN");
			break;

		case GIVE_DECK_STATUS:
			strcpy(name, "GIVE_DECK_STATUS");
			strcat(name, ": ");
			switch (buf[1])
			{
				case STATUS_REQUEST_ON:
					strcat(name, "STATUS_REQUEST_ON");
					break;
				case STATUS_REQUEST_OFF:
					strcat(name, "STATUS_REQUEST_OFF");
					break;
				case STATUS_REQUEST_ONCE:
					strcat(name, "STATUS_REQUEST_ONCE");
					break;
				default:
					break;
			}

			if (activemode)
			{
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
				responseBuffer[1] = DECK_STATUS;
				responseBuffer[2] = DECK_INFO_PLAY;
				sendMessage(3, responseBuffer);
			}
			break;

		case DECK_STATUS:
			strcpy(name, "DECK_STATUS");
			strcat(name, ": ");
			switch (buf[1])
			{
				case DECK_INFO_PLAY:
					strcat(name, "PLAY");
					break;
				case DECK_INFO_RECORD:
					strcat(name, "RECORD");
					break;
				case DECK_INFO_PLAY_REVERSE:
					strcat(name, "PLAY_REVERSE");
					break;
				case DECK_INFO_STILL:
					strcat(name, "STILL");
					break;
				case DECK_INFO_SLOW:
					strcat(name, "SLOW");
					break;
				case DECK_INFO_SLOW_REVERSE:
					strcat(name, "SLOW_REVERSE");
					break;
				case DECK_INFO_FAST_FORWARD:
					strcat(name, "FAST_FORWARD");
					break;
				case DECK_INFO_FAST_REVERSE:
					strcat(name, "FAST_REVERSE");
					break;
				case DECK_INFO_NO_MEDIA:
					strcat(name, "NO_MEDIA");
					break;
				case DECK_INFO_STOP:
					strcat(name, "STOP");
					break;
				case DECK_INFO_SKIP_FORWARD:
					strcat(name, "SKIP_FORWARD");
					break;
				case DECK_INFO_SKIP_REVERSE:
					strcat(name, "SKIP_REVERSE");
					break;
				case DECK_INFO_INDEX_SEARCH_FORWARD:
					strcat(name, "INDEX_SEARCH_FORWARD");
					break;
				case DECK_INFO_INDEX_SEARCH_REVERSE:
					strcat(name, "INDEX_SEARCH_REVERSE");
					break;
				case DECK_INFO_OTHER_STATUS:
					strcat(name, "OTHER_STATUS");
					break;

				default:
					break;
			}
			break;

		case SET_MENU_LANGUAGE:
			strcpy(name, "SET_MENU_LANGUAGE");
			strcat(name, ": ");
			strcat(name, (buf + 1));
			break;

		case STANDBY:
			strcpy(name, "STANDBY");
			setUpdatedStandby();
			break;

		case USER_CONTROL_PRESSED:
			strcpy(name, "USER_CONTROL_PRESSED ");
			strcat(name, ": ");
			switch (buf[1])
			{
				case USER_CONTROL_CODE_SELECT:
					strcat(name, "SELECT");
					break;
				case USER_CONTROL_CODE_UP:
					strcat(name, "UP");
					break;
				case USER_CONTROL_CODE_DOWN:
					strcat(name, "DOWN");
					break;
				case USER_CONTROL_CODE_LEFT:
					strcat(name, "LEFT");
					break;
				case USER_CONTROL_CODE_RIGHT:
					strcat(name, "RIGHT");
					break;
				case USER_CONTROL_CODE_RIGHTUP:
					strcat(name, "RIGHT-UP");
					break;
				case USER_CONTROL_CODE_RIGHTDOWN:
					strcat(name, "RIGHT-DOWN");
					break;
				case USER_CONTROL_CODE_LEFTUP:
					strcat(name, "LEFT-UP");
					break;
				case USER_CONTROL_CODE_LEFTDOWN:
					strcat(name, "LEFT-DOWN");
					break;
				case USER_CONTROL_CODE_EXIT:
					strcat(name, "EXIT");
					break;

				case USER_CONTROL_CODE_PREV_CHANNEL:
					strcat(name, "PREV_CHANNEL");
					break;
				case USER_CONTROL_CODE_EPG:
					strcat(name, "EPG");
					break;

				case USER_CONTROL_CODE_NUMBERS_0:
					strcat(name, "NUMBERS_0");
					break;
				case USER_CONTROL_CODE_NUMBERS_1:
					strcat(name, "NUMBERS_1");
					break;
				case USER_CONTROL_CODE_NUMBERS_2:
					strcat(name, "NUMBERS_2");
					break;
				case USER_CONTROL_CODE_NUMBERS_3:
					strcat(name, "NUMBERS_3");
					break;
				case USER_CONTROL_CODE_NUMBERS_4:
					strcat(name, "NUMBERS_4");
					break;
				case USER_CONTROL_CODE_NUMBERS_5:
					strcat(name, "NUMBERS_5");
					break;
				case USER_CONTROL_CODE_NUMBERS_6:
					strcat(name, "NUMBERS_6");
					break;
				case USER_CONTROL_CODE_NUMBERS_7:
					strcat(name, "NUMBERS_7");
					break;
				case USER_CONTROL_CODE_NUMBERS_8:
					strcat(name, "NUMBERS_8");
					break;
				case USER_CONTROL_CODE_NUMBERS_9:
					strcat(name, "NUMBERS_9");
					break;
				case USER_CONTROL_CODE_F1_BLUE:
					strcat(name, "F1_(BLUE)");
					break;
				case USER_CONTROL_CODE_F2_RED:
					strcat(name, "F2_(RED)");
					break;
				case USER_CONTROL_CODE_F3_GREEN:
					strcat(name, "F3_(GREEN)");
					break;
				case USER_CONTROL_CODE_F4_YELLOW:
					strcat(name, "F4_(YELLOW)");
					break;
				case USER_CONTROL_CODE_F5:
					strcat(name, "F5");
					break;
				case USER_CONTROL_CODE_PLAY:
					strcat(name, "PLAY");
					break;
				case USER_CONTROL_CODE_STOP:
					strcat(name, "STOP");
					break;
				case USER_CONTROL_CODE_PAUSE:
					strcat(name, "PAUSE");
					break;
				case USER_CONTROL_CODE_RECORD:
					strcat(name, "RECORD");
					break;
				case USER_CONTROL_CODE_REWIND:
					strcat(name, "REWIND");
					break;
				case USER_CONTROL_CODE_FASTFORWARD:
					strcat(name, "FASTFORWARD");
					break;

				case USER_CONTROL_CODE_FUNCTION_PLAY:
					strcat(name, "PLAY");
					break;
				case USER_CONTROL_CODE_FUNCTION_PAUSEPLAY:
					strcat(name, "PAUSE-PLAY");
					break;
				case USER_CONTROL_CODE_FUNCTION_RECORD:
					strcat(name, "RECORD");
					break;
				case USER_CONTROL_CODE_FUNCTION_PAUSERECORD:
					strcat(name, "PAUSE-RECORD");
					break;
				case USER_CONTROL_CODE_FUNCTION_STOP:
					strcat(name, "STOP");
					break;
				case USER_CONTROL_CODE_FUNCTION_MUTE:
					strcat(name, "MUTE");
					break;
				case USER_CONTROL_CODE_FUNCTION_RESTORE:
					strcat(name, "RESTORE");
					break;
				case USER_CONTROL_CODE_FUNCTION_TUNE:
					strcat(name, "TUNE");
					break;
				case USER_CONTROL_CODE_FUNCTION_SELECT_MEDIA:
					strcat(name, "SELECT_MEDIA");
					break;
				case USER_CONTROL_CODE_FUNCTION_SELECT_AV_INPUT:
					strcat(name, "SELECT_AV_INPUT");
					break;
				case USER_CONTROL_CODE_FUNCTION_SELECT_AUDIO_INPUT:
					strcat(name, "SELECT_AUDIO_INPUT");
					break;
				case USER_CONTROL_CODE_FUNCTION_POWER_TOGGLE:
					strcat(name, "POWER_TOGGLE");
					break;
				case USER_CONTROL_CODE_FUNCTION_POWER_OFF:
					strcat(name, "POWER_OFF");
					break;
				case USER_CONTROL_CODE_FUNCTION_POWER_ON:
					strcat(name, "POWER_ON");
					break;
				case USER_CONTROL_CODE_ROOT_MENU:
					strcat(name, "ROOT_MENU");
					break;
				case USER_CONTROL_CODE_CONTENTS_MENU:
					strcat(name, "CONTENTS_MENU");
					break;
				case USER_CONTROL_CODE_SETUP_MENU:
					strcat(name, "SETUP_MENU");
					break;
				case USER_CONTROL_CODE_PAGEUP:
					strcat(name, "PAGEUP");
					break;
				case USER_CONTROL_CODE_PAGEDOWN:
					strcat(name, "PAGEDOWN");
					break;
				case USER_CONTROL_CODE_INFO:
					strcat(name, "INFO");
					break;
				case USER_CONTROL_CODE_NEXT:
					strcat(name, "NEXT");
					break;
				case USER_CONTROL_CODE_LAST:
					strcat(name, "LAST");
					break;
				default:
					break;
			}

			if (activemode)
			{
				input_inject(buf[1], 1);
			}

			break;

		case USER_CONTROL_RELEASED:
			strcpy(name, "USER_CONTROL_RELEASED");

			if (activemode)
			{
				input_inject(0xFFFF, 0);
			}

			break;

		case GIVE_OSD_NAME:
			strcpy(name, "GIVE_OSD_NAME");
			if (activemode)
			{
				int len;
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
				responseBuffer[1] = SET_OSD_NAME;
				len = strlen(deviceName);
				if (len > CEC_MAX_DATA_LEN - 2)
					len = CEC_MAX_DATA_LEN - 2;
				strncpy(responseBuffer + 2, deviceName, len);
				sendMessage(len + 2, responseBuffer);
			}
			break;

		case SET_OSD_NAME:
			strcpy(name, "SET_OSD_NAME");
			strcat(name, ": ");
			strcat(name, (buf + 1));
			break;

		case SET_OSD_STRING:
			strcpy(name, "SET_OSD_STRING");
			strcat(name, ": ");
			strcat(name, (buf + 2));
			break;

		case ROUTING_CHANGE:
			strcpy(name, "ROUTING_CHANGE");
			setActiveSource((buf[3] << 8) + buf[4]);
			break;

		case ROUTING_INFORMATION:
			strcpy(name, "ROUTING_INFORMATION");
			setActiveSource((buf[1] << 8) + buf[2]);
			break;

		case ACTIVE_SOURCE:
			strcpy(name, "ACTIVE_SOURCE");
			setActiveSource((buf[1] << 8) + buf[2]);
			break;

		case GIVE_PHYSICAL_ADDRESS:
			strcpy(name, "GIVE_PHYSICAL_ADDRESS");
			sendReportPhysicalAddress();
			break;

		case REPORT_PHYSICAL_ADDRESS:
			strcpy(name, "REPORT_PHYSICAL_ADDRESS");
			break;

		case REQUEST_ACTIVE_SOURCE:
			strcpy(name, "REQUEST_ACTIVE_SOURCE");
			if (activemode)
			{
				unsigned short physicalAddress = getPhysicalAddress();
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
				responseBuffer[1] = ACTIVE_SOURCE;
				responseBuffer[2] = (((physicalAddress >> 12) & 0xf) << 4) + ((physicalAddress >> 8) & 0xf);
				responseBuffer[3] = (((physicalAddress >> 4) & 0xf) << 4) + ((physicalAddress >> 0) & 0xf);
				sendMessage(4, responseBuffer);
			}
			break;

		case SET_STREAM_PATH:
			strcpy(name, "SET_STREAM_PATH");
			if (activemode)
			{
				if (((buf[1] << 8) + buf[2]) == getPhysicalAddress()) // If we are the active source
				{
					unsigned short physicalAddress = getPhysicalAddress();
					responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
					responseBuffer[1] = ACTIVE_SOURCE;
					responseBuffer[2] = (((physicalAddress >> 12) & 0xf) << 4) + ((physicalAddress >> 8) & 0xf);
					responseBuffer[3] = (((physicalAddress >> 4) & 0xf) << 4) + ((physicalAddress >> 0) & 0xf);
					sendMessage(4, responseBuffer);
				}
			}
			break;

		case DEVICE_VENDOR_ID:
			strcpy(name, "DEVICE_VENDOR_ID");
			break;

		case VENDOR_COMMAND:
			strcpy(name, "VENDOR_COMMAND");
			if (activemode)
			{
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
				responseBuffer[1] = FEATURE_ABORT;
				responseBuffer[2] = VENDOR_COMMAND;
				responseBuffer[3] = ABORT_REASON_UNRECOGNIZED_OPCODE;
				sendMessage(4, responseBuffer);
			}
			break;

		case VENDOR_REMOTE_BUTTON_DOWN:
			strcpy(name, "VENDOR_REMOTE_BUTTON_DOWN");
			break;

		case GIVE_DEVICE_VENDOR_ID:
			strcpy(name, "GIVE_DEVICE_VENDOR_ID");
			if (activemode)
			{
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
				responseBuffer[1] = DEVICE_VENDOR_ID;
				// http://standards.ieee.org/develop/regauth/oui/oui.txt
#if defined(UFS912) || defined(UFS913) // Kathrein
				responseBuffer[2] = 0x00;
				responseBuffer[3] = 0xD0;
				responseBuffer[4] = 0x55;
#else
#ifdef ATEVIO7500
				responseBuffer[2] = 0x00;
				responseBuffer[3] = 0x1E;
				responseBuffer[4] = 0xB8;
#else
				responseBuffer[2] = 'D';
				responseBuffer[3] = 'B';
				responseBuffer[4] = 'X';
#endif
#endif
				sendMessage(5, responseBuffer);
			}
			break;

		case MENU_REQUEST:
			strcpy(name, "MENU_REQUEST");
			strcat(name, ": ");
			switch (buf[1])
			{
				case MENU_REQUEST_TYPE_ACTIVATE:
					strcat(name, "ACTIVATE");
					break;
				case MENU_REQUEST_TYPE_DEACTIVATE:
					strcat(name, "DEACTIVATE");
					break;
				case MENU_REQUEST_TYPE_QUERY:
					strcat(name, "QUERY");
					break;
				default:
					break;
			}

			if (activemode)
			{
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
				responseBuffer[1] = MENU_STATUS;
				responseBuffer[2] = MENU_STATE_ACTIVATE;
				sendMessage(3, responseBuffer);
			}
			break;

		case MENU_STATUS:
			strcpy(name, "MENU_STATUS");
			strcat(name, ": ");
			switch (buf[1])
			{
				case MENU_STATE_ACTIVATE:
					strcat(name, "ACTIVATE");
					break;
				case MENU_STATE_DEACTIVATE:
					strcat(name, "DEACTIVATE");
					break;
				default:
					break;
			}
			break;

		case GIVE_DEVICE_POWER_STATUS:
			strcpy(name, "GIVE_DEVICE_POWER_STATUS");
			if (activemode)
			{
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
				responseBuffer[1] = REPORT_POWER_STATUS;
				responseBuffer[2] = POWER_STATUS_ON;
				sendMessage(3, responseBuffer);
			}
			break;

		case REPORT_POWER_STATUS:
			strcpy(name, "REPORT_POWER_STATUS");
			strcat(name, ": ");
			switch (buf[1])
			{
				case POWER_STATUS_ON:
					strcat(name, "ON");
					break;
				case POWER_STATUS_STANDBY:
					strcat(name, "STANDBY");
					break;
				case POWER_STATUS_GOING_TO_ON:
					strcat(name, "GOING_TO_ON");
					break;
				case POWER_STATUS_GOING_TO_STANDBY:
					strcat(name, "GOING_TO_STANDBY");
					break;
				default:
					break;
			}
			break;

		case GIVE_MENU_LANGUAGE:
			strcpy(name, "SET_STREAM_PATH");
			break;

		case SET_AUDIO_RATE:
			strcpy(name, "SET_AUDIO_RATE");
			strcat(name, ": ");
			switch (buf[1])
			{
				case MENU_STATE_ACTIVATE:
					strcat(name, "ACTIVATE");
					break;
				case MENU_STATE_DEACTIVATE:
					strcat(name, "DEACTIVATE");
					break;
				default:
					break;
			}
			break;

		case CEC_VERSION:
			strcpy(name, "CEC_VERSION");
			strcat(name, ": ");
			switch (buf[1])
			{
				case CEC_VERSION_V11:
					strcat(name, "1.1");
					break;
				case CEC_VERSION_V12:
					strcat(name, "1.2");
					break;
				case CEC_VERSION_V12A:
					strcat(name, "1.2a");
					break;
				case CEC_VERSION_V13:
					strcat(name, "1.3");
					break;
				case CEC_VERSION_V13A:
					strcat(name, "1.3a");
					break;
				case CEC_VERSION_V14:
					strcat(name, "1.4");
					break;
				default:
					break;
			}
			break;

		case GET_CEC_VERSION:
			strcpy(name, "GET_CEC_VERSION");
			if (activemode)
			{
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
				responseBuffer[1] = CEC_VERSION;
				responseBuffer[2] = CEC_VERSION_V13A;
				sendMessage(3, responseBuffer);
			}
			break;

		case VENDOR_COMMAND_WITH_ID:
			strcpy(name, "VENDOR_COMMAND_WITH_ID");
			if (activemode)
			{
				responseBuffer[0] = (getLogicalDeviceType() << 4) + (src & 0xF);
				responseBuffer[1] = FEATURE_ABORT;
				responseBuffer[2] = VENDOR_COMMAND_WITH_ID;
				responseBuffer[3] = ABORT_REASON_UNRECOGNIZED_OPCODE;
				sendMessage(4, responseBuffer);
			}
			break;

		default:
			strcpy(name, "UNKNOWN");
			break;
	}

	dprintk(4, "\tis %s\n", name);
}

void parseRawMessage(unsigned int len, unsigned char buf[])
{
	int ic;

	// Header
	unsigned char src = buf[0] >> 4;
	unsigned char dst = buf[0] & 0x0F;

	unsigned char dataLen = len - 1;

	if (dataLen > CEC_MAX_DATA_LEN)
	{
		dprintk(0, "Incoming Message was too long! (%u)\n", dataLen);
		return;
	}

	dprintk(2, "\tFROM 0x%02x TO 0x%02x : %3u : ", src, dst, dataLen);
	for (ic = 0; ic < dataLen; ic++)
		dprintk(2, "%02x ", buf[ic + 1]);
	dprintk(2, "\n");

	if (dataLen > 0)
	{
		parseMessage(src, dst, dataLen, buf + 1);
		if (!activemode)
			AddMessageToBuffer(buf, len);
	}
	else
	{
		dprintk(4, "\tis PING\n");
		//Lets check if the ping was send from or id, if so this means that
		//the deviceId pinged ist already been taken
		if (src == dst && src == getLogicalDeviceType())
		{
			if (getLogicalDeviceType() != DEVICE_TYPE_UNREG)
				sendPingWithAutoIncrement();
		}
	}
}

//================
// Higher Level Functions

void sendReportPhysicalAddress(void)
{
	unsigned short physicalAddress = getPhysicalAddress();
	unsigned char responseBuffer[SEND_BUF_SIZE];
	memset(responseBuffer, 0, SEND_BUF_SIZE);

	responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
	responseBuffer[1] = REPORT_PHYSICAL_ADDRESS;
	responseBuffer[2] = (((physicalAddress >> 12) & 0xf) << 4) + ((physicalAddress >> 8) & 0xf);
	responseBuffer[3] = (((physicalAddress >> 4) & 0xf) << 4)  + ((physicalAddress >> 0) & 0xf);
	responseBuffer[4] = getDeviceType();
	sendMessage(5, responseBuffer);
}

void setSourceAsActive(void)
{
	unsigned short physicalAddress = getPhysicalAddress();
	unsigned char responseBuffer[SEND_BUF_SIZE];
	memset(responseBuffer, 0, SEND_BUF_SIZE);

	responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
	responseBuffer[1] = ACTIVE_SOURCE;
	responseBuffer[2] = (((physicalAddress >> 12) & 0xf) << 4) + ((physicalAddress >> 8) & 0xf);
	responseBuffer[3] = (((physicalAddress >> 4) & 0xf) << 4)  + ((physicalAddress >> 0) & 0xf);
	sendMessage(4, responseBuffer);
}

void wakeTV(void)
{
	unsigned char responseBuffer[SEND_BUF_SIZE];
	memset(responseBuffer, 0, SEND_BUF_SIZE);

	responseBuffer[0] = (getLogicalDeviceType() << 4) + (DEVICE_TYPE_TV & 0xF);
	responseBuffer[1] = IMAGE_VIEW_ON;
	sendMessage(2, responseBuffer);
}

void sendInitMessage(void)
{
	unsigned char responseBuffer[SEND_BUF_SIZE];
	memset(responseBuffer, 0, SEND_BUF_SIZE);
	// Determine if TV is On

	responseBuffer[0] = (getLogicalDeviceType() << 4) + (DEVICE_TYPE_TV & 0xF);
	responseBuffer[1] = GIVE_DEVICE_POWER_STATUS;
	sendMessage(2, responseBuffer);
}

void sendPingWithAutoIncrement(void)
{
	unsigned char responseBuffer[1];
	char ldt = 1 << deviceType;

	dprintk(1, "%s - 1\n", __func__);
	// advance to the first matching device type
	while (!(ldt & logicalDeviceTypeChoices[logicalDeviceTypeChoicesIndex][0]))
		logicalDeviceTypeChoicesIndex++;
	dprintk(1, "sendPingWithAutoIncrement - 1\n");
	setIsFirstKiss(1);

	logicalDeviceType = logicalDeviceTypeChoices[logicalDeviceTypeChoicesIndex++][1];
	responseBuffer[0] = (logicalDeviceType << 4) + (logicalDeviceType & 0xF);
	dprintk(1, "%s - 2\n", __func__);
	sendMessage(1, responseBuffer);
	dprintk(1, "%s - 3\n", __func__);
	// advance to the next matching device type
	while (!(ldt & logicalDeviceTypeChoices[logicalDeviceTypeChoicesIndex][0]))
		logicalDeviceTypeChoicesIndex++;
}

void sendOneTouchPlay(void)
{
	unsigned short physicalAddress = getPhysicalAddress();
	unsigned char responseBuffer[SEND_BUF_SIZE];
	memset(responseBuffer, 0, SEND_BUF_SIZE);

	dprintk(2, "sendOneTouchPlay - 1\n");
	responseBuffer[0] = (getLogicalDeviceType() << 4) + (DEVICE_TYPE_TV & 0xF);
	responseBuffer[1] = IMAGE_VIEW_ON;
	dprintk(2, "sendOneTouchPlay - 2\n");
	sendMessage(2, responseBuffer);
	dprintk(2, "sendOneTouchPlay - 3\n");
	udelay(10000);
	dprintk(2, "sendOneTouchPlay - 4\n");
	memset(responseBuffer, 0, SEND_BUF_SIZE);

	responseBuffer[0] = (getLogicalDeviceType() << 4) + (BROADCAST & 0xF);
	responseBuffer[1] = ACTIVE_SOURCE;
	responseBuffer[2] = (((physicalAddress >> 12) & 0xf) << 4) + ((physicalAddress >> 8) & 0xf);
	responseBuffer[3] = (((physicalAddress >> 4) & 0xf) << 4)  + ((physicalAddress >> 0) & 0xf);
	dprintk(2, "sendOneTouchPlay - 5\n");
	sendMessage(4, responseBuffer);
	dprintk(2, "sendOneTouchPlay - 6\n");
}

void sendSystemStandby(int deviceId)
{
	unsigned char responseBuffer[SEND_BUF_SIZE];
	memset(responseBuffer, 0, SEND_BUF_SIZE);

	responseBuffer[0] = (getLogicalDeviceType() << 4) + (deviceId & 0xF);
	responseBuffer[1] = STANDBY;
	sendMessage(2, responseBuffer);
}
