/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshmP.h                                           */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory transport private structures         */
/*                                                            */
/**************************************************************/

#ifndef _EMBXSHMP_H
#define _EMBXSHMP_H

#include <embx_types.h>
#include <embx_osheaders.h>
#include <embx_osinterface.h>
#include <embx_debug.h>
#include <embxP.h>

#include "embxshm.h"
#include "embxshm_cache.h"
#include "debug_ctrl.h"

#ifdef __SH4__
#define EMBXSHM_MEM_UNCACHED(_x) ((EMBX_VOID *)(((EMBX_ULONG)(_x) & 0x1fffffff) | 0xa0000000))
#define EMBXSHM_MEM_CACHED(_x) ((EMBX_VOID *)(((EMBX_ULONG)(_x) & 0x1fffffff) | 0x80000000))
#else
#define EMBXSHM_MEM_UNCACHED(_x) (_x)
#define EMBXSHM_MEM_CACHED(_x) (_x)
#endif

/*
 * The types we use
 */
typedef struct EMBXSHM_LocalPortHandle_s        EMBXSHM_LocalPortHandle_t;
typedef struct EMBXSHM_RemotePortHandle_s       EMBXSHM_RemotePortHandle_t;
typedef struct EMBXSHM_RemotePortLink_s         EMBXSHM_RemotePortLink_t;
typedef struct EMBXSHM_LocalPortShared_s        EMBXSHM_LocalPortShared_t;
typedef struct EMBXSHM_ConnectBlockState_s      EMBXSHM_ConnectBlockState_t;
typedef struct EMBXSHM_RecEventList_s           EMBXSHM_RecEventList_t;
typedef struct EMBXSHM_PipeElement_s            EMBXSHM_PipeElement_t;
typedef struct EMBXSHM_Pipe_s                   EMBXSHM_Pipe_t;
typedef struct EMBXSHM_Object_s                 EMBXSHM_Object_t;
typedef struct EMBXSHM_TCB_s                    EMBXSHM_TCB_t;
typedef struct EMBXSHM_Transport_s              EMBXSHM_Transport_t;

typedef struct EMBXSHM_Buffer_s                 EMBXSHM_Buffer_t;
typedef struct EMBXSHM_BufferList_s             EMBXSHM_BufferList_t;
typedef struct EMBXSHM_BusyWaitArea_s		EMBXSHM_BusyWaitArea_t;
typedef struct EMBXSHM_Spinlock_s               EMBXSHM_Spinlock_t;
typedef struct EMBXSHM_HeapControlBlockShared_s EMBXSHM_HeapControlBlockShared_t;
typedef struct EMBXSHM_HeapControlBlock_s       EMBXSHM_HeapControlBlock_t;

typedef EMBX_TransportHandle_t                  EMBXSHM_TransportHandle_t;

/*
 * These macros cope with warp between local and bus addresses
 */
#define LOCAL_TO_BUS(a, o) ((void *)((uintptr_t) (a)+(o)))
#define BUS_TO_LOCAL(a, o) ((void *)((uintptr_t) (a)-(o)))

/*
 * Typed versions of the above generic macros
 */
#define BUS_TO_BUFFER(a, o) ((EMBXSHM_Buffer_t *) BUS_TO_LOCAL (a, o))
#define BUFFER_TO_BUS(a, o) ((EMBXSHM_Buffer_t *) LOCAL_TO_BUS (a, o))

/*
 * Enum for heap manager use
 */
enum
{
    FREELIST_8_BYTE,
    FREELIST_16_BYTE,
    FREELIST_24_BYTE,
    FREELIST_32_BYTE,
    FREELIST_64_BYTE,
    FREELIST_128_BYTE,
    FREELIST_256_BYTE,
    FREELIST_512_BYTE,
    FREELIST_LARGE_BLOCKS,
    FREELIST_BREAK_VALUES,
    FREELIST_MAX_INDEX
};

#define EMBXSHM_BUFFER_ALLOCATED         ((EMBXSHM_Buffer_t *) EMBX_BUFFER_ALLOCATED)
#define EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE 512

/*
 * This is an identical copy of EMBX_Buffer_t with an element renamed
 * there are asserts proving equivalence in the memory init code
 */
struct EMBXSHM_Buffer_s
{
    EMBX_UINT         size;
    EMBX_TRANSPORT    htp;
    EMBXSHM_Buffer_t *next;

