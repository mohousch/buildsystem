/*
 * mme_linux_user.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * MME implementation for Linux user space (mostly an ioctl wrapper)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include "embx_osinterface.h"
#include "mme.h"
#include "mmeP.h"
#include "mme_linux.h"
#include "mme_debugP.h"
#include "mme_hostP.h"

#define MME_USER_LOCAL_TRANSFORMER_HANDLE_PREFIX (MME_TRANSFORMER_HANDLE_PREFIX | 0x01000000)

#define IS_LOCAL_TRANSFORMER(x) (((x) & MME_USER_LOCAL_TRANSFORMER_HANDLE_PREFIX) == MME_USER_LOCAL_TRANSFORMER_HANDLE_PREFIX)

#define LOCAL_HANDLE(h) (h & (~MME_USER_LOCAL_TRANSFORMER_HANDLE_PREFIX))
/* 
 * Until we have EMBX built in user space
 */
#define EMBX_OS_MemAlloc(X)  (malloc(X))
#define EMBX_OS_MemFree(X)   (free(X))
#define EMBX_OS_ThreadCreate LinuxUser_ThreadCreate
#define EMBX_OS_ThreadDelete LinuxUser_ThreadDelete

/* -------------------------------------------------------------------------------
 * Static instance state for this compilation unit
 * -------------------------------------------------------------------------------
 */

#define DEVICE_CLOSED ((int)-1)
static  int deviceFd = DEVICE_CLOSED;

static  EMBX_MUTEX     apiLock;
static  EMBX_EVENT     threadDataTaken;
static  HostManager_t* manager; /* Required for local transformer state */

/*  ------------------------------------------------------------------------------
 *  For the handle hash tables
 * -------------------------------------------------------------------------------
 */
typedef struct HashEntry_s {
	MME_TransformerHandle_t	handle;
	void*                   data;
	struct HashEntry_s*     next;
} HashEntry_t ;


#define HANDLE_TABLE_SIZE MAX_TRANSFORMER_INSTANCES
static HashEntry_t validHandles[HANDLE_TABLE_SIZE];
static int         handleCount;

/* -------------------------------------------------------------------------------
 * Prototypes
 * -------------------------------------------------------------------------------
 */
pthread_t* LinuxUser_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name);
int        LinuxUser_ThreadDelete(pthread_t*);

static void       ThreadEntry(void* context);

/* Utility functions for handle checks */

static __inline int  FindHandle(MME_TransformerHandle_t handle, void** data);
static __inline int  InsertHandle(MME_TransformerHandle_t handle, void* data);
static __inline int  RemoveHandle(MME_TransformerHandle_t handle);
static __inline void CleanupHandles(void);
static __inline int  GetHandles(void);

/* -------------------------------------------------------------------------------
 *  Init/deinit functions
 * -------------------------------------------------------------------------------
 */

/* MME_Init()
 * Initialize MME
 */
#define DEBUG_FUNC_STR "MME_Init"

#include <stdio.h>

MME_ERROR MME_Init(void)
{
        MME_ERROR status;
        if (deviceFd == DEVICE_CLOSED) {
	        deviceFd = open("/dev/mme", O_RDWR);
		if (deviceFd < 0) {
			MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "File %s, line %d: failed to open /dev/mme - errno %d\n", __FILE__, __LINE__, errno));
			
		        status =  MME_DRIVER_NOT_INITIALIZED;
                        goto exit;
                }
		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "deviceFd %d\n", deviceFd));
        } else {
	        status = MME_DRIVER_ALREADY_INITIALIZED;
                goto exit;
        }

        if (!EMBX_OS_MUTEX_INIT(&apiLock)) {
                status = MME_NOMEM;
                goto exit;
        }

        if (!EMBX_OS_EVENT_INIT(&threadDataTaken)) {
                EMBX_OS_MUTEX_DESTROY(&apiLock);
                status = MME_NOMEM;
                goto exit;
        }

	manager = EMBX_OS_MemAlloc(sizeof(HostManager_t));
	if (!manager) {
		return MME_NOMEM;
	}
	memset(manager, 0, sizeof(*manager));

	manager->transformerInstances = MME_LookupTable_Create(MAX_TRANSFORMER_INSTANCES, 1);
	if (!manager->transformerInstances) {
                status = MME_NOMEM;
		goto exit;
	}
	if (EMBX_FALSE == EMBX_OS_MUTEX_INIT(&manager->monitorMutex)) {
                status = MME_NOMEM;
		goto exit;
	}
	if (EMBX_FALSE == EMBX_OS_MUTEX_INIT(&manager->eventLock)) {
                status = MME_NOMEM;
		goto exit;
	}
	if (EMBX_FALSE == EMBX_OS_EVENT_INIT(&manager->waitorDone)) {
                status = MME_NOMEM;
		goto exit;
	}

        status = MME_SUCCESS;
