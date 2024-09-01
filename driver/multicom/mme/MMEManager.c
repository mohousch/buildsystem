/*
 * MMEManager.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Companion side entry point functions
 */

#include <stdlib.h>

#include "mme_companionP.h"
#include "debug_ctrl.h"

/*******************************************************************/

/* map 5000 -> 4, 4000 -> 3, ..., 1000 -> 0 without using decimal division */
#define MME_PRIORITY_TO_ID(pri) (pri >> 10)

/*******************************************************************/

static MMEManager_t* manager;

static MME_ERROR createExecutionLoop(MMEManager_t *manager, int id);

/*******************************************************************/

static void receiveInitMessage(TransformerInitMessage * message)
{
	EMBX_ERROR err;
	EMBX_PORT replyPort;
	MMEReceiverFactory *factory;
	MMEReceiver *receiver;
	MME_ERROR res;

	message->result = MME_NOMEM;	/* preliminary result */

	/* Connect to the reply port. */
	err = EMBX_ConnectBlock(manager->transportHandle, message->portName, &replyPort);
	if (err != EMBX_SUCCESS) {
		/* ??? This is a bit of a problem. We cannot connect to the port that we should now send
		   the error message to. */
		MME_Info(MME_INFO_MANAGER, ("EMBX_ConnectBlock() to reply port failed, error=%d\n", err));
		return;
	}

	/* Need a lock on this list */
	/* Find the right receiver factory. */
	factory = manager->factoryList;
	while (factory != NULL && strcmp(factory->transformerType, message->transformerType) != 0) {
		factory = factory->next;
	}

	if (factory == NULL) {
		/* No such factory available. */
		MME_Info(MME_INFO_MANAGER, ("No receiver factory found\n"));
		res = MME_UNKNOWN_TRANSFORMER;
	} else {
		/* Create a receiver object. This will result in the constructors being called as well. */
		res = MME_Receiver_Create(factory, &receiver);
		if (MME_SUCCESS == res) {
			int id = MME_PRIORITY_TO_ID(message->priority);

			res = createExecutionLoop(manager, id);
			MME_Assert(MME_SUCCESS == res); /* TODO: proper error handling */

#ifdef EMBX_RECEIVE_CALLBACK
			/* Initialize the receiver. */
			/* This call will also fail if an existing transformer does not support multi-instantiation. */
			res = MME_Receiver_Init_Callback(receiver, manager, manager->transportHandle, message, replyPort,
						         &(manager->commandEvents[id]), factory->createThread);
#else
			/* Initialize the receiver. */
			/* This call will also fail if an existing transformer does not support multi-instantiation. */
			res = MME_Receiver_Init(receiver, manager, manager->transportHandle, message, replyPort,
						         &(manager->commandEvents[id]));
#endif /* EMBX_RECEIVE_CALLBACK */
			if (res == MME_SUCCESS) {
				/* Add the new receiver to the list. */
				EMBX_OS_MUTEX_TAKE(&(manager->receiverListMutexes[id]));

				receiver->next = manager->receiverLists[id];
				manager->receiverLists[id] = receiver;

				EMBX_OS_MUTEX_RELEASE(&(manager->receiverListMutexes[id]));

				/* We are successfully done. */
				message->result = res;
				EMBX_SendMessage(replyPort, message, message->messageSize);
				return;
			}

			/* Could not initialize receiver. Clean up and return error. */
			MME_Receiver_Destroy(receiver);
		}
	}

	/* Send the message back to the host with an error. */
	message->result = res;
	EMBX_SendMessage(replyPort, message, message->messageSize);
	EMBX_ClosePort(replyPort);
}

/*******************************************************************/