    /*
     * Beginning of user accessible data
     */
    EMBX_UINT         data[1];
};

#define SIZEOF_EMBXSHM_BUFFER        offsetof (EMBXSHM_Buffer_t, data)

/*
 * Structure used to manage free lists
 */
struct EMBXSHM_BufferList_s
{
    EMBXSHM_Spinlock_t  *lock;
    EMBXSHM_Buffer_t    *head;

    /* conceptually these form a union with head (but not lock)
     * however there is not need for this as the structure must
     * be the size of a cache line anyway.
     */
    uintptr_t		 smallBlockBreak;
    uintptr_t		 largeBlockBreak;

    /* pad until we are cache line sized */
    char _padding[EMBXSHM_MAX_CACHE_LINE - 2*sizeof(void *) - 2*sizeof(uintptr_t)];
};

/*
 * Cache line sized structure to be used to store a single marker value and reserve the
 * remainder of the cache line.
 */
struct EMBXSHM_BusyWaitArea_s
{
    volatile EMBX_UINT marker;

    char _padding[EMBXSHM_MAX_CACHE_LINE - sizeof(EMBX_UINT)];
};

/*
 * A spinlock
 */
struct EMBXSHM_Spinlock_s
{
#ifndef __SPARC__
    EMBXSHM_BusyWaitArea_t n[EMBXSHM_MAX_CPUS];
#else
    volatile EMBX_UCHAR lock;
#endif
    EMBXSHM_BusyWaitArea_t count;
};

/*
 * Master heap control block in shared memory
 */
struct EMBXSHM_HeapControlBlockShared_s
{
    EMBXSHM_BufferList_t  freelist[FREELIST_MAX_INDEX];
};

/*
 * Private heap control block, held on each CPU in local memory
 */
struct EMBXSHM_HeapControlBlock_s
{
    EMBXSHM_Spinlock_t               *lockCache[FREELIST_MAX_INDEX];
    EMBX_MUTEX                        lockPartner[FREELIST_MAX_INDEX];
    EMBXSHM_HeapControlBlockShared_t *hcbShared;   /* Pointer to master heap control block
                                                      in shared mem */
};

/* 
 * Thread IDs used by the transport
 */
#define EMBXSHM_CLOSEPORT_WORKER 0
#define EMBXSHM_NEWPORT_WORKER   1
#define EMBXSHM_NUM_WORKERS      2

/*
 * The fat message pipes.
 *
 * A pipe element consists of 7 words of payload
 * and 1 word of control info => 32 bytes
 */
typedef enum
{
    EMBXSHM_PIPE_CTRL_FREE                 = 0,
    EMBXSHM_PIPE_CTRL_MSG_SEND             = 1,
    EMBXSHM_PIPE_CTRL_OBJ_SEND             = 2,
    EMBXSHM_PIPE_CTRL_OBJ_UPDATE           = 3,
    EMBXSHM_PIPE_CTRL_PORT_CREATE          = 4,
    EMBXSHM_PIPE_CTRL_PORT_CLOSED          = 5,
    EMBXSHM_PIPE_CTRL_TRANSPORT_INVALIDATE = 6

} EMBXSHM_PipeCtrl_t;

struct EMBXSHM_PipeElement_s
{
    union
    {
        EMBX_UINT rawData[5];
        struct
        {
            EMBXSHM_LocalPortShared_t *port;
            EMBX_UINT                  handle;
            EMBX_VOID                 *data;
            EMBX_UINT                  offset;
            EMBX_UINT                  size;
            
        } obj_send;

        struct
        {
            EMBXSHM_LocalPortShared_t *port;
            EMBX_VOID                 *data;
            EMBX_UINT                  size;

        } msg_send;

        struct
        {
            EMBXSHM_LocalPortShared_t *port;
            EMBX_UINT                  cpuID;

        } port_closed;

    } data;

    EMBX_UINT   _padding[2]; /* padding to make the pipes cache line sized */

    EMBXSHM_PipeCtrl_t control;
};

#define EMBXSHM_PIPE_ELEMENTS 256

struct EMBXSHM_Pipe_s
{
    EMBXSHM_PipeElement_t  elements [EMBXSHM_PIPE_ELEMENTS];
                                                /* Enqueue elements */
    EMBXSHM_Spinlock_t    *lock;                /* Write access lock */
    EMBX_UINT              index;               /* Shared writers index */

