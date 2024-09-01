/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_osinterface.h                                        */
/*                                                                 */
/* Description:                                                    */
/*         Operating system abstraction interface for EMBX         */
/*         shell and transport implementations                     */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBX_OSINTERFACE_H
#define _EMBX_OSINTERFACE_H

#include "embx_typesP.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * - Approximate millisecond delay macro.
 */
#if defined(__OS21__)

#define EMBX_OS_Delay(_a)  task_delay( ((_a)*time_ticks_per_sec())/1000 )

#elif defined(__WINCE__) || defined(WIN32)

#define EMBX_OS_Delay(_a) Sleep((_a));

#elif defined(__LINUX__) && defined(__KERNEL__)

#define EMBX_OS_Delay(_a) do { set_current_state(TASK_INTERRUPTIBLE); schedule_timeout(((_a)*HZ)/1000); } while (0)

#elif defined(__SOLARIS__) || defined(__LINUX__)

#define EMBX_OS_Delay(_a) { \
    struct timespec __x;\
    __x.tv_sec  = _a / 1000;\
    __x.tv_nsec = (_a % 1000)*1000000;\
    nanosleep (&__x, 0);\
}

#endif /* __OS21__ */

/*
 * - Local Register (on chip peripherals) and external device memory access
 */
 
#if defined(__LINUX__) && defined(__KERNEL__)

/* These must use ST40 P2 or P4 addresses to work correctly */

#define EMBX_OS_LREG_READ32(a)    ctrl_inl((unsigned long)(a))
#define EMBX_OS_LREG_WRITE32(a,v) ctrl_outl((unsigned long)(v), (unsigned long)(a))
#define EMBX_OS_LREG_READ16(a)    ctrl_inw((unsigned long)(a))
#define EMBX_OS_LREG_WRITE16(a,v) ctrl_outw((unsigned short)(v),(unsigned long)(a))
#define EMBX_OS_LREG_READ8(a)     ctrl_inb((unsigned long)(a))
#define EMBX_OS_LREG_WRITE8(a,v)  ctrl_outb((unsigned char)(v), (unsigned long)(a))

/* These must use virtual addresses returned by ioremap to work correctly */

#define EMBX_OS_MEM_READ32(a)     readl((unsigned long)(a))
#define EMBX_OS_MEM_WRITE32(a,v)  writel((unsigned long)(v), (unsigned long)(a))
#define EMBX_OS_MEM_READ16(a)     readw((unsigned long)(a))
#define EMBX_OS_MEM_WRITE16(a,v)  writew((unsigned short)(v),(unsigned long)(a))
#define EMBX_OS_MEM_READ8(a)      readb((unsigned long)(a))
#define EMBX_OS_MEM_WRITE8(a,v)   writeb((unsigned char)(v), (unsigned long)(a))

#else

typedef volatile unsigned long  EMBX_vuint32_t;
typedef volatile unsigned short EMBX_vuint16_t;
typedef volatile unsigned char  EMBX_vuint8_t;

/* For these macros to work correctly they require different "flavours"
 * of addresses on the different platforms.
 *
 * On WinCE you must use virtual addresses mapped
 * to the uncached physical address space.
 *
 * On OS21/ST40 you must use P2 addresses.
 *
 */

#define EMBX_OS_LREG_READ32(a)    (*((EMBX_vuint32_t *) (a)))
#define EMBX_OS_LREG_WRITE32(a,v) (*((EMBX_vuint32_t *) (a)) = (v))
#define EMBX_OS_LREG_READ16(a)    (*((EMBX_vuint16_t *) (a)))
#define EMBX_OS_LREG_WRITE16(a,v) (*((EMBX_vuint16_t *) (a)) = (v))
#define EMBX_OS_LREG_READ8(a)     (*((EMBX_vuint8_t  *) (a)))
#define EMBX_OS_LREG_WRITE8(a,v)  (*((EMBX_vuint8_t  *) (a)) = (v))

