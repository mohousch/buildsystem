/*******************************************************************/
/* Copyright 2008 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_solaris.c                                            */
/*                                                                 */
/* Description:                                                    */
/*         Operating system abstraction implementation             */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "debug_ctrl.h"

/*----------------------------- MEMORY ALLOCATION ---------------------------*/

EMBX_VOID *EMBX_OS_ContigMemAlloc(EMBX_UINT size, EMBX_UINT align)
{
    void *alignedAddr = NULL;

    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemAlloc(size=%u, align=%u)\n",size,align));

    /* allocate a carefully aligned block of memory */
    alignedAddr = memalign(align, size);

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


/*------------------------ MEMORY ADDRESS TRANSLATION -----------------------*/

EMBX_VOID *EMBX_OS_PhysMemMap(EMBX_UINT pMem, int size, int cached)
{
    EMBX_VOID *vaddr = NULL;

    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemMap(0x%08x, %d)\n", (unsigned int) pMem, size));

    EMBX_Info(EMBX_INFO_OS, ("PhysMemMap: *vMem = %p\n", vaddr));
    
    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemMap\n"));
    
    /* Not implemented */
    return vaddr;
}

void EMBX_OS_PhysMemUnMap(EMBX_VOID *vMem)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemUnMap\n"));

    /* Not implemented */

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
    /* Not implemented */
    return EMBX_INVALID_ARGUMENT;
}


/*--------------------------- THREAD CREATION -------------------------------*/

EMBX_THREAD EMBX_OS_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name)
{
    pthread_t *tid;

    EMBX_Info(EMBX_INFO_OS, (">>>>ThreadCreate\n"));

    tid = (pthread_t *)EMBX_OS_MemAlloc(sizeof(pthread_t));

    if(tid != EMBX_INVALID_THREAD)
    {
        if(pthread_create(tid, NULL, (void*(*)(void*))thread, param))
        {
            EMBX_DebugMessage(("ThreadCreate: task_create failed.\n"));

            EMBX_OS_MemFree(tid);
            tid = EMBX_INVALID_THREAD;
        }
    }

    EMBX_Info(EMBX_INFO_OS, ("<<<<ThreadCreate\n"));

    return tid;
}

EMBX_ERROR EMBX_OS_ThreadDelete(EMBX_THREAD thread)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>EMBX_OS_ThreadDelete\n"));

    if(thread == EMBX_INVALID_THREAD) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS (invalid task)\n"));
        return EMBX_SUCCESS;
    }

    pthread_join((pthread_t)thread, NULL);
    EMBX_OS_MemFree(thread);

    EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}


EMBX_ERROR EMBX_OS_EventCreate(EMBX_EVENT *event)
{
    return (sem_init(event, 0, 0) == 0 ? EMBX_SUCCESS : EMBX_SYSTEM_ERROR);
}

void EMBX_OS_EventDelete(EMBX_EVENT *event)
{
    sem_destroy(event);
}

/*
 * Block forever waiting for EVENT to be signalled,
 * or timeout after timeout (ms) have expired
 * set timeout to EMBX_TIMEOUT_INFINITE to wait forever
 *
 * Returns EMBX_SUCCESS if signalled, EMBX_SYSTEM_TIMEOUT or EMBX_SYSTEM_INTERRUPT otherwise
 */
EMBX_ERROR EMBX_OS_EventWaitTimeout(EMBX_EVENT *event, unsigned long timeout)
{
    int res;

    /* We don't support timed waits on Solaris */
    res = sem_wait(event);

    return ((res == 0) ? EMBX_SUCCESS : EMBX_SYSTEM_INTERRUPT);
}

void EMBX_OS_EventSignal(EMBX_EVENT *event)
{
    sem_post(event);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
