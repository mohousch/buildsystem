/* #define DEBUG */

/*
 * RemoteTransformer.c
 *
 * Copyright (C) STMicroelectronics Limited 2003-2004. All rights reserved.
 *
 * Remote transformer management for MME host side.
 */

#include "mme_hostP.h"

/*******************************************************************/

/* Private data type describing a single EMBX port */
struct RemotePort {
	EMBX_PORT		port;
	char			name[EMBX_MAX_PORT_NAME+1];
	int			valid;
};

/* Private data type that fully describes the remote transformer instance */
typedef struct RemoteTransformer {
	Transformer_t super;
	EMBX_PORT		adminPort;		/* supplied by the caller of Create */

	MME_TransformerHandle_t mmeHandle;		/* created on the MME side; this is NOT the host handle! */

	struct RemotePort	sendPort;		/* to talk to the MME */
	struct RemotePort	replyPort;		/* to receive message from the MME */

	EMBX_THREAD		receiverThread;
	EMBX_EVENT		terminateWasReplied;
	MME_ERROR		terminationResult;

	EMBX_EVENT		isAliveWasReplied;	/* Event which is posted when IsStillAlive request completes */

	int			maxCommandSlots;	/* array size */
	int			numCommandSlots;	/* current number of used command slots */
	int			commandIndex;		/* index inside the array */
	MME_CommandId_t		commandIDCounter;	/* generates unique commandIDs for both 
							 * TRANSFORM and SEND BUFFERS */
	CommandSlot_t*		commandSlots;
	EMBX_MUTEX		commandSlotLock;

	EMBX_VOID ** volatile	bufferCache;
} RemoteTransformer_t;

/*******************************************************************/

static MME_ERROR RemoteTransformer_ReleaseCommandSlot(
		Transformer_t* transformer, CommandSlot_t* slot);
static MME_ERROR RemoteTransformer_WaitForMessage(Transformer_t*  transformer, 
                                MME_Event_t*    eventPtr,
		                MME_Command_t** commandPtr);

/*******************************************************************/

/* This code forms a small cache of pre-allocated blocks that can be allocated by
 * the host without using (comparitively slow) spinlocks and semaphores. additionally
 * block coalescing does not take place which saves a number of uncached reads
 */
static void *allocBuffer(RemoteTransformer_t *remoteTransformer, MME_UINT sz)
{
	EMBX_ERROR err;
	EMBX_VOID *ptr;

	if (sz <= 4096) {
		EMBX_OS_INTERRUPT_LOCK();		

		if (remoteTransformer->bufferCache) {
			ptr = remoteTransformer->bufferCache;

#ifdef DEBUG   			
			MME_Assert(((int)ptr & 3) == 0);

			if (*(int *)((char *)ptr + 4000) != EMBX_BUFFER_FREE ||
			    *(int *)((char *)ptr + 4) != EMBX_BUFFER_FREE)
			{
				EMBX_debug_print("ERROR: allocBuf ptr %p next %p is not free 0x%x/0x%x\n",
						 ptr, *remoteTransformer->bufferCache, 
						 *(int *)((char *)ptr + 4000),
						 *(int *)((char *)ptr + 4));
				
				EMBX_debug_print("ERROR: time freed %x caller %p now 0x%x caller %p\n",
						 *(int *)((char *)ptr + 4004),
						 *(void **)((char *)ptr + 4008),
						 (int) EMBX_OS_TIME_NOW(),
						 __builtin_return_address(0));
						 
				MME_Assert(0);
			}
#endif
			remoteTransformer->bufferCache = *remoteTransformer->bufferCache;

#ifdef DEBUG
			*(int *)((char *)ptr + 4000) = EMBX_BUFFER_ALLOCATED;
			*(int *)((char *)ptr + 4004) = EMBX_OS_TIME_NOW();
			*(void **)((char *)ptr + 4008) = __builtin_return_address(0);
#endif

			EMBX_OS_INTERRUPT_UNLOCK();

			/* this assert technically has side effects (alters sz) so have a
			 * care if altering this function
			 */
			MME_Assert(EMBX_SUCCESS == EMBX_GetBufferSize(ptr, &sz));
			MME_Assert(4096 == sz);
			return ptr;
		}

		sz = 4096;
		EMBX_OS_INTERRUPT_UNLOCK();
	}
	err = EMBX_Alloc(remoteTransformer->super.info->handle, sz, &ptr);

#ifdef DEBUG
	if (err == EMBX_SUCCESS && sz == 4096) {
		MME_Assert(((int)ptr & 3) == 0);

		*(int *)((char *)ptr + 4000) = EMBX_BUFFER_ALLOCATED;
		*(int *)((char *)ptr + 4004) = EMBX_OS_TIME_NOW();
		*(void **)((char *)ptr + 4008) = __builtin_return_address(0);
	}
#endif

	return (EMBX_SUCCESS == err ? ptr : 0);
}

/* The list management here is extremely vulnerable to duplicate frees. this
 * will make the buffer list cyclic resulting either in the buffer release
 * code in RemoteTransformer_Term() looping forever or (worse) memory
 * being allocated twice.
 */
static void freeBuffer(RemoteTransformer_t *remoteTransformer, EMBX_VOID *ptr)
{
	EMBX_UINT sz;
	EMBX_ERROR err;

	err = EMBX_GetBufferSize(ptr, &sz);
	MME_Assert(EMBX_SUCCESS == err);

	if (sz == 4096) {
		EMBX_VOID **p = ptr;

		EMBX_OS_INTERRUPT_LOCK();
#ifdef DEBUG

		MME_Assert(((int)ptr & 3) == 0);

		if (*(int *)((char *)ptr + 4000) != EMBX_BUFFER_ALLOCATED)
		{
			EMBX_debug_print("ERROR: freeBuf ptr %p is not allocated 0x%x\n",
					 ptr, 
					 *(int *)((char *)ptr + 4000));
			       
			EMBX_debug_print("ERROR: time alloc 0x%x caller %p now 0x%x caller %p\n",
					 *(int *)((char *)ptr + 4004),
					 *(void **)((char *)ptr + 4008),
					 (int) EMBX_OS_TIME_NOW(),
					 __builtin_return_address(0));

			MME_Assert(0);
		}
#endif
		MME_Assert(ptr != remoteTransformer->bufferCache);
		
		*p = remoteTransformer->bufferCache;
		remoteTransformer->bufferCache = p;

#ifdef DEBUG		
		*(int *)((char *)ptr + 4000) = EMBX_BUFFER_FREE;
		*(int *)((char *)ptr + 4)    = EMBX_BUFFER_FREE;
		*(int *)((char *)ptr + 4004) = EMBX_OS_TIME_NOW();
		*(void **)((char *)ptr + 4008) = __builtin_return_address(0);
#endif
			
		EMBX_OS_INTERRUPT_UNLOCK();
	} else {
		EMBX_ERROR err = EMBX_Free(ptr);
		MME_Assert(EMBX_SUCCESS == err);
	}
}

/*******************************************************************/

static MME_ERROR createInitMessage(RemoteTransformer_t* remoteTransformer, const char* name,
		MME_TransformerInitParams_t* params, EMBX_VOID** buffer)
{
	TransformerInitMessage *message;
	unsigned paramsSize;
	int messageSize;

	MME_Assert(params->StructSize == sizeof(MME_TransformerInitParams_t));

	messageSize = sizeof(TransformerInitMessage);
	paramsSize  = params->TransformerInitParamsSize;
	messageSize += paramsSize;

	message = allocBuffer(remoteTransformer, messageSize);
	if (0 == message ) {
		return MME_NOMEM;
	}

	message->id = TMESSID_INIT;
	message->messageSize = messageSize;
	message->priority = params->Priority;
	message->result = MME_SUCCESS;

	MME_Assert(strlen(name) < MME_MAX_TRANSFORMER_NAME);
	strcpy(message->transformerType, name);

	MME_Assert(strlen(remoteTransformer->replyPort.name) < EMBX_MAX_PORT_NAME);
	strcpy(message->portName, remoteTransformer->replyPort.name);

	/* Copy TransformerInitParams data */
	/* TODO: these should be byte swapped on a big endian machine */
	memcpy((void *) (message + 1), params->TransformerInitParams_p, paramsSize);

	*buffer = message;
	return MME_SUCCESS;
}

static MME_ERROR cleanupInitMessage(RemoteTransformer_t* remoteTransformer, EMBX_VOID * buffer)
{
	if (buffer) {
		freeBuffer(remoteTransformer, buffer);
	}
	return MME_SUCCESS;
}

