/*
 * embx_mailbox.c
 * DO NOT RENAME: this file must not be called embxmailbox.c or the object
 * file it produces will name clash with the linked Linux kernel module.
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * 
 */

#include <embx_debug.h>
#include <embx_types.h>
#include <embx_osinterface.h>

#include "embxmailbox.h"
#include "embxmailboxP.h"

#ifdef __WINCE__
//#include "SetThreadTag.h"
#endif

#ifndef EMBX_INFO_MAILBOX
#define EMBX_INFO_MAILBOX   0
#endif

#ifndef EMBX_INFO_INTERRUPT
#define EMBX_INFO_INTERRUPT 0
#endif

#if defined(__LINUX__) && defined(__KERNEL__) && defined(MODULE)

/* module information */

MODULE_DESCRIPTION("EMBX Utility Library to manage the hardware mailbox.");
MODULE_AUTHOR("STMicroelectronics Ltd.");
#ifdef MULTICOM_GPL
MODULE_LICENSE("GPL");
#else
MODULE_LICENSE("(c) 2002-2003 STMicroelectronics. All rights reserved.");
#endif /* MULTICOM_GPL */

/* configurable parameters */

static char     * mailbox0;
module_param     (mailbox0, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox0, "Configuration string of the form `addr:irq:flags...'");

static char     * mailbox1;
module_param     (mailbox1, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox1, "Configuration string of the form `addr:irq:flags...'");

static char     * mailbox2;
module_param     (mailbox2, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox2, "Configuration string of the form `addr:irq:flags...'");

static char     * mailbox3;
module_param     (mailbox3, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox3, "Configuration string of the form `addr:irq:flags...'");

EXPORT_SYMBOL( EMBX_Mailbox_Init );
EXPORT_SYMBOL( EMBX_Mailbox_Deinit );
EXPORT_SYMBOL( EMBX_Mailbox_Register );
EXPORT_SYMBOL( EMBX_Mailbox_Deregister );
EXPORT_SYMBOL( EMBX_Mailbox_Alloc );
EXPORT_SYMBOL( EMBX_Mailbox_Synchronize );
EXPORT_SYMBOL( EMBX_Mailbox_Free );
EXPORT_SYMBOL( EMBX_Mailbox_UpdateInterruptHandler );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptEnable );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptDisable );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptClear );
EXPORT_SYMBOL( EMBX_Mailbox_InterruptRaise );
EXPORT_SYMBOL( EMBX_Mailbox_StatusGet );
EXPORT_SYMBOL( EMBX_Mailbox_StatusSet );
EXPORT_SYMBOL( EMBX_Mailbox_StatusMask );
EXPORT_SYMBOL( EMBX_Mailbox_AllocLock );
EXPORT_SYMBOL( EMBX_Mailbox_FreeLock );
#ifdef EMBX_VERBOSE
EXPORT_SYMBOL( EMBX_Mailbox_TakeLock );
EXPORT_SYMBOL( EMBX_Mailbox_ReleaseLock );
#endif
EXPORT_SYMBOL( EMBX_Mailbox_GetSharedHandle );
EXPORT_SYMBOL( EMBX_Mailbox_GetLockFromHandle );

static int parseMailboxString(char *mailbox, EMBX_VOID **pAddr, EMBX_INT *pIrq, EMBX_Mailbox_Flags_t *pFlags)
{
    struct { char *name; EMBX_Mailbox_Flags_t flag; } *pLookup, lookup[] = {
	{ "set1",     EMBX_MAILBOX_FLAGS_SET1     },
	{ "st20",     EMBX_MAILBOX_FLAGS_ST20     },
	{ "set2",     EMBX_MAILBOX_FLAGS_SET2     },
	{ "st40",     EMBX_MAILBOX_FLAGS_ST40     },
	{ "activate", EMBX_MAILBOX_FLAGS_ACTIVATE },
	{ "passive",  EMBX_MAILBOX_FLAGS_PASSIVE  },
	{ NULL, 0 }
    };

    EMBX_Info(EMBX_INFO_MAILBOX, (">>>parseMailboxString(\"%s\", ...)\n", mailbox));
    
    if (NULL == mailbox) {
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<parseMailboxString = -EINVAL:1\n"));
	return -EINVAL;
    }

    /* parse the numeric values (do we need to do any checking on pAddr */
    if (2 != sscanf(mailbox, "0x%x:%d", (unsigned *) pAddr, pIrq)) {
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<parseMailboxString = -EINVAL:2\n"));
	return -EINVAL;
    }

    /* scan for the flags */
    *pFlags = 0;
    for (pLookup = lookup; NULL != pLookup->name; pLookup++) {
	if (strstr(mailbox, pLookup->name)) {
	    *pFlags |= pLookup->flag;
	}
    }
    
    EMBX_Info(EMBX_INFO_MAILBOX, ("<<<parseMailboxString = 0\n"));
    return 0;
}

int __init embxmailbox_init( void )
{
    char *mailbox[4] = { mailbox0, mailbox1, mailbox2, mailbox3 };
    EMBX_VOID *base[4];
    EMBX_INT  irq[4];
    EMBX_Mailbox_Flags_t flags[4];
    int i, j;
    EMBX_ERROR err;

    /* check that our module parameters make sense before we try to
     * bring everything up.
     */
    for (i=0, j=0; i<4; i++) {
	if (mailbox[i]) {
	    if (0 == parseMailboxString(mailbox[i], base+j, irq+j, flags+j)) {
		j++;
	    } else {
		return -EINVAL;
	    }
	}
    }

#if defined(__LINUX__) && defined(__KERNEL__) && defined(CONFIG_32BIT)
    for (i=0; i<j; i++) {
	void *iomap;

	/* MULTICOM_32BIT_SUPPORT: Must map the physical Mailbox register addresses into kernel space */
	iomap = ioremap_nocache((unsigned) base[i], PAGE_SIZE);

	EMBX_Info(EMBX_INFO_MAILBOX, ("EMBXMAILBOX_INIT mapped mbox %p to %p\n", base[i], iomap)); 
	
	if (iomap == NULL)
	    return -EINVAL;
	
	base[i] = iomap;
    }
#endif

    /* now bring everything up */
    err = EMBX_Mailbox_Init();
    for (i=0; i<j && EMBX_SUCCESS == err; i++) {
	err = EMBX_Mailbox_Register(base[i], irq[i], -1, flags[i]);
    }

    return (EMBX_SUCCESS == err ? 0 : -ENOMEM);
}

void __exit embxmailbox_deinit( void )
{
    EMBX_Mailbox_Deinit();
    return;
}

