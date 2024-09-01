/**************************************************************************/
/* Name :   fup                                                           */
/*                                                                        */
/* Author:  Schischu                                                      */
/*                                                                        */
/* Licence: This file is subject to the terms and conditions of the       */
/*          GNU General Public License version 2.                         */
/**************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <gcrypt.h> /* sha1 / crc32 */
#include <fcntl.h>

#include <zlib.h>

#include "crc16.h"
#include "dummy.h"

#define VERSION "1.7"
#define DATE "25.10.2011"

//#define USE_ZLIB

uint16_t blockCounter = 0;
uint16_t blockCounterTotal = 0;

#define MAX_PART_NUMBER 17
#define EXTENSION_LEN 32

#define PART_SIGN 1
#define PART_FLASH 2

#define DATA_BLOCKSIZE 0x7FFA

#if 0
struct
{
	uint32_t Id;
	char Extension[EXTENSION_LEN];
	char Description[EXTENSION_LEN];
	uint32_t Offset;
	uint32_t Size;
	uint32_t Flags;
} tPartition;

(0x0,  ".loader.mtd0",   "Loader", 0x00000000, 0x00100000, (PART_FLASH)),
(0x01,     ".app.mtd1",     "App", 0x00b00000, 0x00700000, (PART_FLASH | PART_SIGN)),
(0x02, ".config0.mtd5", "Config0", 0x02100000, 0x00040000, (PART_FLASH)),
(0x03, ".config4.mtd5", "Config4", 0x02140000, 0x00040000, (PART_FLASH)),
(0x04, ".config8.mtd5", "Config8", 0x02180000, 0x00020000, (PART_FLASH)),
(0x05, ".configA.mtd5", "ConfigA", 0x021A0000, 0x00020000, (PART_FLASH)),

(0x06,  ".kernel.mtd6",  "Kernel", 0x00100000, 0x00300000, (PART_FLASH)),
(0x07,     ".dev.mtd4",  "Loader", 0x00000000, 0x00100000, (PART_FLASH | PART_SIGN)),
(0x08,  ".rootfs.mtd8",  "Loader", 0x00000000, 0x00100000, (PART_FLASH | PART_SIGN)),
(0x09,    ".user.mtd9",  "Loader", 0x00000000, 0x00100000, (PART_FLASH)),
(0x10,           ".10",  "Loader", 0x00000000, 0x00100000, (PART_FLASH)),
#endif

uint8_t has[MAX_PART_NUMBER];
FILE *fd[MAX_PART_NUMBER];
char ext[MAX_PART_NUMBER][EXTENSION_LEN] =
{
	".loader.mtd0",
	".app.mtd1",
	".config0.mtd5", //F mtd5 offset 0x00000000
	".config4.mtd5", //E mtd5 offset 0x00040000
	".config8.mtd5",
	".configA.mtd5", //C mtd5 offset 0x000A0000
	".kernel.mtd6",
	".dev.mtd4",
	".rootfs.mtd8",
	".user.mtd9",
	".10",
	".11",
	".12",
	".13",
	".14",
	".15",
	".16"
};

#if 0
void fromint16_t(uint8_t **int16_tBuf, uint16_t val)
{
	*int16_tBuf[0] = val >> 8;
	*int16_tBuf[1] = val & 0xFF;
}
#endif

uint16_t toShort(uint8_t int16_tBuf[2])
{
	return (uint16_t)(int16_tBuf[1] + (int16_tBuf[0] << 8));
}

uint16_t extractShort(uint8_t dataBuf[], uint16_t pos)
{
	uint8_t int16_tBuf[2];
	memcpy(int16_tBuf, dataBuf + pos, 2);
	return toShort(int16_tBuf);
}

uint16_t readShort(FILE *file)
{
	uint8_t int16_tBuf[2];
	if (fread(int16_tBuf, 1, 2, file) == 2)
		return toShort(int16_tBuf);
	else return 0;
}

int32_t extractAndWrite(FILE *file, uint8_t *buffer, uint16_t len, uint16_t decLen)
{
#ifdef USE_ZLIB
	if (len != decLen)
	{
		//zlib
		z_stream strm;
		uint8_t out[decLen];
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		inflateInit(&strm);
		strm.avail_in = len;
		strm.next_in = buffer;
		strm.avail_out = decLen;
		strm.next_out = out;
		inflate(&strm, Z_NO_FLUSH);
		uint16_t have = decLen - strm.avail_out;
		inflateEnd(&strm);
		fwrite(out, 1, decLen, file);
		return decLen;
	}
	else
#endif
	{
		fwrite(buffer, 1, len, file);
		return len;
	}
}

