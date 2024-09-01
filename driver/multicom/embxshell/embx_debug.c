/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_debug.c                                              */
/*                                                                 */
/* Description:                                                    */
/*    Debug implementations for EMBX driver and transports         */
/*    A modularized transport will use the debug control           */
/*    as implemented here in the embx shell module.                */
/*                                                                 */
/*******************************************************************/

#include "embx_osheaders.h"

#if defined(__OS20__)
#include <debug.h>
#include <stdarg.h>
#endif

#if defined(__OS21__)
#include <os21/kernel.h>
#include <stdarg.h>
#endif /* __OS20__, __OS21__ */

#define STR_SIZE            512
#define FMT_SIZE            128

#ifndef __LINUX__

#if defined(__OS21__)
/*
 * Implement a weak version of this function which will be picked up if we link against an old OS21 
 */
int _os21_kernel_vsnprintf(char *start_ptr, size_t size, const char  *fmt, va_list ap) __attribute__ ((weak));

int _os21_kernel_vsnprintf(char *start_ptr, size_t size, const char  *fmt, va_list ap)
{
    /* This API was removed/renamed in OS21 V2.4.X and later */
    extern int _kernel_vsnprintf(char *start_ptr, size_t size, const char  *fmt, va_list ap) __attribute__ ((weak));

    return _kernel_vsnprintf(start_ptr, size, fmt, ap);
}
#endif /* __OS21__ */

void EMBX_debug_print(char *Format, ...) 
{
#if defined (__WINCE__)

    wchar_t wc_debug_str[2*STR_SIZE];
    wchar_t wc_format[2*FMT_SIZE];
    char format[FMT_SIZE];
    va_list input_args;
    int i;

    memset( (void *)wc_debug_str, '\0', 2*STR_SIZE );
    memset( (void *)wc_format, '\0', 2*FMT_SIZE );
    memset( (void *)format, '\0', FMT_SIZE );

    va_start( input_args, Format );
    strcpy( format, Format );

    for( i = 0; i < (int)strlen(format); i++ )
    {
        switch( format[i] )
        {
        case '%':
            while( (format[i] != ' ') && (format[i] != '\0') )
            {
                if( format[i] == 's' ) format[i] = 'S';
                if( format[i] == 'c' ) format[i] = 'C';
                i++;
            }
            break;
        default:
            break;
        }
    }

    for( i = 0; i < (int)strlen(format); i++ )
        wc_format[i] = format[i];

    wvsprintf(&wc_debug_str[0], wc_format, input_args);

    NKDbgPrintfW( wc_debug_str );

#endif /* __WINCE__ */    

#ifndef EMBX_UNHOSTED

#if defined (__OS20__)
    char buf[STR_SIZE];
    int l;
    va_list args;

    va_start(args, Format);
    l = vsprintf(buf, Format, args);
    va_end(args);

    if (l > STR_SIZE) { 
	/* hitting this breakpoint implies that STR_SIZE is too small */
	debugbreak();
    } else {
	debugmessage(buf);
    }
#endif /* __OS20__ */

#if defined(__OS21__)
    /* NB: These are internal OS21 APIs */
    extern int _os21_kernel_vsnprintf(char *start_ptr, size_t size, const char  *fmt, va_list ap);
    extern void _md_kernel_puts(const char*);

    task_t* task;
    int     interruptInfo;

    int size;
    char buf[STR_SIZE];

    va_list args;
    va_start(args, Format);

    if (task_context(&task, &interruptInfo) == task_context_interrupt) {
	size = _os21_kernel_vsnprintf(buf, STR_SIZE, Format, args);
	
	_md_kernel_puts(buf);
    }
    else {
	/* Workaround for a bug on the ST200 where the interrupt context
	 * debug messages seem to interfere and cause the LX to crash
	 */
	interrupt_lock();
	vfprintf(stdout, Format, args);
	fflush(stdout);      
	interrupt_unlock();
    }

    va_end(args);
#endif /* __OS21__ */

#endif /* EMBX_UNHOSTED */

}

#endif /* !__LINUX__ */

static int debug_enable = 0;

extern int EMBX_debug_enabled(void)
{
    return debug_enable;
}

extern void EMBX_debug_enable(int b)
{
    debug_enable = (b != 0);
}

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
