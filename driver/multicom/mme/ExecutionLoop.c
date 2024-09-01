/*
 * ExecutionLoop.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Host implementation of the execution loops used by local transformers.
 */

#include "mmeP.h"
#include "mme_hostP.h"

#if defined __LINUX__ && !defined __KERNEL__
/* Until we have EMBX built in user space */
#include <stdlib.h>
#define EMBX_OS_MemAlloc(X)   (malloc(X))
#define EMBX_OS_MemFree(X)    (free(X))
#define EMBX_OS_ThreadCreate  LinuxUser_ThreadCreate
#define EMBX_OS_ThreadDelete  LinuxUser_ThreadDelete

pthread_t* LinuxUser_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name);
int        LinuxUser_ThreadDelete(pthread_t*);
#endif

/* map 5000 -> 4, 4000 -> 3, ..., 1000 -> 0 without using decimal division */
#define MME_PRIORITY_TO_ID(pri) (pri >> 10)

static void executionLoopTask(void *params)
{
	HostManager_t *manager = ((void**) params)[0];
	int id = (int) (((void **) params)[1]);

	MME_ERROR res;
	MME_Command_t *command;

	EMBX_OS_MemFree(params);

	while (!EMBX_OS_EVENT_WAIT(manager->commandEvents + id) &&
	       manager->loopTasksRunning[id]) {
		LocalTransformer_t *transformer;
		
		/* get the command to be executed (spurious signals can happen if
		 * commands are aborted)
		 */
		res = MME_MessageQueue_Dequeue(manager->messageQueues + id, (void **) &command);
		if (MME_SUCCESS == res) {
			/* lookup the local transformer instanciation */
			res = MME_LookupTable_Find(manager->transformerInstances, 
					  MME_CMDID_GET_TRANSFORMER(command->CmdStatus.CmdId), 
					  (void **) &transformer);
			MME_Assert(MME_SUCCESS == res);

			/* perform the command */
			MME_LocalTransformer_ProcessCommand(transformer, command);
		}
	}
}

static MME_ERROR initializeActualLoop(HostManager_t *manager, int id)
{
	struct { MME_UINT priority; const char *name; } lookup[MME_NUM_EXECUTION_LOOPS] = {
		{ MME_TUNEABLE_EXECUTION_LOOP_LOWEST_PRIORITY,       "ExecutionLoopLowest" },
		{ MME_TUNEABLE_EXECUTION_LOOP_BELOW_NORMAL_PRIORITY, "ExecutionLoopBelowNormal" },
		{ MME_TUNEABLE_EXECUTION_LOOP_NORMAL_PRIORITY,       "ExecutionLoopNormal" },
		{ MME_TUNEABLE_EXECUTION_LOOP_ABOVE_NORMAL_PRIORITY, "ExecutionLoopAboveNormal" },
		{ MME_TUNEABLE_EXECUTION_LOOP_HIGHEST_PRIORITY,      "ExecutionLoopHighest" },
	};

	void **params;

	MME_Assert(manager);
	MME_Assert(id < MME_NUM_EXECUTION_LOOPS);

	if (manager->loopTasksRunning[id]) {
		return MME_SUCCESS;
	}

	MME_Assert(0 == manager->loopTasks[id]);
	
	if (!EMBX_OS_EVENT_INIT(manager->commandEvents + id)) {
		return MME_NOMEM;
	}

	/* spawn the worker thread */
	params = EMBX_OS_MemAlloc(2*sizeof(void *)); /* matched by created thread */
	params[0] = manager;
	params[1] = (void *) id;
	manager->loopTasks[id] = EMBX_OS_ThreadCreate(executionLoopTask, 
		       params, MME_GetTuneable(lookup[id].priority), lookup[id].name);
	if (0 == manager->loopTasks[id]) {
		goto cleanup_event;
	}

	/* mark the task as running */
	manager->loopTasksRunning[id] = 1;

	return MME_SUCCESS;

   cleanup_event:
	EMBX_OS_EVENT_DESTROY(manager->commandEvents + id);
	
	return MME_NOMEM;
}

MME_ERROR MME_ExecutionLoop_Init(HostManager_t *manager, MME_Priority_t priority,
                        MMEMessageQueue **msgQ, EMBX_EVENT **event)
{
	MME_ERROR res;
	int id = MME_PRIORITY_TO_ID(priority);

	MME_Assert(manager);
	MME_Assert(msgQ);
	MME_Assert(event);

	if (!manager->loopTasksRunning[id]) {
		res = MME_MessageQueue_Init(manager->messageQueues + id, 64);
		if (MME_SUCCESS != res) {
			return res;
		}

		res = initializeActualLoop(manager, id);
		if (MME_SUCCESS != res) {
			(void) MME_MessageQueue_Term(manager->messageQueues + id);
			return res;
		}
	}
	
	*msgQ = manager->messageQueues + id;
	*event = manager->commandEvents + id;

	return MME_SUCCESS;
}


void MME_ExecutionLoop_TerminateAll(HostManager_t *manager)
{
	int i;

	MME_Assert(manager);

	for (i=0; i<MME_NUM_EXECUTION_LOOPS; i++) {
		if (manager->loopTasksRunning[i]) {
			/* request that the task to stop running (and unblock it so it can
			 * observe our request)
			 */
			manager->loopTasksRunning[i] = 0;
			EMBX_OS_EVENT_POST(manager->commandEvents + i);

			/* delete the thread (will wait for the thread to exit) */
			EMBX_OS_ThreadDelete(manager->loopTasks[i]);

			/* destroy the event and the message queue */
			EMBX_OS_EVENT_DESTROY(manager->commandEvents + i);
			MME_MessageQueue_Term(manager->messageQueues + i);
		}
	}
}
