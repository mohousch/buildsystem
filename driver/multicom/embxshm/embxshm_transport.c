/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_transport.c                                  */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport                            */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"


/*
 * Routine to see if any participants in the transport
 * are left.
 */
static EMBX_BOOL activeParticipants (EMBXSHM_Transport_t *tpshm, EMBXSHM_TCB_t *tcb)
{
    EMBX_UINT i;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>activeParticipants()\n"));

    EMBX_Assert(tpshm);
    EMBX_Assert(tcb && EMBXSHM_ALIGNED_TO_CACHE_LINE(tcb));

    EMBXSHM_READS(&tcb->activeCPUs);

    /* this function is called in a tight busy wait loop
     * so we must ensure any buffers are cleared
     */
    tpshm->buffer_flush(tpshm, (void *) &(tcb->activeCPUs[0].marker));

    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        if (tcb->activeCPUs[i].marker)
        {
            EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<activeParticipants = EMBX_TRUE\n"));
            return EMBX_TRUE;
        }
    }

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<activeParticipants = EMBX_FALSE\n"));
    return EMBX_FALSE;
}



/*
 * Helper function to kill off any worker threads started
 * by the transport. This is used in closedown and to tidy
 * up initialization if it fails.
 */
EMBX_ERROR EMBXSHM_killThread (EMBXSHM_Transport_t *tpshm,
                               EMBX_UINT            threadName)
{
    EMBX_ERROR res = EMBX_SUCCESS;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>EMBXSHM_killThread()\n"));

    switch (threadName)
    {
        case EMBXSHM_NEWPORT_WORKER:
        {
            if(tpshm->locksValid && tpshm->secondHalfDieFlag == 0)
            {
		tpshm->secondHalfDieFlag = 1;

                EMBX_OS_EVENT_POST (&tpshm->secondHalfEvent);
            }

	    if (tpshm->secondHalfThread != EMBX_INVALID_THREAD)
	    {
		res = EMBX_OS_ThreadDelete(tpshm->secondHalfThread);
		if (res == EMBX_SUCCESS)
		{
		    tpshm->secondHalfThread = EMBX_INVALID_THREAD;
		}
	    }

            tpshm->secondHalfDieFlag = 0;
            break;
        } /* EMBXSHM_NEWPORT_WORKER */
        case EMBXSHM_CLOSEPORT_WORKER:
        {
            if(tpshm->locksValid && tpshm->closedPortDieFlag == 0)
            {
		tpshm->closedPortDieFlag = 1;

                EMBX_OS_EVENT_POST (&tpshm->closedPortEvent);
            }

 	    if (tpshm->closedPortThread != EMBX_INVALID_THREAD)
	    {
		res = EMBX_OS_ThreadDelete(tpshm->closedPortThread);
		if (res == EMBX_SUCCESS)
		{
		    tpshm->closedPortThread = EMBX_INVALID_THREAD;
		}
	    }

            tpshm->closedPortDieFlag = 0;
            break;
        } /* EMBXSHM_CLOSEPORT_WORKER */
        default:
        {
            EMBX_Assert(0);
            res = EMBX_INVALID_ARGUMENT;
        } /* default */
    }

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<EMBXSHM_killThread\n"));

    return res;
}



/*
 * Transport method to perform closedown of a transport
 */
