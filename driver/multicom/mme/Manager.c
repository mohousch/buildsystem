/*
 * Manager.c
 *
 * Copyright (C) STMicroelectronics Limited 2003-2004. All rights reserved.
 *
 * Code to handle (most of) the API function entry points and transformer lifetimes.
 */

#include "mme_hostP.h"
#include "mme_debugP.h"

/*******************************************************************/

/* The internal form of a data buffer descriptor used by MME_AllocDataBuffer() and
 * MME_FreeDataBuffer().
 */
struct InternalDataBuffer {
	MME_DataBuffer_t buffer;
	MME_AllocationFlags_t flags;
	MME_ScatterPage_t pages[1];
};

/*******************************************************************/

/* Global instance variable. This is the only place MME context is held. */
static HostManager_t *manager;

#ifndef MULTIHOST

/*******************************************************************/

/* Construct a one-way message requesting complete termination of our partner CPU. */
static MME_ERROR createTerminateMMEMessage(TransportInfo_t *tpInfo,
					   TransformerTerminateMMEMessage** buffer) {
	TransformerTerminateMMEMessage *message;
	EMBX_ERROR err;

	MME_Assert(manager);
	MME_Assert(tpInfo);
	MME_Assert(buffer);
	
	err = EMBX_Alloc(tpInfo->handle, sizeof(*message), (EMBX_VOID **) &message);
	if (EMBX_SUCCESS == err) {
		message->id = TMESSID_TERMINATE_MME;
		message->messageSize = sizeof(*message);
		message->result = MME_SUCCESS;
		*buffer = message;
	}
	
	/* map the EMBX error to an MME one */
	return EMBX_SUCCESS == err ? MME_SUCCESS :
	       EMBX_NOMEM == err   ? MME_NOMEM :
	                             MME_EMBX_ERROR;
}

#if 0
static void cleanupTerminateMMEMessage(TransportInfo_t *tpInfo, 
				       TransformerTerminateMMEMessage* buffer)
{
	EMBX_ERROR err;
	err = EMBX_Free(buffer);
	MME_Assert(EMBX_SUCCESS == err);
}
#endif

/*******************************************************************/

static MME_ERROR issueTerminateMMEMessages(TransportInfo_t* tpInfo)
{
	EMBX_ERROR err;
	MME_ERROR res;
	int i;

	MME_Assert(manager);
	MME_Assert(tpInfo);

	for (i=0; i<4; i++) {
		char adminPortName[] = "MMECompanionAdmin#0";
		EMBX_PORT adminPort;
		TransformerTerminateMMEMessage *message = NULL;

		/* Generate the port name we want to examine */
		adminPortName[sizeof(adminPortName) - 2] = '0' + i;

		/* Attempt to connect to the admin port */
		err = EMBX_Connect(tpInfo->handle, adminPortName, &adminPort);
		if (EMBX_PORT_NOT_BIND == err) {
			continue;
		};
		MME_Assert(EMBX_SUCCESS == err); /* no recovery possible */

		/* Allocate the terminate message */
		res = createTerminateMMEMessage(tpInfo, &message);
		MME_Assert(MME_SUCCESS == res); /* no recovery possible */

		/* Send the request and disconnect */
		err = EMBX_SendMessage(adminPort, message, message->messageSize);
		MME_Assert(MME_SUCCESS == res); /* no recovery possible */
		
		/* Close the admin port */
		err = EMBX_ClosePort(adminPort);
		MME_Assert(MME_SUCCESS == res); /* no recovery possible */
	}

	return MME_SUCCESS;
}

#endif /* !MULTIHOST */

/*******************************************************************/

static MME_ERROR createTransportInfo(const char *name, TransportInfo_t **tpInfo_p)
{
	const char portName[] = "MMEHostReplyPort#0";

	TransportInfo_t *tpInfo;
	EMBX_ERROR err;
	
	/* Allocate space for the transport descriptor */
	tpInfo = EMBX_OS_MemAlloc(sizeof(TransportInfo_t));
	if (!tpInfo) {
		return MME_NOMEM;
	}
	memset(tpInfo, 0, sizeof(TransportInfo_t));

	/* Open a transport handle */
	err = EMBX_OpenTransport(name, &tpInfo->handle);
	if (EMBX_SUCCESS != err) {
		goto error_recovery;
	}

	/* Create the reply port. */
	memcpy(tpInfo->replyPortName, portName, sizeof(portName));

	/* MULTIHOST support */
	do {
		MME_Info(MME_INFO_MANAGER, ("   EMBX_CreatePort(), port name <%s>...\n", 
					    tpInfo->replyPortName));
		
		/* Create the reply port. This port is purely synchronous (it only receives
		 * messages that are replies to outstanding messages) and as such does not
		 * need its own management thread.
		 */
		err = EMBX_CreatePort(tpInfo->handle, tpInfo->replyPortName, &tpInfo->replyPort);

	} while (EMBX_ALREADY_BIND == err && 
	         tpInfo->replyPortName[sizeof(portName)-2]++ < '9');

	if (EMBX_SUCCESS != err) {
		goto error_recovery;
	}
	
	*tpInfo_p = tpInfo;
	return MME_SUCCESS;

    error_recovery:
    
    	if (tpInfo->handle) {
    		err = EMBX_CloseTransport(tpInfo->handle);
    		MME_Assert(EMBX_SUCCESS == err);
    	}
    	EMBX_OS_MemFree(tpInfo);
    	
    	return MME_EMBX_ERROR;
}

