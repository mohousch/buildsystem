/*
 *   tools.c - audio/video switch tools
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
void set_bits(unsigned char *regs, int regIndex, unsigned char value, int start_bit, int nr_bits)
{
    regs[regIndex] = (regs[regIndex] & (~(((1 << nr_bits) - 1) << start_bit))) | (value << start_bit);
}

unsigned char get_bits(unsigned char *regs, int regIndex, int start_bit, int nr_bits)
{
    return ((regs[regIndex] >> start_bit) & ((1 << nr_bits) - 1));

}
