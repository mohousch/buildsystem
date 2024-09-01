/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_init.c                                       */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport                            */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"
#include "embxP.h"

#define EMBXSHM_DEFAULT_PORT_TABLE_SIZE 32

/*
 * Cleaup a transport's synchronization objects, this is
 * public as it is also used during closedown.
 */
void EMBXSHM_destroyTransportSyncObjects (EMBXSHM_Transport_t *tpshm)
{
    EMBX_Info(EMBX_INFO_INIT, (">>>EMBXSHM_destroyTransportSyncObjects(0x%08x)\n", (unsigned) tpshm));

    if(tpshm->locksValid)
    {
        EMBX_OS_MUTEX_DESTROY (&tpshm->connectionListMutex);
        EMBX_OS_MUTEX_DESTROY (&tpshm->portTableMutex);
        EMBX_OS_MUTEX_DESTROY (&tpshm->objectTableMutex);
        EMBX_OS_MUTEX_DESTROY (&tpshm->portConnectMutex);
        EMBX_OS_EVENT_DESTROY (&tpshm->secondHalfEvent);
        EMBX_OS_MUTEX_DESTROY (&tpshm->secondHalfMutex);
        EMBX_OS_EVENT_DESTROY (&tpshm->closedPortEvent);
        EMBX_OS_MUTEX_DESTROY (&tpshm->tpLock);

        tpshm->locksValid = EMBX_FALSE;
    }

    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_destroyTransportSyncObjects\n"));
}



/*
 * Create all transport muxtex and event objects
 */