/*******************************************************************/
#define DEBUG_FUNC_STR "createTransformMessage:RemoteTransformer"

static MME_ERROR createTransformMessage(RemoteTransformer_t* remoteTransformer, 
						    MME_Command_t * cmd, EMBX_VOID ** buffer)
{
	union {
		TransformerTransformMessage *message;
		MME_DataBuffer_t *buffer;
		MME_ScatterPage_t *page;
		EMBX_HANDLE *hdl;
		char *ch;
		int i;
		void *p;
		void **pp;
	} p, q, size;

	MME_ERROR res = MME_NOMEM;
	EMBX_ERROR err;
	unsigned int i, j;

	/* all argument checking should have been performed further up the food chain
	 * so all the argument checking in this function can asserted
	 */
	MME_Assert(remoteTransformer);
	MME_Assert(cmd);
	MME_Assert(buffer);

        MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Command 0x%08x\n", (int)cmd));

	/* determine the size of message we will need */
	size.i = 0;
	size.message += 1;
	size.buffer += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;
	size.pp += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;
	for (i=0; i < (cmd->NumberInputBuffers + cmd->NumberOutputBuffers); i++) {
		size.page += cmd->DataBuffers_p[i]->NumberOfScatterPages;
		size.hdl  += cmd->DataBuffers_p[i]->NumberOfScatterPages;
	}
	size.i += cmd->CmdStatus.AdditionalInfoSize;
	size.i += cmd->ParamSize;

	/* allocate the message */
	p.p = allocBuffer(remoteTransformer, size.i);
	if (0 == p.p) {
		return MME_NOMEM;
	}
	*buffer = p.p;

	/* populate the fundamental message structure */
	p.message = (TransformerTransformMessage *) (*buffer);
	p.message->id = TMESSID_TRANSFORM;
	p.message->messageSize = size.i;
	p.message->hostCommand = cmd;
	p.message->receiverInstance = (MME_UINT)remoteTransformer->mmeHandle;  /* used for sanity check on companion */
	p.message->command = *cmd;
	p.message->command.DataBuffers_p = NULL;	/* invalid local pointers */
	p.message->transformer = remoteTransformer;
	p.message->numInputBuffers = cmd->NumberInputBuffers; 
	p.message->numOutputBuffers = cmd->NumberOutputBuffers;
	p.message += 1;

	/* copy the data buffers into our message */
	for (i=0; i < (cmd->NumberInputBuffers + cmd->NumberOutputBuffers); i++) {
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Buffer %d at 0x%08x\n", 
                         i, (int)(cmd->DataBuffers_p[i]) ));
		*p.buffer++ = *cmd->DataBuffers_p[i];
	}

	/* skip the pre-allocated data buffers structure */
	p.pp += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;

	/* copy the scatter pages into our message */
	for (i=0; i < (cmd->NumberInputBuffers + cmd->NumberOutputBuffers); i++) {
		MME_DataBuffer_t *buf = cmd->DataBuffers_p[i];
		MME_ScatterPage_t *pages;
		EMBX_HANDLE *hdls;
		
		pages = p.page;
		p.page += buf->NumberOfScatterPages;
		hdls = p.hdl;
		p.hdl += buf->NumberOfScatterPages;

		for (j=0; j < buf->NumberOfScatterPages; j++) {
			MME_ScatterPage_t *page = &buf->ScatterPages_p[j];

			MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Page %d at 0x%08x\n", j, (int)page ));

			/* copy the original page into the data buffer */
			pages[j] = *page;

			if ((page->FlagsIn & MME_DATA_PHYSICAL))
			{
				void *addr;

				/* Check physical address against the Warp ranges by attempting to 
				 * convert it back using EMBX_Address(). If this succeeds we know it
				 * lies within a Warp range
				 */
				err = EMBX_Address(remoteTransformer->super.info->handle, (EMBX_INT)page->Page_p, &addr);

				if (EMBX_SUCCESS == err || EMBX_INCOHERENT_MEMORY == err)
				{
					/* simple case if the address is already a physical one */
					hdls[j] = (EMBX_HANDLE)page->Page_p;
					
					MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR 
									"Page 0x%08x, Pointer warp %s, Handle 0x%08x buf-id %d, page-id %d\n", 
									(int) page->Page_p, 
									"PHYS",
									(int)hdls[j], i, j));
					continue;
				}
				else {
					/* XXXX Help! What can we do now as there is no kernel mapping for this user page */
					res = MME_EMBX_ERROR;
					MME_Info(1, (DEBUG_NOTIFY_STR "Page 0x%08x Size %d Failed to warp PHYS addr err %d, i %d, j %d\n", 
                                                     (int) page->Page_p, page->Size, err, i, j));
					goto error_recovery;
				}

			}

			/* Intercept NULL pointer addresses as soon as we can 
			 * otherwise we will end up attempting to perform VirtToPhys or data copy on it
			 *
			 * But allow through zero sized pages as some drivers can generate these under normal conditions
			 */
			if ((page->Page_p == NULL) && page->Size)
			{
				res = MME_INVALID_ARGUMENT;
				goto error_recovery;
			}

			/* serialize the object (try direct pointer warping before falling back to registering
			 * the object)
			 */
			err = EMBX_Offset(remoteTransformer->super.info->handle,
			                  page->Page_p, (EMBX_INT *) hdls + j);

			MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR 
                                 "Page 0x%08x, Pointer warp %s, Handle 0x%08x buf-id %d, page-id %d\n", 
                                 (int) page->Page_p, 
				 (EMBX_INCOHERENT_MEMORY==err) ? "PURGE" :
				                                 (EMBX_SUCCESS==err) ? "YES" : "NO",
	                         (int)hdls[j], i, j));

			if (EMBX_INCOHERENT_MEMORY == err) {
				if (0 == (page->FlagsIn & MME_DATA_CACHE_COHERENT)) {
					EMBX_OS_PurgeCache(page->Page_p, page->Size);
				}
			} else if (EMBX_SUCCESS != err) {
				err = EMBX_RegisterObject(remoteTransformer->super.info->handle, 
							  page->Page_p, page->Size, hdls + j);
				if (EMBX_SUCCESS != err) {
					res = MME_EMBX_ERROR;
					MME_Info(1, (DEBUG_NOTIFY_STR "Page 0x%08x Size %d Failed to reg obj err %d, i %d, j %d\n", 
                                                     (int) page->Page_p, page->Size, err, i, j));
					goto error_recovery;
				}
                        }

			/* update the copy (or view) of the object at our destination port */
			if ((0xf0000000 & hdls[j]) == 0xf0000000) {

				err = EMBX_UpdateObject(remoteTransformer->sendPort.port, 
							hdls[j], 0, page->Size);
				if (EMBX_SUCCESS != err) {
					MME_Info(1, (DEBUG_NOTIFY_STR "Failed to update obj err %d\n", err ));
					res = MME_EMBX_ERROR;
					goto error_recovery;
				}
			}
		}
	}
	
	/* TODO: Need to use a macro to swap all args to little endian if on a big endian machine */

	if (cmd->CmdStatus.AdditionalInfoSize > 0) {
		memcpy(p.p, cmd->CmdStatus.AdditionalInfo_p, 
		       cmd->CmdStatus.AdditionalInfoSize);
		p.ch += cmd->CmdStatus.AdditionalInfoSize;
	}

	if (cmd->ParamSize > 0) {
		memcpy(p.p, cmd->Param_p, cmd->ParamSize);
	}

	return MME_SUCCESS;

    error_recovery:
	/* to recover from the error we walk through the currently marshalled data knowing
	 * that p (in any of its forms) points to the first uninitialized byte in the 
	 * buffer.
	 */
	
	/* get to the scatter page and handles section of the buffer */
	q.p = *buffer;
	q.message += 1;
	q.buffer += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;
	q.pp += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;

	/* now iterate though the buffer deregistering the objects */
	for (i=0; i < (cmd->NumberInputBuffers + cmd->NumberOutputBuffers) && q.p < p.p; i++) {
		q.page += cmd->DataBuffers_p[i]->NumberOfScatterPages;
		for (j=0; j < cmd->DataBuffers_p[i]->NumberOfScatterPages && q.p < p.p; j++) {
			if ((0xf0000000 & q.hdl[j]) == 0xf0000000) {
				err = EMBX_DeregisterObject(
						remoteTransformer->super.info->handle, q.hdl[j]);
				MME_Assert(EMBX_SUCCESS == err); /* no error recovery possible */
			}
		}
		q.hdl += cmd->DataBuffers_p[i]->NumberOfScatterPages;
	}

	/* unallocate the buffer */

	freeBuffer(remoteTransformer, buffer);

	return res;
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR "cleanupTransformMessage:RemoteTransformer"
static MME_ERROR cleanupTransformMessage(RemoteTransformer_t* remoteTransformer, EMBX_VOID * buffer)
{
	union {
		TransformerTransformMessage *message;
		MME_DataBuffer_t *buffer;
		MME_ScatterPage_t *page;
		EMBX_HANDLE *hdl;
		char *ch;
		int i;
		void *p;
		void **pp;
	} p;

	MME_Command_t *cmd;
	EMBX_ERROR err;
	unsigned int i, j;

	p.p = buffer;
	MME_Assert(p.message->numInputBuffers == p.message->hostCommand->NumberInputBuffers);
	MME_Assert(p.message->numOutputBuffers == p.message->hostCommand->NumberOutputBuffers);

	cmd = p.message->hostCommand;

	p.message += 1;
	p.buffer += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;
	p.pp += cmd->NumberInputBuffers + cmd->NumberOutputBuffers;

	for (i = 0; i < (cmd->NumberInputBuffers + cmd->NumberOutputBuffers); i++) {
		MME_DataBuffer_t *buf = cmd->DataBuffers_p[i];
		MME_ScatterPage_t *pages = p.page;

		p.page += buf->NumberOfScatterPages;

		for (j=0; j < buf->NumberOfScatterPages; j++) {
			if (i >= cmd->NumberInputBuffers) {
				buf->ScatterPages_p[j].BytesUsed = pages[j].BytesUsed;
				buf->ScatterPages_p[j].FlagsOut = pages[j].FlagsOut;
			}

			if ((0xf0000000 & p.hdl[j]) == 0xf0000000) {
				err = EMBX_DeregisterObject(
						remoteTransformer->super.info->handle, p.hdl[j]);
				MME_Assert(EMBX_SUCCESS == err); /* no error recovery possible */
			}
		}
		
		p.hdl += buf->NumberOfScatterPages;
	}

	/* Deal with status transformer paramters */
	if (cmd->CmdStatus.AdditionalInfoSize > 0) {
		memcpy(cmd->CmdStatus.AdditionalInfo_p, p.p, cmd->CmdStatus.AdditionalInfoSize);
	}

	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR
/*******************************************************************/

static MME_ERROR createTerminateMessage(RemoteTransformer_t* remoteTransformer, int messageSize, EMBX_VOID ** buffer)
{
	TransformerTerminateMessage *message;

	messageSize += sizeof(TransformerTerminateMessage);

	*buffer = allocBuffer(remoteTransformer, messageSize);
	if (*buffer) {
		message = (TransformerTerminateMessage *) (*buffer);

		message->id = TMESSID_TERMINATE;
		message->messageSize = messageSize;
		message->mmeHandle = remoteTransformer->mmeHandle;
		message->result = MME_SUCCESS;

		return MME_SUCCESS;
	}

	return MME_NOMEM;
}

static MME_ERROR cleanupTerminateMessage(RemoteTransformer_t *remoteTransformer, EMBX_VOID * buffer, EMBX_BOOL dead)
{
	if (dead)
		/* TIMEOUT_SUPPORT: Don't release dead buffers to the cache as they may have had
		 * their buf->htp mangled making them useless
		 */
		(void) EMBX_Release(remoteTransformer->super.info->handle, buffer);
	else
		freeBuffer(remoteTransformer, buffer);
	return MME_SUCCESS;
}

static MME_ERROR createIsAliveMessage(RemoteTransformer_t* remoteTransformer, EMBX_VOID ** buffer)
{
	int messageSize;
 	TransformerIsAliveMessage *message;

	messageSize = sizeof(TransformerIsAliveMessage);

	*buffer = allocBuffer(remoteTransformer, messageSize);
	if (*buffer) {
		message = (TransformerIsAliveMessage *) (*buffer);

		message->id = TMESSID_IS_ALIVE;
		message->messageSize = messageSize;
		message->mmeHandle = remoteTransformer->mmeHandle;

		return MME_SUCCESS;
	}

	return MME_NOMEM;
}

static MME_ERROR cleanupIsAliveMessage(RemoteTransformer_t *remoteTransformer, EMBX_VOID * buffer, EMBX_BOOL dead)
{
	if (dead)
		/* TIMEOUT_SUPPORT: Don't release dead buffers to the cache as they may have had
		 * their buf->htp mangled making them useless
		 */
		(void) EMBX_Release(remoteTransformer->super.info->handle, buffer);
	else
		freeBuffer(remoteTransformer, buffer);
	return MME_SUCCESS;
}

/*******************************************************************/

#define DEBUG_FUNC_STR  "receiveTransformMessage"
static void receiveTransformMessage(RemoteTransformer_t *remoteTransformer, 
				   TransformerTransformMessage *message,
                                   MME_Event_t*    eventPtr,
 				   MME_Command_t** commandPtr)
{
	MME_UINT slot;
	MME_CommandStatus_t *status;

	cleanupTransformMessage(remoteTransformer, message);

	/* Find the right slot for this commandID. */
	slot = MME_CMDID_GET_COMMAND(message->command.CmdStatus.CmdId);

	/* Setup our results and update the client's MME_CommandStatus_t. */
	*eventPtr = MME_COMMAND_COMPLETED_EVT;
	*commandPtr = remoteTransformer->commandSlots[slot].command;
	status = (MME_CommandStatus_t*) &((*commandPtr)->CmdStatus);

	status->Error = message->command.CmdStatus.Error;
	status->ProcessedTime = message->command.CmdStatus.ProcessedTime;

	/* At the moment that we write the command state, a sampling client will know
	  we are finished with the command. The command has completed or failed. */
	status->State = message->command.CmdStatus.State;

	RemoteTransformer_ReleaseCommandSlot((Transformer_t *) remoteTransformer,
			&(remoteTransformer->commandSlots[slot]));

	MME_Info(MME_INFO_TRANSFORMER,
			(DEBUG_NOTIFY_STR "MME_COMMAND_COMPLETED_EVT (cmd %08x @ 0x%08x) status %d/%d\n", 
			 message->command.CmdStatus.CmdId, (unsigned) *commandPtr,
			 status->Error, status->State));
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR "receiveStavationMessage: "
static void receiveStavationMessage(RemoteTransformer_t *remoteTransformer, 
				    TransformerStarvationMessage *message,
                                    MME_Event_t*    eventPtr,
				    MME_Command_t** commandPtr)
{
	MME_UINT slot;
	MME_CommandStatus_t *status;

	/* Find the right slot for this commandID. */
	slot = MME_CMDID_GET_COMMAND(message->status.CmdId);

	*eventPtr = (MME_Event_t) message->event;

	/* Copy the status from the message to the client's MME_CommandStatus_t. */
	*commandPtr = remoteTransformer->commandSlots[slot].command;
	status = (MME_CommandStatus_t*) &((*commandPtr)->CmdStatus);
	status->Error = message->status.Error;
	status->ProcessedTime = message->status.ProcessedTime;
	status->State = message->status.State;

	MME_Info(MME_INFO_TRANSFORMER,
			(DEBUG_NOTIFY_STR "Stavation event  %d (cmd %08x @ 0x%08x)\n",
			*eventPtr, message->status.CmdId, (unsigned) *commandPtr));
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR  "RemoteTransformer_WaitForMessage"
static MME_ERROR RemoteTransformer_WaitForMessage(Transformer_t*  transformer, 
						  MME_Event_t*    eventPtr,
						  MME_Command_t** commandPtr)
{
	EMBX_RECEIVE_EVENT event;
	RemoteTransformer_t* remoteTransformer = (RemoteTransformer_t*)transformer;
	EMBX_ERROR err;
	
	MME_Assert(transformer);
	MME_Assert(eventPtr);
	MME_Assert(commandPtr);

	err = EMBX_ReceiveBlock(remoteTransformer->replyPort.port, &event);
	if (EMBX_SUCCESS != err) {
		*eventPtr = MME_COMMAND_COMPLETED_EVT;
		*commandPtr = NULL;
		return (err == EMBX_SYSTEM_INTERRUPT ? MME_SYSTEM_INTERRUPT
						     : MME_TRANSFORMER_NOT_RESPONDING);
	}
	
	MME_Assert(event.data);
	MME_Assert(event.type == EMBX_REC_MESSAGE);

	/* received message. */
	switch (((MME_UINT *) event.data)[0]) {
	case TMESSID_TRANSFORM:
		receiveTransformMessage(remoteTransformer, 
					(TransformerTransformMessage *) event.data, eventPtr, commandPtr);
		freeBuffer(remoteTransformer, event.data);
		return MME_SUCCESS;

	case TMESSID_STARVATION: {
		receiveStavationMessage(remoteTransformer,
					(TransformerStarvationMessage *) event.data, eventPtr, commandPtr);
		freeBuffer(remoteTransformer, event.data);
		return MME_SUCCESS;
	}
	case TMESSID_TERMINATE: {
		/* A Terminate message returned */
		remoteTransformer->terminationResult = ((TransformerTerminateMessage *) event.data)->result;
		MME_Assert(MME_SUCCESS == remoteTransformer->terminationResult);

		cleanupTerminateMessage(remoteTransformer,
					(TransformerTerminateMessage *) event.data, EMBX_FALSE);
		*eventPtr = MME_COMMAND_COMPLETED_EVT;
		*commandPtr = NULL;
		EMBX_OS_EVENT_POST(&remoteTransformer->terminateWasReplied);
		return MME_TRANSFORMER_NOT_RESPONDING;
	}
	case TMESSID_IS_ALIVE: {
		/* An IsAlive message returned */
		cleanupIsAliveMessage(remoteTransformer, (TransformerIsAliveMessage *) event.data, EMBX_FALSE);
		*eventPtr = MME_COMMAND_COMPLETED_EVT;
		*commandPtr = NULL; /* This will avoid a user callback */
		EMBX_OS_EVENT_POST(&remoteTransformer->isAliveWasReplied);
		return MME_SUCCESS;
	}
	default:
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Unexpected message: %x\n", (unsigned) ((MME_UINT *) event.data)[0]));
#ifdef DEBUG
		EMBX_debug_print("event %p type %d handle %x data %p size %d\n",
				 &event, event.type, event.handle, event.data, event.size);
		EMBX_debug_print("event data %x %x %x %x\n",
				 ((MME_UINT *) event.data)[0],
				 ((MME_UINT *) event.data)[1],
				 ((MME_UINT *) event.data)[2],
				 ((MME_UINT *) event.data)[3]);
#endif
				 
		MME_Assert(0);
	}
	
	return MME_INTERNAL_ERROR;		
}	
#undef DEBUG_FUNC_STR

#ifdef EMBX_RECEIVE_CALLBACK
#define DEBUG_FUNC_STR  "RemoteTransformer_Callback"
static EMBX_ERROR RemoteTransformer_Callback(EMBX_HANDLE transformer, 
					     EMBX_RECEIVE_EVENT *event)
{
	RemoteTransformer_t* remoteTransformer = (RemoteTransformer_t*)transformer;
	EMBX_ERROR err = MME_INTERNAL_ERROR;

	MME_Event_t evt;
	MME_Command_t *cmd;
	
	MME_Assert(transformer);

	MME_Assert(event->data);
	MME_Assert(event->type == EMBX_REC_MESSAGE);

	if (!remoteTransformer->mmeHandle)
	{
		/* Not fully initialised */
		return EMBX_DRIVER_NOT_INITIALIZED;
	}
	  
	/* received message. */
	switch (((MME_UINT *) event->data)[0]) {
	case TMESSID_TRANSFORM:
		receiveTransformMessage(remoteTransformer, 
					(TransformerTransformMessage *) event->data, &evt, &cmd);
		freeBuffer(remoteTransformer, event->data);
		err = EMBX_SUCCESS;
		
		break;
	case TMESSID_STARVATION: {
		receiveStavationMessage(remoteTransformer,
					(TransformerStarvationMessage *) event->data, &evt, &cmd);
		freeBuffer(remoteTransformer, event->data);
		err = EMBX_SUCCESS;

		break;
	}
	case TMESSID_TERMINATE: {
		/* A Terminate message returned */
		remoteTransformer->terminationResult = ((TransformerTerminateMessage *) event->data)->result;
		MME_Assert(MME_SUCCESS == remoteTransformer->terminationResult);

		cleanupTerminateMessage(remoteTransformer,
					(TransformerTerminateMessage *) event->data, EMBX_FALSE);
		evt = MME_COMMAND_COMPLETED_EVT;
		cmd = NULL;
		EMBX_OS_EVENT_POST(&remoteTransformer->terminateWasReplied);
		err = MME_TRANSFORMER_NOT_RESPONDING;

		break;
	}
	case TMESSID_IS_ALIVE: {
		/* An IsAlive message returned */
		cleanupIsAliveMessage(remoteTransformer, (TransformerIsAliveMessage *) event.data, EMBX_FALSE);
		evt = MME_COMMAND_COMPLETED_EVT;
		cmd = NULL; /* This will avoid a user callback */
		EMBX_OS_EVENT_POST(&remoteTransformer->isAliveWasReplied);
		err = MME_SUCCESS;
	}
	default:
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Unexpected message: %x\n", (unsigned) ((MME_UINT *) event->data)[0]));
		MME_Assert(0);
	}

	if (err == EMBX_SUCCESS)
	{
		/* perform the callback if this has been requested */
		if (MME_COMMAND_COMPLETED_EVT != evt ||
		    (cmd && (MME_COMMAND_END_RETURN_NOTIFY == cmd->CmdEnd))) {
			/* it would really make more sense to assert Callback and make it illegal to send
			 * commands with MME_COMMAND_END_RETURN_NOTIFY fail if Callback is NULL. However this
			 * would mean changing the behavior of the API.
			 */
			if (remoteTransformer->super.initParams.Callback) {
				remoteTransformer->super.initParams.Callback(evt, cmd,
									     remoteTransformer->super.initParams.CallbackUserData);
			}
		}
	}

	return err;
}	
#undef DEBUG_FUNC_STR

#endif /* EMBX_RECEIVE_CALLBACK */

static void receiverThread(void* ctx)
{
	RemoteTransformer_t* remoteTransformer = (RemoteTransformer_t*) ctx;
	MME_ERROR res;

	MME_Assert(remoteTransformer);
	MME_Assert(remoteTransformer->replyPort.valid);

	while (1) {
		MME_Event_t evt;
		MME_Command_t *cmd;

		/* wait for a transformer message (callback) to be received */		
		res = RemoteTransformer_WaitForMessage(
				(Transformer_t *) remoteTransformer, &evt, &cmd);
		
		if (MME_SUCCESS != res) {
			break;
		}
		
		/* perform the callback if this has been requested */
		if (MME_COMMAND_COMPLETED_EVT != evt ||
		    (cmd && (MME_COMMAND_END_RETURN_NOTIFY == cmd->CmdEnd))) {
		    	/* it would really make more sense to assert Callback and make it illegal to send
		    	 * commands with MME_COMMAND_END_RETURN_NOTIFY fail if Callback is NULL. However this
		    	 * would mean changing the behavior of the API.
		    	 */
		    	if (remoteTransformer->super.initParams.Callback) {
				remoteTransformer->super.initParams.Callback(evt, cmd,
						remoteTransformer->super.initParams.CallbackUserData);
		    	}
		}
	}
	
	MME_Info(MME_INFO_TRANSFORMER, ("receiverThread: terminating due (res = %d)\n", res));
}

/*******************************************************************/

static void RemoteTransformer_Destroy(Transformer_t* transformer)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	EMBX_ERROR err;

	remoteTransformer->super.info->referenceCount--;

	/* clean up the buffer cache (allocated during execution) */
	while (remoteTransformer->bufferCache) {
		void *buf = allocBuffer(remoteTransformer, 1);
		if (buf) {
			EMBX_ERROR err;
			/* To support companion recovery we have to call a special EMBX 
			 * free function which takes the transport handle as buf->htp 
			 * may have been rewritten by the remote cpu
			 */
			err = EMBX_Release(remoteTransformer->super.info->handle, buf);
			MME_Assert(EMBX_SUCCESS == err);
		}
	}
	
	if (remoteTransformer->adminPort) {
		err = EMBX_ClosePort(remoteTransformer->adminPort);
		MME_Assert(EMBX_SUCCESS == err); /* no recovery possible */
	}
	
	EMBX_OS_EVENT_DESTROY(&remoteTransformer->terminateWasReplied);
	EMBX_OS_EVENT_DESTROY(&remoteTransformer->isAliveWasReplied);
	EMBX_OS_MUTEX_DESTROY(&remoteTransformer->commandSlotLock);
	EMBX_OS_MemFree(remoteTransformer->commandSlots);
	EMBX_OS_MemFree(remoteTransformer);
	
	
}

/*******************************************************************/

static MME_ERROR RemoteTransformer_Init(Transformer_t* transformer, 
                                        const char* name,
					MME_TransformerInitParams_t * params, 
                                        MME_TransformerHandle_t handle, 
                                        EMBX_BOOL createThread)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	EMBX_ERROR err;
	EMBX_RECEIVE_EVENT event;
	TransformerInitMessage *message = NULL;
	char receiverThreadName[EMBX_MAX_PORT_NAME+1];
	int messageSize,n;
	MME_ERROR result, res;

	result = MME_NOMEM;

	remoteTransformer->super.handle = handle;

	/* TODO: We should take a copy of our (transformer specific) parameters. These might
	 * be deallocated once the transformer is initialized.
	 */
	remoteTransformer->super.initParams = *params;

	MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: initialising '%s'\n", name));

	/* Create the reply port. We use the unique handle to create a unique reply port name.*/
	/* MULTIHOST support: Use the transformer virtual address (perhaps phys would be safer?) */
	n = sprintf(remoteTransformer->replyPort.name, "MMEReply_0x%x", (int) transformer);
	MME_Assert(n < EMBX_MAX_PORT_NAME);
#ifdef EMBX_RECEIVE_CALLBACK
	if (createThread == EMBX_FALSE) {
		/* Create a port with a Callback hook */
		err = EMBX_CreatePortCallback(remoteTransformer->super.info->handle, 
					      remoteTransformer->replyPort.name, &remoteTransformer->replyPort.port,
					      RemoteTransformer_Callback,
					      (EMBX_HANDLE) transformer);
	}
	else
#endif /* EMBX_RECEIVE_CALLBACK */
		err = EMBX_CreatePort(remoteTransformer->super.info->handle, 
				      remoteTransformer->replyPort.name, &remoteTransformer->replyPort.port);
	if (EMBX_SUCCESS != err) {
		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Cannot create port '%s'\n",
		                                remoteTransformer->replyPort.name));
		goto embx_error_recovery;
	}
	remoteTransformer->replyPort.valid = 1;

	MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Created port '%s', port 0x%08x\n",
	                                remoteTransformer->replyPort.name,
					(unsigned) remoteTransformer->replyPort.port));
	
	/* Create the init message */
	res = createInitMessage(remoteTransformer, name, params, (EMBX_VOID **) &message);
	if (MME_SUCCESS != res) {
		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Cannot create initialisation message\n"));
		goto error_recovery;
	}

	/* Remember size for check of reply message */
	messageSize = message->messageSize;	

	/* Send the message */
	err = EMBX_SendMessage(remoteTransformer->adminPort, message, messageSize);
	if (err != EMBX_SUCCESS) {
		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Cannot send initialisation message\n"));
		goto embx_error_recovery;
	}

	/* Record ownership transfer (see error_recovery) */
	message = NULL;

	MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Waiting for initialisation reply\n"));

	/* Wait for an answer from the companion */
	err = EMBX_ReceiveBlock(remoteTransformer->replyPort.port, &event);
	if (EMBX_SUCCESS != err) {
		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Error while waiting for initialisation reply\n"));
		goto embx_error_recovery;
	}

	/* Check if the message is as expected */
	message = (TransformerInitMessage *) event.data;
	
	MME_Assert(EMBX_REC_MESSAGE == event.type);
	MME_Assert(event.size >= messageSize);
	MME_Assert(message && message->id == TMESSID_INIT);
	
	if (MME_SUCCESS != message->result) {
		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Error initialising remote transformer\n"));
		res = message->result;
		goto error_recovery;
	}

	/* The MME successfully created its transformer */
	strncpy(remoteTransformer->sendPort.name, message->portName, EMBX_MAX_PORT_NAME);
	remoteTransformer->sendPort.name[EMBX_MAX_PORT_NAME] = '\0';
	remoteTransformer->mmeHandle = message->mmeHandle;

	MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Connecting to transformer port '%s'\n",
	                                remoteTransformer->sendPort.name));

	/* Connect to the MME port */
	err = EMBX_ConnectBlock(remoteTransformer->super.info->handle, 
			        remoteTransformer->sendPort.name,
				&remoteTransformer->sendPort.port);
	if (EMBX_SUCCESS != err) {
		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Cannot connect to port '%s'\n",
		                                remoteTransformer->sendPort.name));
		goto embx_error_recovery;
	}
	remoteTransformer->sendPort.valid = 1;

	MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Connected to port '%s', port 0x%08x\n",
					remoteTransformer->sendPort.name,
					(unsigned) remoteTransformer->sendPort.port));

	if (createThread) {
		/* Create our receiver task for the reply port */
		sprintf(receiverThreadName, "HostRec%x", handle);

		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Creating receiver thread '%s'\n",
						receiverThreadName));

		/* TODO: think about whether we want all the receiver threads to have the same priority */
		remoteTransformer->receiverThread = EMBX_OS_ThreadCreate(receiverThread,
			(void *) remoteTransformer, RECEIVER_TASK_PRIORITY, receiverThreadName);
		if (EMBX_INVALID_THREAD == remoteTransformer->receiverThread) {
			MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Cannot create receiver thread '%s'\n",
			                                receiverThreadName));
			res = MME_NOMEM;
			goto error_recovery;
		}

		MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: Created receiver thread '%s', thread 0x%08x\n",
						receiverThreadName, (unsigned) remoteTransformer->receiverThread));
        }

	/* Cleanup and exit */
	cleanupInitMessage(remoteTransformer, message);
	MME_Info(MME_INFO_TRANSFORMER, ("RemoteTransformer_Init: SUCCESS\n"));
	return MME_SUCCESS;

    embx_error_recovery:

    	/* As normal error recovery but map the EMBX error code to a MME error code first */
    	res = (EMBX_NOMEM == err ? MME_NOMEM : MME_EMBX_ERROR);
    	
    error_recovery:
    
   	if (remoteTransformer->mmeHandle) {
   		MME_Assert(message);
   		message->id = TMESSID_TERMINATE;
   		err = EMBX_SendMessage(remoteTransformer->adminPort, message, message->messageSize);
   		if (EMBX_SUCCESS == err) {
   			/* TODO: is blocking really a good idea during error recovery? */
   			err = EMBX_ReceiveBlock(remoteTransformer->replyPort.port, &event);
   			message = (EMBX_SUCCESS == err ? event.data : NULL);
   		}
   		
   		remoteTransformer->mmeHandle = 0;
   	}
   	
   	if (remoteTransformer->sendPort.valid) {
   		EMBX_ClosePort(remoteTransformer->sendPort.port);
   		remoteTransformer->sendPort.valid = 0;
   	}
   	
    	if (remoteTransformer->replyPort.valid) {
    		EMBX_ClosePort(remoteTransformer->replyPort.port);
    		remoteTransformer->replyPort.valid = 0;
    	}
    	
    	if (message) {
    		cleanupInitMessage(remoteTransformer, message);
    	}
    
    	return res;
}