static void receiveTerminateMessage(TransformerTerminateMessage * message)
{
	EMBX_PORT replyPort = EMBX_INVALID_HANDLE_VALUE;
	EMBX_ERROR err;
	MME_ERROR res;
	MMEReceiver **prev = NULL, *receiver = NULL;
	int id;

	/* Search each receiver list for a matching handle */
	for (id=0; id<MME_NUM_EXECUTION_LOOPS; id++) {
		if (manager->loopTasksRunning[id]) {
			EMBX_OS_MUTEX_TAKE(&(manager->receiverListMutexes[id]));

			/* iterate of the receiver list keeping track of the linkage pointer */
			for (prev = &(manager->receiverLists[id]), receiver = *prev;
			     NULL != receiver;
			     prev = &(receiver->next), receiver = *prev) {
				if (receiver == (void *) (message->mmeHandle)) {
					/* even Java lets you use labelled jumps to do this ... */
					goto double_break;
				}
			}

			EMBX_OS_MUTEX_RELEASE(&(manager->receiverListMutexes[id]));
		} else {
			MME_Assert(0 == manager->loopTasks[id]);
			MME_Assert(0 == manager->receiverLists[id]);
		}

	}
    double_break:

        /* WARNING: if receiver is non-NULL we still own receiverListMutexes[id] */

	if (NULL != receiver) {
		replyPort = MME_Receiver_GetReplyPort(receiver);

		/* Terminate the receiver. */
		res = MME_Receiver_Term(receiver);
		if (res == MME_SUCCESS) {
			/* remove the receiver from the list */
			*prev = receiver->next;

			/* destruct and free the receiver. */
			MME_Receiver_Destroy(receiver);
		}

		EMBX_OS_MUTEX_RELEASE(&(manager->receiverListMutexes[id]));
	} else {
		MME_Info(MME_INFO_MANAGER, ("receiveTerminateMessage could not find receiver, no reply port available!\n"));
		res = MME_INVALID_HANDLE;
	}

	/* send the message back to the host with the appropriate return code */
	message->result = res;
	err = EMBX_SendMessage(replyPort, message, message->messageSize);
	MME_Assert(EMBX_SUCCESS == err); /* cannot recover */
	if (MME_SUCCESS == res) {
		err = EMBX_ClosePort(replyPort); /* cannot recover */
		MME_Assert(EMBX_SUCCESS == err);
	}
}

static void receiveIsAliveMessage(TransformerIsAliveMessage * message)
{
	EMBX_PORT replyPort = EMBX_INVALID_HANDLE_VALUE;
	EMBX_ERROR err;

	MMEReceiver **prev = NULL, *receiver = NULL;
	int id;

	/* Search each receiver list for a matching handle */
	for (id=0; id<MME_NUM_EXECUTION_LOOPS; id++) {
		if (manager->loopTasksRunning[id]) {
			EMBX_OS_MUTEX_TAKE(&(manager->receiverListMutexes[id]));

			/* iterate of the receiver list keeping track of the linkage pointer */
			for (prev = &(manager->receiverLists[id]), receiver = *prev;
			     NULL != receiver;
			     prev = &(receiver->next), receiver = *prev) {
				if (receiver == (void *) (message->mmeHandle)) {
					/* even Java lets you use labelled jumps to do this ... */
					goto double_break;
				}
			}

			EMBX_OS_MUTEX_RELEASE(&(manager->receiverListMutexes[id]));
		} else {
			MME_Assert(0 == manager->loopTasks[id]);
			MME_Assert(0 == manager->receiverLists[id]);
		}

	}
    double_break:

        /* WARNING: if receiver is non-NULL we still own receiverListMutexes[id] */

	if (NULL != receiver) {
		replyPort = MME_Receiver_GetReplyPort(receiver);
		
		EMBX_OS_MUTEX_RELEASE(&(manager->receiverListMutexes[id]));

		/* send the message back to the host */
		err = EMBX_SendMessage(replyPort, message, message->messageSize);
		MME_Assert(EMBX_SUCCESS == err); /* cannot recover */
	} else {
		MME_Info(MME_INFO_MANAGER, ("receiveIsAliveMessage could not find receiver, no reply port available!\n"));
	}
}

/*******************************************************************/