static EMBX_ERROR createTransportSyncObjects (EMBXSHM_Transport_t *tpshm)
{
    EMBX_BOOL tpLock              = EMBX_FALSE;
    EMBX_BOOL connectionListMutex = EMBX_FALSE;
    EMBX_BOOL portTableMutex      = EMBX_FALSE;
    EMBX_BOOL objectTableMutex    = EMBX_FALSE;
    EMBX_BOOL portConnectMutex    = EMBX_FALSE;
    EMBX_BOOL secondHalfEvent     = EMBX_FALSE;
    EMBX_BOOL secondHalfMutex     = EMBX_FALSE;
    EMBX_BOOL closedPortEvent     = EMBX_FALSE;

    EMBX_Info(EMBX_INFO_INIT, (">>>createTransportSyncObjects(0x%08x)\n", (unsigned) tpshm));

    /*
     * Clean up any existing mutexes and events. We may have been
     * called after a partially completed initialization.
     */
    EMBXSHM_destroyTransportSyncObjects(tpshm);


    /*
     * Initialise all our local locks
     */
    if (0 == (tpLock = EMBX_OS_MUTEX_INIT (&tpshm->tpLock)))
        goto error_exit;


    if (0 == (connectionListMutex = EMBX_OS_MUTEX_INIT (&tpshm->connectionListMutex)))
        goto error_exit;

    
    if (0 == (portTableMutex = EMBX_OS_MUTEX_INIT (&tpshm->portTableMutex)))
        goto error_exit;

    
    if (0 == (objectTableMutex = EMBX_OS_MUTEX_INIT (&tpshm->objectTableMutex)))
        goto error_exit;

    
    if (0 == (portConnectMutex = EMBX_OS_MUTEX_INIT (&tpshm->portConnectMutex)))
        goto error_exit;

    
    if (0 == (secondHalfEvent = EMBX_OS_EVENT_INIT (&tpshm->secondHalfEvent)))
        goto error_exit;

    
    if (0 == (secondHalfMutex = EMBX_OS_MUTEX_INIT (&tpshm->secondHalfMutex)))
        goto error_exit;

    
    if (0 == (closedPortEvent = EMBX_OS_EVENT_INIT (&tpshm->closedPortEvent)))
        goto error_exit;


    tpshm->locksValid = EMBX_TRUE;

    EMBX_Info(EMBX_INFO_INIT, ("<<<createTransportSyncObjects = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;


error_exit:

    if (tpLock)
        EMBX_OS_MUTEX_DESTROY (&tpshm->tpLock);


    if (connectionListMutex)
        EMBX_OS_MUTEX_DESTROY (&tpshm->connectionListMutex);


    if (portTableMutex)
        EMBX_OS_MUTEX_DESTROY (&tpshm->portTableMutex);


    if (objectTableMutex)
        EMBX_OS_MUTEX_DESTROY (&tpshm->objectTableMutex);


    if (portConnectMutex)
        EMBX_OS_MUTEX_DESTROY (&tpshm->portConnectMutex);


    if (secondHalfEvent)
        EMBX_OS_EVENT_DESTROY (&tpshm->secondHalfEvent);


    if (secondHalfMutex)
        EMBX_OS_MUTEX_DESTROY (&tpshm->secondHalfMutex);


    if (closedPortEvent)
        EMBX_OS_EVENT_DESTROY (&tpshm->closedPortEvent);


    EMBX_Info(EMBX_INFO_INIT, ("<<<createTransportSyncObjects = EMBX_NOMEM\n"));
    return EMBX_NOMEM;
}



EMBX_ERROR EMBXSHM_genericSlaveInit (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_TCB_t *tcb;

    EMBX_Info(EMBX_INFO_INIT, (">>>EMBXSHM_genericSlaveInit(0x%08x)\n", (unsigned) tpshm));

    EMBX_Assert (tpshm);
    EMBX_Assert (tpshm->tcb && EMBXSHM_ALIGNED_TO_CACHE_LINE(tpshm->tcb));

    tcb = tpshm->tcb;

    /*
     * Wait for master to boot ...
     */
    EMBXSHM_READS(&tcb->activeCPUs[tpshm->masterCPU].marker);
    while (!tcb->activeCPUs[tpshm->masterCPU].marker)
    {
        EMBX_OS_Delay (10);

        /* ensure any buffers are flushed before we re-examine the
         * shared data
         */
        tpshm->buffer_flush (tpshm, (void *) &(tcb->activeCPUs[tpshm->masterCPU].marker));

        EMBXSHM_READS(&tcb->activeCPUs[tpshm->masterCPU].marker);
    }
    
    /*
     * Check that we agree on the number of participants.
     */
    EMBXSHM_READS(&tcb->participants);
    if (memcmp (tcb->participants, tpshm->participants, sizeof (tpshm->participants)))
    {
        EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_genericSlaveInit = EMBX_INVALID_STATUS\n"));
        return EMBX_INVALID_STATUS;
    }

    /*
     * Initialise the shared heap
     */
    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_genericSlaveInit = EMBXSHM_memoryInit()\n"));
    return EMBXSHM_memoryInit(tpshm, tpshm->heap, 0);
}



EMBX_ERROR EMBXSHM_genericMasterInit (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_TCB_t *tcb;
    EMBX_ERROR err;
    int i;

    EMBX_Info(EMBX_INFO_INIT, (">>>EMBXSHM_genericMasterInit(0x%08x)\n", (unsigned) tpshm));

    EMBX_Assert (tpshm);
    EMBX_Assert (tpshm->tcb && EMBXSHM_ALIGNED_TO_CACHE_LINE(tpshm->tcb));

    tcb = tpshm->tcb;

    /*
     * Zero the transport control block
     */
    memset(tcb, 0, sizeof(EMBXSHM_TCB_t));

    /*
     * Initialise the shared heap
     */
    err = EMBXSHM_memoryInit(tpshm, tpshm->heap, tpshm->heapSize);
    if (EMBX_SUCCESS != err) 
    {
        EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_genericMasterInit = %d\n", err));
        return err;
    }

    /*
     * Initialise all the pipes for the participants
     */
    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        if (tpshm->participants[i])
        {
            EMBXSHM_Pipe_t *pipe = &tcb->pipes[i];
	    
	    EMBX_Assert(pipe && EMBXSHM_ALIGNED_TO_CACHE_LINE(pipe));

            if ((pipe->lock = EMBXSHM_createSpinlock (tpshm)) == 0)
            {
                goto error_exit;
            }
        }
    }

    /*
     * Create the spinlocks
     */
    if ((tcb->portTableSpinlock = EMBXSHM_createSpinlock (tpshm)) == 0)
        goto error_exit;


    if ((tcb->objectTableSpinlock = EMBXSHM_createSpinlock (tpshm)) == 0)
        goto error_exit;


    if ((tcb->portConnectSpinlock = EMBXSHM_createSpinlock (tpshm)) == 0)
        goto error_exit;


    /*
     * Set port table size to that specified, or a default
     */
    if (tpshm->transport.transportInfo.maxPorts > 0)
    {
        tcb->portTableSize = tpshm->transport.transportInfo.maxPorts;
    }
    else
    {
        tcb->portTableSize = EMBXSHM_DEFAULT_PORT_TABLE_SIZE;
    }

    /*
     * Allocate and zero the port and object tables
     */
    if ((tcb->portTable = (EMBXSHM_LocalPortShared_t**)
                          EMBXSHM_malloc (tpshm, tcb->portTableSize*sizeof (EMBXSHM_LocalPortShared_t *))) == 0)
    {
        goto error_exit;
    }
    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(tcb->portTable));

    memset (tcb->portTable, 0, sizeof (EMBXSHM_LocalPortShared_t *) * tcb->portTableSize);

    if ((tcb->objectTable = (EMBXSHM_Object_t*)
                            EMBXSHM_malloc (tpshm, tpshm->objectTableSize*sizeof (EMBXSHM_Object_t))) == 0)
    {
        goto error_exit;
    }
    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(tcb->objectTable));

    memset (tcb->objectTable, 0, sizeof (EMBXSHM_Object_t) * tpshm->objectTableSize);

    /*
     * Register our idea of what the transport looks like into the TCB
     */
    memcpy (tcb->participants, tpshm->participants, sizeof (tpshm->participants));

    EMBXSHM_WROTE_ARRAY(tcb->portTable, tcb->portTableSize);
    EMBXSHM_WROTE_ARRAY(tcb->objectTable, tpshm->objectTableSize);

    /*
     * Convert pointers in shared memory to bus addresses
     */
    tcb->portTable           = (EMBXSHM_LocalPortShared_t**)LOCAL_TO_BUS(tcb->portTable, tpshm->pointerWarp);
    tcb->objectTable         = (EMBXSHM_Object_t*)LOCAL_TO_BUS (tpshm->tcb->objectTable, tpshm->pointerWarp);
    tcb->portTableSpinlock   = EMBXSHM_serializeSpinlock (tpshm, tcb->portTableSpinlock);
    tcb->objectTableSpinlock = EMBXSHM_serializeSpinlock (tpshm, tcb->objectTableSpinlock);
    tcb->portConnectSpinlock = EMBXSHM_serializeSpinlock (tpshm, tcb->portConnectSpinlock);

    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        if (tpshm->participants[i])
        {
            EMBXSHM_Pipe_t *pipe = &tcb->pipes[i];
	    EMBX_Assert(pipe && EMBXSHM_ALIGNED_TO_CACHE_LINE(pipe));
	    pipe->lock = EMBXSHM_serializeSpinlock(tpshm, pipe->lock);
        }
    }

    EMBXSHM_WROTE(tcb);

    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_genericMasterInit = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;


