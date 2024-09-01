/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_locks.c                                      */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory spinlock abstraction                 */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"
#ifdef EMBXSHM_HARDWARE_LOCKS
#include "embxmailbox.h"
#endif

/*
 * Wait for some short period of time without accessing the bus
 */
#define BUSY_WAIT() do { volatile int i=0; while (i<1000) i++; } while (0)

/*
 * Routine to create a new spinlock
 */
EMBXSHM_Spinlock_t *EMBXSHM_createSpinlock (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_Spinlock_t *lock;

    EMBX_Info(EMBX_INFO_LOCKS, (">>>EMBX_createSpinlock\n"));

    EMBX_Assert(tpshm);

#ifdef EMBXSHM_HARDWARE_LOCKS
    if (EMBX_SUCCESS != EMBX_Mailbox_AllocLock((EMBX_MailboxLock_t **) &lock)) {
	lock = 0;
    }
#else
    lock = EMBXSHM_malloc(tpshm, sizeof(*lock));
    if (NULL != lock) 
    {
	memset (lock, 0, sizeof (*lock));
    }
#endif

    EMBX_Info(EMBX_INFO_LOCKS, ("<<<EMBX_createSpinlock = 0x%08x\n", (unsigned) lock));
    return lock;
}


/*
 * Delete a spinlock
 */
void EMBXSHM_deleteSpinlock (EMBXSHM_Transport_t *tpshm,
                             EMBXSHM_Spinlock_t         *lock)
{
    EMBX_Info(EMBX_INFO_LOCKS, (">>>EMBX_deleteSpinLock\n"));
    EMBX_Assert(tpshm);
    EMBX_Assert(lock);
#ifdef EMBXSHM_HARDWARE_LOCKS
    EMBX_Mailbox_FreeLock((EMBX_MailboxLock_t *) lock);
#else
    EMBXSHM_free(tpshm, lock);
#endif
    EMBX_Info(EMBX_INFO_LOCKS, ("<<<EMBX_deleteSpinLock\n"));
}

EMBXSHM_Spinlock_t *EMBXSHM_serializeSpinlock (EMBXSHM_Transport_t *tpshm,
                                               EMBXSHM_Spinlock_t *lock)
{
    EMBXSHM_Spinlock_t *res;

    EMBX_Info(EMBX_INFO_LOCKS, (">>>EMBXSHM_serializeSpinlock\n"));
    EMBX_Assert(tpshm);
    EMBX_Assert(lock);
#ifdef EMBXSHM_HARDWARE_LOCKS
{   EMBX_ERROR err = EMBX_Mailbox_GetSharedHandle((EMBX_MailboxLock_t *) lock, (EMBX_UINT *) &res);
    EMBX_Assert(EMBX_SUCCESS == err); 
}
#else
    res = LOCAL_TO_BUS (lock, tpshm->pointerWarp);
#endif
    EMBX_Info(EMBX_INFO_LOCKS, ("<<<EMBXSHM_serializeSpinlock\n"));
    return res;
}

EMBXSHM_Spinlock_t *EMBXSHM_deserializeSpinlock (EMBXSHM_Transport_t *tpshm,
                                                 EMBXSHM_Spinlock_t *lock)
{
    EMBXSHM_Spinlock_t *res;

    EMBX_Info(EMBX_INFO_LOCKS, (">>>EMBXSHM_serializeSpinlock\n"));
    EMBX_Assert(tpshm);
    EMBX_Assert(lock);
#ifdef EMBXSHM_HARDWARE_LOCKS
{   EMBX_ERROR err = EMBX_Mailbox_GetLockFromHandle((EMBX_UINT) lock, (EMBX_MailboxLock_t **) &res);
    EMBX_Assert(EMBX_SUCCESS == err); 
}
#else
    res = BUS_TO_LOCAL (lock, tpshm->pointerWarp);
#endif
    EMBX_Info(EMBX_INFO_LOCKS, ("<<<EMBXSHM_serializeSpinlock\n"));
    return res;
}


/*
 * Guaranteee addr is not held in a buffer in the memory system.
 */
void EMBXSHM_defaultBufferFlush (EMBXSHM_Transport_t *tpshm, void *addr)
{
    /* on an unbuffered system there is no work to do here */
}

/*
 * Acquire a spinlock
 */