    EMBX_UINT _padding[6]; /* padding to make the pipe structure cache line sized */
};


/*
 * The private portion of local port handle, held only in local memory.
 *
 * NOTES
 *
 * blockedReceivers, freeRecEvents, and freeRecBlockers
 * are managed as LIFO lists. This is because it is (a) simplest to
 * do, and (b) gives best chance of hitting the D-cache, in a system
 * where we keep hitting enqueuing, dequeuing, enqueuing, dequeuing, off
 * these lists, and (c) we do not need to maintain any specific ordering.
 *
 * The receivedHead/receivedTail list is managed as FIFO, to maintain the
 * delivery order of items received.
 *
 * Serialisation of access to these lists is managed by masking interrupts
 * since (a) they are only accessible to the local CPU, and (b) they need
 * to be manipulated at interrupt time.
 */
struct EMBXSHM_LocalPortHandle_s
{
    EMBX_LocalPortHandle_t     port;             /* Must be first (think C++ base class) */

#ifdef EMBX_RECEIVE_CALLBACK
    EMBX_Callback_t            callback;	 /* Port callback function */
    EMBX_HANDLE                callbackHandle;	 /* Handle to pass to callback function */
#endif /* EMBX_RECEIVE_CALLBACK */

    EMBX_EventList_t          *blockedReceivers; /* People waiting for input */
    EMBXSHM_RecEventList_t    *freeRecEvents;    /* Free receive events nodes */
    EMBXSHM_RecEventList_t    *receivedHead;     /* FIFO list of delivered items */
    EMBXSHM_RecEventList_t    *receivedTail;     /* to this port */

    EMBX_UINT                  tableIndex;       /* Where we are in the port table */

    EMBXSHM_LocalPortShared_t *localPortShared;  /* Part which resides in shared memory */
};

/*
 * The portion of a local port handle, which lives in shared memory
 */
struct EMBXSHM_LocalPortShared_s
{
    EMBX_UINT                  owningCPU;                      /* Owner of this local port */

    EMBXSHM_RemotePortLink_t  *remoteConnections;              /* List of remote connections 
                                                                  from this CPU */
    EMBXSHM_LocalPortHandle_t *localPort;                      /* Pointer into owning CPU's 
                                                                  local memory */

    EMBX_CHAR                  portName[EMBX_MAX_PORT_NAME+1]; /* Copy of port name */

    /* portName is already cache line aligned and is ignored in this calculation */
    EMBX_CHAR _padding0[EMBXSHM_MAX_CACHE_LINE - (2*sizeof(void *) + sizeof(EMBX_UINT))];

    /* TODO: these should be removed and replaced with pipe messages */
    EMBXSHM_BusyWaitArea_t     invalidated[EMBXSHM_MAX_CPUS];	/* Invalidation sync flags */
};

/*
 * A remote port structure, which lives in local memory
 */
struct EMBXSHM_RemotePortHandle_s
{
    EMBX_RemotePortHandle_t    port;            /* Must be first (think C++ base class) */

    EMBXSHM_RemotePortLink_t  *linkageShared;   /* Linkage for enqueing on a local port 
                                                   (shared memory) */

    EMBX_MUTEX                 accessMutex;     /* Required for checking / changing state */
    EMBX_UINT                  destinationCPU;  /* Who owns the destination */
    union
    {
        EMBXSHM_LocalPortShared_t *sharedPort;  /* If destination on another CPU */
        EMBXSHM_LocalPortHandle_t *localPort;   /* If destination is local */

    } destination;
}; 

/*
 * A remote port linkage structure, which lives in shared memory
 */
struct EMBXSHM_RemotePortLink_s
{
    EMBXSHM_RemotePortLink_t    *prevConnection;   /* Linkage for enqueing on a local port */
    EMBXSHM_RemotePortLink_t    *nextConnection;

    EMBX_UINT                    owningCPU;        /* Who owns this remote port */
    EMBXSHM_RemotePortHandle_t  *remotePort;       /* Pointer back into local memory of owner */

    char _padding[EMBXSHM_MAX_CACHE_LINE - (3*sizeof(void*) + sizeof(EMBX_UINT))];
};

