/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_localport.c                                  */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport                            */
/*         local port implementation                          */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"

#define _LOCAL_OFFSET ((EMBX_UINT)&((EMBX_Buffer_t *)0)->data)

#define POLL_INTERVAL 10	/* ms */
#define MAX_DELAY     1000	/* ms */

/* HACK: The weird code in embxshm_death allows the TCB's activeCPUs
 * to be modified but this change is not broadcast to other processors.
 * This means that the local copy of the participants map may become
 * desynchronized. This code is called (off the critical path) during
 * port shutdown to ensure that the local participants map can be put
 * back into sync.
 */
static int updateLocalParticipantsMap(EMBXSHM_Transport_t *tpshm, int cpuID)
{
    int active;

    EMBXSHM_READS(&tpshm->tcb->activeCPUs[cpuID].marker);
    active = tpshm->tcb->activeCPUs[cpuID].marker;

    if (active != tpshm->participants[cpuID]) {
	EMBX_Assert(0 == active && 1 == tpshm->participants[cpuID]);
	EMBX_Info(EMBX_INFO_SHM, ("   Detected removal of cpu %d\n", cpuID));
	tpshm->participants[cpuID] = active;
	return 1;
    }

    return 0;
}

/*
 * Notify all participants in the transport (including us!) that
 * a local port is going away, and they must invalidate all
 * their remote ports.
 * When this call returns all participants have acknowledged its
 * demise, so we can carry on.
 */
static void notifyPeersOfPortClosure (EMBXSHM_Transport_t       *tpshm,
                                      EMBXSHM_LocalPortShared_t *localPortShared)
{
    EMBXSHM_PipeElement_t  element;
    EMBX_UINT              i;

    EMBX_Info(EMBX_INFO_SHM, (">>>notifyPeersOfPortClosure\n"));

    EMBX_Assert (tpshm);
    EMBX_Assert (localPortShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));

    /*
     * The invalidated area should have been untouched since boot
     * check this.
     */
#ifdef EMBX_VERBOSE
    EMBXSHM_READS(&localPortShared->invalidated);
    for (i=0; i<EMBXSHM_MAX_CPUS; i++) {
	EMBX_Assert(0 == localPortShared->invalidated[i].marker);
    }
