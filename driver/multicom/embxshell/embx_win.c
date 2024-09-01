/*******************************************************************/
/* Copyright 2008 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_win.c                                                */
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
    
    /* Currently this isn't supported on WIN32/WinCE */
    EMBX_DebugMessage(("ContigMemAlloc: Not supported on WinCE\n"));
    
    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemAlloc = 0x%08x\n", (unsigned) alignedAddr));
    return (EMBX_VOID *)alignedAddr;
}


void EMBX_OS_ContigMemFree(EMBX_VOID *addr, EMBX_UINT size)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>ContigMemFree\n"));

    /* Currently this isn't supported on WIN32/WinCE */
    EMBX_DebugMessage(("ContigMemFree: Not supported on WinCE\n"));

    EMBX_Info(EMBX_INFO_OS, ("<<<<ContigMemFree\n"));
}



EMBX_VOID *EMBX_OS_MemAlloc(EMBX_UINT size)
{
    void *pAddr;

#if defined(__WINCE__)
    pAddr = (EMBX_VOID *)VirtualAlloc( NULL,
                                       (DWORD)size,
                                       (MEM_COMMIT | MEM_RESERVE),
                                       PAGE_READWRITE );
#else

    pAddr = (EMBX_VOID *)malloc(size);

#endif

    return pAddr;
}



/* This is an ANSI C like memory deallocate. 
 *
 * Because it is ANSI C like it is defined that this function will ignore
 * a NULL argument and return immediately.
 */
void EMBX_OS_MemFree(EMBX_VOID *addr)
{
#if defined(__WINCE__)
    if (addr)
    {
        if( VirtualFree((LPVOID)addr, (DWORD)0, MEM_RELEASE) )
        {
            VirtualFree((LPVOID)addr, (DWORD)0, MEM_DECOMMIT );
        }
        else
        {
            EMBX_DebugMessage(("MemFree: VirtualFree failed\n"));
        }
    }
#else

    free((void *)addr);

#endif

}


/* this API provides a simple cache of initialized OS Events
 * because creating/deleting them are an expensive operations
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

    /* XXXX: This is unsafe - perhaps use an atomic test&set ?? */
    if (!cacheInitialised)
    {
        cacheInitialised = 1;
	EMBX_OS_MUTEX_INIT(&cacheLock);
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

	    ev->event = CreateEvent(NULL,FALSE,FALSE,NULL);
	}
    }
    
    return ev;
}

void EMBX_OS_EventDelete(EMBX_EVENT ev)
{
    EMBX_OS_MUTEX_TAKE(&cacheLock);

    EMBX_Assert(ev);
    EMBX_Assert(ev->next == (EMBX_EVENT) EMBX_HANDLE_VALID);
    
    ev->next = cacheHead;
    cacheHead = ev;
    
    EMBX_OS_MUTEX_RELEASE(&cacheLock);
}

/*
 * Block forever waiting for EVENT to be signalled,
 * or timeout after timeout (ms) have expired
 * set timeout to EMBX_TIMEOUT_INFINITE to wait forever
 *
 * Returns EMBX_SUCCESS if signalled, EMBX_SYSTEM_TIMEOUT or EMBX_SYSTEM_INTERRUPT otherwise
 */
EMBX_ERROR EMBX_OS_EventWaitTimeout(EMBX_EVENT ev, unsigned long timeout)
{
    DWORD  res;

    if (timeout == EMBX_TIMEOUT_INFINITE)
	res = WaitForSingleObject(ev->event, INFINITE);
    else
	res = WaitForSingleObject(ev->event, timeout);
	
    if (res == WAIT_OBJECT_0)
	/* Event was signalled */
	return EMBX_SUCCESS;
    
    /* Did wait timeout or get aborted ? */
    return ((res == WAIT_TIMEOUT) ? EMBX_SYSTEM_TIMEOUT : EMBX_SYSTEM_INTERRUPT);
}

/*
 * Signal an EVENT
 */
void EMBX_OS_EventSignal(EMBX_EVENT ev)
{
    SetEvent(ev->event);
}


/*------------------------ MEMORY ADDRESS TRANSLATION -----------------------*/