#define EMBXSHM_SHADOW_OBJECT_MAGIC   0xF7D10000
#define EMBXSHM_ZEROCOPY_OBJECT_MAGIC 0xFA570000
#define EMBXSHM_OBJECT_INDEX_MASK     0x0000ffff

/*
 * Format of object, held in the shared memory object table
 */
struct EMBXSHM_Object_s
{
    EMBX_UINT  valid;       /* 1 = used, 0 = free */
    EMBX_UINT  shadow;      /* 1 = shadowed in shared memory, 0 = real obj in shared memory */
    EMBX_UINT  owningCPU;   /* Who owns me ? */
    EMBX_UINT  size;        /* Size in bytes */
    EMBX_VOID *sharedData;  /* Ptr to data in shared memory */
    EMBX_VOID *localData;   /* Ptr to data in (owner's) local memory (if shadowing) */

    EMBX_UINT  mode;        /* Cache mode */

    char _padding[EMBXSHM_MAX_CACHE_LINE - (5*sizeof(EMBX_UINT) + 2*sizeof(void*))];
};

/*
 * The transport control block, which lives in shared memory
 *
 * This structure is designed to have variable (must be unique within a cache line)
 * values held at the start. Values that remains constant for the life time of the
 * transport are groups together at the end of this structure.
 */
struct EMBXSHM_TCB_s
{
    EMBXSHM_Pipe_t              pipes[EMBXSHM_MAX_CPUS]; /* The pipes */

    EMBXSHM_BusyWaitArea_t      activeCPUs[EMBXSHM_MAX_CPUS];	/* Invalidation sync flags */
							 /* Who's active */
    EMBXSHM_Spinlock_t         *portTableSpinlock;       /* Lock to access the port table & size
                                                            fields */
    EMBXSHM_LocalPortShared_t **portTable;
    EMBX_UINT                   portTableSize;           /* Global port table and size */

    EMBX_UINT                   participants[EMBXSHM_MAX_CPUS];  /* Participants */

    EMBXSHM_Spinlock_t         *objectTableSpinlock;     /* Lock to access the global object 
                                                            table */
    EMBXSHM_Object_t           *objectTable;             /* Global object table */

    EMBXSHM_Spinlock_t         *portConnectSpinlock;     /* Needed to close/invalidate ports
							    ports on local ports */

    EMBX_UINT			_padding[16-6-EMBXSHM_MAX_CPUS];
                                            
};

/*
 * A receive event node, held in local memory only
 */
struct EMBXSHM_RecEventList_s
{
    EMBXSHM_RecEventList_t *next;
    EMBX_RECEIVE_EVENT      recev;
};

/*
 * A connect block node, which is used to manage blocked threads
 * waiting to connect to a given local port
 * Held only in local memory.
 */
struct EMBXSHM_ConnectBlockState_s
{
    const EMBX_CHAR            *portName;                /* Waiting to connect to this port */
    EMBX_TransportHandle_t     *requestingHandle;        /* Link back to owning transport */
    EMBX_RemotePortHandle_t   **port;                    /* Where to port port handle */
};

/*
 * The transport structure, held only in local memory.
 */
struct EMBXSHM_Transport_s
{
    EMBX_Transport_t              transport;             /* Must be first (think C++ base class) */

    EMBX_EventState_t            *initEvent;             /* Used during async initialise */


    /*
     * Synchronisation, Worker Threads and associated state
     */
    EMBX_BOOL                     locksValid;            /* locks and mutexes valid flag */

    EMBX_MUTEX                    tpLock;                /* Transport lock */

    EMBX_MUTEX                    portTableMutex;        /* Mutex for local access to portTable */


    EMBX_UINT                     objectTableSize;       /* size of the shared object table */
    EMBX_UINT		          objectTableIndex;	 /* Index that we previously found an
                                                            empty slot in the object table */
    EMBX_MUTEX                    objectTableMutex;      /* Mutes for local access the object 
                                                            table */

    EMBX_EventList_t             *connectionRequests;    /* List of local pending connection
                                                            requests */
    EMBX_MUTEX                    connectionListMutex;   /* Mutex to access the above */

    EMBX_MUTEX                    portConnectMutex;      /* Mutex to close / invalidate any port */
                                                         /* Use in conjunction with 
							    tcb->portConnectSpinlock */

