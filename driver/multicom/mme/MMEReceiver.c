/*
 * MMEReceiver.c
 *
 * Copyright (C) STMicroelectronics Limited 2003, 2004. All rights reserved.
 *
 * A receiver object contains a dispatch queue and other supporting items.
 */

#include "mme_companionP.h"
#include "debug_ctrl.h"

/*******************************************************************/

/* this is additional internal command state used to indicate that the command
 * is subject to an internal error and no buffer updates are required (and may
 * even be harmful).
 */
#define MME_COMMAND_DIED_EVT 0xdead

/*******************************************************************/

static MME_ERROR convertTransformMessage(MMEReceiver *receiver, EMBX_VOID *buffer)
{
	union {
		TransformerTransformMessage *message;
		MME_DataBuffer_t *buffer;
		MME_ScatterPage_t *page;
		EMBX_HANDLE *hdl;
		char *ch;
		void *p;
		void **pp;
	} p;

	EMBX_ERROR err;
	MME_Command_t *cmd;
	MME_DataBuffer_t *buf_p, **buf_pp;
	int i, j, nBufs;

	MME_Assert(receiver);
	MME_Assert(buffer);

	/* a few up front validation checks */
	p.p = buffer;
	MME_Assert(p.message->receiverInstance == (MME_UINT) receiver);
	MME_Assert(p.message->messageSize >= sizeof(TransformerTransformMessage));

	/* extract the command pointer */
	cmd = &p.message->command;
	MME_Assert(MME_TRANSFORM == cmd->CmdCode ||
	           MME_SEND_BUFFERS == cmd->CmdCode ||
		   MME_SET_GLOBAL_TRANSFORM_PARAMS == cmd->CmdCode);
	p.message += 1;

	nBufs = cmd->NumberInputBuffers + cmd->NumberOutputBuffers;

	/* extract the data buffer pointers */
	buf_p = p.buffer;
	p.buffer += nBufs;
	buf_pp = p.p;
	cmd->DataBuffers_p = buf_pp;
	p.pp += nBufs;

	for (i=0; i < nBufs; i++) {
		int nPages = buf_p[i].NumberOfScatterPages;
		MME_ScatterPage_t *pages = p.page;
		
		/* update the pointers to and in the data buffers */
		buf_pp[i] = &buf_p[i];
		buf_p[i].ScatterPages_p = pages;

		/* shift p to the list of object handles */
		p.page += nPages;

		/* get the address for each scatter page */
		for (j=0; j < nPages; j++) {
			EMBX_HANDLE hdl = *p.hdl++;

			/* Don't perform any translations on buffers flagged as being PHYSICAL addresses */
			if (((pages[j].FlagsIn|pages[j].FlagsOut) & MME_DATA_PHYSICAL)) {
				pages[j].Page_p = (MME_ScatterPage_t *)hdl;
				continue;
			}

			if ((0xf0000000 & hdl) == 0xf0000000) {
				EMBX_UINT size;

                                pages[j].Page_p = NULL;
				err = EMBX_GetObject(receiver->transportHandle, hdl,
						     &pages[j].Page_p, &size);
				MME_Assert(EMBX_SUCCESS == err);
				MME_Assert(pages[j].Size == size);
			} else {
				MME_Info(MME_INFO_RECEIVER, 
					 ("convertTransformMessage buffer %d, page %d - 0x%08x, handle 0x%08x\n",
					  i, j, pages[j].Page_p, hdl));
				
				err = EMBX_Address(receiver->transportHandle, hdl, &pages[j].Page_p);
				if (EMBX_INCOHERENT_MEMORY == err) {
					MME_Info(MME_INFO_RECEIVER, 
					("convertTransformMessage - incoherent memory\n"));
					if (0 == (pages[j].FlagsOut & MME_REMOTE_CACHE_COHERENT)) {
						EMBX_OS_PurgeCache(pages[j].Page_p, pages[j].Size);
						MME_Info(MME_INFO_RECEIVER, 
						("convertTransformMessage - purging cache\n"));
					}
				} else {
					MME_Assert(EMBX_SUCCESS == err);
				}
			}
		}
	}

	/* finally extract the parametric information (if there is any) */
	if (cmd->CmdStatus.AdditionalInfoSize > 0) {
		cmd->CmdStatus.AdditionalInfo_p = p.p;
		p.ch += cmd->CmdStatus.AdditionalInfoSize;
	}

	if (cmd->ParamSize > 0) {
		cmd->Param_p = p.p;
	}

	return MME_SUCCESS;
}

