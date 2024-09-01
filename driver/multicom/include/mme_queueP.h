/*
 * MMEMessageQueue.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * MME API internal header file (for both host and companion)
 */

#ifndef MMEMESSAGEQUEUE_H
#define MMEMESSAGEQUEUE_H

#include "embx_osinterface.h"
#include "mme.h"

typedef struct MMEMessageQueueEntry MMEMessageQueueEntry;
struct MMEMessageQueueEntry {
	MMEMessageQueueEntry *next;
	unsigned int priority;
	void *message;
};

typedef struct MMEMessageQueue {
	MMEMessageQueueEntry *queue;	/* queue array */
	EMBX_MUTEX queueLock;

	int maxEntries;		/* array size */
	MMEMessageQueueEntry *sorted_front;
	MMEMessageQueueEntry *sorted_back;
	MMEMessageQueueEntry *unsorted_front;
	MMEMessageQueueEntry *unsorted_back;
	MMEMessageQueueEntry *free_list;
} MMEMessageQueue;

MME_ERROR MME_MessageQueue_Init(MMEMessageQueue * msgQ, int maxEntries);
void      MME_MessageQueue_Term(MMEMessageQueue * msgQ);
MME_ERROR MME_MessageQueue_Enqueue(MMEMessageQueue * msgQ, void *message, unsigned int priority);
MME_ERROR MME_MessageQueue_Dequeue(MMEMessageQueue * msgQ, void **message);
MME_ERROR MME_MessageQueue_Peek(MMEMessageQueue * msgQ, void **message);
MME_ERROR MME_MessageQueue_RemoveByValue(MMEMessageQueue *msgQ, void *message);
MME_ERROR MME_MessageQueue_RemoveAll(MMEMessageQueue * msgQ);

#endif