error_exit:

    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        if (tpshm->participants[i] && tcb->pipes[i].lock)
            EMBXSHM_deleteSpinlock (tpshm, tcb->pipes[i].lock);

    }

    if (tcb->objectTableSpinlock)
        EMBXSHM_deleteSpinlock (tpshm, tcb->objectTableSpinlock);


    if (tcb->portTableSpinlock)
        EMBXSHM_deleteSpinlock (tpshm, tcb->portTableSpinlock);


    if (tcb->portConnectSpinlock)
        EMBXSHM_deleteSpinlock (tpshm, tcb->portConnectSpinlock);


    if (tcb->objectTable)
        EMBXSHM_free (tpshm, tcb->objectTable);


    if (tcb->portTable)
        EMBXSHM_free (tpshm, tcb->portTable);


    /*
     * Zero the tcb for safety
     */
    memset(tcb, 0, sizeof(EMBXSHM_TCB_t));
    EMBXSHM_WROTE(tcb);

    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_genericMasterInit = EMBX_NOMEM\n"));
    return EMBX_NOMEM;
}



/*
 * Transport initialization
 */
static EMBX_ERROR doInitialize (EMBXSHM_Transport_t *tpshm)
{
    EMBX_ERROR     res;
    EMBXSHM_TCB_t *tcb;
    EMBX_UINT      i;

    EMBX_Info(EMBX_INFO_INIT, (">>>doInitialize(0x%08x)\n", (unsigned) tpshm));

    EMBX_Assert (tpshm);

    /*
     * Check that those structures held in shared memory are aligned to a cache line
     * boundary.
     *
     * Each of the following asserts checks a constant expression. Many compilers
     * warn about conditional statements (such as assert) that evaluate a constant
     * expression. For this reason we fold in tpshm above to confuse the warning
     * analyizers.
     */
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_BufferList_t)));
    /*EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_Spinlock_t)));*/
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_HeapControlBlockShared_t)));
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_PipeElement_t)));
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_Pipe_t)));
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_LocalPortShared_t)));
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_RemotePortLink_t)));
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_Object_t)));
    EMBX_Assert (tpshm && EMBXSHM_ALIGNED_TO_CACHE_LINE(sizeof(EMBXSHM_TCB_t)));

    EMBX_Info(EMBX_INFO_INIT, ("   Transport Control Block = 0x%08x\n", (unsigned) tpshm->tcb));

    /*
     * Create the new port notification thread
     */
    tpshm->closedPortThread = EMBX_OS_ThreadCreate(
            (void (*)(void*)) EMBXSHM_portClosedNotification, (void *)tpshm,
            EMBX_DEFAULT_THREAD_PRIORITY, "EMBXSHM-PortClosed");

    if (tpshm->closedPortThread == EMBX_INVALID_THREAD)
    {
        EMBX_Info(EMBX_INFO_SHM, ("<<<doInitialize = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }

    /*
     * This thread goes on to become the new port notification thread
     *
     * Call the function setup by the transport factory
     * to do the main part of the shared memory transport
     * initialization, including any platform specific setup.
     */
    res = tpshm->setup_shm_environment (tpshm);
    if (res != EMBX_SUCCESS)
    {
        EMBXSHM_killThread (tpshm, EMBXSHM_CLOSEPORT_WORKER);

        EMBX_Info(EMBX_INFO_SHM, ("<<<doInitialize = %d\n", res));
        return res;
    }

    /*
     * We cannot guarantee the the tcb pointer has been initialized until after we have
     * called setup_shm_environment()
     */
    EMBX_Assert (tpshm->tcb && EMBXSHM_ALIGNED_TO_CACHE_LINE(tpshm->tcb));
    tcb = tpshm->tcb;
    EMBXSHM_READS(tcb);

    /*
     * fill in the outstanding parts of the transport info
     */
    tpshm->transport.transportInfo.memStart = tpshm->warpRangeAddr;
    tpshm->transport.transportInfo.memEnd   = (void *) ((uintptr_t) tpshm->warpRangeAddr + tpshm->warpRangeSize-1);
    
    /*
     * Take a local cached copy of the spinlock addresses stored in the
     * TCB, to make lock taking a _little_ bit faster.
     */
    tpshm->portTableSpinlock   = EMBXSHM_deserializeSpinlock(tpshm, tcb->portTableSpinlock);
    tpshm->objectTableSpinlock = EMBXSHM_deserializeSpinlock(tpshm, tcb->objectTableSpinlock);
    tpshm->portConnectSpinlock = EMBXSHM_deserializeSpinlock(tpshm, tcb->portConnectSpinlock);

    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        if (tpshm->participants[i])
        {
	    tpshm->pipeLocks[i] = EMBXSHM_deserializeSpinlock(tpshm, tcb->pipes[i].lock);
        }
        else
        {
            tpshm->pipeLocks[i] = 0;
        }
    }

    /*
     * Install interrupt handler, must be done before we declare
     * ourselves active to the rest of the transport.
     */
    tpshm->install_isr (tpshm, (void (*)(void*))EMBXSHM_interruptHandler);

    /*
     * Mark this CPU as active.
     */
    tcb->activeCPUs[tpshm->cpuID].marker = 1;
    EMBXSHM_WROTE(&tcb->activeCPUs[tpshm->cpuID]);

    EMBX_Info(EMBX_INFO_SHM, ("<<<doInitialize = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}



/*
 * Worker thread used to implement async initialization
 */
static void initThread (void *param)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)param;
    EMBX_ERROR res = EMBX_SUCCESS;

    EMBX_Info(EMBX_INFO_SHM, (">>>initThread\n"));

    EMBX_Assert (tpshm);

    EMBX_OS_MUTEX_TAKE (&tpshm->tpLock);

    if (tpshm->initEvent)
    {
        /*
         * A task is still waiting for the initialize
         * so signal it once the initialization has completed.
         *
         * TODO: allow the initialzation to be really asychronous
         *       and interruptable during its busy waiting.
         */
        res = doInitialize(tpshm);

        tpshm->initEvent->result = res;

        EMBX_Info(EMBX_INFO_SHM, ("   waking calling thread\n"));
        EMBX_OS_EVENT_POST (&tpshm->initEvent->event);

        tpshm->initEvent = 0;
    }

    EMBX_OS_MUTEX_RELEASE (&tpshm->tpLock);

    if (res != EMBX_SUCCESS)
        return;
    
    /*
     * Now become the thread that manages the second half interrupt processing.
     */
    EMBXSHM_secondHalfHandler (tpshm);

    /*
     * Check to see if we have been remotely invalidated. If so
     * we are responsible for tidying up.
     *
     * DEADLOCK fix: We must not do this if we are already in the CLOSEDOWN sequence
     */
    if (tpshm->transport.state != EMBX_TP_IN_CLOSEDOWN && tpshm->remoteInvalidateFlag)
    {
        _EMBX_DriverMutexTake();
        tpshm->transport.methods->invalidate(&(tpshm->transport));
        _EMBX_DriverMutexRelease();
    }

    EMBX_Info(EMBX_INFO_SHM, ("<<<initThread\n"));
}



/*
 * Transport method to initialise
 * We _only_ support asynchronous initialisation
 */
EMBX_ERROR EMBXSHM_initialize (EMBX_Transport_t  *tp, EMBX_EventState_t *ev)
{
    EMBX_ERROR res;
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;

    EMBX_Info(EMBX_INFO_INIT, (">>>EMBXSHM_initialize\n"));

    EMBX_Assert (tpshm);
    EMBX_Assert (ev);

    /*
     * Create the sync objects required by the transport
     */
    res = createTransportSyncObjects (tpshm);
    if (res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_INFO_SHM, ("<<<EMBX_initialize = %d\n", res));
        return res;
    }

    /*
     * Save
     */
    tpshm->initEvent = ev;
    
    /*
     * Create the init thread. This completes initialisation
     * and then goes on to become the new port notification
     * processing thread.
     */
    tpshm->secondHalfThread = EMBX_OS_ThreadCreate (initThread,
                                                 (void*)tpshm,
                                                 EMBX_DEFAULT_THREAD_PRIORITY,
                                                 "EMBXSHM-NewPort");

    if (tpshm->secondHalfThread == EMBX_INVALID_THREAD)
    {
        EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_initialize = EMBX_SYSTEM_ERROR\n"));

        tpshm->initEvent = 0;
        EMBXSHM_destroyTransportSyncObjects (tpshm);

        return EMBX_SYSTEM_ERROR;
    }

    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_initialize = EMBX_BLOCK\n"));
    return EMBX_BLOCK;
}



/*
 * Transport method to rip down an async. init that may on going
 */
EMBX_VOID EMBXSHM_interruptInit (EMBX_Transport_t *tp)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;

    EMBX_Info(EMBX_INFO_INIT, (">>>EMBXSHM_initerruptInit\n"));

    EMBX_Assert (tpshm);

    EMBX_OS_MUTEX_TAKE(&tpshm->tpLock);

    /* We cannot actually interrupt the busy waiting at the moment,
     * so just forget about the event so we don't signal it (it is
     * about to disappear from the callers stack frame).
     */
    tpshm->initEvent = 0;

    EMBX_OS_MUTEX_RELEASE(&tpshm->tpLock);

    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_initerruptInit\n"));
}


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