static void destroyTransportInfo(TransportInfo_t *tpInfo)
{
	MME_ERROR res;
	EMBX_ERROR err;

	MME_Assert(manager);
	MME_Assert(tpInfo);

#ifndef MULTIHOST
	res = issueTerminateMMEMessages(tpInfo);
	MME_Assert(MME_SUCCESS == res);
#endif

	err = EMBX_ClosePort(tpInfo->replyPort);
	MME_Assert(EMBX_SUCCESS == err);

	err = EMBX_CloseTransport(tpInfo->handle);
	MME_Assert(EMBX_SUCCESS == err);

	EMBX_OS_MemFree(tpInfo);
}



/* Construct a two-way message to determine if the named transformer can be
 * instanciated by (has been registered on) a particular companion processor.
 */
static MME_ERROR createRegisteredMessage(TransportInfo_t *tpInfo, const char *name,
					 TransformerRegisteredMessage **msg)
{
	EMBX_ERROR err;
	void *buf;

	err = EMBX_Alloc(tpInfo->handle, sizeof(TransformerRegisteredMessage), &buf);
	if (EMBX_SUCCESS != err) {
		return EMBX_NOMEM == err ? MME_NOMEM : MME_EMBX_ERROR;
	}

	*msg = buf;
	(*msg)->id = TMESSID_TRANSFORMER_REGISTERED;
	(*msg)->messageSize = sizeof(TransformerRegisteredMessage);
	(*msg)->result = MME_UNKNOWN_TRANSFORMER;
	
	/* MULTIHOST support */
	strcpy((*msg)->portName, tpInfo->replyPortName);
	strcpy((*msg)->transformerType, name);

	return MME_SUCCESS;
}

static void cleanupRegisteredMessage(TransportInfo_t *tpInfo, TransformerRegisteredMessage *msg)
{
	EMBX_ERROR err;

	err = EMBX_Free(msg);
	MME_Assert(EMBX_SUCCESS == err);
}

/*******************************************************************/

#ifndef MULTIHOST
static MME_ERROR findTransformerOnHost(const char *name, RegisteredLocalTransformer_t **rlt)
{
	RegisteredLocalTransformer_t *elem;

	MME_Assert(name);

	for (elem = manager->localTransformerList; elem; elem = elem->next) {
		if (0 == strcmp(name, elem->name)) {
			if (rlt) {
				*rlt = elem;
			}
			break;
		}
	}

	return (elem ? MME_SUCCESS : MME_INVALID_ARGUMENT);
}
#endif /* !MULTIHOST */

static MME_ERROR findTransformerWithinTransport(TransportInfo_t *tpInfo, const char *name, 
						EMBX_PORT *adminPort_p)
{
	MME_ERROR res;
	EMBX_ERROR err;
	TransformerRegisteredMessage *msg = 0;
	EMBX_PORT  adminPort = 0;
	MME_UINT i;

	MME_Assert(manager);
	MME_Assert(name);
	MME_Assert(tpInfo);

	res = createRegisteredMessage(tpInfo, name, &msg);
	if (MME_SUCCESS != res) {
		return res;
	}

	for (i=0; i<4; i++) {
		char adminPortName[] = "MMECompanionAdmin#0";
		EMBX_RECEIVE_EVENT event;

		/* generate the port name we want to examine */
		adminPortName[sizeof(adminPortName) - 2] = '0' + i;

		/* attempt to connect to the admin port */
		err = EMBX_Connect(tpInfo->handle, adminPortName, &adminPort);
		if (EMBX_PORT_NOT_BIND == err) {
			continue;
		} else if (EMBX_SUCCESS != err) {
			goto error_recovery;
		}

		/* send the request and disconnect from the port */
		err = EMBX_SendMessage(adminPort, msg, sizeof(*msg));
		if (EMBX_SUCCESS != err) {
			goto error_recovery;
		}
		msg = 0;

		/* wait for the reply */
		/* TIMEOUT SUPPORT: Allow the receive to timeout */
		err = EMBX_ReceiveBlockTimeout(tpInfo->replyPort, &event, MME_TRANSFORMER_TIMEOUT);
		if (EMBX_SUCCESS != err) {
			goto error_recovery;
		}
		MME_Assert(event.type == EMBX_REC_MESSAGE);
		MME_Assert(event.size == sizeof(*msg));


		/* interpret said reply */
		msg = event.data;
		MME_Assert(msg->messageSize == sizeof(*msg));
		if (MME_SUCCESS == msg->result) {
			break;
		}

		err = EMBX_ClosePort(adminPort);
		if (EMBX_SUCCESS != err) {
			goto error_recovery;
		}
		adminPort = 0;
	}

	cleanupRegisteredMessage(tpInfo, msg);

	if (i < 4) {
		*adminPort_p = adminPort;
		return MME_SUCCESS;
	} else {
		return MME_UNKNOWN_TRANSFORMER;
	}

    error_recovery:

	/* TIMEOUT SUPPORT: Return MME_TRANSFORMER_NOT_RESPONDING if timeout occurs */
	res = (EMBX_NOMEM == err ? MME_NOMEM : (EMBX_SYSTEM_TIMEOUT == err ? MME_TRANSFORMER_NOT_RESPONDING : MME_EMBX_ERROR));

	if (adminPort) {
		err = EMBX_ClosePort(adminPort);
		MME_Assert(EMBX_SUCCESS == err);
	}

	if (msg) {
		cleanupRegisteredMessage(tpInfo, msg);
	}
	
	return res;
}