    EMBX_EVENT                    secondHalfEvent;       /* A second half event has occured */
    EMBX_THREAD                   secondHalfThread;      /* Thread handling the above event */
    volatile EMBX_UINT            secondHalfRunFlag;     /* Run interrupt hdlr from task ctx */
    volatile EMBX_UINT            secondHalfDieFlag;     /* A request that the thread must exit */
    EMBX_MUTEX			  secondHalfMutex;	 /* A mutex to prevent portions of the
                                                            interrupt system being reentered */
			

    EMBX_EVENT                    closedPortEvent;       /* A port has been closed */
    EMBX_THREAD                   closedPortThread;      /* Thread handling the above event */
    volatile EMBX_UINT            closedPortDieFlag;     /* A request that the closed port thread 
                                                            exit */


    /*
     * Transport configuration data
     */
    EMBX_UINT                     masterCPU;        /* Who's master */
    EMBX_UINT                     cpuID;            /* Who am I ? */
    EMBX_UINT                     participants[EMBXSHM_MAX_CPUS]; 
                                                    /* The CPUs we expect in this transport */
    EMBX_UINT                     maxCPU;	    /* The largest cpuID within the system */
    EMBX_UINT                     freeListSize;     /* How long a local port's free lists are */
    EMBX_UINT                     pointerWarp;      /* Used for pointer warping */
    EMBX_VOID                    *warpRangeAddr;    /* Smallest address for which the pointer 
                                                       warp is valid */
    EMBX_UINT                     warpRangeSize;    /* Size of memory for which the pointer 
                                                       warp is valid */

    EMBX_VOID                    *warpRangeAddr2;    /* Secondary Warp range */
    EMBX_UINT                     warpRangeSize2;    /* Secondary Warp range size */
    EMBX_UINT                     pointerWarp2;      /* Seconday Warp range pointer warp */
						     

    /* 
     * Shared memory state
     */
    EMBXSHM_TCB_t                *tcb;                   /* Pointer to transport ctrl block in  
                                                            shared mem */

    EMBXSHM_HeapControlBlock_t   *hcb;                   /* Pointer to heap control block */
    EMBX_VOID                    *heap;                  /* Address of heap in shared memory */
    EMBX_UINT                     heapSize;              /* Size of heap */

    EMBX_UINT                     inPipeIndex;           /* Index into CPU's pipe */
    EMBX_ERROR			  inPipeStalled;

    EMBXSHM_LocalPortShared_t*    closedPorts[EMBXSHM_MAX_CPUS];

    volatile EMBX_UINT            remoteInvalidateFlag;  /* Used to mark a transport that has
                                                            received a remote shutdown */


    /*
     * The following are local cached copies of the spinlock addresses
     * taken from the TCB - these pointers live in local memory, and are
     * in local address format.
     */
    EMBXSHM_Spinlock_t           *portTableSpinlock;     /* Lock to access the port table & 
                                                            size fields */
    EMBXSHM_Spinlock_t           *objectTableSpinlock;   /* Lock to access the global object 
                                                            table */
    EMBXSHM_Spinlock_t           *portConnectSpinlock;   /* Needed to close/invalidate ports */
    EMBXSHM_Spinlock_t           *pipeLocks[EMBXSHM_MAX_CPUS]; /* Write locks for pipes */    

    /*
     * The following function pointers are filled in by transport factories
     * to customise the behaviour on different platforms and environments
     */

    void                          (*install_isr)(EMBXSHM_Transport_t *, void (*)(void*));
    void                          (*remove_isr) (EMBXSHM_Transport_t *);
    void                          (*raise_int)  (EMBXSHM_Transport_t *, EMBX_UINT);
    void                          (*clear_int)  (EMBXSHM_Transport_t *);
    void			  (*enable_int) (EMBXSHM_Transport_t *);
    void			  (*disable_int)(EMBXSHM_Transport_t *);
    void                          (*buffer_flush)(EMBXSHM_Transport_t *, void *);

    EMBX_ERROR                    (*setup_shm_environment)(EMBXSHM_Transport_t *);
    void			  (*cleanup_shm_environment)(EMBXSHM_Transport_t *);

    EMBX_VOID                    *factoryConfig; /* The original factory config block */
    EMBX_EVENT                    factoryEvent;  /* For use by factory code as needed */
    EMBX_VOID                    *factoryData;   /* For use by factory code as needed */
};

/*
 * Data items shared within the library
 */