exit:
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", status));
	return status;
}
#undef DEBUG_FUNC_STR

/* MME_Term()
 * Terminate MME
 */
MME_ERROR MME_Term(void)
{
        int status;
	if (GetHandles()>0) {
		return MME_HANDLES_STILL_OPEN;
	}

        if (deviceFd == DEVICE_CLOSED) {
	        return MME_DRIVER_NOT_INITIALIZED;
	}

        status = close(deviceFd);
        if (status != 0) {
	        return MME_HANDLES_STILL_OPEN;
        }
        deviceFd = -1;

        EMBX_OS_EVENT_DESTROY(&threadDataTaken);
        EMBX_OS_MUTEX_DESTROY(&apiLock);

	EMBX_OS_MUTEX_DESTROY(&manager->monitorMutex);
	EMBX_OS_MUTEX_DESTROY(&manager->eventLock);
	EMBX_OS_EVENT_DESTROY(&manager->waitorDone);

	MME_LookupTable_Delete(manager->transformerInstances);

	EMBX_OS_MemFree(manager);
	manager = NULL;

	CleanupHandles();

	return MME_SUCCESS;
}


/* -------------------------------------------------------------------------------
 *  Command functions
 * -------------------------------------------------------------------------------
 */

/* MME_SendCommand()
 * Send a transformer command to a transformer instance
 */
#define DEBUG_FUNC_STR "MME_SendCommand"
MME_ERROR MME_SendCommand(MME_TransformerHandle_t handle, MME_Command_t * commandInfo)
{
	int           driverResult;
	MME_ERROR     result;
	void*         data;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
		MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Device not open\n"));
	        return MME_DRIVER_NOT_INITIALIZED;
        }

        if (0 == handle) {
 	        MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Invalid handle - NULL\n"));
                return MME_INVALID_HANDLE;
        }

        if (NULL == commandInfo ||
            commandInfo->StructSize != sizeof(MME_Command_t)) {
                MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Invalid struct size\n"));
                return MME_INVALID_ARGUMENT;
        }

        if (commandInfo->CmdCode != MME_SET_GLOBAL_TRANSFORM_PARAMS &&
            commandInfo->CmdCode != MME_TRANSFORM &&
            commandInfo->CmdCode != MME_SEND_BUFFERS) {
                MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Invalid command code\n"));
                return MME_INVALID_ARGUMENT;
        }

        if (commandInfo->CmdEnd != MME_COMMAND_END_RETURN_NO_INFO &&
            commandInfo->CmdEnd != MME_COMMAND_END_RETURN_NOTIFY) {
                MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Invalid command notify type\n"));
                return MME_INVALID_ARGUMENT;
        }
#endif
        EMBX_OS_MUTEX_TAKE(&apiLock);

	if (!FindHandle(handle, &data)) {
		MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Handle 0x%08x not found\n", handle));
		EMBX_OS_MUTEX_RELEASE(&apiLock);
		return MME_INVALID_HANDLE;
	}

        EMBX_OS_MUTEX_RELEASE(&apiLock);
	
	if (IS_LOCAL_TRANSFORMER(handle)) {
		/* Local transformer */
		Transformer_t *transformer = (Transformer_t *)data;
		CommandSlot_t *commandPrivate;
		handle = LOCAL_HANDLE(handle);
		result = transformer->vtable->sendCommand(transformer, commandInfo, &commandPrivate);
		MME_Info( MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "Local transformer - result %d\n", result));
	} else {
		SendCommand_t sendCommandInfo;
		sendCommandInfo.handle = handle;
		sendCommandInfo.commandInfo = commandInfo;

		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "about to ioctl send command (0x%08x)\n", 
					       (int)commandInfo));
		driverResult = ioctl(deviceFd, MME_IOX_SENDCOMMAND, &sendCommandInfo);

		result = driverResult?MME_INTERNAL_ERROR:sendCommandInfo.status;
		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "ioctl result (%d)\n", driverResult));
	}
	MME_Info( MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "Result %d\n", result));
	return result;
}
#undef DEBUG_FUNC_STR

/* MME_AbortCommand()
 * Attempt to abort a submitted command
 */
