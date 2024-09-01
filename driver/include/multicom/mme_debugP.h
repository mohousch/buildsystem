/*
 * mme_debug.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Debug macros for MME host and companion
 */

#ifndef _MME_DEBUG_H
#define _MME_DEBUG_H

#define DEBUG_NOTIFY_STR "          " DEBUG_FUNC_STR ": "
#define DEBUG_ERROR_STR  "**ERROR** " DEBUG_FUNC_STR ": "

#if defined(__LINUX__) && defined (__KERNEL__)

#define EMBX_debug_print printk

#elif defined(__SOLARIS__) || (defined(WIN32) && !defined(__WINCE__)) || \
             (defined(__LINUX__) && !defined(__KERNEL__))

#include <assert.h>
#include <stdio.h>

#define EMBX_debug_print printf

#else

#include <assert.h>
#ifdef __cplusplus
extern "C" 
#endif
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

#ifndef MME_INFO
#define MME_INFO 0
#endif /* MME_INFO */

#ifndef MME_INFO_MANAGER
#define MME_INFO_MANAGER 0
#endif /* MME_INFO_MANAGER */

#ifndef MME_INFO_TRANSFORMER
#define MME_INFO_TRANSFORMER 0
#endif /* MME_INFO_TRANSFORMER */

#ifndef MME_INFO_LINUX_USER
#define MME_INFO_LINUX_USER 0
#endif /* MME_INFO_LINUX_USER */

#ifndef MME_INFO_LINUX_MODULE
#define MME_INFO_LINUX_MODULE 0
#endif /* MME_INFO_LINUX_MODULE */

#if defined (MME_VERBOSE)

#define MME_Info(__a, __b)   \
do {                          \
    if(__a | MME_INFO)       \
        EMBX_debug_print __b; \
} while(0)


#define MME_DebugMessage(__a)                              \
do {                                                        \
    if(EMBX_debug_enabled())                                \
    {                                                       \
        EMBX_debug_print("%s(%d): ", __FILE__, __LINE__);   \
        EMBX_debug_print __a;                               \
    }                                                       \
} while(0)

#define MME_DebugOn(__a)     \
do {                          \
    if(__a)                   \
        EMBX_debug_enable(1); \
} while(0)


#define MME_DebugOff(__a)    \
do {                          \
    if(__a)                   \
        EMBX_debug_enable(0); \
} while(0)

#if defined(__LINUX__) && defined(__KERNEL__)
#define MME_Assert(X) \
    ((X) ? \
	(void) 0 : \
	(void) printk("*** assertion failed: %s, file %s, line %d\n", #X, __FILE__, __LINE__))
#else
#define MME_Assert(X) assert(X)
#endif /* __LINUX__ */

#else /* MME_VERBOSE */

#define MME_Info(__a, __b) do { if (0 && (__a)) EMBX_debug_print __b; } while(0)
#define MME_DebugMessage(__b) do { if (0) EMBX_debug_print __a; } while(0)
#define MME_DebugOn(__a) do { if (0) ((void) (__a)); } while(0)
#define MME_DebugOff(__a) do { if (0) ((void) (__a)); } while(0)
#define MME_Assert(X) ((void) (0 && (X)))

#endif /* MME_VERBOSE */


/***************************************************************
 * ALL CODE BELOW THIS POINT IS OBSOLETE AND SHOULD ULTIMATELY *
 * BE REMOVED FROM THE CODEBASE                                *
 ***************************************************************/

#if defined _ICC || defined _MSC_VER
#define __inline__ __inline
#endif
/* empty function (optimized to nothing) */

#if MME_INFO

void MME_Debug_Print (const char  * szFormat, ...);	/* standard prototype for debug output */
#define DP MME_Debug_Print

#else

/* Empty debug function, this functions is inlined and optimized to nothing. Calls to it
 * will therefore not have any effect on release builds.
 */
static __inline__ void DebugPrintEmpty(const char * szFormat, ...) { (void) &DebugPrintEmpty; }
#define DP while(0) DebugPrintEmpty

#endif

#endif /* _MME_DEBUG_H */

