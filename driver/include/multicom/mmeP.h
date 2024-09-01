/*
 * mmeP.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Private header file for useful MME types
 */

#ifndef MMEP_H
#define MMEP_H

#include "mme.h"
#include "mme_messagesP.h"

/*******************************************************************/

extern MME_UINT MME_GetTuneable(MME_Tuneable_t key);

#define MANAGER_TASK_PRIORITY                MME_GetTuneable(MME_TUNEABLE_MANAGER_THREAD_PRIORITY)
#define RECEIVER_TASK_PRIORITY               MME_GetTuneable(MME_TUNEABLE_TRANSFORMER_THREAD_PRIORITY)

#define MME_NUM_EXECUTION_LOOPS 5

#define MAX_TRANSFORMER_SEND_COMMANDS 32
#define MAX_TRANSFORMER_SEND_BUFFERS  20
#define MAX_TRANSFORMER_SEND_OPS (MAX_TRANSFORMER_SEND_COMMANDS + MAX_TRANSFORMER_SEND_BUFFERS)
#define MAX_TRANSFORMER_INSTANCES     32

/* TIMEOUT SUPPORT : How long to wait (ms) for a transformer TERM to respond */
#define MME_TRANSFORMER_TIMEOUT              MME_GetTuneable(MME_TUNEABLE_TRANSFORMER_TIMEOUT)

/*******************************************************************/

#define MME_TRANSFORMER_HANDLE_PREFIX 0x40800000

/* Pack and unpack command identifiers */
#define MME_CMDID_PACK(tid, cid, pIssue) \
	( (*(pIssue) = (*(pIssue) >= 0xffff ? 0 : *(pIssue)+1)), \
	  (((tid) & 255) <<24) + ((cid)<<16) + *(pIssue) )
#define MME_CMDID_GET_TRANSFORMER(cmdid) (((cmdid) >> 24) & 255)
#define MME_CMDID_GET_COMMAND(cmdid)     (((cmdid) >> 16) & 255)
#define MME_CMDID_GET_ISSUE(cmdid)       ((cmdid) & 0xFFFF)

/*******************************************************************/

#if defined DEBUG_DUMP
#if DEBUG_DUMP

#define DUMP_FIELD(s, f, d) printf( "Field: %s: " f "\n", #d, s->d )

__inline static void DumpInitMessage(TransformerInitMessage *message) {
	DUMP_FIELD(message, "%d", id);
	DUMP_FIELD(message, "%d", messageSize);
	DUMP_FIELD(message, "%s", transformerType);
	DUMP_FIELD(message, "%d", priority);
	DUMP_FIELD(message, "%d", result);
	DUMP_FIELD(message, "%s", portName);
	DUMP_FIELD(message, "0x%08x", mmeHandle);
	DUMP_FIELD(message, "%d", numCommandQueueEntries);
}

__inline static void DumpTransformMessage(TransformerTransformMessage *message) {
	
	printf("message 0x%08x\n", message);
	DUMP_FIELD(message, "%d", id);
	DUMP_FIELD(message, "%d", messageSize);
	DUMP_FIELD(message, "0x%08x", hostCommand);
	DUMP_FIELD(message, "0x%08x", receiverInstance);
	DUMP_FIELD(message, "0x%08x", command.StructSize);
	DUMP_FIELD(message, "0x%08x", command.CmdCode);
	DUMP_FIELD(message, "0x%08x", command.CmdEnd);
	DUMP_FIELD(message, "0x%08x", command.DueTime);
	DUMP_FIELD(message, "%d", command.NumberInputBuffers);
	DUMP_FIELD(message, "%d", command.NumberOutputBuffers);
	DUMP_FIELD(message, "0x%08x", command.DataBuffers_p);
	DUMP_FIELD(message, "0x%08x", command.CmdStatus.CmdId);
	DUMP_FIELD(message, "%d", command.CmdStatus.State);
	DUMP_FIELD(message, "0x%08x", command.CmdStatus.ProcessedTime);
	DUMP_FIELD(message, "%d", command.CmdStatus.Error);
}

__inline static void DumpReplyCode(char x) {

	switch (x) {
	case TMESSID_TRANSFORM:
		printf("ReplyType TMESSID_TRANSFORM\n");
	case TMESSID_ABORT: 
		printf("ReplyType TMESSID_ABORT\n");
	case TMESSID_STARVATION:
		printf("ReplyType TMESSID_STARVATION\n");
	case TMESSID_TERMINATE:
		printf("ReplyType TMESSID_TERMINATE\n");
	default:
		printf("ReplyType UNKNOWN\n") ;
	}
}
#endif
#endif

#endif /* MMEP_H */
