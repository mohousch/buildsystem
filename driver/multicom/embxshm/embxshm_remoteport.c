/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_remoteport.c                                 */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport                            */
/*         remote port implementation                         */
/*                                                            */
/**************************************************************/

#include "embxshm.h"
#include "embxshmP.h"

/* 
 * Routine to close a port
 */
void EMBXSHM_portClose(EMBXSHM_Transport_t *tpshm)
{
        EMBX_UINT i;

        /*
         * Scan for a notification from each CPU, and process 
         */
        for (i=0; i<EMBXSHM_MAX_CPUS; i++)
        {
            EMBXSHM_LocalPortShared_t *localPortShared;

            if ((localPortShared = tpshm->closedPorts[i]) != 0)
            {
                EMBXSHM_RemotePortLink_t *prev = 0;
                EMBXSHM_RemotePortLink_t *this;

		EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));

                /*
                 * Take the port close/invalidate locks to ensure serialisation
                 */
                EMBXSHM_takeSpinlock (tpshm, &tpshm->portConnectMutex, tpshm->portConnectSpinlock);

		EMBXSHM_READS(localPortShared);

                /*
                 * Scan the list of remote ports, looking for ones we own, and invalidate them
                 * To invalidate them we unlink them from the local port their connected to,
                 * and mark them as invalid.
                 */
                this = localPortShared->remoteConnections;

                while (this)
                {
                    this = (EMBXSHM_RemotePortLink_t*)BUS_TO_LOCAL (this, tpshm->pointerWarp);
		    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(this));

		    EMBXSHM_READS(this);

                    /*
                     * Do we own this remote port?
                     */
                    if (this->owningCPU == tpshm->cpuID)
                    {
                        EMBXSHM_RemotePortHandle_t *remotePortHandle = this->remotePort;

			EMBX_Info(EMBX_INFO_REMOTEPORT, ("   invalidating remote port 0%08x\n", 
			                                (unsigned) remotePortHandle));

                        EMBX_Assert (remotePortHandle);
                        EMBX_Assert (remotePortHandle->linkageShared == this);

                        /*
                         * Take the accessMutex, since we're about to change
                         * its state (serialise with msg/obj send).
                         * All we change is the state - all other fields left
                         * intact.
                         */
                        EMBX_OS_MUTEX_TAKE (&remotePortHandle->accessMutex);

                        remotePortHandle->port.state = EMBX_HANDLE_INVALID;

                        /*
                         * Remove remote port from list on the local port
                         */
                        if (prev)
                        {
			    EMBXSHM_RemotePortLink_t *p = BUS_TO_LOCAL(prev, tpshm->pointerWarp);
			    EMBXSHM_READS(&p->nextConnection);
			    p->nextConnection = this->nextConnection;
			    EMBXSHM_WROTE(&p->nextConnection);
                        }
                        else
                        {
                            localPortShared->remoteConnections = this->nextConnection;
			    EMBXSHM_WROTE(&localPortShared->remoteConnections);
                        }

                        if (this->nextConnection)
                        {
                            EMBXSHM_RemotePortLink_t *next = BUS_TO_LOCAL (this->nextConnection, 
									   tpshm->pointerWarp);
			    EMBXSHM_READS(&next->prevConnection);
                            next->prevConnection = this->prevConnection;
			    EMBXSHM_WROTE(&next->prevConnection);
                        }

                        prev = this->prevConnection;

                        remotePortHandle->linkageShared->prevConnection = 0;
                        remotePortHandle->linkageShared->nextConnection = 0;
			EMBXSHM_WROTE(remotePortHandle->linkageShared);

                        EMBX_OS_MUTEX_RELEASE (&remotePortHandle->accessMutex);
                    }
                    else
                    {
                        prev = (EMBXSHM_RemotePortLink_t*)LOCAL_TO_BUS (this, tpshm->pointerWarp);
                    }

                    this = this->nextConnection;
                }

                /*
                 * Flag the fact that I've invalidated all my remote ports
                 * on this local port
                 */
                localPortShared->invalidated[tpshm->cpuID].marker = 1;
                tpshm->closedPorts[i]                      = 0;

		EMBXSHM_WROTE(&(localPortShared->invalidated[tpshm->cpuID].marker));

                /*
                 * Done with port close invalidate stuff, so release locks
                 */
                EMBXSHM_releaseSpinlock (tpshm, &tpshm->portConnectMutex, tpshm->portConnectSpinlock);
            }
        }
}