/*******************************************************************/

static MME_ERROR RemoteTransformer_TermOrKill(Transformer_t* transformer, int doKill)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	EMBX_ERROR err;
	EMBX_VOID *buffer;
	TransformerTerminateMessage *message;
	MME_ERROR result, res;
	result = MME_NOMEM;

	if (!remoteTransformer->sendPort.valid) {
		DP("MME port does not exist\n");
		return MME_INVALID_ARGUMENT;
	}
	/* No command must be pending on the companion side */
	if (remoteTransformer->numCommandSlots > 0) {
		DP("RemoteTransformer::Terminate() failed, commands pending\n");
		return MME_COMMAND_STILL_EXECUTING;
	}

	res = createTerminateMessage(remoteTransformer, 0, &buffer);
	if (res != MME_SUCCESS) {
		DP("CreateTerminateMsg() failed, error=%d\n", res);
		return res;
	}

	/* Send the message */
	message = (TransformerTerminateMessage *) buffer;
	DP("Sending %s message @$%08x...\n", (doKill ? "kill" : "terminate"), message);
	if (doKill) {
		EMBX_PORT replyPort;

		/* connect to our own reply port */
		err = EMBX_Connect(remoteTransformer->super.info->handle, remoteTransformer->replyPort.name, &replyPort);
		if (EMBX_SUCCESS != err) {
			DP("RemoteTransformer_Term(): failed to connect to self\n");
			cleanupTerminateMessage(remoteTransformer, message, EMBX_FALSE);
			return MME_EMBX_ERROR;	
		}

		err = EMBX_SendMessage(replyPort, message, message->messageSize);
		if (EMBX_SUCCESS != err) {
			DP("RemoteTransformer_Term(): failed to send message to self\n");
			/* Bugzilla 4962: We suspect the buffer may have a bad handle */
			cleanupTerminateMessage(remoteTransformer, message, EMBX_TRUE);
			return MME_EMBX_ERROR;	
		}

		EMBX_ClosePort(replyPort);
	} else {
		err = EMBX_SendMessage(remoteTransformer->adminPort, message, message->messageSize);
		if (EMBX_SUCCESS != err) {
			DP("RemoteTransformer_Term(): failed to send message to coprocessor\n");
			/* Bugzilla 4962: We suspect the buffer may have a bad handle */
			cleanupTerminateMessage(remoteTransformer, message, EMBX_TRUE);
			return MME_EMBX_ERROR;	
		}
	}

	/* lost ownership */
	message = NULL;

	/* Wait for ReceiverThread to terminate itself after receiving the reply message */
	DP("Waiting for ReceiverThread to terminate itself...\n");
	/* TIMEOUT SUPPORT: Allow wait to timeout */
	res = EMBX_OS_EVENT_WAIT_TIMEOUT(&remoteTransformer->terminateWasReplied, MME_TRANSFORMER_TIMEOUT);
	
	DP("terminateWasReplied res=%d terminationResult=0x%08x\n", res, remoteTransformer->terminationResult);
	
	/* Bugzilla #4228
	 *
	 * If terminate request timed out, return an error and get out.
	 * User will have to tidy up later with KillTransformer
	 */
	if (res == (MME_ERROR)EMBX_SYSTEM_TIMEOUT)
	{
		MME_Assert(doKill == 0);
		/* Bugzilla 4962: Release directly back to the transport in case the handle has been changed */
		cleanupTerminateMessage(remoteTransformer, (TransformerTerminateMessage *) buffer, EMBX_TRUE);
		return MME_TRANSFORMER_NOT_RESPONDING;
	}

	if (remoteTransformer->terminationResult != MME_SUCCESS) {
		DP("Failed to terminate receiver thread properly\n");
		return MME_INTERNAL_ERROR;
	}

