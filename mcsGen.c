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
#include <string.h>
#include "mcsGen.h"
#include "utils.h"
#include "atoh.h"



uint8_t gBitAndByteSwappedBuf[128] = {0};
char gMCSWriteBuf[128] = {0};
char gMCSExtendedLinearAddressBuf[100] = {0};
char gMCSEndOfFileBuf[20] = {0};

int gBitFileRemainingBytes = 0;

FILE *gBitFilePtr = NULL;
FILE *gMCSFilePtr = NULL;
int gBitFileSize = 0;

int gInputBitFilePathsBufIndex = 0;
int gBitFileStartAddressBufIndex = 0;

char gInputBitFilePathsBuf[MCS_GEN_MAX_SUPPORTED_BITFILES][128] = {{0}};
char gBitFileStartAddressBuf[MCS_GEN_MAX_SUPPORTED_BITFILES][10] = {{0}};
char gMCSFilePath[128] = {0};
uint8_t gFileDataBuf[16] = {0};
uint32_t gBitFileStartAddressTable[MCS_GEN_MAX_SUPPORTED_BITFILES] = {0};

extern char gInfoFromBitFile[4][100];

int buildExtendedLinearAddress(char *aOutBuf, uint16_t aExtendedLinearAddress)
{
    uint8_t lChkSum = 0, lDataChkSum = 0;
    uint8_t lType = 4;
    uint8_t lDataLen = 2;
    uint16_t lAddr = 0;
    int lTotalBytesWritten = 0;

    lChkSum += lDataLen + ( lAddr & 0xFF ) + ( (lAddr & 0xFF00 ) >> 8 )
               + lType + ( aExtendedLinearAddress & 0xFF ) + ( (aExtendedLinearAddress & 0xFF00 ) >> 8 );
    lDataChkSum = (~lChkSum) + 1;
    lTotalBytesWritten = sprintf( aOutBuf, ":%02X%04X%02X%04X%02X", lDataLen, lAddr, lType, aExtendedLinearAddress, lDataChkSum);

#if( MCS_GEN_XILINX_VIVADO_TYPE == 1 )
    aOutBuf[lTotalBytesWritten++] = 0x0D;
#endif
    aOutBuf[lTotalBytesWritten++] = 0x0A;
    return lTotalBytesWritten;
}

/**
https://en.wikipedia.org/wiki/Intel_HEX

For example, in the case of the record :0300300002337A1E, the sum of the decoded byte values is
03 + 00 + 30 + 00 + 02 + 33 + 7A = E2. The two's complement of E2 is 1E, which is the
checksum byte appearing at the end of the record.
 */

int buildMCSDataRecord(char *aOutBuf, uint8_t *aData, uint8_t aDataLen, uint16_t aDataRecordAddress)
{
    int i = 0;
    uint8_t lChkSum = 0, lDataChkSum = 0;
    uint8_t lType = 0;
    uint8_t lTotalBytesWritten = 0;
    uint8_t lBytesWritten = 0;

    lTotalBytesWritten = sprintf( aOutBuf, ":%02X%04X%02X", aDataLen, aDataRecordAddress, lType);
    for(i=0; i<aDataLen; i++)
    {
        lBytesWritten += sprintf(aOutBuf + lTotalBytesWritten + (i*2), "%02X",aData[i]);  //i*2 as 2 fields are used up
        lChkSum += aData[i];
    }
    lTotalBytesWritten += lBytesWritten;

    lChkSum += aDataLen + ( aDataRecordAddress & 0xFF ) + ( (aDataRecordAddress & 0xFF00 ) >> 8 ) + lType;
    lDataChkSum = (~lChkSum) + 1;

    lBytesWritten = sprintf(aOutBuf + lTotalBytesWritten ,"%02X",lDataChkSum);
    lTotalBytesWritten += lBytesWritten;

#if( MCS_GEN_XILINX_VIVADO_TYPE == 1 )
    aOutBuf[lTotalBytesWritten++] = 0x0D;
#endif
    aOutBuf[lTotalBytesWritten++] = 0x0A;
    return lTotalBytesWritten;
}

int buildEndOfFile(char *aOutBuf )
{
    uint8_t lChkSum = 0, lDataChkSum = 0;
    uint8_t lType = 1;
    uint8_t lDataLen = 0;
    uint16_t lAddr = 0;
    int lTotalBytesWritten = 0;

    lChkSum += lDataLen + ( lAddr & 0xFF ) + ( (lAddr & 0xFF00 ) >> 8 ) + lType;
    lDataChkSum = (~lChkSum) + 1;
    lTotalBytesWritten = sprintf( aOutBuf, ":%02X%04X%02X%02X", lDataLen, lAddr, lType, lDataChkSum);

    aOutBuf[lTotalBytesWritten++] = 0x0D;
    aOutBuf[lTotalBytesWritten++] = 0x0A;
    return lTotalBytesWritten;
}