static EMBX_ERROR closedown (EMBX_Transport_t *tp, EMBX_EventState_t *ev)
{
    int i;
    EMBX_ERROR res;
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>closedown()\n"));
    EMBX_Assert (tpshm);


    if (tpshm->tcb)
    {
        EMBXSHM_TCB_t *tcb = tpshm->tcb;

	EMBX_Assert(tcb && EMBXSHM_ALIGNED_TO_CACHE_LINE(tcb));

        /*
         * We have to wait for everyone else in the transport to die off,
         * before completing closedown. We leave the interrupt handler
         * in place, so it can service any port close notifications.
         */
        tcb->activeCPUs[tpshm->cpuID].marker = 0;
	EMBXSHM_WROTE(&tcb->activeCPUs[tpshm->cpuID].marker);

        while (activeParticipants (tpshm, tcb))
        {
            EMBX_OS_Delay (50);
        }
    }

    /*
     * It's now safe to kill off our worker threads, these can fail
     * due to taking an OS signal while waiting for the thread to
     * signal termination.
     */
    res = EMBXSHM_killThread(tpshm, EMBXSHM_CLOSEPORT_WORKER);
    if (res != EMBX_SUCCESS)
        return res;


    res = EMBXSHM_killThread(tpshm, EMBXSHM_NEWPORT_WORKER);
    if (res != EMBX_SUCCESS)
        return res;
    

    /*
     * If we're the master, we need to rip down all the shared memory
     * stuff. As we are also going to destroy the shared heap manager there
     * isn't any point in actually releasing the spin locks and shared
     * tables. So just zero the whole tcb structure for safety.
     */
    if (tpshm->tcb && tpshm->cpuID == tpshm->masterCPU)
    {
        memset(tpshm->tcb, 0, sizeof(EMBXSHM_TCB_t));
    }
    EMBXSHM_WROTE(tpshm->tcb);

    /*
     * Forget about the cached spinlock pointers
     */
    tpshm->portTableSpinlock   = 0;
    tpshm->portConnectSpinlock = 0;
    tpshm->objectTableSpinlock = 0;

    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        tpshm->pipeLocks[i] = 0;
    }


    /*
     * Closedown the heap manager
     */
    EMBXSHM_memoryDeinit (tpshm);

    /*
     * Remove our interrupt handler
     */
    tpshm->remove_isr ((void *)tpshm);

    /*
     * Destroy sundry mutexes & events
     */
    EMBXSHM_destroyTransportSyncObjects(tpshm);

    /*
     * Callback into the factory code to free any allocated memory
     */
    if (tpshm->cleanup_shm_environment)
    {
        tpshm->cleanup_shm_environment(tpshm);
    }

    /*
     * Do not free the tpshm structure, this is done at the framework level
     */
    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<closedown = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}



/*
 * Transport method called when a deferred closedown is interrupted.
 */
static EMBX_VOID closeInterrupt (EMBX_Transport_t *tp)
{
    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>closeInterrupt()\n"));
    EMBX_Assert (tp);

    /* As this transport doesn't return EMBX_BLOCK from closedown
     * this has nothing to do.
     */

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<closeInterrupt()\n"));
}



/* 
 * Routine to invalidate all the local ports on a transport,
 * and this transport itself.
 */
static EMBX_VOID doInvalidateHandle (EMBXSHM_TransportHandle_t *th)
{
    EMBX_LocalPortList_t *phl;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>doInvalidateHandle()\n"));
    EMBX_Assert (th);

    /*
     * In this transport we can simply call the invalidate
     * method on each local port. This will implicitly invalidate
     * any remote ports, which must be connected to a local port.
     * It will also wake up any tasks waiting to receive data. Transports
     * involving multiple CPUs may need to do more work to invalidate
     * remote ports connected to a different CPU. 
     */
    phl = th->localPorts;

    for (phl = th->localPorts; phl; phl = phl->next)
    {
        /*
         * Making this call assumes that invalidate_port does not
         * require any transport resources that we already have
         * locked. In this transport that is the case.
         */
        phl->port->methods->invalidate_port (phl->port);
    }

    th->state = EMBX_HANDLE_INVALID;

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<doInvalidateHandle\n"));
}



/*
 * Transport method to invalidate a transport
 */
static EMBX_VOID invalidate (EMBX_Transport_t *tp)
{
    EMBXSHM_Transport_t        *tpshm = (EMBXSHM_Transport_t *)tp;
    EMBX_TransportHandleList_t *thandles = tpshm->transport.transportHandles;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>invalidate()\n"));
    EMBX_Assert (tpshm);

    /*
     * Tell the other CPUs we are about to invalidate the transport.
     */
    if (!tpshm->remoteInvalidateFlag)
    {
        EMBXSHM_PipeElement_t element;
        int i;

        element.control = EMBXSHM_PIPE_CTRL_TRANSPORT_INVALIDATE;

        for (i=0; i<EMBXSHM_MAX_CPUS; i++)
        {
            if (i != tpshm->cpuID && tpshm->participants[i])
            {
                EMBX_ERROR err;

		/*
		 * We are already part way through the operation so we'll
		 * keep trying to repost until we manage it.
		 */
                while (EMBX_SUCCESS != (err = EMBXSHM_enqueuePipeElement (tpshm, i, &element))) 
		{
			EMBX_Assert (EMBX_NOMEM == err);
			EMBX_OS_Delay(10);
		}
            }
        }
    }

    thandles = tpshm->transport.transportHandles;

    /*
     * First of all wake up any pending connection requests
     */
    EMBX_OS_MUTEX_TAKE (&tpshm->connectionListMutex);

    while (tpshm->connectionRequests) {
	EMBX_EventState_t *es = EMBX_EventListFront(&tpshm->connectionRequests);

	EMBX_OS_MemFree(es->data); /* match with CBS IN DATA */

	/* 
	 * When we post this event we have effectively freed es, it must not be used again
	 */
	EMBX_OS_EVENT_POST (&es->event);
    }

    EMBX_OS_MUTEX_RELEASE (&tpshm->connectionListMutex);

    /*
     * Now go and deal with still open transport handles
     */
    while (thandles)
    {
        EMBXSHM_TransportHandle_t *th = (EMBXSHM_TransportHandle_t *)thandles->transportHandle;

        doInvalidateHandle (th);

        thandles = thandles->next;
    }

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<invalidate\n"));
}



