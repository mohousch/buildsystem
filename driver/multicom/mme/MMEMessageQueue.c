/*
 * MMEMessageQueue.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Priority ordered message queue with deferred sorting.
 */

/* NOTE: deferred sorting is implemented to minimize work while the
 * execution loop is pre-empted. this minimizes the cache damage of
 * the interruption.
 */

#include "mme_queueP.h"
#include "debug_ctrl.h"

#if defined __LINUX__ && !defined __KERNEL__
/* Until we have EMBX built in user space */
#include <stdlib.h>
#include <string.h>
#define EMBX_OS_MemAlloc(X)   (malloc(X))
#define EMBX_OS_MemFree(X)    (free(X))
#endif

#if defined WIN32
#define EMBX_OS_MemAlloc(X)   (malloc(X))
#define EMBX_OS_MemFree(X)    (free(X))
#endif

static void sortMessageQueue(MMEMessageQueue *msgQ)
{
	MMEMessageQueueEntry *entry;

	/* take the unsorted list and merge it with the sorted list.
	 * this code is optimized for FIFO operations and the time
	 * comparisions allow time to wrap. they do not directly 
	 * compare values but subtract with overflow wrap and compare
	 * against 0. note ANSI C only guarantees overflow wrap for 
	 * unsigned values (which is why the priority member is unsigned).
	 */

	EMBX_OS_MUTEX_TAKE(&msgQ->queueLock);
	entry = msgQ->unsorted_front;
	while (entry) {
		msgQ->unsorted_front = entry->next;

		if (NULL == msgQ->sorted_front) {
			/* empty list short circuit */
			entry->next = NULL;
			msgQ->sorted_front = entry;
			msgQ->sorted_back = entry;
		} else if (0 >= (int) (msgQ->sorted_back->priority - entry->priority)) {
			/* FIFO ordered queue short circuit */
			entry->next = NULL;
			msgQ->sorted_back->next = entry;
			msgQ->sorted_back = entry;
		} else {
			/* search the queue until we find a entry with a (strictly) more
			 * distant due date
			 */
			MMEMessageQueueEntry **prev, *cur;

			/* search the queue until we find our insertion point. we know
			 * we cannot run off the end otherwise we would have taken one
			 * of the short circuits above.
			 */
			for (prev = &(msgQ->sorted_front), cur = *prev;
			     MME_Assert(cur), 0 >= (int) (cur->priority - entry->priority);
			     prev = &(cur->next), cur = *prev) {} 

			entry->next = cur;
			*prev = entry;
			MME_Assert(*prev != msgQ->sorted_back);
		}

		entry = msgQ->unsorted_front;
	}
	EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
}

MME_ERROR MME_MessageQueue_Init(MMEMessageQueue * msgQ, int maxEntries)
{
	MME_ERROR err;
        int size = maxEntries * sizeof(MMEMessageQueueEntry);

	MME_Info(MME_INFO_QUEUE, (">>>MME_MessageQueue_Init(0x%08x, %d)\n", 
	                          (unsigned) msgQ, maxEntries));

	msgQ->maxEntries = maxEntries;

	msgQ->queue = (MMEMessageQueueEntry*) EMBX_OS_MemAlloc(size);

	if (msgQ->queue == NULL) {
		MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Init = MME_NOMEM\n"));
		return MME_NOMEM;
	}
        memset(msgQ->queue, 0, size);

	if (!EMBX_OS_MUTEX_INIT(&msgQ->queueLock)) {
		MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Init = MME_NOMEM\n"));
		return MME_NOMEM;
	}

	err = MME_MessageQueue_RemoveAll(msgQ);
	MME_Assert(MME_SUCCESS == err); /* currently has no failure path */


	MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Init = MME_SUCCESS\n"));
	return MME_SUCCESS;
}

/* Clean up and deallocate everything. */
void MME_MessageQueue_Term(MMEMessageQueue * msgQ)
{
	MME_Info(MME_INFO_QUEUE, (">>>MME_MessageQueue_Term(0x%08x)\n", (unsigned) msgQ));

	EMBX_OS_MUTEX_DESTROY(&msgQ->queueLock);
	if (msgQ->queue != NULL) {
		EMBX_OS_MemFree(msgQ->queue);
	}
	memset(msgQ, 0, sizeof(*msgQ));

	MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Term\n"));
}

