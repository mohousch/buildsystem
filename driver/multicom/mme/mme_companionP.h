/*
 * mme_companionP.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Internal header file for MME companion side.
 */

#ifndef MME_COMPANIONP_H
#define MME_COMPANIONP_H

#include <embx.h>
#include <embx_osinterface.h>
#include <mmeP.h>
#include <mme_debugP.h>
#include <mme_queueP.h>

/*******************************************************************/

typedef struct MMEManager MMEManager_t;
typedef struct MMEReceiverFactory MMEReceiverFactory;
typedef struct MMEReceiver MMEReceiver;

/*******************************************************************/

struct MMEManager {
	int				mmeNumber;		/* unique number in the system */

	EMBX_TRANSPORT			transportHandle;
	EMBX_PORT			adminPort;
	char				adminPortName[EMBX_MAX_PORT_NAME+1];

	EMBX_PORT			replyPort;

	EMBX_EVENT			commandEvents[MME_NUM_EXECUTION_LOOPS];

	EMBX_THREAD			loopTasks[MME_NUM_EXECUTION_LOOPS];
	MME_UINT			loopTasksRunning[MME_NUM_EXECUTION_LOOPS];

	int				oldTaskPriority;
	MME_UINT			managerRunning;

	EMBX_MUTEX			factoryListMutex;
	EMBX_MUTEX			abortMutex;
	MMEReceiverFactory		*factoryList;

	/* The receiver list is protected by a mutex */
	EMBX_MUTEX			receiverListMutexes[MME_NUM_EXECUTION_LOOPS];
	MMEReceiver			*receiverLists[MME_NUM_EXECUTION_LOOPS];
};

/*******************************************************************/

struct MMEReceiverFactory {
	/* The receiver object factories are held in a single-linked list. */
	MMEReceiverFactory *next;

	const char* transformerType;

	MME_AbortCommand_t             abortFunc;
	MME_GetTransformerCapability_t getTransformerCapabilityFunc;
	MME_InitTransformer_t          initTransformerFunc;
	MME_ProcessCommand_t           processCommandFunc;
	MME_TermTransformer_t          termTransformerFunc;

#ifdef EMBX_RECEIVE_CALLBACK
	EMBX_BOOL                      createThread;
#endif /* EMBX_RECEIVE_CALLBACK */
};

/*******************************************************************/

struct MMEReceiver {
	/* The receiver objects are held in a single-linked list. */
	MMEReceiver *next;

	MME_TransformerHandle_t mmeHandle;

	MMEManager_t *manager;
	EMBX_TRANSPORT transportHandle;

	MME_TransformerInitParams_t initParams;

	EMBX_PORT receiverPort;	/* to receive messages from the host */
	char receiverPortName[EMBX_MAX_PORT_NAME+1];
	MME_UINT receiverPortExists;

	EMBX_PORT replyPort;	/* to reply messages to the host */
	char replyPortName[EMBX_MAX_PORT_NAME+1];

	EMBX_EVENT *commandEvent;

	EMBX_THREAD receiverTask;

	MMEMessageQueue messageQueue;
	TransformerTransformMessage **messageTable;

	/* ??? It is possible that we also need a buffer queue for stream-based transformers
	   where buffers must be capable of overtaking commands. */
	MMEMessageQueue bufferQueue;

	MME_UINT running;		/* is the receiver task running? */

	/* This is the function table to the functions of the transformer API. */
	/* The receiver factory will make these entries. */
	MME_AbortCommand_t             Abort;
	MME_GetTransformerCapability_t GetTransformerCapability;
	MME_InitTransformer_t          InitTransformer;
	MME_TermTransformer_t          TermTransformer;
	MME_ProcessCommand_t           ProcessCommand;
};

/*******************************************************************/

MME_ERROR MME_Receiver_Create(MMEReceiverFactory* factory, MMEReceiver ** receiver_p);
void      MME_Receiver_Destroy(MMEReceiver *);
MME_ERROR MME_Receiver_Init(MMEReceiver *, struct MMEManager *,
			    EMBX_TRANSPORT, TransformerInitMessage *, EMBX_PORT, EMBX_EVENT *);
#ifdef EMBX_RECEIVE_CALLBACK
MME_ERROR MME_Receiver_Init_Callback(MMEReceiver *, struct MMEManager *,
				     EMBX_TRANSPORT, TransformerInitMessage *, EMBX_PORT, EMBX_EVENT *, EMBX_BOOL);
#endif /* EMBX_RECEIVE_CALLBACK */
MME_ERROR MME_Receiver_Term(MMEReceiver *);
MME_ERROR MME_Receiver_NextDueTime(MMEReceiver *, MME_Time_t * dueTime);
MME_ERROR MME_Receiver_ProcessCommand(MMEReceiver *);
EMBX_PORT MME_Receiver_GetReplyPort(MMEReceiver *);
MME_ERROR MME_Receiver_NotifyHost(MMEReceiver *, MME_Event_t event, 
                                TransformerTransformMessage *message, MME_ERROR err);

#endif /* MME_COMPANIONP_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
