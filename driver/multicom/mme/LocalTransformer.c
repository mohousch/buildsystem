/*
 * LocalTransformer.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Implementation of host implemented transformers.
 */

#include "mme_hostP.h"

#if defined __LINUX__ && !defined __KERNEL__
/* Until we have EMBX built in user space */
#include <stdlib.h>
#include <string.h>
#define EMBX_OS_MemAlloc(X)   (malloc(X))
#define EMBX_OS_MemFree(X)    (free(X))
#endif  

/*******************************************************************/

struct LocalTransformer_s {
	Transformer_t super;

	RegisteredLocalTransformer_t *rlt;
	LocalTransformerVTable_t vtable;
	void *contextPointer;

	MMEMessageQueue *messageQueue;
	EMBX_EVENT *commandEvent;

	int commandsInProgress;
	int commandIdCounter;

	MMELookupTable_t* commandLookupTable;
	CommandSlot_t*    commandWaitData;

	EMBX_EVENT*       callbackEvent;
	EMBX_EVENT*       callbackEventHandled;

	MME_Event_t       event;
	MME_Command_t*    command;
};

/*******************************************************************/

static MME_ERROR LocalTransformer_ReleaseCommandSlot(Transformer_t *hostTransformer, CommandSlot_t* slot);

/*******************************************************************/

#ifdef ENABLE_MME_WAITCOMMAND
static void unblockWaitors(LocalTransformer_t* transformer, MME_Event_t event, CommandSlot_t* command) 
{
	/* Lock whilst this thread does event notification */
	HostManager_t* manager = MMEGetManager();
	MME_Assert(manager);

	EMBX_OS_MUTEX_TAKE(&manager->eventLock);

	manager->eventTransformer = transformer;
	manager->eventCommand     = command;
	manager->eventCommandType = event;

	EMBX_OS_EVENT_POST(command->waitor);

	EMBX_OS_EVENT_WAIT(&manager->waitorDone);

	EMBX_OS_MUTEX_RELEASE(&manager->eventLock);
}
#endif

/*******************************************************************/

#define DEBUG_FUNC_STR   "LocalTransformer_Destroy"
static void LocalTransformer_Destroy(Transformer_t* tf)
{
	LocalTransformer_t *transformer = (LocalTransformer_t *) tf;

	if (transformer->callbackEventHandled) {
		MME_Assert(transformer->callbackEvent);

		/* wait for termination to be sent to the user callback thread */
		EMBX_OS_EVENT_WAIT(transformer->callbackEventHandled);

		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Destroying semaphores\n"));
		EMBX_OS_EVENT_DESTROY(transformer->callbackEvent);
		EMBX_OS_EVENT_DESTROY(transformer->callbackEventHandled);
		EMBX_OS_MemFree(transformer->callbackEvent);
		EMBX_OS_MemFree(transformer->callbackEventHandled);
	} else {
		MME_Assert(!transformer->callbackEvent);
	}

	/* unreference the rlt structure */
	transformer->rlt->inUseCount--;

	MME_LookupTable_Delete(transformer->commandLookupTable);
	EMBX_OS_MemFree(transformer->commandWaitData);
	EMBX_OS_MemFree(transformer);
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR   "LocalTransformer_Init"
static MME_ERROR LocalTransformer_Init(Transformer_t* hostTransformer, 
		const char* name, MME_TransformerInitParams_t * params, 
		MME_TransformerHandle_t handle, EMBX_BOOL doCallbacks)
{
	LocalTransformer_t *transformer = (LocalTransformer_t *) hostTransformer;
	MME_ERROR res;

	transformer->super.initParams = *params;
	transformer->super.handle = handle;

	if (doCallbacks) {
		transformer->callbackEvent = transformer->callbackEventHandled = NULL;
        } else {
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Initializing callback events\n"));
		transformer->callbackEvent = 
			EMBX_OS_MemAlloc(sizeof(EMBX_EVENT));
		transformer->callbackEventHandled = 
			EMBX_OS_MemAlloc(sizeof(EMBX_EVENT));

		if (!EMBX_OS_EVENT_INIT(transformer->callbackEvent)) {
			MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Failed to init sem event\n"));
			res = MME_NOMEM;
			goto exit;
		}
		if (!EMBX_OS_EVENT_INIT(transformer->callbackEventHandled)) {
			MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Failed to init sem eventHandled\n"));
			res = MME_NOMEM;
			goto exit;
		}
		/* First time init - ready to update callback data from now */
		EMBX_OS_EVENT_POST(transformer->callbackEventHandled);

        }
	res = MME_ExecutionLoop_Init(transformer->super.manager, params->Priority,
			&(transformer->messageQueue), &(transformer->commandEvent));

	MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "events 0x%08x, 0x%08x, 0x%08x\n", (int)transformer->callbackEvent, (int)transformer->callbackEventHandled, (int)transformer->commandEvent));

	if (MME_SUCCESS != res) {
		return res;
	}

	/* cache part of the rlt structure */
	transformer->vtable = transformer->rlt->vtable;

	res = transformer->vtable.InitTransformer(params->TransformerInitParamsSize, 
	                                           params->TransformerInitParams_p,
						   &transformer->contextPointer);