static MME_ERROR reconvertTransformMessage(MMEReceiver *receiver, EMBX_VOID *buffer)
{
	union {
		TransformerTransformMessage *message;
		MME_DataBuffer_t *buffer;
		MME_ScatterPage_t *page;
		EMBX_HANDLE *hdl;
		void **pp;
	} p;

	EMBX_ERROR err;
	MME_Command_t *cmd;
	MME_DataBuffer_t *buf_p;
	int i, j;

	MME_Assert(receiver);
	MME_Assert(buffer);

	/* a few up front validation checks */
	p.message = buffer;
	MME_Assert(p.message->receiverInstance == (MME_UINT) receiver);
	MME_Assert(p.message->messageSize >= sizeof(TransformerTransformMessage));

	/* extract the command pointer */
	cmd = &p.message->command;
	MME_Assert(MME_TRANSFORM == cmd->CmdCode ||
	           MME_SEND_BUFFERS == cmd->CmdCode ||
		   MME_SET_GLOBAL_TRANSFORM_PARAMS == cmd->CmdCode);
	p.message += 1;

	/* skip the data buffers section */
	buf_p = p.buffer;
	p.buffer += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;
	p.pp += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;

	/* now iterator over the scatter pages */
	for (i=0; i < (cmd->NumberInputBuffers + cmd->NumberOutputBuffers); i++) {
		MME_ScatterPage_t *pages = p.page;

		p.page += buf_p[i].NumberOfScatterPages;

		/* every buffer must be updated so that it does not persist in the cache allowing
		 * stale values to be read when the buffer is reused
		 */
		for (j=0; j < buf_p[i].NumberOfScatterPages; j++) {
			EMBX_HANDLE hdl = p.hdl[j];

	                MME_Info(MME_INFO_RECEIVER, 
	                        ("reconvertTransformMessage buffer %d, page %d - 0x%08x, handle 0x%08x\n", i, j, pages[j].Page_p, hdl));

			/* Don't perform any translations on buffers flagged as being PHYSICAL addresses */
			if (((pages[j].FlagsIn|pages[j].FlagsOut) & MME_DATA_PHYSICAL)) {
				pages[j].Page_p = (MME_ScatterPage_t *) hdl;
				continue;
			}

			if ((0xf0000000 & hdl) == 0xf0000000) {
				err = EMBX_UpdateObject(receiver->replyPort, hdl, 0, pages[j].Size);
				MME_Assert(EMBX_SUCCESS == err);
			} else if (EMBX_INCOHERENT_MEMORY == 
			           EMBX_Address(receiver->transportHandle, hdl, &pages[j].Page_p)) {
	                	MME_Info(MME_INFO_RECEIVER, 
	                        ("reconvertTransformMessage - incoherent memory\n"));

				if (0 == (pages[j].FlagsOut & MME_DATA_CACHE_COHERENT) &&
				    0 == (pages[j].FlagsIn & MME_DATA_TRANSIENT)) {
					EMBX_OS_PurgeCache(pages[j].Page_p, pages[j].Size);
	                		MME_Info(MME_INFO_RECEIVER, 
		                        ("reconvertTransformMessage - purging cache\n"));
				}
			}
		}

		p.hdl += buf_p[i].NumberOfScatterPages;
	}

	return MME_SUCCESS;
}

/*******************************************************************/

