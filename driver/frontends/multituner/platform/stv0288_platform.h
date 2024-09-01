#ifndef stv0288_platform_123
#define stv0288_platform_123

#include "tuner.h"
#include "stv0288.h"

struct stv0288_private_data_s
{
	u32 usedLNB;
	u8  *inittab;
	int min_delay_ms;
};
#endif
// vim:ts=4