/*
 * Notification routine to inform us that a local port has been
 * closed. We need to invalidate all our remote port
 */
void EMBXSHM_portClosedNotification (EMBXSHM_Transport_t *tpshm)
{
    EMBX_Info(EMBX_INFO_REMOTEPORT, (">>>EMBXSHM_portClosedNotification\n"));

    EMBX_Assert (tpshm);

    /*
     * Wait for notification of a local port closing,
     * then look for it and service it
     */
    while (1)
    {
	EMBX_ERROR signalPending;

	EMBX_Info(EMBX_INFO_REMOTEPORT, ("   waiting for port closed notification\n"));
        signalPending = EMBX_OS_EVENT_WAIT (&tpshm->closedPortEvent);
	EMBX_Info(EMBX_INFO_REMOTEPORT, ("   received port closed notification\n"));

        /*
         * If we've been told to die, then do so
         */
        if (signalPending || tpshm->closedPortDieFlag)
        {
	    EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<EMBXSHM_portClosedNotification\n"));
            return;
        }

        /* Close the port */
        EMBXSHM_portClose(tpshm);
    }
}

/*
 * Remote port close method.
 */
static EMBX_ERROR closePort (EMBX_RemotePortHandle_t *ph)
{
    EMBXSHM_RemotePortHandle_t *remotePortHandle = (EMBXSHM_RemotePortHandle_t *)ph;
    EMBXSHM_Transport_t        *tpshm;

    EMBX_Info(EMBX_INFO_REMOTEPORT, (">>>closePort(0x%08x)\n", (unsigned) ph));

    EMBX_Assert (remotePortHandle);

    tpshm = (EMBXSHM_Transport_t *)remotePortHandle->port.transportHandle->transport;
    EMBX_Assert (tpshm);

    /*
     * Take the port close/invalidate locks to ensure serialisation
     */
    EMBXSHM_takeSpinlock (tpshm, &tpshm->portConnectMutex, tpshm->portConnectSpinlock);

    EMBXSHM_READS(remotePortHandle->linkageShared);

    /*
     * No need to take the accessMutex here - we're protected by the shell level
     * transport lock (to serialise with sends), and we're serialised with the
     * port close notification by the portConnect* locks
     */
    if (remotePortHandle->port.state != EMBX_HANDLE_INVALID)
    {
        EMBXSHM_LocalPortShared_t  *localPortShared;
        EMBXSHM_RemotePortLink_t   *prev;

        /*
         * Work out how to access the shared part of the local port
         * structure
         */
        if (remotePortHandle->destinationCPU == tpshm->cpuID)
        {
            localPortShared = remotePortHandle->destination.localPort->localPortShared;
        }
        else
        {
            localPortShared = remotePortHandle->destination.sharedPort;
        }

	EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));
	EMBXSHM_READS(localPortShared);

        prev = remotePortHandle->linkageShared->prevConnection;
	EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(prev));

        /*
         * This port is still connected to a valid destination
         * so remove the port from the destination's remote connection
         * list
         */
        if (prev)
        {
	    EMBXSHM_RemotePortLink_t *p = BUS_TO_LOCAL(prev, tpshm->pointerWarp);

	    EMBXSHM_READS(p);
	    p->nextConnection = remotePortHandle->linkageShared->nextConnection;
	    EMBXSHM_WROTE(p);
        }
        else
        {
            localPortShared->remoteConnections = remotePortHandle->linkageShared->nextConnection;
	    EMBXSHM_WROTE(localPortShared);
        }

        if (remotePortHandle->linkageShared->nextConnection)
        {
            EMBXSHM_RemotePortLink_t *next;

            next = (EMBXSHM_RemotePortLink_t*)BUS_TO_LOCAL (remotePortHandle->linkageShared->nextConnection, tpshm->pointerWarp);
	    EMBXSHM_READS(next);
	    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));
            next->prevConnection = remotePortHandle->linkageShared->prevConnection;

	    EMBXSHM_WROTE(next);
        }
    }

    remotePortHandle->port.state           = EMBX_HANDLE_CLOSED;
    remotePortHandle->port.transportHandle = 0;

    /*
     * Done with port close invalidate stuff, so release locks
     */
    EMBXSHM_releaseSpinlock (tpshm, &tpshm->portConnectMutex, tpshm->portConnectSpinlock);

    EMBXSHM_free (tpshm, remotePortHandle->linkageShared);
    EMBX_OS_MUTEX_DESTROY (&remotePortHandle->accessMutex);
    EMBX_OS_MemFree (remotePortHandle);

    EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<closePort = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Remote port method to send a message
 */
