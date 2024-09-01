/*
 * Synopsys : Provide an OS21 vmem adaption layer so vmem calls do sensible
 *            things even if the OS21 linked does not yet provide the vmem API
 *            (it is present from version 3.0.0 onwards).
 *
 * Copyright (c) STMicroelectronics 2006-2007. All rights reserved.
 *
 * This essentially provides weak implementations of the vmem API functions
 * which boil down to the sensible thing to do; either using bit-twiddling on
 * the ST40 (for 32-bit SE mode the real vmem capable OS21 is required), or
 * using calls to the old mmap API on the ST200.  We only provide any of this if
 * the OS21 version is earlier than 3.0.0.
 *
 * To keep these functions simple and small the sanity of function arguments is
 * not checked as it is in the proper OS21 implementation.
 */

#include <os21.h>

#if OS21_VERSION_MAJOR < 3 || __st220__

#include "vmemadapt.h"

void *       vmem_create        (void * pAddr, unsigned int size, void * vAddr, unsigned int mode) __attribute__ ((weak));
int          vmem_delete        (void * vAddr) __attribute__ ((weak));
int          vmem_virt_to_phys  (void * vAddr, void ** pAddrp) __attribute__ ((weak));
int          vmem_virt_mode     (void * vAddr, unsigned int * modep) __attribute__ ((weak));
unsigned int vmem_min_page_size (void) __attribute__ ((weak));

#ifdef __st231__
/*
 * Declare mmap() interface as weak - we could be linked with old or new OS21
 */
extern void *mmap_translate_cached (void *mapped_address) __attribute__ ((weak));
extern void *mmap_translate_physical (void *mapped_address) __attribute__ ((weak));
extern void *mmap_translate_uncached (void *mapped_address) __attribute__ ((weak));
extern void *mmap_translate_virtual (void *physical_address) __attribute__ ((weak));
extern void *mmap_create (void *pAddr, unsigned int size, mmap_protection_t prot, mmap_cache_policy_t policy,
			  unsigned int page_size) __attribute__ ((weak));
extern int   mmap_delete (void *mapped_address) __attribute__ ((weak));
extern unsigned int mmap_pagesize (unsigned int length) __attribute__ ((weak));

extern int   mmap_enable_speculation (void *pAddr, unsigned int length) __attribute__ ((weak));
extern int   mmap_disable_speculation (void *pAddr)  __attribute__ ((weak));

int          scu_enable_range (void * startAddr, unsigned int length) __attribute__ ((weak));
int          scu_disable_range (void * startAddr) __attribute__ ((weak));
#endif

#ifdef __sh__
int   _st40_vmem_enhanced_mode (void) __attribute__ ((weak));
#endif

#ifdef __st231__
/* In order to link the real physical addresses mapped and the addresses
   returned by vmem_create, we need to keep a linked-list of address pairs.
   This will enable us to unmap the correct region when a call to mmap_delete is
   made.
 */
struct AddressPairNode;
struct AddressPairNode
{
  void* mappedAddr;
  void* vmemAddr;
  struct AddressPairNode* next;
};
static struct AddressPairNode* mappingList = NULL;
#endif /* __st231__ */

void *       vmem_create        (void * pAddr, unsigned int size, void * vAddr, unsigned int mode)
{
#ifdef __sh__
  if (mode & VMEM_CREATE_CACHED)
    return (void*)ADDRESS_IN_CACHED_MEMORY(pAddr);
  if (mode & VMEM_CREATE_UNCACHED)
    return (void*)ADDRESS_IN_UNCACHED_MEMORY(pAddr);
#elif defined(__st231__)
  /* We need to get a suitable page size and convert size to be a multiple of it */
  unsigned int length;
  unsigned int offset = 0;
  unsigned int pageSize = 0;
  void* mappedAddr;
  void* vmemAddr = NULL;
  mmap_cache_policy_t cachePolicy;
  if ((pageSize = mmap_pagesize(size)) == 0)
  {
    /* Either size was 0, or was too big to be covered by 1 page */
    if (!size)
      return (void*)0;
    /* The ST231 supported page sizes are 8KB, 4MB and 256MB - we must have a
       huge length so use 256MB page sizes.
     */
    pageSize = 256 * 1024 * 1024;
  }
  /* Figure out what physical address mapping we should actually request based
     on the selected page size.
   */
  offset = (unsigned int)pAddr % (unsigned int)pageSize;
  mappedAddr = (void*)((unsigned int)pAddr - offset);
  /* Calc length we should request, rounding up to next multiple of page size */
  length = size + offset;
  if (length % pageSize)
    length += pageSize - (length % pageSize);
  
  /* Do cachePolicy */
  if (mode & VMEM_CREATE_CACHED)
    cachePolicy = mmap_cached;
  else if (mode & VMEM_CREATE_UNCACHED)
    cachePolicy = mmap_uncached;
  else
    return NULL;

  /* Fix/Workaround for Bugzilla 6898
   * Before creating a mapping check for an existing one
   */
  vmemAddr = mmap_translate_virtual(mappedAddr);
  vmemAddr = ((mode & VMEM_CREATE_CACHED) ? mmap_translate_cached(vmemAddr) : mmap_translate_uncached(vmemAddr));
  if (!vmemAddr)
    /* Didn't find an existing mapping so create a new one */
    mappedAddr = mmap_create(mappedAddr, length, mmap_protect_rwx, cachePolicy, pageSize);
  else
    mappedAddr = vmemAddr;

  if (mappedAddr)
  {
    /* Add new entry to mappingList */
    struct AddressPairNode* newEntry = (struct AddressPairNode*)malloc(sizeof(struct AddressPairNode));
    vmemAddr = (void*)((unsigned int)mappedAddr + offset);
    newEntry->mappedAddr = mappedAddr;
    newEntry->vmemAddr = vmemAddr;
    newEntry->next = mappingList;
    mappingList = newEntry;
  }
  return vmemAddr;
#endif
  return pAddr;
}

