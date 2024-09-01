/*
 * rpc_userver.h
 *
 * Copyright (C) STMicroelectronics Limited 2003. All rights reserved.
 *
 * Interface definition of the RPC Micro Server
 */

#ifndef _STRPC_RPC_USERVER_H_
#define _STRPC_RPC_USERVER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ioctl.h>

#define _RPCIOCTL '!'

typedef struct RPCIOCTL_Init {
	const char	*transportName;
	const char	*portName;
/*out*/	int		 transportHandle;
/*out*/	int		 portHandle;
} RPCIOCTL_Init_t;
#define RPCIOCTL_INIT _IOWR(_RPCIOCTL, 0, RPCIOCTL_Init_t)

typedef struct RPCIOCTL_Connect {
	int		 transportHandle;
	const char	*portName;
/*out*/	int		 portHandle;
} RPCIOCTL_Connect_t;
#define RPCIOCTL_CONNECT _IOWR(_RPCIOCTL, 1, RPCIOCTL_Connect_t)

typedef struct RPCIOCTL_Wait {
	int		 portHandle;
/*out*/	int		 messageHandle;
/*out*/	int		 bufferSize;
} RPCIOCTL_Wait_t;
#define RPCIOCTL_WAIT _IOWR(_RPCIOCTL, 2, RPCIOCTL_Wait_t)

typedef struct RPCIOCTL_Fetch {
	int		 portHandle;
	int		 messageHandle;
	void		*buffer;
/*out*/	int		 messageSize;
} RPCIOCTL_Fetch_t;
#define RPCIOCTL_FETCH _IOWR(_RPCIOCTL, 3, RPCIOCTL_Fetch_t)

typedef struct RPCIOCTL_ScatterList {
	struct RPCIOCTL_ScatterList *next;
	int		 offset;
	void		*data;
	int		 size;
} RPCIOCTL_ScatterList_t;

typedef struct RPCIOCTL_Send {
	int		 transportHandle;
	int		 portHandle;
	void		*buffer;
	int		 messageSize;
	int		 bufferSize;
	RPCIOCTL_ScatterList_t *scatterList;
} RPCIOCTL_Send_t;
#define RPCIOCTL_REPLY   _IOW(_RPCIOCTL, 4, RPCIOCTL_Send_t)
#define RPCIOCTL_REQUEST _IOW(_RPCIOCTL, 5, RPCIOCTL_Send_t)

/* this ioctl has no arguments */
#define RPCIOCTL_DEINIT _IO(_RPCIOCTL, 6)

#ifdef __cplusplus
}
#endif
#endif /* _STRPC_RPC_USERVER_H_ */
