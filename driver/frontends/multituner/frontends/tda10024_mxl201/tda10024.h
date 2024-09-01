/*
  TDA10024  - DVB-C Demodulator

  Copyright (C) 2011 duckbox

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define TDA10024_TS_PARALLEL 0
#define TDA10024_TS_SERIAL   1
#define TDA10024_TS_OFF      ((u32) (~0))


#define FE_SPECTRUMNORMAL    0 
#define FE_SPECTRUMINVERTED  1
#define FE_SPECTRUMAUTO      2 


#define TDA10024_POWER_D0    0
#define TDA10024_POWER_D3    3
//vim:ts=4