static EMBX_ERROR sendMessage (EMBX_RemotePortHandle_t *ph,
                               EMBX_Buffer_t           *buf,
                               EMBX_UINT                size)
{
    EMBXSHM_RemotePortHandle_t *remotePortHandle = (EMBXSHM_RemotePortHandle_t *)ph;
    EMBXSHM_Transport_t        *tpshm;
    EMBX_ERROR                  err = EMBX_SUCCESS;

    EMBX_Info(EMBX_INFO_REMOTEPORT, (">>>sendMessage(0x%08x, 0x%08x, %d)\n", (unsigned) ph, (unsigned) buf, size));

    EMBX_Assert (ph);
    EMBX_Assert (buf);

    tpshm = (EMBXSHM_Transport_t *)remotePortHandle->port.transportHandle->transport;

    /*
     * Check to see if this is a local or remote send
     */
    if (remotePortHandle->destinationCPU == tpshm->cpuID)
    {
        EMBXSHM_LocalPortShared_t *localPortShared = remotePortHandle->destination.localPort->localPortShared;
	EMBX_RECEIVE_EVENT event;

        EMBX_Assert (localPortShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));

	event.type   = EMBX_REC_MESSAGE;
	event.handle = EMBX_INVALID_HANDLE_VALUE;
	event.data   = (EMBX_VOID *) (&buf->data);
	event.offset = 0;
	event.size   = size;

	/* receiveNotification is part of the interrupt handler system and
	 * must be called with interrupts locked
	 */
	EMBX_OS_MUTEX_TAKE(&tpshm->secondHalfMutex);
	tpshm->disable_int(tpshm);
	err = EMBXSHM_receiveNotification(localPortShared, &event, EMBX_FALSE, EMBX_FALSE);
	tpshm->enable_int(tpshm);
	EMBX_OS_MUTEX_RELEASE(&tpshm->secondHalfMutex);
    }
    else
    {
        /*
         * Send message to a port on another CPU. We encode
         * details of the message in the pipe entry, and
         * let the other guy sort out queuing it up.
         */
        EMBXSHM_PipeElement_t element;

	EMBXSHM_WROTE(buf);
	EMBXSHM_WROTE_ARRAY((char *) (&buf->data), size);

        /*
         * Under the protection of the accessMutex, check the port state
         * is still OK. If it is, then the local port we're pointing to
         * is still OK too. By the time the message gets to the other
         * side (could be another CPU), then the local port could could
         * have been closed out - he will drop it on the floor.
         */
        EMBX_OS_MUTEX_TAKE (&remotePortHandle->accessMutex);

        if (remotePortHandle->port.state != EMBX_HANDLE_VALID)
        {
            EMBX_OS_MUTEX_RELEASE (&remotePortHandle->accessMutex);
	    EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<sendMessage = EMBX_INVALID_STATUS\n"));
            return EMBX_INVALID_STATUS;
        }

        EMBX_Assert (remotePortHandle->destination.sharedPort->owningCPU == remotePortHandle->destinationCPU);

        element.control            = EMBXSHM_PIPE_CTRL_MSG_SEND;
        element.data.msg_send.port = (EMBXSHM_LocalPortShared_t*)
                                     LOCAL_TO_BUS (remotePortHandle->destination.sharedPort, tpshm->pointerWarp);
        element.data.msg_send.data = LOCAL_TO_BUS (&buf->data, tpshm->pointerWarp);
        element.data.msg_send.size = size;

        err = EMBXSHM_enqueuePipeElement (tpshm, remotePortHandle->destinationCPU, &element);

        EMBX_OS_MUTEX_RELEASE (&remotePortHandle->accessMutex);
    }

    EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<sendMessage = %d\n", err));
    return err;
}

/*
 * Remote port method to send an object
 */
