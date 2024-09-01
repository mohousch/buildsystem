/*
 * embxshm_cache.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Cache management functions for EMBXSHM.
 */

#ifndef _EMBXSHM_CACHE_H
#define _EMBXSHM_CACHE_H

#define EMBXSHM_MAX_CACHE_LINE 32
#define EMBXSHM_ALIGNED_TO_CACHE_LINE(x) (0 == (((unsigned) (x)) & (EMBXSHM_MAX_CACHE_LINE - 1)))

/* Cache management macros.
 *
 * Term  | Operation   | Meaning
 * ------+-------------+----------------------------------------------------------
 * reads | cache purge | read imminent AND cache lines are known to be clean
 * wrote | cache flush | need write must reach the hardware
 * uses  | cache purge | wrote with a read imminent OR lines are potentially dirty
 */

#if defined EMBXSHM_CACHED_HEAP

#define EMBXSHM_READS(ptr) EMBX_OS_PurgeCacheWithPrecomputableLength((void *) (ptr), sizeof(*(ptr)))
#define EMBXSHM_WROTE(ptr) EMBX_OS_FlushCacheWithPrecomputableLength((void *) (ptr), sizeof(*(ptr)))
#define EMBXSHM_USES(ptr)  EMBX_OS_PurgeCacheWithPrecomputableLength((void *) (ptr), sizeof(*(ptr)))

#define EMBXSHM_READS_ARRAY(ptr, len) EMBX_OS_PurgeCache((void *) (ptr), sizeof(*(ptr)) * (len))
#define EMBXSHM_WROTE_ARRAY(ptr, len) EMBX_OS_FlushCache((void *) (ptr), sizeof(*(ptr)) * (len))
#define EMBXSHM_USES_ARRAY(ptr, len)  EMBX_OS_PurgeCache((void *) (ptr), sizeof(*(ptr)) * (len))

#else /* !defined EMBXSHM_CACHED_HEAP */

#define EMBXSHM_READS(ptr)            ((void) 0)
#define EMBXSHM_WROTE(ptr)            ((void) 0)
#define EMBXSHM_USES(ptr)             ((void) 0)

#define EMBXSHM_READS_ARRAY(ptr, len) ((void) 0)
#define EMBXSHM_WROTE_ARRAY(ptr, len) ((void) 0)
#define EMBXSHM_USES_ARRAY(ptr, len)  ((void) 0)

#endif /* defined EMBXSHM_CACHED_HEAP */

#endif /* _EMBXSHM_CACHE_H */