#define DEBUG_FUNC_STR "MME_AbortCommand"
MME_ERROR MME_AbortCommand(MME_TransformerHandle_t handle, MME_CommandId_t CmdId)
{
	int driverResult;
	MME_ERROR result;
	void* data;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {

	        return MME_DRIVER_NOT_INITIALIZED;
        }
#endif
        EMBX_OS_MUTEX_TAKE(&apiLock);
	if (!FindHandle(handle, &data)) {
		MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Handle 0x%08x not found\n", handle));
		EMBX_OS_MUTEX_RELEASE(&apiLock);
		return MME_INVALID_HANDLE;
	}
        EMBX_OS_MUTEX_RELEASE(&apiLock);

	if (IS_LOCAL_TRANSFORMER(handle)) {
		/* Local transformer */
		Transformer_t* transformer = (Transformer_t*)data;
		result = transformer->vtable->abortCommand(transformer, CmdId);
	} else {
		Abort_t abortInfo;

		abortInfo.command = CmdId;
		abortInfo.handle  = handle;

		driverResult = ioctl(deviceFd, MME_IOX_ABORTCOMMAND, &abortInfo);
		result = driverResult?MME_INTERNAL_ERROR:abortInfo.status;

		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "ioctl result (%d) status %d\n", 
                         driverResult, result));
	}
	MME_Info( MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "Result %d\n", result));
	return result;
}
#undef DEBUG_FUNC_STR

/* -------------------------------------------------------------------------------
 *  Data buffer functions
 * -------------------------------------------------------------------------------
 */

/* MME_AllocDataBuffer()
 * Allocate a data buffer that is optimal for the transformer instantiation
 * to pass between a host and companion.

 * The InternalDataBuffer structure is used for to reasons - 
   - to store an array of scatter pages (currently only one is used)
   - to record psuedo information about the allocation - to free allocated memory
     and to make MME_SendCommand 'fast' - avoid kernel memory management calls, 
     primarily by recording the phys address of the allocation along with
     the virtual address
  */
#define DEBUG_FUNC_STR  "MME_AllocDataBuffer"
MME_ERROR MME_AllocDataBuffer(  MME_TransformerHandle_t handle,
				MME_UINT size,
				MME_AllocationFlags_t flags,
				MME_DataBuffer_t ** dataBuffer_p)
{
#ifndef MME_LEAN_AND_MEAN
#endif

	static const MME_AllocationFlags_t illegalFlags = (MME_AllocationFlags_t)
		~(MME_ALLOCATION_PHYSICAL | MME_ALLOCATION_CACHED | MME_ALLOCATION_UNCACHED);

	struct InternalDataBuffer *buf;
	void*  data;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
 	        MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Driver not initialized\n"));
	        return MME_DRIVER_NOT_INITIALIZED;
        }

	if (0 == size || NULL == dataBuffer_p || (flags & illegalFlags)) {
 	        MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR 
                          "size==0 ||  NULL == dataBuffer_p || (flags & illegalFlags)\n"));
		return MME_INVALID_ARGUMENT;
	}

	if ((int) size < 0) {
 	        MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "size<0\n"));
		return MME_NOMEM;
	}
