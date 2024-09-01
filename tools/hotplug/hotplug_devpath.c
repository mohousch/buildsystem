#include <stdio.h>
#include "hotplug_basename.h"
#include "hotplug_devpath.h"

bool hotplug_devpath_to_devnode(const char devpath[], char devnode[], size_t size)
{
	const char *str;
	int ret;

	str = hotplug_basename(devpath);
	if (!str)
		return false;

	ret = snprintf(devnode, size, "/dev/%s", str);
	if (ret < 0)
		return false;

	return ((size_t)ret <= size);
}

