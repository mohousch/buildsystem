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

#ifndef CEC_OPCODES_H_
#define CEC_OPCODES_H_

unsigned char getLogicalDeviceType(void);

unsigned char getDeviceType(void);

unsigned short getPhysicalAddress(void);

unsigned short getActiveSource(void);
void setActiveSource(unsigned short addr);

unsigned char getIsFirstKiss(void);
void setIsFirstKiss(unsigned char kiss);

//===============

void parseMessage(unsigned char src, unsigned char dst, unsigned int len, unsigned char buf[]);
void parseRawMessage(unsigned int len, unsigned char buf[]);

//================
// Higher Level Functions

void sendReportPhysicalAddress(void);
void setSourceAsActive(void);
void sendInitMessage(void);
void sendPingWithAutoIncrement(void);
void sendOneTouchPlay(void);
void sendSystemStandby(int deviceId);
//================

#endif