#define EMBX_OS_MEM_READ32(a)     (*((EMBX_vuint32_t *) (a)))
#define EMBX_OS_MEM_WRITE32(a,v)  (*((EMBX_vuint32_t *) (a)) = (v))
#define EMBX_OS_MEM_READ16(a)     (*((EMBX_vuint16_t *) (a)))
#define EMBX_OS_MEM_WRITE16(a,v)  (*((EMBX_vuint16_t *) (a)) = (v))
#define EMBX_OS_MEM_READ8(a)      (*((EMBX_vuint8_t  *) (a)))
#define EMBX_OS_MEM_WRITE8(a,v)   (*((EMBX_vuint8_t  *) (a)) = (v))

#endif /* __LINUX__ */
 
/*
 * - Threads
 */

#define EMBX_DEFAULT_THREAD_STACK_SIZE EMBX_GetTuneable(EMBX_TUNEABLE_THREAD_STACK_SIZE)
#define EMBX_DEFAULT_THREAD_PRIORITY   EMBX_GetTuneable(EMBX_TUNEABLE_THREAD_PRIORITY)
#define EMBX_DEFAULT_MAILBOX_PRIORITY  EMBX_GetTuneable(EMBX_TUNEABLE_MAILBOX_PRIORITY)	/* WinCE IST */
#define EMBX_DEFAULT_THREAD_NAME       ((EMBX_CHAR *)0)
#define EMBX_INVALID_THREAD            ((EMBX_THREAD)0)

EMBX_THREAD EMBX_OS_ThreadCreate (void (*thread)(EMBX_VOID *), EMBX_VOID *param, EMBX_INT priority, const EMBX_CHAR *name);
EMBX_ERROR  EMBX_OS_ThreadDelete (EMBX_THREAD);

/*
 * - Memory Allocation
 */
EMBX_VOID *EMBX_OS_ContigMemAlloc (EMBX_UINT  size, EMBX_UINT align);
void       EMBX_OS_ContigMemFree  (EMBX_VOID *ptr,  EMBX_UINT size);

EMBX_VOID *EMBX_OS_MemAlloc       (EMBX_UINT  size);
void       EMBX_OS_MemFree        (EMBX_VOID *ptr);

/*
 * - Physical Memory mapping
 */
EMBX_VOID *EMBX_OS_PhysMemMap   (EMBX_UINT pMem, EMBX_INT size, EMBX_INT cached);
void       EMBX_OS_PhysMemUnMap (EMBX_VOID *vMem);

EMBX_ERROR EMBX_OS_VirtToPhys   (EMBX_VOID *vMem, EMBX_UINT *pMemp);

/*
 * ST200/OS21 specific speculation control
 */
EMBX_ERROR EMBX_OS_EnableSpeculation(EMBX_VOID *pAddr, EMBX_UINT size);
EMBX_ERROR EMBX_OS_DisableSpeculation(EMBX_VOID *pAddr);

/*
 * SH4/OS21 specific call which returns TRUE when
 * running in 32-bit Space Enhanced Mode (SE)
 */
EMBX_BOOL EMBX_OS_SEMode(void);

/*
 * - Synchronisation primitives
 */
#if defined(__OS21__)

EMBX_EVENT EMBX_OS_EventCreate(void);
void       EMBX_OS_EventDelete(EMBX_EVENT);
EMBX_ERROR EMBX_OS_EventWaitTimeout(EMBX_EVENT, unsigned long);
void       EMBX_OS_EventSignal(EMBX_EVENT);

#define EMBX_OS_MUTEX_INIT(pMutex)      ((*(pMutex) = semaphore_create_fifo(1)) != NULL)
#define EMBX_OS_MUTEX_DESTROY(pMutex)   semaphore_delete(*(pMutex))
#define EMBX_OS_MUTEX_TAKE(pMutex)      semaphore_wait(*(pMutex))
#define EMBX_OS_MUTEX_RELEASE(pMutex)   semaphore_signal(*(pMutex))