#ifdef EMBX_RECEIVE_CALLBACK
	if (remoteTransformer->receiverThread)
#endif /* EMBX_RECEIVE_CALLBACK */
	{
		err = EMBX_OS_ThreadDelete(remoteTransformer->receiverThread);
		if (err != EMBX_SUCCESS) {
			DP("EMBX_OS_ThreadDelete() failed, error=%d\n", err);
		}
	}

	remoteTransformer->receiverThread = NULL;

	/* Close the MME port */
	DP("Close mmePort...\n");
	err = EMBX_ClosePort(remoteTransformer->sendPort.port);
	if (err != EMBX_SUCCESS) {
		DP("EMBX_ClosePort(mmePort) failed, error=%d\n", err);
	}
	/* Close the reply port */
	DP("Invalidate replyPort...\n");
	err = EMBX_InvalidatePort(remoteTransformer->replyPort.port);
	if (err != EMBX_SUCCESS) {
		DP("EMBX_InvalidatePort(replyPort) failed, error=%d\n", err);
	}
	DP("Close replyPort...\n");
	err = EMBX_ClosePort(remoteTransformer->replyPort.port);
	if (err != EMBX_SUCCESS) {
		DP("EMBX_ClosePort(replyPort) failed, error=%d\n", err);
	}
	remoteTransformer->replyPort.valid = EMBX_FALSE;

	return MME_SUCCESS;
}

