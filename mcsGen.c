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

typedef union {
	unsigned short i;
	unsigned char c[2];
}splitUI_t;

uint8_t gEightBitSwapTable[256] = {
		0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
		0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
		0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
		0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
		0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
		0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
		0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
		0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
		0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
		0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
		0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
		0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
		0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
		0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
		0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
		0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
		0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
		0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
		0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
		0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
		0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
		0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
		0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
		0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
		0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
		0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
		0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
		0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
		0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
		0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
		0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
		0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};


uint8_t gBitAndByteSwappedBuf[1024] = {0};
char gMCSWriteBuf[1024] = {0};
char gMCSExtendedLinearAddressBuf[100] = {0};
char gInfoFromBitFile[4][100];
int gBitFileRemainingBytes = 0;
FILE *gBitFilePtr = NULL;
FILE *gMCSFilePtr = NULL;
int gBitFileSize = 0;
unsigned char gFilePath[128] = {0};
unsigned char gMCSFilePath[128] = {0};
uint8_t gFileDataBuf[16] = {0};

void bitAndByteSwap(uint8_t *aPayloadData, int32_t aPayloadSize)
{
    //Swap bytes and bits
    int i = 0;
    for(i=0; i<aPayloadSize; i++ )
    {
        if( ( i % 2) == 0 )
        {
            if( ( i+1 ) < aPayloadSize )
                gBitAndByteSwappedBuf[i+1] = gEightBitSwapTable[ aPayloadData[i] ];
            else
                gBitAndByteSwappedBuf[i] = gEightBitSwapTable[ aPayloadData[i] ];
        }
        else
            gBitAndByteSwappedBuf[i-1] = gEightBitSwapTable[ aPayloadData[i] ];
    }
}

int buildExtendedLinearAddress(char *aOutBuf, uint16_t aExtendedLinearAddress)
{
    uint8_t lChkSum = 0, lDataChkSum = 0;
    uint8_t lType = 4;
    uint8_t lDataLen = 2;
    uint16_t lAddr = 0;

    lChkSum += lDataLen + ( lAddr & 0xFF ) + ( (lAddr & 0xFF00 ) >> 8 )
               + lType + ( aExtendedLinearAddress & 0xFF ) + ( (aExtendedLinearAddress & 0xFF00 ) >> 8 );
    lDataChkSum = (~lChkSum) + 1;
    return sprintf( aOutBuf, ":%02X%04X%02X%04X%02X\r\n", lDataLen, lAddr, lType, aExtendedLinearAddress, lDataChkSum);
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

    sprintf(aOutBuf + lTotalBytesWritten ,"%02X\r\n",lDataChkSum);
    return lTotalBytesWritten + 4;
}


int getFileSize(FILE *aFilePtr)
{
	int lSize = 0;
	fseek(aFilePtr, 0, SEEK_END);
	lSize = ftell(aFilePtr);
	fseek(aFilePtr, 0, SEEK_SET);
	return lSize;
}

void parseBitFileHeader(void)
{
	int lSts = 0;
	int i = 0, j = 0;
	int lFieldNum = 0;
	uint8_t lBitfileHeaderStartPattern[9] = { 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x00};
	splitUI_t lSize;
	uint8_t lByte = 0;

	while( lFieldNum < 5 )
	{
		lSts = 1;
		fread(&lByte, 1, 1, gBitFilePtr);
		lSize.c[1] = lByte;
		fread(&lByte, 1, 1, gBitFilePtr);
		lSize.c[0] = lByte;

		for(j=0; j<lSize.i; j++ )
		{
			if( lFieldNum == 0 )
			{
				fread(&lByte, 1, 1, gBitFilePtr);
				if( lByte != lBitfileHeaderStartPattern[j] )
				{
					printf("Header error! %d %d\n", i, j);
					lSts = 0;
					break;
				}
			}
			else
			{
				fread(&lByte, 1, 1, gBitFilePtr);
				gInfoFromBitFile[lFieldNum-1][j] = lByte;
			}
		}

		if( lSts == 1 )
		{

			lFieldNum++;
			if( lFieldNum == 1 )
			{

				fread(&lByte, 1, 1, gBitFilePtr);
				fread(&lByte, 1, 1, gBitFilePtr);
				fread(&lByte, 1, 1, gBitFilePtr);

				if( lByte == 'a' )
				{

				}
			}
			else if( lFieldNum == 2 )
			{
				fread(&lByte, 1, 1, gBitFilePtr);

				if( lByte == 'b' )
				{

				}
			}
			else if( lFieldNum == 3 )
			{
				fread(&lByte, 1, 1, gBitFilePtr);

				if( lByte == 'c' )
				{

				}
			}
			else if( lFieldNum == 4 )
			{
				fread(&lByte, 1, 1, gBitFilePtr);

				if( lByte == 'd' )
				{

				}
			}
			else if( lFieldNum == 5 )
			{
				fread(&lByte, 1, 1, gBitFilePtr);	//Eat up 'e' field bytes to align to 0xFF
				fread(&lByte, 1, 1, gBitFilePtr);
				fread(&lByte, 1, 1, gBitFilePtr);
				fread(&lByte, 1, 1, gBitFilePtr);
				fread(&lByte, 1, 1, gBitFilePtr);
			}
		}
		else
			break;
	}
}