static void receiveCapabilityMessage(TransformerCapabilityMessage *message)
{
	EMBX_ERROR err;
	EMBX_PORT replyPort;
	MMEReceiverFactory *factory;

	message->result = MME_NOMEM; /* preliminary result */

	/* connect to the reply port */
	if (EMBX_SUCCESS != EMBX_Connect(manager->transportHandle, message->portName, &replyPort)) {
		MME_Assert(0); /* cannot recover */
		return;
	}

	/* TODO: no lock on this list */
	for (factory = manager->factoryList; factory; factory = factory->next) {
		if (0 == strcmp(factory->transformerType, message->transformerType)) {
			break;
		}
	}

	message->capability.TransformerInfo_p = (void *) 
		(message->capability.TransformerInfoSize ? message + 1 : 0);
	message->result = (factory ? factory->getTransformerCapabilityFunc(&message->capability)
	                           : MME_UNKNOWN_TRANSFORMER);

	/* post the reply back again */
	err = EMBX_SendMessage(replyPort, message, message->messageSize);
	MME_Assert(EMBX_SUCCESS == err); /* cannot recover */

	err = EMBX_ClosePort(replyPort);
	MME_Assert(EMBX_SUCCESS == err); /* cannot recover */
}

static void receiveTransformerRegisteredMessage(TransformerRegisteredMessage* message)
{
	EMBX_PORT replyPort;
	EMBX_ERROR embxError;
	MMEReceiverFactory* factory;

	message->result = MME_SUCCESS;	/* preliminary result */

	/* Connect to the reply port. */
	embxError = EMBX_ConnectBlock(manager->transportHandle, message->portName, &replyPort);
	if (EMBX_SUCCESS != embxError) {
		/* ??? This is a bit of a problem. We cannot connect to the port that we should now send
		   the error message to. */
		MME_Info(MME_INFO_MANAGER, ("EMBX_ConnectBlock() to reply port failed, error=%d\n", embxError));
		return;
	}

	/* Need a lock on this list */
	/* Find the right receiver factory. */
	factory = manager->factoryList;
	while (factory &&
	       factory->transformerType &&
	       strcmp(factory->transformerType, message->transformerType) != 0) {
		factory = factory->next;
	}

	if (factory == NULL) {
		/* No such factory available. */
		MME_Info(MME_INFO_MANAGER, ("No receiver found for %s\n", message->transformerType));
		message->result = MME_UNKNOWN_TRANSFORMER;
	}

	/* Send the message back with the updated result field */
	embxError = EMBX_SendMessage(replyPort, message, message->messageSize);
	if (EMBX_SUCCESS != embxError) {
		MME_Info(MME_INFO_MANAGER, ("EMBX_SendMessage() to reply port failed, error=%d\n", embxError));
		return;
	}

	EMBX_ClosePort(replyPort);
}


/* The task entry point for the manager's execution loop task. */

static void executionLoopTask(void *params)
{
	int id = (int) params;

	for (EMBX_OS_EVENT_WAIT(&(manager->commandEvents[id]));
	     manager->loopTasksRunning[id];
	     EMBX_OS_EVENT_WAIT(&(manager->commandEvents[id]))) {

		MMEReceiver *receiver;
		MMEReceiver *lowestReceiver;
		MME_Time_t dueTime, lowestDueTime;
		int found = 0;

		EMBX_OS_MUTEX_TAKE(&(manager->receiverListMutexes[id]));

		EMBX_OS_MUTEX_TAKE(&(manager->abortMutex));

		/* Find the receiver with the lowest dueTime. */
		lowestReceiver = manager->receiverLists[id];
		lowestDueTime = MME_MaxTime_c;
		for (receiver = lowestReceiver; receiver; receiver = receiver->next) {
			if (MME_SUCCESS == MME_Receiver_NextDueTime(receiver, &dueTime)) {
				if (dueTime < lowestDueTime) {
					found = 1;
					lowestReceiver = receiver;
					lowestDueTime = dueTime;
				}
			}
		}

		/* TODO: Note that if we support the bufferQueue, we need to serve it here */
		if (found) {
			EMBX_OS_MUTEX_RELEASE(&(manager->receiverListMutexes[id]));
			/* Execute this receiver's command. */
			/* Mutex will be released once the message is dequeued - otherwise
			   an abort requiest may come take the message off the queue
                           but leave the receiver in the executing state 
                        */
			MME_Receiver_ProcessCommand(lowestReceiver);
		} else {
			/* Nothing is pending. We could waste some time. */
			MME_Info(MME_INFO_MANAGER, ("Execution loop error: Semaphore signalled but no command found!\n"));
			EMBX_OS_MUTEX_RELEASE(&(manager->receiverListMutexes[id]));

			EMBX_OS_MUTEX_RELEASE(&(manager->abortMutex));
		}
	}
}