#endif

    /*
     * Ensure invalidate acknowledgement area is clean
     */
    memset ((void*)localPortShared->invalidated, 0, sizeof (localPortShared->invalidated));
    EMBXSHM_WROTE(localPortShared);

    /*
     * Wake up the closed port handler on this CPU
     */
    tpshm->closedPorts[tpshm->cpuID] = localPortShared;

    /*
     * Close the port on this CPU. Synchronous call to cleanup the port structures on this CPU.
     * Formerly, we signalled a semaphore wo wake the port-closed thread so the system 
     * was symmeterical to the top/bottom half interrupt handlers, but the thread will not 
     * necessarily run in linux kernel mode
     */
    EMBXSHM_portClose(tpshm);

    /*
     * Tell all the other CPUs about the imminent demise of this local port
     */

    element.control                = EMBXSHM_PIPE_CTRL_PORT_CLOSED;
    element.data.port_closed.cpuID = tpshm->cpuID;
    element.data.port_closed.port  = (EMBXSHM_LocalPortShared_t*)LOCAL_TO_BUS(localPortShared, tpshm->pointerWarp); 

    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        if (i != tpshm->cpuID && tpshm->participants[i])
	{
	    EMBX_ERROR err;

	    /*
	     * This is a polled wait the communications pipe to permit an enqueue.
	     * This is a bit clumsy but we are already half way through an 
	     * operation at this point so aborting is a little tricky.
	     */
	    while (EMBX_SUCCESS != (err = EMBXSHM_enqueuePipeElement (tpshm, i, &element))) 
	    {
		EMBX_Assert(EMBX_NOMEM == err);
		EMBX_OS_Delay (POLL_INTERVAL);

		if (updateLocalParticipantsMap(tpshm, i)) {
		    break;
		}
	    }
                EMBX_Info(EMBX_INFO_SHM, ("   Enqueing cpu %d with close notification\n", tpshm->cpuID));
	}
    }

    /*
     * Now wait for all participants to acknowledge its death
     */
    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
        /* 
         * We don't really need to wait on this CPU because a synchnronous call to EMBX_closePort
         * was made above. But it's a sanity check!
         */
        if (tpshm->participants[i])
	{            
	    unsigned int delay, accumulatedDelay, maxPollInterval;
	    int maxDelay;
	    
            EMBX_Info(EMBX_INFO_SHM, ("   Waiting for cpu %d to ack invalidate - address 0x%x, val 0x%x\n", i, (int) &(localPortShared->invalidated[i]), localPortShared->invalidated[i].marker ));

	    maxPollInterval = POLL_INTERVAL; /* might be a function! */
	    accumulatedDelay = 0;
	    delay = 1;
	    maxDelay = MAX_DELAY;
	    
	    EMBXSHM_READS(&localPortShared->invalidated[i].marker);
	    while (!(localPortShared->invalidated[i].marker))
	    {
		/* the time spent delaying here significantly affects the
		 * performance of MME_TermTransformer (multiple port
		 * closures) so here we use a very simple exponential
		 * back-off algorithm to preserve performance in the normal
		 * case.
		 */
		EMBX_OS_Delay (delay);

		/* TIMEOUT SUPPORT: break out of here after a maximum period */
		if ((maxDelay = (maxDelay - delay)) <= 0)
		{
		    EMBX_Info(EMBX_INFO_SHM, ("   TIMEOUT Waiting for cpu %d to ack invalidate - address 0x%x, val 0x%x\n",
					      i, (int) &(localPortShared->invalidated[i]),
					      localPortShared->invalidated[i].marker ));
		    break;
		}

		if (delay < maxPollInterval) {
			accumulatedDelay += delay;
			if (accumulatedDelay > maxPollInterval) {
				accumulatedDelay = 0;
				delay <<= 1;
				if (delay >= maxPollInterval) {
					delay = maxPollInterval;

					if (updateLocalParticipantsMap(tpshm, i)) {
					    break;
					}
				}
			}
		}

		/* ensure any buffering is cleared before we re-examine the
		 * invalidated structure
		 */
		tpshm->buffer_flush (tpshm, (void *) &(localPortShared->invalidated[i].marker));
		
		EMBXSHM_READS(&localPortShared->invalidated[i].marker);
	    }
	    EMBX_Info(EMBX_INFO_SHM, ("   cpu %d has acknowlegded the port closure\n", i));
	}
	else
	{
	    EMBX_Assert(localPortShared->invalidated[i].marker == 0);
	    EMBX_Info(EMBX_INFO_SHM, ("   cpu %d is not a particpatant\n", i));
	}
    }

    EMBX_Info(EMBX_INFO_SHM, ("<<<notifyPeersOfPortClosure\n"));
}

/*
 * Routine to clean up a port, invalidating all remote ports.
 */
static void cleanupLocalPort (EMBXSHM_Transport_t       *tpshm,
                              EMBXSHM_LocalPortHandle_t *localPort,
                              EMBX_ERROR                 blockedResult)
{
    EMBXSHM_LocalPortShared_t  *localPortShared;
    EMBX_EventList_t           *rbl;
    EMBXSHM_RecEventList_t     *evl;
    EMBXSHM_RecEventList_t     *evfl;

    EMBX_Info(EMBX_INFO_SHM, (">>>cleanupLocalPort\n"));

    EMBX_Assert (localPort);
    EMBX_Assert (localPort->port.transportHandle);

    localPortShared = localPort->localPortShared;
    EMBX_Assert (localPortShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));

    /*
     * Invalidate all remote connections to this port. We do this by notifying 
     * all participants that this local port is going away, and then waiting 
     * to see the participants acknowledge.
     */
    notifyPeersOfPortClosure (tpshm, localPortShared);

    /*
     * Free the shared part of the port structure.
     */
    EMBXSHM_free (tpshm, localPort->localPortShared);

    /*
     * Destroy any pending event structures and queued message buffers
     */
    EMBX_OS_INTERRUPT_LOCK ();

    evl  = localPort->receivedHead;
    evfl = localPort->freeRecEvents;

    localPort->receivedHead  = 0;
    localPort->receivedTail  = 0;
    localPort->freeRecEvents = 0;

    EMBX_OS_INTERRUPT_UNLOCK ();

    while (evl)
    {
        EMBXSHM_RecEventList_t *next = evl->next;
    
        if (evl->recev.type == EMBX_REC_MESSAGE)
        {
	    EMBXSHM_free (tpshm, evl->recev.data);
        }

        EMBX_OS_MemFree (evl);
        evl = next;
    }

    while (evfl)
    {
        EMBXSHM_RecEventList_t *next = evfl->next;
    
        EMBX_OS_MemFree (evfl);
        evfl = next;
    }

    /*
     * Destroy the blocked receivers list and wake them up with
     * the supplied error code.
     */
    EMBX_OS_INTERRUPT_LOCK ();

    rbl  = localPort->blockedReceivers;
    localPort->blockedReceivers = 0;