#endif
        EMBX_OS_MUTEX_TAKE(&apiLock);
	if (!FindHandle(handle, &data)) {
		MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Handle 0x%08x not found\n", handle));
		EMBX_OS_MUTEX_RELEASE(&apiLock);
		return MME_INVALID_HANDLE;
	}
        EMBX_OS_MUTEX_RELEASE(&apiLock);

	if (IS_LOCAL_TRANSFORMER(handle)) {
		/* Local transformer */
		buf = EMBX_OS_MemAlloc(sizeof(*buf)+size);
		if (!buf) {
			MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Cannot EMBX_OS_MemAlloc(%d)\n", 
                          sizeof(*buf) + size ));
			return MME_NOMEM;
		}

                /* Following only valid for allocations made by the driver */
		buf->virtAddress = buf->physAddress = buf->allocAddress = NULL;
		buf->mapSize = 0;

		buf->buffer.StructSize = sizeof(MME_DataBuffer_t);
		buf->buffer.NumberOfScatterPages = 1;
		buf->buffer.ScatterPages_p = &buf->pages[0];
		buf->buffer.TotalSize = size;
		buf->flags = flags;
		buf->pages[0].Size = size;
		buf->pages[0].Page_p = &buf[1];
		*dataBuffer_p = &(buf->buffer);

	} else {
                /* The following code gets the driver to allocate page-aligned memory
                   This is mapped to user space with mmap(). The returned MME_DataBuffer_t
                   has additional psuedo information about the memory allocation in 
                   the 
                */
		AllocDataBuffer_t allocInfo;
                int driverResult;
                void* pageBuffer;

		/* Allocate the buffer structure */
		buf = EMBX_OS_MemAlloc(sizeof(*buf));
		if (NULL == buf) {
			MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Cannot EMBX_OS_MemAlloc(%d)\n", 
                          sizeof(*buf) + size ));
			return MME_NOMEM;
		}
		memset(buf, 0, sizeof(*buf));

		allocInfo.handle = handle;
		allocInfo.size = size;
		allocInfo.flags = flags;

		/* This ioctl will allocate *uncached* memory from the EMBX heap 
                   - the size is passed in allocInfo.size 
                   - info about the allocation is returned in other allocInfo fields
                */
		driverResult = ioctl(deviceFd, MME_IOX_ALLOCDATABUFFER, &allocInfo);
		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "ioctl result (%d)\n", driverResult));
		if (0 != driverResult) {
			MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "ioctl error - driver code %d, status %d \n",
                         driverResult, allocInfo.status));
			EMBX_OS_MemFree(buf);
			return MME_NOMEM;
		}

		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR
						 "Calling mmap(0, size 0x%08x, prot READ|WRITE, PRIVATE, device %d, offset 0x%08x)\n",
						 (int)allocInfo.mapSize, deviceFd, (int)allocInfo.offset));

		/* Map the allocated memory into user space */
		pageBuffer = mmap(0, allocInfo.mapSize, PROT_READ | PROT_WRITE, MAP_SHARED,
					deviceFd, allocInfo.offset);
		if (MAP_FAILED == pageBuffer) {
			/* TODO undo allocation */
			MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "mmap failed - errno  %d\n", errno));
			*dataBuffer_p = NULL;
			EMBX_OS_MemFree(buf);
			return MME_NOMEM;
		}

                buf->virtAddress = pageBuffer;                     /* The virtual address assigned */
		buf->physAddress = (void*)allocInfo.offset;        /* The aligned physical address */
		buf->allocAddress = (void*)allocInfo.allocAddress; /* The start phys address - for freeing */
		buf->mapSize = allocInfo.mapSize;                  /* Bytes mapped */
		buf->onEmbxHeap = allocInfo.onEmbxHeap;            /* Has been allocated on EMBX heap */

		buf->buffer.StructSize = sizeof(MME_DataBuffer_t);
		buf->buffer.NumberOfScatterPages = 1;
		buf->buffer.ScatterPages_p = &buf->pages[0];
		buf->buffer.TotalSize = size;
		buf->flags = flags;
		buf->pages[0].Size = size;
		buf->pages[0].Page_p = pageBuffer;
		*dataBuffer_p = &(buf->buffer);

		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "Data buffer at 0x%08x, extra at 0x%08x\n", 
					       (int) (*dataBuffer_p), (int)(&buf->iobufNum) ));
	}
	MME_Info( MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "Data buffer at 0x%08x\n", (int) (*dataBuffer_p)));
	return MME_SUCCESS;
}
#undef DEBUG_FUNC_STR

/* MME_FreeDataBuffer()
 * Free a buffer previously allocated with MME_AllocDataBuffer
 */
#define DEBUG_FUNC_STR  "MME_FreeDataBuffer"
MME_ERROR MME_FreeDataBuffer(MME_DataBuffer_t * DataBuffer_p)
{
	MME_ERROR status;
	InternalDataBuffer_t* internalBuffer = (InternalDataBuffer_t*)DataBuffer_p;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
	        return MME_DRIVER_NOT_INITIALIZED;
        }
	if (NULL == DataBuffer_p) {
		return MME_INVALID_ARGUMENT;
	}
#endif
	if (internalBuffer->allocAddress) {
		/* Remote transformer */
                int driverResult;
		FreeDataBuffer_t freeInfo;
		unsigned mapSize = internalBuffer->mapSize;
		freeInfo.buffer  = internalBuffer->allocAddress;
                freeInfo.onEmbxHeap = internalBuffer->onEmbxHeap;

		if (munmap(DataBuffer_p->ScatterPages_p->Page_p, mapSize)) {
			status = MME_INVALID_ARGUMENT;
			goto exit;
		}

		driverResult = ioctl(deviceFd, MME_IOX_FREEDATABUFFER, &freeInfo);
		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "free ioctl result (%d)\n", driverResult));
		if (0 != driverResult) {
			MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR 
				 "ioctl error - driver code %d, status %d\n", 
				 driverResult, freeInfo.status));
			status = MME_INVALID_ARGUMENT;
			goto exit;
		}
		status = freeInfo.status;

		if (status == MME_SUCCESS)
			EMBX_OS_MemFree(internalBuffer);
	} else {
		/* Local transformer */
		EMBX_OS_MemFree(internalBuffer);
		status = MME_SUCCESS;
	}