exit:
        return res;
}
#undef DEBUG_FUNC_STR

static MME_ERROR LocalTransformer_Term(Transformer_t* hostTransformer)
{
	MME_ERROR res;

	LocalTransformer_t *transformer = (LocalTransformer_t *) hostTransformer;

	if (!MME_LookupTable_IsEmpty(transformer->commandLookupTable)) {
		return MME_COMMAND_STILL_EXECUTING;
	}

	MME_Assert(4096 > transformer->commandsInProgress);

	res = transformer->vtable.TermTransformer(transformer->contextPointer);

	if (MME_SUCCESS == res && transformer->callbackEventHandled && transformer->callbackEvent) {
		/* Unblock the callback wait thread - a NULL command indicates 
                   transformer termination */

		EMBX_OS_EVENT_WAIT(transformer->callbackEventHandled);
		transformer->command = NULL;
		EMBX_OS_EVENT_POST(transformer->callbackEvent);       
	}

	return res;
}

/*******************************************************************/

static MME_ERROR LocalTransformer_Kill(Transformer_t* hostTransformer)
{
	/* killing a transformer remains stupid */
	return MME_INVALID_HANDLE;
}

/*******************************************************************/

static MME_ERROR LocalTransformer_AbortCommand(Transformer_t* hostTransformer, MME_CommandId_t commandId)
{
	LocalTransformer_t *transformer = (LocalTransformer_t *) hostTransformer;
	MME_ERROR res;
	int cid = MME_CMDID_GET_COMMAND(commandId);
	MME_Command_t *command;

	/* check if the command is actually running */
	res = MME_LookupTable_Find(transformer->commandLookupTable, cid, (void **) &command);
	if (MME_SUCCESS != res) {
		return MME_INVALID_ARGUMENT;
	}

	/* see if we can yank it off the command queue */	
	res = MME_MessageQueue_RemoveByValue(transformer->messageQueue, command);

	/* if not our last chance is to get the transformer to do it for us */
	if (MME_SUCCESS != res) {
		res = transformer->vtable.AbortCommand(transformer->contextPointer, commandId);
	}

	if (MME_SUCCESS == res) {
		(void) MME_LocalTransformer_NotifyHost(transformer, 
				MME_COMMAND_COMPLETED_EVT, command, MME_COMMAND_ABORTED);
       		return MME_SUCCESS;
	}

	return MME_INVALID_ARGUMENT;
}

static MME_ERROR LocalTransformer_KillCommand(Transformer_t* hostTransformer, MME_CommandId_t commandId)
{
	/* for a local transformer killing a running command is even more
	 * stupid than normal
	 */
	return MME_INVALID_HANDLE;
}

static MME_ERROR LocalTransformer_KillCommandAll(Transformer_t* hostTransformer)
{
	/* for a local transformer killing a running command is even more
	 * stupid than normal
	 */
	return MME_INVALID_HANDLE;
}

#define DEBUG_FUNC_STR   "LocalTransformer_SendCommand"

static MME_ERROR LocalTransformer_SendCommand(Transformer_t* hostTransformer, MME_Command_t * command, CommandSlot_t** slot) 
{
	LocalTransformer_t *transformer = (LocalTransformer_t *) hostTransformer;
	MME_ERROR res;
	int id;
	MME_CommandStatus_t* status;
	
        MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "hostTransformer 0x%08x, command 0x%08x\n", (int)hostTransformer, (int)command));

	MME_Assert(hostTransformer);
	MME_Assert(command);

	/* generate a unique command id */
	res = MME_LookupTable_Insert(transformer->commandLookupTable, command, &id);
	if (MME_SUCCESS != res) {
		return res;
	}

        MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Inserted command - id 0x%08x\n", id ));

	status = (MME_CommandStatus_t*) &(command->CmdStatus);
	status->CmdId = MME_CMDID_PACK(transformer->super.handle, id, &(transformer->commandIdCounter));
	status->State = MME_COMMAND_PENDING;

	transformer->commandWaitData[id].command = command;
	transformer->commandWaitData[id].waitor = NULL;
	transformer->commandWaitData[id].status = MME_RUNNING;
	*slot = &(transformer->commandWaitData[id]);

	switch (command->CmdCode) {
	case MME_SET_GLOBAL_TRANSFORM_PARAMS:
	case MME_TRANSFORM:
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Enqueue transform\n"));

		/* enqueue it onto the local dispatch queue */
		res = MME_MessageQueue_Enqueue(transformer->messageQueue, command, command->DueTime);
		/* can't fail since there are as many command ids as there are queue elements */
		MME_Assert(MME_SUCCESS == res);

		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Signal event transform available 0x%08x\n", (int) transformer->commandEvent ));

		/* finally signal the execution thread to do the actual work */
		EMBX_OS_EVENT_POST(transformer->commandEvent);

		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Signalled event available\n"));

		break;

	case MME_SEND_BUFFERS:
		/* execute this command immediately */
		/* TODO: should we dispatch this to a higher priority execution thread? */

		/* update the command state and process the transform */
		status->State = MME_COMMAND_EXECUTING;
		res = transformer->vtable.ProcessCommand(transformer->contextPointer, command);
	
		if (MME_TRANSFORM_DEFERRED != res && MME_SUCCESS != res) {

			/* update the state */
			(void) MME_LocalTransformer_NotifyHost(transformer, 
					MME_COMMAND_COMPLETED_EVT, command, res);
		}
		break;
	
	default:
		MME_Assert(0);
	}


	return MME_SUCCESS;
}

