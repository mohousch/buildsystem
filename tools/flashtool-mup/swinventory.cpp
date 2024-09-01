#include <stdint.h>

#include "swinventory.h"

SwInventory::SwInventory()
{
	// First set default values
	/*
	 <MagicNumber value="MARUSWUP" />
	 <HeaderVersion value="100" />
	 <ImageOffset value="00000000h" />
	 <ImageSize value="00A9E200h" />
	 <FlashOffset value="004E0000h" />
	 <SWVersion value="808533614" />
	 <ImageNumber value="1445936176" />
	*/
	this->mHeader = (tSWInventory *) malloc(sizeof(tSWInventory));
	memcpy(this->mHeader->mMagicNumber, (uint8_t *) SW_MAGIC_VALUE,
		   sizeof(SW_MAGIC_VALUE) <= sizeof(this->mHeader->mMagicNumber) ?
		   sizeof(SW_MAGIC_VALUE) : sizeof(this->mHeader->mMagicNumber));
	this->mHeader->mHeaderVersion = 100;
	this->mHeader->mImageOffset   = 0x0;
	this->mHeader->mImageSize     = 0x0;
	this->mHeader->mFlashOffset   = 0x0;
	this->mHeader->mSWVersion     = 0;
	this->mHeader->mImageNumber   = 0;
	this->mUnity = new SwUnity();
}

SwInventory::SwInventory(uint8_t *data, uint32_t offset, uint32_t datalen)
{
	mData = data;
	mDataOffsetToBOF = offset;
	mDataLength = datalen;
}

int32_t SwInventory::parse()
{
	if (this->mDataLength >= sizeof(tSWInventory))
	{
		this->mHeader = (tSWInventory *) this->mData;
		this->mChildDataLength = this->mHeader->mImageSize;
		this->mChildData       = this->mData - this->mDataOffsetToBOF + this->mHeader->mImageOffset + SW_UPDATE_HEADER_SIZE /* jump over pack header */ +
								 MAX_INVENTORY_COUNT * SW_INVENTORY_SIZE /* plus end of all possible inventories */;
		this->mUnity = new SwUnity(this->mChildData, this->mChildDataLength);
		this->mUnity->parse();
		return sizeof(tSWInventory);
	}
	return 0;
}

void SwInventory::print()
{
	char vHelpStr[256];
	printf("*******************************************************\n");
	printf("SWInventory:\n\n");
	strncpy(vHelpStr, (char *)this->mHeader->mMagicNumber, SW_UPDATE_MAGIC_SIZE);
	vHelpStr[SW_UPDATE_MAGIC_SIZE] = '\0';
	printf("mMagicNumber    = %s\n", vHelpStr);
	printf("mHeaderVersion  = %d\n", this->mHeader->mHeaderVersion);
	printf("mImageOffset    = 0x%08X (%d)\n", this->mHeader->mImageOffset, this->mHeader->mImageOffset);
	printf("mImageSize      = 0x%08X (%d)\n", this->mHeader->mImageSize, this->mHeader->mImageSize);
	printf("mFlashOffset    = 0x%08X (%d)\n", this->mHeader->mFlashOffset, this->mHeader->mFlashOffset);
	printf("mSWVersion      = %d\n", this->mHeader->mSWVersion);
	printf("mImageNumber    = %d\n", this->mHeader->mImageNumber);
	this->mUnity->print();
}

void SwInventory::printXML(bool d)
{
	char vHelpStr[256];
	printf("\t\t<SWInventory>\n");
	strncpy(vHelpStr, (char *)this->mHeader->mMagicNumber, SW_UPDATE_MAGIC_SIZE);
	vHelpStr[SW_UPDATE_MAGIC_SIZE] = '\0';
	if (d)
	{
		printf("\t\t\t<MagicNumber value=\"%s\" />\n", vHelpStr);
		printf("\t\t\t<HeaderVersion value=\"%d\" />\n", this->mHeader->mHeaderVersion);
		printf("\t\t\t<ImageOffset value=\"%08Xh\" />\n", this->mHeader->mImageOffset);
		printf("\t\t\t<ImageSize value=\"%08Xh\" />\n", this->mHeader->mImageSize);
		printf("\t\t\t<FlashOffset value=\"%08Xh\" />\n", this->mHeader->mFlashOffset);
		printf("\t\t\t<SWVersion value=\"%d\" />\n", this->mHeader->mSWVersion);
		printf("\t\t\t<ImageNumber value=\"%d\" />\n", this->mHeader->mImageNumber);
	}
	this->mUnity->printXML(d);
	printf("\t\t</SWInventory>\n");
}

int32_t SwInventory::isValid()
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

bool SwInventory::verify()
{
	return this->mUnity->verify();
}

void SwInventory::extract()
{
	this->mUnity->extract();
}

void SwInventory::setPartition(uint32_t flashOffset, char *filename, uint8_t *data, uint32_t dataLength, uint32_t imageOffset)
{
	this->mHeader->mFlashOffset = flashOffset;
	this->mHeader->mImageOffset = imageOffset;
	this->mHeader->mImageSize = dataLength + SW_UPDATE_HEADER_SIZE;
	this->mUnity->setPartition(flashOffset, filename, data, dataLength);
	this->mDataLength = sizeof(tSWInventory);
	this->mData = (uint8_t *) malloc(sizeof(tSWInventory));
	memcpy(this->mData, this->mHeader, sizeof(tSWInventory));
	this->mChildData = (uint8_t *)malloc(this->mUnity->getData(NULL));
	this->mChildDataLength = this->mUnity->getData(&this->mChildData);
	if (this->mChildDataLength % 0x200 != 0)
	{
		this->mChildDataLength += (0x200 - (this->mChildDataLength % 0x200));
		this->mHeader->mImageSize = this->mChildDataLength;
		memcpy(this->mData, this->mHeader, sizeof(tSWInventory));
	}
}

uint32_t SwInventory::getChildData(uint8_t **data)
{
	if (data != NULL)
	{
		memcpy(*data, this->mChildData, this->mChildDataLength);
	}
	return this->mChildDataLength;
}

uint32_t SwInventory::getData(uint8_t **data)
{
	if (data != NULL)
	{
		memcpy(*data, this->mData, this->mDataLength);
	}
	return this->mDataLength;
}

uint32_t SwInventory::getImageOffset()
{
	return this->mHeader->mImageOffset;
}