module_init(embxmailbox_init);
module_exit(embxmailbox_deinit);

#endif

/* TODO: these should be pushed into the shell headers */
typedef unsigned long EMBX_uint32_t;

/*
 * static driver state
 */
static EMBX_BOOL                  _alreadyInitialized;
static EMBX_LocalMailboxSet_t    *_mailboxLocalHead;
static EMBX_MUTEX                 _mailboxLocalLock;
static EMBX_RemoteMailboxSet_t   *_mailboxRemoteHead;
static EMBX_MUTEX                 _mailboxRemoteLock;
static EMBX_MailboxLockSet_t     *_mailboxSpinlockHead;
static EMBX_MUTEX                 _mailboxSpinlockLock;

/*
 * interrupt handler to broker multiple interrupt handlers
 */
#if defined __LINUX__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define IRQ_HANDLED 1
#define IRQ_NONE 0
#endif
static irqreturn_t interruptDispatcher(int irq, void *param)
#elif defined __OS21__
#define IRQ_HANDLED OS21_SUCCESS
#define IRQ_NONE OS21_FAILURE
static int  interruptDispatcher(void *param)
#else
static void interruptDispatcher(void *param)
#endif
{
#if defined __OS21__ || defined __LINUX__
	int res = IRQ_NONE;
#endif
	EMBX_LocalMailboxSet_t *lms = (EMBX_LocalMailboxSet_t *) param;
	int i;

        EMBX_Info( EMBX_INFO_INTERRUPT, (">>>interruptDispatcher()\n"));

	for (i=0; i<4; i++) {
		if (lms->inUse[i]) {
			EMBX_uint32_t enables = lms->mailboxes[i][_EMBX_MAILBOX_ENABLE].reg;
			EMBX_uint32_t status  = lms->mailboxes[i][_EMBX_MAILBOX_STATUS].reg;
			
			if (enables & status) {
				lms->intTable[i].handler(lms->intTable[i].param);
#if defined __OS21__ || defined __LINUX__
				res = IRQ_HANDLED;
#endif
			}
		}
	}

	EMBX_Info( EMBX_INFO_INTERRUPT, ("<<<interruptDispatcher\n"));

#if defined __OS21__ || defined __LINUX__
	return res;
#endif
}

#if defined __WINCE__
/*
 * WinCE interrupt service thread to just call interruptDispatcher()
 */
static void interruptServiceThread (void *param)
{
	EMBX_LocalMailboxSet_t *lms = (EMBX_LocalMailboxSet_t *) param;

	while (1) {
		WaitForSingleObject(lms->intEvent, INFINITE);

		if (lms->intExit) {
			return;
		}

		/* The WinCE port uses a critical section to implement an approximation
		 * of interrupt lock. This will only work if we enter this critical
		 * section before dispatching the interrupt.
		 */
		EMBX_OS_INTERRUPT_LOCK();
		interruptDispatcher (lms);
		EMBX_OS_INTERRUPT_UNLOCK();

		InterruptDone (lms->intNumber);
	}
}
#endif

/*
 * dual purpose secondary interrupt handler
 *
 * primarily we deal with ensuring that activation flags are handled
 * correctly. however we also serve the purpose of making the system
 * explode if illegal interrupts fire.
 */
static void activateHandler(void *param)
{
	EMBX_LocalMailboxSet_t *lms = (EMBX_LocalMailboxSet_t *) param;

	EMBX_Assert(lms->requiresActivating);
	lms->requiresActivating = 0;

	lms->mailboxes[0][_EMBX_MAILBOX_ENABLE_CLEAR].reg = 0x80000000;
	lms->mailboxes[0][_EMBX_MAILBOX_STATUS_CLEAR].reg = 0x80000000;

	EMBX_OS_EVENT_POST(&(lms->activateEvent));
}

/*
 * register a mailbox capable of generating interrupts on this CPU and
 * attach a primary handler to that interrupt
 */
static EMBX_ERROR registerLocalMailbox(EMBX_UINT *localBase, 
                                       EMBX_UINT numMailboxes, 
				       EMBX_BOOL requiresActivating, 
				       EMBX_UINT intNumber, 
				       EMBX_UINT intLevel)
{
	EMBX_LocalMailboxSet_t *lms;
	unsigned int i;
	int res;
	
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>registerLocalMailbox(%p, %d, %d, %d, %d)\n", localBase, numMailboxes, requiresActivating, intNumber, intLevel));

	EMBX_Assert(localBase);
	EMBX_Assert(numMailboxes);

	lms = (EMBX_LocalMailboxSet_t *) EMBX_OS_MemAlloc(sizeof(EMBX_LocalMailboxSet_t));
	if (!lms) {
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerLocalMailbox = EMBX_NOMEM#0\n"));
		return EMBX_NOMEM;
	}

	lms->numValid = numMailboxes;
	lms->requiresActivating = requiresActivating;
	lms->intLevel = intLevel;
	lms->intNumber = intNumber;
	if (!EMBX_OS_EVENT_INIT(&(lms->activateEvent))) {
		EMBX_OS_MemFree((void *) lms);
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerLocalMailbox = EMBX_NOMEM#1\n"));
		return EMBX_NOMEM;
	}

	for (i=0; i<4; i++) {
		lms->intTable[i].handler = activateHandler;
		lms->intTable[i].param   = (void *) lms;
		lms->mailboxes[i]        = (EMBX_Mailbox_t *) (localBase + (i * _EMBX_MAILBOX_SIZEOF));

		if (!lms->requiresActivating && i<numMailboxes) {
			lms->mailboxes[i][_EMBX_MAILBOX_ENABLE].reg = 0;
			lms->mailboxes[i][_EMBX_MAILBOX_STATUS].reg = 0;
		}

		lms->inUse[i]            = !(i < numMailboxes);
	}

#if defined __OS21__
	lms->intHandle = interrupt_handle(intNumber);
	if(!lms->intHandle)
	{
		EMBX_DebugMessage(("registerLocalMailbox - OS21 interrupt name not recognised\n"));
		EMBX_DebugMessage(("    mailbox base = 0x%lx   interrupt name = 0x%lx\n",(unsigned long)localBase,(unsigned long)intNumber));

	        EMBX_OS_EVENT_DESTROY(&(lms->activateEvent));
	        EMBX_OS_MemFree((void *) lms);
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerLocalMailbox = EMBX_INVALID_ARGUMENT#0\n"));
		return EMBX_INVALID_ARGUMENT;
	}

	res  = interrupt_install_shared(lms->intHandle, interruptDispatcher, (void *) lms);
	res += interrupt_enable(lms->intHandle);
	EMBX_Assert(0 == res);
#elif defined __WINCE__
	res = 0;

	/* cannot use EMBX_OS_EVENT_INIT since WinCE requires this handle to be an event */
	lms->intEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!lms->intEvent) {
	        EMBX_OS_EVENT_DESTROY(&(lms->activateEvent));
	        EMBX_OS_MemFree((void *) lms);
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerLocalMailbox = EMBX_NOMEM#2\n"));
		return EMBX_NOMEM;
	}

	lms->intExit = FALSE;

	EMBX_Info(EMBX_INFO_MAILBOX, ("   registerLocalMailbox: InterruptInitialize(0x%08x, 0x%08x, 0, 0)\n", lms->intNumber, lms->intEvent));
	if (!InterruptInitialize (lms->intNumber,
				  lms->intEvent,
				  NULL,
				  0)) {
		EMBX_Info(EMBX_INFO_MAILBOX, ("   registerLocalMailbox: InterruptInitialize FAILED -  err = %d\n", GetLastError()));
		CloseHandle(lms->intEvent);
	        EMBX_OS_EVENT_DESTROY(&(lms->activateEvent));
	        EMBX_OS_MemFree((void *) lms);
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerLocalMailbox = EMBX_INVALID_ARGUMENT#1\n"));
		return EMBX_INVALID_ARGUMENT;
	}

	lms->intThread = CreateThread (NULL,
				       (DWORD) 0,
				       (LPTHREAD_START_ROUTINE) interruptServiceThread,
				       (LPVOID) lms,
				       (DWORD) 0,
				       &lms->intThreadId);

	if (lms->intThread == NULL) {
	        InterruptDisable(lms->intNumber);
		EMBX_DebugMessage(("registerLocalMailbox - CreateThread() failed\n"));
		CloseHandle(lms->intEvent);
	        EMBX_OS_EVENT_DESTROY(&(lms->activateEvent));
	        EMBX_OS_MemFree((void *) lms);
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerLocalMailbox = EMBX_NOMEM#3\n"));
		return EMBX_NOMEM;
	}

	//SetThreadTag(lms->intThread, "embx_mailbox_thread");
	CeSetThreadPriority (lms->intThread, EMBX_DEFAULT_MAILBOX_PRIORITY);