#undef DEBUG_FUNC_STR

static MME_ERROR LocalTransformer_IsStillAlive(Transformer_t* hostTransformer, MME_UINT* alive) 
{
	*alive = 1;
	return MME_SUCCESS;
}


static MME_ERROR LocalTransformer_GetTransformerCapability(
	Transformer_t* hostTransformer, const char* name, 
	MME_TransformerCapability_t* capability)
{
	LocalTransformer_t *transformer = (LocalTransformer_t *) hostTransformer;
	return transformer->rlt->vtable.GetTransformerCapability(capability);
}

static CommandSlot_t* LocalTransformer_LookupCommand(Transformer_t* hostTransformer, int cid)
{
	LocalTransformer_t *transformer = (LocalTransformer_t *) hostTransformer;
	MME_Command_t *command;
	MME_ERROR res;

	/* check if the command is actually running */
	res = MME_LookupTable_Find(transformer->commandLookupTable, cid, (void **) &command);
	if (MME_SUCCESS != res) {
		return NULL;
	}
	if (command != transformer->commandWaitData[cid].command) {
		return NULL;
	}

	return &(transformer->commandWaitData[cid]);
}

static MME_ERROR LocalTransformer_ReleaseCommandSlot(Transformer_t *hostTransformer, CommandSlot_t* slot)
{
	MME_ERROR res;
	LocalTransformer_t *transformer = (LocalTransformer_t *) hostTransformer;
	int commandId = MME_CMDID_GET_COMMAND(slot->command->CmdStatus.CmdId);

	res = MME_LookupTable_Remove(transformer->commandLookupTable, commandId);
	if (MME_SUCCESS != res) {
		return MME_INVALID_ARGUMENT;
	}
	return MME_SUCCESS;
}

/*******************************************************************/

void MME_LocalTransformer_ProcessCommand(LocalTransformer_t *transformer, MME_Command_t *command)
{
	MME_ERROR res;
	MME_CommandStatus_t* status;
	status = (MME_CommandStatus_t*) &(command->CmdStatus);

	/* update the command state and process the transform */
	status->State = MME_COMMAND_EXECUTING;
	res = transformer->vtable.ProcessCommand(transformer->contextPointer, command);
	
	if (MME_TRANSFORM_DEFERRED != res) {
		/* update the state */
		(void) MME_LocalTransformer_NotifyHost(transformer, 
				MME_COMMAND_COMPLETED_EVT, command, res);
	}
}

/****************************************************************
 * Another thread calls this function and is unblocked when an
 * event (or transformer termination) occurs
 * It was provided for Linux user mode support - the i/f
 * between user space callback fn and kernel space
 ****************************************************************/

#define DEBUG_FUNC_STR   "LocalTransformer_WaitForMessage"

static MME_ERROR LocalTransformer_WaitForMessage(Transformer_t*  transformer, 
                                MME_Event_t*    eventPtr,
		                MME_Command_t** commandPtr)
{
       LocalTransformer_t* localTransformer = (LocalTransformer_t*)transformer;

	MME_Assert(eventPtr);
	MME_Assert(commandPtr);

       /* Wait for an event */
       MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Waiting for event signal; eventPtr 0x%08x, commandPtr 0x%08x\n", (int)eventPtr, (int)commandPtr));
       if (EMBX_OS_EVENT_WAIT(localTransformer->callbackEvent)) {
       		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Received OS signal; returning MME_SYSTEM_INTERRUPT\n"));
       		return MME_SYSTEM_INTERRUPT;
       }
       MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Received event signal; event 0x%08x, command 0x%08x\n", (int)*eventPtr, (int)*commandPtr));

       *eventPtr = localTransformer->event;
       *commandPtr = localTransformer->command;

       /* Signal that we have got the data */
       MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Posting handled event\n"));
       EMBX_OS_EVENT_POST(localTransformer->callbackEventHandled);
       MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "Posted handled event\n"));

       /* If the command is null, the transformer is terminating */
       return (*commandPtr ? MME_SUCCESS : MME_TRANSFORMER_NOT_RESPONDING);
}