void EMBXSHM_takeSpinlock (EMBXSHM_Transport_t *tpshm, EMBX_MUTEX *mutex, EMBXSHM_Spinlock_t *lock)
{
#ifndef EMBXSHM_HARDWARE_LOCKS
    int i;
#endif
    int cpuID;
    int cpuMax;

    EMBX_Info(EMBX_INFO_LOCKS, (">>>EMBXSHM_takeSpinlock(0x%08x)\n", (unsigned) lock));

    EMBX_Assert (tpshm);
    EMBX_Assert (mutex);
    EMBX_Assert (lock);

    EMBX_OS_MUTEX_TAKE(mutex);

#ifdef EMBXSHM_HARDWARE_LOCKS

    EMBX_Mailbox_TakeLock((EMBX_MailboxLock_t *) lock);

#else /* EMBXSHM_HARDWARE_LOCKS */

    cpuID = tpshm->cpuID;
    cpuMax = tpshm->maxCPU;

    /*
     * This assert has rather an odd form to ensure that lock->count is
     * only read once. if we write this in naive form we may find that
     * the value of lock->count may change while we are reading it.
     */
    EMBX_Assert(0 == (i = lock->count.marker) || 1 == i);
    EMBX_Assert ((0 <= cpuID) && (cpuID <= cpuMax));

#ifndef __SPARC__
retry:
    lock->n[cpuID].marker = 1;
    EMBXSHM_WROTE(&lock->n[cpuID].marker);
    for (i=0; i<cpuID; i++)
    {
	EMBXSHM_READS(&lock->n[i].marker);
        if (lock->n[i].marker)
        {
            lock->n[cpuID].marker = 0;
	    EMBXSHM_WROTE(&lock->n[cpuID].marker);

	    do {
		EMBXSHM_READS(&lock->n[i].marker);
		tpshm->buffer_flush(tpshm, (void *) &(lock->n[i].marker));
		BUSY_WAIT ();
	    } while (lock->n[i].marker);

            goto retry;
        }
    }
    for (i=(cpuID+1); i<=cpuMax; i++)
    {
	EMBXSHM_READS(&lock->n[i].marker);
        while (lock->n[i].marker)
        {
	    EMBXSHM_READS(&lock->n[i].marker);
	    tpshm->buffer_flush(tpshm, (void *) &(lock->n[i].marker));
            BUSY_WAIT ();
        }
    }
#else /* __SPARC__ */
    (void) i; /* warning suppression */
    __asm__ __volatile__ (
        "\n1:\n\t"
        "ldstub [%0], %%g2\n\t"
        "orcc   %%g2, 0x0, %%g0\n\t"
        "bz,a  3f\n\t"
        " ldub  [%0], %%g2\n\t"
        "2:\n\t"
        "orcc   %%g2, 0x0, %%g0\n\t"
        "bne,a  2b\n\t"
        " ldub  [%0], %%g2\n\t"
        "b,a    1b\n\t"
        " nop\n\t"
        "3:\n"
        : /* no outputs */
        : "r" (&lock->lock)
        : "g2", "memory", "cc");
#endif /* __SPARC__ */

#if defined EMBX_VERBOSE
    EMBXSHM_READS(&lock->count.marker);
    EMBX_Assert (0 == lock->count.marker);
    lock->count.marker++;
    EMBXSHM_WROTE(&lock->count.marker);
#endif /* EMBX_VERBOSE */

#endif /* EMBXSHM_HARDWARE_LOCKS */

    EMBX_Info(EMBX_INFO_LOCKS, ("<<<EMBXSHM_takeSpinlock\n"));
}

/*
 * Release a spinlock
 */
void EMBXSHM_releaseSpinlock (EMBXSHM_Transport_t *tpshm, EMBX_MUTEX *mutex, EMBXSHM_Spinlock_t *lock)
{
    EMBX_Info(EMBX_INFO_LOCKS, (">>>EMBXSHM_releaseSpinlock(0x%08x)\n", (unsigned) lock));

    EMBX_Assert (tpshm);
    EMBX_Assert (mutex);
    EMBX_Assert (lock);
#ifdef EMBXSHM_HARDWARE_LOCKS
    EMBX_Mailbox_ReleaseLock((EMBX_MailboxLock_t *) lock);
#else /* EMBXSHM_HARDWARE_LOCKS */
    EMBX_Assert (/*(0 <= tpshm->cpuID) &&*/ (tpshm->cpuID <= tpshm->maxCPU));

#if defined EMBX_VERBOSE
    EMBXSHM_READS(&lock->count.marker);
    EMBX_Assert (1 == lock->count.marker);
    lock->count.marker--;
    EMBXSHM_WROTE(&lock->count.marker);
#endif /* EMBX_VERBOSE */

#ifndef __SPARC__
    lock->n[tpshm->cpuID].marker = 0;
    EMBXSHM_WROTE(&lock->n[tpshm->cpuID].marker);
#else
    __asm__ __volatile__ ("stb %%g0, [%0]" : : "r" (&lock->lock) : "memory");
#endif /* __SPARC__ */
#endif /* EMBXSHM_HARDWARE_LOCKS */

    EMBX_OS_MUTEX_RELEASE (mutex);

    EMBX_Info(EMBX_INFO_LOCKS, ("<<<EMBXSHM_releaseSpinlock\n"));
}
