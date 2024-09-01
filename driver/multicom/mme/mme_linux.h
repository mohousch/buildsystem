/*
 * Private header for linux user space implementation
 */

#ifndef MME_LINUX_H
#define MME_LINUX_H

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "mme.h"

#include <linux/ioctl.h>

/* Need to coordinate numbers - Nick Haydock reckons he starts at 240 */
#define MME_MAJOR_NUM 231
#define MME_DEV_NAME  "mme"
#define MME_DEV_COUNT 1

/* The structure for MME_IOG_WAITTRANSFORMER */
typedef struct TransformerWait_s {
	/* IN */
	void*                        instance;
        /* OUT */
	MME_ERROR                    status;
        MME_GenericCallback_t	     callback;
        MME_Event_t 		     event;
        MME_Command_t*               command;
} TransformerWait_t;

/* The bi-directinal strcuture for MME_IOS_INITTRANSFORMER */
typedef struct InitTransformer_s {
        /* IN */
        const char*                  name;
	unsigned int                 nameLength;
        MME_TransformerInitParams_t  params;
        /* OUT */
	MME_ERROR                    status;
        MME_TransformerHandle_t      handle;
	void*                        instance;
} InitTransformer_t;

typedef struct TermTransformer_s {
	/* IN */
	MME_TransformerHandle_t      handle;	
        /* OUT */
	MME_ERROR                    status;
} TermTransformer_t;

typedef struct AllocDataBuffer_s {
	/* IN */
	MME_TransformerHandle_t      handle;
	unsigned                     size;
	MME_AllocationFlags_t        flags;
        /* OUT */
	unsigned long                allocAddress;
	unsigned                     mapSize;
	unsigned long                offset;
        int                          onEmbxHeap;
	MME_ERROR                    status;
} AllocDataBuffer_t;

typedef struct FreeDataBuffer_s {
	/* IN */
	void*                        buffer;
        int                          onEmbxHeap;
        /* OUT */
	MME_ERROR                    status;
} FreeDataBuffer_t;

typedef struct SendCommand_s {
	/* INT */
	MME_TransformerHandle_t      handle;
	MME_Command_t*               commandInfo;
        /* OUT */
	MME_ERROR                    status;
} SendCommand_t;

typedef struct Capability_s {
	/* IN */
	char*                        name;
        int                          length;
	MME_TransformerCapability_t* capability;
        /* OUT */
	MME_ERROR                    status;
} Capability_t;

typedef struct Abort_s {
	/* IN */
	MME_CommandId_t              command;
	MME_TransformerHandle_t      handle;	
        /* OUT */
	MME_ERROR                    status;
} Abort_t;

typedef enum IoCtlCmds_e {
        AbortCommand_c = 1,
        TransformerCapability_c,
        SendCommand_c,
        WaitTransformer_c,
        InitTransformer_c,
        TermTransformer_c,
        AllocDataBuffer_c,
        FreeDataBuffer_c
} IoCtlCmds_t;

#define MME_IOCTL_MAGIC 'k'

#define MME_IOX_ABORTCOMMAND          _IOWR(MME_IOCTL_MAGIC, AbortCommand_c,          Abort_t)
#define MME_IOX_TRANSFORMERCAPABILITY _IOWR(MME_IOCTL_MAGIC, TransformerCapability_c, Capability_t)
#define MME_IOX_SENDCOMMAND           _IOWR(MME_IOCTL_MAGIC, SendCommand_c,           SendCommand_t )
#define MME_IOX_WAITTRANSFORMER       _IOWR(MME_IOCTL_MAGIC, WaitTransformer_c,       TransformerWait_t)
#define MME_IOX_INITTRANSFORMER       _IOWR(MME_IOCTL_MAGIC, InitTransformer_c,       InitTransformer_t)
#define MME_IOX_TERMTRANSFORMER       _IOWR(MME_IOCTL_MAGIC, TermTransformer_c,       TermTransformer_t)
#define MME_IOX_ALLOCDATABUFFER       _IOWR(MME_IOCTL_MAGIC, AllocDataBuffer_c,       AllocDataBuffer_t)
#define MME_IOX_FREEDATABUFFER        _IOWR(MME_IOCTL_MAGIC, FreeDataBuffer_c,        FreeDataBuffer_t )


/* The internal form of a data buffer descriptor used by MME_AllocDataBuffer() and
 * MME_FreeDataBuffer().
 */

typedef struct InternalDataBuffer {
	MME_DataBuffer_t      buffer;
	/* TODO - remoce/union this iobuf stuff */
	int                   iobufNum;
	struct kiobuf**       iobuf;
	void*                 physAddress;  /* The page aligned phys address */
	void*                 virtAddress;  /* The virtual address mapped to physAddress */
	void*                 allocAddress; /* The phys address of the allocation - for freeing */
	unsigned              mapSize;      /* The size of the phys->virt mapping in bytes */
        int                   onEmbxHeap;
	MME_AllocationFlags_t flags;
	MME_ScatterPage_t     pages[1];
} InternalDataBuffer_t;

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* MME_LINUX_H */