/*
 * Transport method to open a handle
 */
static EMBX_ERROR openHandle (EMBX_Transport_t        *tp,
                              EMBX_TransportHandle_t **tph)
{
    EMBXSHM_TransportHandle_t *handle;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>openHandle()\n"));
    EMBX_Assert (tp);
    EMBX_Assert (tph);

    /*
     * Allocate memory for handle
     */
    handle = (EMBXSHM_TransportHandle_t *)EMBX_OS_MemAlloc (sizeof (EMBXSHM_TransportHandle_t));
    if (handle != NULL)
    {
        /*
         * Copy default structure contents.
         */
        memcpy (handle, &EMBXSHM_transportHandleTemplate, sizeof (EMBXSHM_TransportHandle_t));
        *tph = (EMBX_TransportHandle_t *)handle;

        EMBX_OS_MODULE_REF();

        EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<openHandle = EMBX_SUCCESS\n"));
        return EMBX_SUCCESS;
    }
    else
    {
        EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<openHandle = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }
}



/*
 * Transport method to close an open handle
 */
static EMBX_ERROR closeHandle (EMBX_Transport_t       *tp,
                               EMBX_TransportHandle_t *tph)
{
    EMBXSHM_Transport_t        *tpshm = (EMBXSHM_Transport_t *)tp;
    EMBX_EventState_t          *es, *next;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>closeHandle()\n"));
    EMBX_Assert (tpshm);
    EMBX_Assert (tph);

    EMBX_OS_MUTEX_TAKE (&tpshm->connectionListMutex);

    /*
     * Unblock processes waiting for a connection through the closing handle
     */
    for (es = tpshm->connectionRequests; es; es = next)
    {
	EMBXSHM_ConnectBlockState_t *cbs = es->data; /* match with CBS IN DATA */

        next = es->next;

        if (cbs->requestingHandle == tph)
        {
	    EMBX_EventListRemove(&tpshm->connectionRequests, es);
	    EMBX_OS_MemFree (cbs);

            es->result = EMBX_TRANSPORT_CLOSED;
            EMBX_OS_EVENT_POST (&es->event);
        }
    }

    EMBX_OS_MUTEX_RELEASE (&tpshm->connectionListMutex);

    /*
     * Make key handle structures invalid to help catch use after close
     */
    tph->state     = EMBX_HANDLE_CLOSED;
    tph->transport = 0;

    EMBX_OS_MemFree (tph);

    EMBX_OS_MODULE_UNREF();

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<closeHandle = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}



/*
 * Transport method to register an object.
 */
