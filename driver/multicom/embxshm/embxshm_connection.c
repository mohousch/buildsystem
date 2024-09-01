/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_connection.c                                 */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport                            */
/*    transport handle port creation and connection methods   */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"

/*
 * Notify all other participants in the transport that
 * a new port has been created.
 */
static void notifyPeersOfNewPort (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_PipeElement_t element;
    EMBX_UINT             i;

    EMBX_Info(EMBX_INFO_SHM, (">>>notifyPeersOfNewPort\n"));

    EMBX_Assert (tpshm);

    element.control = EMBXSHM_PIPE_CTRL_PORT_CREATE;

    /*
     * Send notification to all OTHER CPUs
     */
    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        if (i != tpshm->cpuID && tpshm->participants[i])
        {
	    /*
	     * We really do want to ignore the result of the enqueue. If enqueuePipeElement
	     * fails this will be due to the pipe overflowing. Such an overflow will be
	     * detected by the receiving CPU and that CPU will automatically scan for new
	     * ports.
	     */
            (void) EMBXSHM_enqueuePipeElement (tpshm, i, &element);
        }
    }

    EMBX_Info(EMBX_INFO_SHM, ("<<<notifyPeersOfNewPort\n"));
}

/*
 * Return a reference to a local port held in the port table (searching on name)
 * The table locks MUST be held.
 */
static EMBXSHM_LocalPortShared_t *findPortByName (EMBXSHM_Transport_t *tpshm,
                                                  const EMBX_CHAR           *portName)
{
    EMBX_UINT                   i;
    EMBXSHM_LocalPortShared_t **tbl;
    
    EMBX_Info(EMBX_INFO_SHM, (">>>findPortByName\n"));

    EMBX_Assert (tpshm);
    EMBX_Assert (portName);

    EMBXSHM_READS(&tpshm->tcb->portTable);
    EMBXSHM_READS(&tpshm->tcb->portTableSize);

    EMBX_Assert (tpshm->tcb->portTable);

    tbl = (EMBXSHM_LocalPortShared_t**) BUS_TO_LOCAL(tpshm->tcb->portTable, tpshm->pointerWarp);
    EMBX_Assert (EMBXSHM_ALIGNED_TO_CACHE_LINE(tbl));

    EMBXSHM_READS_ARRAY(tbl, tpshm->tcb->portTableSize);

    for (i=0; i<tpshm->tcb->portTableSize; i++)
    {
        EMBXSHM_LocalPortShared_t *ptr;

	ptr = tbl[i];

        if (ptr)
        {
	    ptr = BUS_TO_LOCAL(ptr, tpshm->pointerWarp);
	    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(ptr));

	    EMBXSHM_READS_ARRAY(ptr, EMBX_MAX_PORT_NAME);

            if (!strcmp (portName, ptr->portName))
            {
		EMBX_Info(EMBX_INFO_SHM, (">>>findPortByName = 0x%08x\n", (unsigned) ptr));
                return ptr;
            }
        }
    }

    EMBX_Info(EMBX_INFO_SHM, (">>>findPortByName = NULL\n"));
    return 0;
}

/*
 * Create a remote port reference to a real port
 * A remote port handle structure is allocated, filled in,
 * and added to the list of connections on the local port.
 */
