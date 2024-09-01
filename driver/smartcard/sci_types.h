/*
 *  Copyright (C) 2010 Duolabs Srl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _SCI_TYPES_H
#define _SCI_TYPES_H

/*****************************
 * DATA TYPES
 *****************************/

/* Architecture specific */
typedef int INT;
typedef unsigned int UINT;

typedef short int SHORT;
typedef unsigned short int USHORT;

typedef unsigned char BYTE;
typedef unsigned char UCHAR;
typedef signed char SBYTE;

typedef long LONG;
typedef unsigned long ULONG;

/* Try to be architecture independent */
typedef signed char INT8;
typedef unsigned char UINT8;

typedef short int INT16;
typedef unsigned short int UINT16;

typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned long U32;

/* Only gcc support these */
typedef long long int INT64;
typedef unsigned long long int UINT64;

#endif /* _SCI_TYPES_H */
