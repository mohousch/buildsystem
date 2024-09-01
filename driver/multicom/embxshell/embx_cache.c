/*
 * embx_cache.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * Cache management functions for operating systems with inadequate in-built support.
 */

#include <embx_debug.h>
#include <embx_osinterface.h>

/* this does not form part of debug_ctrl.h since it is so rare that setting
 * this will provide useful debug trace
 */
#ifndef EMBX_INFO_CACHE
#define EMBX_INFO_CACHE 0
#endif

#if defined __OS21__ && defined __ST200__

#include <machine/archcore.h>

/* although OS21 includes in-built cache management code it is not as agressively unrolled
 * as the code here and is significantly slower over large blocks.
 */
EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz)
{
	size_t start = (size_t) ptr;
	size_t end   = start + sz - 1;

	if (0 == sz) {
		return;
	}

	start &= ~(LX_DCACHE_LINE_BYTES - 1);
	end   &= ~(LX_DCACHE_LINE_BYTES - 1);

	__asm__ __volatile__ (
		       "cmpgeu $b0.7 = %2, %1\n" 
		"	cmpgeu $b0.6 = %3, %1\n"
		"	add	%0 = %3, %4\n"
		"	## prevent overflushing when length is a power of two\n"
		"	goto	purge1\n"
		"	;;\n"
		"purge0:\n"
		"	prgadd	%7[%0]\n"
		"	add	%0 = %0, %4\n"
		"	cmpgeu	$b0.7 = %0, %1\n"
		"	br	$b0.7, purge2\n"
		"	;;\n"
		"	prgadd	%7[%0]\n"
		"	add	%0 = %0, %4\n"
		"	cmpgeu	$b0.6 = %0, %1\n"
		"	br	$b0.6, purge2\n"
		"	;;\n"
		"	prgadd	%7[%0]\n"
		"	br	$b0.5, purge2\n"
		"	;;\n"
		"purge1:\n"
		"	prgadd	%6[%0]\n"
		"	add	%0 = %0, %5\n"
		"	cmpgeu	$b0.5 = %0, %1\n"
		"	goto	purge0\n"
		"	;;\n"
		"purge2:\n"
		"	sync"
		: "=&r" (start)
		: "r" (end), 
		  "r" (start + LX_DCACHE_LINE_BYTES), 
		  "r" (start + 2*LX_DCACHE_LINE_BYTES),
		  "i" (LX_DCACHE_LINE_BYTES),
		  "i" (LX_DCACHE_LINE_BYTES*2),
		  "i" (LX_DCACHE_LINE_BYTES*-3),
		  "i" (LX_DCACHE_LINE_BYTES*-4)
		: "b7", "b6", "b5"
	);
}

#endif /* __OS21__ and __ST200__ */

#if defined WIN32
EMBX_VOID EMBX_OS_PurgeCache(EMBX_VOID *ptr, EMBX_UINT sz)
{
	EMBX_Assert(0);
}
#endif /* WIN32 */
