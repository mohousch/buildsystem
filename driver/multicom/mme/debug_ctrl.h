/*
 * debug_ctrl.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * 
 */

#ifndef _DEBUG_CTRL_H
#define _DEBUG_CTRL_H

#include "mme_debugP.h"

#if !defined(MME_INFO_COMPANION)
#define MME_INFO_COMPANION 0
#endif

#if !defined(MME_INFO_MANAGER)
#define MME_INFO_MANAGER MME_INFO_COMPANION
#endif

#if !defined(MME_INFO_RECEIVER)
#define MME_INFO_RECEIVER MME_INFO_COMPANION
#endif

#if !defined(MME_INFO_QUEUE)
#define MME_INFO_QUEUE MME_INFO_COMPANION
#endif

#endif /* _DEBUG_CTRL_H */
