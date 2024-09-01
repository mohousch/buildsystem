/*
 * EMBX_Mailbox.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * An API to manage the hardware mailbox resources.
 */

#ifndef EMBX_Mailbox_h
#define EMBX_Mailbox_h

#include "embx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Represents with a local or remote mailbox register. */
typedef struct EMBX_Mailbox EMBX_Mailbox_t; 

/* Represents a single spinlock bit in a mailbox. */
typedef struct EMBX_MailboxLock EMBX_MailboxLock_t;

/* Mechanism to control initialization */
typedef enum EMBX_Mailbox_Flags {
	/* select while four enable/status set is the local one */
	EMBX_MAILBOX_FLAGS_SET1  = 0x01,
	EMBX_MAILBOX_FLAGS_SET2  = 0x02,

	/* this mailbox supports spinlocks */
	EMBX_MAILBOX_FLAGS_LOCKS = 0x04,

	/* handle mailboxes that are not always addressable */
	EMBX_MAILBOX_FLAGS_PASSIVE = 0x08,
	EMBX_MAILBOX_FLAGS_ACTIVATE = 0x10,

	/* aliases to match documentation in ST40 Sys. Arch. Manual */
	EMBX_MAILBOX_FLAGS_ST20 = EMBX_MAILBOX_FLAGS_SET1,
	EMBX_MAILBOX_FLAGS_ST40 = EMBX_MAILBOX_FLAGS_SET2
} EMBX_Mailbox_Flags_t;



EMBX_ERROR EMBX_Mailbox_Init(void);
EMBX_VOID  EMBX_Mailbox_Deinit(void);
EMBX_ERROR EMBX_Mailbox_Register(void *pMailbox, int intNumber, int intLevel, EMBX_Mailbox_Flags_t flags); 
EMBX_VOID  EMBX_Mailbox_Deregister(void *pMailbox);

EMBX_ERROR EMBX_Mailbox_Alloc(void (*handler)(void *), void *param, EMBX_Mailbox_t **pMailbox);
EMBX_ERROR EMBX_Mailbox_Synchronize(EMBX_Mailbox_t *local, EMBX_UINT token, EMBX_Mailbox_t **pRemote);
EMBX_VOID  EMBX_Mailbox_Free(EMBX_Mailbox_t *mailbox);
EMBX_ERROR EMBX_Mailbox_UpdateInterruptHandler(EMBX_Mailbox_t *mailbox, void (*handler)(void *), void *param);

EMBX_VOID  EMBX_Mailbox_InterruptEnable(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);
EMBX_VOID  EMBX_Mailbox_InterruptDisable(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);
EMBX_VOID  EMBX_Mailbox_InterruptClear(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);
EMBX_VOID  EMBX_Mailbox_InterruptRaise(EMBX_Mailbox_t *mailbox, EMBX_UINT bit);

EMBX_UINT  EMBX_Mailbox_StatusGet(EMBX_Mailbox_t *mailbox);
EMBX_VOID  EMBX_Mailbox_StatusSet(EMBX_Mailbox_t *mailbox, EMBX_UINT value);
EMBX_VOID  EMBX_Mailbox_StatusMask(EMBX_Mailbox_t *mailbox, EMBX_UINT set, EMBX_UINT clear);

EMBX_ERROR EMBX_Mailbox_AllocLock(EMBX_MailboxLock_t **pLock);
EMBX_VOID  EMBX_Mailbox_FreeLock(EMBX_MailboxLock_t *lock);

#ifdef EMBX_VERBOSE
EMBX_VOID  EMBX_Mailbox_TakeLock(EMBX_MailboxLock_t *lock);
EMBX_VOID  EMBX_Mailbox_ReleaseLock(EMBX_MailboxLock_t *lock);
#else
#define EMBX_Mailbox_TakeLock(lock) \
	do while (*((volatile int *)lock)) { volatile int i=0; while (i<1000) i++; } while(0)
#define EMBX_Mailbox_ReleaseLock(lock) (*((volatile int *) lock) = 0)
#endif

EMBX_ERROR EMBX_Mailbox_GetSharedHandle(EMBX_MailboxLock_t *lock, EMBX_UINT *pHandle);
EMBX_ERROR EMBX_Mailbox_GetLockFromHandle(EMBX_UINT handle, EMBX_MailboxLock_t **pLock);

#ifdef __cplusplus
}
#endif

#endif