static void enqueueCommand(MMEReceiver *receiver, TransformerTransformMessage *message)
{
	MME_ERROR res;
	int tableIndex = MME_CMDID_GET_COMMAND(message->command.CmdStatus.CmdId);

#ifndef MME_LEAN_AND_MEAN
	if (tableIndex>=MAX_TRANSFORMER_SEND_OPS) {
		/* Something has gone wrong! */
		MME_Assert(tableIndex<MAX_TRANSFORMER_SEND_OPS);
		return;
	}
#endif
	/* update the message index table */
	receiver->messageTable[tableIndex] = message;

	switch (message->command.CmdCode) {
	case MME_SET_GLOBAL_TRANSFORM_PARAMS:
		/* TODO: altering the due time for parameter setting is not in the spec ... */
		message->command.DueTime = 0;
		/*FALLTHRU*/

	case MME_TRANSFORM: {
		/* enqueue the message for dispatch by the execution loops */
		MME_CommandStatus_t *status  = (MME_CommandStatus_t*) &(message->command.CmdStatus);

		status->State = MME_COMMAND_PENDING;
		res = MME_MessageQueue_Enqueue(&receiver->messageQueue, message, message->command.DueTime);
		if (MME_SUCCESS == res) {
			/* signal to the execution loop that there is some work to be done */
			EMBX_OS_EVENT_POST(receiver->commandEvent);
		}
		break;
	}
	case MME_SEND_BUFFERS: {
		MME_CommandStatus_t *status  = (MME_CommandStatus_t*) &(message->command.CmdStatus);

		status->State = MME_COMMAND_EXECUTING;
		/* command and execute here rather than in the execution loop
		 * (this allows buffers to be delivered to blocked transformers)
		 *
		 * EMBX_RECEIVE_CALLBACK: This will be called in the interrupt context
		 */
		res = convertTransformMessage(receiver, message);
		if (MME_SUCCESS == res) {
			res = (*(receiver->ProcessCommand)) ((void*)receiver->mmeHandle, &message->command);
		}
		break;
	}
	default:
		res = MME_INTERNAL_ERROR;
		break;
	}

	/* report any errors to the host. we mark the command as died in order
	 * to skip the buffer reconvertion which is not needed for errors and
	 * might fail anyway.
	 *
	 * successful completion will either be notified by the execution loop 
	 * directly (for transforms and parameter updates) and by the transformer
	 * code itself for buffer updates.
	 */
	if (MME_SUCCESS != res && MME_TRANSFORM_DEFERRED != res) {
		res = MME_Receiver_NotifyHost(receiver, MME_COMMAND_DIED_EVT, message, res);
		MME_Assert(MME_SUCCESS == res); /* cannot recover */
	}
}

/*******************************************************************/

static void abortCommand(MMEReceiver *receiver, TransformerAbortMessage *abort) 
{
	MME_ERROR res;
	EMBX_ERROR err;
	TransformerTransformMessage *transform;

	/* lookup the command in the messageTable */
	transform = receiver->messageTable[MME_CMDID_GET_COMMAND(abort->commandId)];
	EMBX_OS_MUTEX_TAKE(&receiver->manager->abortMutex);

	/* see if we can yank it off the command queue */
	res = MME_MessageQueue_RemoveByValue(&receiver->messageQueue, transform);

	/* if not our last chance is to get the transformer to do it for us */
	if (MME_SUCCESS != res) {
		res = (*(receiver->Abort)) ((void*)receiver->mmeHandle, abort->commandId);
	}

	if (MME_SUCCESS == res) {
		/* report the successful abortion */
		res = MME_Receiver_NotifyHost(receiver, MME_COMMAND_COMPLETED_EVT, 
				transform, MME_COMMAND_ABORTED);
		MME_Assert(MME_SUCCESS == res); /* cannot recover */
	}

	/* free up the abort message */
	err = EMBX_Free(abort);
	MME_Assert(EMBX_SUCCESS == err); /* cannot recover */

	EMBX_OS_MUTEX_RELEASE(&receiver->manager->abortMutex);
}

/*******************************************************************/

