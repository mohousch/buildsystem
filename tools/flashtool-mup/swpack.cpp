#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <gcrypt.h> /* sha1 / crc32 */
#include <fcntl.h>
#include "misc.h"
#include "swpack.h"

SwPack::SwPack()
{
	// First set default values
	/*
	 <HeaderVersion value="100" />
	 <ProductCode value="11321000h" />
	 <SWVersion value="101" />
	 <Date value="151653694" str="Mon 1974-10-21 23:01:34 PDT" />
	 <UpdateInfo value="UFS922 Test Software Update" />
	*/
	this->mHeader = (tSWPack *) malloc(sizeof(tSWPack));
	memcpy(this->mHeader->mMagicNumber, (uint8_t *) SW_MAGIC_VALUE,
		   sizeof(SW_MAGIC_VALUE) <= sizeof(this->mHeader->mMagicNumber) ?
		   sizeof(SW_MAGIC_VALUE) : sizeof(this->mHeader->mMagicNumber));
	this->mHeader->mHeaderVersion   = 100;
	//this->mHeader->mProductCode   = 0x11301003; // Kathrein, DVB-S2, simple, Ufs-912
	this->mHeader->mProductCode     = 0x11321000; // Kathrein, DVB-S2, Twin-Pvr, Ufs-922
	this->mHeader->mSWVersion       = 101; //uboot says that this should be 102 for 922, but it seems that this is not used anyway
	this->mHeader->mDate            = time(NULL);
	this->mHeader->mInventoryCount  = 0;
	this->mHeader->mInvalidateFlag  = 0;
	memcpy(this->mHeader->mUpdateInfo, (uint8_t *) "Software Update",
		   sizeof("Software Update") <= sizeof(this->mHeader->mUpdateInfo) ?
		   sizeof("Software Update") : sizeof(this->mHeader->mUpdateInfo));
	mCurInventoryOffset = 0;
}

SwPack::SwPack(uint8_t *data, uint32_t datalen)
{
	mData = data;
	mDataLength = datalen;
}

void SwPack::parse()
{
	this->mHeader = (tSWPack *) mData;
	this->mChildDataOffset = SW_UPDATE_HEADER_SIZE /*sizeof(tSWPack)*/;
	this->mChildDataLength = this->mDataLength - this->mChildDataOffset;
	this->mChildData       = mData + this->mChildDataOffset;
	this->mInventoryCount = 0;
	int32_t offsetNextEntry = 0;
	/* until mInventoryCount seems to be not used loop over all inventories */
	while (this->mInventoryCount < MAX_INVENTORY_COUNT)
	{
		this->mInventory[this->mInventoryCount] = new SwInventory(mChildData + offsetNextEntry, mChildDataOffset + offsetNextEntry, mChildDataLength - offsetNextEntry);
		/*offsetNextEntry +=*/ this->mInventory[this->mInventoryCount]->parse();
		offsetNextEntry += SW_INVENTORY_SIZE;
		if (!this->mInventory[this->mInventoryCount]->isValid())
		{
			delete (this->mInventory[this->mInventoryCount]);
			break;
		}
		this->mInventoryCount++;
	}
}

void SwPack::print()
{
	char vHelpStr[256];
	strncpy(vHelpStr, (char *)this->mHeader->mMagicNumber, SW_UPDATE_MAGIC_SIZE);
	vHelpStr[SW_UPDATE_MAGIC_SIZE] = '\0';
	if (verbose)
	{
		printf("*******************************************************\n");
		printf("SWPack:\n\n");
		printf("mMagicNumber    = %s\n", vHelpStr);
		printf("mHeaderVersion  = %d\n", this->mHeader->mHeaderVersion);
		printf("mProductCode    = 0x%08X\n", this->mHeader->mProductCode);
		printf("mSWVersion      = %d\n", this->mHeader->mSWVersion);
		printf("mDate           = %d\n", (int)this->mHeader->mDate);
		printf("mInventoryCount = %d\n", this->mHeader->mInventoryCount);
		printf("mInvalidateFlag = %d\n", this->mHeader->mInvalidateFlag);
	}
	strncpy(vHelpStr, this->mHeader->mUpdateInfo, SW_UPDATE_INFO_LENGTH);
	vHelpStr[SW_UPDATE_INFO_LENGTH] = '\0';
	if (verbose)
	{
		printf("mUpdateInfo     = %s\n", vHelpStr);
	}
	for (uint32_t i = 0; i < this->mInventoryCount; i++)
	{
		this->mInventory[i]->print();
	}
}