uint16_t readAndCompress(FILE *file, uint8_t **dataBuf, uint16_t pos, uint16_t *uncompressedDataLen)
{
	uint16_t have = 0;
	*uncompressedDataLen = fread((*dataBuf) + pos, 1, *uncompressedDataLen, file);
#ifdef USE_ZLIB
	// So now we have to check if zlib can compress this or not
	z_stream strm;
	uint8_t in[*uncompressedDataLen];
	uint8_t out[*uncompressedDataLen];
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	deflateInit(&strm, Z_DEFAULT_COMPRESSION);
	strm.avail_in = *uncompressedDataLen;
	memcpy(in, (*dataBuf) + pos, *uncompressedDataLen);
	strm.next_in = in;
	//do {
	strm.avail_out = *uncompressedDataLen;
	strm.next_out = out + have;
	deflate(&strm, Z_FINISH);
	have = *uncompressedDataLen - strm.avail_out;
	deflateEnd(&strm);
	if (have < *uncompressedDataLen)
	{
		memcpy((*dataBuf) + pos, out, have);
	}
	else
#endif
	{
		have = *uncompressedDataLen;
	}
	return have;
}

uint16_t insertint16_t(uint8_t **dataBuf, uint16_t pos, uint16_t value)
{
	(*dataBuf)[pos] = value >> 8;
	(*dataBuf)[pos + 1] = value & 0xFF;
	return 2;
}

int32_t writeBlock(FILE *irdFile, FILE *file, uint8_t firstBlock, uint16_t type)
{
	uint16_t blockHeaderPos = 0;
	uint16_t nextBlockHeaderPos = 0;
	uint16_t compressedDataLen = 0;
	uint16_t uncompressedDataLen = DATA_BLOCKSIZE;
	uint16_t dataCrc = 0;
	uint8_t *blockHeader = (uint8_t *)malloc(4);
	uint8_t *dataBuf = (uint8_t *)malloc(DATA_BLOCKSIZE + 4);
	if (firstBlock && type == 0x10)
	{
		// header
		uint16_t resellerId = 0x2303;
		insertint16_t(&dataBuf, 0, type);
		insertint16_t(&dataBuf, 2, resellerId);
		insertint16_t(&dataBuf, 4, 0);
		insertint16_t(&dataBuf, 6, 0);
		insertint16_t(&dataBuf, 8, 0xFFFF);
		insertint16_t(&dataBuf, 10, 0);
		insertint16_t(&dataBuf, 12, 0);
		insertint16_t(&dataBuf, 14, 0);
		compressedDataLen = 12;
		uncompressedDataLen = compressedDataLen;
	}
	else
	{
		compressedDataLen = readAndCompress(file, &dataBuf, 4, &uncompressedDataLen);
		insertint16_t(&dataBuf, 0, type);
		insertint16_t(&dataBuf, 2, uncompressedDataLen);
	}
	dataCrc = crc16(dataCrc, dataBuf, compressedDataLen + 4);
	insertint16_t(&blockHeader, 0, compressedDataLen + 6);
	insertint16_t(&blockHeader, 2, dataCrc);
	fwrite(blockHeader, 1, 4, irdFile);
	fwrite(dataBuf, 1, compressedDataLen + 4, irdFile);
	free(blockHeader);
	free(dataBuf);
	return uncompressedDataLen;
}