int main(int argc, char *argv[])
{
    int lBytes = 0;
    int i = 0, j = 0;
    int lFileReadBytes = 0;
    int lExtendedLinearAddress = 0;
    int lDataRecAddress = 0;
    uint32_t lBitFileStartAddress = 0;


    if( argc >= 7 )
    {
        for(i=1; i<argc; i++ )
        {
            if( strcmp(argv[i], "-b") == 0 )
            {
                if( gInputBitFilePathsBufIndex < MCS_GEN_MAX_SUPPORTED_BITFILES )
                {
                    for(j=0; argv[i+1][j] != '\0'; j++)
                    {
                        gInputBitFilePathsBuf[gInputBitFilePathsBufIndex][j] = argv[i+1][j];
                    }
                    gInputBitFilePathsBuf[gInputBitFilePathsBufIndex][j] = '\0';
                    gInputBitFilePathsBufIndex++;
                }
            }
            else if( strcmp(argv[i], "-a") == 0 )
            {
                if( gBitFileStartAddressBufIndex < MCS_GEN_MAX_SUPPORTED_BITFILES )
                {
                    for(j=0; argv[i+1][j] != '\0'; j++)
                    {
                        gBitFileStartAddressBuf[gBitFileStartAddressBufIndex][j] = argv[i+1][j];
                    }
                    gBitFileStartAddressBuf[gBitFileStartAddressBufIndex][j] = '\0';
                    gBitFileStartAddressBufIndex++;
                }
            }
            else if( strcmp(argv[i], "-mcs") == 0 )
            {
                for(j=0; argv[i+1][j] != '\0'; j++)
                {
                    gMCSFilePath[j] = argv[i+1][j];
                }
                gMCSFilePath[j] = '\0';
            }
        }

        printf("Output File path is [%s]\n", gMCSFilePath);
        gMCSFilePtr = fopen((const char *)gMCSFilePath, "wb+");

        if( gMCSFilePtr )
        {
            printf("Found %d Files\n",gInputBitFilePathsBufIndex);
            for(i=0; i<gInputBitFilePathsBufIndex; i++ )
            {
                lDataRecAddress = 0;
                lBitFileStartAddress = atoh(&gBitFileStartAddressBuf[i][0], 10 );
                printf("Input File path is [%s] will be placed @ 0x%08x\n", &gInputBitFilePathsBuf[i][0], lBitFileStartAddress);
                gBitFilePtr = fopen((const char *)&gInputBitFilePathsBuf[i][0], "rb");
                if( gBitFilePtr )
                {
                    printf("File opened successfuly!\n");
                    gBitFileSize = getFileSize(gBitFilePtr);
                    printf("File size in bytes = %d\n",gBitFileSize);
                    parseBitFileHeader(gBitFilePtr);

                    printf("Bit file header info:\n");
                    for(j=0; j<4;j++)
                        printf("[%s]\n",&gInfoFromBitFile[j][0]);

                    /**Deduct the header size and send the bit file.*/
                    gBitFileRemainingBytes = gBitFileSize - ftell(gBitFilePtr);
                    printf("%d\n", gBitFileRemainingBytes);
                    printf("\n\n");

                    lExtendedLinearAddress = ( lBitFileStartAddress / SIXTEEN_BIT_MAX_VAL ) ;
                    lBytes = buildExtendedLinearAddress(gMCSExtendedLinearAddressBuf, lExtendedLinearAddress);
                    fwrite(gMCSExtendedLinearAddressBuf, 1, lBytes, gMCSFilePtr);

                    while( gBitFileRemainingBytes )
                    {
                        lFileReadBytes = fread(gFileDataBuf, 1, 16, gBitFilePtr);
                        bitAndByteSwap(gBitAndByteSwappedBuf, gFileDataBuf, lFileReadBytes);
                        lBytes = buildMCSDataRecord( gMCSWriteBuf , gBitAndByteSwappedBuf , lFileReadBytes, lDataRecAddress );
                        fwrite(gMCSWriteBuf, 1, lBytes, gMCSFilePtr);

                        lDataRecAddress += lFileReadBytes;
                        if( lDataRecAddress >= SIXTEEN_BIT_MAX_VAL )
                        {
                            lExtendedLinearAddress++;
                            lBytes = buildExtendedLinearAddress(gMCSExtendedLinearAddressBuf, lExtendedLinearAddress);
                            fwrite(gMCSExtendedLinearAddressBuf, 1, lBytes, gMCSFilePtr);
                            lDataRecAddress = 0;
                        }

                        gBitFileRemainingBytes -= lFileReadBytes;
                    }
                    fclose(gBitFilePtr);
                }
                else
                {
                    printf("File open error\n");
                }
            }
            lBytes = buildEndOfFile(gMCSEndOfFileBuf);
            fwrite(gMCSEndOfFileBuf, 1, lBytes, gMCSFilePtr);
            fclose(gMCSFilePtr);
        }
    }
    else
        printf("USAGE: mcsgen.exe -a 0xhhhhhhhhhh -b b.bit -a 0xhhhhhhhhhh -b b.bit -a 0xhhhhhhhhhh -b b.bit -mcs m.mcs\n");
    return 0;
}