#define EMBX_OS_EVENT_INIT(pEvent)      ((*(pEvent) = EMBX_OS_EventCreate()) != NULL)
#define EMBX_OS_EVENT_DESTROY(pEvent)   EMBX_OS_EventDelete(*(pEvent))
#define EMBX_OS_EVENT_WAIT(pEvent)      EMBX_OS_EventWaitTimeout(*(pEvent), EMBX_TIMEOUT_INFINITE)
#define EMBX_OS_EVENT_WAIT_TIMEOUT(pEvent,t) EMBX_OS_EventWaitTimeout(*(pEvent), (t))
#define EMBX_OS_EVENT_POST(pEvent)      EMBX_OS_EventSignal(*(pEvent))

#define EMBX_OS_INTERRUPT_LOCK()        interrupt_lock()
#define EMBX_OS_INTERRUPT_UNLOCK()      interrupt_unlock()

/*
 * Clock/Time functions
 */
#define EMBX_OS_TIME			osclock_t
#define EMBX_OS_TIME_NOW()		time_now()
#define EMBX_OS_TIME_AFTER(A, B)	time_after((A), (B))
#define EMBX_OS_TIME_EXPIRED(S,T)	time_after(time_now(), time_plus((S), ((T)*time_ticks_per_sec())/1000))

#elif defined(__WINCE__) || defined(WIN32)

EMBX_EVENT EMBX_OS_EventCreate(void);
void       EMBX_OS_EventDelete(EMBX_EVENT);
EMBX_ERROR EMBX_OS_EventWaitTimeout(EMBX_EVENT, unsigned long);
void       EMBX_OS_EventSignal(EMBX_EVENT);

#define EMBX_OS_MUTEX_INIT(pMutex)      (InitializeCriticalSection((pMutex)), EMBX_TRUE)
#define EMBX_OS_MUTEX_DESTROY(pMutex)   DeleteCriticalSection((pMutex))
#define EMBX_OS_MUTEX_TAKE(pMutex)      EnterCriticalSection((pMutex))
#define EMBX_OS_MUTEX_RELEASE(pMutex)   LeaveCriticalSection((pMutex))

#define EMBX_OS_EVENT_INIT(pEvent)      ((*(pEvent) = EMBX_OS_EventCreate()) != NULL)
#define EMBX_OS_EVENT_DESTROY(pEvent)   EMBX_OS_EventDelete(*(pEvent))
#define EMBX_OS_EVENT_WAIT(pEvent)      (EMBX_OS_EventWaitTimeout(*(pEvent), EMBX_TIMEOUT_INFINITE))
#define EMBX_OS_EVENT_WAIT_TIMEOUT(pEvent,t) (EMBX_OS_EventWaitTimeout(*(pEvent), (t)))
#define EMBX_OS_EVENT_POST(pEvent)      EMBX_OS_EventSignal(*(pEvent))

void EMBX_OS_InterruptLock(void);
void EMBX_OS_InterruptUnlock(void);
#define EMBX_OS_INTERRUPT_LOCK()	EMBX_OS_InterruptLock();
#define EMBX_OS_INTERRUPT_UNLOCK()	EMBX_OS_InterruptUnlock();

#elif defined(__LINUX__) && defined(__KERNEL__)

EMBX_ERROR EMBX_OS_EventCreate(EMBX_EVENT *);
void       EMBX_OS_EventDelete(EMBX_EVENT *);
EMBX_ERROR EMBX_OS_EventWaitTimeout(EMBX_EVENT *, unsigned long);
void       EMBX_OS_EventSignal(EMBX_EVENT *);

#define EMBX_OS_MUTEX_INIT(pMutex)      (sema_init((pMutex),1) , EMBX_TRUE)
#define EMBX_OS_MUTEX_DESTROY(pMutex)
#define EMBX_OS_MUTEX_TAKE(pMutex)      down((pMutex))
#define EMBX_OS_MUTEX_RELEASE(pMutex)   up((pMutex))

#define EMBX_OS_EVENT_INIT(pEvent)      (EMBX_OS_EventCreate((pEvent)) == EMBX_SUCCESS)
#define EMBX_OS_EVENT_DESTROY(pEvent)	
#define EMBX_OS_EVENT_WAIT(pEvent)      EMBX_OS_EventWaitTimeout((pEvent), EMBX_TIMEOUT_INFINITE)
#define EMBX_OS_EVENT_WAIT_TIMEOUT(pEvent,t) EMBX_OS_EventWaitTimeout((pEvent),(t))
#define EMBX_OS_EVENT_POST(pEvent)      EMBX_OS_EventSignal((pEvent))