exit:
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", status));
	return status;
}
#undef DEBUG_FUNC_STR

/* -------------------------------------------------------------------------------
 * Transformer init/deinit functions
 * -------------------------------------------------------------------------------
 */

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
		*handle = id | MME_USER_LOCAL_TRANSFORMER_HANDLE_PREFIX;
	}

	return res;
}

static void deregisterTransformerInstance(MME_TransformerHandle_t handle)
{
	MME_ERROR res;

	MME_Assert(manager);

        handle = LOCAL_HANDLE(handle);
        
	res = MME_LookupTable_Remove(manager->transformerInstances, handle);

	MME_Assert(MME_SUCCESS == res);
}

/* MME_InitTransformer()
 * Create a transformer instance
 */
#define DEBUG_FUNC_STR "MME_InitTransformer"

MME_ERROR MME_InitTransformer(const char *name,
        		      MME_TransformerInitParams_t * params,
                              MME_TransformerHandle_t * handle)
{
	MME_ERROR          status;
	void* data;
	RegisteredLocalTransformer_t *rlt;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
                MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "File %s, line %d: device not open\n", 
                         __FILE__, __LINE__));
	        return MME_DRIVER_NOT_INITIALIZED;
        }

        if (NULL == name || NULL == params || NULL == handle ||
            params->StructSize != sizeof(MME_TransformerInitParams_t) ||
	    strlen(name) > (MME_MAX_TRANSFORMER_NAME-1)) {
                MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "File %s, line %d: invalid argument\n", 
                         __FILE__, __LINE__));
                return MME_INVALID_ARGUMENT;
        }
#endif
	status = findTransformerOnHost(name, &rlt);
	if (MME_SUCCESS == status) {
	        Transformer_t* transformer;

		/* A local transformer is registered */
  	        MME_Info( MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "Host transformer \n"));

        	transformer = MME_LocalTransformer_Create(manager, rlt);
		status = registerTransformerInstance(transformer, handle);

		if (status != MME_SUCCESS) {
			goto cleanup_info;
		}

  	        MME_Info( MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "Host transformer - handle 0x%08x\n", (int)*handle ));

		/* Initialize the transformer */
		status = transformer->vtable->init(transformer, name, params, *handle, EMBX_TRUE);

		if (status != MME_SUCCESS) {
			MME_Info( MME_INFO_MANAGER, (DEBUG_ERROR_STR 
                                 "File %s, line %d: cannot init transformer, error %d\n", 
                                 __FILE__, __LINE__, status));
			goto cleanup_info;
		}
		data = transformer;
	} else {
		/* Attempt to use a remote transformer */
		EMBX_THREAD        thread = NULL;
		int                driverResult;
		InitTransformer_t  initInfo;
		initInfo.name       = name;
		initInfo.nameLength = strlen(name);
		initInfo.params     = *params;
		initInfo.handle     = 0;
		initInfo.status     = MME_INTERNAL_ERROR;

		EMBX_OS_MUTEX_TAKE(&apiLock);

		/* Now do the initialization */
		driverResult = ioctl(deviceFd, MME_IOX_INITTRANSFORMER, &initInfo);
		MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "ioctl result (%d)\n", driverResult));
		status = initInfo.status;
		if (0 != driverResult || MME_SUCCESS != status) {
			MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "ioctl error - driver code %d, status %d \n", 
                         driverResult, status));
			goto cleanup_info;
		}

		*handle = initInfo.handle;

		/* Create the thread */
		thread = EMBX_OS_ThreadCreate(ThreadEntry, &initInfo, 0, "Callback");
		if (EMBX_INVALID_THREAD == thread) {
			status = MME_NOMEM;		
			goto cleanup_info;
		}
		EMBX_OS_EVENT_WAIT(&threadDataTaken);
		data = thread;
        }
	if (!InsertHandle(*handle, data)) {
		status = MME_NOMEM;
		goto cleanup_info;
	}
	
        EMBX_OS_MUTEX_RELEASE(&apiLock);
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "<<<< (MME_SUCCESS)\n"));
	return MME_SUCCESS;

cleanup_info:
        EMBX_OS_MUTEX_RELEASE(&apiLock);
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", status));
	return status;
}
#undef DEBUG_FUNC_STR

/* MME_TermTransformer()
 * Terminate a transformer instance
 */