/*******************************************************************/

static MME_ERROR createTransformerInstance(const char *name, Transformer_t **tfp)
{
	MME_ERROR res;
	MME_ERROR res2 = MME_UNKNOWN_TRANSFORMER;
	RegisteredLocalTransformer_t *rlt;
	Transformer_t *tf;
	TransportInfo_t *tpInfo;

	MME_Assert(manager);
	MME_Assert(name);

#ifndef MULTIHOST
	/* first check whether this is a locally registered transformer (because this is much
	 * faster than querying the companions)
	 */
	res = findTransformerOnHost(name, &rlt);
	if (MME_SUCCESS == res) {
		tf = MME_LocalTransformer_Create(manager, rlt);
		if (tf) {
			*tfp = tf;
			return MME_SUCCESS;
		} else {
			return MME_NOMEM;
		}
	}
#endif /* !MULTIHOST */

	/* now start looking for the named transformer in each of the registered transports */
	for (tpInfo = manager->transportList; tpInfo; tpInfo = tpInfo->next) {
		EMBX_PORT adminPort;

		res = findTransformerWithinTransport(tpInfo, name, &adminPort);
		if (MME_SUCCESS == res) {
			/* RemoteTransformer_Create will update tpInfo's reference count and
			 * keep a reference to it. adminPort is transfered outright to the
			 * transformer
			 */
			tf = MME_RemoteTransformer_Create(manager, tpInfo, adminPort);
			if (tf) {
				*tfp = tf;
				return MME_SUCCESS;
			} else {
				return MME_NOMEM;
			}
		}
		/* TIMEOUT SUPPORT: Remember res if we see a timeout from any of the transports */
		else if (res == MME_TRANSFORMER_NOT_RESPONDING)
			res2 = res;
	}

	MME_Assert(res2 == MME_UNKNOWN_TRANSFORMER || res2 == MME_TRANSFORMER_NOT_RESPONDING);

	/* TIMEOUT SUPPORT: Returns MME_TRANSFORMER_NOT_RESPONDING if we failed
	 * to communicate with any of the transformers. MME_UNKNOWN_TRANSFORMER otherwise
	 */
	return res2;
}

/*******************************************************************/

static MME_ERROR registerTransformerInstance(Transformer_t* transformer, 
                                          MME_TransformerHandle_t * handle)
{
	MME_ERROR res;
	int id;

	MME_Assert(manager);
	MME_Assert(transformer);
	MME_Assert(handle);

	res = MME_LookupTable_Insert(manager->transformerInstances, transformer, &id);
	if (MME_SUCCESS == res) {
		*handle = MME_TRANSFORMER_HANDLE_PREFIX + id;
	}

	return res;
}

static void deregisterTransformerInstance(MME_TransformerHandle_t handle)
{
	MME_ERROR res;

	MME_Assert(manager);

	/* clear the prefix */
	handle -= MME_TRANSFORMER_HANDLE_PREFIX;

	res = MME_LookupTable_Remove(manager->transformerInstances, handle);

	MME_Assert(MME_SUCCESS == res);
}

static MME_ERROR findTransformerInstance(MME_TransformerHandle_t handle,
                                         Transformer_t** transformer)
{
	void *p;
	MME_ERROR res;

	MME_Assert(manager);
	MME_Assert(transformer);
	handle -= MME_TRANSFORMER_HANDLE_PREFIX;

	res = MME_LookupTable_Find(manager->transformerInstances, handle, &p);
	if (MME_SUCCESS == res) {
		*transformer = (Transformer_t *) p;
	}

	return res;
}

MME_ERROR MME_FindTransformerInstance(MME_TransformerHandle_t handle,
                                         Transformer_t** transformer) {
        MME_ERROR error;                                          	
	/* protect access */
	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	
	error = findTransformerInstance(handle, transformer);
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return error;
}
/*******************************************************************/

MME_ERROR MME_AbortCommand(MME_TransformerHandle_t handle, MME_CommandId_t commandID)
{
	Transformer_t *transformer;
	MME_ERROR result;

#ifndef MME_LEAN_AND_MEAN
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}
#endif

	/* protect access */
	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

	result = findTransformerInstance(handle, &transformer);

	if (result != MME_SUCCESS) {
		/* cannot find transformer for this handle */
		result = MME_INVALID_HANDLE;
		goto EXIT;
	}
	result = transformer->vtable->abortCommand(transformer, commandID);
	if (result != MME_SUCCESS) {
		result = MME_INVALID_ARGUMENT;
		goto EXIT;
	}

	result = MME_SUCCESS;