#ifdef EMBX_RECEIVE_CALLBACK
/* the task entry point for the receiver object's message broker task */
static EMBX_ERROR receiverCallback(EMBX_HANDLE mmeReceiver, EMBX_RECEIVE_EVENT *event)
{
	MMEReceiver *receiver = (MMEReceiver *) mmeReceiver;

	MME_Assert(EMBX_REC_MESSAGE == event->type);
	MME_Assert(sizeof(MME_UINT) <= event->size);
	MME_Assert(NULL != event->data);

	/* the message ID decides. */
	switch (*((MME_UINT*) event->data)) {
	case TMESSID_TRANSFORM:
		enqueueCommand(receiver, (TransformerTransformMessage *) event->data);
		break;
	  
	case TMESSID_ABORT: {
		abortCommand(receiver, (TransformerAbortMessage *) event->data);
	}	break;
	  
	default:
		MME_Assert(0); /* not reached */
		break;
	}

	return EMBX_SUCCESS;
}
#endif /* EMBX_RECEIVE_CALLBACK */

/* the task entry point for the receiver object's message broker task */
static void receiverTask(void *mmeReceiver)
{
	EMBX_ERROR err;
	EMBX_RECEIVE_EVENT event;
	MMEReceiver *receiver = (MMEReceiver *) mmeReceiver;

	MME_Info(MME_INFO_RECEIVER, (">>>ReceiverTaskMMEReceiver(0x%08x)\n", (unsigned) mmeReceiver));

	while (1) {
		/* wait for a host request. */
		err = EMBX_ReceiveBlock(receiver->receiverPort, &event);
		if (err == EMBX_SUCCESS) {
			MME_Assert(EMBX_REC_MESSAGE == event.type);
			MME_Assert(sizeof(MME_UINT) <= event.size);
			MME_Assert(NULL != event.data);

			/* the message ID decides. */
			switch (*((MME_UINT*) event.data)) {
			case TMESSID_TRANSFORM:
				enqueueCommand(receiver, (TransformerTransformMessage *) event.data);
				break;

			case TMESSID_ABORT: {
				abortCommand(receiver, (TransformerAbortMessage *) event.data);
			}	break;

			default:
				MME_Assert(0); /* not reached */
				break;
			}
		} else {
			if (!receiver->running) {
				break;
			}

			/* the error was probably not due to port closure, lets
			 * not chew the whole CPU while we wait for the
			 * situation to improve 
			 */
			EMBX_OS_Delay(10);
		}
	}

	MME_Info(MME_INFO_RECEIVER, ("<<<ReceiverTaskMMEReceiver\n"));
}

/*******************************************************************/

MME_ERROR MME_Receiver_NotifyHost(MMEReceiver *recv, MME_Event_t event, TransformerTransformMessage *transform, MME_ERROR res)
{
	EMBX_ERROR err = EMBX_INVALID_ARGUMENT;
	void *buffer;

	MME_CommandStatus_t *status = (MME_CommandStatus_t*) &(transform->command.CmdStatus);

	MME_Info(MME_INFO_RECEIVER, (">>>MME_Receiver_NotifyHost 0x%08x, event %d, cmdid %08x\n",
	                             (unsigned) recv, event, status->CmdId));

	status->Error = res;

	MME_Assert(recv);
	MME_Assert(transform);
	MME_Assert(transform->id == TMESSID_TRANSFORM);

	switch (event) {
	case MME_COMMAND_COMPLETED_EVT:
		res = reconvertTransformMessage(recv, transform);
		if (MME_SUCCESS == transform->command.CmdStatus.Error) {
			status->Error = res;
		}
		/*FALLTHRU*/

	case MME_COMMAND_DIED_EVT:
		/* Update the command status structure and hand everything back to the host */
		status->State = (MME_SUCCESS == transform->command.CmdStatus.Error) ?
		                MME_COMMAND_COMPLETED : MME_COMMAND_FAILED;
		err = EMBX_SendMessage(recv->replyPort, transform, transform->messageSize);
		MME_Info(MME_INFO_RECEIVER, ("   EMBX_SendMessage() = %d\n", err));
		break;

	case MME_DATA_UNDERFLOW_EVT:
	case MME_NOT_ENOUGH_MEMORY_EVT:
		/* Allocate a starvation message and send it to the host side, which disposes the buffer */
		err = EMBX_Alloc(recv->transportHandle, sizeof(TransformerStarvationMessage), &buffer);
		if (EMBX_SUCCESS == err) {
			TransformerStarvationMessage *starvation = (TransformerStarvationMessage *) buffer;

			starvation->id = TMESSID_STARVATION;
			starvation->messageSize = sizeof(TransformerStarvationMessage);
			starvation->event = event;
			memcpy(&(starvation->status), &(transform->command.CmdStatus), sizeof(MME_CommandStatus_t));

			err = EMBX_SendMessage(recv->replyPort, starvation, starvation->messageSize);
		}
		break;

	default:
		MME_Assert(0);
		break;
	}

	/* remap the errors as we return */
	MME_Info(MME_INFO_RECEIVER, ("<<<MME_Receiver_NotifyHost = %s\n",
	         (EMBX_SUCCESS == err ? "MME_SUCCESS" :
	          EMBX_NOMEM   == err ? "MME_NOMEM" :
		                        "MME_INTERNAL_ERROR")));

	return (EMBX_SUCCESS == err ? MME_SUCCESS :
	        EMBX_NOMEM   == err ? MME_NOMEM :
		                      MME_INTERNAL_ERROR);
}