static EMBX_ERROR registerObject (EMBX_Transport_t *tp,
                                  EMBX_VOID        *object,
                                  EMBX_UINT         size,
                                  EMBX_HANDLE      *userHandle)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;
    EMBX_ERROR           res;
    EMBXSHM_Object_t    *obj;
    EMBX_UINT            index, found;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>registerObject(0x%08x, 0x%08x, %d, 0x%08x)\n",
                                     (unsigned) tp, (unsigned) object, size, (unsigned) userHandle));
    EMBX_Assert (tpshm);
    EMBX_Assert (object);
    EMBX_Assert (userHandle);

    /*
     * Lock the table
     */
    EMBXSHM_takeSpinlock (tpshm, &tpshm->objectTableMutex, tpshm->objectTableSpinlock);

    EMBXSHM_READS(&tpshm->tcb->objectTable);

    /*
     * Find a spare slot for this object. We resume searching from the last place this
     * processor found a spare slot. This dramatically improves general case performance
     * especially if there are no long lived objects (long lived objects will cause occasional
     * 'lumps' in the runtime of this function).
     */
    obj = (EMBXSHM_Object_t*)BUS_TO_LOCAL (tpshm->tcb->objectTable, tpshm->pointerWarp);
    EMBX_Assert (obj && EMBXSHM_ALIGNED_TO_CACHE_LINE(obj));

    EMBXSHM_READS_ARRAY(obj, tpshm->objectTableSize);

    found = 0;
    for (index=tpshm->objectTableIndex; index<tpshm->objectTableSize; index++)
    {
	if (obj[index].valid == 0)
	{
	    obj[index].valid = 1;
	    EMBXSHM_WROTE(&(obj[index].valid));	 
	    found = 1;
	    break;
	}
    }
 
    if (!found)
    {
	for (index=0; index<tpshm->objectTableIndex; index++)
	{
	    if (obj[index].valid == 0)
	    {					 
		obj[index].valid = 1;
		EMBXSHM_WROTE(&(obj[index].valid));	 
		found = 1;
		break;
	    }
	}
    }
    if (found)
    {
	tpshm->objectTableIndex = index + 1;
    }

    /*
     * We can unlock the table now
     */
    EMBXSHM_releaseSpinlock (tpshm, &tpshm->objectTableMutex, tpshm->objectTableSpinlock);

    /*
     * If we found room, register it
     */
    if (found)
    {
	EMBX_UINT paddr = 0;

	/* MULTICOM_32BIT_SUPPORT: The Warp range is now a physical address range 
	 *
	 * Converts the supplied vaddr to a physical one iff it lies within the warp ranges or SHM heap
	 *   
	 *   res indicates the cache mode (e.g. EMBX_SUCCESS == coherent)
	 */
#ifdef __TDT__
/* stm24 prob with virt_to_phys makro */
	res = tp->methods->virt_to_phys_alt(tp, object, &paddr);
#else
	res = tp->methods->virt_to_phys(tp, object, &paddr);
#endif

        /*
         * If it resides entirely in shared memory, no need to mirror it
         */
        if (res == EMBX_SUCCESS) /* XXXX addy 16.11.07 Can only support Uncached Zero copy objects currently */
        {
            obj[index].owningCPU  = tpshm->cpuID;
            obj[index].size       = size;
            obj[index].sharedData = (EMBX_VOID *)paddr;
            obj[index].localData  = object;
            obj[index].shadow     = 0;
	    obj[index].mode       = res;

            *userHandle = (EMBX_HANDLE)(index + EMBXSHM_ZEROCOPY_OBJECT_MAGIC);

	    EMBX_Info (EMBX_INFO_TRANSPORT, ("    registered zero copy object %p index %d paddr 0x%08x handle 0x%08x\n",
					     object, index, paddr, *userHandle));

            res         = EMBX_SUCCESS;
        }
        else
        {
            EMBX_VOID *shadow;

            if ((shadow = EMBXSHM_malloc (tpshm, size)) != 0)
            {
                obj[index].owningCPU  = tpshm->cpuID;
                obj[index].size       = size;
                obj[index].sharedData = (EMBX_VOID *)LOCAL_TO_BUS (shadow, tpshm->pointerWarp);
                obj[index].localData  = object;
                obj[index].shadow     = 1;
#ifdef EMBXSHM_CACHED_HEAP
		obj[index].mode       = EMBX_INCOHERENT_MEMORY;
#else
		obj[index].mode       = EMBX_SUCCESS;
#endif

                *userHandle = (EMBX_HANDLE)(index + EMBXSHM_SHADOW_OBJECT_MAGIC);

		EMBX_Info (EMBX_INFO_TRANSPORT, ("    registered shadow object %p index %d shadow %p paddr 0x%08x handle 0x%08x\n",
						 object, index, shadow, paddr, *userHandle));

                res         = EMBX_SUCCESS;
            }
            else
            {
                obj[index].valid      = 0;
                res = EMBX_NOMEM;
            }
        }
    }
    else
    {
        res = EMBX_NOMEM;
    }

    EMBXSHM_WROTE(obj+index);

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<registerObject(..., *userHandle = 0x%08x) = %d\n",
                                     (unsigned) *userHandle, res));
    return res;
}