EXIT:
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return result;
}

/*******************************************************************/

MME_ERROR MME_GetTransformerCapability(const char *name, MME_TransformerCapability_t * capability)
{

	MME_ERROR res;
	Transformer_t *transformer;

#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }

        if (NULL == name || NULL == capability ||
            capability->StructSize != sizeof(MME_TransformerCapability_t)) {
                return MME_INVALID_ARGUMENT;
        }
#endif

	/* protect access */
	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

	/* Find a registered transformer (registered with MME_RegisterTransformer) */
	res = createTransformerInstance(name, &transformer);
	if (MME_SUCCESS == res) {
		EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
		res = transformer->vtable->getTransformerCapability(transformer, name, capability);
		EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

		transformer->vtable->destroy(transformer);
	}

	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return res;
}

/*******************************************************************/

#define DEBUG_FUNC_STR   "MME_InitTransformerInternal"

MME_ERROR MME_InitTransformerInternal(const char* name, MME_TransformerInitParams_t * params, 
				      MME_TransformerHandle_t * handle, EMBX_BOOL createThread)
{
	Transformer_t *transformer;
	MME_ERROR result;

#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
		MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "Driver not initialized\n"));
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }

        if (NULL == name || NULL == params || NULL == handle ||
            params->StructSize != sizeof(MME_TransformerInitParams_t) ||
	    strlen(name) > (MME_MAX_TRANSFORMER_NAME-1)) {
		MME_Info(MME_INFO_MANAGER, 
                         (DEBUG_ERROR_STR "Invalid argument\n"
                          "Name 0x%08x, params 0x%08x, handle 0x%08x, sizeok %d, strlen(name) %d\n",
                          (int)name, (int)params, (int)handle, 
                          (NULL == params) ? -1 : (params->StructSize == sizeof(MME_TransformerInitParams_t)),
                          (NULL == name) ? 0 : strlen(name)
                         )
                        );
                return MME_INVALID_ARGUMENT;
        }
#endif

	/* protect access */
	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

	/* Find a registered transformer (registered with MME_RegisterTransformer) */
	result = createTransformerInstance(name, &transformer);
	if (result != MME_SUCCESS) {
		MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "File %s, line %d: cannot find transformer %s\n", 
                         __FILE__, __LINE__, name));
		goto EXIT_A;
	}

	/* Register the transformer.locally and get a handle */
	result = registerTransformerInstance(transformer, handle);
	if (result != MME_SUCCESS) {
		MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR 
                          "File %s, line %d: cannot RegisterTransformer(), error %d\n",
                          __FILE__, __LINE__, result));
		result = MME_NOMEM;
		goto EXIT_B;
	}

	/* Initialize the transformer */
	result = transformer->vtable->init(transformer, name, params, *handle, createThread);

	if (result != MME_SUCCESS) {
		MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "File %s, line %d: cannot init transformer, error %d\n", __FILE__, __LINE__, result));
		goto EXIT_C;
	}

	/* We are successful */
	result = MME_SUCCESS;
	goto EXIT_A;

EXIT_C:
	deregisterTransformerInstance(*handle);
EXIT_B:
	transformer->vtable->destroy(transformer);
EXIT_A:
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return result;
}
#undef DEBUG_FUNC_STR

MME_ERROR MME_InitTransformer(const char* name, MME_TransformerInitParams_t * params, 
			      MME_TransformerHandle_t * handle)
{
	/* Initialise a transformer that uses helper threads */
	return MME_InitTransformerInternal(name, params, handle, EMBX_TRUE);
}

#ifdef EMBX_RECEIVE_CALLBACK
MME_ERROR MME_InitTransformer_Callback(const char* name, MME_TransformerInitParams_t * params, 
				       MME_TransformerHandle_t * handle)
{
	/* Initialise a transformer that uses Callbacks rather than helper threads */
	return MME_InitTransformerInternal(name, params, handle, EMBX_FALSE);
}
#endif /* EMBX_RECEIVE_CALLBACK */

/*******************************************************************/

MME_ERROR MME_IsStillAlive(MME_TransformerHandle_t handle, MME_UINT* alive)
{
	Transformer_t *transformer;
	MME_ERROR result;

#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);
	result = findTransformerInstance(handle, &transformer);
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);

	if (result != MME_SUCCESS) {
		/* cannot find transformer for this handle */
		return MME_INVALID_HANDLE;
	}

	result = transformer->vtable->isStillAlive(transformer, alive);

	return result;
}

/*******************************************************************/
#define DEBUG_FUNC_STR  "MME_KillCommand"
MME_ERROR MME_KillCommand(MME_TransformerHandle_t handle, MME_CommandId_t commandID)
{
	Transformer_t *transformer;
	MME_ERROR result;

#ifndef MME_LEAN_AND_MEAN
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}
#endif

	/* protect access */
	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

	result = findTransformerInstance(handle, &transformer);

	if (result != MME_SUCCESS) {
		/* cannot find transformer for this handle */
		result = MME_INVALID_HANDLE;
		goto EXIT;
	}
	result = transformer->vtable->killCommand(transformer, commandID);
	if (result != MME_SUCCESS) {
		result = MME_INVALID_ARGUMENT;
		goto EXIT;
	}

	result = MME_SUCCESS;