#define EMBX_OS_INTERRUPT_LOCK()	local_irq_disable();
#define EMBX_OS_INTERRUPT_UNLOCK()	local_irq_enable();

/*
 * Clock/Time functions
 */
#define EMBX_OS_TIME			unsigned long
#define EMBX_OS_TIME_NOW()		jiffies
#define EMBX_OS_TIME_AFTER(A, B)	time_after((A), (B))
#define EMBX_OS_TIME_EXPIRED(S,T)	time_after(jiffies, (S)+msecs_to_jiffies((T)))

#elif defined(__SOLARIS__) || defined(__LINUX__)

EMBX_ERROR EMBX_OS_EventCreate(EMBX_EVENT *);
void       EMBX_OS_EventDelete(EMBX_EVENT *);
EMBX_ERROR EMBX_OS_EventWaitTimeout(EMBX_EVENT *, unsigned long);
void       EMBX_OS_EventSignal(EMBX_EVENT *);

#define EMBX_OS_MUTEX_INIT(pMutex)      (sem_init((pMutex), 0, 1) == 0)
#define EMBX_OS_MUTEX_DESTROY(pMutex)   sem_destroy(pMutex)
#define EMBX_OS_MUTEX_TAKE(pMutex)      sem_wait((pMutex))
#define EMBX_OS_MUTEX_RELEASE(pMutex)   sem_post((pMutex))

#define EMBX_OS_EVENT_INIT(pEvent)      (EMBX_OS_EventCreate((pEvent)) == EMBX_SUCCESS)
#define EMBX_OS_EVENT_DESTROY(pEvent)	EMBX_OS_EventDelete((pEvent))
#define EMBX_OS_EVENT_WAIT(pEvent)      EMBX_OS_EventWaitTimeout((pEvent), EMBX_TIMEOUT_INFINITE)
#define EMBX_OS_EVENT_WAIT_TIMEOUT(pEvent,t) EMBX_OS_EventWaitTimeout((pEvent),(t))
#define EMBX_OS_EVENT_POST(pEvent)      EMBX_OS_EventSignal((pEvent))

/* Interrupt lock is not usually supported on Solaris so these
 * must be supplied by the simulation environment. Their
 * implementations will vary dependant on the ways asynchronous
 * messages are delivered in a particular simulation.
 */
void interrupt_lock   (void);
void interrupt_unlock (void);
#define EMBX_OS_INTERRUPT_LOCK()        interrupt_lock()
#define EMBX_OS_INTERRUPT_UNLOCK()      interrupt_unlock()

#else

#error Undefined OS Environment

#endif /* __OS21__ */


/* Driver reference counting, currently only needed for Linux
 * but may be useful in WinCE in the future.
 */
#if defined(__LINUX__) && defined(__KERNEL__)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
#define EMBX_OS_MODULE_REF()    MOD_INC_USE_COUNT
#define EMBX_OS_MODULE_UNREF()  MOD_DEC_USE_COUNT
#else
#define EMBX_OS_MODULE_REF()
#define EMBX_OS_MODULE_UNREF()
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
#define SET_MODULE_OWNER(some_struct) do { } while (0)
#endif

#else

#define EMBX_OS_MODULE_REF()
#define EMBX_OS_MODULE_UNREF()

#endif /* __LINUX__ */

/*
 * Cache management.
 */
#if defined __OS21__ && defined __sh__

#define EMBX_OS_PurgeCache(ptr, sz) do { if (sz) cache_purge_data((ptr), (sz)); } while(0)
#if 1
#define EMBX_OS_FlushCache(ptr, sz) do { if (sz) cache_flush_data((ptr), (sz)); } while(0)
#else
#define EMBX_OS_FlushCache(ptr, sz) EMBX_OS_PurgeCache(ptr, sz)
#endif