static MME_ERROR createExecutionLoop(MMEManager_t *manager, int id)
{
	const struct { MME_UINT priority; const char *name; } lookup[MME_NUM_EXECUTION_LOOPS] = {
		{ MME_TUNEABLE_EXECUTION_LOOP_LOWEST_PRIORITY,       "ExecutionLoopLowest" },
		{ MME_TUNEABLE_EXECUTION_LOOP_BELOW_NORMAL_PRIORITY, "ExecutionLoopBelowNormal" },
		{ MME_TUNEABLE_EXECUTION_LOOP_NORMAL_PRIORITY,       "ExecutionLoopNormal" },
		{ MME_TUNEABLE_EXECUTION_LOOP_ABOVE_NORMAL_PRIORITY, "ExecutionLoopAboveNormal" },
		{ MME_TUNEABLE_EXECUTION_LOOP_HIGHEST_PRIORITY,      "ExecutionLoopHighest" },
	};

	MME_Assert(manager);
	MME_Assert(id < MME_NUM_EXECUTION_LOOPS);

	if (manager->loopTasksRunning[id]) {
		return MME_SUCCESS;
	}

	MME_Assert(0 == manager->loopTasks[id]);
	MME_Assert(0 == manager->receiverLists[id]);
	
	/* this code can only be called from the manager thread and therefore needs
	 * no thread protection.
	 */
	if (!EMBX_OS_MUTEX_INIT(&(manager->receiverListMutexes[id]))) {
		return MME_NOMEM;
	}

	if (!EMBX_OS_EVENT_INIT(&(manager->commandEvents[id]))) {
		goto cleanup_mutex;
	}

	/* spawn the worker thread */
	manager->loopTasks[id] = EMBX_OS_ThreadCreate((void(*)()) executionLoopTask, 
		    (void *) id, MME_GetTuneable(lookup[id].priority), lookup[id].name);
	if (0 == manager->loopTasks[id]) {
		goto cleanup_event;
	}

	/* mark the task as running */
	manager->loopTasksRunning[id] = 1;

	return MME_SUCCESS;

   cleanup_event:
	EMBX_OS_EVENT_DESTROY(&(manager->commandEvents[id]));
	
   cleanup_mutex:
	EMBX_OS_MUTEX_DESTROY(&(manager->receiverListMutexes[id]));

	return MME_NOMEM;
}

static void terminateExecutionLoops(MMEManager_t *mgr)
{
	int i;

	for (i=0; i<MME_NUM_EXECUTION_LOOPS; i++) {
		if (mgr->loopTasksRunning[i]) {
			mgr->loopTasksRunning[i] = 0;
			EMBX_OS_EVENT_POST(&(mgr->commandEvents[i]));
			EMBX_OS_ThreadDelete(mgr->loopTasks[i]);
			EMBX_OS_EVENT_DESTROY(&(mgr->commandEvents[i]));
			EMBX_OS_MUTEX_DESTROY(&(mgr->receiverListMutexes[i]));

			mgr->loopTasks[i] = NULL;
			mgr->receiverLists[i] = NULL;
		}
	}
}