static MME_ERROR RemoteTransformer_Term(Transformer_t* transformer)
{
	return RemoteTransformer_TermOrKill(transformer, 0);
}

static MME_ERROR RemoteTransformer_Kill(Transformer_t* transformer)
{
	return RemoteTransformer_TermOrKill(transformer, 1);
}

/*******************************************************************/

static CommandSlot_t* RemoteTransformer_LookupCommand(Transformer_t* transformer, int index)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	CommandSlot_t* val;
	if (index>=0 && index<remoteTransformer->maxCommandSlots) {
		val = &(remoteTransformer->commandSlots[index]);
	} else {
		val = NULL;
	}
	return val;
}

static MME_ERROR RemoteTransformer_GetTransformerCapability(
		Transformer_t *transformer, 
		const char *name, MME_TransformerCapability_t * capability)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	MME_ERROR res = MME_NOMEM;

	EMBX_TRANSPORT hdl;
	EMBX_PORT replyPort;
	int n, messageSize; 
	TransformerCapabilityMessage *message;
	EMBX_RECEIVE_EVENT event;

	MME_Assert(remoteTransformer);
	MME_Assert(name);
	MME_Assert(capability);

	hdl = remoteTransformer->super.info->handle;
	MME_Assert(hdl);

	/* calculate the size of the capability message */
	messageSize = sizeof(*message) + capability->TransformerInfoSize;

	/* allocate somewhere to store the message we are about to send */
	message = allocBuffer(remoteTransformer, messageSize);
	if (0 == message) {
		return MME_NOMEM;
	}

	/* create a port to be used to reply to this message */
	n = sprintf(message->portName, "MMECapability_0x%x", (unsigned) message);
	MME_Assert(n < EMBX_MAX_PORT_NAME);
	if (EMBX_SUCCESS != EMBX_CreatePort(hdl, message->portName, &replyPort)) {
		goto error_recovery_alloc;
	}

	/* populate the rest of the message */
	message->id = TMESSID_CAPABILITY;
	message->messageSize = messageSize;
	strncpy(message->transformerType, name, MME_MAX_TRANSFORMER_NAME);
	message->transformerType[MME_MAX_TRANSFORMER_NAME - 1] = '\0';
	message->capability = *capability;

	/* send the message and wait for a reply */
	if (EMBX_SUCCESS != EMBX_SendMessage(remoteTransformer->adminPort, message, sizeof(*message)))
		goto error_recovery_createport;
	else
	{
		EMBX_ERROR err;

		/* TIMEOUT SUPPORT: Issues a blocking wait which will exit after MME_TRANSFORMER_TIMEOUT */
		err = EMBX_ReceiveBlockTimeout(replyPort, &event, MME_TRANSFORMER_TIMEOUT);
		
		if (EMBX_SUCCESS != err) {
			res = ((err == EMBX_SYSTEM_TIMEOUT) ? MME_TRANSFORMER_NOT_RESPONDING : MME_EMBX_ERROR);
			
			goto error_recovery_createport;
		}
	}

	message = event.data;
	MME_Assert(event.type == EMBX_REC_MESSAGE);
	MME_Assert(event.size >= messageSize);
	MME_Assert(message->id == TMESSID_CAPABILITY);
	MME_Assert(message->messageSize == messageSize);

	/* fill in the rest of the message */
	res = message->result;
	message->capability.TransformerInfo_p = capability->TransformerInfo_p;
	*capability = message->capability;
	memcpy(capability->TransformerInfo_p, message+1, capability->TransformerInfoSize);

	/* fall into the clean up code */

    error_recovery_createport:
	(void) EMBX_ClosePort(replyPort);

    error_recovery_alloc:
	if (res != MME_SUCCESS)
		/* TIMEOUT SUPPORT: Don't release dead buffers to the cache as they may have had
		 * their buf->htp mangled making them useless
		 */
		(void) EMBX_Release(remoteTransformer->super.info->handle, message);
	else
		(void) freeBuffer(remoteTransformer, message);

	return res;
}

