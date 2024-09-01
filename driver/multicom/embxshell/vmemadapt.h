/* 
 * Synopsys : Provide an OS21 vmem adaption layer so vmem calls do sensible
 *            things even if the OS21 linked does not yet provide the vmem API.
 *
 * Copyright (C) STMicroelectronics Ltd. 2006.
 * 
 * All rights reserved. 
 */ 

#if !defined(_VMEMADAPT_H_)
#define _VMEMADAPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __sh__
#include <os21/st40.h>
#endif /* __sh__ */

#ifdef __st231__
#include <os21/st200.h>
#endif /* __st231__ */

#define VMEM_CREATE_CACHED          (1 << 0)
#define VMEM_CREATE_UNCACHED        (1 << 1)
#define VMEM_CREATE_WRITE_BUFFER    (1 << 2)
#define VMEM_CREATE_NO_WRITE_BUFFER (1 << 3)
#define VMEM_CREATE_READ            (1 << 4)
#define VMEM_CREATE_WRITE           (1 << 5)
#define VMEM_CREATE_EXECUTE         (1 << 6)
#define VMEM_CREATE_LOCK            (1 << 7)
#define VMEM_CREATE_EXCL            (1 << 8)

extern void *       vmem_create        (void * pAddr, unsigned int size, void * vAddr, unsigned int mode);
extern int          vmem_delete        (void * vAddr);
extern int          vmem_virt_to_phys  (void * vAddr, void ** pAddrp);
extern int          vmem_virt_mode     (void * vAddr, unsigned int * modep);
extern unsigned int vmem_min_page_size (void);

#ifdef __sh__
#define ADDRESS_IN_PHYS_MEMORY(x)   ST40_PHYS_ADDR(x)
#endif /* __sh__ */

#ifdef __cplusplus
}
#endif

#endif /* !_VMEMADAPT_H_ */