/* API implementations decared in mme.h 
 * Ought to move to a generic source file at some point
 */

MME_ERROR MME_DeregisterTransport(const char *name)
{
	EMBX_ERROR err;
	MME_Info(MME_INFO_MANAGER, ("MME_DeregisterTransport()\n"));
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	if (NULL == name) {
		return MME_INVALID_ARGUMENT;
	}

	MME_Info(MME_INFO_MANAGER, ("EMBX_InvalidatePort() - handle %d\n", manager->adminPort));
	err = EMBX_InvalidatePort(manager->adminPort);
	if (err != EMBX_SUCCESS) {
		MME_Info(MME_INFO_MANAGER, ("EMBX_InvalidatePort() failed, error=%d\n", err));
		return MME_HANDLES_STILL_OPEN;
	}

	MME_Info(MME_INFO_MANAGER, ("EMBX_ClosePort() - handle %d\n", manager->adminPort));
	EMBX_ClosePort(manager->adminPort);
	if (err != EMBX_SUCCESS) {
		MME_Info(MME_INFO_MANAGER, ("EMBX_ClosePort() failed, error=%d\n", err));
		return MME_HANDLES_STILL_OPEN;
	}

	MME_Info(MME_INFO_MANAGER, ("EMBX_CloseTransport() - handle %d\n", manager->transportHandle));
	err = EMBX_CloseTransport(manager->transportHandle);
	if (err != EMBX_SUCCESS) {
		MME_Info(MME_INFO_MANAGER, ("EMBX_CloseTransport() failed, error=%d\n", err));
		return MME_HANDLES_STILL_OPEN;
	}

	return MME_SUCCESS;
}

MME_ERROR MME_RegisterTransport(const char *transportName)
{
	const char portName[] = "MMECompanionAdmin#0";
	EMBX_ERROR err;

	if (NULL == transportName) {
		return MME_INVALID_ARGUMENT;
	}

	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	MME_Info(MME_INFO_MANAGER, ("EMBX_OpenTransport(), name <%s>...\n", transportName));
	/* Open the transport. */
	err = EMBX_OpenTransport(transportName, &manager->transportHandle);
	MME_Info(MME_INFO_MANAGER, ("EMBX_OpenTransport() - handle %d\n", manager->transportHandle));

	if (EMBX_SUCCESS != err) {
		MME_Info(MME_INFO_MANAGER, ("Failed to open EMBX_OpenTransport() - EMBX err %d\n", err));
		return MME_INVALID_ARGUMENT;
	}

	/* Create the administration port. */
	memcpy(manager->adminPortName, portName, sizeof(portName));

	do {
		MME_Info(MME_INFO_MANAGER, ("   EMBX_CreatePort(), port name <%s>...\n", 
					   manager->adminPortName));
		err = EMBX_CreatePort(manager->transportHandle, 
				      manager->adminPortName, &manager->adminPort);
	} while (EMBX_ALREADY_BIND == err && 
	         manager->adminPortName[sizeof(portName)-2]++ < '9');
	if (err != EMBX_SUCCESS) {
		MME_Info(MME_INFO_MANAGER, ("EMBX_CreatePort(%s) failed - error %d\n", manager->adminPortName, err));
		EMBX_CloseTransport(manager->transportHandle);
		manager->transportHandle = 0;
		return MME_INVALID_ARGUMENT;
	}
	MME_Info(MME_INFO_MANAGER, ("EMBX_CreatePort(%s) handle 0x%08x\n", manager->adminPortName, manager->adminPort));

	manager->managerRunning = 1;
	return MME_SUCCESS;
}

