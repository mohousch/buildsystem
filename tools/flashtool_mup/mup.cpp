/**************************************************************************/
/* Name :   mup                                                           */
/*                                                                        */
/* Author:  Schischu                                                      */
/*                                                                        */
/* Licence: This file is subject to the terms and conditions of the       */
/*          GNU General Public License version 2.                         */
/**************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <gcrypt.h> /* sha1 / crc32 */
#include <fcntl.h>

#include "swpack.h"

#define MUP_VERSION "1.4"

#define NOR_RAW 0
#define NAND_RAW 1
#define NAND_YAFFS2 2
#define NAND_YAFFS2_ERASE 3

bool verbose = true;

void clear(FILE *fp)
{
	char buf[255];
	while (fgets(buf, 255, fp) != NULL && feof(fp))
	{
		/* puts (buf); */
	}
}

int32_t main(int32_t argc, char *argv[])
{
	FILE *file;
	int32_t i, data_len;
	uint8_t *buffer;
	struct stat buf;
	bool doInfo = false;
	bool doVerify = false;
	bool doXML = false;
	bool doXMLDetail = false;
	bool doExtract = false;
	bool doCreate = false;
	if (argc == 3 && strlen(argv[1]) == 1 && !strncmp(argv[1], "i", 1))
	{
		doInfo = true;
	}
	else if (argc == 3 && strlen(argv[1]) == 1 && !strncmp(argv[1], "v", 1))
	{
		doVerify = true;
	}
	else if (argc == 3 && strlen(argv[1]) == 1 && !strncmp(argv[1], "x", 1))
	{
		doXML = true;
	}
	else if (argc == 3 && strlen(argv[1]) == 2 && !strncmp(argv[1], "xx", 2))
	{
		doXMLDetail = true;
	}
	else if (argc == 3 && strlen(argv[1]) == 1 && !strncmp(argv[1], "e", 1))
	{
		doExtract = true;
	}
	else if (argc == 3 && strlen(argv[1]) == 1 && !strncmp(argv[1], "c", 1))
	{
		doCreate = true;
		verbose = false;
	}
	else
	{
		printf("Version: %s %s\n", argv[0], MUP_VERSION);
		printf("Usage:\n");
		printf("For image information:\n");
		printf("  %s i FILENAME       Info as TXT\n", argv[0]);
		printf("  %s x FILENAME       Info as XML (basic)\n", argv[0]);
		printf("  %s xx FILENAME      Info as XML (detailed)\n", argv[0]);
		printf("For image verification:\n");
		printf("  %s v FILENAME       \n", argv[0]);
		printf("For image extraction:\n");
		printf("  %s e FILENAME        \n", argv[0]);
		printf("For image generation:\n");
		printf("  %s c FILENAME        \n", argv[0]);
		exit(10);
	}
	if (doInfo || doVerify || doXML || doXMLDetail || doExtract)
	{
		//////////////////////////////////
		if (stat(argv[2], &buf) < 0)
		{
			fprintf(stderr, "Cannot open(stat) %s", argv[2]);
			return -2;
		}
		data_len = buf.st_size;
		file = fopen(argv[2], "r");
		if (file == NULL)
		{
			fprintf(stderr, "Unable to open %s\n", argv[1]);
			return -3;
		}
		buffer = (uint8_t *) malloc(data_len);
		if (buffer == NULL)
		{
			fprintf(stderr, "Unable to get memory %d\n", data_len);
			return -4;
		}
		i = fread(buffer, 1, data_len, file);
		//////////////////////////////////
		SwPack *swpack = new SwPack(buffer, i);
		swpack->parse();
		if (doInfo)
		{
			swpack->print();
		}
		else if (doVerify)
		{
			if (swpack->verify())
			{
				printf("Image is correct\n");
			}
			else
			{
				printf("Image is NOT correct\n");
			}
		}
		else if (doXML || doXMLDetail)
		{
			printf("<MARUSWUP version=\"1.0\">\n");
			swpack->printXML(doXMLDetail);
			printf("</MARUSWUP>\n");
		}
		else if (doExtract)
		{
			swpack->extract();
		}
		/* act of solidarity ;) */
		free(buffer);
		fclose(file);
	}
	else if (doCreate)
	{
		//////////////////////////////////
		/*
		- create list of partitions
		- create swpack
		- swpack set boxtype
		- swpack append partition (address, filename)
		- swpack create update image
		*/
		char outputName[1024];
		strcpy(outputName, argv[2]);
		uint32_t inputBufferLength = 255;
		char inputBuffer[inputBufferLength + 1];
		char partitionName[inputBufferLength + 1];
		int32_t productCode = 0;
		int32_t flashOffset = -1;
		int32_t nand = 0;
		int32_t blockSize = 0;
		SwPack *swpack = new SwPack();
		verboseprintf("Select ProductCode\n");
		verboseprintf("1: 0x11321000 - Kathrein UFS-922\n");
		verboseprintf("2: 0x11301003 - Kathrein UFS-912\n");
		verboseprintf("3: 0x11301006 - Kathrein UFS-913\n");
		verboseprintf(":> ");
		scanf("%d", &productCode);
		switch (productCode)
		{
			case 1:
			{
				productCode = 0x11321000;
				break;
			}
			case 3:
			{
				productCode = 0x11321006;
				break;
			}
			case 2:
			default:
			{
				productCode = 0x11301003;
				break;
			}
		}
		swpack->setProductCode(productCode);
		verboseprintf("Enter partitions. \"FLASHOFFSET, BLOCKSIZE, NAND, FILENAME;\"\n");
		verboseprintf("Finish list with \";\"\n");
		verboseprintf("Example: \n");
		verboseprintf("\t0x004E0000, 0x0, 0, 922.cramfs\n");
		verboseprintf("\t0x00040000, 0x0, 0, uImage.922.105\n");
		verboseprintf("\t0x002A0000, 0x0, 0, root.922.105.cramfs\n");
		verboseprintf(";\n");
		fflush(stdin);
		clear(stdin);
		while (1)
		{
			verboseprintf(":> ");
			flashOffset = -1;
			scanf("0x%X, ", &flashOffset);
			if (flashOffset == -1)
			{
				verboseprintf("\n flashOffset missing\n");
				break;
			}
			blockSize = -1;
			scanf("0x%X, ", &blockSize);
			if (blockSize == -1)
			{
				verboseprintf("\n blockSize missing\n");
				break;
			}
			scanf("%d, ", &nand);
			if (nand != NOR_RAW && nand != NAND_RAW && nand != NAND_YAFFS2 && nand != NAND_YAFFS2_ERASE)
			{
				verboseprintf("\n nand missing\n");
				break;
			}
			scanf("%s", inputBuffer);
			if (verbose)
			{
				printf("\tflashOffset: %d, blockSize: %d, inputBuffer: %s\n", flashOffset, blockSize, inputBuffer);
			}
			fflush(stdin);
			clear(stdin);
			if (!strcmp(inputBuffer, "foo"))
			{
				data_len = 8;
				buffer = (uint8_t *) malloc(8);
				buffer[0] = 0xFF;
				buffer[1] = 0xFF;
				buffer[2] = 0xFF;
				buffer[3] = 0xFF;
				buffer[4] = 0xFF;
				buffer[5] = 0xFF;
				buffer[6] = 0xFF;
				buffer[7] = 0xFF;
				i = 8;
			}
			else
			{
				stat(inputBuffer, &buf);
				data_len = buf.st_size;
				file = fopen(inputBuffer, "r");
				if (file == NULL)
				{
					fprintf(stderr, "Unable to open %s\n", inputBuffer);
					return -3;
				}
				buffer = (uint8_t *) malloc(data_len);
				if (buffer == NULL)
				{
					fprintf(stderr, "Unable to get mem %d\n", data_len);
					return -4;
				}
				i = fread(buffer, 1, data_len, file);
				fclose(file);
			}
			if (nand == NAND_RAW)
			{
				if (productCode == 0x11321006)
				{
					sprintf(partitionName, "/2/3/"); // NAND RAW
				}
				else
				{
					sprintf(partitionName, "/2/O3/"); // NAND RAW
				}
				strcat(partitionName, inputBuffer);
			}
			else if (nand == NAND_YAFFS2)
			{
				if (blockSize == 0)
				{
					blockSize = data_len;
					if (blockSize % (1 << 16) != 0)
					{
						blockSize = (blockSize >> 16) + 1;
					}
					else
					{
						blockSize = blockSize >> 16;
					}
				}
				else
				{
					blockSize = blockSize >> 16;
				}
				if (verbose)
				{
					printf("BS: %d\n", blockSize);
				}
				if (productCode == 0x11321006)
				{
					sprintf(partitionName, "/%X/5/", blockSize); // NAND YAFFS2
				}
				else
				{
					sprintf(partitionName, "/%X/O5/", blockSize); // NAND YAFFS2
				}
				strcat(partitionName, inputBuffer);
			}
			else if (nand == NAND_YAFFS2_ERASE)
			{
				if (blockSize == 0)
				{
					blockSize = data_len;
					if (blockSize % (1 << 16) != 0)
					{
						blockSize = (blockSize >> 16) + 1;
					}
					else
					{
						blockSize = blockSize >> 16;
					}
				}
				else
				{
					blockSize = blockSize >> 16;
				}
				if (verbose)
				{
					printf("BS: %d\n", blockSize);
				}
				sprintf(partitionName, "/%X/E5/", blockSize); // NAND YAFFS2
				strcat(partitionName, inputBuffer);
			}
			else
			{
				strcpy(partitionName, inputBuffer); // NOR RAW
			}
			swpack->appendPartition(flashOffset, partitionName, (uint8_t *)buffer, i);
			free(buffer);
		}
		swpack->printXML(true);
		data_len = swpack->createImage(&buffer);
		file = fopen(outputName, "wb");
		if (file == NULL)
		{
			fprintf(stderr, "Unable to open %s\n", outputName);
			return -3;
		}
		if (buffer == NULL)
		{
			fprintf(stderr, "Unable to get memory %d\n", data_len);
			return -4;
		}
		i = fwrite(buffer, 1, data_len, file);
		fclose(file);
#if 0
		SwPack *swpack = new SwPack();
		swpack->setProductCode(0x11321000);
		char *part1 = strdup("922.cramfs");
		char *part2 = strdup("uImage.922.105");
		char *part3 = strdup("root.922.105.cramfs");
		//////////////
		stat(part1, &buf);
		data_len = buf.st_size;
		file = fopen(part1, "r");
		if (file == NULL)
		{
			fprintf(stderr, "Unable to open %s\n", part1);
			return -3;
		}
		buffer = (uint8_t *) malloc(data_len);
		if (buffer == NULL)
		{
			fprintf(stderr, "Unable to get memory %d\n", data_len);
			return -4;
		}
		i = fread(buffer, 1, data_len, file);
		fclose(file);
		swpack->appendPartition(0x004E0000, part1, (uint8_t *)buffer, i);
		free(buffer);
		//////////////
		stat(part2, &buf);
		data_len = buf.st_size;
		file = fopen(part2, "r");
		if (file == NULL)
		{
			fprintf(stderr, "Unable to open %s\n", part2);
			return -3;
		}
		buffer = (uint8_t *) malloc(data_len);
		if (buffer == NULL)
		{
			fprintf(stderr, "Unable to get memory %d\n", data_len);
			return -4;
		}
		i = fread(buffer, 1, data_len, file);
		fclose(file);
		swpack->appendPartition(0x00040000, part2, (uint8_t *)buffer, i);
		free(buffer);
		//////////////
		stat(part3, &buf);
		data_len = buf.st_size;
		file = fopen(part3, "r");
		if (file == NULL)
		{
			fprintf(stderr, "Unable to open %s\n", part3);
			return -3;
		}
		buffer = (uint8_t *) malloc(data_len);
		if (buffer == NULL)
		{
			fprintf(stderr, "Unable to get memory %d\n", data_len);
			return -4;
		}
		i = fread(buffer, 1, data_len, file);
		fclose(file);
		swpack->appendPartition(0x002A0000, part3, (uint8_t *)buffer, i);
		free(buffer);
		/////////////
		swpack->printXML(true);
		data_len = swpack->createImage(&buffer);
		file = fopen("update.img", "wb");
		if (file == NULL)
		{
			fprintf(stderr, "Unable to open %s\n", part1);
			return -3;
		}
		if (buffer == NULL)
		{
			fprintf(stderr, "Unable to get memory %d\n", data_len);
			return -4;
		}
		for (int32_t j = 0; j < 20; j++)
		{
			printf("%02X ", *(buffer + j));
		}
		printf("\n");
		i = fwrite(buffer, 1, data_len, file);
		fclose(file);
#endif
	}
	return 0;
}