/*
 * Transport method to deregister an object. 
 */
static EMBX_ERROR deregisterObject (EMBX_Transport_t *tp,
                                    EMBX_HANDLE       handle)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;
    EMBX_ERROR           res   = EMBX_INVALID_ARGUMENT;
    EMBXSHM_Object_t    *obj;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>deregisterObject(0x%08x, 0x%08x)\n", 
                                     (unsigned) tp, (unsigned) handle));
    EMBX_Assert (tpshm);

    /*
     * Lock the table
     */
    EMBXSHM_takeSpinlock (tpshm, &tpshm->objectTableMutex, tpshm->objectTableSpinlock);

    if ((obj = EMBXSHM_findObject (tpshm, handle)) != 0)
    {
	EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(obj));

	EMBXSHM_READS(obj);

        if (obj->owningCPU == tpshm->cpuID)
        {
            if (obj->shadow) 
            {
                EMBXSHM_free(tpshm, (EMBX_VOID *) BUS_TO_LOCAL(obj->sharedData, tpshm->pointerWarp));
                obj->shadow = 0;
            }

            obj->valid      = 0;
            obj->owningCPU  = -1;
            obj->size       = 0;
            obj->sharedData = 0;
            obj->localData  = 0;
	    obj->mode       = 0;

	    EMBXSHM_WROTE(obj);

            res = EMBX_SUCCESS;
        }
    }

    /*
     * We can unlock the table now
     */
    EMBXSHM_releaseSpinlock (tpshm, &tpshm->objectTableMutex, tpshm->objectTableSpinlock);

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<deregisterObject = %d\n", res));
    return res;
}



/*
 * Helper routine to locate the object referred to by a handle.
 */
EMBXSHM_Object_t *EMBXSHM_findObject (EMBXSHM_Transport_t *tpshm,
                                      EMBX_HANDLE          handle)
{
    EMBXSHM_Object_t *obj = 0;
    EMBX_UINT         index;

    EMBX_Info (EMBX_INFO_INTERRUPT, (">>>EMBXSHM_findObject(0x%08x, 0x%08x)\n", 
                                     (unsigned) tpshm, (unsigned) handle));
    EMBX_Assert (tpshm);
    EMBX_Assert (tpshm->tcb);
    EMBX_Assert (tpshm->tcb->objectTable);

    if ((index = (handle & EMBXSHM_OBJECT_INDEX_MASK)) < tpshm->objectTableSize)
    {
        obj = (EMBXSHM_Object_t *)BUS_TO_LOCAL (tpshm->tcb->objectTable + index, tpshm->pointerWarp);

        EMBX_Assert (obj && EMBXSHM_ALIGNED_TO_CACHE_LINE(obj));
	EMBXSHM_READS(obj);

        if (!obj->valid)
        {
            obj = 0;
        }
    }

    EMBX_Info (EMBX_INFO_INTERRUPT, ("<<<EMBXSHM_findObject = 0x%08x\n", (unsigned)obj));
    return obj;
}



/*
 * Transport method to return the data buffer and size of an object
 */
static EMBX_ERROR getObject (EMBX_Transport_t *tp,
                             EMBX_HANDLE       handle,
                             EMBX_VOID       **object,
                             EMBX_UINT        *size)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;
    EMBX_ERROR           res   = EMBX_SUCCESS;
    EMBXSHM_Object_t    *obj;

    EMBX_Info (EMBX_INFO_TRANSPORT, (">>>getObject(0x%08x, 0x%08x, ...)\n", 
                                     (unsigned) tp, (unsigned) handle));
    EMBX_Assert (tpshm);
    EMBX_Assert (object);
    EMBX_Assert (size);

    if ((obj = EMBXSHM_findObject (tpshm, handle)) != 0)
    {
	/* EMBXSHM_READS(obj); */ /* already performed by EMBXSHM_findObject */
        if (obj->owningCPU == tpshm->cpuID)
        {
            *object = obj->localData;
        }
        else
        {
#if 0
            *object = BUS_TO_LOCAL (obj->sharedData, tpshm->pointerWarp);
#else
#ifdef __TDT__
/* stm24 prob */
            res = tp->methods->phys_to_virt_alt (tp, (EMBX_UINT)obj->sharedData, object);
#else
            res = tp->methods->phys_to_virt (tp, (EMBX_UINT)obj->sharedData, object);
#endif
#endif
        }
        *size = obj->size;
    }
    else
    {
        res = EMBX_INVALID_ARGUMENT;
    }

    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<getObject(*object = 0x%08x, *size = %d) = %d\n", 
                                     (unsigned) *object, *size, res));
    return res;
}