static EMBX_ERROR sendObject (EMBX_RemotePortHandle_t *ph,
                              EMBX_HANDLE              handle,
                              EMBX_UINT                offset,
                              EMBX_UINT                size)
{
    EMBXSHM_RemotePortHandle_t *remotePortHandle = (EMBXSHM_RemotePortHandle_t *)ph;
    EMBXSHM_Transport_t        *tpshm;
    EMBXSHM_Object_t           *obj;
    EMBX_ERROR                  err = EMBX_SUCCESS;

    EMBX_Info(EMBX_INFO_REMOTEPORT, (">>>sendObject(0x%08x, ...)\n", (unsigned) ph));

    EMBX_Assert (ph);
    EMBX_Assert (ph->transportHandle);

    tpshm = (EMBXSHM_Transport_t *)ph->transportHandle->transport;

    EMBX_Assert (tpshm);

    /*
     * Do some up front sanity checks
     */
    obj = EMBXSHM_findObject (tpshm, handle);

    if ((obj == 0) || ((offset+size) > obj->size))
    {
	EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<sendObject = EMBX_INVALID_ARGUMENT\n"));
        return EMBX_INVALID_ARGUMENT;
    }

    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(obj));

    /*
     * See if this is a local or remote send
     */
    if (remotePortHandle->destinationCPU == tpshm->cpuID)
    {
        /*
         * Its a local send
         */
        EMBXSHM_LocalPortShared_t *localPortShared = remotePortHandle->destination.localPort->localPortShared;
	EMBX_RECEIVE_EVENT event;

        EMBX_Assert (localPortShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(localPortShared));

	event.type   = EMBX_REC_OBJECT;
	event.handle = handle;
	if (obj->owningCPU == tpshm->cpuID)
	{
		event.data = obj->localData;
	}
	else
	{
		event.data = BUS_TO_LOCAL (obj->sharedData, tpshm->pointerWarp);
	}
	event.offset = offset;
	event.size   = size;

	/* receiveNotification is part of the interrupt handler system and
	 * must be called with interrupts locked
	 */
	EMBX_OS_MUTEX_TAKE(&tpshm->secondHalfMutex);
	tpshm->disable_int(tpshm);
	err = EMBXSHM_receiveNotification(localPortShared, &event, EMBX_FALSE, EMBX_FALSE);
	tpshm->enable_int(tpshm);
	EMBX_OS_MUTEX_RELEASE(&tpshm->secondHalfMutex);
    }
    else
    {
        /*
         * This is an object send to a port owned by another CPU.
         * We encode details of the object in the pipe entry, and
         * let the other guy sort out queuing it up.
         */
        EMBXSHM_PipeElement_t element;
	EMBX_VOID *dst = BUS_TO_LOCAL (obj->sharedData, tpshm->pointerWarp);

        /*
         * If this object is shadowed, and I own it
         * then I need to update the copy in shared memory.
         */
        if (obj->shadow && (obj->owningCPU == tpshm->cpuID))
        {

            memcpy ((void*)((size_t)dst+offset),
                    (void*)((size_t)obj->localData+offset),
                    size);
        } 

	/* MULTICOM_32BIT_SUPPORT: Must flush the cache as necessary */
	if (obj->mode == EMBX_INCOHERENT_MEMORY)
	  EMBX_OS_FlushCache((char *)dst+offset, size);

        /*
         * Under the protection of the accessMutex, check the port state
         * is still OK. If it is, then the local port we're pointing to
         * is still OK too. By the time the message gets to the other
         * side (could be another CPU), then the local port could could
         * have been closed out - he will drop it on the floor.
         */
        EMBX_OS_MUTEX_TAKE (&remotePortHandle->accessMutex);

        if (remotePortHandle->port.state != EMBX_HANDLE_VALID)
        {
            EMBX_OS_MUTEX_RELEASE (&remotePortHandle->accessMutex);
            return EMBX_INVALID_STATUS;
        }

        EMBX_Assert (remotePortHandle->destination.sharedPort->owningCPU == remotePortHandle->destinationCPU);

        element.control              = EMBXSHM_PIPE_CTRL_OBJ_SEND;
        element.data.msg_send.port   = (EMBXSHM_LocalPortShared_t*)
                                           LOCAL_TO_BUS (remotePortHandle->destination.sharedPort, tpshm->pointerWarp);
        element.data.obj_send.handle = handle;
        element.data.obj_send.data   = obj->sharedData;
        element.data.obj_send.offset = offset;
        element.data.obj_send.size   = size;

        err = EMBXSHM_enqueuePipeElement (tpshm, remotePortHandle->destinationCPU, &element);

        EMBX_OS_MUTEX_RELEASE (&remotePortHandle->accessMutex);
    }

    EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<sendObject = %d\n", err));
    return err;
}

