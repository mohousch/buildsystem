/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_debug.h                                              */
/*                                                                 */
/* Description:                                                    */
/*    Debug macros for EMBX driver and transports                  */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBX_DEBUG_H
#define _EMBX_DEBUG_H

#if defined(__LINUX__)

#if defined(__KERNEL__)

#define EMBX_debug_print printk

#else

#include <stdio.h>
#define EMBX_debug_print printf

#endif /* __KERNEL__ */

#elif defined(__SOLARIS__) || (defined(WIN32) && !defined(__WINCE__))

#include <stdio.h>

#define EMBX_debug_print printf

#else

void EMBX_debug_print(char *format,...);

#endif /* __LINUX__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int  EMBX_debug_enabled (void);
extern void EMBX_debug_enable  (int);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#if defined (EMBX_VERBOSE)

#ifndef EMBX_INFO
#define EMBX_INFO 0
#endif /* EMBX_INFO */

#define EMBX_Info(__a, __b)   \
do {                          \
    if(__a | EMBX_INFO)       \
        EMBX_debug_print __b; \
} while(0)


#define EMBX_DebugMessage(__a)                              \
do {                                                        \
    if(EMBX_debug_enabled())                                \
    {                                                       \
        EMBX_debug_print("%s(%d): ", __FILE__, __LINE__);   \
        EMBX_debug_print __a;                               \
    }                                                       \
} while(0)

#define EMBX_DebugOn(__a)     \
do {                          \
    if(__a)                   \
        EMBX_debug_enable(1); \
} while(0)


#define EMBX_DebugOff(__a)    \
do {                          \
    if(__a)                   \
        EMBX_debug_enable(0); \
} while(0)

#if defined(__LINUX__) && defined(__KERNEL__)
#define EMBX_Assert(X) \
    ((X) ? \
	(void) 0 : \
	(void) printk("*** assertion failed: %s, file %s, line %d\n", #X, __FILE__, __LINE__))
#else
#define EMBX_Assert(X) assert(X)
#endif /* __LINUX__ */

#else /* EMBX_VERBOSE */


#define EMBX_Info(__a, __b)
#define EMBX_DebugMessage(__b)
#define EMBX_DebugOn(__a)
#define EMBX_DebugOff(__a)
#define EMBX_Assert(X)

#endif /* EMBX_VERBOSE */

#endif /* _EMBX_DEBUG_H */

