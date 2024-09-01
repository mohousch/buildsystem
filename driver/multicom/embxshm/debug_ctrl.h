/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: debug_ctrl.h                                              */
/*                                                                 */
/* Description:                                                    */
/*         Local debug control defines for embxshell               */
/*                                                                 */
/*******************************************************************/

#ifndef _DEBUG_CTRL_H
#define _DEBUG_CTRL_H

#include "embx_debug.h"

#if defined (EMBX_VERBOSE)

#if !defined(EMBX_INFO_SHM)
#define EMBX_INFO_SHM		0
#endif

#if !defined(EMBX_INFO_FACTORY)
#define EMBX_INFO_FACTORY	EMBX_INFO_SHM
#endif

#if !defined(EMBX_INFO_INIT)
#define EMBX_INFO_INIT		EMBX_INFO_SHM
#endif

#if !defined(EMBX_INFO_HEAP)
#define EMBX_INFO_HEAP		EMBX_INFO_SHM
#endif

#if !defined(EMBX_INFO_LOCKS)
#define EMBX_INFO_LOCKS		EMBX_INFO_SHM
#endif

#if !defined(EMBX_INFO_REMOTEPORT)
#define EMBX_INFO_REMOTEPORT	EMBX_INFO_SHM
#endif

#if !defined(EMBX_INFO_TRANSPORT)
#define EMBX_INFO_TRANSPORT	EMBX_INFO_SHM
#endif

#if !defined(EMBX_INFO_INTERRUPT)
#if defined(__LINUX__)
/* on this platform it is safe to print from interrupt handlers */
#define EMBX_INFO_INTERRUPT     EMBX_INFO_SHM
#else
#define EMBX_INFO_INTERRUPT	0
#endif
#endif

#endif /* EMBX_VERBOSE */


#endif /* _DEBUG_CTRL_H */