/*
 * Remote port method to perform an object update
 */
static EMBX_ERROR updateObject (EMBX_RemotePortHandle_t *ph,
                                EMBX_HANDLE              handle,
                                EMBX_UINT                offset,
                                EMBX_UINT                size)
{
    EMBXSHM_RemotePortHandle_t *remotePortHandle = (EMBXSHM_RemotePortHandle_t *)ph;
    EMBXSHM_Transport_t        *tpshm;
    EMBXSHM_Object_t           *obj;
    EMBX_ERROR                  err = EMBX_SUCCESS;

    EMBX_Info(EMBX_INFO_REMOTEPORT, (">>>updateObject(0x%08x, ...)\n", (unsigned) ph));

    EMBX_Assert (ph);
    EMBX_Assert (ph->transportHandle);

    tpshm = (EMBXSHM_Transport_t *)ph->transportHandle->transport;

    EMBX_Assert (tpshm);

    /*
     * Do some up front sanity checks
     */
    obj = EMBXSHM_findObject (tpshm, handle);

    if ((obj == 0) || ((offset+size) > obj->size))
    {
	EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<updateObject = EMBX_INVALID_ARGUMENT\n"));
        return EMBX_INVALID_ARGUMENT;
    }

    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(obj));

    /*
     * See if this is a local or remote send/update
     */
    if (remotePortHandle->destinationCPU == tpshm->cpuID)
    {
        /*
         * A local update, so we're all done
         */
	EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<updateObject = EMBX_SUCCESS\n"));
        return EMBX_SUCCESS;
    }
    else
    {
        /*
         * This is an object update to a port owned by another CPU.
         * We may need to copy shadowed data around, and possibly
         * send an update message to the other guy.
         */
        EMBXSHM_PipeElement_t element;

	EMBX_VOID *dst = BUS_TO_LOCAL (obj->sharedData, tpshm->pointerWarp);

	/*
	 * Update the shared copy of this object is it is shadowed from a master
	 * copy on this CPU.
	 */
	if (obj->owningCPU == tpshm->cpuID && obj->shadow)
	{
	    memcpy ((void*)((size_t)dst+offset), (void*)((size_t)obj->localData+offset), size);
	}

	/*
	 * Ensure the object data is not held in our own cache.
	 */
	if (obj->mode == EMBX_INCOHERENT_MEMORY)
	  EMBX_OS_FlushCache((char *)dst+offset, size);

	/*
	 * Under the protection of the accessMutex, check the port state
	 * is still OK. If it is, then the local port we're pointing to
	 * is still OK too. By the time the message gets to the other
	 * side (could be another CPU), then the local port could could
	 * have been closed out - he will drop it on the floor.
	 */
	EMBX_OS_MUTEX_TAKE (&remotePortHandle->accessMutex);

	if (remotePortHandle->port.state != EMBX_HANDLE_VALID)
	{
	    EMBX_OS_MUTEX_RELEASE (&remotePortHandle->accessMutex);
	    return EMBX_INVALID_STATUS;
	}

	EMBX_Assert (remotePortHandle->destination.sharedPort->owningCPU == 
	             remotePortHandle->destinationCPU);

	/*
	 * Now inform the partner CPU that the object has been updated.
	 */
	element.control              = EMBXSHM_PIPE_CTRL_OBJ_UPDATE;
	element.data.msg_send.port   = (EMBXSHM_LocalPortShared_t*) LOCAL_TO_BUS (
		remotePortHandle->destination.sharedPort, tpshm->pointerWarp);
	element.data.obj_send.handle = handle;
	element.data.obj_send.data   = obj->sharedData;
	element.data.obj_send.offset = offset;
	element.data.obj_send.size   = size;

	err = EMBXSHM_enqueuePipeElement (tpshm, remotePortHandle->destinationCPU, &element);

	EMBX_OS_MUTEX_RELEASE (&remotePortHandle->accessMutex);
    }

    EMBX_Info(EMBX_INFO_REMOTEPORT, ("<<<sendObject = %d\n", err));
    return err;
}

/*
 * Remote port handle structures
 */
static EMBX_RemotePortMethods_t remotePortMethods =
{
    closePort,
    sendMessage,
    sendObject,
    updateObject
};
 
EMBXSHM_RemotePortHandle_t EMBXSHM_remoteportHandleTemplate =
{
    { &remotePortMethods, EMBX_HANDLE_INVALID }
};