#ifdef EMBX_RECEIVE_CALLBACK
    localPort->callback = NULL;
    localPort->callbackHandle = 0;
#endif /* EMBX_RECEIVE_CALLBACK */

    EMBX_OS_INTERRUPT_UNLOCK ();

    
    while (rbl)
    {
        EMBX_EventState_t *ev = EMBX_EventListFront(&rbl);

        ev->result = blockedResult;
        EMBX_OS_EVENT_POST (&ev->event);
    }

    EMBX_Info(EMBX_INFO_SHM, ("<<<cleanupLocalPort\n"));
}

/*
 * Local port method to invalidate itself
 */
static EMBX_ERROR invalidatePort (EMBX_LocalPortHandle_t *ph)
{
    EMBXSHM_LocalPortHandle_t *localPort = (EMBXSHM_LocalPortHandle_t *)ph;
    EMBXSHM_Transport_t *tpshm;
    EMBXSHM_LocalPortHandle_t **tbl;

    EMBX_Info(EMBX_INFO_SHM, (">>>invalidatePort\n"));

    EMBX_Assert (ph);

    tpshm = (EMBXSHM_Transport_t *)localPort->port.transportHandle->transport;
    EMBX_Assert (tpshm);

    EMBXSHM_takeSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    if (ph->state == EMBX_HANDLE_INVALID) 
    {
        EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);
	return EMBX_INVALID_PORT;
    }

    ph->state = EMBX_HANDLE_INVALID;

    /*
     * We need to rip it out of the port table, BEFORE we send the invalidates
     */
    EMBXSHM_READS(&tpshm->tcb->portTable);
    tbl = (EMBXSHM_LocalPortHandle_t**)BUS_TO_LOCAL (tpshm->tcb->portTable, tpshm->pointerWarp);

    EMBXSHM_READS(&tbl[localPort->tableIndex]);
    EMBX_Assert (tbl[localPort->tableIndex] ==
                 (EMBXSHM_LocalPortHandle_t*)LOCAL_TO_BUS (localPort->localPortShared, tpshm->pointerWarp));

    tbl[localPort->tableIndex] = 0;
    EMBXSHM_WROTE(&tbl[localPort->tableIndex]);

    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portTableMutex, tpshm->portTableSpinlock);

    /*
     * Cleanup any memory associated with the port (other then the local port
     * handle)
     */
    cleanupLocalPort (tpshm, localPort, EMBX_PORT_INVALIDATED);
    
    EMBX_Info(EMBX_INFO_SHM, ("<<<invalidatePort = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Local port method to close itself
 */
static EMBX_ERROR closePort (EMBX_LocalPortHandle_t *ph)
{
    EMBX_Info(EMBX_INFO_SHM, (">>>closePort\n"));

    EMBX_Assert (ph);

    if (EMBX_HANDLE_INVALID != ph->state)
    {
	EMBX_ERROR err;
	err = invalidatePort(ph);
	EMBX_Assert(EMBX_SUCCESS == err);
	EMBX_Assert(EMBX_HANDLE_INVALID == ph->state);
    }

    /*
     * Set the state to closed to enable paranoia checks.
     */
    ph->state = EMBX_HANDLE_CLOSED;

    /*
     * Its now safe to free up its resources
     */
    EMBX_OS_MemFree (ph);

    EMBX_Info(EMBX_INFO_SHM, ("<<<closePort = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Polling version of receive.
 */
static EMBX_ERROR receive (EMBX_LocalPortHandle_t *ph,
                           EMBX_RECEIVE_EVENT     *recev)
{
    EMBXSHM_LocalPortHandle_t  *localPort;
    EMBXSHM_Transport_t        *tpshm;
    EMBXSHM_RecEventList_t     *receiveEvent;
    EMBXSHM_RecEventList_t     *freeRecEvents;

    EMBX_Info(EMBX_INFO_SHM, (">>>receive\n"));

    EMBX_Assert (ph);
    EMBX_Assert (recev);

    localPort = (EMBXSHM_LocalPortHandle_t *)ph;

    tpshm = (EMBXSHM_Transport_t *)localPort->port.transportHandle->transport;
    EMBX_Assert (tpshm);

    /*
     * If there's nothing pending, you're out of luck
     */
    if (localPort->receivedHead == 0)
    {
	EMBX_Info(EMBX_INFO_SHM, ("<<<receive = EMBX_INVALID_STATUS\n"));
        return EMBX_INVALID_STATUS;
    }

    /*
     * Pull off the next pending message
     */
    EMBX_OS_INTERRUPT_LOCK ();

    receiveEvent = localPort->receivedHead;

    if ((localPort->receivedHead = receiveEvent->next) == 0)
    {
        localPort->receivedTail = 0;
    }

    EMBX_OS_INTERRUPT_UNLOCK ();

    /*
     * Take a copy of the event
     */
    *recev = receiveEvent->recev;

    /*
     * Free off the list stucture we used to manage
     * the buffer
     */
    EMBX_OS_INTERRUPT_LOCK ();

    freeRecEvents            = localPort->freeRecEvents;
    receiveEvent->next       = freeRecEvents;
    localPort->freeRecEvents = receiveEvent;

    if (NULL == freeRecEvents && !tpshm->secondHalfRunFlag) 
    {
	/*
	 * We are recovering from an overflow condition here. As such
	 * it is possible that there are events for this port pending
	 * on the communications pipe without an interrupt being 
	 * asserted. To prevent the system locking up we will
	 * speculatively schedule the second half interrupt handler.
	 */

	tpshm->secondHalfRunFlag = 1;
	/* OS20 does not permit semaphores to be signaled from interrupt */
	EMBX_OS_INTERRUPT_UNLOCK ();
	EMBX_OS_EVENT_POST (&tpshm->secondHalfEvent);
    } else {
        EMBX_OS_INTERRUPT_UNLOCK ();
    }

    EMBX_Info(EMBX_INFO_SHM, ("<<<receive = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Blocking version of receive
 */
static EMBX_ERROR receiveBlock (EMBX_LocalPortHandle_t *ph,
                                EMBX_EventState_t      *es,
                                EMBX_RECEIVE_EVENT     *recev)
{
    EMBXSHM_LocalPortHandle_t  *localPort;

    EMBX_Info(EMBX_INFO_SHM, (">>>receiveBlock\n"));

    EMBX_Assert (ph);
    EMBX_Assert (es);
    EMBX_Assert (recev);

    localPort = (EMBXSHM_LocalPortHandle_t *)ph;

    /*
     * Have a quick poll...
     */
    if (receive (ph, recev) == EMBX_SUCCESS)
    {
	EMBX_Info(EMBX_INFO_SHM, ("<<<receiveBlock = EMBX_SUCCESS\n"));
        return EMBX_SUCCESS;
    }

    EMBX_OS_INTERRUPT_LOCK ();

    /*
     * Have another peek under interrupt lock this time
     * If there is now something there, call on to
     * receive since it will now work.
     */
    if (localPort->receivedHead)
    {
        EMBX_OS_INTERRUPT_UNLOCK ();
	EMBX_Info(EMBX_INFO_SHM, ("<<<receiveBlock (tail call to receive)\n"));
        return receive (ph, recev);
    }

    /*
     * Nope, still nothing there, so we need to block so link the receive event into
     * the list of receivers.
     */
    es->data = recev; /* matched with RECEV IN DATA */
    EMBX_EventListAdd(&(localPort->blockedReceivers), es);
    
    EMBX_OS_INTERRUPT_UNLOCK ();

    EMBX_Info(EMBX_INFO_SHM, ("<<<receiveBlock = EMBX_BLOCK (event = 0x%08x)\n", (unsigned) &es));
    return EMBX_BLOCK;
}

/*
 * Port method to tidy up an interrupted blocked receiver
 */
static EMBX_VOID receiveInterrupt (EMBX_LocalPortHandle_t   *ph,
                                   EMBX_EventState_t *es)
{
    EMBXSHM_LocalPortHandle_t *localPort;

    EMBX_Info(EMBX_INFO_SHM, (">>>receiveInterrupt\n"));

    EMBX_Assert (ph);
    EMBX_Assert (es);

    localPort = (EMBXSHM_LocalPortHandle_t *)ph;

    /*
     * Find the blocked receiver with the given event state pointer
     * and remove its entry from the list as its task has just been
     * interrupted by something other than EMBX and its about to
     * disappear from under us.
     */
    EMBX_OS_INTERRUPT_LOCK ();
    EMBX_EventListRemove(&(localPort->blockedReceivers), es);
    EMBX_OS_INTERRUPT_UNLOCK ();

    EMBX_Info(EMBX_INFO_SHM, ("<<<receiveInterrupt\n"));
}


/*
 * Notification that someone has plonked a message or object into this CPU's
 * pipe.
 *
 * See if anyone is waiting, and if so give them the event directly. If no
 * one is waiting, enqueue the event.
 *
 * We are called from interrupt handler context or with mailbox interrupts 
 * already disabled, this protects use from re-entry. However we are *not*
 * called with the timeslice interrupt locked out so we must still lock
 * interrupts when we modify the lists.
 */
EMBX_ERROR EMBXSHM_receiveNotification (EMBXSHM_LocalPortShared_t *localPortShared,
                               EMBX_RECEIVE_EVENT        *event,
			       EMBX_BOOL                  updateOnly,
			       EMBX_BOOL		  interruptCtx)
{
    EMBXSHM_LocalPortHandle_t *localPort;
    EMBXSHM_Transport_t       *tpshm;

    EMBX_Info(EMBX_INFO_INTERRUPT, (">>>EMBXSHM_receiveNotification(0x%08x, ..., %s, %s)\n", 
		(unsigned) localPortShared, 
		(updateOnly ? "true" : "false"), (interruptCtx ? "interrupt" : "thread") ));

    EMBX_Assert (localPortShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));
    EMBX_Assert (event);
    EMBX_Assert ((!updateOnly) || (EMBX_REC_OBJECT == event->type));

    EMBXSHM_READS(&localPortShared->localPort);

    localPort = localPortShared->localPort;
    EMBX_Assert (localPort);

    tpshm = (EMBXSHM_Transport_t *) localPort->port.transportHandle->transport;
    EMBX_Assert (tpshm);

    /*
     * If the local port is not open - drop the data
     */
    if (localPort->port.state != EMBX_HANDLE_VALID)
    {
	EMBX_Info(EMBX_INFO_INTERRUPT, ("   Invalid port handle - dropped incoming message on the floor\n"
	                                "<<<EMBXSHM_newMessageNotification\n"));

	/*
	 * We must return EMBX_SUCCESS here since with have performed
	 * 'error recovery' by ignoring the totally invalid message
	 */
        return EMBX_SUCCESS;
    }

    /*
     * Ensure our incoming data is not held in the local data cache. This includes the buffer
     * header which is read by the shell (i.e. without cache management) in EMBX_SendMessage()
     * and EMBX_Free().
     */
    EMBXSHM_READS((EMBX_Buffer_t *) (((size_t)event->data) - SIZEOF_EMBXSHM_BUFFER));
    EMBXSHM_READS_ARRAY(((char *) event->data), event->size);

    /*
     * If we are a object notification we may have to update the shared object
     */
    if (EMBX_REC_OBJECT == event->type)
    {
	/*
	 * We need to check to see if this is a shadowed object which we own.
	 * If it is we must copy the updated data from the shared memory into
	 * the local memory/ Also, any pointers to data passed around in event
	 * structures should refer to the local data, not the shared data.
	 */
	EMBXSHM_Object_t *obj = EMBXSHM_findObject(tpshm, event->handle);
	EMBX_Assert(obj && 
		    EMBXSHM_ALIGNED_TO_CACHE_LINE(obj) &&
	            ((event->offset+event->size) <= obj->size));

	if (obj->shadow && (obj->owningCPU == tpshm->cpuID))
	{
	    /* 
	     * If we are running from an interrupt we cannot perform the memcpy
	     * (which is time consuming. We are forced to return an error, in
	     * the fullness of time this function will then be called from task
	     * context and the update can take place.
	     */
	    if (interruptCtx) 
	    {
		EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<EMBXSHM_newMessageNotification = EMBX_INVALID_STATUS\n"));
		return EMBX_INVALID_STATUS;
	    }

	    /* copy the object back to its original location */
#if defined __OS20__ || defined __OS21__
	    EMBX_Assert (task_context(NULL, NULL) != task_context_interrupt);
#endif
	    EMBX_Info(EMBX_INFO_INTERRUPT, ("   updating object 0x%08x -> 0x%08x\n",
	              (unsigned) event->data, (unsigned) obj->localData));

	    memcpy ((void *) ((uintptr_t) obj->localData),
	            (void *) ((uintptr_t) event->data),
		    event->size);

	    /* update our idea of where the data is kept */
	    event->data = obj->localData;
	}
    } else {
	/* ensure the buffer header is not in the local cache */
	EMBXSHM_READS((EMBXSHM_Buffer_t *) (((size_t) event->data) - SIZEOF_EMBXSHM_BUFFER));
    }
    
    /*
     * Do we have to place this event on the port receive queue
     */
    if (!updateOnly)
    {
	EMBX_OS_INTERRUPT_LOCK();

	/*
	 * Someone waiting? Let them have the data
	 */
	if (localPort->blockedReceivers)
	{
	    EMBX_EventState_t *blockedReceiver;
	    EMBX_RECEIVE_EVENT *dest;

	    /*
	     * Pick the first blocked receiver, drop the lock since we no
	     * longer need it (and on OS20 we cannot post the event while
	     * holding it) and put the event info directly into its receive
	     * event structure, then signal it to wake up.
	     */
	    blockedReceiver = EMBX_EventListFront(&localPort->blockedReceivers);
	    EMBX_OS_INTERRUPT_UNLOCK();

	    blockedReceiver->result = EMBX_SUCCESS;
	    dest = blockedReceiver->data; /* matched with RECEV IN DATA */
	    *dest = *event;
	    EMBX_Info(EMBX_INFO_INTERRUPT, 
	              ("   directly waking blocked process (recev = 0x%08x, event = 0x%08x)\n", 
		      (unsigned) dest, (unsigned) &blockedReceiver->event));
	    EMBX_OS_EVENT_POST (&blockedReceiver->event);
	}
#ifdef EMBX_RECEIVE_CALLBACK
	/* Run any callback functions */
	else if (localPort->callback)
	{
	    (void) localPort->callback(localPort->callbackHandle, event);
	    EMBX_OS_INTERRUPT_UNLOCK();
	}
#endif /* EMBX_RECEIVE_CALLBACK */
	else
	{
	    /*
	     * No-one was waiting, so we must allocate an event off of the
	     * port's free list. If we run out - game over!
	     * Put the resulting event on the pending list.
	     */
	    EMBXSHM_RecEventList_t *receiveEvent = localPort->freeRecEvents;

	    if (NULL == receiveEvent)
	    {
		EMBX_OS_INTERRUPT_UNLOCK();

		EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<EMBXSHM_newMessageNotification = EMBX_NOMEM\n"));
		return EMBX_NOMEM;
	    }

	    EMBX_Info(EMBX_INFO_INTERRUPT, ("   queuing message on port 0x%08x\n", (unsigned) localPort));

	    localPort->freeRecEvents = receiveEvent->next;

	    receiveEvent->recev = *event;
	    receiveEvent->next  = NULL;

	    if (NULL == localPort->receivedHead)
	    {
		localPort->receivedHead = receiveEvent;
	    }
	    else
	    {
		localPort->receivedTail->next = receiveEvent;
	    }

	    localPort->receivedTail = receiveEvent;

	    EMBX_OS_INTERRUPT_UNLOCK();
	}
    }

    EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<EMBXSHM_newMessageNotification = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Local port handle structures
 */
static EMBX_LocalPortMethods_t localPortMethods =
{
    invalidatePort,
    closePort,
    receive,
    receiveBlock,
    receiveInterrupt
};

EMBXSHM_LocalPortHandle_t  EMBXSHM_localportHandleTemplate =
{
    { &localPortMethods, EMBX_HANDLE_INVALID }
};

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
