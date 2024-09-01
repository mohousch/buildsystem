/*
 * mme_hostP.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Internal header file for MME host side.
 */

#ifndef MME_HOSTP_H
#define MME_HOSTP_H

#include <embx_osinterface.h>

#include <mmeP.h>
#include <mme_debugP.h>
#include <mme_queueP.h>

#include "LookupTable.h"

/*******************************************************************/

typedef struct Transformer Transformer_t;
typedef struct TransformerVTable TransformerVTable_t;
typedef struct LocalTransformer_s LocalTransformer_t;
typedef struct LocalTransformerVTable LocalTransformerVTable_t;
typedef struct RegisteredLocalTransformer RegisteredLocalTransformer_t;
typedef struct TransportInfo TransportInfo_t;
typedef struct NewCommand_s NewCommand_t;
typedef struct CommandSlot CommandSlot_t;
typedef struct HostManager HostManager_t;

/*******************************************************************/

typedef enum {
	MME_RUNNING = MME_NOT_ENOUGH_MEMORY_EVT + 1000
} CommandStatus_t;

/*******************************************************************/

struct Transformer {
	const TransformerVTable_t *vtable;
	HostManager_t *manager;
	TransportInfo_t *info;
	MME_TransformerHandle_t handle;
	MME_TransformerInitParams_t initParams;
};

struct TransformerVTable {
	/* Lifetime managment */
	void (*destroy)(Transformer_t *);
	MME_ERROR (*init)(Transformer_t *, const char *, 
			  MME_TransformerInitParams_t *, MME_TransformerHandle_t, EMBX_BOOL);
	MME_ERROR (*term)(Transformer_t *);
	MME_ERROR (*kill)(Transformer_t *);

	/* Operations upon a transformer */
	MME_ERROR (*abortCommand)(Transformer_t *, MME_CommandId_t);
	MME_ERROR (*killCommand)(Transformer_t *, MME_CommandId_t);
  	MME_ERROR (*killCommandAll)(Transformer_t *);
	MME_ERROR (*sendCommand)(Transformer_t *, MME_Command_t *, CommandSlot_t **);
	MME_ERROR (*isStillAlive)(Transformer_t *, MME_UINT *);
	MME_ERROR (*getTransformerCapability)(Transformer_t *, const char *, 
					      MME_TransformerCapability_t *);

	/* Wait for events with a separate thread */
	MME_ERROR (*waitForMessage)(Transformer_t*  transformer, 
                                   MME_Event_t*    eventPtr,
		     		   MME_Command_t** commandPtr);

	/* Command slot management */
	/* TODO: are these used any other than for MME_WaitCommand() */
	CommandSlot_t* (*lookupCommand)(Transformer_t *, int);
	MME_ERROR (*releaseCommandSlot)(Transformer_t *, CommandSlot_t *slot);
};

struct LocalTransformerVTable {
	LocalTransformerVTable_t *next;
	
	MME_AbortCommand_t AbortCommand;
	MME_GetTransformerCapability_t GetTransformerCapability;
	MME_InitTransformer_t InitTransformer;
	MME_ProcessCommand_t ProcessCommand;
	MME_TermTransformer_t TermTransformer;
};

struct RegisteredLocalTransformer {
	RegisteredLocalTransformer_t *next;

	char *name;
	LocalTransformerVTable_t vtable;
	int inUseCount;
};

/* This describes a single registered transport and what transformers it currently has open */
struct TransportInfo {
	TransportInfo_t		*next;
	MME_UINT		referenceCount;

	EMBX_TRANSPORT		handle;
	EMBX_PORT		replyPort;
	char			replyPortName[EMBX_MAX_PORT_NAME+1];	/* MULTIHOST support */
};


struct NewCommand {
	EMBX_EVENT*		event;
	struct NewCommand_s*	next;
};

struct CommandSlot {
	MME_CommandEndType_t endType;
	MME_Command_t*	     command;
	EMBX_EVENT*	     waitor;
	CommandStatus_t	     status;
  	EMBX_VOID           *message;
};

struct HostManager {
	EMBX_MUTEX		monitorMutex;
	TransportInfo_t*	transportList;		/* linked list */
	int			maxMultiMediaEngines;	/* array size */
	int			numMultiMediaEngines;	/* valid entries in array */
	int			transportId;		/* used to name admin ports uniquely */
	RegisteredLocalTransformer_t *localTransformerList;
	MMELookupTable_t*       transformerInstances;	/* table of transformer instances */

	/* The following fields are used to implement MME_WaitCommand */
	EMBX_MUTEX		eventLock;		/* Lock on all event notification fields */
	EMBX_EVENT		waitorDone;		/* Signal manager when a waitor is awoken
	                                                   and event handled */

#ifdef ENABLE_MME_WAITCOMMAND
	CommandSlot_t*		eventCommand;		/* An event - for MME_WaitCommand() */
	MME_Event_t		eventCommandType;	/* The event type - for MME_WaitCommand() */
	void*			eventTransformer;	/* Transformer associated with the event */
	NewCommand_t*		newCommandEvents;	/* Linked list of events to signal when 
							   new command is submitted */
	MME_Command_t*		newCommand;
	Transformer_t*		newCommandTransformer;
#endif
	/* Manage the locally implemented transformer's execution contexts */
	int			loopTasksRunning[MME_NUM_EXECUTION_LOOPS];
	EMBX_THREAD		loopTasks[MME_NUM_EXECUTION_LOOPS];
	EMBX_EVENT		commandEvents[MME_NUM_EXECUTION_LOOPS];
	MMEMessageQueue		messageQueues[MME_NUM_EXECUTION_LOOPS];
};

/*******************************************************************/

MME_ERROR MME_ExecutionLoop_Init(HostManager_t *manager, MME_Priority_t priority,
                        MMEMessageQueue **msgQ, EMBX_EVENT **event);
void MME_ExecutionLoop_TerminateAll(HostManager_t *manager);

/*******************************************************************/

Transformer_t *MME_RemoteTransformer_Create(HostManager_t *, 
					TransportInfo_t* info, EMBX_PORT adminPort);

Transformer_t *MME_LocalTransformer_Create(HostManager_t *, RegisteredLocalTransformer_t *);
MME_ERROR MME_LocalTransformer_NotifyHost(LocalTransformer_t *transformer, 
				      MME_Event_t event, MME_Command_t *command, MME_ERROR err);
void MME_LocalTransformer_ProcessCommand( LocalTransformer_t *transformer, MME_Command_t *command);

MME_ERROR MME_InitTransformerInternal(const char* name, MME_TransformerInitParams_t * params, 
                                      MME_TransformerHandle_t * handle, EMBX_BOOL createThread);


MME_ERROR MME_FindTransformerInstance(MME_TransformerHandle_t handle, Transformer_t** transformer);

MME_ERROR MME_WaitForMessage(Transformer_t*  transformer, 
                             MME_Event_t*    eventType,
                             MME_Command_t** command);

/*******************************************************************/

#endif /* MME_HOSTP_H */