static EMBX_ERROR createRemotePort (EMBXSHM_Transport_t       *tpshm,
                                    EMBXSHM_LocalPortShared_t *localPortShared,
                                    EMBX_RemotePortHandle_t  **port)
{
    EMBXSHM_RemotePortHandle_t *remotePortHandle;
    EMBXSHM_RemotePortLink_t   *remotePortLink;

    EMBX_Info(EMBX_INFO_SHM, (">>>createRemotePort\n"));

    EMBX_Assert (tpshm);
    EMBX_Assert (localPortShared);
    EMBX_Assert (port);

    /*
     * Allocate required memory
     */
    remotePortHandle = (EMBXSHM_RemotePortHandle_t *)EMBX_OS_MemAlloc (sizeof (EMBXSHM_RemotePortHandle_t));
    if (NULL == remotePortHandle)
    {
	EMBX_Info(EMBX_INFO_SHM, ("<<<createRemotePort = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }

    if ((remotePortLink = (EMBXSHM_RemotePortLink_t*)EMBXSHM_malloc (tpshm,
                                                                     sizeof (EMBXSHM_RemotePortLink_t))) == 0)
    {
        EMBX_OS_MemFree (remotePortHandle);
	EMBX_Info(EMBX_INFO_SHM, ("<<<createRemotePort = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }
    EMBX_Assert(remotePortLink && EMBXSHM_ALIGNED_TO_CACHE_LINE(remotePortLink));

    /*
     * Initialize the new remote port structure
     */
    memcpy (remotePortHandle, &EMBXSHM_remoteportHandleTemplate, sizeof (EMBXSHM_RemotePortHandle_t));

    /*
     * Create access lock
     */
    if (!EMBX_OS_MUTEX_INIT (&remotePortHandle->accessMutex))

    {
        EMBX_OS_MemFree (remotePortHandle);
        EMBXSHM_free (tpshm, remotePortLink);
	EMBX_Info(EMBX_INFO_SHM, ("<<<createRemotePort = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }

    EMBXSHM_takeSpinlock (tpshm, &tpshm->portConnectMutex, tpshm->portConnectSpinlock);

    EMBXSHM_READS(localPortShared);

    if (localPortShared->owningCPU == tpshm->cpuID)
    {
        remotePortHandle->destination.localPort = localPortShared->localPort;
    }
    else
    {
        remotePortHandle->destination.sharedPort = localPortShared;
    }

    remotePortHandle->destinationCPU  = localPortShared->owningCPU;
    remotePortHandle->linkageShared   = remotePortLink;

    /*
     * Add it to the local port's list of remote connections
     */
    remotePortLink->owningCPU  = tpshm->cpuID;
    remotePortLink->remotePort = remotePortHandle;

    remotePortLink->nextConnection = localPortShared->remoteConnections;
    remotePortLink->prevConnection = 0;

    if (localPortShared->remoteConnections)
    {
        EMBXSHM_RemotePortLink_t *ptr;

        ptr = BUS_TO_LOCAL (localPortShared->remoteConnections, tpshm->pointerWarp);
	EMBX_Assert(ptr && EMBXSHM_ALIGNED_TO_CACHE_LINE(ptr));
	EMBXSHM_READS(&ptr->prevConnection);
        ptr->prevConnection = (EMBXSHM_RemotePortLink_t*)LOCAL_TO_BUS (remotePortLink, tpshm->pointerWarp);
	EMBXSHM_WROTE(&ptr->prevConnection);
    }

    localPortShared->remoteConnections = (EMBXSHM_RemotePortLink_t *)LOCAL_TO_BUS (remotePortLink, tpshm->pointerWarp);

    EMBXSHM_WROTE(remotePortLink);
    EMBXSHM_WROTE(&localPortShared->remoteConnections);

    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portConnectMutex, tpshm->portConnectSpinlock);

    *port = &remotePortHandle->port;

    EMBX_Info(EMBX_INFO_SHM, ("<<<createRemotePort = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Find where to put a new port in the port table
 * The table locks MUST be held
 * Returns the index of the first free slot, of the
 * table size if none exists.
 */
static EMBX_UINT findFreePortTableSlot (EMBXSHM_Transport_t *tpshm,
                                        const EMBX_CHAR           *portName)
{
    EMBX_UINT                   i;
    EMBXSHM_LocalPortShared_t **tbl;

    EMBX_Info(EMBX_INFO_SHM, (">>>findFreePortTableSlot\n"));

    EMBX_Assert (tpshm);
    EMBX_Assert (portName);

    EMBXSHM_READS(&tpshm->tcb->portTable);
    EMBXSHM_READS(&tpshm->tcb->portTableSize);

    EMBX_Assert (tpshm->tcb->portTable);

    tbl = (EMBXSHM_LocalPortShared_t**) BUS_TO_LOCAL(tpshm->tcb->portTable, tpshm->pointerWarp);
    EMBX_Assert (EMBXSHM_ALIGNED_TO_CACHE_LINE(tbl));

    EMBXSHM_READS_ARRAY(tbl, tpshm->tcb->portTableSize);

    for (i=0; i<tpshm->tcb->portTableSize; i++)
    {
        if (!tbl[i])
        {
            break;
        }
    }

    EMBX_Info(EMBX_INFO_SHM, ("<<<findFreePortTableSlot = %d\n", i));
    return i;
}

/*
 * Grow the local port table to accomodate a new port.
 * The port table locks MUST be held
 */
static EMBX_ERROR growPortTable (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_LocalPortShared_t **tbl;
    EMBX_UINT                  newTableSize;
    EMBXSHM_LocalPortShared_t *newTable;

    EMBX_Info(EMBX_INFO_SHM, (">>>growPortTable\n"));

    EMBX_Assert (tpshm);

    EMBXSHM_READS(&tpshm->tcb->portTable);
    EMBXSHM_READS(&tpshm->tcb->portTableSize);

    EMBX_Assert (tpshm->tcb->portTable);
    tbl = BUS_TO_LOCAL (tpshm->tcb->portTable, tpshm->pointerWarp);
    EMBX_Assert (EMBXSHM_ALIGNED_TO_CACHE_LINE(tbl));

    EMBXSHM_READS_ARRAY(tbl, tpshm->tcb->portTableSize);

    /*
     * Check we are allowed to grow it
     */
    newTableSize = tpshm->tcb->portTableSize * 2;
    EMBX_Assert(newTableSize > tpshm->tcb->portTableSize);
    if (tpshm->transport.transportInfo.maxPorts > 0)
    {
	EMBX_Info(EMBX_INFO_SHM, ("<<<growPortTable = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }

    /*
     * Reallocate it, to double its current size
     */
    if ((newTable = EMBXSHM_malloc (tpshm,
                                    newTableSize*sizeof (EMBXSHM_LocalPortShared_t *))) == 0)
    {
	EMBX_Info(EMBX_INFO_SHM, ("<<<growPortTable = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }

    memset (newTable, 0, newTableSize*sizeof (EMBXSHM_LocalPortShared_t *));
    memcpy (newTable, tbl,
            tpshm->tcb->portTableSize*sizeof (EMBXSHM_LocalPortShared_t *));

    EMBXSHM_free (tpshm, tbl);

    EMBX_Assert (EMBXSHM_ALIGNED_TO_CACHE_LINE(newTable));
    tpshm->tcb->portTable     = (EMBXSHM_LocalPortShared_t**)LOCAL_TO_BUS (newTable, tpshm->pointerWarp);
    tpshm->tcb->portTableSize = newTableSize;

    EMBXSHM_WROTE(&tpshm->tcb->portTable);
    EMBXSHM_WROTE(&tpshm->tcb->portTableSize);
    EMBXSHM_WROTE_ARRAY(newTable, newTableSize);

    EMBX_Info(EMBX_INFO_SHM, ("<<<growPortTable = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Complete any pending local connections for a newly created port.
 * The caller should hold the port table locks.
 */
static void completeConnections (EMBXSHM_Transport_t       *tpshm,
                                 EMBXSHM_LocalPortShared_t *localPortShared)
{
    EMBX_EventState_t *es;
    EMBX_EventState_t *next;

    EMBX_Info(EMBX_INFO_SHM, (">>>completeConnections\n"));
    
    EMBX_Assert (tpshm);
    EMBX_Assert (localPortShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));

    EMBX_OS_MUTEX_TAKE (&tpshm->connectionListMutex);

    EMBXSHM_READS(&localPortShared->portName);

    for (es = tpshm->connectionRequests; es; es = next)
    {
	EMBXSHM_ConnectBlockState_t *cbs = es->data; /* match with CBS IN DATA */

        next = es->next;
        
        /*
         * See if we've found a port to connect to 
         */
	EMBX_Info(EMBX_INFO_SHM, ("   comparing %s and %s\n", localPortShared->portName, cbs->portName));
        if (!strcmp (localPortShared->portName, cbs->portName))
        {
	    /*
	     * We've found one; remove it from the list.
	     */
	    EMBX_EventListRemove(&tpshm->connectionRequests, es);

            /*
             * Try to make that connection
             */
	    es->result = createRemotePort (tpshm, localPortShared, cbs->port);

            /*
             * Signal the wait - it either worked or it didn't
	     * NOTE: from the moment we signal the event we no longer own es
             */
            EMBX_OS_EVENT_POST (&es->event);

            EMBX_OS_MemFree (cbs);
        }
    }

    EMBX_OS_MUTEX_RELEASE (&tpshm->connectionListMutex);

    EMBX_Info(EMBX_INFO_SHM, ("<<<completeConnections\n"));
}

/*
 * Transport method to create a new local port
 */
#ifdef EMBX_RECEIVE_CALLBACK
static EMBX_ERROR createLocalPort (EMBX_TransportHandle_t  *tp,
				   const EMBX_CHAR         *portName,
				   EMBX_LocalPortHandle_t **port,
				   EMBX_Callback_t          callback, EMBX_HANDLE handle)
#else
static EMBX_ERROR createLocalPort (EMBX_TransportHandle_t  *tp,
				   const EMBX_CHAR         *portName,
				   EMBX_LocalPortHandle_t **port)
#endif /* EMBX_RECEIVE_CALLBACK */
{           
    EMBXSHM_LocalPortShared_t  *localPortShared = 0;
    EMBXSHM_Transport_t        *tpshm;
    EMBXSHM_LocalPortHandle_t  *localPort;
    EMBXSHM_LocalPortShared_t **portTable;
    EMBX_ERROR                  result;
    EMBX_UINT                   freeSlot;
    EMBX_UINT                   i;

    EMBX_Info(EMBX_INFO_SHM, (">>>createLocalPort\n"));

    EMBX_Assert (tp);
    EMBX_Assert (portName);
    EMBX_Assert (port);

    tpshm = (EMBXSHM_Transport_t *)tp->transport;

    /*
     * Take the port table locks, so we can mess with the port table
     */
    EMBXSHM_takeSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    /*
     * Check if it already exists in the transport table
     */
    if (findPortByName (tpshm, portName))
    {
        EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

	EMBX_Info(EMBX_INFO_SHM, ("<<<createLocalPort = EMBX_ARLEADY_BIND\n"));
        return EMBX_ALREADY_BIND;
    }

    /*
     * Find a free slot in the port table (if there is one...)
     */
    freeSlot = findFreePortTableSlot (tpshm, portName);

    EMBXSHM_READS(&tpshm->tcb->portTable);
    EMBXSHM_READS(&tpshm->tcb->portTableSize);

    /*
     * Increase the size of the port table if there is no space
     */
    if (freeSlot == tpshm->tcb->portTableSize)
    {
        if ((result = growPortTable (tpshm)) != EMBX_SUCCESS)
        {
            EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

	    EMBX_Info(EMBX_INFO_SHM, ("<<<createLocalPort = %d\n", result));
            return result;
        }
    }

    /*
     * Create & init the local port struct
     */
    localPort = (EMBXSHM_LocalPortHandle_t *)EMBX_OS_MemAlloc (sizeof (EMBXSHM_LocalPortHandle_t));
    if (NULL == localPort)
    {
        goto error_exit;
    }
    memset(localPort, 0, sizeof (EMBXSHM_LocalPortHandle_t));
    memcpy (localPort, &EMBXSHM_localportHandleTemplate, sizeof (EMBXSHM_LocalPortHandle_t));

    /*
     * Alloc space for the shared part of the new port handle
     */
    if ((localPortShared = EMBXSHM_malloc (tpshm,
                                           sizeof (EMBXSHM_LocalPortShared_t))) == 0)
    {
        goto error_exit;
    }
    EMBX_Assert(localPortShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));
    memset (localPortShared, 0, sizeof (EMBXSHM_LocalPortShared_t));

    /*
     * Fill in sundry fields
     */
    localPortShared->owningCPU         = tpshm->cpuID;
    localPortShared->remoteConnections = 0;
    localPortShared->localPort         = localPort;

    localPort->tableIndex              = freeSlot;
    localPort->localPortShared         = localPortShared;

    /*
     * Put a copy of the port name in the shared section
     */
    strcpy (localPortShared->portName, portName);

    /*
     * Create and populate free lists for the port
     */
    for (i=0; i<tpshm->freeListSize; i++)
    {
        EMBXSHM_RecEventList_t *ev;

        ev = (EMBXSHM_RecEventList_t *)EMBX_OS_MemAlloc (sizeof (EMBXSHM_RecEventList_t));
        if (ev != NULL)
        {
            ev->next                 = localPort->freeRecEvents;
            localPort->freeRecEvents = ev;
        }
        else
        {
            goto error_exit;
        }
    }

#ifdef EMBX_RECEIVE_CALLBACK
    localPort->callback = callback;
    localPort->callbackHandle = handle;
#endif /* EMBX_RECEIVE_CALLBACK */

    /*
     * Convert addresses in shared memory to bus format
     */
    portTable           = (EMBXSHM_LocalPortShared_t**)BUS_TO_LOCAL (tpshm->tcb->portTable, tpshm->pointerWarp);
    EMBXSHM_READS(&portTable[freeSlot]); /* lose potentially stale neighbouring info */
    portTable[freeSlot] = (EMBXSHM_LocalPortShared_t*)LOCAL_TO_BUS (localPortShared, tpshm->pointerWarp);

    EMBXSHM_WROTE(localPortShared);
    EMBXSHM_WROTE(&portTable[freeSlot]);

    /*
     * Fill in the state & port handle now, before we notify the peers,
     * because they might try to use the port before the shell gets round
     * to filling these in.
     */
    localPort->port.transportHandle = tp;
    localPort->port.state           = EMBX_HANDLE_VALID;

    /*
     * Complete connections for any blocked local connection requests to this port.
     */
    completeConnections (tpshm, localPortShared);
    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    /*
     * Let the others know we've changed the port table
     */
    notifyPeersOfNewPort (tpshm);

    /*
     * Pass back the port we just made
     */
    *port = &localPort->port;

    EMBX_Info(EMBX_INFO_SHM, ("<<<createLocalPort = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;

/*
 * Tidy up logic in the event of failure. Appologies for the goto
 * used here, but it does simplify things a lot
 */
error_exit:

    if (localPort)
    {
        while (localPort->freeRecEvents)
        {
            EMBXSHM_RecEventList_t *next = localPort->freeRecEvents->next;

            EMBX_OS_MemFree (localPort->freeRecEvents);
            localPort->freeRecEvents = next;
        }

        EMBX_OS_MemFree (localPort);
    }

    if (localPortShared)
    {
        EMBXSHM_free (tpshm, localPortShared);
    }

    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    *port = 0;

    EMBX_Info(EMBX_INFO_SHM, ("<<<createLocalPort = EMBX_NOMEM\n"));
    return EMBX_NOMEM;
}


/*
 * Connect to a remote port (but don't block if it doesn't exist on the remote side) 
 */
static EMBX_ERROR connectAsync (EMBX_TransportHandle_t   *tp,
                                const EMBX_CHAR          *portName,
                                EMBX_RemotePortHandle_t **port)
{                       
    EMBX_ERROR                 result;
    EMBXSHM_Transport_t       *tpshm;
    EMBXSHM_LocalPortShared_t *localPortShared;

    EMBX_Info(EMBX_INFO_SHM, (">>>connectAsync\n"));

    EMBX_Assert (tp);
    EMBX_Assert (portName);
    EMBX_Assert (port);

    tpshm = (EMBXSHM_Transport_t *)tp->transport;

    /*          
     * Find the port to which we want to connect; if we couldn't find it, we
     * need to return that the port name was invalid, ie. hasn't been bound.
     */         
    EMBXSHM_takeSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    localPortShared = findPortByName (tpshm, portName);

    if (localPortShared)
    {
	EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));
        result = createRemotePort (tpshm, localPortShared, port);
    }
    else
    {
        result = EMBX_PORT_NOT_BIND;
    }

    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    EMBX_Info(EMBX_INFO_SHM, ("<<<connectAsync = %d\n", result));
    return result;
}

/*
 * Open a connection to a named port on this transport, and get ready
 * for a blocking wait on it to complete.
 */
static EMBX_ERROR connectBlock (EMBX_TransportHandle_t   *tp,
                                const EMBX_CHAR          *portName,
                                EMBX_EventState_t        *es,
                                EMBX_RemotePortHandle_t **port)
{
    EMBXSHM_Transport_t       *tpshm;
    EMBXSHM_LocalPortShared_t *localPortShared;
    EMBX_ERROR                 result;

    EMBX_Info(EMBX_INFO_SHM, (">>>connectBlock\n"));

    EMBX_Assert (tp);
    EMBX_Assert (portName);
    EMBX_Assert (es);
    EMBX_Assert (port);

    tpshm = (EMBXSHM_Transport_t *)tp->transport;

    /*
     * Look to see if its there ready for us
     */
    EMBXSHM_takeSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    localPortShared = findPortByName (tpshm, portName);

    if (localPortShared)
    {
        /*
         * It is, so lets connect to it
         */
	EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));
        result = createRemotePort (tpshm, localPortShared, port);
    }
    else
    {
        EMBXSHM_ConnectBlockState_t *cbs;

        /*
         * The port name has not been found so try and block the task until it
         * is created, then wake the waiter. 
         */
        cbs = (EMBXSHM_ConnectBlockState_t *)EMBX_OS_MemAlloc (sizeof (EMBXSHM_ConnectBlockState_t));
        if (NULL == cbs)
        {
            EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

	    EMBX_Info(EMBX_INFO_SHM, ("<<<connectBlock = EMBX_NOMEM\n"));
            return EMBX_NOMEM;
        }

	cbs->portName = portName;
	cbs->port = port;
	cbs->requestingHandle = tp;
	es->data = cbs; /* match with CBS IN DATA */

        EMBX_OS_MUTEX_TAKE (&tpshm->connectionListMutex);
	EMBX_EventListAdd(&tpshm->connectionRequests, es);
        EMBX_OS_MUTEX_RELEASE (&tpshm->connectionListMutex);

	/*
	 * We will be unblocked by completeConnections which is called from task context
	 * whenever we are notified that a new port has been created.
	 */
        result = EMBX_BLOCK;
    }

    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    EMBX_Info(EMBX_INFO_SHM, ("<<<connectBlock = %d\n", result));
    return result;
}

/*
 * Function to rip a pending connection down
 */
static EMBX_VOID connectInterrupt (EMBX_TransportHandle_t *tp,
                                   EMBX_EventState_t      *es)
{
    EMBXSHM_Transport_t        *tpshm;

    EMBX_Info(EMBX_INFO_SHM, (">>>connectInterrupt\n"));

    EMBX_Assert (tp);
    EMBX_Assert (es);

    tpshm = (EMBXSHM_Transport_t *)tp->transport;

    EMBX_OS_MUTEX_TAKE (&tpshm->connectionListMutex);
    EMBX_EventListRemove(&(tpshm->connectionRequests), es);
    EMBX_OS_MUTEX_RELEASE (&tpshm->connectionListMutex);

    EMBX_OS_MemFree (es->data); /* match with CBS IN DATA */

    EMBX_Info(EMBX_INFO_SHM, ("<<<connectInterrupt\n"));
    return;
}

/*
 * Notification routine called when the port table has been updated
 * to include a new port. We scan our list of blocked connectors to
 * see if one of our waiters can proceed.
 */
void EMBXSHM_newPortNotification (EMBXSHM_Transport_t *tpshm)
{
    EMBX_UINT                   i;
    EMBXSHM_LocalPortShared_t **tbl;

    EMBX_Info(EMBX_INFO_SHM, (">>>EMBXSHM_newPortNotification\n"));

    EMBX_Assert (tpshm);

    EMBXSHM_takeSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    EMBXSHM_READS(&tpshm->tcb->portTable);
    EMBXSHM_READS(&tpshm->tcb->portTableSize);

    /*
     * For each port in the table, try to complete any pending connections
     */
    EMBX_Assert (tpshm->tcb->portTable);
    tbl = (EMBXSHM_LocalPortShared_t**)BUS_TO_LOCAL(tpshm->tcb->portTable, tpshm->pointerWarp);
    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(tbl));

    EMBXSHM_READS_ARRAY(tbl, tpshm->tcb->portTableSize);

    for (i=0; i<tpshm->tcb->portTableSize; i++)
    {
	if (tbl[i])
	{
	    completeConnections (tpshm,
				(EMBXSHM_LocalPortShared_t*)BUS_TO_LOCAL (tbl[i], tpshm->pointerWarp));
	}
    }

    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    EMBX_Info(EMBX_INFO_SHM, ("<<<EMBXSHM_newPortNotification\n"));
}

/*
 * Transport handle structures
 */
static EMBX_TransportHandleMethods_t transportHandleMethods =
{
    createLocalPort,
    connectAsync,
    connectBlock,
    connectInterrupt
};

EMBXSHM_TransportHandle_t EMBXSHM_transportHandleTemplate =
{
    &transportHandleMethods,
    EMBX_HANDLE_INVALID
};