/*******************************************************************/
#define DEBUG_FUNC_STR  "RemoteTransformer_SendCommand"
static MME_ERROR RemoteTransformer_SendCommand(Transformer_t* transformer, MME_Command_t * commandInfo, CommandSlot_t** slot)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	EMBX_VOID *buffer;
	EMBX_ERROR err;
	MME_CommandStatus_t *status;
	TransformerTransformMessage *message;
	MME_ERROR result;
	int i;
	MME_CommandCode_t cmdCode = commandInfo->CmdCode;
	int newCommandIndex;

	if (!remoteTransformer->sendPort.valid) {
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "MME port does not exist\n"));
		return MME_INVALID_ARGUMENT;
	}

	status = (MME_CommandStatus_t*) &(commandInfo->CmdStatus);

	MME_Assert(cmdCode == MME_TRANSFORM || cmdCode == MME_SEND_BUFFERS || 
	           cmdCode == MME_SET_GLOBAL_TRANSFORM_PARAMS);

	/* Acquire lock on the transformer's command slots */
	EMBX_OS_MUTEX_TAKE(&remoteTransformer->commandSlotLock);

	if (remoteTransformer->numCommandSlots >= remoteTransformer->maxCommandSlots) {
		/* no free slot */
		EMBX_OS_MUTEX_RELEASE(&remoteTransformer->commandSlotLock);
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "SendCommand: maxCommandSlots overflow\n"));
		return MME_NOMEM;
	}

	/* Find a free command slot so we can memorize parameters */
	for (i = 0; 
	     i < remoteTransformer->maxCommandSlots && 
	         remoteTransformer->commandSlots[remoteTransformer->commandIndex].status 
				!= (CommandStatus_t)MME_COMMAND_COMPLETED_EVT; 
	     i++) {

		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "RemoteTransformer: SendCommand: CommandIndex %d is already used\n", remoteTransformer->commandIndex));

		remoteTransformer->commandIndex++;
		if (remoteTransformer->commandIndex >= remoteTransformer->maxCommandSlots) {
			remoteTransformer->commandIndex = 0;
		}
	}

	if (i >= remoteTransformer->maxCommandSlots) {
		EMBX_OS_MUTEX_RELEASE(&remoteTransformer->commandSlotLock);
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "SendCommand error: No free command slot found!\n"));
		return MME_NOMEM;
	}
	newCommandIndex = remoteTransformer->commandIndex;

	/* Generate a unique command ID per transformer */
	status->CmdId = MME_CMDID_PACK(
			(remoteTransformer->super.handle)-MME_TRANSFORMER_HANDLE_PREFIX,
		        newCommandIndex, &(remoteTransformer->commandIDCounter));

	MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "RemoteTransformer: SendCommand: code=%d CmdId=0x%x slot=%d\n", 
	   cmdCode, status->CmdId, newCommandIndex));

	/* In order to avoid sending buffers, but then being unable to allocate the transform message,
	   we create and fill in the transform message first
	*/
	result = createTransformMessage(remoteTransformer, commandInfo, &buffer);
	if (result != MME_SUCCESS) {
		EMBX_OS_MUTEX_RELEASE(&remoteTransformer->commandSlotLock);
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "CreateTransformMsg() failed, result=%d\n", result));
		return MME_NOMEM;
	}
	/* Setting the command state to FAILED allows us to identify incomplete SendCommand()s
	   at our receiver task.
	*/
	status->State = MME_COMMAND_FAILED;	

	/* New command slot entry. Initially, assume that the transform message can't be sent */
	status->Error = MME_SUCCESS;
	remoteTransformer->commandSlots[newCommandIndex].endType   = commandInfo->CmdEnd;
	/* In the moment that the status is not zero, the receiver task regards it as valid */
	remoteTransformer->commandSlots[newCommandIndex].command = commandInfo;
	remoteTransformer->commandSlots[newCommandIndex].waitor = NULL;
	remoteTransformer->commandSlots[newCommandIndex].status = MME_RUNNING;
	/* TIMEOUT SUPPORT: Save the transform buffer address in case this transform never completes */
	remoteTransformer->commandSlots[newCommandIndex].message = buffer;

	remoteTransformer->numCommandSlots++;

	MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "RemoteTransformer: numCommandSlots %d slotIndex %d\n",
					remoteTransformer->numCommandSlots, remoteTransformer->commandIndex));

	EMBX_OS_MUTEX_RELEASE(&remoteTransformer->commandSlotLock);

	/* TODO: Should we move the state here or on the companion once the command is
	 * scheduled - I think the latter 
	 */
	status->State = MME_COMMAND_EXECUTING;

	message = (TransformerTransformMessage *) buffer;

	/* Send the message */
	err = EMBX_SendMessage(remoteTransformer->sendPort.port, message, message->messageSize);
	if (err != EMBX_SUCCESS) {
		/* The message could not be sent. Mark the command slot as incomplete */
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "EMBX_SendMessage() failed, error=%d\n", err));
		/* Mark incomplete command */
		status->State = MME_COMMAND_FAILED;	
		freeBuffer(remoteTransformer, buffer);
		return MME_NOMEM;
	} else {
		/* Lost ownership */
		buffer = NULL;
	}

	*slot = &(remoteTransformer->commandSlots[newCommandIndex]);

	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/