#undef DEBUG_FUNC_STR 

/*******************************************************************/
#define DEBUG_FUNC_STR   "LocalTransformer_NotifyHost"
MME_ERROR MME_LocalTransformer_NotifyHost(
	LocalTransformer_t *transformer, MME_Event_t event, MME_Command_t *command, MME_ERROR err)
{
	MME_CommandStatus_t* status = (MME_CommandStatus_t*) &(command->CmdStatus);
	int commandId = MME_CMDID_GET_COMMAND(command->CmdStatus.CmdId);
	int doCallback;

	status->Error = err;

	if (MME_COMMAND_COMPLETED_EVT == event) {
		/* update the command state */
		status->State = (MME_SUCCESS == command->CmdStatus.Error ? 
				 MME_COMMAND_COMPLETED : MME_COMMAND_FAILED);
	}

	doCallback = MME_COMMAND_END_RETURN_NOTIFY == command->CmdEnd && transformer->super.initParams.Callback;
	/* Free the command slot 
           - must do this prior to the callback because this thread may not be run for some time 
             and in the meantime the app may attempt to terminate the transformer
           - TO DO should use a mutex instead */

	LocalTransformer_ReleaseCommandSlot((Transformer_t*)transformer, &(transformer->commandWaitData[commandId]));

	MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "doCallback %d\n", doCallback));

	if (transformer->callbackEvent) {
		/* Do not call the callback function directly - but signal another thread
                   which will do it */

		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "waiting for handled sem\n"));

		/* Wait till the waitor is ready */
		EMBX_OS_EVENT_WAIT(transformer->callbackEventHandled);
		transformer->event = event;
		transformer->command = command;
		/* Signal that the waitor can read the event/command data */
		EMBX_OS_EVENT_POST(transformer->callbackEvent);       

		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "posting event avail sem\n"));

	} else {
		/* make the callback */
		MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "explicit callback\n"));
		if (doCallback) {
			MME_Info(MME_INFO_TRANSFORMER, (DEBUG_NOTIFY_STR "explicit callback to 0x%08x\n", (int)transformer->super.initParams.Callback ));

			transformer->super.initParams.Callback(event, command, transformer->super.initParams.CallbackUserData);
		}
        }

	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR 

/*******************************************************************/

Transformer_t* MME_LocalTransformer_Create(HostManager_t *manager, RegisteredLocalTransformer_t *rlt)
{
	static const TransformerVTable_t vtable = {
		LocalTransformer_Destroy,
		LocalTransformer_Init,
		LocalTransformer_Term,
		LocalTransformer_Kill,
		LocalTransformer_AbortCommand,
		LocalTransformer_KillCommand,
		LocalTransformer_KillCommandAll,
		LocalTransformer_SendCommand,
		LocalTransformer_IsStillAlive,
		LocalTransformer_GetTransformerCapability,
		LocalTransformer_WaitForMessage,
		LocalTransformer_LookupCommand,
		LocalTransformer_ReleaseCommandSlot
	};
	
	LocalTransformer_t *transformer = EMBX_OS_MemAlloc(sizeof(LocalTransformer_t));

	if (!transformer) {
		return NULL;
	}

	memset(transformer, 0, sizeof(LocalTransformer_t));
	transformer->super.vtable = &vtable;
	transformer->super.manager = manager;
	transformer->super.info = 0;
	transformer->rlt = rlt;
	transformer->commandLookupTable = MME_LookupTable_Create(32, 0);
	transformer->commandWaitData = EMBX_OS_MemAlloc(32 * sizeof(CommandSlot_t));
	if (!transformer->commandLookupTable || !transformer->commandWaitData) {
		goto error_recovery;
	}
	memset(transformer->commandWaitData, 0, 32 * sizeof(CommandSlot_t));
	
	/* reference the registered transformer so it cannot be deregistered while in use */
	rlt->inUseCount++;

	return &(transformer->super);

    error_recovery:
	
	if (transformer->commandLookupTable) { 
		MME_LookupTable_Delete(transformer->commandLookupTable);
	}

	if (transformer->commandWaitData) {
		EMBX_OS_MemFree(transformer->commandWaitData);
	}

	EMBX_OS_MemFree(transformer);
	return NULL;
}
