#ifndef SWPACK_H_
#define SWPACK_H_

#include <stdint.h>
#include "swinventory.h"

class SwPack
{
	public:
		SwPack();
		SwPack(uint8_t *data, uint32_t datalen);
		void print();
		void printXML(bool d);
		void parse();
		void parseXML();

		bool verify();
		void extract();

		void setProductCode(uint32_t code)
		{
			this->mHeader->mProductCode = code;
		}

		void appendPartition(uint32_t flashOffset, char *filename, uint8_t *data, uint32_t dataLength);
		int32_t createImage(uint8_t **data);

	private:
		typedef struct sSWPack
		{
			uint8_t  mMagicNumber[SW_UPDATE_MAGIC_SIZE];
			uint32_t mHeaderVersion;
			uint32_t mProductCode;
			uint32_t mSWVersion;
			uint32_t mDate; /*time_t*/
			uint32_t mInventoryCount; /* not used ->loop over MAX_INVENTORY_COUNT */
			uint32_t mInvalidateFlag;
			char     mUpdateInfo[SW_UPDATE_INFO_LENGTH];
		} tSWPack;

		tSWPack     *mHeader;
		uint32_t    mDataLength;
		uint8_t     *mData;
		char        *mXML;

		uint32_t    mChildDataOffset;
		uint32_t    mChildDataLength;
		uint8_t     *mChildData;

		uint32_t    mInventoryCount;
		SwInventory *mInventory[MAX_INVENTORY_COUNT];

	private:
		uint32_t    mCurInventoryOffset;
};

#endif