MME_ERROR MME_Init(void)
{
	if (manager) {
		return MME_DRIVER_ALREADY_INITIALIZED;
	}

	manager = malloc(sizeof(MMEManager_t));
	if (NULL == manager) {
		return MME_NOMEM;
	}
	memset(manager, 0, sizeof(*manager));

	if (!EMBX_OS_MUTEX_INIT(&(manager->factoryListMutex))) {
		goto error_recovery;
	}

	if (!EMBX_OS_MUTEX_INIT(&(manager->abortMutex))) {
		goto error_recovery;
	}

	return MME_SUCCESS;

    error_recovery:
    	
    	if (manager->factoryListMutex) {
    		EMBX_OS_MUTEX_DESTROY(&(manager->factoryListMutex));
    	}
    	if (manager->abortMutex) {
    		EMBX_OS_MUTEX_DESTROY(&(manager->abortMutex));
    	}
    	EMBX_OS_MemFree(manager);
	
	return MME_NOMEM;
}

MME_ERROR MME_Deinit(void)
{
	EMBX_ERROR err;
	int i;
	
	if (manager == NULL) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	MME_Info(MME_INFO_MANAGER, ("TerminateMMEManager...\n"));

	/* We cannot terminate if there is any receiver 
	 *
	 * TODO: this is not thread-safe (and the receiver list locks are no
	 * good here). we really need request the manager thread perform the
	 * shutdown, the same code would be safe there!
	 */
	for (i=0; i<MME_NUM_EXECUTION_LOOPS; i++) {
		if (NULL != manager->receiverLists[i]) {
			MME_Info(MME_INFO_MANAGER, ("Cannot terminate MMEManager, receiverList isn't empty\n"));
			return MME_COMMAND_STILL_EXECUTING;	/* potentially, this is true */
		}
	}

	/* reap all the execution loop threads */
	terminateExecutionLoops(manager);

	/* TODO: there should normally be no need to reap the thread since it this code
	 * should refuse to run if MME_Run() is still active.
	 */
	manager->managerRunning = 0;

	/* TODO: this cleanup should be delegated to MME_DeregisterTransport() */
	/* Close the administration port. */
	MME_Info(MME_INFO_MANAGER, ("Invalidate adminPort...\n"));
	err = EMBX_InvalidatePort(manager->adminPort);
	MME_Info(MME_INFO_MANAGER, ("Invalidate adminPort...err %d\n", err));

	MME_Info(MME_INFO_MANAGER, ("Close adminPort...\n"));
	err = EMBX_ClosePort(manager->adminPort);
	MME_Info(MME_INFO_MANAGER, ("Close adminPort...err %d\n", err));

	/* Close the transport and EMBX. */
	MME_Info(MME_INFO_MANAGER, ("Close Transport...\n"));
	err = EMBX_CloseTransport(manager->transportHandle);
	MME_Info(MME_INFO_MANAGER, ("EMBX_CloseTransport() - handle %d\n", manager->transportHandle));
	if (err != EMBX_SUCCESS) {
		MME_Info(MME_INFO_MANAGER, ("EMBX_CloseTransport() failed, error=%d\n", err));
	}

	/* Delete the mutex. */
	EMBX_OS_MUTEX_DESTROY(&(manager->factoryListMutex));
	EMBX_OS_MUTEX_DESTROY(&(manager->abortMutex));
	
	EMBX_OS_MemFree(manager);
	manager = NULL;
	return MME_SUCCESS;	
}

