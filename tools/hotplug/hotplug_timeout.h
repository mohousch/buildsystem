#ifndef HOTPLUG_TIMEOUT_H
#define HOTPLUG_TIMEOUT_H

struct timeout
{
	unsigned long val;
};

void timeout_init(struct timeout *t, unsigned long ms);
int timeout_exceeded(struct timeout *t);

#endif


