#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#include "misc.h"

char *strTime(uint32_t /*time_t*/ now)
{
	struct tm *ts;
	char buf[80];
	time_t tmp_now = (time_t)now;
	ts = localtime(&tmp_now);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
	return strdup(buf);
}

void verboseprintf(const char *printstring)
{
	if (verbose)
	{
		printf("%s", printstring);
	}
}