EXIT:
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return result;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/
#define DEBUG_FUNC_STR  "MME_KillCommandAll"
MME_ERROR MME_KillCommandAll(MME_TransformerHandle_t handle)
{
	Transformer_t *transformer;
	MME_ERROR result;

#ifndef MME_LEAN_AND_MEAN
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}
#endif

	/* protect access */
	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

	result = findTransformerInstance(handle, &transformer);

	if (result != MME_SUCCESS) {
		/* cannot find transformer for this handle */
		result = MME_INVALID_HANDLE;
		goto EXIT;
	}
	result = transformer->vtable->killCommandAll(transformer);
	if (result != MME_SUCCESS) {
		result = MME_INVALID_ARGUMENT;
		goto EXIT;
	}

	result = MME_SUCCESS;
EXIT:
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return result;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/
#define DEBUG_FUNC_STR  "MME_SendCommand"
MME_ERROR MME_SendCommand(MME_TransformerHandle_t handle, MME_Command_t * commandInfo)
{
	Transformer_t *transformer;
	MME_ERROR         result;
	CommandSlot_t     *commandPrivate;

#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
                MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Driver not initialized\n"));
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }

        if (0 == handle) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Invalid handle - NULL\n"));
                return MME_INVALID_HANDLE;
        }

        if (NULL == commandInfo ||
            commandInfo->StructSize != sizeof(MME_Command_t)) {
                MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Invalid struct size\n"));
                return MME_INVALID_ARGUMENT;
        }

        if (commandInfo->CmdCode != MME_SET_GLOBAL_TRANSFORM_PARAMS &&
            commandInfo->CmdCode != MME_TRANSFORM &&
            commandInfo->CmdCode != MME_SEND_BUFFERS) {
                MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Invalid command code\n"));
                return MME_INVALID_ARGUMENT;
        }

        if (commandInfo->CmdEnd != MME_COMMAND_END_RETURN_NO_INFO &&
            commandInfo->CmdEnd != MME_COMMAND_END_RETURN_NOTIFY) {
                MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Invalid command notify type\n"));
                return MME_INVALID_ARGUMENT;
        }
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);

	result = findTransformerInstance(handle, &transformer);
	if (result != MME_SUCCESS) {
		/* cannot find transformer for this handle */
                MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Cannot find transformer with handle 0x%08x\n", handle));
		result = MME_INVALID_HANDLE;
		goto EXIT;
	}

	result = transformer->vtable->sendCommand(transformer, commandInfo, &commandPrivate);

#ifdef ENABLE_MME_WAITCOMMAND
	/* Wake up any thread waiting for new commands with MME_WaitCommand
           or place the new command in a 1-deep buffer for thread that is
           about to wait
         */

	/* Grab this mutex to prevent any command completion events from 
           fiddling with the manager event state */
	EMBX_OS_MUTEX_TAKE(&manager->eventLock);

	if (manager->newCommandEvents) {
		/* One or more threads waiting for a new command event */
		int i, waitors=0;
		NewCommand_t* current;

		manager->eventTransformer = transformer;
		manager->eventCommand = commandPrivate;
		manager->eventCommandType = MME_NEW_COMMAND_EVT;

		current = manager->newCommandEvents;
		while (current) {
			waitors++;
			EMBX_OS_EVENT_POST(current->event);
			current = current->next;
		}

		for (i=0; i<waitors; i++) {
			/* Wait for the last waitor thread to unblock and remove it's event */
			EMBX_OS_EVENT_WAIT(&manager->waitorDone);
		}

		/* All waitor threads have let us know they have dealt with new event - now remove them from the list */
		RemoveNewCommandEvents_HostManager();

		manager->newCommand = NULL;
		manager->newCommandTransformer = NULL;
	} else {
		manager->newCommand = commandInfo;
		manager->newCommandTransformer = transformer;
	}
	EMBX_OS_MUTEX_RELEASE(&manager->eventLock);
#endif

