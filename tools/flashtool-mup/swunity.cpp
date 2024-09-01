#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#if GCRY
#include <gcrypt.h> /* sha1 / crc32 */
#endif
#include <fcntl.h>

#include "crc32.h"
#include "sh1.h"
#include "misc.h"
#include "swunity.h"

SwUnity::SwUnity()
{
	// First set default values
	/*
	 <MagicNumber value="MARUSWUP" />
	 <HeaderVersion value="100" />
	 <ProductCode value="11321000h" />
	 <SWVersion value="100" />
	 <Date value="1267205316" str="Fri 2010-02-26 09:28:36 PST" />
	 <FlashOffset value="004E0000h" />
	 <DataLength value="00A9E000h" />
	 <Hash value="88CB7D8B62D5C69256859269B38D3B6E2A2F1320h" />
	 <CRC value="84FC2BE0h" />
	 <FileName value="922.cramfs" />
	 <UpdateInfo value="UFS922 Test Software Update - (Main Software) " />
	*/
	this->mHeader = (tSWUnity *) malloc(sizeof(tSWUnity));
	memcpy(this->mHeader->mMagicNumber, (uint8_t *) SW_MAGIC_VALUE,
		   sizeof(SW_MAGIC_VALUE) <= sizeof(this->mHeader->mMagicNumber) ?
		   sizeof(SW_MAGIC_VALUE) : sizeof(this->mHeader->mMagicNumber));
	this->mHeader->mHeaderVersion   = 100;
	//this->mHeader->mProductCode   = 0x11301003; // Kathrein, DVB-S2, simple, Ufs-912
	this->mHeader->mProductCode     = 0x11321000; // Kathrein, DVB-S2, Twin-Pvr, Ufs-922
	this->mHeader->mSWVersion       = 101; //uboot says that this should be 102 für 922, but it seems that this is not used anyway
	this->mHeader->mDate            = time(NULL);
	this->mHeader->mFlashOffset     = 0;
	this->mHeader->mDataLength      = 0;
	memcpy(this->mHeader->mUpdateInfo, (uint8_t *) "Software Update",
		   sizeof("Software Update") <= sizeof(this->mHeader->mUpdateInfo) ?
		   sizeof("Software Update") : sizeof(this->mHeader->mUpdateInfo));
}

SwUnity::SwUnity(uint8_t *data, uint32_t datalen)
{
	mData = data;
	mDataLength = datalen;
}

int32_t SwUnity::parse()
{
	if (this->mDataLength >= sizeof(tSWUnity))
	{
		this->mHeader = (tSWUnity *) this->mData;
		this->mChildDataLength = this->mDataLength - SW_UPDATE_HEADER_SIZE;
		if (this->mChildDataLength > this->mHeader->mDataLength)
		{
			//printf("Not equal !\n\t%d - %d\n", this->mChildDataLength, this->mHeader->mDataLength);
			this->mChildDataLength = this->mHeader->mDataLength;
		}
		this->mChildData = this->mData + SW_UPDATE_HEADER_SIZE;
		return sizeof(tSWUnity);
	}
	return 0;
}

void SwUnity::print()
{
	char vHelpStr[256];
	printf("*******************************************************\n");
	printf("SWUnity:\n\n");
	strncpy(vHelpStr, (char *)this->mHeader->mMagicNumber, SW_UPDATE_MAGIC_SIZE);
	vHelpStr[SW_UPDATE_MAGIC_SIZE] = '\0';
	printf("mMagicNumber    = %s\n", vHelpStr);
	printf("mHeaderVersion  = %d\n", this->mHeader->mHeaderVersion);
	printf("mProductCode    = %d\n", this->mHeader->mProductCode);
	printf("mSWVersion      = %d\n", this->mHeader->mSWVersion);
	printf("mDate           = %d\n", (int)this->mHeader->mDate);
	printf("mFlashOffset    = 0x%08X (%d)\n", this->mHeader->mFlashOffset, this->mHeader->mFlashOffset);
	printf("mDataLength     = 0x%08X (%d)\n", this->mHeader->mDataLength, this->mHeader->mDataLength);
	strncpy(vHelpStr, this->mHeader->mHashValue, 20);
	vHelpStr[20] = '\0';
	printf("mHashValue      =\n");
	for (int32_t i = 0; i < 20; i++)
	{
		printf("0x%02x ", vHelpStr[i] & 0xff);
		if (((i + 1) % 10) == 0)
		{
			printf("\n");
		}
	}
	printf("mCRC            = 0x%08X (%d)\n", this->mHeader->mCRC, this->mHeader->mCRC);
	strncpy(vHelpStr, (char *)this->mHeader->mFileName, SW_UPDATE_FILENAME_LENGTH);
	vHelpStr[SW_UPDATE_FILENAME_LENGTH] = '\0';
	printf("mFileName       = %s\n", vHelpStr);
	strncpy(vHelpStr, (char *)this->mHeader->mUpdateInfo, SW_UPDATE_INFO_LENGTH);
	vHelpStr[SW_UPDATE_INFO_LENGTH] = '\0';
	printf("mUpdateInfo     = %s\n", vHelpStr);
}

