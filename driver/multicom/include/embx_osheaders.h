/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_osheaders.h                                          */
/*                                                                 */
/* Description:                                                    */
/*         All main OS includes required by the shell and          */ 
/*         transport implementations. This eliminates most         */
/*         conditional compilation requirements from individual    */
/*         source files. Note that standard C library includes     */
/*         MUST NOT be used when building for the Linux kernel.    */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBX_OSHEADERS_H
#define _EMBX_OSHEADERS_H

#if defined(__OS21__) || defined(__SOLARIS__) || \
    (defined(__LINUX__) && !defined(__KERNEL__))

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#endif


#if defined(__OS21__)
#include <os21.h>

#if defined(__sh__)
#include <os21/st40.h>
#elif defined(__ST200__)
#include <os21/st200.h>
#else 
#error Unsupported processor on OS21
#endif /* __sh__ */

/* task_context_interrupt was replaced in OS21 V2.4.X and later */
#if ( OS21_VERSION_MAJOR >= 3 ) || ( OS21_VERSION_MAJOR >= 2 && OS21_VERSION_MINOR >= 4 )
#define task_context_interrupt task_context_system
#endif

typedef unsigned int uintptr_t;

#endif /* __OS21__ */


#if defined(__LINUX__) && defined(__KERNEL__)

#include <linux/kernel.h> /* printk() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
#include <linux/config.h>
#endif
#include <linux/stddef.h>
#include <linux/ptrace.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>

#include <asm/cacheflush.h>
#include <asm/system.h>   /* cli(), *_flags */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <asm/io.h>

#include <asm/pgtable.h> /* VMALLOC_START */

#include <linux/mm.h>    /* vmalloc_to_page() */

#include <asm/unistd.h>  /* __NR_sched_setscheduler */

#include <linux/ioport.h> /* Bugzilla 4293: Need struct resource for bpa2.h */

#ifdef __SH4__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)
#include <linux/bigphysarea.h>
#else
#include <linux/bpa2.h>
#endif
#endif

/* back compatibility hack */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define MODULE_PARM_int(x) MODULE_PARM(x, "i")
#define MODULE_PARM_charp(x) MODULE_PARM(x, "s")
#define module_param(x,y,z) MODULE_PARM_##y(x)
#else
#include <linux/moduleparam.h>
#endif /* Linux < 2.6.0 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
typedef unsigned int uintptr_t;
#endif

#endif /* __LINUX__ */

#if defined(__WINCE__) || defined(WIN32)

#include <windows.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#endif /* __WINCE__ || WIN32 */

#if defined(__WINCE__) 

#include <bsp.h>
#include <sh4_intc.h>
#include <dbgapi.h>

#endif /* __WINCE__ */

#if defined (__SOLARIS__) || \
    (defined (__LINUX__) && !defined(__KERNEL__))

#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#endif /* __SOLARIS__ */
#endif /* _EMBX_OSHEADERS_H */
