#if defined __LINUX__ && defined __KERNEL__
#include <linux/kernel.h>
#else
#include <stdarg.h>
#include <stdio.h>
#endif

void MME_Debug_Print (const char * szFormat, ...)
{
#ifndef DISABLE_ALL_DEBUG_OUT
    va_list marker;  
        
    /* Initialize list pointer */
    va_start (marker, szFormat);

    /* Print debug string using variable argument list */
#if defined __LINUX__ && defined __KERNEL__
    vprintk(szFormat, marker);
#else
    vprintf(szFormat, marker);
#endif
    va_end (marker);
#endif /* #if !DISABLE_ALL_DEBUG_OUT */
}