EXIT:
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
        MME_Info( MME_INFO_MANAGER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", result));
	return result;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/
#define DEBUG_FUNC_STR   "MME_TermTransformer"

MME_ERROR MME_TermTransformer(MME_TransformerHandle_t handle)
{
	Transformer_t *transformer;
	MME_ERROR res;

#ifndef MME_LEAN_AND_MEAN
        if (NULL == manager) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Driver not initialized\n"));
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);

	/* Find the transformer */
	res = findTransformerInstance(handle, &transformer);
	if (res != MME_SUCCESS) {
		EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Failed to find transformer instance\n"));
		return MME_INVALID_HANDLE;
	}

	
	res = transformer->vtable->term(transformer);
	if (MME_SUCCESS == res) {
		deregisterTransformerInstance(handle);

		transformer->vtable->destroy(transformer);
	}
	
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return res;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/
#define DEBUG_FUNC_STR   "MME_KillTransformer"

MME_ERROR MME_KillTransformer(MME_TransformerHandle_t handle)
{
	Transformer_t *transformer;
	MME_ERROR res;

#ifndef MME_LEAN_AND_MEAN
        if (NULL == manager) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Driver not initialized\n"));
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);

	/* Find the transformer */
	res = findTransformerInstance(handle, &transformer);
	if (res != MME_SUCCESS) {
		EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Failed to find transformer instance\n"));
		return MME_INVALID_HANDLE;
	}

	
	res = transformer->vtable->kill(transformer);
	if (MME_SUCCESS == res) {
		deregisterTransformerInstance(handle);

		transformer->vtable->destroy(transformer);
	}
	
	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return res;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/

#define DEBUG_FUNC_STR   "MME_AllocDataBuffer"

MME_ERROR MME_AllocDataBuffer(MME_TransformerHandle_t handle, MME_UINT size,
			      MME_AllocationFlags_t flags, MME_DataBuffer_t ** dataBuffer_p) 
{
	static const MME_AllocationFlags_t illegalFlags = (MME_AllocationFlags_t)
		~(MME_ALLOCATION_PHYSICAL | MME_ALLOCATION_CACHED | MME_ALLOCATION_UNCACHED);

	Transformer_t *transformer;
	struct InternalDataBuffer *buf;
	unsigned localSize;

#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Driver not initialized\n"));
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }

	if (0 == size || NULL == dataBuffer_p || (flags & illegalFlags)) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "size==0 ||  NULL == dataBuffer_p || (flags & illegalFlags)\n"));
		return MME_INVALID_ARGUMENT;
	}

	if (0 > (int) size) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "size<0\n"));
		return MME_NOMEM;
	}
#endif

	/* lookup whether we should allocate using EMBX_OS_MemAlloc or EMBX_Alloc() */
	if (MME_SUCCESS != findTransformerInstance(handle, &transformer)) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "invalid transformer handle\n"));
		return MME_INVALID_HANDLE;
	}
	if (0 == transformer->info) {
		/* this is a local transformers so we can't allocate memory using EMBX_Alloc */
		flags |= MME_ALLOCATION_CACHED;
	}

	/* Allocate the buffer structure */
	/* Cannot present a "negative" value to EMBX_OS_MemAlloc on SHLINUX KERNEL mode
         * as it "succeeds" and returns a non-NULL value
         */
        localSize = (int) (sizeof(*buf) + (flags & MME_ALLOCATION_CACHED ? size + MME_MAX_CACHE_LINE-1 : 0));
        if (0 > (int) localSize) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "localSze <0\n"));
		return MME_NOMEM;
        }

	buf = EMBX_OS_MemAlloc(localSize);

	if (NULL == buf) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Cannot EMBX_OS_MemAlloc(%d)\n", 
                          sizeof(*buf) + (flags & MME_ALLOCATION_CACHED ? size : 0) ));
		return MME_NOMEM;
	}

	/* populate the buffer structure */
	memset(buf, 0, sizeof(*buf));
	buf->buffer.StructSize = sizeof(MME_DataBuffer_t);
	buf->buffer.NumberOfScatterPages = 1;
	buf->buffer.ScatterPages_p = buf->pages;
	buf->buffer.TotalSize = size;
	buf->flags = flags;
	buf->pages[0].Size = size;

	if (flags & MME_ALLOCATION_CACHED) {
		/* We MUST align the data buffer to a cacheline boundary to keep this safe */
		buf->pages[0].Page_p = (void *) MME_CACHE_LINE_ALIGN((buf + 1));
	} else {
		/* if transportHandle is 0 this will fail so we have no specific path to
		 * prevent uncached allocations for local transformers.
		 */
		EMBX_ERROR err = EMBX_Alloc(transformer->info->handle, size, 
					    &buf->pages[0].Page_p);

		if (EMBX_SUCCESS != err) {
     	                MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Cannot EMBX_Alloc(0x%08x, %d, 0x%08x)\n",
                                                     transformer->info->handle, size, (unsigned) &buf->pages[0].Page_p));
			EMBX_OS_MemFree(buf);
			return MME_NOMEM;
		}
	}

	*dataBuffer_p = &(buf->buffer);
	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR


MME_ERROR MME_FreeDataBuffer(MME_DataBuffer_t *dataBuffer) 
{
	struct InternalDataBuffer *buf = (struct InternalDataBuffer *) dataBuffer;
	
#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }

	if (NULL == buf) {
		return MME_INVALID_ARGUMENT;
	}

	if (buf->buffer.ScatterPages_p != buf->pages) {
		return MME_INVALID_ARGUMENT;
	}
#endif
	
	if (0 == (buf->flags & MME_ALLOCATION_CACHED)) {
		if (EMBX_SUCCESS != EMBX_Free(buf->pages[0].Page_p)) {
			return MME_INVALID_ARGUMENT;
		}
	}

	EMBX_OS_MemFree(buf);

	return MME_SUCCESS;
}
/*******************************************************************/

#ifndef MULTIHOST

