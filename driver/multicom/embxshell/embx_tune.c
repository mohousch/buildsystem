/*******************************************************************/
/* Copyright 2006 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_tune.c                                               */
/*                                                                 */
/* Description:                                                    */
/*         Manage EMBX tuneables                                   */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"
#include "embx_debug.h"

#if defined(__OS20__)

#define DEFAULT_THREAD_STACK_SIZE (16*1024)
#define DEFAULT_THREAD_PRIORITY   MAX_USER_PRIORITY

#elif defined(__OS21__)

#define DEFAULT_THREAD_STACK_SIZE (OS21_DEF_MIN_STACK_SIZE*2)
#define DEFAULT_THREAD_PRIORITY   MAX_USER_PRIORITY

#elif defined(__WINCE__) || defined(WIN32)

#define DEFAULT_THREAD_PRIORITY   THREAD_PRIORITY_NORMAL
#define DEFAULT_THREAD_STACK_SIZE 0
#define DEFAULT_MAILBOX_PRIORITY  THREAD_PRIORITY_HIGHEST

#elif defined(__LINUX__) && defined(__KERNEL__)

#define DEFAULT_THREAD_STACK_SIZE 0
#define DEFAULT_THREAD_PRIORITY   -(20 + 70)	/* RT - see also mme_tune.c */

#else

#define DEFAULT_THREAD_PRIORITY 0
#define DEFAULT_THREAD_STACK_SIZE 0

#endif /* __OS20__ */

static EMBX_UINT tuneables[EMBX_TUNEABLE_MAX] = {
    DEFAULT_THREAD_STACK_SIZE,
    DEFAULT_THREAD_PRIORITY,
#if defined(__WINCE__) || defined(WIN32)
    DEFAULT_MAILBOX_PRIORITY,
#endif
};

EMBX_ERROR EMBX_ModifyTuneable (EMBX_Tuneable_t key, EMBX_UINT value)
{
    /* allow only valid values to be modified */
    switch (key) {

#if defined __OS20__ || defined __OS21__
    case EMBX_TUNEABLE_THREAD_STACK_SIZE:
    case EMBX_TUNEABLE_THREAD_PRIORITY:
	break;
#elif defined __LINUX__ && defined __KERNEL__
    case EMBX_TUNEABLE_THREAD_PRIORITY:
	break;
#elif defined __LINUX__ || defined __SOLARIS__
    /* no tuneables for Linux/Solaris user mode */
#elif defined __WINCE__ || defined WIN32
    case EMBX_TUNEABLE_THREAD_PRIORITY:
    case EMBX_TUNEABLE_MAILBOX_PRIORITY:
	break;
#else
#error "Unknown operating system"
#endif
    
    default:
	return EMBX_INVALID_ARGUMENT;
    }

    tuneables[key] = value;
    return EMBX_SUCCESS;
}

EMBX_UINT EMBX_GetTuneable (EMBX_Tuneable_t key)
{
    EMBX_Assert(0 <= key && key < EMBX_TUNEABLE_MAX);
    return tuneables[key];
}