/*******************************************************************/

#ifdef EMBX_RECEIVE_CALLBACK
MME_ERROR MME_Receiver_Init_Callback(MMEReceiver * receiver, MMEManager_t *manager,
				     EMBX_TRANSPORT transportHandle, TransformerInitMessage * message,
				     EMBX_PORT replyPort, EMBX_EVENT *event, EMBX_BOOL createThread)
#else
MME_ERROR MME_Receiver_Init(MMEReceiver * receiver, MMEManager_t *manager,
			    EMBX_TRANSPORT transportHandle, TransformerInitMessage * message,
			    EMBX_PORT replyPort, EMBX_EVENT *event)
#endif /* EMBX_RECEIVE_CALLBACK */
{
	MME_TransformerInitParams_t *initParams;
	MME_ERROR res = MME_NOMEM; 
	MME_ERROR msgQ = MME_NOMEM;
	MME_ERROR bufQ = MME_NOMEM; 
	MME_ERROR init = MME_NOMEM;
	EMBX_ERROR port = EMBX_NOMEM;

	MME_Info(MME_INFO_RECEIVER, ("InitializeMMEReceiver...\n"));
	receiver->manager = manager;
	receiver->transportHandle = transportHandle;
	receiver->replyPort = replyPort;
	receiver->commandEvent = event;

	strncpy(receiver->replyPortName, message->portName, EMBX_MAX_PORT_NAME);
	receiver->replyPortName[EMBX_MAX_PORT_NAME - 1] = '\0';

	msgQ = MME_MessageQueue_Init(&receiver->messageQueue, MAX_TRANSFORMER_SEND_COMMANDS);
	bufQ = MME_MessageQueue_Init(&receiver->bufferQueue, MAX_TRANSFORMER_SEND_BUFFERS);
	receiver->messageTable = EMBX_OS_MemAlloc(
			MAX_TRANSFORMER_SEND_OPS * sizeof(receiver->messageTable[0]));
	if (MME_SUCCESS != msgQ || MME_SUCCESS != bufQ || NULL == receiver->messageTable) {
		goto error_recovery;
	}

	/* Copy the init parameters, because they may be gone after the init message goes back to
	 * the host.
	 */
	initParams = &receiver->initParams;
	initParams->StructSize = sizeof(MME_TransformerInitParams_t);
	initParams->Callback = (MME_GenericCallback_t) NULL;
	initParams->CallbackUserData = (void *) NULL;
	initParams->TransformerInitParamsSize = message->messageSize - sizeof(TransformerInitMessage);
	initParams->TransformerInitParams_p = (initParams->TransformerInitParamsSize) ?
					      (MME_GenericParams_t*) (message + 1) : 0;

	/* Get a handle from the transformer. */
	init = (*(receiver->InitTransformer)) (initParams->TransformerInitParamsSize, 
			initParams->TransformerInitParams_p, (void**)&receiver->mmeHandle);
	if (MME_SUCCESS != init) {
		res = init;
		goto error_recovery;
	}

	/* Create the receiver port. */
	MME_Assert(0 == receiver->receiverPortExists);
	sprintf(receiver->receiverPortName, "rec%s", message->portName);
#ifdef EMBX_RECEIVE_CALLBACK
	if (createThread == EMBX_FALSE) {
		printf("EMBX_RECEIVE_CALLBACK: Installing Callback for companion\n");
		port = EMBX_CreatePortCallback(receiver->transportHandle, receiver->receiverPortName, &receiver->receiverPort,
					       receiverCallback, (EMBX_HANDLE) receiver);
		if (EMBX_SUCCESS != port) {
			goto error_recovery;
		}
		receiver->receiverPortExists = 1;
	}
	else
#endif /* EMBX_RECEIVE_CALLBACK */
	{
		port = EMBX_CreatePort(receiver->transportHandle, receiver->receiverPortName, &receiver->receiverPort);
		if (EMBX_SUCCESS != port) {
			goto error_recovery;
		}
		receiver->receiverPortExists = 1;
		
		/* Create the receiver task with a name like the receiver port and mid priority (e.g. 111 of 255). */
		MME_Assert(0 == receiver->running);
		receiver->running = 1;
		receiver->receiverTask = EMBX_OS_ThreadCreate(&receiverTask, receiver, RECEIVER_TASK_PRIORITY, receiver->receiverPortName);
		if (EMBX_INVALID_THREAD == receiver->receiverTask) {
			receiver->running = 0;
			goto error_recovery;
		}
	}

	/* Store the receiver port name and the MME handle into the message. */
	strncpy(message->portName, receiver->receiverPortName, EMBX_MAX_PORT_NAME);
		message->portName[EMBX_MAX_PORT_NAME - 1] = '\0';

	/* The handle the host uses for future association with the
	 * companion instance (e.g. terminate request) is the 'receiver' pointer'
	 */
	message->mmeHandle = (MME_UINT)receiver;
	message->numCommandQueueEntries = MAX_TRANSFORMER_SEND_COMMANDS;

	/* Done. */
	return MME_SUCCESS;

    error_recovery:

	if (EMBX_SUCCESS == port) {
		EMBX_ClosePort(receiver->receiverPort);
	}

	if (MME_SUCCESS == init) {
		(*(receiver->TermTransformer)) ((void*)receiver->mmeHandle);
	}

	free(receiver->messageTable);
	
	if (MME_SUCCESS == bufQ) {
		MME_MessageQueue_Term(&receiver->bufferQueue);
	}

	if (MME_SUCCESS == msgQ) {
		MME_MessageQueue_Term(&receiver->messageQueue);
	}

	return res;
}

