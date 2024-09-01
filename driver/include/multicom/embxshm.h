/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm.h                                            */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport interface                  */
/*                                                            */
/**************************************************************/

#ifndef _EMBXSHM_H
#define _EMBXSHM_H

#include <embx_types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define EMBXSHM_MAX_CPUS 8

typedef struct
{
    EMBX_CHAR  name[EMBX_MAX_TRANSPORT_NAME+1];  /* The name of this transport */
    EMBX_UINT  cpuID;                           /* Local CPU ID (0 == master) */
    EMBX_UINT  participants[EMBXSHM_MAX_CPUS];  /* 1 == participating in this transport */
    EMBX_UINT  pointerWarp;                     /* Offset when pointer warping */
                                                /* LocalAddr + offset == BusAddr */
                                                /* BusAddr - offset == LocalAddr */

    EMBX_VOID *sharedAddr;                      /* Address of base of shared memory */
    EMBX_UINT  sharedSize;                      /* Size of shared memory */
    EMBX_ERROR (*notifyFn)(void *data, EMBX_UINT opcode, void *param);
						/* Function called during event notification,
						   it is responsible for interrupt knocking etc. */
    void      *notifyData;			/* User supplied data that is passed to notifyFn */


    EMBX_UINT  maxPorts;                        /* Max ports supported (0 == no limit) */
    EMBX_UINT  maxObjects;                      /* Number of shared objects to support */
    EMBX_UINT  freeListSize;                    /* Number of pre-allocated free nodes per port */
    EMBX_VOID *warpRangeAddr;                   /* Smallest address for which pointer warp is valid */
    EMBX_UINT  warpRangeSize;                   /* Size of memory for which pointer warp is valid */

} EMBXSHM_GenericConfig_t;

/* opcodes that may be supplied to the notifyFn() */
enum {
	EMBXSHM_OPCODE_RENDEZVOUS,
	EMBXSHM_OPCODE_INSTALL_ISR,
	EMBXSHM_OPCODE_REMOVE_ISR,
	EMBXSHM_OPCODE_RAISE_INTERRUPT,
	EMBXSHM_OPCODE_CLEAR_INTERRUPT,
	EMBXSHM_OPCODE_ENABLE_INTERRUPT,
	EMBXSHM_OPCODE_DISABLE_INTERRUPT,
	EMBXSHM_OPCODE_BUFFER_FLUSH
};

/* data structure passed for EMBXSHM_OPCODE_INSTALL_ISR */
typedef struct 
{
	void (*handler)(void *);
	void *param;
	EMBX_UINT cpuID;
} EMBXSHM_GenericConfig_InstallIsr_t;

typedef struct
{
    EMBX_CHAR  name[EMBX_MAX_TRANSPORT_NAME+1]; /* The name of this transport */
    EMBX_UINT  cpuID;                           /* Local CPU ID (0 == master) */
    EMBX_UINT  participants[EMBXSHM_MAX_CPUS];  /* 1 == participating in this transport */
    EMBX_UINT  pointerWarp;                     /* Offset when pointer warping */
                                                /* LocalAddr + offset == BusAddr */
                                                /* BusAddr - offset == LocalAddr */
    EMBX_UINT  maxPorts;                        /* Max ports supported (0 == no limit) */
    EMBX_UINT  maxObjects;                      /* Number of shared objects to support */
    EMBX_UINT  freeListSize;                    /* Number of pre-allocated free nodes per port */
    EMBX_VOID *sharedAddr;                      /* Address of base of shared memory */
    EMBX_UINT  sharedSize;                      /* Size of shared memory */
    EMBX_VOID *warpRangeAddr;                   /* Primary warp range base address */
    EMBX_UINT  warpRangeSize;                   /* Primary warp range size */
    EMBX_VOID *warpRangeAddr2;                  /* Secondary Warp range base address */
    EMBX_UINT  warpRangeSize2;                  /* Secondary Warp range size */
} EMBXSHM_MailboxConfig_t;

EMBX_Transport_t *EMBXSHM_mailbox_factory (EMBX_VOID *param);
EMBX_Transport_t *EMBXSHMC_mailbox_factory (EMBX_VOID *param);


typedef struct
{
    EMBXSHM_MailboxConfig_t mailbox;

    EMBX_VOID *empiAddr;         /* The EMPI's base address        */
    EMBX_VOID *mailboxAddr;      /* The mailbox base address       */
    EMBX_VOID *sysconfAddr;      /* The sys. config. base address  */
    EMBX_UINT  sysconfMaskSet;   /* A mask to apply to the sysconf */
    EMBX_UINT  sysconfMaskClear; /* A mask to apply to the sysconf */
} EMBXSHM_EMPIMailboxConfig_t;

EMBX_Transport_t *EMBXSHM_empi_mailbox_factory (EMBX_VOID *param);
EMBX_Transport_t *EMBXSHMC_empi_mailbox_factory (EMBX_VOID *param);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EMBXSHM_H */


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
