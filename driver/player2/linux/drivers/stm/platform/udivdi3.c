/*
 * Simple __udivdi3 function which doesn't use FPU.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>

extern u64 __xdiv64_32(u64 n, u32 d);

u64 __udivdi3(u64 n, u64 d)
{
	if (unlikely(d & 0xffffffff00000000ULL))
	{
		printk("Approximating 64-bit/64-bit division : %llu / %llu\n", n, d);
		while (d & 0xffffffff00000000ULL)
		{
			n /= 2;
			d /= 2;
		}
	}
	return __xdiv64_32(n, (u32)d);
}

/* warning This implementation of __umoddi3 may be redundant because __umoddi3 is available under stlinux24 which does not provide this symbol explicittly either. */
uint32_t __umoddi3(u64 xp, u32 y)
{
	uint32_t rem;
	uint64_t q = __xdiv64_32(xp, y);
	rem = xp - q * y;
	return rem;
}

EXPORT_SYMBOL(__umoddi3);
EXPORT_SYMBOL(__udivdi3);