void SwPack::printXML(bool d)
{
	char vHelpStr[256];
	if (verbose)
	{
		printf("\t<SWPack>\n");
		strncpy(vHelpStr, (char *)this->mHeader->mMagicNumber, SW_UPDATE_MAGIC_SIZE);
		vHelpStr[SW_UPDATE_MAGIC_SIZE] = '\0';
		if (d)
		{
			printf("\t\t<MagicNumber value=\"%s\" />\n", vHelpStr);
		}
		printf("\t\t<HeaderVersion value=\"%d\" />\n", this->mHeader->mHeaderVersion);
		printf("\t\t<ProductCode value=\"%08Xh\" />\n", this->mHeader->mProductCode);
		printf("\t\t<SWVersion value=\"%d\" />\n", this->mHeader->mSWVersion);
		printf("\t\t<Date value=\"%d\" str=\"%s\" />\n", (int)this->mHeader->mDate, strTime(this->mHeader->mDate));
		if (d)
		{
			printf("\t\t<InventoryCount value=\"%d\" />\n", this->mHeader->mInventoryCount);
			printf("\t\t<InvalidateFlag value=\"%d\" />\n", this->mHeader->mInvalidateFlag);
		}
		strncpy(vHelpStr, this->mHeader->mUpdateInfo, SW_UPDATE_INFO_LENGTH);
		vHelpStr[SW_UPDATE_INFO_LENGTH] = '\0';
		printf("\t\t<UpdateInfo value=\"%s\" />\n", vHelpStr);
		for (uint32_t i = 0; i < this->mInventoryCount; i++)
		{
			this->mInventory[i]->printXML(d);
		}
		printf("\t</SWPack>\n");
	}
}

bool SwPack::verify()
{
	bool rtv = true;
	for (uint32_t i = 0; i < this->mInventoryCount; i++)
	{
		if (!this->mInventory[i]->verify())
		{
			rtv = false;
		}
	}
	return rtv;
}

void SwPack::extract()
{
	for (uint32_t i = 0; i < this->mInventoryCount; i++)
	{
		this->mInventory[i]->extract();
	}
	return;
}

void SwPack::appendPartition(uint32_t flashOffset, char *filename, uint8_t *data, uint32_t dataLength)
{
	this->mInventory[this->mInventoryCount] = new SwInventory();
	this->mInventory[this->mInventoryCount]->setProductCode(this->mHeader->mProductCode);
	this->mInventory[this->mInventoryCount]->setPartition(flashOffset, filename, data, dataLength, mCurInventoryOffset);
	mCurInventoryOffset += this->mInventory[this->mInventoryCount]->getChildData(NULL);
	this->mInventoryCount++;
}

int32_t SwPack::createImage(uint8_t **data)
{
	/*int */mDataLength = SW_UPDATE_HEADER_SIZE + MAX_INVENTORY_COUNT * SW_INVENTORY_SIZE;
	for (uint32_t i = 0; i < this->mInventoryCount; i++)
	{
		mDataLength += this->mInventory[i]->getChildData(NULL);
	}
	if (verbose)
	{
		printf("TOTAL SIZE: %u\n", mDataLength);
	}
	mData = (uint8_t *)malloc(mDataLength);
	memcpy(mData, mHeader, sizeof(tSWPack));
	for (uint32_t i = 0; i < this->mInventoryCount; i++)
	{
		int32_t cDataLenght = this->mInventory[i]->getData(NULL);
		uint8_t *cData = (uint8_t *)malloc(cDataLenght);
		this->mInventory[i]->getData(&cData);
		memcpy(mData + SW_UPDATE_HEADER_SIZE + i * SW_INVENTORY_SIZE, cData, cDataLenght);
		free(cData);
		cDataLenght = this->mInventory[i]->getChildData(NULL);
		cData = (uint8_t *)malloc(cDataLenght);
		this->mInventory[i]->getChildData(&cData);
		memcpy(mData + this->mInventory[i]->getImageOffset() + SW_UPDATE_HEADER_SIZE + MAX_INVENTORY_COUNT * SW_INVENTORY_SIZE, cData, cDataLenght);
		free(cData);
	}
	*data = (uint8_t *)malloc(this->mDataLength);
	memcpy(*data, this->mData, this->mDataLength);
	return mDataLength;
}