#elif defined __SH4__ && defined __LINUX__
	res = request_irq(intNumber, interruptDispatcher, 0, "mailbox", (void *) lms);
	EMBX_Assert(0 == res);
#else
#error Unsupported OS/CPU
#endif

	EMBX_OS_MUTEX_TAKE(&_mailboxLocalLock);
	lms->next = _mailboxLocalHead;
	_mailboxLocalHead = lms;
	EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerLocalMailbox = EMBX_SUCCESS\n"));
	return EMBX_SUCCESS;
}

static void deregisterLocalMailbox(EMBX_UINT *localBase)
{
	EMBX_LocalMailboxSet_t *prev, *lms;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>deregisterLocalMailbox(%p)\n", localBase));

	EMBX_OS_MUTEX_TAKE(&_mailboxLocalLock);

	/* hunt for the correct mailbox */
	for (prev=NULL, lms=_mailboxLocalHead;
	     lms && (localBase != (EMBX_UINT *) (lms->mailboxes[0]));
	     prev=lms, lms=prev->next) {}
	
	/* if we found a matching element then unlink it from
	 * the list, remote its interrupt handler and free its
	 * storage
	 */
	if (lms) {
		*(prev ? &prev->next : &_mailboxLocalHead) = lms->next;

#if defined __OS21__
		interrupt_disable(lms->intHandle);
		interrupt_uninstall(lms->intHandle);
#elif defined __WINCE__
		lms->intExit = TRUE;
		SetEvent(lms->intEvent);
		EMBX_OS_Delay (500);
		CloseHandle (lms->intThread);
		CloseHandle (lms->intEvent);
#elif defined __SH4__ && defined __LINUX__
		free_irq(lms->intNumber, (void *) lms);
#else
#error Unsupported CPU/OS
#endif

		EMBX_OS_EVENT_DESTROY(&(lms->activateEvent));
		EMBX_OS_MemFree((EMBX_VOID *) lms);
	}

	EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<deregisterLocalMailbox\n"));
}

static EMBX_ERROR findLocalMailbox(
		EMBX_Mailbox_t *mailbox, EMBX_LocalMailboxSet_t **pSet, int *pId)
{
	EMBX_LocalMailboxSet_t *lms;
	int i;

	/* This function must be called with the mailbox local lock taken */
	for (lms = _mailboxLocalHead; lms; lms = lms->next) {
		for (i=0; i<4; i++) {
			if (lms->mailboxes[i] == mailbox) {
				*pSet = lms;
				*pId  = i;
				return EMBX_SUCCESS;
			}
		}
	}

	return EMBX_INVALID_ARGUMENT;
}

static EMBX_ERROR registerRemoteMailbox(EMBX_UINT *remoteBase, 
                                        EMBX_UINT numMailboxes, 
					EMBX_BOOL requiresActivating)
{
	EMBX_RemoteMailboxSet_t *rms;
	int i;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>registerRemoteMailbox(%p, %d, %d)\n", remoteBase, numMailboxes, requiresActivating));

	EMBX_Assert(remoteBase);
	EMBX_Assert(numMailboxes);

	rms = (EMBX_RemoteMailboxSet_t *) EMBX_OS_MemAlloc(sizeof(EMBX_RemoteMailboxSet_t));
	if (!rms) {
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerRemoteMailbox = EMBX_NOMEM#0\n"));
		return EMBX_NOMEM;
	}

	rms->numValid = numMailboxes;
	rms->requiresActivating = requiresActivating;
	for (i=0; i<4; i++) {
		rms->mailboxes[i] = (EMBX_Mailbox_t *) (remoteBase + (i * _EMBX_MAILBOX_SIZEOF));

	}

	EMBX_OS_MUTEX_TAKE(&_mailboxRemoteLock);
	rms->next = _mailboxRemoteHead;
	_mailboxRemoteHead = rms;
	EMBX_OS_MUTEX_RELEASE(&_mailboxRemoteLock);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<registerRemoteMailbox = EMBX_SUCCESS\n"));
	return EMBX_SUCCESS;
}

