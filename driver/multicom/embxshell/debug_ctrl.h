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

#if !defined(EMBX_INFO_SHELL)
#define EMBX_INFO_SHELL	    0
#endif

#if !defined(EMBX_INFO_INIT)
#define EMBX_INFO_INIT      EMBX_INFO_SHELL
#endif /* EMBX_INFO_INIT */

#if !defined(EMBX_INFO_TRANSPORT)
#define EMBX_INFO_TRANSPORT EMBX_INFO_SHELL
#endif /* EMBX_INFO_TRANSPORT */

#if !defined(EMBX_INFO_BUFFER)
#define EMBX_INFO_BUFFER    EMBX_INFO_SHELL
#endif /* EMBX_INFO_BUFFER */

#if !defined(EMBX_INFO_PORT)
#define EMBX_INFO_PORT      EMBX_INFO_SHELL
#endif /* EMBX_INFO_PORT */

#if !defined(EMBX_INFO_OS)
#define EMBX_INFO_OS        EMBX_INFO_SHELL
#endif /* EMBX_INFO_OS */


#endif /* EMBX_VERBOSE */


#endif /* _DEBUG_CTRL_H */