/* Optional transport method to return the state of a memory address (is it or it it not
 * cache coherent
 */
static EMBX_ERROR testState (EMBX_Transport_t *tp, EMBX_VOID *addr)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;

    int res;
    
    EMBX_Assert(tp);
    EMBX_Assert(addr);

    /* First check to see if this is an EMBXSHM heap address */
    if (addr >= tpshm->heap && addr < (void *)((unsigned) tpshm->heap + tpshm->heapSize))
    {
#ifdef EMBXSHM_CACHED_HEAP
	res = EMBX_INCOHERENT_MEMORY;
#else
	res = EMBX_SUCCESS;
#endif
    }
    else
    {
	EMBX_UINT paddr;

	/* Do an OS specific translation of the virtual address,
	 * return code gives the cache mode, i.e. EMBX_SUCCESS or EMBX_INCOHERENT_MEMORY
	 */
	res = EMBX_OS_VirtToPhys(addr, &paddr);
    }
	
    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<testState(0x%08x, 0x%08x) = %s\n", 
                                     (unsigned) tp, (unsigned) addr, 
				     res == EMBX_SUCCESS ? "COHERENT" : "INCOHERENT"));
    
    return res;
}

static EMBX_ERROR virtToPhys (EMBX_Transport_t *tp, EMBX_VOID *addr, EMBX_UINT *paddrp)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;

    int res;
    
    EMBX_Assert(tp);
    EMBX_Assert(addr);

    /* First check to see if this is an EMBXSHM heap address */
    if (addr >= tpshm->heap && addr < (void *)((unsigned) tpshm->heap + tpshm->heapSize))
    {
	*paddrp = (EMBX_UINT) LOCAL_TO_BUS(addr, tpshm->pointerWarp);
	
#ifdef EMBXSHM_CACHED_HEAP
	res = EMBX_INCOHERENT_MEMORY;
#else
	res = EMBX_SUCCESS;
#endif
    }
    else
    {
	EMBX_UINT paddr;
	
	/* Do an OS specific lookup of the virtual address */
	res = EMBX_OS_VirtToPhys(addr, &paddr);

	EMBX_Info(EMBX_INFO_TRANSPORT, ("    virtToPhys: addr %p -> 0x%08x : %d\n", addr, paddr, res));
	
	if (res != EMBX_INVALID_ARGUMENT)
	{
	    /* MULTICOM_32BIT_SUPPORT: Support two disjoint Warp ranges */
	    if (((EMBX_UINT)tpshm->warpRangeAddr <= paddr && ((EMBX_UINT)tpshm->warpRangeAddr + tpshm->warpRangeSize) >= paddr) ||
		((EMBX_UINT)tpshm->warpRangeAddr2 <= paddr && ((EMBX_UINT)tpshm->warpRangeAddr2 + tpshm->warpRangeSize2) >= paddr))
	    {
		*paddrp = paddr;

		EMBX_Info(EMBX_INFO_TRANSPORT, ("    Zero Copy: addr %p paddr 0x%08x mode %d\n", addr, paddr, res));
	    }
	    else
	    {
		EMBX_Info(EMBX_INFO_TRANSPORT, ("    paddr 0x%08x missed warp 0x%08x-0x%08x warp2 0x%08x-0x%08x\n",
						paddr,
						(EMBX_UINT)tpshm->warpRangeAddr, (EMBX_UINT)tpshm->warpRangeAddr + tpshm->warpRangeSize,
						(EMBX_UINT)tpshm->warpRangeAddr2, (EMBX_UINT)tpshm->warpRangeAddr2 + tpshm->warpRangeSize2));

		res = EMBX_INVALID_ARGUMENT;
	    }
	}
    }
	
    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<virtToPhys(0x%08x, 0x%08x) -> 0x%08x = %d\n", 
                                     (unsigned) tp, (unsigned) addr, *paddrp, res));
    
    return res;
}