static void deregisterRemoteMailbox(EMBX_UINT *remoteBase)
{
	EMBX_RemoteMailboxSet_t *prev, *curr;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>deregisterRemoteMailbox(%p)\n", remoteBase));
	EMBX_OS_MUTEX_TAKE(&_mailboxRemoteLock);

	/* hunt for the correct mailbox */
	for (prev=NULL, curr=_mailboxRemoteHead;
	     curr && (remoteBase != (EMBX_UINT *) (curr->mailboxes[0]));
	     prev=curr, curr=prev->next) {}
	
	/* if we found a matching element then unlink it from
	 * the list and free its storage
	 */
	if (curr) {
		*(prev ? &prev->next : &_mailboxRemoteHead) = curr->next;
		EMBX_OS_MemFree((EMBX_VOID *) curr);
	}

	EMBX_OS_MUTEX_RELEASE(&_mailboxRemoteLock);
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<deregisterRemoteMailbox\n"));
}

static EMBX_ERROR registerSpinlocks(EMBX_UINT *lockBase)
{
	EMBX_MailboxLockSet_t *mls;
	int i;

	EMBX_Assert(lockBase);

	mls = (EMBX_MailboxLockSet_t *) EMBX_OS_MemAlloc(sizeof(EMBX_MailboxLockSet_t));
	if (!mls) {
		return EMBX_NOMEM;
	}

	for (i=0; i<32; i++) {
		mls->spinlocks[i] = (EMBX_MailboxLock_t *) (lockBase + (i * _EMBX_MAILBOX_LOCK_SIZEOF));
		mls->owner[i] = EMBX_FALSE;

	}

	EMBX_OS_MUTEX_TAKE(&_mailboxSpinlockLock);
	mls->next = _mailboxSpinlockHead;
	mls->inUseMask = (EMBX_Mailbox_t *) (lockBase - _EMBX_MAILBOX_LOCKS + 
	                                     _EMBX_MAILBOX_SET1 + (3 * _EMBX_MAILBOX_SIZEOF));
	_mailboxSpinlockHead = mls;
	EMBX_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);

	return EMBX_SUCCESS;
}

static void deregisterSpinlocks(EMBX_UINT *lockBase)
{
	EMBX_MailboxLockSet_t *prev, *curr;

	EMBX_OS_MUTEX_TAKE(&_mailboxSpinlockLock);

	/* hunt for the correct mailbox */
	for (prev=NULL, curr=_mailboxSpinlockHead;
	     curr && (lockBase != (EMBX_UINT *) (curr->spinlocks[0]));
	     prev=curr, curr=prev->next) {}
	
	/* if we found a matching element then unlink it from
	 * the list and free its storage
	 */
	if (curr) {
		*(prev ? &prev->next : &_mailboxSpinlockHead) = curr->next;
		EMBX_OS_MemFree((EMBX_VOID *) curr);
	}

	EMBX_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);
}

static EMBX_ERROR findSpinlock(
		EMBX_MailboxLock_t *spinlock, EMBX_MailboxLockSet_t **pSet, int *pId)
{
	EMBX_MailboxLockSet_t *mls;
	int i;

	/* This function must be called with the mailbox spinlock lock taken */

	for (mls = _mailboxSpinlockHead; mls; mls = mls->next) {
		for (i=0; i<32; i++) {
			if (mls->spinlocks[i] == spinlock) {
				*pSet = mls;
				*pId  = i;
				return EMBX_SUCCESS;
			}
		}
	}

	return EMBX_INVALID_ARGUMENT;
}

/*
 * scan all remote mailboxes looking for the magic token 
 */
static EMBX_Mailbox_t *scanRemoteMailboxes(EMBX_UINT token)
{
	EMBX_RemoteMailboxSet_t *rms;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>scanRemoteMailboxes()\n"));

	/* This function must be called with the mailbox remote lock taken */

	for (rms = _mailboxRemoteHead; rms; rms = rms->next) {
		unsigned int i;

		/* if this mailbox requires activating then activate it (generate
		 * an interrupt before scanning
		 */
		if (rms->requiresActivating) {
			rms->mailboxes[0][_EMBX_MAILBOX_ENABLE_SET].reg = 0x80000000;
			rms->mailboxes[0][_EMBX_MAILBOX_STATUS_SET].reg = 0x80000000;
			rms->requiresActivating = EMBX_FALSE;
		}

		for (i=0; i<rms->numValid; i++) {
			EMBX_Info(EMBX_INFO_MAILBOX, ("   scanning 0x%08x = 0x%08x\n",
				      (unsigned) &rms->mailboxes[i][_EMBX_MAILBOX_ENABLE].reg,
			              rms->mailboxes[i][_EMBX_MAILBOX_ENABLE].reg));

			if (token == rms->mailboxes[i][_EMBX_MAILBOX_ENABLE].reg) {
				EMBX_Info(EMBX_INFO_MAILBOX, ("<<<scanRemoteMailboxes() = 0x%08x\n", (unsigned) rms->mailboxes[i]));
				return rms->mailboxes[i];
			}
		}
	}

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<scanRemoteMailboxes() = NULL\n"));
	return NULL;
}

static EMBX_ERROR getInterruptState(EMBX_Mailbox_t *mailbox, void (**pHandler)(void *), void **pParam)
{
	EMBX_ERROR err;
	EMBX_LocalMailboxSet_t *lms;
	int id;

	/* This function must be called with the mailbox local lock taken */

	err = findLocalMailbox(mailbox, &lms, &id);

	if (EMBX_SUCCESS == err) {
		*pHandler = lms->intTable[id].handler;
		*pParam   = lms->intTable[id].param;
	}

	return err;
}

static EMBX_ERROR setInterruptState(EMBX_Mailbox_t *mailbox, void (*handler)(void *), void *param)
{
	EMBX_ERROR err;
	EMBX_LocalMailboxSet_t *lms;
	int id;

	/* This function must be called with the mailbox local lock taken */

	err = findLocalMailbox(mailbox, &lms, &id);

	if (EMBX_SUCCESS == err) {
		lms->intTable[id].handler = handler;
		lms->intTable[id].param   = param;
	}

	return err;
}

/*
 * This function registers a set of mailbox registers (e.g. eight 
 * enable/status pairs). This set must the be the set of registers that
 * can generate an interrupt on the local processor. In order that the
 * mailbox manager can handle the interrupt it must be notified of the
 * interrupt number and (depending on the CPU) the interrupt level.
 *
 * THIS FUNCTION IS NOT THREAD-SAFE
 *
 * Errors:
 *   EMBX_NOMEM
 *   EMBX_ALREADY_INITIALIZED
 */
