/**************************************************************************/
/* Name :   pad                                                           */
/*                                                                        */
/* Author:  Schischu                                                      */
/*                                                                        */
/* Licence: This file is subject to the terms and conditions of the       */
/*          GNU General Public License version 2.                         */
/**************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 4096

int32_t main(int32_t argc, char *argv[])
{
	int32_t padToSize = 0, readBytes = 0;
	uint8_t buffer[BUFSIZE];
	FILE *input = fopen(argv[2], "rb");
	FILE *output = fopen(argv[3], "wb");
	sscanf(argv[1], "%x", &padToSize);
	while ((readBytes = fread(buffer, 1, BUFSIZE, input)) > 0)
	{
		padToSize -= fwrite(buffer, 1, readBytes, output);
	}
	memset(buffer, 0xFF, BUFSIZE);
	while (padToSize > 0)
	{
		padToSize -= fwrite(buffer, 1, padToSize > BUFSIZE ? BUFSIZE : padToSize, output);
	}
	return 0;
}