void SwUnity::printXML(bool d)
{
	char vHelpStr[256];
	printf("\t\t\t<SWUnity>\n");
	strncpy(vHelpStr, (char *)this->mHeader->mMagicNumber, SW_UPDATE_MAGIC_SIZE);
	vHelpStr[SW_UPDATE_MAGIC_SIZE] = '\0';
	if (d)
	{
		printf("\t\t\t\t<MagicNumber value=\"%s\" />\n", vHelpStr);
		printf("\t\t\t\t<HeaderVersion value=\"%d\" />\n", this->mHeader->mHeaderVersion);
		printf("\t\t\t\t<ProductCode value=\"%08Xh\" />\n", this->mHeader->mProductCode);
		printf("\t\t\t\t<SWVersion value=\"%d\" />\n", this->mHeader->mSWVersion);
		printf("\t\t\t\t<Date value=\"%d\" str=\"%s\" />\n", (int)this->mHeader->mDate, strTime(this->mHeader->mDate));
	}
	printf("\t\t\t\t<FlashOffset value=\"%08Xh\" />\n", this->mHeader->mFlashOffset);
	if (d)
	{
		printf("\t\t\t\t<DataLength value=\"%08Xh\" />\n", this->mHeader->mDataLength);
	}
	strncpy(vHelpStr, this->mHeader->mHashValue, 20);
	vHelpStr[20] = '\0';
	if (d)
	{
		printf("\t\t\t\t<Hash value=\"");
		for (int32_t i = 0; i < 20; i++)
		{
			printf("%02X", vHelpStr[i] & 0xff);
		}
		printf("h\" />\n");
		printf("\t\t\t\t<CRC value=\"%08Xh\" />\n", this->mHeader->mCRC);
	}
	strncpy(vHelpStr, (char *)this->mHeader->mFileName, SW_UPDATE_FILENAME_LENGTH);
	vHelpStr[SW_UPDATE_FILENAME_LENGTH] = '\0';
	printf("\t\t\t\t<FileName value=\"%s\" />\n", vHelpStr);
	strncpy(vHelpStr, (char *)this->mHeader->mUpdateInfo, SW_UPDATE_INFO_LENGTH);
	vHelpStr[SW_UPDATE_INFO_LENGTH] = '\0';
	printf("\t\t\t\t<UpdateInfo value=\"%s\" />\n", vHelpStr);
#if 0
	printf("\t\t\t\t<Data value=\"");
	for (int32_t i = 0; i < this->mChildDataLength; i++)
	{
		printf("%02X", this->mChildData[i]);
	}
	printf("h\" />\n");
#endif
	printf("\t\t\t</SWUnity>\n");
}

int32_t SwUnity::isValid()
{
	if (strncmp((char *)this->mHeader->mMagicNumber, (char *)SW_MAGIC_VALUE, SW_UPDATE_MAGIC_SIZE) != 0)
	{
		return 0;
	}
	if (this->mHeader->mHeaderVersion != SW_UPDATER_VERSION)
	{
		return 0;
	}
	return 1;
}

void SwUnity::calcSH1(uint8_t **sh1_hash, uint32_t *sh1_hash_len)
{
#if GCRY
	/* let us see how int32_t is the hash key for SHA1 ... */
	*sh1_hash_len = gcry_md_get_algo_dlen(GCRY_MD_SHA1);
	*sh1_hash = (uint8_t *)malloc((*sh1_hash_len) * sizeof(uint8_t));
	/* calculate the hash */
	gcry_md_hash_buffer(GCRY_MD_SHA1, *sh1_hash, this->mChildData, this->mChildDataLength);
#else
	SHA1_CTX mContext;
	*sh1_hash_len = 20;
	*sh1_hash = (uint8_t *)malloc((*sh1_hash_len) * sizeof(uint8_t));
	SHA1Init(&mContext);
	SHA1Update(&mContext, this->mChildData, this->mChildDataLength);
	SHA1Final(*sh1_hash, &mContext);
#endif
	return;
}

