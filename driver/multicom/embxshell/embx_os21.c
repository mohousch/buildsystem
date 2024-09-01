/*******************************************************************/
/* Copyright 2008 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_os21.c                                               */
/*                                                                 */
/* Description:                                                    */
/*         Operating system abstraction implementation             */
/*                                                                 */
/*******************************************************************/

#include "embxP.h"
#include "embx_osinterface.h"
#include "debug_ctrl.h"

/* Force the weak definitions of the OS21 VMEM API into this code */
#include "vmemadapt.c"

/*
 * This header is needed to collect the prototype for memalign.
 */
#include <malloc.h>

/*----------------------------- MEMORY ALLOCATION ---------------------------*/

EMBX_VOID *EMBX_OS_ContigMemAlloc(EMBX_UINT size, EMBX_UINT align)
{
    void **alignedAddr = NULL;

    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemAlloc(size=%u, align=%u)\n",size,align));

    /* allocate an carefully aligned block of memory */
    alignedAddr = memalign(align, size);

#if defined(__sh__)
    if (alignedAddr) {
	/* ensure there are no cache entries covering this address */
	cache_invalidate_data(alignedAddr, size);
    }
#endif

    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemAlloc = 0x%08x\n", (unsigned) alignedAddr));
    return (EMBX_VOID *)alignedAddr;
}

void EMBX_OS_ContigMemFree(EMBX_VOID *addr, EMBX_UINT size)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemFree\n"));

    free(addr);

    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemFree\n"));
}



EMBX_VOID *EMBX_OS_MemAlloc(EMBX_UINT size)
{
    void *pAddr;
    
    pAddr = (EMBX_VOID *)malloc(size);
    
    return pAddr;
}



/* This is an ANSI C like memory deallocate. 
 *
 * Because it is ANSI C like it is defined that this function will ignore
 * a NULL argument and return immediately.
 */
void EMBX_OS_MemFree(EMBX_VOID *addr)
{
    free((void *)addr);
}


/* this API provides a simple cache of initialized OS Events
 * because creating/deleting them is an expensive operation
 *
 * The trick here is to define the OS EMBX_EVENT as a OS level event
 * handle (e.g. semaphore) plus a next pointer. This descriptor can then
 * be added and removed from a linked list on demand
 */
static EMBX_EVENT cacheHead = NULL;
static EMBX_MUTEX cacheLock;

static unsigned cacheInitialised = 0;

EMBX_EVENT EMBX_OS_EventCreate(void)
{
    EMBX_EVENT ev = NULL;

    /* This is not multithread safe - perhaps use an atomic test&set ? */
    if (!cacheInitialised)
    {
        cacheInitialised = 1;
	(void) EMBX_OS_MUTEX_INIT(&cacheLock);
    }
    
    EMBX_OS_MUTEX_TAKE(&cacheLock);
    
    if ((ev = cacheHead) != NULL)
    {
        cacheHead = cacheHead->next;

	/* Mark desc as being in use */
	ev->next = (EMBX_EVENT) EMBX_HANDLE_VALID;

	EMBX_OS_MUTEX_RELEASE(&cacheLock);
    }
    else
    {
        EMBX_OS_MUTEX_RELEASE(&cacheLock);
      
	/* Dynamically allocate a new cache container
	 * and initialise the event inside it
	 */
	ev = EMBX_OS_MemAlloc(sizeof(*ev));
	
	if (ev)
	{
	    /* Mark desc as being in use */
	    ev->next = (EMBX_EVENT) EMBX_HANDLE_VALID;

	    ev->event = semaphore_create_fifo(0);
	}
    }
    
    return ev;
}

void EMBX_OS_EventDelete(EMBX_EVENT ev)
{
    EMBX_OS_MUTEX_TAKE(&cacheLock);

    EMBX_Assert(ev);
    EMBX_Assert(ev->next == (EMBX_EVENT) EMBX_HANDLE_VALID);

    EMBX_Assert(semaphore_value(ev->event) == 0);
    
    ev->next = cacheHead;
    cacheHead = ev;
    
    EMBX_OS_MUTEX_RELEASE(&cacheLock);
}

/*
 * Block forever waiting for EVENT to be signalled,
 * or timeout after timeout (ms) have expired
 * set timeout to EMBX_TIMEOUT_INFINITE to wait forever
 *
 * Returns EMBX_SUCCESS if signalled, EMBX_SYSTEM_TIMEOUT otherwise
 */
EMBX_ERROR EMBX_OS_EventWaitTimeout(EMBX_EVENT ev, unsigned long timeout)
{
    int res;
    osclock_t time;

    EMBX_Assert(ev);

    if (timeout != EMBX_TIMEOUT_INFINITE)
    {
	/* OS21 semaphore timeout takes an absolute time value
	 * so we calculate that here
	 */
	time = time_plus(time_now(), (timeout*time_ticks_per_sec())/1000);
	res = semaphore_wait_timeout(ev->event, &time);
    }
    else
	/* Infinite wait */
	res = semaphore_wait(ev->event);

    if (res == OS21_FAILURE)
	return EMBX_SYSTEM_TIMEOUT;

    return EMBX_SUCCESS;
}

