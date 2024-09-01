/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_typesP.h                                             */
/*                                                                 */
/* Description:                                                    */
/*         EMBX2 Private types                                     */
/*                                                                 */
/*******************************************************************/


#ifndef _EMBX_TYPESP_H
#define _EMBX_TYPESP_H

#include "embx_osheaders.h"
#include "embx_types.h"


/********************************************************************
 * OS Specific types
 */

#if defined(__OS21__)

typedef struct event_cache {
  semaphore_t* event;
  struct event_cache *next;
} * EMBX_EVENT;
typedef semaphore_t* EMBX_MUTEX;

typedef void (*EMBX_ISR_HANDLER_T)(void *);
typedef void (*EMBX_IST_HANDLER_T)(void *);

typedef task_t* EMBX_THREAD;

#endif /* __OS21__ */


#if defined(__WINCE__) || defined(WIN32)

typedef struct event_cache {
  HANDLE event;
  struct event_cache *next;
} * EMBX_EVENT;
typedef CRITICAL_SECTION EMBX_MUTEX;

typedef void (*EMBX_ISR_HANDLER_T)(void *);
typedef void (*EMBX_IST_HANDLER_T)(void *);

typedef HANDLE EMBX_THREAD;

#endif /* __WINCE__ */


#if defined(__LINUX__) && defined(__KERNEL__)

typedef struct embx_event {
    spinlock_t		lock;
    unsigned int	count;
    struct list_head	wait_list;
} EMBX_EVENT;

typedef struct semaphore   EMBX_MUTEX;

typedef void (*EMBX_ISR_HANDLER_T)(int,void *,struct pt_regs *);
typedef void (*EMBX_IST_HANDLER_T)(void *);

typedef void* EMBX_THREAD;

#endif /* __LINUX__ */


#if defined(__SOLARIS__) || \
    (defined(__LINUX__) && !defined(__KERNEL__))

typedef sem_t EMBX_EVENT;
typedef sem_t EMBX_MUTEX;

typedef void (*EMBX_ISR_HANDLER_T)(void *);
typedef void (*EMBX_IST_HANDLER_T)(void *);

typedef pthread_t* EMBX_THREAD;

#endif /* __SOLARIS__ */



/********************************************************************
 * State
 */

typedef enum {
    EMBX_DVR_UNINITIALIZED = 0,
    EMBX_DVR_INITIALIZED,
    EMBX_DVR_IN_SHUTDOWN,
    EMBX_DVR_FAILED
} EMBX_DriverState_t;


typedef enum {
    EMBX_BUFFER_ALLOCATED = (int)0xA10CA7ED,
    EMBX_BUFFER_FREE      = (int)0xF2EEB10C
} EMBX_BufferState_t;


typedef enum {
    EMBX_HANDLE_VALID   = (int)0xA11DAD1E,
    EMBX_HANDLE_INVALID = (int)0x5EE252E1, /* ~EMBX_HANDLE_VALID */
    EMBX_HANDLE_CLOSED  = (int)0xC102EDAD
} EMBX_HandleState_t;


typedef enum {
    EMBX_TP_UNINITIALIZED = 0,
    EMBX_TP_INITIALIZED,
    EMBX_TP_IN_INITIALIZATION,
    EMBX_TP_IN_CLOSEDOWN,
    EMBX_TP_FAILED
} EMBX_TransportState_t;



/********************************************************************
 * Handle management
 */

typedef struct {
    EMBX_HANDLE  handle;
    EMBX_VOID   *object;
} EMBX_HandleEntry_t;


typedef struct {
    EMBX_UINT           size;
    EMBX_HandleEntry_t *table;
    EMBX_UINT           nextIndex;
    EMBX_HANDLE         lastFreed;
    EMBX_UINT           nAllocated;
} EMBX_HandleManager_t;


#define EMBX_HANDLE_DEFAULT_TABLE_SIZE 128
#define EMBX_HANDLE_MIN_TABLE_SIZE     32

