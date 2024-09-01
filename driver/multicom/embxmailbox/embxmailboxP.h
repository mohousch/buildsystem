/*
 * EMBX_MailboxP.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Private header file for the hardware mailbox mangager.
 */

#ifndef EMBX_MailboxP_h
#define EMBX_MailboxP_h

#include "embx_osinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EMBX_Mailbox {
	volatile EMBX_UINT reg;
};

struct EMBX_MailboxLock {
	volatile EMBX_UINT lock;
};

typedef struct EMBX_InterruptHandler {
void (*handler)(void *);
	void *param;
} EMBX_InterruptHandler_t;

typedef struct EMBX_LocalMailboxSet {
	struct EMBX_LocalMailboxSet    *next;
	EMBX_UINT			numValid;
	EMBX_BOOL			requiresActivating;
	EMBX_EVENT			activateEvent;
	int		                intLevel;
	int		                intNumber;
#if defined __OS21__
	interrupt_t		       *intHandle;
#elif defined __WINCE__
        HANDLE                          intEvent;
        EMBX_BOOL                       intExit;
        HANDLE                          intThread;
        DWORD                           intThreadId;
#endif
	EMBX_InterruptHandler_t         intTable[4];
	EMBX_Mailbox_t		       *mailboxes[4];
	EMBX_BOOL			inUse[4];
} EMBX_LocalMailboxSet_t;

typedef struct EMBX_RemoteMailboxSet {
	struct EMBX_RemoteMailboxSet    *next;
	EMBX_UINT                        numValid;
	EMBX_Mailbox_t			*mailboxes[4];
	EMBX_BOOL			 requiresActivating;
} EMBX_RemoteMailboxSet_t;

typedef struct EMBX_MailboxLockSet {
	struct EMBX_MailboxLockSet *next;
	EMBX_MailboxLock_t         *spinlocks[32];
	EMBX_BOOL                   owner[32];
	EMBX_Mailbox_t             *inUseMask;
} EMBX_MailboxLockSet_t;

enum {
	/* abolute offsets */
	_EMBX_MAILBOX_SET1		= 0x004 / 4,
	_EMBX_MAILBOX_SET2		= 0x104 / 4,
	_EMBX_MAILBOX_LOCKS		= 0x200 / 4,

	/* deltas between types of register */
	_EMBX_MAILBOX_SIZEOF		= 0x004 / 4,
	_EMBX_MAILBOX_LOCK_SIZEOF	= 0x004 / 4,

	/* offsets of various registers from a EMBX_Mailbox_t * */
	_EMBX_MAILBOX_STATUS		= 0x000 / 4,
	_EMBX_MAILBOX_STATUS_SET	= 0x020 / 4,
	_EMBX_MAILBOX_STATUS_CLEAR      = 0x040 / 4,
	_EMBX_MAILBOX_ENABLE            = 0x060 / 4,
	_EMBX_MAILBOX_ENABLE_SET	= 0x080 / 4,
	_EMBX_MAILBOX_ENABLE_CLEAR	= 0x0a0 / 4
};

#ifdef __cplusplus
}
#endif

#endif /* EMBX_MailboxP_h */
