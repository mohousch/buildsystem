#ifndef MISC_H_
#define MISC_H_

#include <time.h>
#include <string.h>
#include <stdint.h>

#define SW_MAGIC_VALUE "MARUSWUP"

enum
{
	SW_UPDATE_HEADER_SIZE     = 512,
	SW_UPDATE_CRYPTO_SIZE     = 256,
	SW_UPDATE_MAGIC_SIZE      = 8,
	SW_UPDATE_INFO_LENGTH     = 128,
	SW_UPDATE_FILENAME_LENGTH = 32,

	SW_INVENTORY_SIZE         = 64,
	MAX_INVENTORY_COUNT       = 8,

	SW_UPDATER_VERSION        = 100,
};

char *strTime(uint32_t /*time_t*/ now);
void verboseprintf(const char *printstring);
extern bool verbose;

#endif