#define EMBX_OS_PurgeCacheWithPrecomputableLength(ptr, sz) \
        do { \
                if (sz <= 32) { \
                        EMBX_Assert((32 - (((unsigned) (ptr)) & 31)) <= 32); \
                        __asm__ __volatile__ ("ocbp\t@%0\n" ::"r" (ptr)); \
                } else { \
                        cache_purge_data((ptr), sz); \
                } \
        } while (0)
#if 1
#define EMBX_OS_FlushCacheWithPrecomputableLength(ptr, sz) \
	do { \
		if (sz <= 32) { \
			EMBX_Assert((32 - (((unsigned) (ptr)) & 31)) <= 32); \
			__asm__ __volatile__ ("ocbwb\t@%0\n" ::"r" (ptr)); \
		} else { \
			cache_flush_data((void *) (ptr), sz); \
		} \
	} while (0)
#else
#define EMBX_OS_FlushCacheWithPrecomputableLength(ptr, sz) \
	EMBX_OS_PurgeCacheWithPrecomputableLength(ptr, sz)
#endif

#elif defined __OS21__ && defined __ST200__

EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz);
#define EMBX_OS_FlushCache(ptr, sz) EMBX_OS_PurgeCache(ptr, sz)

#define EMBX_OS_PurgeCacheWithPrecomputableLength(ptr, sz) \
	do { \
		if (sz <= 32) { \
			EMBX_Assert((32 - (((unsigned) (ptr)) & 31)) <= 32); \
			__asm__ __volatile__ ("prgadd 0[%0]; ;; ; sync" :: "r" (ptr)); \
		} else { \
			EMBX_OS_PurgeCache((void *) (ptr), sz); \
		} \
	} while (0)
#define EMBX_OS_FlushCacheWithPrecomputableLength(ptr, sz) \
	EMBX_OS_PurgeCacheWithPrecomputableLength(ptr, sz)

#elif defined __LINUX__ && defined __KERNEL__

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
#define EMBX_OS_PurgeCache(ptr, sz) dma_cache_wback_inv(ptr, sz)
#define EMBX_OS_FlushCache(ptr, sz) dma_cache_wback(ptr, sz)
#else
#define EMBX_OS_PurgeCache(ptr, sz) __flush_purge_region(ptr, sz)
#define EMBX_OS_FlushCache(ptr, sz) __flush_wback_region(ptr, sz)
#endif

#define EMBX_OS_PurgeCacheWithPrecomputableLength(ptr, sz) EMBX_OS_PurgeCache(ptr, sz)
#define EMBX_OS_FlushCacheWithPrecomputableLength(ptr, sz) EMBX_OS_FlushCache(ptr, sz)

#elif defined __LINUX__ && !defined __KERNEL__

EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz);
#define EMBX_OS_FlushCache(ptr, sz) EMBX_OS_PurgeCache(ptr, sz)

#define EMBX_OS_PurgeCacheWithPrecomputableLength(ptr, sz) EMBX_OS_PurgeCache(ptr, sz)
#define EMBX_OS_FlushCacheWithPrecomputableLength(ptr, sz) EMBX_OS_FlushCache(ptr, sz)

#elif (defined __WINCE__ || defined WIN32)

EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz);
#define EMBX_OS_FlushCache(ptr, sz) EMBX_OS_PurgeCache(ptr, sz)

#define EMBX_OS_PurgeCacheWithPrecomputableLength(ptr, sz) EMBX_OS_PurgeCache(ptr, sz)
#define EMBX_OS_FlushCacheWithPrecomputableLength(ptr, sz) EMBX_OS_FlushCache(ptr, sz)

#endif /* Cache management facilities */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#if ! defined __STDC_VERSION__
#if defined _ICC
#define EMBX_INLINE __inline
#elif defined __GNUC__
#define EMBX_INLINE __inline__
#else
#define EMBX_INLINE
#endif
#else
#if __STDC_VERSION__ >= 199901L
#define EMBX_INLINE inline
#elselock->n[cpuID].marker
#define EMBX_INLINE
#endif
#endif

#endif /* _EMBX_OSINTERFACE_H */