MME_ERROR MME_MessageQueue_Enqueue(MMEMessageQueue * msgQ, void *message, unsigned int priority)
{
	MMEMessageQueueEntry *entry;

	MME_Info(MME_INFO_QUEUE, (">>>MME_MessageQueue_Enqueue(0x%08x, 0x%08x)\n", 
	                          (unsigned) msgQ, (unsigned) message));

	EMBX_OS_MUTEX_TAKE(&msgQ->queueLock);

	/* fetch a free linkage structure from the free list */
	entry = msgQ->free_list;
	if (NULL == entry) {
		EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
		MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Enqueue = MME_NOMEM\n"));
		return MME_NOMEM;
	}
	msgQ->free_list = entry->next;

	/* fill in the linkage structure */
	entry->next = NULL;
	entry->priority = priority;
	entry->message = message;

	/* add it to the unsorted FIFO queue. we use a FIFO queue in order to
	 * exploit the FIFO optimizations in SortMMEMessageQueue.
	 */
	if (NULL == msgQ->unsorted_front) {
		msgQ->unsorted_front = entry;
	} else {
		msgQ->unsorted_back->next = entry;
	}
	msgQ->unsorted_back = entry;

	EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
	MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Enqueue = MME_SUCCESS\n"));
	return MME_SUCCESS;
}

MME_ERROR MME_MessageQueue_Dequeue(MMEMessageQueue * msgQ, void **message)
{
	MMEMessageQueueEntry *entry;

	MME_Info(MME_INFO_QUEUE, (">>>MME_MessageQueue_Dequeue(0x%08x, ...)\n", (unsigned) msgQ));

	sortMessageQueue(msgQ);

	EMBX_OS_MUTEX_TAKE(&msgQ->queueLock);

	/* dequeue the first message */
	entry = msgQ->sorted_front;
	if (NULL == entry) {
		*message = NULL;
		MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Dequeue(..., 0x%08x) = MME_INVALID_HANDLE\n",
		                          (unsigned) *message));
		EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
		return MME_INVALID_HANDLE;
	}
	msgQ->sorted_front = entry->next;

	/* push it onto the free list */
	entry->next = msgQ->free_list;
	msgQ->free_list = entry;

	*message = entry->message;

	MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Dequeue(..., 0x%08x) = MME_SUCCESS\n",
				  (unsigned) *message));
	EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
	return MME_SUCCESS;
}

/* Peek does not dequeue the entry. */
MME_ERROR MME_MessageQueue_Peek(MMEMessageQueue * msgQ, void **message)
{
	MME_Info(MME_INFO_QUEUE, (">>>MME_MessageQueue_Peek(0x%08x, ...)\n", (unsigned) msgQ));

	sortMessageQueue(msgQ);
	EMBX_OS_MUTEX_TAKE(&msgQ->queueLock);
	if (NULL == msgQ->sorted_front) {
		*message = NULL;
		MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Peek(..., 0) = MME_INVALID_HANDLE\n"));
		EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
		return MME_INVALID_HANDLE;
	}

	*message = msgQ->sorted_front->message;

	EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);

	MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_Peek(..., 0x%08x) = MME_SUCCESS\n",
				  (unsigned) *message));
	return MME_SUCCESS;
}

MME_ERROR MME_MessageQueue_RemoveByValue(MMEMessageQueue *msgQ, void *message)
{
	MMEMessageQueueEntry **prev, *cur;

	sortMessageQueue(msgQ);

	EMBX_OS_MUTEX_TAKE(&msgQ->queueLock);
	for (cur = *(prev = &msgQ->sorted_front); cur; cur = *(prev = &cur->next)) {
		if (cur->message == message) {
			/* dequeue the message */
			*prev = cur->next;

			/* push it onto the free list */
			cur->next = msgQ->free_list;
			msgQ->free_list = cur;
			break;
		}
	}

	EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
	return (cur ? MME_SUCCESS : MME_INVALID_ARGUMENT);
}

MME_ERROR MME_MessageQueue_RemoveAll(MMEMessageQueue * msgQ)
{
	MMEMessageQueueEntry **prev;
	int i;

	MME_Info(MME_INFO_QUEUE, (">>>MME_MessageQueue_RemoveAll(0x%08x)\n", (unsigned) msgQ));
	MME_Assert(msgQ);
	MME_Assert(msgQ->maxEntries > 0);

	EMBX_OS_MUTEX_TAKE(&msgQ->queueLock);

	/* initialize the free list */
	for (prev = &(msgQ->free_list), i=0;
	     i < msgQ->maxEntries;
	     prev = &(msgQ->queue[i].next), i++) {
		*prev = &(msgQ->queue[i]);
	}
	*prev = NULL;

	msgQ->sorted_front = 0;
	msgQ->unsorted_front = 0;

	EMBX_OS_MUTEX_RELEASE(&msgQ->queueLock);
	MME_Info(MME_INFO_QUEUE, ("<<<MME_MessageQueue_RemoveAll = MME_SUCCESS\n"));
	return MME_SUCCESS;
}
