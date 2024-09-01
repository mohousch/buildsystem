#ifndef HOTPLUG_DEVPATH_H
#define HOTPLUG_DEVPATH_H

#include <stdbool.h>

bool hotplug_devpath_to_devnode(const char devpath[], char devnode[], size_t size);

#endif
