/*******************************************************************/
/* Copyright 2006 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: mme_tune.c                                                */
/*                                                                 */
/* Description:                                                    */
/*         Manage MME tuneables                                    */
/*                                                                 */
/*******************************************************************/

#include "mme.h"
#include "mmeP.h"
#include "mme_debugP.h"

#include "embx_osheaders.h"

/* TIMEOUT SUPPORT: Default transformer timeout value in ms */
#define DEFAULT_TRANSFORMER_TIMEOUT                  MME_TIMEOUT_INFINITE /* disabled by default */

#define HAS_TASK_PRIORITY 1

#ifdef __OS21__

#define DEFAULT_MANAGER_TASK_PRIORITY                200
#define DEFAULT_RECEIVER_TASK_PRIORITY               111
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST	      51
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL  49
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL        47
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL  45
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST        43

#elif defined __OS20__

#define DEFAULT_MANAGER_TASK_PRIORITY                 14
#define DEFAULT_RECEIVER_TASK_PRIORITY                10
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST	      10
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL   9
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL         8
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL   7
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST         6

#elif defined __WINCE__ || defined WIN32

#define DEFAULT_MANAGER_TASK_PRIORITY                THREAD_PRIORITY_HIGHEST
#define DEFAULT_RECEIVER_TASK_PRIORITY               THREAD_PRIORITY_ABOVE_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST      THREAD_PRIORITY_HIGHEST
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL THREAD_PRIORITY_ABOVE_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL       THREAD_PRIORITY_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL THREAD_PRIORITY_BELOW_NORMAL
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST       THREAD_PRIORITY_LOWEST

#elif defined(__LINUX__) && defined(__KERNEL__)

#define DEFAULT_MANAGER_TASK_PRIORITY                 -(20 + 60)	/* RT */
#define DEFAULT_RECEIVER_TASK_PRIORITY                -(20 + 50)	/* RT */
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST	      -20		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL  -10		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL          0		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL   10		/* nice */
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST         19		/* nice */

#else

#undef  HAS_TASK_PRIORITY
#define HAS_TASK_PRIORITY                            0
#define DEFAULT_MANAGER_TASK_PRIORITY                0
#define DEFAULT_RECEIVER_TASK_PRIORITY               0
#define DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST      0
#define DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL 0
#define DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL       0
#define DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL 0
#define DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST       0


#endif

static MME_UINT tuneables[MME_TUNEABLE_MAX] = {
	DEFAULT_MANAGER_TASK_PRIORITY,
	DEFAULT_RECEIVER_TASK_PRIORITY,
	DEFAULT_EXECUTION_TASK_PRIORITY_HIGHEST,
	DEFAULT_EXECUTION_TASK_PRIORITY_ABOVE_NORMAL,
	DEFAULT_EXECUTION_TASK_PRIORITY_NORMAL,
	DEFAULT_EXECUTION_TASK_PRIORITY_BELOW_NORMAL,
	DEFAULT_EXECUTION_TASK_PRIORITY_LOWEST,
	DEFAULT_TRANSFORMER_TIMEOUT
};

MME_ERROR MME_ModifyTuneable (MME_Tuneable_t key, MME_UINT value)
{
	switch (key) {
	case MME_TUNEABLE_MANAGER_THREAD_PRIORITY:
        case MME_TUNEABLE_TRANSFORMER_THREAD_PRIORITY:
        case MME_TUNEABLE_EXECUTION_LOOP_HIGHEST_PRIORITY:
        case MME_TUNEABLE_EXECUTION_LOOP_ABOVE_NORMAL_PRIORITY:
        case MME_TUNEABLE_EXECUTION_LOOP_NORMAL_PRIORITY:
        case MME_TUNEABLE_EXECUTION_LOOP_BELOW_NORMAL_PRIORITY:
        case MME_TUNEABLE_EXECUTION_LOOP_LOWEST_PRIORITY:
		if (!HAS_TASK_PRIORITY) {
			return MME_INVALID_ARGUMENT;
		}
		break;

        case MME_TUNEABLE_TRANSFORMER_TIMEOUT:
	  	break;

	default:
		return MME_INVALID_ARGUMENT;
	}

	tuneables[key] = value;
	return EMBX_SUCCESS;
}

MME_UINT MME_GetTuneable (MME_Tuneable_t key)
{
	MME_Assert(0 <= key && key < MME_TUNEABLE_MAX);
	return tuneables[key];
}
