/*
 * mme_messagesP.h
 *
 * Copyright (C) STMicroelectronics Limited 2003, 2004. All rights reserved.
 *
 * Descriptions of all structures messages between host and companion.
 */

#ifndef MME_MESSAGESP_H
#define MME_MESSAGESP_H

#include <embx.h>
#include "mme.h"

enum TransformerMessageID {
	TMESSID_INIT,
	TMESSID_TERMINATE,
	TMESSID_TERMINATE_MME,		/* this terminates the complete MME manager */
	TMESSID_TRANSFORM,
	TMESSID_STARVATION,
	TMESSID_ABORT,
	TMESSID_TRANSFORMER_REGISTERED,
	TMESSID_CAPABILITY,
	TMESSID_IS_ALIVE
};

typedef struct	{
	MME_UINT                id;
	MME_UINT                messageSize; /* in bytes */
	char                    transformerType[MME_MAX_TRANSFORMER_NAME];
	MME_Priority_t		priority;

	/* The following fields are filled in by the host and the MME. */
	MME_ERROR               result;
	char                    portName[EMBX_MAX_PORT_NAME+1];
	MME_TransformerHandle_t	mmeHandle;
	MME_UINT                numCommandQueueEntries;

	/* If there's additional TransformerInitParams_p, they will follow here... */
} TransformerInitMessage;


typedef struct {
	MME_UINT		id;
	MME_UINT		messageSize;	/* in bytes */

	MME_TransformerHandle_t	mmeHandle;

	/* The following fields are filled in by the MME. */
	MME_ERROR		result;
} TransformerTerminateMessage;

typedef struct {
	MME_UINT		id;
	MME_UINT		messageSize;	/* in bytes */

	MME_TransformerHandle_t	mmeHandle;

} TransformerIsAliveMessage;

typedef struct TransformerCapabilityMessage {
	MME_UINT		id;
	MME_UINT		messageSize;

	char			transformerType[MME_MAX_TRANSFORMER_NAME];
	char			portName[EMBX_MAX_PORT_NAME+1];

	/* The following fields are filled in by the MME. */
	MME_ERROR		result;
	MME_TransformerCapability_t capability;
	
	/* The capability parameters will follow this message */
} TransformerCapabilityMessage;

typedef struct {
	MME_UINT		id;
	MME_UINT		messageSize;	/* in bytes */
	char			portName[EMBX_MAX_PORT_NAME+1]; /* why? */

	/* The following fields are filled in by the MME. */
	MME_ERROR		result;
} TransformerTerminateMMEMessage;


typedef struct {
	MME_UINT		id;
	MME_UINT		messageSize;		/* in bytes */

	MME_Command_t		*hostCommand;		/* for the host MME implementation only */
	MME_UINT		receiverInstance;	/* for the companion MME implementation only */
	MME_Command_t		command;		/* MUST follow the receiverInstance field */

	/* If we ever wanted to go with only one reply port for all transformers, we would need a
	   HostTransformer pointer to identify the receiver of the reply. The MME must not use this! */
	void			*transformer;

	/* Now follow the parameters defined by Param_p and ParamSize which include the convention */

	/* (Start of buffer passing convention)
	*/
	MME_UINT		numInputBuffers;	/* input MME_DataBuffer_t structures, even if it's zero */
	MME_UINT		numOutputBuffers;	/* output MME_DataBuffer_t structures, even if it's zero */

	/* Then a variable structure follows
	    - Array (possibly empty) of the MME_DataBuffer_t structures for input,
		  immediately followed by those for output.

		- Then this sequence of two arrays follows for each data buffer:
			- Array (possibly empty) of n MME_ScatterPage_t structures.
			  (There are MME_DataBuffer_t.NumberOfScatterPages of them)
			- Array (possibly empty) of n EMBX object handles
			  (There are MME_DataBuffer_t.NumberOfScatterPages of them)

	    - The number of generic parameters in the CmdStatus struct is 
	      encoded in the status field above 

	    - An array of 64 bit-wide CmdStatus generic parameters (possibly zero)

	    - The number of generic transformer-specific parameters is encoded in
	      the command field above

	    - An array of 64 bit-wide transformer-specific parameters (possibly zero)

  */

} TransformerTransformMessage;


typedef struct {
	MME_UINT		id;
	MME_UINT		messageSize;	/* in bytes */
	MME_Event_t		event;
	MME_CommandStatus_t	status;
} TransformerStarvationMessage;


typedef struct {
	MME_UINT		id;
	MME_UINT		messageSize;	/* in bytes */
	MME_CommandId_t		commandId;
	MME_ERROR		result;
	void			*event;
	MME_ERROR		*storedResult;
} TransformerAbortMessage;

typedef struct {
	MME_UINT		id;
	MME_UINT		messageSize;	/* in bytes */
	MME_ERROR		result;
	char                    portName[EMBX_MAX_PORT_NAME+1];
	char                    transformerType[MME_MAX_TRANSFORMER_NAME];
} TransformerRegisteredMessage;

#endif /* MME_MESSAGESP_H */
