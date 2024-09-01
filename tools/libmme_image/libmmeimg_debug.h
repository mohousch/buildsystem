#ifndef _LIBMMEIMG_DEBUG_H
#define _LIBMMEIMG_DEBUG_H

#include <stdio.h>
#define DEBUG
#ifdef  DEBUG
#define DEBUG_PRINT(format, ...) printf("(%s): " format "\n", __func__, ## __VA_ARGS__)
#else
#define DEBUG_PRINT(format,...)
#endif

#endif