#define DEBUG_FUNC_STR "RemoteTransformer_AbortCommand"
static MME_ERROR RemoteTransformer_AbortCommand(Transformer_t* transformer, MME_CommandId_t commandId)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	EMBX_ERROR err;
	EMBX_VOID *buffer;
	TransformerAbortMessage *message;
	CommandSlot_t *slot;

	/* TODO: shouldn't this be asserted! */
	if (!remoteTransformer->sendPort.valid) {
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "MME port does not exist\n"));
		return MME_DRIVER_NOT_INITIALIZED;
	}

	/* confirm that the command is actually pending on the companion before we
	 * issue the abort */
	slot = &(remoteTransformer->commandSlots[MME_CMDID_GET_COMMAND(commandId)]);
	if (slot->status != MME_RUNNING || commandId != slot->command->CmdStatus.CmdId) { 
		return MME_INVALID_ARGUMENT;
	}

	/* Allocate an abort message and send it to the companion side, which disposes the buffer */
	buffer = allocBuffer(remoteTransformer, sizeof(TransformerAbortMessage));
	if (0 == buffer) {
		return MME_NOMEM;
	}

	/* initialize the message */
	message = (TransformerAbortMessage *) buffer;
	message->id = TMESSID_ABORT;
	message->messageSize = sizeof(TransformerAbortMessage);
	message->commandId = commandId;

	/* post the message */
	err = EMBX_SendMessage(remoteTransformer->sendPort.port, message, message->messageSize);
	if (err != EMBX_SUCCESS) {
		/* If sending the message did not work, there is nothing we can do. */
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "EMBX_SendMessage(TransformerAbortMessage) failed, error=%d\n", err));
		freeBuffer(remoteTransformer, buffer);
		return MME_NOMEM;
	}
	
	/* success does not indicate the command has been aborted only
	 * that the request has been sent
	 */
	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/
#define DEBUG_FUNC_STR "RemoteTransformer_KillCommand"
static MME_ERROR RemoteTransformer_KillCommand(Transformer_t* transformer, MME_CommandId_t commandId)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	MME_ERROR result;
	EMBX_ERROR err;
	CommandSlot_t *slot;
	EMBX_VOID *buffer;
	TransformerTransformMessage *message;
	EMBX_PORT replyPort;

	/* TODO: shouldn't this be asserted! */
	if (!remoteTransformer->sendPort.valid) {
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_ERROR_STR "MME port does not exist\n"));
		return MME_DRIVER_NOT_INITIALIZED;
	}

	/* Acquire lock on the transformer's command slots */
	EMBX_OS_MUTEX_TAKE(&remoteTransformer->commandSlotLock);

	/* confirm that the command is actually pending on the companion before we
	 * issue the abort */
	slot = &(remoteTransformer->commandSlots[MME_CMDID_GET_COMMAND(commandId)]);
	if (slot->status != MME_RUNNING || commandId != slot->command->CmdStatus.CmdId) { 
		EMBX_OS_MUTEX_RELEASE(&remoteTransformer->commandSlotLock);
		return MME_INVALID_ARGUMENT;
	}

	MME_Info(MME_INFO_TRANSFORMER, 
		 (DEBUG_NOTIFY_STR "CmdId 0x%x slot %p command %p\n",
		  commandId, slot, slot->command));

	/* mark the command as complete */
	slot->command->CmdStatus.Error = MME_COMMAND_ABORTED;
	slot->command->CmdStatus.State = MME_COMMAND_FAILED;

	/* TIMEOUT SUPPORT: Free off the failed transform buffer */
	MME_Assert(slot->message);
	(void) EMBX_Release(remoteTransformer->super.info->handle, slot->message);
	slot->message = NULL;

	EMBX_OS_MUTEX_RELEASE(&remoteTransformer->commandSlotLock);

	/* fake up a completed transform request */
	result = createTransformMessage(remoteTransformer, slot->command, &buffer);
	if (result != MME_SUCCESS) {
		MME_Info(MME_INFO_TRANSFORMER, 
			 (DEBUG_NOTIFY_STR "Failed to create transform message %d\n", result));
		return result;
	}

	/* connect to our own reply port */
	err = EMBX_Connect(remoteTransformer->super.info->handle, remoteTransformer->replyPort.name, &replyPort);
	if (EMBX_SUCCESS != err) {
		MME_Info(MME_INFO_TRANSFORMER, 
			 (DEBUG_NOTIFY_STR "Failed to connect to reply port %d\n", err));
		return MME_EMBX_ERROR;
	}

	/* send the message */
	message = (TransformerTransformMessage *) buffer;
	err = EMBX_SendMessage(replyPort, message, message->messageSize);
	if (EMBX_SUCCESS != err) {
		MME_Info(MME_INFO_TRANSFORMER, 
			 (DEBUG_NOTIFY_STR "Failed to SendMessage %d\n", err));
		EMBX_ClosePort(replyPort);
		return MME_EMBX_ERROR;
	}

	EMBX_ClosePort(replyPort);
	
	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/