uint32_t SwUnity::calcCRC32(uint8_t **crc32_hash, uint32_t *crc32_hash_len)
{
#if GCRY
	/* let us see how int32_t is the hash key for CRC32 ... */
	*crc32_hash_len = gcry_md_get_algo_dlen(GCRY_MD_CRC32);
	*crc32_hash = (uint8_t *)malloc((*crc32_hash_len) * sizeof(uint8_t));
	/* calculate the hash */
	gcry_md_hash_buffer(GCRY_MD_CRC32, *crc32_hash, this->mChildData, this->mChildDataLength);
#endif
	if (verbose)
	{
		printf("%d\n", this->mChildDataLength);
		for (int32_t i = 0; i < 40 && i < this->mChildDataLength; i++)
		{
			printf("%02X ", this->mChildData[i]);
		}
		printf("\n");
		if (this->mChildDataLength >= 80)
		{
			for (int32_t i = 0; i < 40; i++)
			{
				printf("%02X ", this->mChildData[this->mChildDataLength - 41 + i]);
			}
			printf("\n");
		}
	}
	return crc32(this->mChildData, this->mChildDataLength);
}

bool SwUnity::verify()
{
	bool crc32 = false;
	bool sh1   = false;
	uint8_t *sh1HashArray = NULL;
	uint32_t sh1HashLength = 0;
	calcSH1(&sh1HashArray, &sh1HashLength);
	printf("SH1 ORG:  ");
	for (int32_t i = 0; i < 20; i++)
	{
		printf("%02X ", this->mHeader->mHashValue[i] & 0xff);
	}
	printf("\n");
	printf("SH1 CALC: ");
	for (int32_t i = 0; i < 20; i++)
	{
		printf("%02X ", sh1HashArray[i] & 0xff);
	}
	printf("\n");
	if (memcmp(this->mHeader->mHashValue, sh1HashArray, sh1HashLength))
	{
		sh1 = false;
	}
	else
	{
		sh1 = true;
	}
	uint8_t *crc32HashArray = NULL;
	uint32_t crc32Hash = 0;
	uint32_t crc32HashLength = 0;
	crc32Hash = calcCRC32(&crc32HashArray, &crc32HashLength);
	/*crc32Hash = crc32HashArray[0];
	crc32Hash += crc32HashArray[1]<<8;
	crc32Hash += crc32HashArray[2]<<16;
	crc32Hash += crc32HashArray[3]<<24;*/
	printf("CRC32 %08X - %08X\n", crc32Hash, this->mHeader->mCRC);
	if (crc32Hash == this->mHeader->mCRC)
	{
		crc32 = true;
	}
	else
	{
		crc32 = false;
	}
	return crc32 && sh1;
}

void SwUnity::extract()
{
	char vHelpStr[256];
	int32_t fd;
	uint32_t i;
	strncpy(vHelpStr, (char *)this->mHeader->mFileName, SW_UPDATE_FILENAME_LENGTH);
	vHelpStr[SW_UPDATE_FILENAME_LENGTH] = '\0';
	for (int32_t i = 0; i < strlen(vHelpStr); i++)
	{
		if (vHelpStr[i] == '/' || vHelpStr[i] == '\\')
		{
			vHelpStr[i] = '_';
		}
	}
	fd = open(vHelpStr, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0)
	{
		printf("error opening %s\n", vHelpStr);
		return;
	}
	i = write(fd, this->mChildData, this->mChildDataLength);
	if (i != this->mChildDataLength)
	{
		printf("error reading data %d / %d\n", i, this->mChildDataLength);
	}
	close(fd);
}

void SwUnity::setPartition(uint32_t flashOffset, char *filename, uint8_t *data, uint32_t dataLength)
{
	this->mHeader->mFlashOffset = flashOffset;
	memcpy(this->mHeader->mFileName, filename, strlen(filename));
	this->mHeader->mFileName[strlen(filename)] = '\0';
	this->mHeader->mDataLength = dataLength;
	this->mChildDataLength = dataLength;
	this->mChildData = (uint8_t *)malloc(dataLength);
	memcpy(this->mChildData, data, dataLength);
	uint8_t *sh1HashArray = NULL;
	uint32_t sh1HashLength = 0;
	calcSH1(&sh1HashArray, &sh1HashLength);
	for (int32_t i = 0; i < 20; i++)
	{
		this->mHeader->mHashValue[i] = sh1HashArray[i];
	}
	uint32_t crc32HashLength = 0;
	this->mHeader->mCRC = calcCRC32(NULL, &crc32HashLength);
	this->mDataLength = this->mChildDataLength + SW_UPDATE_HEADER_SIZE;
	this->mData = (uint8_t *) malloc(this->mDataLength);
	memcpy(this->mData, this->mHeader, sizeof(tSWUnity));
	memcpy(this->mData + SW_UPDATE_HEADER_SIZE, this->mChildData, this->mChildDataLength);
}

uint32_t SwUnity::getData(uint8_t **data)
{
	if (data != NULL)
	{
		memcpy(*data, this->mData, this->mDataLength);
	}
	return this->mDataLength;
}