int32_t readBlock(FILE *file, const char *name, uint8_t firstBlock)
{
	printf("%s:%s[%d]\n", __FILE__, __func__, __LINE__);
	uint16_t len = 0;
	uint16_t crc = 0;
	uint16_t type = 0;
	len = readShort(file);
	if (len == 0) return 0;
	crc = readShort(file);
	printf(" LEN  = %X (%d)\n", len, len);
	printf(" CRC  = %X\n", crc);
	uint8_t dataBuf[len - 2/*crc*/];
	if (fread(dataBuf, 1, len - 2/*crc*/, file) != len - 2/*crc*/)
		return 0;
	uint16_t dataCrc = 0;
	dataCrc = crc16(dataCrc, dataBuf, len - 2/*crc*/);
	printf("  CALC CRC = %04X\n", dataCrc);
	if (crc != dataCrc)
		getchar();
	type = extractShort(dataBuf, 0);
	printf("  TYPE = %X\n", type);
	blockCounterTotal++;
	if (firstBlock && type == 0x10)
	{
		printf("-> header\n");
		{
			uint16_t fpVersion = extractShort(dataBuf, 0);
			uint16_t systemId1 = extractShort(dataBuf, 2);
			uint16_t systemId2 = extractShort(dataBuf, 4);
			uint16_t blockcount1 = extractShort(dataBuf, 6);
			uint16_t blockcount2 = extractShort(dataBuf, 8);
			uint16_t softwareversion1 = extractShort(dataBuf, 10);
			uint16_t softwareversion2 = extractShort(dataBuf, 12);
			uint16_t softwareversion3 = extractShort(dataBuf, 14);
			printf("  fpVersion: %04X systemId: %04X%04X blockcount: %04X%04X\n", fpVersion, systemId1, systemId2, blockcount1, blockcount2);
			printf("  softwareVersion: %04X%04X%04X\n", softwareversion1, softwareversion2, softwareversion3);
			//[2]
			//[4]
		}
	}
	else
	{
		if (type < MAX_PART_NUMBER)
		{
			if (!has[type])
			{
				blockCounter = 1;
				has[type] = 1;
				char nameOut[strlen(name) + 1 + strlen(ext[type])];
				strncpy(nameOut, name, strlen(name));
				strncpy(nameOut + strlen(name), ext[type], strlen(ext[type]));
				nameOut[strlen(name) + strlen(ext[type])] = '\0';
				fd[type] = fopen(nameOut, "wb");
				printf("-> %s\n", nameOut);
			}
			printf("   %s (%d)\n", ext[type], blockCounter++);
			uint16_t decLen = extractShort(dataBuf, 2);
			printf("    LEN = %X (%d)\n", decLen, decLen);
			extractAndWrite(fd[type], dataBuf + 4, len - 6, decLen);
		}
		else
		{
			getchar();
		}
	}
	return len;
}

