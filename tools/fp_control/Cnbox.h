#ifndef __CNBOX_H
#define __CNBOX_H

struct set_standby_s
{
	time_t localTime;
};

struct set_time_s
{
	time_t localTime;
};

struct cnbox_ioctl_data
{
	union
	{
		struct set_standby_s standby;
		struct set_time_s time;
	} u;
};

#endif