static EMBX_ERROR physToVirt (EMBX_Transport_t *tp, EMBX_UINT paddr, EMBX_VOID **vaddrp)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;

    EMBX_VOID *vaddr;

    int res;
    
    EMBX_Assert(tp);
    EMBX_Assert(paddr);

    /* MULTICOM_32BIT_SUPPORT: Support two disjoint Warp ranges */
    if (((EMBX_UINT)tpshm->warpRangeAddr <= paddr && ((EMBX_UINT)tpshm->warpRangeAddr + tpshm->warpRangeSize) > paddr))
    {
	vaddr = BUS_TO_LOCAL(paddr, tpshm->pointerWarp);
    }
    else if (((EMBX_UINT)tpshm->warpRangeAddr2 <= paddr && ((EMBX_UINT)tpshm->warpRangeAddr2 + tpshm->warpRangeSize2) > paddr))
    {
	vaddr = BUS_TO_LOCAL(paddr, tpshm->pointerWarp2);
    }
    else
	return EMBX_INVALID_ARGUMENT;
    
    /*
     * Found a vaddr, now work out its cache mode 
     */
    *vaddrp = vaddr;
    
    /* First check to see if this is an EMBXSHM heap address */
    if (vaddr >= tpshm->heap && vaddr < (void *)((unsigned) tpshm->heap + tpshm->heapSize))
    {
#ifdef EMBXSHM_CACHED_HEAP
	res = EMBX_INCOHERENT_MEMORY;
#else
	res = EMBX_SUCCESS;
#endif
    }
    else
    {
	EMBX_UINT paddr;

	/* Translate virtual back to physical to get the cache mode */
	res = EMBX_OS_VirtToPhys(vaddr, &paddr);
    }
    
    EMBX_Info (EMBX_INFO_TRANSPORT, ("<<<physToVirt(0x%08x, 0x%08x -> %p) = %d\n", 
                                     (unsigned) tp, (unsigned) paddr, *vaddrp, 
				     res));
    
    return res;
}

static EMBX_ERROR removeProcessor (EMBX_Transport_t *tp, EMBX_UINT cpuID)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *)tp;
    EMBXSHM_TCB_t       *tcb;

    if (0 == cpuID) {
	EMBX_DebugMessage(("Failed 'cannot remove host processor'\n"));
	return EMBX_INVALID_ARGUMENT;
	
    }
    
    if (cpuID >= EMBXSHM_MAX_CPUS) {
	EMBX_DebugMessage(("Failed 'CPU ID is too large'\n"));
	return EMBX_INVALID_ARGUMENT;
    }

    tcb = tpshm->tcb;
    EMBXSHM_READS(tcb);

    /* mark the processor as inactive */
    tcb->activeCPUs[cpuID].marker = 0;
    EMBXSHM_WROTE(&tcb->activeCPUs[cpuID]);	
    
    /* update the local participants map used to determine when to send port
     * open/close notifications. for remote processors this will be performed
     * automatically [see update updateLocalParticipantsMap()] but we don't
     * want to needlessly deadlock if the dead processors comms. pipe
     * overflows.
     */
    tpshm->participants[cpuID] = 0;
    
    return EMBX_SUCCESS;
}

/*
 * Transport structures
 */
static EMBX_TransportMethods_t transportMethods =
{
    EMBXSHM_initialize,
    EMBXSHM_interruptInit,
    closedown,
    closeInterrupt,
    invalidate,
    openHandle,
    closeHandle,
    EMBXSHM_allocateMemory,
    EMBXSHM_freeMemory,
    registerObject,
    deregisterObject,
    getObject,
    testState,
    virtToPhys,
    physToVirt,
    removeProcessor
};

EMBXSHM_Transport_t EMBXSHM_transportTemplate =
{
    {
        &transportMethods,     /* methods */
        {                      /* transportInfo */
            "" ,               /* name */
            EMBX_FALSE,        /* isInitialized */
            EMBX_TRUE,         /* usesZeroCopy */
            EMBX_TRUE,         /* allowsPointerTranslation */
            EMBX_TRUE          /* allowsMultipleConnections */
        },
        EMBX_TP_UNINITIALIZED  /* state */
    } /* EMBX_Transport_t */
};

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
