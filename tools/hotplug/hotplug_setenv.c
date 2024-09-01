#include <stdlib.h>
#include "hotplug_setenv.h"

int hotplug_setenv_bool(const char *name, bool b)
{
	return setenv(name, b ? "1" : "0", 1);
}