EMBX_VOID *EMBX_OS_PhysMemMap(EMBX_UINT pMem, int size, int cached)
{
    EMBX_VOID *vaddr = NULL;

    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemMap(0x%08x, %d)\n", (unsigned int) pMem, size));

#if defined(__WINCE__)
    {
	SYSTEM_INFO sysinfo;
	unsigned int pagesize, alignment, virt, physical;
	
	/*
	 * get the system page size
	 */
	GetSystemInfo(&sysinfo);
	pagesize = sysinfo.dwPageSize;
	
	/*
	 * warp the physical address to be aligned on a page boundary
	 * and increase size acordingly
	 */ 
	physical  = (unsigned int)pMem & ~(pagesize-1);
	alignment = (unsigned int)pMem - physical;
	size     += alignment;
	
	virt = (unsigned int) VirtualAlloc(NULL, (DWORD) size, MEM_RESERVE, PAGE_NOACCESS);
	if (virt != 0)
	{
	    if (!VirtualCopy((void *) virt, (void *) physical,(DWORD) size, 
			     (cached ? PAGE_READWRITE : (PAGE_READWRITE|PAGE_NOCACHE))))
	    {
		EMBX_DebugMessage(("PhysMemMap: VirtualCopy failed\n"));
		VirtualFree((void *) virt, 0, MEM_RELEASE);
		virt = 0;
	    }
	    else
	    {
		/*
		 * unwarp the virtual address
		 */
		virt += alignment;
		EMBX_DebugMessage(("PhysMemMap: *vMem = 0x%X\n", virt));
	    }
	}

	vaddr = (EMBX_VOID *) virt;
    }
#endif

    EMBX_Info(EMBX_INFO_OS, ("<<<<PhysMemMap\n"));
    
    return vaddr;
}



void EMBX_OS_PhysMemUnMap(EMBX_VOID *vMem)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>PhysMemUnMap\n"));

#if defined (__WINCE__)
    {

	SYSTEM_INFO sysinfo;
	unsigned int virt = (unsigned int) vMem;
	
	
	/*
	 * free the memory after warping it back to its original value
	 */
	GetSystemInfo(&sysinfo);
	if (!VirtualFree((LPVOID) (virt & ~(sysinfo.dwPageSize)), (DWORD)0, MEM_RELEASE ))
	{
	    EMBX_DebugMessage(("PhysMemUnmap: VirtualFree failed\n"));
	}
    }
#endif

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
    DWORD  ThreadId;
    HANDLE hThread;
    
    EMBX_Info(EMBX_INFO_OS, (">>>>ThreadCreate\n"));

    hThread = CreateThread( NULL,
                           (DWORD)0,
                           (LPTHREAD_START_ROUTINE)thread,
                           (LPVOID)param,
                           (DWORD)0,
                           &ThreadId );

    if( hThread != NULL )
    {
#if defined (__WINCE__)
	CeSetThreadPriority(hThread, priority);
#else
	SetThreadPriority(hThread, priority);
#endif
    }
    else
    {
        EMBX_DebugMessage(("ThreadCreate: CreateThread failed\n"));
    }

    EMBX_Info(EMBX_INFO_OS, ("<<<<ThreadCreate\n"));

    return hThread;
}

EMBX_ERROR EMBX_OS_ThreadDelete(EMBX_THREAD thread)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>EMBX_OS_ThreadDelete\n"));

    if(thread == EMBX_INVALID_THREAD) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS (invalid task)\n"));
        return EMBX_SUCCESS;
    }

    if(WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SYSTEM_INTERRUPT\n"));
        return EMBX_SYSTEM_INTERRUPT;
    }

    CloseHandle(thread);

    EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*----------------------------- INTERRUPT LOCK ------------------------------*/

static CRITICAL_SECTION interrupt_critical_section;
static int interrupt_critical_section_valid = 0;

void EMBX_OS_InterruptLock(void)
{
	/* TODO: initialization of the critical section is not thread-safe */
	if (0 == interrupt_critical_section_valid) {
		InitializeCriticalSection(&interrupt_critical_section);
		interrupt_critical_section_valid = 1;
	}

	EnterCriticalSection(&interrupt_critical_section);
}

void EMBX_OS_InterruptUnlock(void)
{
	LeaveCriticalSection(&interrupt_critical_section);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