EMBX_ERROR EMBX_Mailbox_Init(void)
{
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Init()\n"));
	if (_alreadyInitialized) {
		EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Init = EMBX_ALREADY_INITIALIZED\n"));
		return EMBX_ALREADY_INITIALIZED;
	}

	if (!EMBX_OS_MUTEX_INIT(&_mailboxLocalLock)) {
		EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Init = EMBX_NOMEM:0\n"));
		return EMBX_NOMEM;
	}

	if (!EMBX_OS_MUTEX_INIT(&_mailboxRemoteLock)) {
		EMBX_OS_MUTEX_DESTROY(&_mailboxLocalLock);
		EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Init = EMBX_NOMEM:1\n"));
		return EMBX_NOMEM;
	}

	if (!EMBX_OS_MUTEX_INIT(&_mailboxSpinlockLock)) {
		EMBX_OS_MUTEX_DESTROY(&_mailboxRemoteLock);
		EMBX_OS_MUTEX_DESTROY(&_mailboxLocalLock);
		EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Init = EMBX_NOMEM:2\n"));
		return EMBX_NOMEM;
	}

	_alreadyInitialized = EMBX_TRUE;
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Init = EMBX_SUCCESS\n"));
	return EMBX_SUCCESS;
}

void EMBX_Mailbox_Deinit (void)
{
  EMBX_LocalMailboxSet_t  *lms, *nlms;
  EMBX_RemoteMailboxSet_t *rms, *nrms;
  EMBX_MailboxLockSet_t   *mls, *nmls;

  EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Deinit()\n"));

  /* Deregister all of the local mailboxes */
  for (lms = _mailboxLocalHead; lms != NULL; lms = nlms) {
    nlms = lms->next;
    deregisterLocalMailbox ((EMBX_UINT *) lms->mailboxes[0]);
  }

  /* Deregister all of the remote mailboxes */
  for (rms = _mailboxRemoteHead; rms != NULL; rms = nrms) {
    nrms = rms->next;
    deregisterRemoteMailbox ((EMBX_UINT *)rms->mailboxes[0]);
  }

  /* Deregister all of the spin locks */
  for (mls = _mailboxSpinlockHead; mls != NULL; mls = nmls) {
    nmls = mls->next;
    deregisterSpinlocks ((EMBX_UINT *)mls->spinlocks[0]);
  }

  /* Destroy all of the locks created in EMBX_MailboxInit() */
  EMBX_OS_MUTEX_DESTROY(&_mailboxLocalLock);
  EMBX_OS_MUTEX_DESTROY(&_mailboxRemoteLock);
  EMBX_OS_MUTEX_DESTROY(&_mailboxSpinlockLock);

  /* Indicate that we are not initialised */
  _alreadyInitialized = EMBX_FALSE;

  EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Deinit = EMBX_SUCCESS\n"));
}

EMBX_ERROR EMBX_Mailbox_Register(void *pMailbox, int intNumber, int intLevel, EMBX_Mailbox_Flags_t flags)
{
	EMBX_UINT   numMailboxes;
	EMBX_UINT  *local;
	EMBX_UINT  *remoteSet1;
	EMBX_UINT  *remoteSet2;
	EMBX_UINT  *locks;
	EMBX_BOOL   localNeedsActivating;
	EMBX_BOOL   remoteNeedsActivating;
	EMBX_ERROR  err = EMBX_SUCCESS;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Register(0x%08x, %d, %d, %d)\n", (unsigned) pMailbox, intNumber, intLevel, flags));
	EMBX_Assert(pMailbox);

	if (flags & EMBX_MAILBOX_FLAGS_SET1) {
		local        = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET1;
		remoteSet1   = NULL;
		remoteSet2   = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET2;
	} else if (flags & EMBX_MAILBOX_FLAGS_SET2) {
		local        = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET2;
		remoteSet1   = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET1;
		remoteSet2   = NULL;
	} else {
		local        = NULL;
		remoteSet1   = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET1;
		remoteSet2   = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET2;
	}

	if (flags & EMBX_MAILBOX_FLAGS_LOCKS) {
		numMailboxes = 3;
		locks        = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_LOCKS;
	} else {
		numMailboxes = 4;
		locks        = NULL;
	}

	localNeedsActivating  = (0 != (flags & EMBX_MAILBOX_FLAGS_PASSIVE));
	remoteNeedsActivating = (0 != (flags & EMBX_MAILBOX_FLAGS_ACTIVATE));

	if (local) {
		err = registerLocalMailbox(local, numMailboxes, 
		                           localNeedsActivating, intNumber, intLevel);
	}

	if (remoteSet1 && (err == EMBX_SUCCESS)) {
		err = registerRemoteMailbox(remoteSet1, numMailboxes, 
					    remoteNeedsActivating);
	}

	if (remoteSet2 && (err == EMBX_SUCCESS)) {
		err = registerRemoteMailbox(remoteSet2, numMailboxes,
					    remoteNeedsActivating);
	}

	if (locks && (err == EMBX_SUCCESS)) {
		err = registerSpinlocks(locks);
	}

	if (err != EMBX_SUCCESS) {
		deregisterLocalMailbox(local);
		deregisterRemoteMailbox(remoteSet1);
		deregisterRemoteMailbox(remoteSet2);
		deregisterSpinlocks(locks);
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Register = %d\n", err));
		return err;
	}

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Register = EMBX_SUCCESS\n"));
	return EMBX_SUCCESS;
}

/*
 * This is a simple function to reclaim any memory allocated my the mailbox
 * manager.
 */
EMBX_VOID EMBX_Mailbox_Deregister(void *pMailbox)
{
	EMBX_UINT *set1, *set2, *locks;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Deregister()\n"));

	set1  = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET1;
	set2  = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_SET2;
	locks = (EMBX_UINT *) pMailbox + _EMBX_MAILBOX_LOCKS;

	deregisterLocalMailbox(set1);
	deregisterLocalMailbox(set2);
	deregisterRemoteMailbox(set1);
	deregisterRemoteMailbox(set2);
	deregisterSpinlocks(locks);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Deregister\n"));
}

/*
 * Allocate a local mailbox register to a particular task. This function will
 * reserve a register and attach a user supplied handler to it. This handler
 * will be called whenever that register is the cause of an interupt.
 *
 * After allocation the mailbox registers will have its status set to zero
 * and have no bits enabled.
 *
 * Errors:
 *   EMBX_INVALID_STATE
 *   EMBX_NOMEM
 */