/*
 * Signal an EVENT
 */
void EMBX_OS_EventSignal(EMBX_EVENT ev)
{
    EMBX_Assert(ev);

    semaphore_signal(ev->event);
}

/*------------------------ MEMORY ADDRESS TRANSLATION -----------------------*/

EMBX_VOID *EMBX_OS_PhysMemMap(EMBX_UINT pMem, int size, int cached)
{
    EMBX_VOID *vaddr = NULL;

    unsigned mode;

    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemMap(0x%08x, %d)\n", (unsigned int) pMem, size));

    mode = VMEM_CREATE_READ|VMEM_CREATE_WRITE;
    
    if (cached)
	mode |= VMEM_CREATE_CACHED;
    else
	mode |= VMEM_CREATE_UNCACHED | VMEM_CREATE_NO_WRITE_BUFFER;
    
    vaddr = vmem_create((EMBX_VOID *)pMem, size, NULL, mode);
    
    if (NULL == vaddr) {
	EMBX_DebugMessage(("PhysMemMap: pMem %p size %d cached %d failed\n", pMem, size, cached));
    }

    EMBX_Info(EMBX_INFO_OS, ("PhysMemMap: *vMem = %p\n", vaddr));
    
    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemMap\n"));
    
    return vaddr;
}



void EMBX_OS_PhysMemUnMap(EMBX_VOID *vMem)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemUnMap\n"));

    vmem_delete(vMem);

    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemUnMap\n"));
}

/* 
 * Translate a kernel virtual address to a phyiscal one 
 *
 * Returns either EMBX_SUCESS or EMBX_INCOHERENT_MEMORY when translation succeeds
 * Returns EMBX_INVALID_ARGUMENT otherwise
 */
EMBX_ERROR EMBX_OS_VirtToPhys(EMBX_VOID *vaddr, EMBX_UINT *paddrp)
{
    unsigned mode;
    
    /* OS21: This should work in both 29-bit and 32-bit modes */
    if (vmem_virt_to_phys(vaddr, (EMBX_VOID *)paddrp))
	return EMBX_INVALID_ARGUMENT;
    
    if (vmem_virt_mode(vaddr, &mode))
	/* Lookup failed - assume incoherent */
	return EMBX_INCOHERENT_MEMORY;
    
    if ((mode & VMEM_CREATE_CACHED) || (mode & VMEM_CREATE_WRITE_BUFFER))
	return EMBX_INCOHERENT_MEMORY;
    
    /* Success and memory is coherent */
    return EMBX_SUCCESS;
}

#ifdef __ST231__
/*
 * ST200 specific functions to enable and disable the memory speculation feature
 */
EMBX_ERROR EMBX_OS_EnableSpeculation(EMBX_VOID *pAddr, EMBX_UINT size)
{
    int res;

    res = scu_enable_range(pAddr, size);

    return (res ? EMBX_SYSTEM_ERROR : EMBX_SUCCESS);
}

EMBX_ERROR EMBX_OS_DisableSpeculation(EMBX_VOID *pAddr)
{
    int res;
    
    res = scu_disable_range(pAddr);

    return (res ? EMBX_SYSTEM_ERROR : EMBX_SUCCESS);
}

#endif /* __ST231__ */

#ifdef __sh__
/*
 * SH4 specific code which detects whether SE mode (32-bit) is in operation
 */
EMBX_BOOL EMBX_OS_SEMode(void)
{
    extern int _st40_vmem_enhanced_mode (void);

    return _st40_vmem_enhanced_mode() ? EMBX_TRUE : EMBX_FALSE;
}
#endif /* __sh__ */

/*--------------------------- THREAD CREATION -------------------------------*/

static char *_default_name = "embx";

static char *get_default_name(void)
{
    /* Trivial for now, think about adding an incrementing
     * number to the name, but how do we deal with mutual
     * exclusion, or don't we care for this?
     */
    return _default_name;
}

EMBX_THREAD EMBX_OS_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name)
{
    task_t *t;

    EMBX_Info(EMBX_INFO_OS, (">>>>ThreadCreate\n"));

    if(name == EMBX_DEFAULT_THREAD_NAME)
    {
        name = get_default_name();
    }

    t = task_create(thread, param, EMBX_DEFAULT_THREAD_STACK_SIZE, priority, name, 0);
    if(t == EMBX_INVALID_THREAD)
    {
        EMBX_DebugMessage(("ThreadCreate: task_create failed.\n"));
    }

    EMBX_Info(EMBX_INFO_OS, ("<<<<ThreadCreate\n"));

    return t;
}

EMBX_ERROR EMBX_OS_ThreadDelete(EMBX_THREAD thread)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>EMBX_OS_ThreadDelete\n"));

    if(thread == EMBX_INVALID_THREAD) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS (invalid task)\n"));
        return EMBX_SUCCESS;
    }

    if(task_wait(&thread, 1, TIMEOUT_INFINITY) != 0)
    {
        EMBX_DebugMessage(("EMBX_OS_ThreadDelete: task_wait failed.\n"));
        return EMBX_SYSTEM_INTERRUPT;        
    }

    task_delete(thread);

    EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