MME_ERROR MME_NotifyHost(MME_Event_t event, MME_Command_t *command, MME_ERROR error)
{
	LocalTransformer_t *transformer;
	MME_ERROR res;

#ifndef MME_LEAN_AND_MEAN
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	if (NULL == command) {
		return MME_INVALID_ARGUMENT;
	}
#endif

	/* lookup the local transformer instanciation */
	res = MME_LookupTable_Find(manager->transformerInstances, 
				  MME_CMDID_GET_TRANSFORMER(command->CmdStatus.CmdId), 
				  (void **) &transformer);

	if (MME_SUCCESS != res) {
		return MME_INVALID_ARGUMENT;
	}

	return MME_LocalTransformer_NotifyHost(transformer, event, command, error);
}

/*******************************************************************/

MME_ERROR MME_RegisterTransformer(const char *name,
                                  MME_AbortCommand_t abortCommand,
                                  MME_GetTransformerCapability_t getTransformerCapability,
                                  MME_InitTransformer_t initTransformer,
                                  MME_ProcessCommand_t processCommand,
                                  MME_TermTransformer_t termTransformer)
{
	RegisteredLocalTransformer_t *elem;

#ifndef MME_LEAN_AND_MEAN
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	if (NULL == name) {
		return MME_INVALID_ARGUMENT;
	}
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);

	/* check for multiple registrations */
	for (elem = manager->localTransformerList; elem; elem = elem->next) {
		if (0 == strcmp(name, elem->name)) {
			EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
			return MME_INVALID_ARGUMENT;
		}
	}

	/* register the transformer */
	elem = EMBX_OS_MemAlloc(sizeof(*elem) + strlen(name) + 1);
	
	/* populate and enqueue the structure */
	elem->name = (char *) (elem + 1);
	strcpy(elem->name, name);

	elem->vtable.AbortCommand = abortCommand;
	elem->vtable.GetTransformerCapability = getTransformerCapability;
	elem->vtable.InitTransformer = initTransformer;
	elem->vtable.ProcessCommand = processCommand;
	elem->vtable.TermTransformer = termTransformer;

	elem->inUseCount = 0;

	elem->next = manager->localTransformerList;
	manager->localTransformerList = elem;

	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return MME_SUCCESS;
}


MME_ERROR MME_DeregisterTransformer(const char *name)
{
	RegisteredLocalTransformer_t *elem, **prev;

#ifndef MME_LEAN_AND_MEAN
	if (NULL == manager) {
		return MME_DRIVER_NOT_INITIALIZED;
	}

	if (NULL == name) {
		return MME_INVALID_ARGUMENT;
	}
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);

	for (elem = *(prev = &manager->localTransformerList); elem; elem = *(prev = &elem->next)) {
		if (0 == strcmp(name, elem->name)) {
			break;
		}
	}

	if (elem && 0 == elem->inUseCount) {
		*prev = elem->next;
		EMBX_OS_MemFree(elem);

		EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
		return MME_SUCCESS;
	}

	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	return MME_INVALID_ARGUMENT;
}

#endif /* !MULTIHOST */

/*******************************************************************/

#ifndef MULTIHOST
#define DEBUG_FUNC_STR "MME_RegisterTransport"
MME_ERROR MME_RegisterTransport(const char *name)
#else
#define DEBUG_FUNC_STR "MME_HostRegisterTransport"
MME_ERROR MME_HostRegisterTransport(const char *name)
#endif
{
	MME_ERROR res;
	EMBX_ERROR err;
	TransportInfo_t *tpInfo = 0;

#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
		MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "MME not initialized\n"));
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }

	if (name == NULL) {
		MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "Name not valid\n"));
		return MME_INVALID_ARGUMENT;
	}
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

#ifndef MME_LEAN_AND_MEAN
	/* prevent duplicate transport registrations */
	for (tpInfo = manager->transportList; tpInfo; tpInfo = tpInfo->next) {
		EMBX_TPINFO embxTpInfo;

		err = EMBX_GetTransportInfo(tpInfo->handle, &embxTpInfo);
		MME_Assert(EMBX_SUCCESS == err);

		if (0 == strcmp(name, embxTpInfo.name)) {
			EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
			MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "Transport already registered\n"));
			return MME_INVALID_ARGUMENT;
		}
	}
#endif

	/* allocate space for the transport descriptor */
	res = createTransportInfo(name, &tpInfo);
	if (MME_SUCCESS == res) {
		/* update the lists with in the manager */
		tpInfo->next = manager->transportList;
		manager->transportList = tpInfo;
	}

	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
       	MME_Info(MME_INFO_MANAGER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", res));
	return res;
}
#undef DEBUG_FUNC_STR

#ifndef MULTIHOST
#define DEBUG_FUNC_STR "MME_DeregisterTransport"
MME_ERROR MME_DeregisterTransport(const char* name)
#else
#define DEBUG_FUNC_STR "MME_HostDeregisterTransport"
MME_ERROR MME_HostDeregisterTransport(const char* name)
#endif
{
	EMBX_ERROR err;
	TransportInfo_t *tpInfo, **prev;
	MME_ERROR res;

#ifndef MME_LEAN_AND_MEAN
        if (manager == NULL) {
		MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "MME not initialized\n"));
                return MME_DRIVER_NOT_INITIALIZED;      /* the manager must exist */
        }

	if (name == NULL) {
		MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "Transport name NULL\n"));
		return MME_INVALID_ARGUMENT;
	}