#define EMBX_HANDLE_INITIAL_VALUE  0x01000000

#define EMBX_HANDLE_INDEX_MASK     0x0000FFFF
#define EMBX_HANDLE_VALID_BIT      0x80000000
#define EMBX_HANDLE_ISSUE_MASK     0x00FF0000
#define EMBX_HANDLE_ISSUE_INC      0x00010000
#define EMBX_HANDLE_TYPE_MASK      0x7F000000

#define EMBX_HANDLE_TYPE_TRANSPORT  0x01000000
#define EMBX_HANDLE_TYPE_LOCALPORT  0x02000000
#define EMBX_HANDLE_TYPE_REMOTEPORT 0x03000000
#define EMBX_HANDLE_TYPE_FACTORY    0x04000000
#define EMBX_HANDLE_TYPE_ZEROCOPY_OBJECT 0x7a000000

/********************************************************************
 * Forward Declarations
 */
struct EMBX_BufferList_s;
typedef struct EMBX_BufferList_s EMBX_BufferList_t;

struct EMBX_EventState_s;
typedef struct EMBX_EventState_s EMBX_EventState_t;

struct EMBX_LocalPortHandle_s;
typedef struct EMBX_LocalPortHandle_s EMBX_LocalPortHandle_t;

struct EMBX_LocalPortMethods_s;
typedef struct EMBX_LocalPortMethods_s EMBX_LocalPortMethods_t;

struct EMBX_LocalPortList_s;
typedef struct EMBX_LocalPortList_s EMBX_LocalPortList_t;

struct EMBX_RemotePortHandle_s;
typedef struct EMBX_RemotePortHandle_s EMBX_RemotePortHandle_t;

struct EMBX_RemotePortMethods_s;
typedef struct EMBX_RemotePortMethods_s EMBX_RemotePortMethods_t;

struct EMBX_RemotePortList_s;
typedef struct EMBX_RemotePortList_s EMBX_RemotePortList_t;

struct EMBX_TransportMethods_s;
typedef struct EMBX_TransportMethods_s EMBX_TransportMethods_t;

struct EMBX_TransportList_s;
typedef struct EMBX_TransportList_s EMBX_TransportList_t;

struct EMBX_TransportHandle_s;
typedef struct EMBX_TransportHandle_s EMBX_TransportHandle_t;

struct EMBX_TransportHandleMethods_s;
typedef struct EMBX_TransportHandleMethods_s EMBX_TransportHandleMethods_t;

struct EMBX_TransportHandleList_s;
typedef struct EMBX_TransportHandleList_s EMBX_TransportHandleList_t;

struct EMBX_TransportFactory_s;
typedef struct EMBX_TransportFactory_s EMBX_TransportFactory_t;



/********************************************************************
 * Event queues
 */

struct EMBX_EventState_s {
    EMBX_EVENT         event;
    EMBX_ERROR         result;
    EMBX_EventState_t *prev;
    EMBX_EventState_t *next;
    EMBX_VOID         *data;
};

/*
 * Event lists are treated as circularly linked lists thus a list
 * is actually a synonym for the node type.
 */
typedef EMBX_EventState_t EMBX_EventList_t;

/********************************************************************
 * Message Buffers
 *
 * The order of members in the buffer structure is deliberate, so
 * that there is a greater chance of detecting "use after free"
 * errors. The state and transport handle should not be touched by the
 * heap management when put on the free list. However there is no
 * gaurantee of this if using system memory allocators and they can
 * always get overwritten by other means, but every little helps.
 */

#define EMBX_CAST_TO_BUFFER_T(ptr)  ((EMBX_Buffer_t *)((unsigned long)(ptr) - offsetof(EMBX_Buffer_t, data)))

typedef struct {
    EMBX_UINT          size;
    EMBX_TRANSPORT     htp;
    EMBX_BufferState_t state;

    /* Beginning of user accessible data, MUST BE LAST ENTRY in structure */
    EMBX_UINT          data[1]; 

} EMBX_Buffer_t;


