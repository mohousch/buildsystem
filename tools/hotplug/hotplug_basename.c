#include <string.h>
#include "hotplug_basename.h"

const char *hotplug_basename(const char *path)
{
	const char *name;

	name = strrchr(path, '/');
	if (name == NULL)
		return path;

	return &name[1];
}

