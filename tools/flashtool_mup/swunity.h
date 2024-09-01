#ifndef SWUNITY_H_
#define SWUNITY_H_

#include <stdint.h>
#include "misc.h"

class SwUnity
{
	public:
		SwUnity();
		SwUnity(uint8_t *data, uint32_t datalen);
		void print();
		void printXML(bool d);
		int parse();
		int isValid();

		bool verify();
		void extract();

		void setProductCode(uint32_t code)
		{
			this->mHeader->mProductCode = code;
		}

		void setPartition(uint32_t flashOffset, char *filename, uint8_t *data, uint32_t dataLength);
		uint32_t getData(uint8_t **data);
		void setmDataFree(); //dbo
	private:
		void calcSH1(uint8_t **sh1_hash, uint32_t *sh1_hash_len);
		uint32_t calcCRC32(uint8_t **crc32_hash, uint32_t *crc32_hash_len);

	private:
		typedef struct sSWUnity
		{
			uint8_t  mMagicNumber[SW_UPDATE_MAGIC_SIZE];
			uint32_t mHeaderVersion;
			uint32_t mProductCode;
			uint32_t mSWVersion;
			uint32_t mDate; /*time_t*/
			uint32_t mFlashOffset;
			uint32_t mDataLength;

			char     mHashValue[20];
			uint32_t mCRC;

			uint8_t  mFileName[SW_UPDATE_FILENAME_LENGTH];
			uint8_t  mUpdateInfo[SW_UPDATE_INFO_LENGTH];
		} tSWUnity;

		// Parsed Unity Header
		tSWUnity *mHeader;

		// Unity Header + Partition Data
		uint32_t mDataLength;
		uint8_t  *mData;

		// Partition Data
		uint32_t mChildDataLength;
		uint8_t  *mChildData;
};

#endif
