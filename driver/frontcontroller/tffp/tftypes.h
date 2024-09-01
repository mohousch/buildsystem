#ifndef TFTYPES_H
#define TFTYPES_H

#include <linux/version.h>

#define ulong64 		unsigned long long
#define sulong64		static unsigned long long

#ifndef dword
typedef unsigned long	dword;
#endif
#ifndef word
typedef unsigned short	word;
#endif
#ifndef byte
typedef unsigned char	byte;
#endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,17)
typedef unsigned char   bool;
#endif


#define rvoid			register void
#define rbyte			register byte
#define rchar			register char
#define rword			register word
#define rdword			register dword
#define rshort			register short
#define rlong			register long
#define rint			register int

#define svoid			static void
#define sbyte			static byte
#define schar			static char
#define sword			static word
#define sdword			static dword
#define sshort			static short
#define slong			static long
#define sint			static int

#define vvoid			volatile void
#define vbyte			volatile byte
#define vchar			volatile char
#define vword			volatile word
#define vdword			volatile dword
#define vshort			volatile short
#define vlong			volatile long
#define vint			volatile int

#define vsvoid			volatile static void
#define vsbyte			volatile static byte
#define vschar			volatile static char
#define vsword			volatile static word
#define vsdword 		volatile static dword
#define vsshort 		volatile static short
#define vslong			volatile static long
#define vsint			volatile static int

#ifndef NULL
#define NULL			0
#endif

#ifndef TRUE
#define TRUE			1
#endif
#ifndef FALSE
#define FALSE			0
#endif

#ifndef true
#define true			1
#endif
#ifndef false
#define false			0
#endif

#endif