#define DEBUG_FUNC_STR "MME_TermTransformer"
MME_ERROR MME_TermTransformer(MME_TransformerHandle_t handle)
{
	int driverResult;
	MME_ERROR status;
	void* data;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
	        return MME_DRIVER_NOT_INITIALIZED;
        }
#endif


        EMBX_OS_MUTEX_TAKE(&apiLock);

	if (!FindHandle(handle, &data)) {
		MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Handle 0x%08x not found\n", handle));
		status = MME_INVALID_HANDLE; 
		goto exit;
	}

	if (IS_LOCAL_TRANSFORMER(handle)) {
		/* Local transformer */
		Transformer_t *transformer = (Transformer_t*) data;
		status = transformer->vtable->term(transformer);
		if (MME_SUCCESS == status) {
			deregisterTransformerInstance(handle);
			transformer->vtable->destroy(transformer);
		} else {
			goto exit;
		}
        } else {
		/* Remote transformer */
		/* The admin/callback thread should die automatically */
		EMBX_THREAD thread = (EMBX_THREAD)data;
		TermTransformer_t termInfo;
		termInfo.handle = handle;
		driverResult = ioctl(deviceFd, MME_IOX_TERMTRANSFORMER, &termInfo);
		if (0 != driverResult) {
			status = MME_INTERNAL_ERROR;
			goto exit;
		}
	        status = termInfo.status;
		if (MME_SUCCESS != status) {
			goto exit;
		}
		/* Waits for thread the terminate */
		EMBX_OS_ThreadDelete(thread);
        }
	RemoveHandle(handle);

exit:
        EMBX_OS_MUTEX_RELEASE(&apiLock);
	return status;
}
#undef DEBUG_FUNC_STR

/* MME_GetTransformerCapability()
 * Obtain the capabilities of a transformer
 */
#define DEBUG_FUNC_STR "MME_GetTransformerCapability"
MME_ERROR MME_GetTransformerCapability(const char *name,
				       MME_TransformerCapability_t * capability)
{
	MME_ERROR status;
	RegisteredLocalTransformer_t *rlt;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
	        status =  MME_DRIVER_NOT_INITIALIZED;
		goto exit;
        }

        if (NULL == name || NULL == capability ||
            capability->StructSize != sizeof(MME_TransformerCapability_t)) {
                status =  MME_INVALID_ARGUMENT;
		goto exit;
        }
#endif

	if (MME_SUCCESS == findTransformerOnHost(name, &rlt)) {
		/* Local transformer */
		Transformer_t *transformer = MME_LocalTransformer_Create(manager, rlt);
		if (transformer) {
			status = transformer->vtable->getTransformerCapability(transformer, name, capability);
			transformer->vtable->destroy(transformer);
		} else {
			status = MME_NOMEM;
                }		
	} else {
		int driverResult;
		Capability_t capInfo;
		capInfo.name = (char*) name;
		capInfo.length = strlen(name);
		capInfo.capability = capability;

		driverResult = ioctl(deviceFd, MME_IOX_TRANSFORMERCAPABILITY, &capInfo);
		status = driverResult?MME_INTERNAL_ERROR:capInfo.status;
	}
exit:
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", status));
	return status;
}
#undef DEBUG_FUNC_STR

/* -------------------------------------------------------------------------------
 *  Local transformer APIs
 * -------------------------------------------------------------------------------
 */

/* MME_RegisterTransformer()
 * Register a transformer after which intantiations may be made
 */
MME_ERROR MME_RegisterTransformer(const char *name,
				  MME_AbortCommand_t abortFunc,
				  MME_GetTransformerCapability_t getTransformerCapabilityFunc,
				  MME_InitTransformer_t initTransformerFunc,
				  MME_ProcessCommand_t processCommandFunc,
				  MME_TermTransformer_t termTransformerFunc)
{
	/* TODO - local xformers */
	RegisteredLocalTransformer_t *elem;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
	        return MME_DRIVER_NOT_INITIALIZED;
        }

	if (NULL == name) {
		return MME_INVALID_ARGUMENT;
	}
#endif
        EMBX_OS_MUTEX_TAKE(&apiLock);

	/* check for multiple registrations */
	for (elem = manager->localTransformerList; elem; elem = elem->next) {
		if (0 == strcmp(name, elem->name)) {
			EMBX_OS_MUTEX_RELEASE(&apiLock);
			return MME_INVALID_ARGUMENT;
		}
	}

	/* register the transformer */
	elem = EMBX_OS_MemAlloc(sizeof(*elem) + strlen(name) + 1);
	
	/* populate and enqueue the structure */
	elem->name = (char *) (elem + 1);
	strcpy((char *) elem->name, name);

	elem->vtable.AbortCommand = abortFunc;
	elem->vtable.GetTransformerCapability = getTransformerCapabilityFunc;
	elem->vtable.InitTransformer = initTransformerFunc;
	elem->vtable.ProcessCommand = processCommandFunc;
	elem->vtable.TermTransformer = termTransformerFunc;

	elem->inUseCount = 0;

	elem->next = manager->localTransformerList;
	manager->localTransformerList = elem;

	EMBX_OS_MUTEX_RELEASE(&apiLock);
	return MME_SUCCESS;
}