/* Run the manager. This call only returns when everything is shut down. */
MME_ERROR MME_Run(void)
{
	MME_ERROR result;
	EMBX_ERROR err;
	EMBX_RECEIVE_EVENT event;
	MME_UINT messageID;
	int running = 1;
	int oldTaskPriority;

	if (manager == NULL) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	/* Note that the manager should be running at high priority (e.g. 200 of 255). */
	oldTaskPriority = task_priority_set(NULL, MANAGER_TASK_PRIORITY);

	while (running && manager->managerRunning) {
		/* Wait for a host request. */
		MME_Info(MME_INFO_MANAGER, ("Waiting for host request... on port 0x%08x\n", manager->adminPort));
		err = EMBX_ReceiveBlock(manager->adminPort, &event);
		if (err == EMBX_SUCCESS) {
			if (event.type == EMBX_REC_MESSAGE && event.size >= sizeof(MME_UINT) && event.data != NULL) {
				/* The message ID decides. */
				messageID = *((MME_UINT*) event.data);
				switch (messageID) {
				case TMESSID_INIT:
					/* It is a TransformerInitMessage. */
					receiveInitMessage((TransformerInitMessage *) event.data);
					break;

				case TMESSID_TERMINATE:
					/* It is a TransformerTerminateMessage to terminate a single transformer. */
					receiveTerminateMessage((TransformerTerminateMessage *) event.data);
					break;

				case TMESSID_CAPABILITY: 
					receiveCapabilityMessage((TransformerCapabilityMessage *) event.data);
					break;
					
				case TMESSID_TERMINATE_MME: {
					/* It is a TransformerTerminateMessage to terminate all of the MME! */
					TransformerTerminateMMEMessage *msg = (TransformerTerminateMMEMessage*) event.data;
					result = MME_Term();
					msg->result = result;
					running = 0;
					/* ??? Problem: We need to tell someone if termination worked, but there's no port to talk to,
					   and EMBX has been shut down as well. */
					break;
				}
				case TMESSID_TRANSFORMER_REGISTERED:
					receiveTransformerRegisteredMessage((TransformerRegisteredMessage*) event.data);
					break;
				case TMESSID_IS_ALIVE:
					receiveIsAliveMessage((TransformerIsAliveMessage *) event.data);
					break;

				default:
					MME_Info(MME_INFO_MANAGER, ("Received unknown message ID %d\n", messageID));
					break;
				}
			} else {
				/* Strange message arrived. */
				MME_Info(MME_INFO_MANAGER, ("Strange message: type %d, size %d, data %0x\n", event.type, event.size, event.data));
			}
		} else {
			if (err == EMBX_INVALID_PORT || err == EMBX_PORT_INVALIDATED) {
				MME_Info(MME_INFO_MANAGER, ("RunMMEManager:EMBX_ReceiveBlock() returned due to invalid(ated) port, error=%d\n", err));
				result = MME_Term();
				if (result != MME_SUCCESS) {
					MME_Info(MME_INFO_MANAGER, ("TerminateMMEManager() failed, result=%d\n", result));
				}
				running = 0;
			} else {
				MME_Info(MME_INFO_MANAGER, ("RunMMEManager: EMBX_ReceiveBlock() failed, error=%d\n", err));
			}
		}
	}

	task_priority_set(NULL, oldTaskPriority);

	return MME_SUCCESS;
}

#ifdef EMBX_RECEIVE_CALLBACK
MME_ERROR MME_RegisterTransformerInt(const char *name,
				     MME_AbortCommand_t abortFunc,
				     MME_GetTransformerCapability_t getTransformerCapabilityFunc,
				     MME_InitTransformer_t initTransformerFunc,
				     MME_ProcessCommand_t processCommandFunc,
				     MME_TermTransformer_t termTransformerFunc,
				     MME_UINT createThread)
#else
MME_ERROR MME_RegisterTransformer(const char *name,
				  MME_AbortCommand_t abortFunc,
				  MME_GetTransformerCapability_t getTransformerCapabilityFunc,
				  MME_InitTransformer_t initTransformerFunc,
				  MME_ProcessCommand_t processCommandFunc,
				  MME_TermTransformer_t termTransformerFunc)