extern EMBXSHM_Transport_t        EMBXSHM_transportTemplate;
extern EMBXSHM_TransportHandle_t  EMBXSHM_transportHandleTemplate;
extern EMBXSHM_LocalPortHandle_t  EMBXSHM_localportHandleTemplate;
extern EMBXSHM_RemotePortHandle_t EMBXSHM_remoteportHandleTemplate;

/*
 * Transport prototypes
 */
void EMBXSHM_newPortNotification     (EMBXSHM_Transport_t *tpshm);

EMBX_ERROR EMBXSHM_receiveNotification
                                     (EMBXSHM_LocalPortShared_t *localPortShared,
                                      EMBX_RECEIVE_EVENT        *event,
                                      EMBX_BOOL                  updateOnly,
				      EMBX_BOOL			 interruptCtx);

void EMBXSHM_portClosedNotification  (EMBXSHM_Transport_t *tpshm);

void EMBXSHM_portClose(EMBXSHM_Transport_t *tpshm);

EMBXSHM_Object_t *EMBXSHM_findObject (EMBXSHM_Transport_t *tpshm,
                                      EMBX_HANDLE          handle);

EMBX_ERROR EMBXSHM_enqueuePipeElement (EMBXSHM_Transport_t   *tpshm,
                                      EMBX_UINT              destCPU,
                                      EMBXSHM_PipeElement_t *element);

void EMBXSHM_interruptHandler   (EMBXSHM_Transport_t *tpshm);
void EMBXSHM_secondHalfHandler  (EMBXSHM_Transport_t *tpshm);

EMBXSHM_Spinlock_t *EMBXSHM_createSpinlock (EMBXSHM_Transport_t *tpshm);
void EMBXSHM_deleteSpinlock     (EMBXSHM_Transport_t *tpshm, EMBXSHM_Spinlock_t *lock);
EMBXSHM_Spinlock_t *EMBXSHM_serializeSpinlock (EMBXSHM_Transport_t *tpshm,
                                               EMBXSHM_Spinlock_t *lock);
EMBXSHM_Spinlock_t *EMBXSHM_deserializeSpinlock (EMBXSHM_Transport_t *tpshm,
                                               EMBXSHM_Spinlock_t *lock);

void EMBXSHM_defaultBufferFlush (EMBXSHM_Transport_t *tpshm, void *addr);
void EMBXSHM_takeSpinlock       (EMBXSHM_Transport_t *tpshm,
                                 EMBX_MUTEX          *mutex,
                                 EMBXSHM_Spinlock_t  *spinlock);
void EMBXSHM_releaseSpinlock    (EMBXSHM_Transport_t *tpshm, 
                                 EMBX_MUTEX          *mutex,
                                 EMBXSHM_Spinlock_t  *spinlock);

EMBX_ERROR EMBXSHM_memoryInit ( EMBXSHM_Transport_t *tpshm,
                                EMBX_VOID *base,
                                EMBX_UINT size);

void EMBXSHM_memoryDeinit (EMBXSHM_Transport_t *tpshm);

EMBX_ERROR EMBXSHM_allocateMemory (EMBX_Transport_t *tp, 
                                   size_t size, 
				   EMBX_Buffer_t **pBuf);

EMBX_ERROR EMBXSHM_freeMemory (EMBX_Transport_t *tp, EMBX_Buffer_t *block);

void *EMBXSHM_malloc (EMBXSHM_Transport_t *tpshm, size_t size);
void  EMBXSHM_free   (EMBXSHM_Transport_t *tpshm, void  *ptr);


EMBX_ERROR EMBXSHM_initialize (EMBX_Transport_t  *tp, EMBX_EventState_t *ev);

EMBX_VOID EMBXSHM_interruptInit (EMBX_Transport_t *tp);

EMBX_ERROR EMBXSHM_killThread (EMBXSHM_Transport_t *tpshm, EMBX_UINT threadName);

void EMBXSHM_destroyTransportSyncObjects (EMBXSHM_Transport_t *tpshm);

EMBX_ERROR EMBXSHM_defaultWaitForParticipants (EMBXSHM_Transport_t *tpshm);

EMBX_ERROR EMBXSHM_genericMasterInit (EMBXSHM_Transport_t *tpshm);
EMBX_ERROR EMBXSHM_genericSlaveInit  (EMBXSHM_Transport_t *tpshm);

#endif /* _EMBXSHMP_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
