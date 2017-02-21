 /**
   Intel MCS format file generator which is used by Xilinx FPGAs
   Copyright (C) 2017 by Someshwar Mysore Sridharan <someshwar.ms@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//stops parsing, if a NULL or space or is encountered or entire length of the buffer is read
uint32_t atoh(char *aBuf, int aSize)
{
    int i = 0;
    uint32_t lValue = 0;
    unsigned char lChar = 0;

    for(i=0; ( (i < aSize ) && (aBuf[i] != '\0') && (aBuf[i] != ' ') ); i++)
    {
        if( (aBuf[i] >= '0') && ( aBuf[i] <= '9') )
        {
            lChar = aBuf[i] - '0';
        }
        else if( (aBuf[i] >= 'A') && ( aBuf[i] <= 'F') )
        {
            lChar = (aBuf[i] - 'A') + 10;
        }
        else if( (aBuf[i] >= 'a') && ( aBuf[i] <= 'f') )
        {
            lChar = (aBuf[i] - 'a') + 10;
        }
        lValue *= 16;
        lValue += lChar;
    }
    return lValue;
}