EMBX_ERROR EMBX_Mailbox_Alloc(void (*handler)(void *), void *param, EMBX_Mailbox_t **pMailbox)
{
	EMBX_LocalMailboxSet_t *lms;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Alloc()\n"));
	EMBX_Assert(handler);
	EMBX_Assert(pMailbox);

	EMBX_OS_MUTEX_TAKE(&_mailboxLocalLock);
	for (lms = _mailboxLocalHead; lms; lms = lms->next) {
		int i;
		for (i=0; i<4; i++) {
			if (!(lms->inUse[i])) {
				lms->inUse[i] = EMBX_TRUE;
				EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);
			
				/* block until this mailbox is activated */
				if (lms->requiresActivating) {
					EMBX_OS_EVENT_WAIT(&(lms->activateEvent));
					EMBX_Assert(0 == lms->requiresActivating);
				}

				lms->intTable[i].handler = handler;
				lms->intTable[i].param = param;

				lms->mailboxes[i][_EMBX_MAILBOX_ENABLE].reg = 0;
				lms->mailboxes[i][_EMBX_MAILBOX_STATUS].reg = 0;

				*pMailbox = lms->mailboxes[i];

				EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Alloc(.., *%p) = EMBX_SUCCESS\n", *pMailbox));
				return EMBX_SUCCESS;
			}
		}
	}

	EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Alloc = EMBX_NOMEM\n"));
	return EMBX_NOMEM;
}

/*
 * Syncronize an allocated local mailbox with a remote mailbox whose 
 * syncronization token matches our own.
 *
 * Errors:
 *   EMBX_INVALID_STATE
 */
struct synchronizeHandlerParamBlock {
	EMBX_Mailbox_t *mailbox;
	EMBX_EVENT event;
};

static void synchronizeHandler(void *param)
{
	struct synchronizeHandlerParamBlock *pb = (struct synchronizeHandlerParamBlock *) param;

	EMBX_Info(EMBX_INFO_INTERRUPT, (">>>synchronizeHandler()\n"));

	if (pb->mailbox[_EMBX_MAILBOX_ENABLE].reg == pb->mailbox[_EMBX_MAILBOX_STATUS].reg)
	{
		/* wake our parent task */
		EMBX_OS_EVENT_POST(&(pb->event));

		/* disable further interrupts on this mailbox (which will prevent)
		 * the value of status changing
		 */
		pb->mailbox[_EMBX_MAILBOX_ENABLE].reg = 0;
	} else {
		EMBX_Info(EMBX_INFO_INTERRUPT, ("   synchronizeHandler: detected spurious write "
				                "(0x%08x vs. 0x%08x)\n",
				                pb->mailbox[_EMBX_MAILBOX_ENABLE].reg,
				                pb->mailbox[_EMBX_MAILBOX_STATUS].reg));

		/* this is a spurious write which we will undo */
		pb->mailbox[_EMBX_MAILBOX_STATUS].reg = 0;
	}

	EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<synchronizeHandler\n"));
}

EMBX_ERROR EMBX_Mailbox_Synchronize(EMBX_Mailbox_t *local, EMBX_UINT token, EMBX_Mailbox_t **pRemote)
{
	EMBX_UINT       oldEnables;
	EMBX_UINT       oldStatus;
	void          (*oldHandler)(void *) = NULL;
	void           *oldParam = NULL;
	EMBX_Mailbox_t *remoteMailbox;
	struct synchronizeHandlerParamBlock paramBlock;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Synchronize(%p, ...)\n", local));

	paramBlock.mailbox = local;
	if (!EMBX_OS_EVENT_INIT(&(paramBlock.event))) {
		EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Synchronize = EMBX_NOMEM\n"));
		return EMBX_NOMEM;
	}

	/* Must take the local lock before disabling interrupts as we
	 * must not de-schedule the task with interrupts disabled
         */
	EMBX_OS_MUTEX_TAKE(&_mailboxLocalLock);

	EMBX_OS_INTERRUPT_LOCK();

	/* backup the original enables register and handler */
	oldEnables = local[_EMBX_MAILBOX_ENABLE].reg;
	oldStatus  = local[_EMBX_MAILBOX_STATUS].reg;
	getInterruptState(local, &oldHandler, &oldParam);

	/* replace the handler and set the enables register to the token */
	local[_EMBX_MAILBOX_ENABLE].reg = token;
	local[_EMBX_MAILBOX_STATUS].reg = 0;
	setInterruptState(local, synchronizeHandler, (void *) &paramBlock);

	EMBX_OS_INTERRUPT_UNLOCK();

	EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);

	EMBX_OS_MUTEX_TAKE(&_mailboxRemoteLock);

	/* scan for a matching token on the remote mailboxes */
	remoteMailbox = scanRemoteMailboxes(token);
	if (remoteMailbox) {
		/* let the other side know that we have successfully found it */
		remoteMailbox[_EMBX_MAILBOX_STATUS].reg = token;
	}

	EMBX_OS_MUTEX_RELEASE(&_mailboxRemoteLock);

	/* wait for the other side to notify us that it has found us */
	EMBX_Info(EMBX_INFO_MAILBOX, ("   waiting for interrupt from partner\n"));
	EMBX_OS_EVENT_WAIT(&(paramBlock.event));
	EMBX_OS_EVENT_DESTROY(&(paramBlock.event));

	if (!remoteMailbox) {
		/* scan the other mailboxes for our token (which we know
		 * cannot fail this time)
		 */
		EMBX_OS_MUTEX_TAKE(&_mailboxRemoteLock);

		remoteMailbox = scanRemoteMailboxes(token);
		EMBX_Assert(remoteMailbox);

		/* let the other side know we have successfully found it */
		remoteMailbox[_EMBX_MAILBOX_STATUS].reg = token;

		EMBX_OS_MUTEX_RELEASE(&_mailboxRemoteLock);
	}

	/* restore the original enables register and handler */
	EMBX_OS_MUTEX_TAKE(&_mailboxLocalLock);

	EMBX_OS_INTERRUPT_LOCK();
	local[_EMBX_MAILBOX_ENABLE].reg = oldEnables;
	local[_EMBX_MAILBOX_STATUS].reg = oldStatus;
	setInterruptState(local, oldHandler, oldParam);
	EMBX_OS_INTERRUPT_UNLOCK();

	EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);

	*pRemote = remoteMailbox;
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Synchronize(..., *%p) = EMBX_SUCCESS\n", *pRemote));
	return EMBX_SUCCESS;
}

/*
 * Deallocate a local or remote mailbox register. Free any associated memory 
 * and for local mailboxes desensitize the mailbox to all interrupts.
 */
