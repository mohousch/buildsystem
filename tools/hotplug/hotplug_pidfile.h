#ifndef HOTPLUG_PIDFILE_H
#define HOTPLUG_PIDFILE_H

#include <sys/types.h>

int pidfile_read(pid_t *pid, const char *fmt, ...);
int pidfile_write(pid_t pid, const char *fmt, ...);
int pidfile_unlink(const char *fmt, ...);

#endif