/* MME_DeregisterTransformer()
 * Deregister a transformer that has been registered
 */
MME_ERROR MME_DeregisterTransformer(const char *name)
{
	RegisteredLocalTransformer_t *elem, **prev;
#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
	        return MME_DRIVER_NOT_INITIALIZED;
        }

	if (NULL == name) {
		return MME_INVALID_ARGUMENT;
	}
#endif
	/* TODO - local xformers */
        EMBX_OS_MUTEX_TAKE(&apiLock);

	for (elem = *(prev = &manager->localTransformerList); elem; elem = *(prev = &elem->next)) {
		if (0 == strcmp(name, elem->name)) {
			break;
		}
	}

	if (elem && 0 == elem->inUseCount) {
		*prev = elem->next;
		EMBX_OS_MemFree(elem);

		EMBX_OS_MUTEX_RELEASE(&apiLock);
		return MME_SUCCESS;
	}

	EMBX_OS_MUTEX_RELEASE(&apiLock);
	return MME_INVALID_ARGUMENT;
}

/* MME_NotifyHost()
 * For local transformers
 */
#define DEBUG_FUNC_STR "MME_NotifyHost"
MME_ERROR MME_NotifyHost(MME_Event_t event, MME_Command_t *command, MME_ERROR error)
{
	LocalTransformer_t *transformer;
	MME_ERROR status;

#ifndef MME_LEAN_AND_MEAN
	if (DEVICE_CLOSED == deviceFd) {
	        status = MME_DRIVER_NOT_INITIALIZED;
		goto exit;
        }

	if (NULL == command) {
		status = MME_INVALID_ARGUMENT;
		goto exit;
	}
#endif
	/* lookup the local transformer instanciation */
	status = MME_LookupTable_Find(manager->transformerInstances, 
				  MME_CMDID_GET_TRANSFORMER(command->CmdStatus.CmdId), 
				  (void **) &transformer);

	if (MME_SUCCESS != status) {
		status = MME_INVALID_ARGUMENT;
		goto exit;
	}

	status = MME_LocalTransformer_NotifyHost(transformer, event, command, error);
exit:       
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR "<<<< (%d)\n", status));
	return status;
}
#undef DEBUG_FUNC_STR

/* -------------------------------------------------------------------------------
 *  Not in user space
 * -------------------------------------------------------------------------------
 */

/* MME_RegisterTransport()
 * Register an existing EMBX transport for use by MME
 */
MME_ERROR MME_RegisterTransport(const char *name)
{
	return MME_NOT_IMPLEMENTED;
}

/* MME_DeregisterTransport()
 * Deregister an EMBX transport being used by MME
 */
MME_ERROR MME_DeregisterTransport(const char *name)
{
	return MME_NOT_IMPLEMENTED;
}

/* -------------------------------------------------------------------------------
 *  Callback thread
 *
 *  Thread repeatly makes an ioctl call to wait for callback until thread
 *  a non MME_SUCCESS code is returned.
 * -------------------------------------------------------------------------------
 */

#define DEBUG_FUNC_STR   "ThreadEntry(): mme_user_data.c"

static void ThreadEntry(void* context)
{
        InitTransformer_t*    initInfo = (InitTransformer_t*)context;
	MME_GenericCallback_t callback = initInfo->params.Callback;
	void *		      userData = initInfo->params.CallbackUserData;
	void*                 instance = initInfo->instance;

	EMBX_OS_EVENT_POST(&threadDataTaken);

        while (1) {
		int result;
                TransformerWait_t waitInfo;
		waitInfo.instance = instance;
		/* Wait for an error or for a callback request (retrying on signal delivery) */
                do {
                	result = ioctl(deviceFd, MME_IOX_WAITTRANSFORMER, &waitInfo);
                } while (0 != result && EINTR == errno);

                if (0 != result || MME_SUCCESS != waitInfo.status) {
	                break;
                }

                /* MME_SUCCESS - do callback */
                if (callback && MME_COMMAND_END_RETURN_NOTIFY==waitInfo.command->CmdEnd) {
 		        MME_Info(MME_INFO_LINUX_USER, (DEBUG_NOTIFY_STR 
                                 "Callback command 0x%08x\n",
                                 (int) waitInfo.command));
                        (callback)(waitInfo.event, waitInfo.command, userData);
		}
        }
	pthread_exit(0);
}
#undef DEBUG_FUNC_STR