struct EMBX_BufferList_s {
    EMBX_Buffer_t     *buffer;
    EMBX_BufferList_t *prev;
    EMBX_BufferList_t *next;
};



/********************************************************************
 * - Transports and Transport factories
 */

struct EMBX_TransportFactory_s {
    EMBX_TransportFactory_fn *create_fn;
    EMBX_VOID                *argument;

    EMBX_TransportFactory_t  *prev;
    EMBX_TransportFactory_t  *next;
};


struct EMBX_TransportMethods_s {
    EMBX_ERROR (*initialize)        (EMBX_Transport_t *, EMBX_EventState_t *);
    EMBX_VOID  (*init_interrupt)    (EMBX_Transport_t *);

    EMBX_ERROR (*closedown)         (EMBX_Transport_t *, EMBX_EventState_t *);
    EMBX_VOID  (*close_interrupt)   (EMBX_Transport_t *);

    EMBX_VOID  (*invalidate)        (EMBX_Transport_t *);

    EMBX_ERROR (*open_handle)       (EMBX_Transport_t *, EMBX_TransportHandle_t **);
    EMBX_ERROR (*close_handle)      (EMBX_Transport_t *, EMBX_TransportHandle_t *);

    EMBX_ERROR (*alloc_buffer)      (EMBX_Transport_t *, EMBX_UINT, EMBX_Buffer_t **);
    EMBX_ERROR (*free_buffer)       (EMBX_Transport_t *, EMBX_Buffer_t *);

    EMBX_ERROR (*register_object)   (EMBX_Transport_t *, EMBX_VOID *, EMBX_UINT, EMBX_HANDLE *);
    EMBX_ERROR (*deregister_object) (EMBX_Transport_t *, EMBX_HANDLE);
    EMBX_ERROR (*get_object)        (EMBX_Transport_t *, EMBX_HANDLE, EMBX_VOID **, EMBX_UINT *);

    EMBX_ERROR (*test_state)        (EMBX_Transport_t *, EMBX_VOID *);

#ifdef __TDT__
    /* stlinux24 has phys_to_virt and virt_to_phys macros hiding these function pointers */
    /* MULTICOM_32BIT_SUPPORT: Zero copy address translation methods */
    EMBX_ERROR (*virt_to_phys_alt)      (EMBX_Transport_t *, EMBX_VOID *, EMBX_UINT *);
    EMBX_ERROR (*phys_to_virt_alt)      (EMBX_Transport_t *, EMBX_UINT, EMBX_VOID **);
#else
    EMBX_ERROR (*virt_to_phys)      (EMBX_Transport_t *, EMBX_VOID *, EMBX_UINT *);
    EMBX_ERROR (*phys_to_virt)      (EMBX_Transport_t *, EMBX_UINT, EMBX_VOID **);
#endif

    EMBX_ERROR (*remove_processor)  (EMBX_Transport_t *, EMBX_UINT);
};


struct EMBX_Transport_s {
    EMBX_TransportMethods_t    *methods;

    EMBX_TPINFO                 transportInfo;
    EMBX_TransportState_t       state;
    EMBX_TransportFactory_t    *parent;

    EMBX_EventList_t           *initWaitList;

    EMBX_TransportHandleList_t *transportHandles;
    EMBX_EventState_t          *closeEvent;
};


struct EMBX_TransportList_s {
    EMBX_Transport_t     *transport;
    EMBX_TransportList_t *next;
};


struct EMBX_TransportHandleMethods_s {
#ifdef EMBX_RECEIVE_CALLBACK
    EMBX_ERROR (*create_port)       (EMBX_TransportHandle_t *, const EMBX_CHAR *, EMBX_LocalPortHandle_t **,
				     EMBX_Callback_t, EMBX_HANDLE handle);
#else
    EMBX_ERROR (*create_port)       (EMBX_TransportHandle_t *, const EMBX_CHAR *, EMBX_LocalPortHandle_t **);
#endif /* EMBX_RECEIVE_CALLBACK */