#endif
{
	MMEReceiverFactory* factory;

#ifndef MME_LEAN_AND_MEAN
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	if (NULL == name) {
		return MME_INVALID_ARGUMENT;
	}

	/* iterate over the (singly linked) factory list looking for duplicates */
	for (factory = manager->factoryList; NULL != factory; factory = factory->next) {
		if (0 == strcmp(factory->transformerType, name)) {
			return MME_INVALID_ARGUMENT;
		}
	}
#endif

	factory = malloc(sizeof(MMEReceiverFactory) + strlen(name) + 1);
	if (NULL == factory) {
		return MME_NOMEM;
	}

	factory->transformerType              = (const char *) (factory + 1);
	strcpy((void *) factory->transformerType, name);
	factory->abortFunc                    = abortFunc;
	factory->getTransformerCapabilityFunc = getTransformerCapabilityFunc;
	factory->initTransformerFunc          = initTransformerFunc;
	factory->processCommandFunc           = processCommandFunc;
	factory->termTransformerFunc          = termTransformerFunc;
#ifdef EMBX_RECEIVE_CALLBACK
	factory->createThread                 = createThread;
#endif /* EMBX_RECEIVE_CALLBACK */

	EMBX_OS_MUTEX_TAKE(&(manager->factoryListMutex));
	factory->next = manager->factoryList;
	manager->factoryList = factory;
	EMBX_OS_MUTEX_RELEASE(&(manager->factoryListMutex));

	return MME_SUCCESS;
}

#ifdef EMBX_RECEIVE_CALLBACK
MME_ERROR MME_RegisterTransformer(const char *name,
				  MME_AbortCommand_t abortFunc,
				  MME_GetTransformerCapability_t getTransformerCapabilityFunc,
				  MME_InitTransformer_t initTransformerFunc,
				  MME_ProcessCommand_t processCommandFunc,
				  MME_TermTransformer_t termTransformerFunc)
{
	/* Register Transformer with helper threads */
	return MME_RegisterTransformerInt(name, abortFunc, getTransformerCapabilityFunc, initTransformerFunc,
					  processCommandFunc, termTransformerFunc, EMBX_TRUE);
}

MME_ERROR MME_RegisterTransformer_Callback(const char *name,
					   MME_AbortCommand_t abortFunc,
					   MME_GetTransformerCapability_t getTransformerCapabilityFunc,
					   MME_InitTransformer_t initTransformerFunc,
					   MME_ProcessCommand_t processCommandFunc,
					   MME_TermTransformer_t termTransformerFunc)
{
	/* Register Transformer with Callbacks instead of helper threads */
	return MME_RegisterTransformerInt(name, abortFunc, getTransformerCapabilityFunc, initTransformerFunc,
					  processCommandFunc, termTransformerFunc, EMBX_FALSE);
}
#endif /* EMBX_RECEIVE_CALLBACK */

MME_ERROR MME_DeregisterTransformer(const char *name) {
	MME_ERROR result = MME_INVALID_ARGUMENT;
	MMEReceiverFactory *factory, **prev;

	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	if (NULL == name) {
		return MME_INVALID_ARGUMENT;
	}

	/* iterate over the (singly linked) factory list until we find 
	 * a factory or reach the end
	 */
	EMBX_OS_MUTEX_TAKE(&(manager->factoryListMutex));
	for (prev = &(manager->factoryList), factory = *prev;
	     NULL != factory;
	     prev = &(factory->next), factory = *prev) {
		if (0 == strcmp(factory->transformerType, name)) {
			/* unlink and free the node */
			*prev = factory->next;
			free(factory);

			result = MME_SUCCESS;
			break;
		}
	}
	EMBX_OS_MUTEX_RELEASE(&(manager->factoryListMutex));

	return result;
}

MME_ERROR MME_NotifyHost(MME_Event_t event, MME_Command_t* command, MME_ERROR err)
{
	/* assume the receiver field is immediately before the MME_Command_t field in the 
	 * TransformerTransformMessage struct
	 */	
	TransformerTransformMessage* message = (TransformerTransformMessage*) 
		(((char*)command) - offsetof(TransformerTransformMessage, command));

#ifndef MME_LEAN_AND_MEAN
	if (NULL == command) {
		return MME_INVALID_ARGUMENT;
	}

	if (event<MME_COMMAND_COMPLETED_EVT || event>MME_NOT_ENOUGH_MEMORY_EVT) {
		return MME_INVALID_ARGUMENT;
	}
#endif

	return MME_Receiver_NotifyHost((MMEReceiver *) message->receiverInstance, event, message, err);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
