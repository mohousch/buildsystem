/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_osinterface.h                                        */
/*                                                                 */
/* Description:                                                    */
/*         EMBX2 public types                                      */
/*                                                                 */
/*******************************************************************/


#ifndef _EMBX_TYPES_H
#define _EMBX_TYPES_H

/*
 * Basic data types
 */
typedef void           EMBX_VOID;
typedef char           EMBX_CHAR;
typedef unsigned char  EMBX_UCHAR;
typedef unsigned char  EMBX_BYTE;
typedef short          EMBX_SHORT;
typedef unsigned short EMBX_USHORT;
typedef int            EMBX_INT;
typedef unsigned int   EMBX_UINT;
typedef long           EMBX_LONG;
typedef unsigned long  EMBX_ULONG;
typedef int            EMBX_BOOL;

#define EMBX_FALSE 0
#define EMBX_TRUE  (!EMBX_FALSE)



/*
 * Opaque 32bit handles
 */
typedef EMBX_UINT     EMBX_HANDLE;
typedef EMBX_HANDLE   EMBX_FACTORY;
typedef EMBX_HANDLE   EMBX_TRANSPORT;
typedef EMBX_HANDLE   EMBX_PORT;

#define EMBX_INVALID_HANDLE_VALUE ((EMBX_HANDLE)0)



/*
 * Status codes
 */
typedef enum
{
    /* Public API status codes */
    EMBX_SUCCESS = 0,
    EMBX_DRIVER_NOT_INITIALIZED,
    EMBX_ALREADY_INITIALIZED,
    EMBX_NOMEM,
    EMBX_INVALID_ARGUMENT,
    EMBX_INVALID_PORT,
    EMBX_INVALID_STATUS,
    EMBX_INVALID_TRANSPORT,
    EMBX_TRANSPORT_INVALIDATED,
    EMBX_TRANSPORT_CLOSED,
    EMBX_PORTS_STILL_OPEN,
    EMBX_PORT_INVALIDATED,
    EMBX_PORT_CLOSED,
    EMBX_PORT_NOT_BIND,
    EMBX_ALREADY_BIND,
    EMBX_CONNECTION_REFUSED,
    EMBX_SYSTEM_INTERRUPT,
    EMBX_SYSTEM_ERROR,
    EMBX_INCOHERENT_MEMORY,
    EMBX_SYSTEM_TIMEOUT,

    /* Private implementation status codes */
    EMBX_BLOCK = (int)64
} EMBX_ERROR;


typedef enum {
    EMBX_TUNEABLE_THREAD_STACK_SIZE,
    EMBX_TUNEABLE_THREAD_PRIORITY,
    EMBX_TUNEABLE_MAILBOX_PRIORITY,	/* WinCE Mailbox IST thread */
    
    EMBX_TUNEABLE_MAX
} EMBX_Tuneable_t;


/*
 * Transport information
 */

#define EMBX_MAX_TRANSPORT_NAME 31
#define EMBX_MAX_PORT_NAME      31

typedef struct
{
    EMBX_CHAR  name[EMBX_MAX_TRANSPORT_NAME+1];
    EMBX_BOOL  isInitialized;
    EMBX_BOOL  usesZeroCopy;
    EMBX_BOOL  allowsPointerTranslation;
    EMBX_BOOL  allowsMultipleConnections;
    EMBX_UINT  maxPorts;
    EMBX_UINT  nrOpenHandles;
    EMBX_UINT  nrPortsInUse;
    EMBX_VOID *memStart;
    EMBX_VOID *memEnd;
    EMBX_INT   reserved[4];
} EMBX_TPINFO;



/*
 * EMBX_Receive{Block} return structure
 */
typedef enum { EMBX_REC_MESSAGE, EMBX_REC_OBJECT } EMBX_RECEIVE_TYPE;

typedef struct
{
    EMBX_RECEIVE_TYPE type;
    EMBX_HANDLE       handle;
    EMBX_VOID        *data;
    EMBX_UINT         offset;
    EMBX_UINT         size;        
} EMBX_RECEIVE_EVENT;

/*
 * EMBX TIMEOUT support
 */
#define EMBX_TIMEOUT_INFINITE	(0xfffffff)

/*
 * Opaque declarations of private driver structures.
 */
struct EMBX_Transport_s;
typedef struct EMBX_Transport_s EMBX_Transport_t;

typedef EMBX_Transport_t *EMBX_TransportFactory_fn(EMBX_VOID *);

/* Callback function for EMBX Receive */
typedef EMBX_ERROR (*EMBX_Callback_t) (EMBX_HANDLE handle, EMBX_RECEIVE_EVENT *event);

#endif /* _EMBX_TYPES_H */