int32_t main(int32_t argc, char *argv[])
{
	FILE *file;
	int32_t pos = 0;
	uint8_t firstBlock = 1;
	if (argc == 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-s", 2) == 0)
	{
		uint32_t crc = 0;
		char signedFileName[128];
		strcpy(signedFileName, argv[2]);
		strcat(signedFileName, ".signed");
		FILE *signedFile = fopen(signedFileName, "wb");
		file = fopen(argv[2], "r");
		uint8_t buffer[0x2710];
		while (!feof(file))
		{
			int32_t count = fread(buffer, 1, 0x2710, file); //Actually it would be enpugh to fseek and only to read q byte.
			fwrite(buffer, 1, count, signedFile);
			crc = crc32(crc, buffer, 1/*0x2710*/);
		}
		printf("Signed footer: %08X\n", crc);
		fwrite(&crc, 1, 4, signedFile);
		fclose(file);
		fclose(signedFile);
	}
	else if (argc == 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-t", 2) == 0)
	{
		uint32_t crc = 0;
		uint32_t orgcrc = 0;
		file = fopen(argv[2], "r");
		uint8_t buffer[0x2710];
		while (!feof(file))  // Actually we would ned to remove the sign at the end
		{
			int32_t count = fread(buffer, 1, 0x2710, file);
			if (count != 0x2710)
			{
				orgcrc = (buffer[count - 1] << 24) + (buffer[count - 2] << 16) + (buffer[count - 3] << 8) + (buffer[count - 4]);
			}
			crc = crc32(crc, buffer, 1/*0x2710*/);
		}
		printf("Signed footer: %08X\n", crc);
		printf("Original Signed footer: %08X\n", orgcrc);
		fclose(file);
	}
	else if (argc == 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-x", 2) == 0)
	{
		for (int32_t i = 0; i < MAX_PART_NUMBER; i++)
		{
			has[i] = 0;
			fd[i] = NULL;
		}
		file = fopen(argv[2], "r");
		while (!feof(file))
		{
			pos = ftell(file);
			int32_t len = readBlock(file, argv[2], firstBlock);
			firstBlock = 0;
			if (len > 0)
			{
				pos += len + 2;
				fseek(file, pos, SEEK_SET);
			}
			else
			{
				break;
			}
		}
		fclose(file);
		for (int32_t i = 0; i < MAX_PART_NUMBER; i++)
		{
			if (fd[i] != NULL)
			{
				fclose(fd[i]);
			}
		}
	}
	else if (argc == 2 && strlen(argv[1]) == 2 && strncmp(argv[1], "-v", 2) == 0)
	{
		printf("Version: %s Date: %s\n", VERSION, DATE);
	}
	else if (argc >= 3 && strlen(argv[1]) == 2 && strncmp(argv[1], "-c", 2) == 0)
	{
		FILE *irdFile = fopen(argv[2], "w+");
		uint16_t totalBlockCount = 0;
		uint16_t headerDataBlockLen = 0;
		// Header
		headerDataBlockLen = writeBlock(irdFile, NULL, 1, 0x10);
		headerDataBlockLen += 4;
		uint8_t appendPartCount = argc - 3;
		for (int32_t i = 0; i < appendPartCount; i += 2)
		{
			uint8_t type = 0x01;
			if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-feelinglucky", 13) == 0)
			{
				type = 0x00;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-a", 2) == 0)
			{
				type = 0x01;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-1", 2) == 0)
			{
				type = 0x01;
			}
			else if (strlen(argv[3 + i]) == 3 && strncmp(argv[3 + i], "-c0", 3) == 0)
			{
				type = 0x02;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-2", 2) == 0)
			{
				type = 0x02;
			}
			else if (strlen(argv[3 + i]) == 3 && strncmp(argv[3 + i], "-c4", 3) == 0)
			{
				type = 0x03;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-3", 2) == 0)
			{
				type = 0x03;
			}
			else if (strlen(argv[3 + i]) == 3 && strncmp(argv[3 + i], "-c8", 3) == 0)
			{
				type = 0x04;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-4", 2) == 0)
			{
				type = 0x04;
			}
			else if (strlen(argv[3 + i]) == 3 && strncmp(argv[3 + i], "-ca", 3) == 0)
			{
				type = 0x05;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-5", 2) == 0)
			{
				type = 0x05;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-k", 2) == 0)
			{
				type = 0x06;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-6", 2) == 0)
			{
				type = 0x06;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-d", 2) == 0)
			{
				type = 0x07;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-7", 2) == 0)
			{
				type = 0x07;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-r", 2) == 0)
			{
				type = 0x08;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-8", 2) == 0)
			{
				type = 0x08;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-u", 2) == 0)
			{
				type = 0x09;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-9", 2) == 0)
			{
				type = 0x09;
			}
			else if (strlen(argv[3 + i]) == 3 && strncmp(argv[3 + i], "-10", 3) == 0)
			{
				type = 0x10;
			}
			FILE *file = fopen(argv[3 + i + 1], "rb");
			printf("Adding %s\n", argv[3 + i + 1]);
			uint16_t partBlocksize = totalBlockCount;
			while (writeBlock(irdFile, file, 0, type) == DATA_BLOCKSIZE)
			{
				totalBlockCount++;
				printf(".");
			}
			totalBlockCount++;
			partBlocksize = totalBlockCount - partBlocksize;
			printf("\nAdded %d Blocks, total %d\n", partBlocksize, totalBlockCount);
			fclose(file);
		}
		// Refresh Header
		uint8_t *dataBuf = (uint8_t *)malloc(headerDataBlockLen);
		// Read Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fread(dataBuf, 1, headerDataBlockLen, irdFile);
		// Update Blockcount
		insertint16_t(&dataBuf, 8, totalBlockCount);
		// Rewrite Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fwrite(dataBuf, 1, headerDataBlockLen, irdFile);
		// Update CRC
		uint16_t dataCrc = crc16(0, dataBuf, headerDataBlockLen);
		insertint16_t(&dataBuf, 0, dataCrc);
		// Rewrite CRC
		fseek(irdFile, 0x02, SEEK_SET);
		fwrite(dataBuf, 1, 2, irdFile);
		free(dataBuf);
		fclose(irdFile);
	}
	else if (argc >= 3 && strlen(argv[1]) == 3 && strncmp(argv[1], "-ce", 3) == 0)
	{
		FILE *irdFile = fopen(argv[2], "w+");
		uint16_t totalBlockCount = 0;
		uint16_t headerDataBlockLen = 0;
		//uint8_t * headerDataBloc;
		// Header
		headerDataBlockLen = writeBlock(irdFile, NULL, 1, 0x10);
		headerDataBlockLen += 4;
		uint8_t appendPartCount = argc - 3;
		for (int32_t i = 0; i < appendPartCount; i += 2)
		{
			uint8_t type = 0x01;
			if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-f", 2) == 0)
			{
				// Original APP, now FW
				type = 0x01;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-k", 2) == 0)
			{
				// KERNEL
				type = 0x06;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-e", 2) == 0)
			{
				//Original ROOT, now EXT
				type = 0x08;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-g", 2) == 0)
			{
				//Original DEV, now G
				type = 0x07;
			}
			else if (strlen(argv[3 + i]) == 2 && strncmp(argv[3 + i], "-r", 2) == 0)
			{
				//Original USER, now ROOT
				type = 0x09; // 7 if dev, than we need to split the file in 3 files
			}
			if (type == 0x01 || type == 0x08 || type == 0x07)
			{
				printf("Adding signed dummy squashfs header");
				FILE *file = fopen("dummy.squash.signed.padded", "w");
				printf(".");
				if (file != NULL)
				{
					printf(".");
					fwrite(dummy, 1, dummy_size, file);
					printf(".");
					fclose(file);
					printf(".");
					file = fopen("dummy.squash.signed.padded", "rb");
					printf(".");
					if (file != NULL)
					{
						while (writeBlock(irdFile, file, 0, type) == DATA_BLOCKSIZE)
						{
							printf(".");
							totalBlockCount++;
						}
						totalBlockCount++;
						fclose(file);
					}
					else
						printf("\nCould not write signed dummy squashfs header (2)\n");
					remove("dummy.squash.signed.padded");
				}
				else
					printf("\nCould not write signed dummy squashfs header (1)\n");
				printf("\n");
			}
			if (strncmp(argv[3 + i + 1], "foo", 3) != 0)
			{
				FILE *file = fopen(argv[3 + i + 1], "rb");
				if (file != NULL)
				{
					/*if(type == 0x07) { // DEV ROOT
					   // We have to split it onto 3 partitions
					   // dev    2E0000 2,875MB (300000 - 20000)
					   // config 100000 1MB
					   // user   1E00000 30MB
					   uint8_t buffer[0x1000];
					}*/
					printf("Adding %s", argv[3 + i + 1]);
					uint16_t partBlocksize = totalBlockCount;
					while (writeBlock(irdFile, file, 0, type) == DATA_BLOCKSIZE)
					{
						totalBlockCount++;
						printf(".");
					}
					totalBlockCount++;
					partBlocksize = totalBlockCount - partBlocksize;
					printf("\nAdded %d Blocks, total %d\n", partBlocksize, totalBlockCount);
					fclose(file);
				}
				else
				{
					printf("\nCould not append %s\n", argv[3 + i + 1]);
					printf("\n");
				}
			}
			else
			{
				printf("This is a foo partition\n");
			}
		}
		/// Refresh Header
		uint8_t *dataBuf = (uint8_t *)malloc(headerDataBlockLen);
		// Read Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fread(dataBuf, 1, headerDataBlockLen, irdFile);
		// Update Blockcount
		insertint16_t(&dataBuf, 8, totalBlockCount);
		// Rewrite Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fwrite(dataBuf, 1, headerDataBlockLen, irdFile);
		// Update CRC
		uint16_t dataCrc = crc16(0, dataBuf, headerDataBlockLen);
		insertint16_t(&dataBuf, 0, dataCrc);
		// Rewrite CRC
		fseek(irdFile, 0x02, SEEK_SET);
		fwrite(dataBuf, 1, 2, irdFile);
		free(dataBuf);
		fclose(irdFile);
	}
	else if (argc == 4 && strlen(argv[1]) == 2 && strncmp(argv[1], "-r", 2) == 0)
	{
		uint8_t headerDataBlockLen = 0;
		uint32_t resellerId = 0;
		sscanf(argv[3], "%x", &resellerId);
		FILE *irdFile = fopen(argv[2], "r+");
		if (irdFile == NULL)
		{
			printf("Error opening %s\n", argv[2]);
		}
		headerDataBlockLen = readShort(irdFile);
		headerDataBlockLen -= 2;
		printf("Changing reseller id to %04x\n", resellerId);
		/// Refresh Header
		uint8_t *dataBuf = (uint8_t *)malloc(headerDataBlockLen);
		// Read Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fread(dataBuf, 1, headerDataBlockLen, irdFile);
		// Update Blockcount
		insertint16_t(&dataBuf, 2, resellerId & 0xFFFF);
		// Rewrite Header Data Block
		fseek(irdFile, 0x04, SEEK_SET);
		fwrite(dataBuf, 1, headerDataBlockLen, irdFile);
		// Update CRC
		uint16_t dataCrc = crc16(0, dataBuf, headerDataBlockLen);
		insertint16_t(&dataBuf, 0, dataCrc);
		// Rewrite CRC
		fseek(irdFile, 0x02, SEEK_SET);
		fwrite(dataBuf, 1, 2, irdFile);
		free(dataBuf);
		fclose(irdFile);
	}
	else //show usage
	{
#ifdef USE_ZLIB
		uint8_t zlib = 1;
#else
		uint8_t zlib = 0;
#endif
		printf("\n");
		printf("Version: %s Date: %s USE_ZLIB: %d                                                        \n", VERSION, DATE, zlib);
		printf("\n");
		printf("Usage: %s -xcstv []                                                                      \n", argv[0]);
		printf("       -x [update.ird]           Extract IRD                                             \n");
		printf("       -c [update.ird] Options   Create IRD                                              \n");
		printf("         Options:                                                                        \n");
		printf("              [file.part]         Append Loader   (0) 0x00000000 - 0x000FFFFF (1   MB)   \n");
		printf("          -k  [file.part]         Append Kernel   (6) 0x00100000 - 0x003FFFFF (3   MB)   \n");
		printf(" *        -ao [file.part]         Append App Org  (*) 0x00400000 - 0x00aFFFFF (7   MB)   \n");
		printf(" s        -a  [file.part]         Append App      (1) 0x00b00000 - 0x011FFFFF (7   MB)   \n");
		printf(" s        -r  [file.part]         Append Root     (8) 0x01200000 - 0x01dFFFFF (12  MB)   \n");
		printf(" s        -d  [file.part]         Append Dev      (7) 0x01e00000 - 0x020FFFFF (3   MB)   \n");
		printf("          -c0 [file.part]         Append Config0  (2) 0x02100000 - 0x0213FFFF (256 KB)   \n");
		printf("          -c4 [file.part]         Append Config4  (3) 0x02140000 - 0x0217FFFF (256 KB)   \n");
		printf("          -c8 [file.part]         Append Config8  (4) 0x02180000 - 0x0219FFFF (128 KB)   \n");
		printf("          -ca [file.part]         Append ConfigA  (5) 0x021A0000 - 0x021BFFFF (128 KB)   \n");
		printf(" *        -cc [file.part]         Append ConfigC  (*) 0x021C0000 - 0x021FFFFF (256 KB)   \n");
		printf("          -u  [file.part]         Append User     (9) 0x02200000 - 0x03FFFFFF (30  MB)   \n");
		printf("          -1  [file.part]         Append Type 1   (1)                                    \n");
		printf("          ...                                                                            \n");
		printf("          -10 [file.part]         Append Type 10 (10)                                    \n");
		//printf("          -16 [file.part]         Append Type 16 (16)                                    \n");
		printf("       -ce [update.ird] Options   Create Enigma2 IRD                                     \n");
		printf("         Options:                                                                        \n");
		printf("          -k [file.part]          Append Kernel       0x00100000 - 0x003FFFFF (3      MB)\n");
		printf("          -f [file.part]          Append FW           0x00B20000 - 0x011FFFFF (6.875  MB)\n");
		//printf("          -r [file.part]          Append Root         0x01E20000 - 0x03FFFFFF (33.875 MB)\n");
		printf("          -r [file.part]          Append Root         0x02200000 - 0x03FFFFFF (30     MB)\n");
		printf("          -e [file.part]          Append Ext          0x01220000 - 0x01DFFFFF (11.875 MB)\n");
		printf("          -g [file.part]          Append G            0x01E20000 - 0x020FFFFF (2.875  MB)\n");
		printf("       -s [unsigned.squashfs]    Sign squashfs part                                      \n");
		printf("       -t [signed.squashfs]      Test signed squashfs part                               \n");
		printf("       -r [update.ird] id        Change reseller id (e.g. 2303 for atevio 7500)          \n");
		printf("       -v                        Display version                                         \n");
		printf("\n");
		printf("Note: For creating squashfs part. use mksquashfs v3.3                                    \n");
		printf("      ./mksquashfs squashfs-root flash.rootfs.own.mtd8 -nopad -le                        \n");
		printf("\n");
		printf("Example:                                                                                 \n");
		printf("  Creating a new IRD file with rootfs and kernel:                                        \n");
		printf("   %s -c my.ird -r flash.rootfs.own.mtd8.signed -k uimage.mtd6                           \n", argv[0]);
		printf("\n");
		printf("  Extracting a IRD file:                                                                 \n");
		printf("   %s -x my.ird                                                                          \n", argv[0]);
		printf("\n");
		printf("  Signing a squashfs partition:                                                          \n");
		printf("   %s -s my.squashfs                                                                     \n", argv[0]);
		return -1;
	}
	return 0;
}