#define DEBUG_FUNC_STR "RemoteTransformer_KillCommandAll"
static MME_ERROR RemoteTransformer_KillCommandAll(Transformer_t* transformer)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	int i;
	MME_ERROR res = MME_SUCCESS;

	/* Find and kill all outstanding commands for this transformer */
	for (i = 0; i < remoteTransformer->maxCommandSlots; i++) {
		CommandSlot_t *slot = &(remoteTransformer->commandSlots[i]);
		
		if (slot->status != MME_RUNNING)
			continue;

		MME_Info(MME_INFO_TRANSFORMER, 
			 (DEBUG_NOTIFY_STR "slot %p[%d] status %d command %p state %d\n",
			  slot, i, slot->status, slot->command, (slot->command ? slot->command->CmdStatus.State : -1)));

		if (slot->command && slot->command->CmdStatus.State == MME_COMMAND_EXECUTING) {
			MME_ERROR result;
			MME_CommandId_t commandId = slot->command->CmdStatus.CmdId;
			
			result = RemoteTransformer_KillCommand(transformer, commandId);
			
			MME_Info(MME_INFO_TRANSFORMER, 
				 (DEBUG_NOTIFY_STR "Kill CommandIndex %d CmdId 0x%x = %d\n",
				  i, commandId, result));

			/* Save any failure results */
			if (result != MME_SUCCESS)
				res = result;
		}
	}

	return res;
}

/*******************************************************************/

static MME_ERROR RemoteTransformer_IsStillAlive(Transformer_t* transformer, MME_UINT* alive)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	MME_ERROR res = MME_NOMEM;

	EMBX_ERROR err;
	EMBX_VOID *buffer;
	TransformerIsAliveMessage *message;

	MME_Assert(remoteTransformer);

	if (!remoteTransformer->sendPort.valid) {
		DP("MME port does not exist\n");
		return MME_INVALID_ARGUMENT;
	}

	res = createIsAliveMessage(remoteTransformer, &buffer);
	if (res != MME_SUCCESS) {
		DP("CreateIsAliveMsg() failed, error=%d\n", res);
		return res;
	}

	/* Send the message */
	message = (TransformerIsAliveMessage *) buffer;
	DP("Sending IsAlive message @$%08x...\n", message);

	err = EMBX_SendMessage(remoteTransformer->adminPort, message, message->messageSize);
	if (EMBX_SUCCESS != err) {
		DP("RemoteTransformer_IsStillAlive(): failed to send message to coprocessor\n");
		cleanupIsAliveMessage(remoteTransformer, message, EMBX_TRUE);
		return MME_EMBX_ERROR;	
	}

	/* lost ownership */
	message = NULL;

	/* Wait for isAlive response via the receiver thread */
	/* TIMEOUT SUPPORT: Allow wait to timeout */
	res = EMBX_OS_EVENT_WAIT_TIMEOUT(&remoteTransformer->isAliveWasReplied, MME_TRANSFORMER_TIMEOUT);
	
	DP("isAliveWasReplied wait buffer %p res=%d\n", buffer, res);
	
	if (res)
	{
		/* TIMEOUT SUPPORT: Assume companion is dead and tidy up */
		cleanupIsAliveMessage(remoteTransformer, (TransformerIsAliveMessage *) buffer, EMBX_TRUE);
		
		/* He's dead Jim */
		*alive = 0;
		
		return res;
	}
	
	/* It's alive! */
	*alive = 1;

	return res;
}

/*******************************************************************/

static MME_ERROR RemoteTransformer_ReleaseCommandSlot(Transformer_t* transformer, CommandSlot_t* slot)
{
	RemoteTransformer_t *remoteTransformer = (RemoteTransformer_t *) transformer;
	EMBX_OS_MUTEX_TAKE(&remoteTransformer->commandSlotLock);

	MME_Assert(slot->status == MME_RUNNING);
	MME_Assert(remoteTransformer->numCommandSlots > 0);

	remoteTransformer->numCommandSlots--;
	slot->status = MME_COMMAND_COMPLETED_EVT;

	EMBX_OS_MUTEX_RELEASE(&remoteTransformer->commandSlotLock);

	return MME_SUCCESS;
}

/*******************************************************************
 * Receiver thread functions
 *******************************************************************/

#ifdef ENABLE_MME_WAITCOMMAND
static void unblockWaitors(RemoteTransformer_t* remoteTransformer, MME_Event_t event, CommandSlot_t* command) 
{
	HostManager_t* manager = MMEGetManager();
	
	/* Lock whilst this thread does event notification */
	MME_Assert(manager);

	EMBX_OS_MUTEX_TAKE(&manager->eventLock);

	manager->eventTransformer = remoteTransformer;
	manager->eventCommand     = command;
	manager->eventCommandType = event;

	EMBX_OS_EVENT_POST(command->waitor);

	EMBX_OS_EVENT_WAIT(&manager->waitorDone);

	EMBX_OS_MUTEX_RELEASE(&manager->eventLock);
}
#endif

/*******************************************************************/

Transformer_t* MME_RemoteTransformer_Create(HostManager_t *manager, TransportInfo_t* info, EMBX_PORT adminPort)
{
	static const TransformerVTable_t vtable = {
	        RemoteTransformer_Destroy,
	        RemoteTransformer_Init,
	        RemoteTransformer_Term,
	        RemoteTransformer_Kill,
	        RemoteTransformer_AbortCommand,
		RemoteTransformer_KillCommand,
		RemoteTransformer_KillCommandAll,
	        RemoteTransformer_SendCommand,
	        RemoteTransformer_IsStillAlive,
	        RemoteTransformer_GetTransformerCapability,
		RemoteTransformer_WaitForMessage,
		RemoteTransformer_LookupCommand,
		RemoteTransformer_ReleaseCommandSlot
	};

	RemoteTransformer_t* remoteTransformer;
	int i;
	
	remoteTransformer = EMBX_OS_MemAlloc(sizeof(RemoteTransformer_t));
	if (!remoteTransformer) {
		return NULL;
	}

	/* zero the structure */
	memset(remoteTransformer, 0, sizeof(RemoteTransformer_t));

	/* initialize the non-zero elements */
	remoteTransformer->super.vtable     = &vtable;
	remoteTransformer->super.manager    = manager;
	remoteTransformer->super.info       = info;
	remoteTransformer->adminPort        = adminPort;
	remoteTransformer->maxCommandSlots  = MAX_TRANSFORMER_SEND_OPS;

	/* allocate the command slots (used to keep track of in-flight commands) */
	remoteTransformer->commandSlots = EMBX_OS_MemAlloc(sizeof(CommandSlot_t) *
							   remoteTransformer->maxCommandSlots);
	if (0 == remoteTransformer->commandSlots) {
		goto error_recovery;
	}
	
	for (i=0; i<remoteTransformer->maxCommandSlots; i++) {
		remoteTransformer->commandSlots[i].command = 0;
		remoteTransformer->commandSlots[i].status = MME_COMMAND_COMPLETED_EVT;
	}

	if (!EMBX_OS_EVENT_INIT(&remoteTransformer->terminateWasReplied)) {
		goto error_recovery;
	}

	if (!EMBX_OS_EVENT_INIT(&remoteTransformer->isAliveWasReplied)) {
		EMBX_OS_EVENT_DESTROY(&remoteTransformer->terminateWasReplied);	
		goto error_recovery;
	}
	
	if (!EMBX_OS_MUTEX_INIT(&remoteTransformer->commandSlotLock)) {
		EMBX_OS_EVENT_DESTROY(&remoteTransformer->terminateWasReplied);	
		EMBX_OS_EVENT_DESTROY(&remoteTransformer->isAliveWasReplied);	
		goto error_recovery;
	}
	
	/* reference the transport info structure (so it can't be deregistered) */
	info->referenceCount++;
	
	return &remoteTransformer->super;
	
    error_recovery:
    	
    	/* EMBX_OS_MemFree() will ignore NULL pointers */
    	EMBX_OS_MemFree(remoteTransformer->commandSlots);
    	EMBX_OS_MemFree(remoteTransformer);

    	return NULL;
}



/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