#endif

	EMBX_OS_MUTEX_TAKE(&manager->monitorMutex);	

	MME_Info(MME_INFO_MANAGER, (DEBUG_NOTIFY_STR "Looking for transport %s\n", name));
	/* search the transport list for the appropriate transport */
	for (tpInfo = *(prev = &manager->transportList); tpInfo; tpInfo = *(prev = &tpInfo->next)) {
		EMBX_TPINFO embxTpInfo;

		err = EMBX_GetTransportInfo(tpInfo->handle, &embxTpInfo);
		MME_Assert(EMBX_SUCCESS == err);

		if (0 == strcmp(name, embxTpInfo.name)) {
			break;
		}
	}

	if (!tpInfo) {
		MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "Transport not found\n"));
		res = MME_INVALID_ARGUMENT;
	} else if (0 != tpInfo->referenceCount) {
		MME_Info(MME_INFO_MANAGER, (DEBUG_ERROR_STR "Handles still open\n"));
		res = MME_HANDLES_STILL_OPEN;
	} else {
		res = MME_SUCCESS;
		
		MME_Info(MME_INFO_MANAGER, (DEBUG_NOTIFY_STR "Removing transport\n"));
		/* remove the transport information from the list and cleanup */
		*prev = tpInfo->next;
		destroyTransportInfo(tpInfo);
	}

	EMBX_OS_MUTEX_RELEASE(&manager->monitorMutex);
	MME_Info(MME_INFO_MANAGER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", res));
	return res;
}
#undef DEBUG_FUNC_STR

/*******************************************************************/

#ifndef MULTIHOST
MME_ERROR MME_Init(void)
#else
MME_ERROR MME_HostInit(void)
#endif
{
	if (NULL != manager) {
		return MME_DRIVER_ALREADY_INITIALIZED;
	}

	manager = EMBX_OS_MemAlloc(sizeof(HostManager_t));
	if (!manager) {
		return MME_NOMEM;
	}
	memset(manager, 0, sizeof(HostManager_t));

	manager->transformerInstances = MME_LookupTable_Create(MAX_TRANSFORMER_INSTANCES, 1);
	if (!manager->transformerInstances) {
		goto error_path;
	}

	if (EMBX_FALSE == EMBX_OS_MUTEX_INIT(&manager->monitorMutex)) {
		goto error_path;
	}
	if (EMBX_FALSE == EMBX_OS_MUTEX_INIT(&manager->eventLock)) {
		goto error_path;
	}
	if (EMBX_FALSE == EMBX_OS_EVENT_INIT(&manager->waitorDone)) {
		goto error_path;
	}
	return MME_SUCCESS;

    error_path:

	if (manager->transformerInstances) {
		MME_LookupTable_Delete(manager->transformerInstances);
	}
	EMBX_OS_MemFree(manager);
	manager = NULL;
	
	return MME_NOMEM;
}

#define DEBUG_FUNC_STR "MME_Term"

#ifndef MULTIHOST
MME_ERROR MME_Term(void)
#else
MME_ERROR MME_HostTerm(void)
#endif
{
	TransportInfo_t *tpInfo, *next;

#ifndef MME_LEAN_AND_MEAN
	if (manager == NULL) {
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Failed to find transformer instance\n"));
                return MME_DRIVER_NOT_INITIALIZED;
        }
#endif

	if (!MME_LookupTable_IsEmpty(manager->transformerInstances)) {
		/* Need to close all transformers first */
 	        MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR "Handles still open\n"));
		return MME_HANDLES_STILL_OPEN;
	}

	/* Deregister all the transports (so that the companions receive terminate messages) */
	for (tpInfo = manager->transportList; tpInfo && (next = tpInfo->next, 1); tpInfo = next) {
		destroyTransportInfo(tpInfo);
	}

	EMBX_OS_MUTEX_DESTROY(&manager->monitorMutex);
	EMBX_OS_MUTEX_DESTROY(&manager->eventLock);
	EMBX_OS_EVENT_DESTROY(&manager->waitorDone);

	MME_LookupTable_Delete(manager->transformerInstances);

	EMBX_OS_MemFree(manager);
	manager = NULL;

	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR

#ifndef MULTIHOST
MME_ERROR MME_WaitForMessage(Transformer_t*  transformer, 
                             MME_Event_t*    eventPtr,
 	                     MME_Command_t** commandPtr)
{
	MME_Assert(transformer->vtable->waitForMessage);

	/* this is not an external visible API so invalid arguments are an
	 * internal error.
	 */
	MME_Assert(transformer); 
	MME_Assert(eventPtr);
	MME_Assert(commandPtr);

	return transformer->vtable->waitForMessage(
			transformer, eventPtr, commandPtr);
}

#endif /* !MULTIHOST */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