#ifdef EMBX_RECEIVE_CALLBACK
MME_ERROR MME_Receiver_Init(MMEReceiver * receiver, MMEManager_t *manager,
			    EMBX_TRANSPORT transportHandle, TransformerInitMessage * message,
			    EMBX_PORT replyPort, EMBX_EVENT *event)
{
	/* Standard MME_Receiver_Init() with helper threads */
	return MME_Receiver_Init_Callback(receiver, manager, transportHandle, message,
					  replyPort, event, EMBX_TRUE);
}
#endif /* EMBX_RECEIVE_CALLBACK */

MME_ERROR MME_Receiver_Term(MMEReceiver * receiver)
{
	void *buffer;
	MME_ERROR result;

	/* If there are any outstanding commands on the queue then the host has, incorrectly,
	 * permitted us to terminate while not fully idle. This is a bug on the host (or bad
	 * state tracking on the companion.
	 */
	MME_Assert(MME_INVALID_HANDLE == MME_MessageQueue_Peek(&receiver->messageQueue, &buffer));
	
	/* Try to terminate the transformer. */
	result = (*(receiver->TermTransformer)) ((void*)receiver->mmeHandle);
	if (result != MME_SUCCESS) {
		MME_Info(MME_INFO_RECEIVER, ("Failed to terminate transformer\n"));
		return result;
	}
	/* The transformer was terminated. We now MUST close everything. */

	/* This sequence correctly terminates the receiver task. */
	receiver->running = 0;
	EMBX_InvalidatePort(receiver->receiverPort);
#ifdef EMBX_RECEIVE_CALLBACK
	if (receiver->receiverTask)
#endif /* EMBX_RECEIVE_CALLBACK */
		(void) EMBX_OS_ThreadDelete(receiver->receiverTask);
	EMBX_ClosePort(receiver->receiverPort);
	receiver->receiverPortExists = 0;

	EMBX_OS_MemFree(receiver->messageTable);
	MME_MessageQueue_Term(&receiver->bufferQueue);
	MME_MessageQueue_Term(&receiver->messageQueue);

	return MME_SUCCESS;
}