    EMBX_ERROR (*connect)           (EMBX_TransportHandle_t *, const EMBX_CHAR *, EMBX_RemotePortHandle_t **);

    EMBX_ERROR (*connect_block)     (EMBX_TransportHandle_t *, const EMBX_CHAR *,
                                                               EMBX_EventState_t *,
                                                               EMBX_RemotePortHandle_t **);

    EMBX_VOID  (*connect_interrupt) (EMBX_TransportHandle_t *, EMBX_EventState_t *);
};


struct EMBX_TransportHandleList_s {
    EMBX_TransportHandle_t     *transportHandle;

    EMBX_TransportHandleList_t *prev;
    EMBX_TransportHandleList_t *next;
};


struct EMBX_TransportHandle_s {
    EMBX_TransportHandleMethods_t *methods;

    EMBX_HandleState_t     state;

    /* A record of the public HANDLE is kept so that
     * receive on a copying transport can give internally
     * created buffers a valid "htp" field.
     */
    EMBX_TRANSPORT         publicHandle;

    EMBX_Transport_t      *transport;
    EMBX_LocalPortList_t  *localPorts;
    EMBX_RemotePortList_t *remotePorts;
};



/********************************************************************
 * Ports
 */

struct EMBX_LocalPortMethods_s {
    EMBX_ERROR (*invalidate_port)   (EMBX_LocalPortHandle_t *);
    EMBX_ERROR (*close_port)        (EMBX_LocalPortHandle_t *);

    EMBX_ERROR (*receive)           (EMBX_LocalPortHandle_t *, EMBX_RECEIVE_EVENT *);
    EMBX_ERROR (*receive_block)     (EMBX_LocalPortHandle_t *, EMBX_EventState_t  *, EMBX_RECEIVE_EVENT *);
    EMBX_VOID  (*receive_interrupt) (EMBX_LocalPortHandle_t *, EMBX_EventState_t  *);
};


struct EMBX_LocalPortHandle_s {
    EMBX_LocalPortMethods_t *methods;

    EMBX_HandleState_t       state;
    EMBX_TransportHandle_t  *transportHandle;
    EMBX_CHAR                portName[EMBX_MAX_PORT_NAME+1];
};


struct EMBX_LocalPortList_s {
    EMBX_LocalPortHandle_t *port;

    EMBX_LocalPortList_t   *prev;
    EMBX_LocalPortList_t   *next;
};


struct EMBX_RemotePortMethods_s {
    EMBX_ERROR (*close_port)    (EMBX_RemotePortHandle_t *);
    EMBX_ERROR (*send_message)  (EMBX_RemotePortHandle_t *, EMBX_Buffer_t *, EMBX_UINT);
    EMBX_ERROR (*send_object)   (EMBX_RemotePortHandle_t *, EMBX_HANDLE    , EMBX_UINT, EMBX_UINT);
    EMBX_ERROR (*update_object) (EMBX_RemotePortHandle_t *, EMBX_HANDLE    , EMBX_UINT, EMBX_UINT);
};


struct EMBX_RemotePortHandle_s {
    EMBX_RemotePortMethods_t *methods;

    EMBX_HandleState_t        state;
    EMBX_TransportHandle_t   *transportHandle;
};


struct EMBX_RemotePortList_s {
    EMBX_RemotePortHandle_t *port;

    EMBX_RemotePortList_t   *prev;
    EMBX_RemotePortList_t   *next;
};



/********************************************************************
 * EMBX Context
 */

typedef struct {
    EMBX_DriverState_t       state;

    volatile EMBX_BOOL       isLockInitialized;
    EMBX_MUTEX               lock;

    EMBX_TransportFactory_t  *transportFactories;
    EMBX_TransportList_t     *transports;

    EMBX_EventList_t         *deinitWaitList;
} EMBX_Context_t;

#endif /* _EMBX_TYPESP_H */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