//char gTestPrintBuf[100] = {0};
//uint8_t gTestBuf[20] = {0x57, 0x6F, 0x77, 0x21, 0x20, 0x44, 0x69, 0x64, 0x20, 0x79, 0x6F, 0x75, 0x20, 0x72, 0x65, 0x61};
//uint8_t gTestBuf[20] = {0x02, 0x33, 0x7A};
int main(int argc, char *argv[])
{
    //int lBytes = 0;
    int i = 0;
    int lFileReadBytes = 0;
    int lExtendedLinearAddress = 0;
    int lDataRecAddress = 0;
    //lBytes = buildMCSDataRecord(gTestPrintBuf, gTestBuf, 3, 0x0030);
    //for(i=0; i<lBytes;i++)
    //    printf("%02x ",gTestPrintBuf[i]);
    //printf("%s",gTestPrintBuf);

    if( strcmp(argv[1], "-bitfile") == 0 )
	{
		for(i=0; argv[2][i] != '\0'; i++)
		{
			gFilePath[i] = argv[2][i];
		}
		gFilePath[i] = '\0';
	}

    if( strcmp(argv[3], "-mcsfile") == 0 )
	{
		for(i=0; argv[4][i] != '\0'; i++)
		{
			gMCSFilePath[i] = argv[4][i];
		}
		gMCSFilePath[i] = '\0';
	}
    printf("Input File path is [%s]\n", gFilePath);
    printf("Output File path is [%s]\n", gMCSFilePath);

	gBitFilePtr = fopen((const char *)gFilePath, "rb");
	gMCSFilePtr = fopen((const char *)gMCSFilePath, "w+");

    if( gMCSFilePtr )
    {
        if( gBitFilePtr )
        {
            printf("File opened successfuly!\n");
            gBitFileSize = getFileSize(gBitFilePtr);
            printf("File size in bytes = %d\n",gBitFileSize);
            parseBitFileHeader();

            printf("Bit file header info:\n");
            for(i=0; i<4;i++)
                printf("[%s]\n",&gInfoFromBitFile[i][0]);

            //Deduct the header size and send the bit file.
            gBitFileRemainingBytes = gBitFileSize - ftell(gBitFilePtr);
            printf("%d\n", gBitFileRemainingBytes);
            printf("\n\n");

            lExtendedLinearAddress = 0;
            buildExtendedLinearAddress(gMCSExtendedLinearAddressBuf, lExtendedLinearAddress);
            fprintf(gMCSFilePtr, "%s", gMCSExtendedLinearAddressBuf);
            while( gBitFileRemainingBytes )
            {
                lFileReadBytes = fread(gFileDataBuf, 1, 16, gBitFilePtr);
                bitAndByteSwap(gFileDataBuf, lFileReadBytes);
                buildMCSDataRecord( gMCSWriteBuf , gBitAndByteSwappedBuf , lFileReadBytes, lDataRecAddress );
                fprintf(gMCSFilePtr, "%s", gMCSWriteBuf );

                lDataRecAddress += lFileReadBytes;
                if( lDataRecAddress >= 64*1024 )
                {
                    lExtendedLinearAddress++;
                    buildExtendedLinearAddress(gMCSExtendedLinearAddressBuf, lExtendedLinearAddress);
                    fprintf(gMCSFilePtr, "%s", gMCSExtendedLinearAddressBuf );
                    lDataRecAddress = 0;
                }

                gBitFileRemainingBytes -= lFileReadBytes;
            }
            fclose(gBitFilePtr);
        }
        fclose(gMCSFilePtr);
	}

    return 0;
}