/*******************************************************************/

MME_ERROR MME_Receiver_NextDueTime(MMEReceiver * receiver, MME_Time_t * dueTime)
{
	void *buffer;
	TransformerTransformMessage *message;
	MME_ERROR res;

	res = MME_MessageQueue_Peek(&receiver->messageQueue, &buffer);
	if (res == MME_SUCCESS) {
		message = (TransformerTransformMessage *) buffer;
		*dueTime = message->command.DueTime;
	}
	return res;
}

EMBX_PORT MME_Receiver_GetReplyPort(MMEReceiver * receiver)
{
	return receiver->replyPort;
}

/*******************************************************************/

/* Actually perform a transform (command). This is called from one of the execution loops. */
MME_ERROR MME_Receiver_ProcessCommand(MMEReceiver * receiver)
{
	void *buffer;
	TransformerTransformMessage *message;
	MME_ERROR res;
	MME_Event_t evt = MME_COMMAND_COMPLETED_EVT;

	res = MME_MessageQueue_Dequeue(&receiver->messageQueue, &buffer);

	/* This is taken by the managers execution loop to prevent abortion until the
	 * head of the queue has been removed.
	 */
	EMBX_OS_MUTEX_RELEASE(&receiver->manager->abortMutex);

	if (res == MME_SUCCESS) {
		message = (TransformerTransformMessage *) buffer;
		/* convert buffer pointers and execute if all goes well */
		res = convertTransformMessage(receiver, message);
		if (MME_SUCCESS == res) {
			res = (*(receiver->ProcessCommand)) ((void*)receiver->mmeHandle, &message->command);
		} else {
			/* MME_COMMAND_DIED_EVT suppresses message reconvertion */
			evt = MME_COMMAND_DIED_EVT;
		}

		if (MME_TRANSFORM_DEFERRED != res) {
			res = MME_Receiver_NotifyHost(receiver, evt, message, res);
			MME_Assert(MME_SUCCESS == res); /* cannot recover */
		}
	}
	return res;
}

/*******************************************************************/

void MME_Receiver_Destroy(MMEReceiver * receiver)
{
	/* Destruct local stuff. */
	EMBX_OS_MemFree(receiver);
}

MME_ERROR MME_Receiver_Create(MMEReceiverFactory* factory, MMEReceiver ** receiver_p)
{
	MMEReceiver* receiver = EMBX_OS_MemAlloc(sizeof(MMEReceiver));
	if (NULL == receiver) {
		return MME_NOMEM;
	}
	memset(receiver, 0, sizeof(MMEReceiver));

	/* Configure our virtual function table. */
	receiver->Abort                    = factory->abortFunc;
	receiver->GetTransformerCapability = factory->getTransformerCapabilityFunc;
	receiver->InitTransformer          = factory->initTransformerFunc;
	receiver->ProcessCommand           = factory->processCommandFunc;
	receiver->TermTransformer          = factory->termTransformerFunc;
	
	/* No other non-zero members */
	
	*receiver_p = receiver;
	return MME_SUCCESS;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