EMBX_VOID EMBX_Mailbox_Free(EMBX_Mailbox_t *mailbox)
{
	EMBX_LocalMailboxSet_t  *lms;
	int id;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_Free()\n"));

	/* if this is a local mailbox then mark it as no longer in use
	 * and suppress any futher interrupts
	 */
	EMBX_OS_MUTEX_TAKE(&_mailboxLocalLock);
	if (EMBX_SUCCESS == findLocalMailbox(mailbox, &lms, &id)) {
		EMBX_Assert(lms->inUse[id]);
		EMBX_Assert(!lms->requiresActivating);

		/* clear the in use flag and desensitize the mailbox to all
		 * interrupts
		 */
		lms->inUse[id] = EMBX_FALSE;
		lms->mailboxes[id][_EMBX_MAILBOX_ENABLE_CLEAR].reg = (EMBX_UINT) -1;
		lms->intTable[id].handler = NULL;
		lms->intTable[id].param = NULL;
	}
	EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);

	/* there are no resources to free for a remote mailbox */
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_Free\n"));
}

/*
 * Change the current interrupt handler (which must be installed when 
 * the mailbox is allocated) to the supplied replacement.
 */
EMBX_ERROR EMBX_Mailbox_UpdateInterruptHandler(EMBX_Mailbox_t *mailbox, void (*handler)(void *), void *param)
{
	EMBX_ERROR err;
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_UpdateInterruptHandler()\n"));

	EMBX_OS_MUTEX_TAKE(&_mailboxLocalLock);
	err = setInterruptState(mailbox, handler, param);
	EMBX_OS_MUTEX_RELEASE(&_mailboxLocalLock);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_UpdateInterruptHandler\n"));
	return err;
}

/*
 * Sensitize the supplied mailbox (which is usually local) to the supplied
 * bit number.
 */
EMBX_VOID EMBX_Mailbox_InterruptEnable(EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_InterruptEnable()\n"));
	EMBX_Assert(mailbox);

	mailbox[_EMBX_MAILBOX_ENABLE_SET].reg = (EMBX_UINT) (1 << bit);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_InterruptEnable\n"));
}

/*
 * Desensitize the supplied mailbox (which is usually local) to the supplied
 * bit number.
 */
EMBX_VOID EMBX_Mailbox_InterruptDisable(EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
#if defined __OS21__
	int p;
#endif
	EMBX_Info(EMBX_INFO_INTERRUPT, (">>>EMBX_Mailbox_InterruptDisable()\n"));
	EMBX_Assert(mailbox);

#if defined __OS21__
	/* 
	 * OS21 panics if an interrupt goes away while it is handling 
	 * interrupts (the interrupt handler does not return OS21_SUCCESS).
	 * Thus is the machine takes an interrupt and in the same cycle
	 * clears the enables bit then OS21 will panic due to an unhandled
	 * interrupt. For this reason we must suppress interrupts before we
	 * clear the enables bit.
	 */
	p = interrupt_mask_all();
#endif

	mailbox[_EMBX_MAILBOX_ENABLE_CLEAR].reg = (EMBX_UINT) (1 << bit);

	/*
	 * Make sure the call happens synchronously. We cannot read 
	 * _EMBX_MAILBOX_STATUS_CLEAR to confirm the write since that
	 * could cause a bus errror (because its write only) however the
	 * bus guarantees ordering to the same peripheral so we can read
	 * from a different address in the same peripheral.
	 */
	(void) mailbox[_EMBX_MAILBOX_ENABLE].reg;

#if defined __OS21__
	interrupt_unmask(p);
#endif

	EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<EMBX_Mailbox_InterruptDisable\n"));
}

/*
 * Clear the supplied status bit of the supplied mailbox (which is usually
 * local).
 */
EMBX_VOID EMBX_Mailbox_InterruptClear(EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
#if defined __OS21__
	int p;
#endif

	EMBX_Info(EMBX_INFO_INTERRUPT, (">>>EMBX_Mailbox_InterruptClear()\n"));
	EMBX_Assert(mailbox);

#if defined __OS21__
	/* 
	 * OS21 panics if an interrupt goes away while it is handling 
	 * interrupts (the interrupt handler does not return OS21_SUCCESS).
	 * Thus is the machine takes an interrupt and in the same cycle
	 * clears the enables bit then OS21 will panic due to an unhandled
	 * interrupt. For this reason we must suppress interrupts before we
	 * clear the enables bit.
	 */
	p = interrupt_mask_all();
#endif

	mailbox[_EMBX_MAILBOX_STATUS_CLEAR].reg = (EMBX_UINT) (1 << bit);

	/*
	 * Make sure the call happens synchronously. We cannot read 
	 * _EMBX_MAILBOX_STATUS_CLEAR to confirm the write since that
	 * could cause a bus errror (because its write only) however the
	 * bus guarantees ordering to the same peripheral so we can read
	 * from a different address in the same peripheral.
	 */
	(void) mailbox[_EMBX_MAILBOX_STATUS].reg;

#if defined __OS21__
	interrupt_unmask(p);
#endif

	EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<EMBX_Mailbox_InterruptClear\n"));
}

/*
 * Set the supplied status bit of the supplied mailbox (which is usually remote).
 */
EMBX_VOID EMBX_Mailbox_InterruptRaise(EMBX_Mailbox_t *mailbox, EMBX_UINT bit)
{
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_InterruptRaise()\n"));
	EMBX_Assert(mailbox);

	mailbox[_EMBX_MAILBOX_STATUS_SET].reg = (EMBX_UINT) (1 << bit);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_InterruptRaise\n"));
}

/*
 * Read the status registers of the supplied mailbox.
 */
EMBX_UINT EMBX_Mailbox_StatusGet(EMBX_Mailbox_t *mailbox)
{
	EMBX_UINT status;
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_StatusGet(%p)\n", mailbox));
	EMBX_Assert(mailbox);
	status = (EMBX_UINT) mailbox[_EMBX_MAILBOX_STATUS].reg;
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_StatusGet = 0x%08x\n", status));
	return status;
}

/*
 * Assign value to the the status register of the supplied mailbox.
 */
EMBX_VOID EMBX_Mailbox_StatusSet(EMBX_Mailbox_t *mailbox, EMBX_UINT value)
{
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_StatusSet(%p, %x)\n", mailbox, value));
	EMBX_Assert(mailbox);
	mailbox[_EMBX_MAILBOX_STATUS].reg = value;
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_StatusSet\n"));
}

/*
 * Apply set/clear masks to the status register of the supplied mailbox.
 */
