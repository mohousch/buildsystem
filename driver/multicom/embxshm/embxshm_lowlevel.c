/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_lowlevel.c                                   */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport,                           */
/*         low level support routines                         */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"

static void dequeuePipeElement (EMBXSHM_Transport_t *tpshm, EMBX_BOOL interruptCtx);

/*
 * Routine to enqueue a pipe element to a CPU
 */
EMBX_ERROR EMBXSHM_enqueuePipeElement (EMBXSHM_Transport_t   *tpshm,
                                      EMBX_UINT              destCPU,
                                      EMBXSHM_PipeElement_t *element)
{
    EMBXSHM_Pipe_t     *pipe;
    EMBXSHM_Spinlock_t *lock;
    EMBX_UINT           index;

    EMBX_Info(EMBX_INFO_SHM, (">>>EMBXSHM_enqueuePipeElement(..., %d, ...)\n", destCPU));

    EMBX_Assert (destCPU < EMBXSHM_MAX_CPUS);
    EMBX_Assert (element);
    EMBX_Assert (EMBXSHM_PIPE_CTRL_FREE < element->control && element->control <= EMBXSHM_PIPE_CTRL_TRANSPORT_INVALIDATE);

    pipe = &tpshm->tcb->pipes[destCPU];
    EMBX_Assert(pipe && EMBXSHM_ALIGNED_TO_CACHE_LINE(pipe));
    lock = tpshm->pipeLocks[destCPU];

    EMBXSHM_takeSpinlock (tpshm, &tpshm->tpLock, lock);

    EMBXSHM_READS(&pipe->index);

    /* force a local copy for working with */
    index = pipe->index;
    EMBX_Info(EMBX_INFO_SHM, ("   write index = %d\n", index));

    /* this cannot be dirty but can spontaneously change */
    EMBXSHM_USES(&pipe->elements[index]);

    if (pipe->elements[index].control != EMBXSHM_PIPE_CTRL_FREE)
    {
        /*
         * Pipe is full
         */
	EMBXSHM_releaseSpinlock (tpshm, &tpshm->tpLock, lock);
	EMBX_Info(EMBX_INFO_SHM, ("<<<EMBXSHM_enqueuePipeElement = EMBX_NOMEM\n"));
        return EMBX_NOMEM;
    }

    /*
     * Copy element into pipe - data 1st, then control,
     * so we ensure that data is valid when control is written
     */
    pipe->elements[index].data      = element->data;
    pipe->elements[index].control   = element->control;
    EMBXSHM_WROTE(&pipe->elements[index]);

    /*
     * Move onto the next element
     */
    pipe->index = (index < (EMBXSHM_PIPE_ELEMENTS - 1) ? index + 1 : 0);
    EMBXSHM_WROTE(&pipe->index);

    EMBXSHM_releaseSpinlock (tpshm, &tpshm->tpLock, lock);

    tpshm->raise_int (tpshm, destCPU);

    EMBX_Info(EMBX_INFO_SHM, ("<<<EMBXSHM_enqueuePipeElement = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * Transport interrupt handler.
 */
void EMBXSHM_interruptHandler(EMBXSHM_Transport_t *tpshm)
{
    if (tpshm->secondHalfRunFlag) 
    {
	/*
	 * Suppress this interrupt until the second half handler has run.
	 */
	tpshm->disable_int(tpshm);
    }

    /*
     * Fetch all available elements from the pipe.
     */
    dequeuePipeElement(tpshm, EMBX_TRUE);
}

/*
 * Called when a pipe entry is added to our input queue.
 */
static void dequeuePipeElement (EMBXSHM_Transport_t *tpshm, EMBX_BOOL interruptCtx)
{
    EMBX_Info(EMBX_INFO_INTERRUPT, (">>>EMBXSHM_interruptHandler\n"));

    EMBX_Assert (tpshm);

    /*
     * Read pipe to exhaustion
     */
    while (1)
    {
	EMBXSHM_PipeElement_t *pElement;
	EMBXSHM_PipeCtrl_t control;
	EMBXSHM_LocalPortShared_t *port;
	EMBX_RECEIVE_EVENT event;
	int cpuID;

	EMBX_UINT pipeIndex = tpshm->inPipeIndex;

	/* 
	 * Clear the interrupt before we examine the pipe 
	 */
	tpshm->clear_int(tpshm);

	/*
	 * It is possible for the control word to be re-examined without
	 * other activity if we receive back to back interrupts
	 */
	tpshm->buffer_flush (tpshm, (void *) &(pElement));

	/*
         * Examine the pipe and dispatch any events found in the pipe.
	 * We will leave the interrupt state if there is nothing in
	 * the pipe or if the first element cannot be dequeued.
	 */

	pElement = &tpshm->tcb->pipes[tpshm->cpuID].elements[pipeIndex];
	EMBX_Assert(pElement && EMBXSHM_ALIGNED_TO_CACHE_LINE(pElement));
	EMBXSHM_READS(pElement);

	control = pElement->control;
	EMBX_Info(EMBX_INFO_INTERRUPT, ("   read index = %d (0x%08x); control = %d\n", tpshm->inPipeIndex, (unsigned) pElement, control));
	switch (control)
	{
	case EMBXSHM_PIPE_CTRL_FREE:
	    if (EMBX_SUCCESS != tpshm->inPipeStalled) {
		/*
		 * If the pipe overflowed it is possible that a new port 
		 * was created but we were not notified because the pipe
		 * had stalled.
		 */
		EMBX_OS_EVENT_POST (&tpshm->secondHalfEvent);

		tpshm->inPipeStalled = EMBX_SUCCESS;
	    }
	    EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<EMBXSHM_interruptHandler (no messages)\n"));
	    return;

	case EMBXSHM_PIPE_CTRL_MSG_SEND:
	    /* intialize the constant components */
	    event.type = EMBX_REC_MESSAGE;
	    event.offset = 0;
	    event.handle = EMBX_INVALID_HANDLE_VALUE;

	    /* populate the rest of the structure from shared memory */
	    port = BUS_TO_LOCAL(pElement->data.msg_send.port, tpshm->pointerWarp);
	    event.data = BUS_TO_LOCAL(pElement->data.msg_send.data, tpshm->pointerWarp);
	    event.size = pElement->data.msg_send.size;

	    /* dispatch the event */
	    EMBX_Assert(port->owningCPU == tpshm->cpuID);
	    if (EMBX_SUCCESS != EMBXSHM_receiveNotification(port, &event, EMBX_FALSE, interruptCtx))
	    {
		EMBX_Info(EMBX_INFO_INTERRUPT, 
		          ("<<<EMBXSHM_interruptHandler (cannot receive message on port 0x%08x)\n",
			  (unsigned) port));
		return;
	    }
	    break;

	case EMBXSHM_PIPE_CTRL_OBJ_SEND:
	case EMBXSHM_PIPE_CTRL_OBJ_UPDATE:
	    /* initialize constant components */
	    event.type   = EMBX_REC_OBJECT;

	    /* populate the rest of the structure from shared memory */
	    port         = BUS_TO_LOCAL(pElement->data.obj_send.port, tpshm->pointerWarp);
	    event.handle = pElement->data.obj_send.handle;
	    event.data   = BUS_TO_LOCAL(pElement->data.obj_send.data, tpshm->pointerWarp);
	    event.offset = pElement->data.obj_send.offset;
	    event.size   = pElement->data.obj_send.size;

	    /* dispatch the event */
	    EMBX_Assert(port->owningCPU == tpshm->cpuID);
	    if (EMBX_SUCCESS != 
	        EMBXSHM_receiveNotification(port, &event, control == EMBXSHM_PIPE_CTRL_OBJ_UPDATE,
					    interruptCtx))
	    {
		EMBX_Info(EMBX_INFO_INTERRUPT, 
		          ("<<<EMBXSHM_interruptHandler (cannot receive object on port 0x%08x)\n",
			  (unsigned) port));

		if (interruptCtx) {
			/* cause the second half handler to run. when run as a second half
			 * we are permitted to call memcpy to update shadowed objects.
			 */
			tpshm->disable_int(tpshm);
			tpshm->secondHalfRunFlag = 1;
			EMBX_OS_EVENT_POST (&tpshm->secondHalfEvent);
		}

		return;
	    }
	    break;
	    
        case EMBXSHM_PIPE_CTRL_PORT_CREATE:
            EMBX_OS_EVENT_POST (&tpshm->secondHalfEvent);
            break;
	
	case EMBXSHM_PIPE_CTRL_PORT_CLOSED:
	    port         = BUS_TO_LOCAL(pElement->data.obj_send.port, tpshm->pointerWarp);
	    cpuID        = pElement->data.port_closed.cpuID;

	    /* check that the handshake protocol has not be violated */
            EMBX_Assert (tpshm->closedPorts[cpuID] == 0);

	    /* communicate with our helper task */
            tpshm->closedPorts[cpuID] = port;
            EMBX_OS_EVENT_POST (&tpshm->closedPortEvent);
	    break;

	case EMBXSHM_PIPE_CTRL_TRANSPORT_INVALIDATE:
	    tpshm->remoteInvalidateFlag = 1;
	    tpshm->secondHalfDieFlag = 1;
	    EMBX_OS_EVENT_POST (&tpshm->secondHalfEvent);
	    break;

        default:
	    EMBX_Info(EMBX_INFO_INTERRUPT, ("Bad pipe entry! ctrl = 0x%x\n", control));
            EMBX_Assert (0);
	    break;
        }

	/*
	 * Consume the element from the  pipe.
	 */
	pElement->control = EMBXSHM_PIPE_CTRL_FREE;
	EMBXSHM_WROTE(&pElement->control);
	tpshm->inPipeIndex = (pipeIndex < (EMBXSHM_PIPE_ELEMENTS - 1) ? pipeIndex + 1 : 0);

	/*
	 * Check the preceding element for overflow (ordering respective to consumtion
	 * is important here or we might fail to detect the overflow).
	 */
	pipeIndex = (0 == pipeIndex ? EMBXSHM_PIPE_ELEMENTS : pipeIndex) - 1;
	pElement = &tpshm->tcb->pipes[tpshm->cpuID].elements[pipeIndex];
	EMBXSHM_READS(&pElement->control);
	if (EMBXSHM_PIPE_CTRL_FREE != pElement->control) 
	{
		tpshm->inPipeStalled = EMBX_SYSTEM_ERROR;
	}
    }

    EMBX_Assert(0);
}

/*
 * The second half handler is used to call dequeuePipeElement from 
 * task context where we are allowed to perform lengthy operations
 * such as memcpy.
 */
void EMBXSHM_secondHalfHandler(EMBXSHM_Transport_t *tpshm)
{

    while (1) {
	EMBX_ERROR signalPending = EMBX_OS_EVENT_WAIT(&tpshm->secondHalfEvent);

	if (signalPending || tpshm->secondHalfDieFlag) {
	    return;
	}

	if (tpshm->secondHalfRunFlag) {
	    EMBX_OS_MUTEX_TAKE(&tpshm->secondHalfMutex);
	    tpshm->disable_int(tpshm);

	    dequeuePipeElement(tpshm, EMBX_FALSE);

	    tpshm->secondHalfRunFlag = 0;
	    tpshm->enable_int(tpshm);
	    EMBX_OS_MUTEX_RELEASE(&tpshm->secondHalfMutex);
	} else {
	    EMBXSHM_newPortNotification(tpshm);
	}
    }

    EMBX_Assert(0);
}