pthread_t* LinuxUser_ThreadCreate(void (*thread)(void *), void *param, int priority, const char* name)
{
	pthread_t *tid = (pthread_t *) EMBX_OS_MemAlloc(sizeof(pthread_t));
	
        pthread_attr_t      attr;
#ifdef ROUND_ROBIN_THREADS
        struct sched_param  schedParam;
#endif

        pthread_attr_init( &attr );

#ifdef ROUND_ROBIN_THREADS
        /* Round robin scheduling */
        pthread_attr_setschedpolicy( &attr, SCHED_RR );

	/* Can only priority adjust if running SCHED_RR or SCHED_FIFO */
        schedParam.sched_priority = priority;
        pthread_attr_setschedparam( &attr,  &schedParam );
#endif

	if(pthread_create(tid, &attr, (void*(*)(void*))thread, param)) {
		EMBX_OS_MemFree(tid);
		tid = EMBX_INVALID_THREAD;
        }
	return tid;
}

int LinuxUser_ThreadDelete(pthread_t* thread)
{
	void* exitCode;
	if(pthread_join(*thread, &exitCode)) {
		return EMBX_FALSE;
        }

	EMBX_OS_MemFree(thread);
	return EMBX_TRUE;
}

/* -------------------------------------------------------------------------------
 *  Utility functions for handle checks
 * -------------------------------------------------------------------------------
 */

#define DEBUG_FUNC_STR   "FindHandle(): " __FILE__
static __inline int FindHandle(MME_TransformerHandle_t handle, void** data)
{
	/* LSB indicates whether handle entry is valid */
	/* TODO - make thread void* and use this for validness */
	int i = handle & (HANDLE_TABLE_SIZE-1);

	HashEntry_t* entry = &validHandles[i];

	while (entry) {
		if (entry->handle && (entry->handle == handle)) {
			*data = entry->data;
			return EMBX_TRUE;
		}
		entry = entry->next;
	}
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR
				       "Cannot find handle - handle 0x%08x, index %d\n", (int)handle, i));
	return EMBX_FALSE;
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR   "InsertHandle(): " __FILE__
static __inline int InsertHandle(MME_TransformerHandle_t handle, void* data) 
{
	/* LSB indicates whether handle entry is valid */

	int i = handle & (HANDLE_TABLE_SIZE-1);
	HashEntry_t* entry = &validHandles[i];

	while (entry) {
		if (entry->handle && (entry->handle == handle)) {
			/* Already present */
			return EMBX_FALSE;		
		} else if (0 == entry->handle) {
			entry->handle = handle;
			entry->data = data;
			handleCount++;
			return EMBX_TRUE;
		}
                if (!entry->next) {
                        /* Allocate empty - next time roud the entry will be filled in */
                        entry->next = EMBX_OS_MemAlloc(sizeof(HashEntry_t));
                        entry = entry->next;
                        entry->handle = 0; entry->next = NULL;
                } else {
                        entry = entry->next;
                }
        }

	MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR
				       "Cannot insert handle - handle 0x%08x, index %d\n", (int)handle, i));
	return EMBX_FALSE;
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR   "RemoveHandle(): " __FILE__
static __inline int RemoveHandle(MME_TransformerHandle_t handle)
{
	int i = handle & (HANDLE_TABLE_SIZE-1);
	HashEntry_t* entry = &validHandles[i];

	while (entry) {
		if (entry->handle && (entry->handle == handle)) {
			entry->handle = 0;
			handleCount--;
			return EMBX_TRUE;
		}
		entry = entry->next;
        }
	MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR
				       "Cannot remove handle - handle 0x%08x, index %d\n", (int)handle, i));
	return EMBX_FALSE;
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR   "CleanupHandles(): " __FILE__
static __inline void CleanupHandles(void)
{
	int i;
	for (i=0; i<HANDLE_TABLE_SIZE; i++) {
		HashEntry_t* entry = validHandles[i].next;
		validHandles[i].handle = 0;
		while (entry) {
			HashEntry_t* tofree = entry;
			entry = entry->next;
			EMBX_OS_MemFree(tofree);
		}
	}
	handleCount = 0;
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR   "GetHandles(): " __FILE__
static __inline int GetHandles(void)
{
	return handleCount;
}
#undef DEBUG_FUNC_STR