EMBX_VOID EMBX_Mailbox_StatusMask(EMBX_Mailbox_t *mailbox, EMBX_UINT set, EMBX_UINT clear)
{
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_StatusMask()\n"));
	EMBX_Assert(mailbox);
	mailbox[_EMBX_MAILBOX_STATUS_SET].reg   = set;
	mailbox[_EMBX_MAILBOX_STATUS_CLEAR].reg = clear;
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_StatusMask\n"));
}

/*
 * Allocate a lock. This will mark the lock as in-use to prevent
 * it being allocated on other CPUs.
 *
 * Errors:
 *   EMBX_NOMEM
 */
EMBX_ERROR EMBX_Mailbox_AllocLock(EMBX_MailboxLock_t **pLock)
{
	EMBX_MailboxLockSet_t *mls;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_AllocLock()\n"));
	EMBX_Assert(pLock);

	for (mls = _mailboxSpinlockHead; mls; mls = mls->next) {
		int i;
		for (i=0; i<32; i++) {
			/* if this lock is in use move onto the next one */
			if (mls->inUseMask[_EMBX_MAILBOX_STATUS].reg & (unsigned) (1 << i)) {
				continue;
			}

			/* take the lock */
			EMBX_Mailbox_TakeLock(mls->spinlocks[i]);

			/* the lock mave changed since we obtained it so if the
			 * lock is not in use move onto the next one
			 */
			if (mls->inUseMask[_EMBX_MAILBOX_STATUS].reg & (EMBX_UINT) (1 << i)) {
				EMBX_Mailbox_ReleaseLock(mls->spinlocks[i]);
				continue;
			}

			/* allocate the lock */
			mls->inUseMask[_EMBX_MAILBOX_STATUS_SET].reg = (EMBX_UINT) (1 << i);
			*pLock = mls->spinlocks[i];
			EMBX_Assert(EMBX_FALSE == mls->owner[i]);
			mls->owner[i] = EMBX_TRUE;

			/* release to lock and return */
			EMBX_Mailbox_ReleaseLock(mls->spinlocks[i]);
			EMBX_Assert(mls->inUseMask[_EMBX_MAILBOX_STATUS].reg & (EMBX_UINT) (1 << i));
			EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_AllocLock = EMBX_SUCCESS\n"));
			return EMBX_SUCCESS;
		}
	}

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_AllocLock = EMBX_NOMEM\n"));
	return EMBX_NOMEM;
}

/*
 * Deallocate a lock and free any local memory being used to manage
 * it.
 */
EMBX_VOID EMBX_Mailbox_FreeLock(EMBX_MailboxLock_t *spinlock)
{
	EMBX_MailboxLockSet_t *mls;
	int id;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_FreeLock()\n"));
	EMBX_Assert(spinlock);

	EMBX_OS_MUTEX_TAKE(&_mailboxSpinlockLock);
	/* this is safe because ANSI C guarantees lazy evaluation */
	if ((EMBX_SUCCESS == findSpinlock(spinlock, &mls, &id)) &&
	    (EMBX_FALSE != mls->owner[id])) {
		mls->inUseMask[_EMBX_MAILBOX_STATUS_CLEAR].reg = (EMBX_UINT) (1 << id);
		mls->owner[id] = EMBX_FALSE;
	}
	EMBX_OS_MUTEX_RELEASE(&_mailboxSpinlockLock);
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_FreeLock\n"));
}

/*
 * (Busy) wait for the supplied mailbox lock.
 */
#ifdef EMBX_VERBOSE
EMBX_VOID EMBX_Mailbox_TakeLock(EMBX_MailboxLock_t *lock)
{
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_TakeLock(%p)\n", lock));

	while (1 == lock->lock) {
		volatile int i=0;
		while (i<1000) i++;
	}

	/* oh ... hoopy [http://en.wikipedia.org/wiki/Hoopy] */
	EMBX_Assert(1 == lock->lock);

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_TakeLock\n"));
}
#endif

/*
 * Unlock the supplied mailbox lock.
 */
#ifdef EMBX_VERBOSE
EMBX_VOID EMBX_Mailbox_ReleaseLock(EMBX_MailboxLock_t *lock)
{
	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_ReleaseLock(%p)\n", lock));
	EMBX_Assert(1 == lock->lock);
	lock->lock = 0;
	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_ReleaseLock\n"));
}
#endif

/*
 * Convert a local spinlock pointer into an opaque multi-CPU handle.
 */
EMBX_ERROR EMBX_Mailbox_GetSharedHandle(EMBX_MailboxLock_t *lock, EMBX_UINT *pHandle)
{
	EMBX_MailboxLockSet_t *mls;
	EMBX_UINT handle;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_GetSharedHandle()\n"));
	EMBX_Assert(lock);

	handle=0;
	for (mls = _mailboxSpinlockHead; mls; mls = mls->next) {
		int i;
		for (i=0; i<32; i++) {
			handle++;
			if (lock == mls->spinlocks[i]) {
				/* check that this lock really is allocated */
				EMBX_Assert(mls->inUseMask[_EMBX_MAILBOX_STATUS].reg & (EMBX_UINT) (1 << i));

				*pHandle = handle;
				EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_GetSharedHandle = EMBX_SUCCESS\n"));
				return EMBX_SUCCESS;
			}
		}
	}

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_GetSharedHandle = EMBX_INVALID_ARGUMENT\n"));
	return EMBX_INVALID_ARGUMENT;
}

/*
 * Convert an opaque multi-CPU handle back into a lock pointer.
 */
EMBX_ERROR EMBX_Mailbox_GetLockFromHandle(EMBX_UINT handle, EMBX_MailboxLock_t **pLock)
{
	EMBX_MailboxLockSet_t *mls;
	EMBX_UINT i, h;

	EMBX_Info(EMBX_INFO_MAILBOX, (">>>EMBX_Mailbox_GetLockFromHandle()\n"));
	EMBX_Assert(pLock);

	h = 0;
	for (mls = _mailboxSpinlockHead; mls; mls = mls->next) {
		for (i=0; i<32; i++) {
			h++;
			if (h == handle) {
				*pLock = mls->spinlocks[i];

				/* check that this lock really is allocated */
				EMBX_Assert(mls->inUseMask[_EMBX_MAILBOX_STATUS].reg & (EMBX_UINT) (1 << i));

				EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_GetLockFromHandle = EMBX_INVALID_SUCCESS\n"));
				return EMBX_SUCCESS;
			}
		}
	}

	EMBX_Info(EMBX_INFO_MAILBOX, ("<<<EMBX_Mailbox_GetLockFromHandle = EMBX_INVALID_ARGUMENT\n"));
	return EMBX_INVALID_ARGUMENT;
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
