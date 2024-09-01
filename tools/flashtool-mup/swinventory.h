#ifndef SWINVENTORY_H_
#define SWINVENTORY_H_

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
#include "swunity.h"

class SwInventory
{
	public:
		SwInventory();
		SwInventory(uint8_t *data, uint32_t offset, uint32_t datalen);
		void print();
		void printXML(bool d);
		int32_t parse();
		int32_t isValid();

		bool verify();
		void extract();

		void setProductCode(unsigned int code)
		{
			this->mUnity->setProductCode(code);
		}

		void setPartition(unsigned int flashOffset, char *filename, uint8_t *data, uint32_t dataLength, uint32_t imageOffset);
		uint32_t getData(unsigned char **data);
		uint32_t getChildData(unsigned char **data);

		uint32_t getImageOffset();
	private:
		typedef struct sSWInventory
		{
			uint8_t  mMagicNumber[SW_UPDATE_MAGIC_SIZE];
			uint32_t mHeaderVersion;
			uint32_t mImageOffset;
			uint32_t mImageSize;//200h greater than datalength in unity
			uint32_t mFlashOffset;//same as in unity
			uint32_t mSWVersion;
			uint32_t mImageNumber;
		} tSWInventory;

		tSWInventory *mHeader;

		uint32_t mDataOffsetToBOF;
		uint32_t mDataLength;
		uint8_t *mData;

		uint32_t mChildDataLength;
		uint8_t *mChildData;

		SwUnity *mUnity;
};

#endif
