/*$Source$*/
/*****************************文件头部注释*************************************/
//
//			Copyright (C), 2012-2017, AV Frontier Tech. Co., Ltd.
//
//
// 文 件 名： $RCSfile$
//
// 创 建 者： D26LF
//
// 创建时间： 2012.09.07
//
// 最后更新： $Date$
//
//				$Author$
//
//				$Revision$
//
//				$State$
//
// 文件描述： define
//
/******************************************************************************/

#ifndef __YWDEFS_H
#define __YWDEFS_H

/******************************** 文件包含************************************/

#include <linux/delay.h>
#include <linux/slab.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************** 常量定义************************************/

typedef char S8;
typedef unsigned char U8;

typedef short S16;
typedef unsigned short U16;

typedef int S32;
typedef unsigned int U32;

#ifndef INTTYPE
typedef char INT8;
typedef unsigned char UINT8;

typedef short INT16;
typedef unsigned short UINT16;

typedef int INT32;
typedef unsigned int UINT32;

typedef unsigned long long UINT64;
typedef unsigned long long U64;

typedef long long INT64;
typedef long long S64;

#define INTTYPE
#endif

//typedef unsigned int IOARCH_Handle_t;
typedef unsigned int YW_ErrorType_T;

#ifndef DEFINED_BOOL
#define DEFINED_BOOL
typedef int BOOL;
/* BOOL type constant values */
#ifndef TRUE
#define TRUE (1 == 1)
#endif
#ifndef FALSE
#define FALSE (!TRUE)
#endif
#endif

#ifndef NULL
#define NULL 0
#endif

typedef const char *ST_Revision_t;
#define YWTUNERi_MAX_TUNER_NUM 4

#define YW_NO_ERROR 0
#define YWHAL_ERROR_NO_MEMORY -1
#define YWHAL_ERROR_INVALID_HANDLE -2
#define YWHAL_ERROR_BAD_PARAMETER -3
#define YWHAL_ERROR_FEATURE_NOT_SUPPORTED -4
#define YWHAL_ERROR_UNKNOWN_DEVICE -5

#define SUCCESS 0 /* Success return */
#define RET_SUCCESS ((INT32)0)
#define ERR_FAILUE -9

/******************************** 数据结构************************************/

/******************************** 宏 定 义************************************/

/******************************** 变量定义************************************/

/******************************** 变量引用************************************/

/******************************** 函数声明************************************/

#define YWOS_TaskSleep(ms) msleep(ms)
#define usleep(us) udelay(us)

#define YWOS_Free(p) kfree(p)
#define YWOS_Malloc(n) kmalloc(n, GFP_KERNEL)

#define YWLIB_Memset(a, b, c) memset(a, b, c)
#define YWLIB_Memcpy(a, b, c) memcpy(a, b, c)
#define YWLIB_Strcpy(a, b) strcpy(a, b)

#define YWOSTRACE(x)

#if defined(MODULE)
#define malloc(n) YWOS_Malloc(n)
#define free(p) YWOS_Free(p)
#endif

/******************************** 函数定义************************************/

#ifdef __cplusplus
}
#endif

#endif /* __YWDEFS_H */
/* EOF------------------------------------------------------------------------*/

/* BOL-------------------------------------------------------------------*/
//$Log$
/* EOL-------------------------------------------------------------------*/