int          vmem_delete        (void * vAddr)
{
#ifdef __st231__
  /* We need to search the mappingList for a match and delete it if one is found
     else return failure.
   */
  struct AddressPairNode* curPtr;
  struct AddressPairNode* lastPtr = NULL;
  for (curPtr = mappingList; curPtr; lastPtr = curPtr, curPtr = curPtr->next)
  {
    if (curPtr->vmemAddr == vAddr)
    {
      if (OS21_SUCCESS == mmap_delete(curPtr->mappedAddr))
      {
        /* Remove from mappingList and return OS21_SUCCESS */
        if (lastPtr)
          lastPtr->next = curPtr->next;
        else
          mappingList = curPtr->next;
        free(curPtr);
        return OS21_SUCCESS;
      }
      return OS21_FAILURE;
    }
  }
  /* Succeed on no match */
#endif
  return OS21_SUCCESS;
}

int          vmem_virt_to_phys  (void * vAddr, void ** pAddrp)
{
#ifdef __sh__
  *pAddrp = (void*)ADDRESS_IN_PHYS_MEMORY(vAddr);
#elif defined(__st231__)
  void* physAddr;
  if ((physAddr = mmap_translate_physical(vAddr)))
  {
    *pAddrp = physAddr;
    return OS21_SUCCESS;
  }
  if ((unsigned int)mmap_translate_physical((void*)((unsigned int)vAddr+1)) == 1)
  {
    *pAddrp = 0;
    return OS21_SUCCESS;
  }
  return OS21_FAILURE;
#else
  *pAddrp = vAddr;
#endif
  return OS21_SUCCESS;
}

int          vmem_virt_mode     (void * vAddr, unsigned int * modep)
{
  /* We assume read, write and execute */
  *modep = (VMEM_CREATE_READ | VMEM_CREATE_WRITE | VMEM_CREATE_EXECUTE);
#ifdef __sh__
  if ((unsigned int)vAddr >= 0xA0000000 && (unsigned int)vAddr < 0xC0000000)
    *modep |= VMEM_CREATE_UNCACHED;
  else if ((unsigned int)vAddr >= 0x80000000 && (unsigned int)vAddr < 0xA0000000)
    *modep |= VMEM_CREATE_CACHED;
#elif defined(__st231__)
  if (vAddr == mmap_translate_uncached(vAddr))
    *modep |= VMEM_CREATE_UNCACHED;
  else if (vAddr == mmap_translate_cached(vAddr))
    *modep |= VMEM_CREATE_CACHED;
  else
    return OS21_FAILURE; /* Couldn't figure it out... */
#endif
  return OS21_SUCCESS;
}

unsigned int vmem_min_page_size (void)
{
#ifdef __st231__
  return mmap_pagesize(1);
#endif
  return 4096;
}

#ifdef __st231__
int scu_enable_range (void * startAddr, unsigned int length)
{
    return mmap_enable_speculation(startAddr, length);
}

int scu_disable_range (void * startAddr)
{
    return mmap_disable_speculation(startAddr);
}
#endif /* __st231__ */

#ifdef __sh__
int _st40_vmem_enhanced_mode (void)
{
    /* No _st40_vmem_enhanced_mode symbol so we must be linked against
     * an old 29-bit only capable OS21
     */
    return EMBX_FALSE;
}
#endif

#endif /* OS21_VERSION_MAJOR < 3 || __st220__ */
