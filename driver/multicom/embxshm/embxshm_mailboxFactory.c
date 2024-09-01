/**************************************************************/
/* Copyright STMicroelectronics 2003, all rights reserved     */
/*                                                            */
/* File: embxshm_mailboxFactory.c                             */
/*                                                            */
/* Description:                                               */
/*    Factory function to configure the transport by reading  */
/*    data from the mailbox registers                         */
/*                                                            */
/**************************************************************/

#include "embx_osheaders.h"
#include "embx_osinterface.h"
#include "embxP.h"
#include "embxshmP.h"
#include "embxshm.h"
#include "embxmailbox.h"

/* Enable/Disable the SHM Cached Heap */
#ifdef EMBXSHM_CACHED_HEAP
#define CACHED_HEAP	1
#else
#define CACHED_HEAP	0
#endif

/* SH4 29-bit mode specific macro to strip out the P1/P2 seg bits from an address giving us a true physical one
 *
 * This allows us to support users who haven't (yet) changed their 29-bit warpRange config addresses to use real
 * physical addresses. We must mask out the 29-bit upper bits or else the ST200 code will map the incorrect memory
 */
#if defined(__LINUX__) && defined(__KERNEL__) && defined(__SH4__) && !defined(CONFIG_32BIT)
#define EMBX_PADDR(addr) (void *)PHYSADDR((EMBX_UINT)addr)
#elif defined(__OS21__) && defined(__SH4__)
/* Only convert the address when NOT running in SE Mode */
#define EMBX_PADDR(addr) (EMBX_OS_SEMode() ? (addr) : (void *)ST40_PHYS_ADDR((EMBX_UINT)addr))
#else
/* All other cases leave addr untouched, i.e. assume its a true physical one already */
#define EMBX_PADDR(addr) (addr)
#endif

#if (defined __SH4__    && defined __OS21__   ) || \
    (defined __SH4__    && defined __LINUX__) || \
    (defined __SH4__    && defined __WINCE__) || \
    (defined __ST200__  && defined __OS21__   )


typedef struct EMBXSHM_MailboxFactoryData 
{
	EMBX_Mailbox_t *mailbox[EMBXSHM_MAX_CPUS];
	EMBX_ERROR (*expose_shared_memory)(EMBXSHM_Transport_t *tpshm);

	/* these are used to allow the automatically allocated shared heap to be freed again */
	void *shared_addr_or_null;
	unsigned int shared_size_or_zero;
} EMBXSHM_MailboxFactoryData_t;


/*
 * make the system explode if an interrupt goes off unexpectedly
 */
static void illegalInterruptHandler(void *p)
{
    EMBX_Assert(0);
}

/*
 * enable the mailbox interrupt and update the interrupt handler
 */
static void install_isr (EMBXSHM_Transport_t *tpshm, void (*handler)(void *))
{
    EMBXSHM_MailboxFactoryData_t *mfd = (EMBXSHM_MailboxFactoryData_t *) tpshm->factoryData;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>install_isr(0x%08x)\n", (unsigned) tpshm));
    EMBX_Mailbox_UpdateInterruptHandler(mfd->mailbox[tpshm->cpuID], handler, (void *) tpshm);
    EMBX_Mailbox_InterruptEnable(mfd->mailbox[tpshm->cpuID], 0);
    EMBX_Info(EMBX_INFO_FACTORY, ("<<<install_isr\n"));;

}

/*
 * disable the mailbox interrupt and remove the interrupt handler
 */
static void remove_isr (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_MailboxFactoryData_t *mfd = (EMBXSHM_MailboxFactoryData_t *) tpshm->factoryData;
    
    EMBX_Info(EMBX_INFO_FACTORY, (">>>remove_isr(0x%08x)\n", (unsigned) tpshm));
    EMBX_Mailbox_InterruptDisable(mfd->mailbox[tpshm->cpuID], 0);
    EMBX_Mailbox_UpdateInterruptHandler(mfd->mailbox[tpshm->cpuID], illegalInterruptHandler, NULL);
    EMBX_Info(EMBX_INFO_FACTORY, ("<<<remove_isr\n"));
}

/*
 * cause an interrupt to raised on the other CPU
 */
static void raise_int (EMBXSHM_Transport_t *tpshm, EMBX_UINT destCPU)
{
    EMBXSHM_MailboxFactoryData_t *mfd = (EMBXSHM_MailboxFactoryData_t *) tpshm->factoryData;

/*     EMBX_Info(EMBX_INFO_FACTORY, (">>>raise_int(0x%08x, %d)\n", (unsigned) tpshm, destCPU)); */
    EMBX_Mailbox_InterruptRaise(mfd->mailbox[destCPU], 0);
/*     EMBX_Info(EMBX_INFO_FACTORY, ("<<<raise_int\n")); */
}

/*
 * clear an interrupt asserted on this cpu
 */
static void clear_int (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_MailboxFactoryData_t *mfd = (EMBXSHM_MailboxFactoryData_t *) tpshm->factoryData;

    EMBX_Info(EMBX_INFO_INTERRUPT, (">>>clear_int(0x%08x)\n", (unsigned) tpshm));
    EMBX_Mailbox_InterruptClear(mfd->mailbox[tpshm->cpuID], 0);
    EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<clear_int\n"));
}

/*
 * unmask interrupts asserts on this cpu
 */
static void enable_int (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_MailboxFactoryData_t *mfd = (EMBXSHM_MailboxFactoryData_t *) tpshm->factoryData;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>enable_int(0x%08x)\n", (unsigned) tpshm));
    EMBX_Mailbox_InterruptEnable(mfd->mailbox[tpshm->cpuID], 0);
    EMBX_Info(EMBX_INFO_FACTORY, ("<<<enable_int\n"));
}

/*
 * mask interrupts asserts on this cpu
 */
static void disable_int (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_MailboxFactoryData_t *mfd = (EMBXSHM_MailboxFactoryData_t *) tpshm->factoryData;

    EMBX_Info(EMBX_INFO_INTERRUPT, (">>>disable_int(0x%08x)\n", (unsigned) tpshm));
    EMBX_Mailbox_InterruptDisable(mfd->mailbox[tpshm->cpuID], 0);
    EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<disable_int\n"));
}

/*
 * this transport does not have a buffer to flush
 */
static void buffer_flush (EMBXSHM_Transport_t *tpshm, void *address)
{
}

/*
 * this function will use mailbox token syncronization to attach to
 * the other participants in this transport.
 *
 * no CPU will exit this function until it has connected to its
 * peers. Additionally the slave CPUs will not exit until they
 * are fully connected *and* they have observed an advertisment
 * from the master.
 */
static EMBX_ERROR connectMailboxes(EMBXSHM_Transport_t *tpshm, EMBXSHM_MailboxConfig_t **pConfig)
{
    EMBXSHM_MailboxFactoryData_t *mfd = (EMBXSHM_MailboxFactoryData_t *) tpshm->factoryData;
    EMBX_UINT cpuID = tpshm->cpuID;
    EMBX_ERROR res;
    EMBX_INT  i;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>connectMailboxes(0x%08x, ...)\n", (unsigned) tpshm));

    /* allocate an entry in the local mailbox */
    res = EMBX_Mailbox_Alloc(illegalInterruptHandler, NULL, &(mfd->mailbox[cpuID]));
    if (EMBX_SUCCESS != res)
    {
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<connectMailboxes = %d\n", res));
	return res;
    }

    EMBX_Assert(0 == EMBX_Mailbox_StatusGet(mfd->mailbox[cpuID]));

    /* example connection sequence for four CPUs
     *
     *	0	1	2	3
     *
     *	SH03	SH13	SH23
     *	SH02	SH12		SH23
     *	SH01		SH12	SH13
     *		SH01	SH02	SH03
     *
     * We order in this odd manner so that the master (CPU 0)
     * will not complete the token exhange until all other
     * CPUs have completed their token exchanges. This allows
     * the master to send the address of the configuration
     * block to the other CPUs without risking colliding with
     * their syncronisations.
     */

    /* obtain the handle to any remote mailboxes that we have */
    for (i=(EMBXSHM_MAX_CPUS-1); i >= 0; i--) 
    {
	union { char c[4]; EMBX_UINT n; } token = { { 'S', 'H' } };

	if (tpshm->participants[i] && i != cpuID) 
	{
	    if (i < tpshm->cpuID)
	    {
		token.c[2] = '0' + i;
		token.c[3] = '0' + cpuID;
	    }
	    else
	    {
		token.c[2] = '0' + cpuID;
		token.c[3] = '0' + i;
	    }

	    EMBX_Info(EMBX_INFO_FACTORY, ("   synchronizing with SH%c%c\n", token.c[2], token.c[3]));
	    res = EMBX_Mailbox_Synchronize(mfd->mailbox[cpuID], token.n, &(mfd->mailbox[i]));
	    EMBX_Assert(EMBX_SUCCESS == res);

	    if (0 == cpuID) 
	    {
		/* as the master processor we must for our partner to reset their
		 * status register then post the address of the configuration
		 * data to that mailbox.
		 */
		
		EMBX_Info(EMBX_INFO_FACTORY, ("   waiting to post configuration to slave mailbox\n"));

		while (0 != EMBX_Mailbox_StatusGet(mfd->mailbox[i]))
		{
		    EMBX_OS_Delay(10);
		}

		EMBX_Info(EMBX_INFO_FACTORY, ("   posting 0x%08x (derived from 0x%08x) to slave\n", 
		                              (unsigned) LOCAL_TO_BUS(*pConfig, tpshm->pointerWarp),
					      (unsigned) *pConfig));
		EMBX_Mailbox_StatusSet(mfd->mailbox[i], (int) LOCAL_TO_BUS(*pConfig, tpshm->pointerWarp));
		
		/* now we must wait for the slave to acknowledge receipt of its
		 * configuration. this is required because as soon as the master
		 * has finished connecting to the mailboxes it will start booting
		 * which can interfere with the posted value.
		 */
		while (0 != EMBX_Mailbox_StatusGet(mfd->mailbox[i]))
		{
		    EMBX_OS_Delay(10);
		}
	    }
	    else if (0 == i)
	    {
		/* as a slave processor that has just sychronized with the master
		 * we will be able to read a configuration address from our 
		 * mailbox.
		 */
		
		EMBX_Info(EMBX_INFO_FACTORY, ("   waiting for master to  post configuration\n"));

		while (0 == EMBX_Mailbox_StatusGet(mfd->mailbox[cpuID]))
		{
		    EMBX_OS_Delay(10);
		}

		*pConfig = BUS_TO_LOCAL(EMBX_Mailbox_StatusGet(mfd->mailbox[cpuID]), tpshm->pointerWarp);
		EMBX_Mailbox_StatusSet(mfd->mailbox[cpuID], 0);
		EMBX_Info(EMBX_INFO_FACTORY, ("   received 0x%08x from master\n", (unsigned) *pConfig));
	    }
	    else
	    {
	    	/* as a slave who has not yet sychronized with the master (due to
	    	 * the ordering properties of the token exchange) our status must
	    	 * still be zero
	    	 */
		EMBX_Assert(0 == EMBX_Mailbox_StatusGet(mfd->mailbox[cpuID]));
	    }
	}
    }

    /* it is possible that the master processor has already raised an interrupt
     * on this processor therefore we ignore the state of the first bit
     */
    EMBX_Assert(0 == (0xfffffffe & EMBX_Mailbox_StatusGet(mfd->mailbox[cpuID])));

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<connectMailboxes = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * this transport performs the configuration that it was too early
 * to perform in the factory function.
 *
 * note on forms of addresses: 
 * 
 * for user convenience both sharedAddr and warpRangeAddr are specified
 * as local addresses in the config structure (i.e. the master). for
 * implementation convenience these pointers are passed to slaves as
 * bus addresses. we are not allowed to modify these configurations
 * since they
 */
static EMBX_ERROR master_configureTransport(EMBXSHM_Transport_t *tpshm, EMBXSHM_MailboxConfig_t *config)
{
    EMBXSHM_MailboxFactoryData_t *mfd = tpshm->factoryData;
    void *localSharedAddr;
    EMBX_UINT localSharedpAddr;
    EMBX_ERROR res;

    EMBX_Assert(0 == tpshm->cpuID);

    EMBX_Info(EMBX_INFO_FACTORY, (">>>configureTransport(0x%08x, 0x%08x)\n", (unsigned) tpshm, (unsigned) config));

    EMBXSHM_READS(config);

    /* a note on forms of addresses
     *
     * for user convenience the sharedAddr is still specified
     * as a local address in the config structure (i.e. the master). 
     *
     * MULTICOM_32BIT_SUPPORT: The warpRangeAddr should be a physical address when
     * running in 32-bit mode but in 29-bit mode P1/P2 addresses are still allowed.
     *
     * for implementation convenience these pointers are passed to slaves 
     * as bus addresses (since the master cannot convert them to slave local
     * addresses since it does not know what the pointer warp is). we are not
     * allowed to directly modify the master supplied configuration structure
     * since this is shared by multiple slaves. thus we treat the configuration
     * structure differently for a master vesus a slave and store the result
     * in local variables.
     */
    localSharedAddr = config->sharedAddr;
    EMBX_Info(EMBX_INFO_FACTORY, ("   old localSharedAddr = 0x%08x\n", (unsigned)localSharedAddr));

    /* allocate the memory if the shared address is NULL */
    if (NULL == localSharedAddr)
    {
	localSharedAddr = EMBX_OS_ContigMemAlloc(config->sharedSize, config->sharedSize);
	if (NULL == localSharedAddr) 
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:1\n"));
	    return EMBX_NOMEM;
	}
	
	EMBX_Info(EMBX_INFO_FACTORY, ("   new localSharedAddr = 0x%08x\n", (unsigned)localSharedAddr));

	/* stash the original pointer safely away so we can free it later */
	mfd->shared_addr_or_null = localSharedAddr;
	mfd->shared_size_or_zero = config->sharedSize;
	
	res = EMBX_OS_VirtToPhys(localSharedAddr, &localSharedpAddr);
	EMBX_Assert(res != EMBX_INVALID_ARGUMENT);
	
	/*
	 * Remap the physical memory we have just allocated either as cached or uncached
	 */
	localSharedAddr = EMBX_OS_PhysMemMap(localSharedpAddr, config->sharedSize, CACHED_HEAP);
	
	if (NULL == localSharedAddr) 
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:2\n"));
	    return EMBX_NOMEM;
	}

	EMBX_Info(EMBX_INFO_FACTORY, ("   map localSharedAddr = 0x%08x localSharedpAddr 0x%08x\n", 
				      (unsigned)localSharedAddr, localSharedpAddr));
    }

    /* Here we assume we have a virtual localSharedAddr */
    EMBX_Assert(localSharedAddr != NULL);
    res = EMBX_OS_VirtToPhys(localSharedAddr, &localSharedpAddr);
    EMBX_Assert(res != EMBX_INVALID_ARGUMENT);
    EMBX_Assert(CACHED_HEAP || res != EMBX_INCOHERENT_MEMORY);

    /* MULTICOM_32BIT_SUPPORT: Overwrite the supplied Warp delta to match the Shared heap mapping */
    tpshm->pointerWarp = ((char *) localSharedpAddr - (char *) localSharedAddr);
    
    /* configure the parts of the transport structure that can
     * be determined only from a complete factory config.
     */
    tpshm->transport.transportInfo.maxPorts = config->maxPorts;
    tpshm->objectTableSize		    = (config->maxObjects > 0 ? 
                                               config->maxObjects :
					       EMBX_HANDLE_DEFAULT_TABLE_SIZE);
    tpshm->freeListSize			    = config->freeListSize;
    tpshm->tcb				    = localSharedAddr;
    tpshm->heap				    = (void *) ((uintptr_t) localSharedAddr + sizeof(EMBXSHM_TCB_t));
    tpshm->heapSize                         = config->sharedSize - sizeof(EMBXSHM_TCB_t);

    /* MULTICOM_32BIT_SUPPORT: Save the Warp range as a physical address range */
    if (0 != config->warpRangeSize) {
	tpshm->warpRangeAddr = EMBX_PADDR(config->warpRangeAddr);
	tpshm->warpRangeSize = config->warpRangeSize;

	/* Shared memory segment must be enclosed by the Primary Warp Range */
	EMBX_Assert(localSharedpAddr >= (unsigned) tpshm->warpRangeAddr && localSharedpAddr < ((unsigned) tpshm->warpRangeAddr + tpshm->warpRangeSize));

    } else {
	res = EMBX_OS_VirtToPhys(tpshm->heap, (EMBX_UINT *)&tpshm->warpRangeAddr);
	EMBX_Assert(res != EMBX_INVALID_ARGUMENT);
	tpshm->warpRangeSize = tpshm->heapSize;
    }

    EMBX_Assert(0 != tpshm->warpRangeSize);

    /* MULTICOM_32BIT_SUPPORT: Secondary Warp range support */
    if (0 != config->warpRangeSize2) {
	tpshm->warpRangeAddr2 = EMBX_PADDR(config->warpRangeAddr2);
	tpshm->warpRangeSize2 = config->warpRangeSize2;
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("   pointerWarp = 0x%08x [0x%08x]\n", (unsigned) tpshm->pointerWarp, (unsigned) config->pointerWarp));
    EMBX_Info(EMBX_INFO_FACTORY, ("   sharedAddr = 0x%08x [0x%08x]\n", (unsigned) localSharedAddr, (unsigned) config->sharedAddr));
    EMBX_Info(EMBX_INFO_FACTORY, ("   sharedSize = %d\n", config->sharedSize));
    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeAddr = 0x%08x [0x%08x]\n", (unsigned) tpshm->warpRangeAddr, (unsigned) config->warpRangeAddr));
    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeSize = 0x%08x [0x%08x]\n", tpshm->warpRangeSize, config->warpRangeSize));

    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeAddr2 = 0x%08x warpRangeSize2 = 0x%08x\n", (unsigned) tpshm->warpRangeAddr2, tpshm->warpRangeSize2));

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

static EMBX_ERROR slave_configureTransport(EMBXSHM_Transport_t *tpshm, EMBXSHM_MailboxConfig_t *config)
{
    void *localSharedAddr, *localWarpAddr;

    EMBX_Assert(0 != tpshm->cpuID);

    EMBX_Info(EMBX_INFO_FACTORY, (">>>configureTransport(0x%08x, 0x%08x)\n", (unsigned) tpshm, (unsigned) config));

    /* we may not be able to directly address the configuration pointer because it sits
     * in another CPU's protection zone 
     */
    {
	EMBXSHM_MailboxConfig_t *virtualConfig;
	void *sharedAddr;
	unsigned int sharedSize;

	/* Map the config structure into addressable space */
	virtualConfig = EMBX_OS_PhysMemMap((EMBX_UINT)config, sizeof(EMBXSHM_MailboxConfig_t), CACHED_HEAP);

	EMBX_Info(EMBX_INFO_FACTORY, ("   configureTransport: virtualConfig = 0x%08x\n", 
	                             (unsigned) virtualConfig));

	if (NULL == virtualConfig) 
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:0\n"));
	    return EMBX_NOMEM;
	}
	
#if defined __ST231__
	/* find the bounds of the shared memory without using dismissible loads */
	__asm__ ("ldw %0 = 0,%1" : "=r" (sharedAddr) : "r" (&(virtualConfig->sharedAddr)));
	__asm__ ("ldw %0 = 0,%1" : "=r" (sharedSize) : "r" (&(virtualConfig->sharedSize)));

	/* enable speculation for this shared memory. failure is not fatal since OS21 does
	 * not distingish between 'failed' and 'already the case, thank you flower'.
	 */
	if (EMBX_OS_EnableSpeculation(sharedAddr, sharedSize))
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("   configureTransport: Failed to Enable speculation for 0x%p-0x%p\n",
					  sharedAddr, (void *) ((uintptr_t)sharedAddr + sharedSize)));
	}
#else
	sharedAddr = virtualConfig->sharedAddr;
	sharedSize = virtualConfig->sharedSize;
#endif /* __ST231__ */

	config = virtualConfig;
    }

    EMBXSHM_READS(config);

    /* a note on forms of addresses
     *
     * for implementation convenience these pointers are passed to slaves 
     * as bus addresses
     */

    localSharedAddr = config->sharedAddr;
    EMBX_Info(EMBX_INFO_FACTORY, ("   old localSharedAddr = 0x%08x\n", (unsigned)localSharedAddr));
    /* it is 'impossible' for the local address to be NULL */
    EMBX_Assert(NULL != localSharedAddr);

    {
	EMBX_VOID *vaddr;
	EMBX_UINT  pAddr;
	EMBX_VOID *warpRangeAddr;
	EMBX_UINT  warpRangeSize;

	if (config->warpRangeSize != 0)
	{
	    warpRangeAddr = config->warpRangeAddr;
	    warpRangeSize = config->warpRangeSize;

	} else {
	    warpRangeAddr = localSharedAddr;
	    warpRangeSize = config->sharedSize;
	}

	EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeAddr = 0x%08x size = 0x%08x\n", (unsigned)warpRangeAddr, warpRangeSize));

	/* Shared memory segment must be enclosed by the Primary Warp Range */
	EMBX_Assert(localSharedAddr >= warpRangeAddr && localSharedAddr < (EMBX_VOID *)((unsigned) warpRangeAddr + warpRangeSize));
      
	/* Assume warpRangeAddr is supplied as a physical address */
	pAddr = (EMBX_UINT)warpRangeAddr;

	/* Map the whole of the Warp range into VMEM */
	vaddr = EMBX_OS_PhysMemMap(pAddr, warpRangeSize, CACHED_HEAP);
      
	if (NULL == vaddr) {
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:1\n"));
	    return EMBX_NOMEM;
	}
      
	EMBX_Info(EMBX_INFO_FACTORY, ("   mapped warpRangeAddr = 0x%08x [0x%08x]\n", (unsigned)vaddr, (unsigned) pAddr));
      
	/* Overwrite the Warp delta to map to the VMEM Warp mapping */
	tpshm->pointerWarp = ((char *) pAddr - (char *) vaddr);

	EMBX_Info(EMBX_INFO_FACTORY, ("   pointerWarp = 0x%08x\n", tpshm->pointerWarp));
      
	/* update the local pointer to the shared heap */
	localSharedAddr = BUS_TO_LOCAL(localSharedAddr, tpshm->pointerWarp);

	EMBX_Info(EMBX_INFO_FACTORY, ("   new  localSharedAddr = 0x%08x\n", (unsigned)localSharedAddr));

	/* stash the local address of the Warp base */
	localWarpAddr = vaddr;
    }

    /* MULTICOM_32BIT_SUPPORT: Secondary Warp range support */
    if (0 != config->warpRangeSize2) 
    {
	EMBX_VOID *vaddr;
	EMBX_UINT  pAddr;

	EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeAddr2 = 0x%08x size = 0x%08x\n", (unsigned)config->warpRangeAddr2, config->warpRangeSize2));
      
	/* Assume warpRangeAddr2 is supplied as a physical address */
	pAddr = (EMBX_UINT) config->warpRangeAddr2;

	/* Map the whole of the Warp range into VMEM */
	vaddr = EMBX_OS_PhysMemMap(pAddr, config->warpRangeSize2, CACHED_HEAP);
      
	if (NULL == vaddr) {
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_NOMEM:1\n"));
	    return EMBX_NOMEM;
	}
      
	EMBX_Info(EMBX_INFO_FACTORY, ("   mapped warpRangeAddr2 = 0x%08x [0x%08x]\n", (unsigned)vaddr, (unsigned) pAddr));
      
	/* Generate the Warp delta to map to the Secondary VMEM Warp mapping */
	tpshm->pointerWarp2 = ((char *) pAddr - (char *) vaddr);
      
	EMBX_Info(EMBX_INFO_FACTORY, ("   pointerWarp2 = 0x%08x\n", tpshm->pointerWarp2));

	/* stash the physical address of the Secondary Warp base */
	tpshm->warpRangeAddr2 = config->warpRangeAddr2;
	tpshm->warpRangeSize2 = config->warpRangeSize2;
    }
    
    /* configure the parts of the transport structure that can
     * be determined only from a complete factory config.
     */
    tpshm->transport.transportInfo.maxPorts = config->maxPorts;
    tpshm->objectTableSize		    = (config->maxObjects > 0 ? 
                                               config->maxObjects :
					       EMBX_HANDLE_DEFAULT_TABLE_SIZE);
    tpshm->freeListSize			    = config->freeListSize;
    tpshm->tcb				    = localSharedAddr;
    tpshm->heap				    = (void *) ((uintptr_t) localSharedAddr + sizeof(EMBXSHM_TCB_t));
    tpshm->heapSize                         = config->sharedSize - sizeof(EMBXSHM_TCB_t);

    /* MULTICOM_32BIT_SUPPORT: Save the Warp range as a physical address range */
    if (0 != config->warpRangeSize) {
	tpshm->warpRangeAddr = config->warpRangeAddr;
	tpshm->warpRangeSize = config->warpRangeSize;
    } else {
//	EMBX_ERROR res = EMBX_OS_VirtToPhys(tpshm->heap, (EMBX_UINT *)&tpshm->warpRangeAddr);
	EMBX_Assert(res != EMBX_INVALID_ARGUMENT);
	tpshm->warpRangeSize = tpshm->heapSize;
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("   pointerWarp = 0x%08x [0x%08x]\n", (unsigned) tpshm->pointerWarp, (unsigned) config->pointerWarp));
    EMBX_Info(EMBX_INFO_FACTORY, ("   sharedAddr = 0x%08x [0x%08x]\n", (unsigned) localSharedAddr, (unsigned) config->sharedAddr));
    EMBX_Info(EMBX_INFO_FACTORY, ("   sharedSize = %d\n", config->sharedSize));
    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeAddr = 0x%08x [0x%08x]\n", (unsigned) tpshm->warpRangeAddr, (unsigned) config->warpRangeAddr));
    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeSize = 0x%08x [0x%08x]\n", tpshm->warpRangeSize, config->warpRangeSize));

    EMBX_Info(EMBX_INFO_FACTORY, ("   warpRangeAddr2 = 0x%08x warpRangeSize2 = 0x%08x\n", (unsigned) tpshm->warpRangeAddr2, tpshm->warpRangeSize2));

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<configureTransport = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

/*
 * setup the shared memory environment for the master
 */
static EMBX_ERROR setup_master_environment(EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_MailboxConfig_t *config = tpshm->factoryConfig;
    EMBXSHM_MailboxFactoryData_t *mfd = tpshm->factoryData;
    EMBXSHM_MailboxConfig_t *remoteConfig;
    EMBX_ERROR res;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>setup_master_environment(0x%08x)\n", (unsigned) tpshm));

    EMBX_Assert(0 == tpshm->masterCPU);
    EMBX_Assert(0 == tpshm->cpuID);

    /* complete the configuration from the supplied config structure */
    res = master_configureTransport(tpshm, config);
    if (EMBX_SUCCESS != res) 
    {
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_master_environment = %d\n", res));
	return res;
    }

    /* provide an opportunity to expose the shared memory */
    if (mfd->expose_shared_memory)
    {
	res = mfd->expose_shared_memory(tpshm);
	if (EMBX_SUCCESS != res)
	{
	    goto error_exit;
	}
    }
    
    /* we can now perform the generic initialization code */
    res = EMBXSHM_genericMasterInit(tpshm);
    if (EMBX_SUCCESS != res)
    {
	goto error_exit;
    }

    /* allocate the block to be sent to the slaves containing all our
     * configuration information (which cannot fail since we guaranteed
     * certain amount of free memory in the shared heap when our
     * factory was registered)
     *
     * this block will never be freed - it is required by the slaves for
     * an undefined period of time
     */
    remoteConfig = (EMBXSHM_MailboxConfig_t *) EMBXSHM_malloc(tpshm, sizeof(*remoteConfig));
    EMBX_Assert(NULL != remoteConfig);

    /* set up the block */
    *remoteConfig = *config;
    remoteConfig->sharedAddr = LOCAL_TO_BUS(tpshm->tcb, tpshm->pointerWarp);
    /* MULTICOM_32BIT_SUPPORT: warpRangeAddr must be a physical address */
    remoteConfig->warpRangeAddr = tpshm->warpRangeAddr;
    remoteConfig->warpRangeAddr2 = tpshm->warpRangeAddr2;

    EMBXSHM_WROTE(remoteConfig);

    /* connect to the mailboxes of other devices */
    res = connectMailboxes(tpshm, &remoteConfig);
    if (EMBX_SUCCESS != res)
    {
	goto error_exit;
    }
    
    EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_master_environment = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;

error_exit:

    /* undo configureTransport */
    if (mfd->shared_addr_or_null) {
	EMBX_OS_ContigMemFree(mfd->shared_addr_or_null, mfd->shared_size_or_zero);
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_master_environment = %d\n", res));
    return res;
}

/*
 * setup the shared memory environment for the slave. this only
 * really involved pluging the previously used components together
 * in a different order.
 */
static EMBX_ERROR setup_slave_environment(EMBXSHM_Transport_t *tpshm)
{
    EMBX_ERROR res;
    EMBXSHM_MailboxConfig_t *config; 
    EMBXSHM_MailboxFactoryData_t *mfd = tpshm->factoryData;
    EMBX_INT i;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>setup_slave_environment(0x%08x)\n", (unsigned) tpshm));

    res = connectMailboxes(tpshm, &config);
    if (EMBX_SUCCESS != res)
    {
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_slave_environment = %d\n", res));
	return res;
    }

    res = slave_configureTransport(tpshm, config);
    EMBX_Assert(EMBX_SUCCESS == res); /* can't fail if we are a slave */
    
    if (EMBX_SUCCESS != res)
	goto error_exit;
    
    res = EMBXSHM_genericSlaveInit(tpshm);
    if (EMBX_SUCCESS != res)
	goto error_exit;

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_slave_environment = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;

error_exit:
    /* undo connectMailboxes() */
    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
	EMBX_Mailbox_Free(mfd->mailbox[i]);
    }
    
    EMBX_Info(EMBX_INFO_FACTORY, ("<<<setup_slave_environment = %d\n", res));
    return res;
}


/*
 * cleanup any allocations performed by this file except the allocation on tpshm itself
 * which is cleaned up at the framework level
 */
static void cleanup_master_environment(EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_MailboxFactoryData_t *mfd = tpshm->factoryData;
    EMBX_INT i;

    /* undo connectMailboxes() */
    for (i=0; i<EMBXSHM_MAX_CPUS; i++)
    {
	EMBX_Mailbox_Free(mfd->mailbox[i]);
    }

    if (mfd->shared_addr_or_null) {
      
        /* Need to unmap the (un)cached map of the shared heap too */
	if (tpshm->tcb != (void *)mfd->shared_addr_or_null)
	    EMBX_OS_PhysMemUnMap(tpshm->tcb);
	  
      EMBX_OS_ContigMemFree(mfd->shared_addr_or_null, mfd->shared_size_or_zero);
    }
    EMBX_OS_MemFree (mfd);
    tpshm->factoryData = NULL;
}

static void cleanup_slave_environment(EMBXSHM_Transport_t *tpshm)
{
    /* we do exactly the same for the slave as we do for the master */
    cleanup_master_environment(tpshm);

    /* Need to unmap the (un)cached map of the warp memory */
    EMBX_OS_PhysMemUnMap(BUS_TO_LOCAL(tpshm->warpRangeAddr, tpshm->pointerWarp));
    
    if (tpshm->warpRangeSize2)
	/* Need to unmap the (un)cached map of the Secondary warp memory */
	EMBX_OS_PhysMemUnMap(BUS_TO_LOCAL(tpshm->warpRangeAddr2, tpshm->pointerWarp2));
}


/*
 * this the mailbox factory function, it is used to register both
 * master and slave transports and works in nearly the same way
 * in each case (arguments checking and one of the specialization
 * functions are the only points of difference.
 */
#ifdef EMBXSHM_CACHED_HEAP
EMBX_Transport_t *EMBXSHMC_mailbox_factory(void *param)
#else
EMBX_Transport_t *EMBXSHM_mailbox_factory(void *param)
#endif
{
    EMBXSHM_MailboxConfig_t *config = (EMBXSHM_MailboxConfig_t *) param;
    EMBXSHM_Transport_t *tpshm;
    EMBXSHM_MailboxFactoryData_t *mfd;
    EMBX_INT i;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>EMBXSHM_mailbox_factory(0x%08x)\n", (unsigned) param));

    /* check the validity of the generic parts of the template */
    if ( (NULL == config) ||
         (NULL == config->name) ||
	 (   0 == config->participants[0]) ||
	 (   0 == config->participants[config->cpuID]) ||
	 (config->cpuID >= EMBXSHM_MAX_CPUS) )
    {
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_mailbox_factory = NULL:0\n"));
	return NULL;
    }

    /* check the template follows the appropriate form for a master of a slave */
    if (0 == config->cpuID)
    {
	/* we are a master */
	if ( (sizeof(EMBXSHM_TCB_t) > config->sharedSize) ||
	     (0 == config->freeListSize) )
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_mailbox_factory = NULL:1\n"));
	    return NULL;
	}
    }
    else
    {
	/* we are a slave */
	if ( (0 != config->sharedAddr) ||
	     (0 != config->sharedSize) ||
	     (0 != config->maxPorts) ||
	     (0 != config->maxObjects) ||
	     (0 != config->freeListSize) )
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_mailbox_factory = NULL:2\n"));
	    return NULL;
	}
    }

    /* memory permitting we know we have a good configuration at this point */

    /* allocate memory for our transport structure (and cope with failures) */
    tpshm = (EMBXSHM_Transport_t *) EMBX_OS_MemAlloc (sizeof(*tpshm));
    mfd = (EMBXSHM_MailboxFactoryData_t *) EMBX_OS_MemAlloc (sizeof(*mfd));
    if (NULL == tpshm || NULL == mfd)
    {
	EMBX_OS_MemFree (tpshm);
	EMBX_OS_MemFree (mfd);
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_mailbox_factory = NULL:3\n"));
	return NULL;
    }

    /* Clear down the initial transport state */
    memset (tpshm, 0, sizeof(*tpshm));

    /* initialize the factory data */
    memset (mfd, 0, sizeof(*mfd));

    /* initialize the transport structure */
    memcpy (tpshm, &EMBXSHM_transportTemplate, sizeof(EMBXSHM_transportTemplate));
    tpshm->transport.transportInfo.allowsMultipleConnections = EMBX_TRUE;
    tpshm->masterCPU       = 0;

    /* copy the details from the generic portion of the config structure */
    strcpy (tpshm->transport.transportInfo.name, config->name);
    for (i=0; i<EMBXSHM_MAX_CPUS; i++) 
    {
	tpshm->participants[i] = config->participants[i];

	/* find the highest id within the system */
	if (tpshm->participants[i])
	{
	    tpshm->maxCPU = i;
	}
    }
    tpshm->cpuID           = config->cpuID;
    tpshm->pointerWarp     = config->pointerWarp;

    /* setup links to the factory specific structures */
    tpshm->factoryConfig   = config;
    tpshm->factoryData     = mfd;
    
    /* setup the behavioral specializations */
    tpshm->install_isr     = install_isr;
    tpshm->remove_isr      = remove_isr;
    tpshm->raise_int       = raise_int;
    tpshm->clear_int       = clear_int;
    tpshm->enable_int      = enable_int;
    tpshm->disable_int     = disable_int;
    tpshm->buffer_flush    = buffer_flush;
    tpshm->setup_shm_environment = (0 == config->cpuID ? setup_master_environment : setup_slave_environment);
    tpshm->cleanup_shm_environment = (0 == config->cpuID ? cleanup_master_environment : cleanup_slave_environment);

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_mailbox_factory = 0x%08x\n", (unsigned) tpshm));
    return (EMBX_Transport_t *) tpshm;
}


/*
 * Functions to extend the mailbox factory to support EMPI based platforms
 */

#define LREG_READ32(p, o) ((EMBX_UINT) EMBX_OS_LREG_READ32((uintptr_t) (p) + (o)))
#define LREG_WRITE32(p, o, v) EMBX_OS_LREG_WRITE32((uintptr_t) (p) + (o), (EMBX_UINT) (v))

enum {
    /* register address */
    EMPI_MPXCFG			    = (0x0028/1),
    EMPI_DMAINV 		    = (0x0030/1),
    EMPI_RBAR5			    = (0x02a0/1),
    EMPI_RSR5			    = (0x02a8/1),
    EMPI_RLAR5			    = (0x02b0/1),
    EMPI_RBAR6			    = (0x02c0/1),
    EMPI_RSR6			    = (0x02c8/1),
    EMPI_RLAR6			    = (0x02d0/1),
    EMPI_RBAR7			    = (0x02e0/1),
    EMPI_RSR7			    = (0x02e8/1),
    EMPI_RLAR7			    = (0x02f0/1),
    MPXARB_CONTROL		    = (0x8010/1),

    /* register bits */
    EMPI_MPXCFG_EN16		    = 0x00000002,

    EMPI_RBAR_ADDR_MASK             = 0x1fff0000,
    EMPI_RLAR_ADDR_MASK             = 0x1fff0000,

    EMPI_RSR_ENABLE		    = 0x00000001,
    EMPI_RSR_BUFFER3		    = 0x00000006,
    EMPI_RSR_WRITE_BYPASS	    = 0x00000008,
    EMPI_RSR_READ_BYPASS	    = 0x00000010,
    EMPI_RSR_SPACE_MASK             = 0x1fff0000,

    MPXARB_CONTROL_MODE_LOW	    = 0x00000010,
    MPXARB_CONTROL_MODE_MASK        = 0x00000030,
    MPXARB_CONTROL_PARK_EXTERNAL    = 0x000000c0,
    MPXARB_CONTROL_PARK_MASK        = 0x000000c0,
    MPXARB_CONTROL_BUS_TENURE32     = 0x00000100,
    MPXARB_CONTROL_BUS_TENURE512    = 0x00000500,
    MPXARB_CONTROL_BUS_TENURE_MASK  = 0x00000700
};

enum {
    SYSCONF_CONTROL2            = (0x0018/1)
};


/*
 * clear the EMPI read buffer (without invalidating the write buffer)
 */
static void empi_buffer_flush (EMBXSHM_Transport_t *tpshm, void *address)
{
    EMBXSHM_EMPIMailboxConfig_t *config = (EMBXSHM_EMPIMailboxConfig_t *) tpshm->factoryConfig;

    EMBX_Info(EMBX_INFO_INTERRUPT, (">>>buffer_flush(0x%08x, 0x%08x)\n", (unsigned) tpshm, (unsigned) address));

    /* this function should only be installed if we are not the master CPU */
    EMBX_Assert(0 != tpshm->cpuID);

    /* flush the read buffer for channel 0 */
    LREG_WRITE32(config->empiAddr, EMPI_DMAINV, 1);

    EMBX_Info(EMBX_INFO_INTERRUPT, ("<<<buffer_flush\n"));
}

static EMBX_ERROR empi_expose_shared_memory (EMBXSHM_Transport_t *tpshm)
{
    EMBXSHM_EMPIMailboxConfig_t *config = (EMBXSHM_EMPIMailboxConfig_t *) tpshm->factoryConfig;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>empi_expose_shared_memory(0x%08x)\n", (unsigned) tpshm));
    EMBX_Assert(0 == tpshm->cpuID);

    /* check if we have to allocate the shared memory ourselves */
    if (NULL == config->mailbox.sharedAddr)
    {
	EMBX_UINT bits;

	/* the factory function should guarantee these conditions are met */
	EMBX_Assert(0 == (config->mailbox.sharedSize & (config->mailbox.sharedSize - 1)));
	EMBX_Assert(NULL != config->empiAddr);

	/* check that the EMPI regions we will need have not already been programmed */
	if ((EMPI_RSR_ENABLE & LREG_READ32(config->empiAddr, EMPI_RSR5)) ||
	    (EMPI_RSR_ENABLE & LREG_READ32(config->empiAddr, EMPI_RSR6)) ||
            (EMPI_RSR_ENABLE & LREG_READ32(config->empiAddr, EMPI_RSR7)))
	{
	    EMBX_Info(EMBX_INFO_FACTORY, ("<<<empi_expose_shared_memory = EMBX_SYSTEM_ERROR\n"));
	    return EMBX_SYSTEM_ERROR;
	}

	/* enable 128-bit burst accesses */
	LREG_WRITE32(config->empiAddr, EMPI_MPXCFG, EMPI_MPXCFG_EN16);

	/* configure the MPX arbiter */
	bits = LREG_READ32(config->empiAddr, MPXARB_CONTROL);
	bits &= ~(MPXARB_CONTROL_MODE_MASK | 
		  MPXARB_CONTROL_PARK_MASK | 
		  MPXARB_CONTROL_BUS_TENURE_MASK);
	bits |=   MPXARB_CONTROL_MODE_LOW |
	          MPXARB_CONTROL_PARK_EXTERNAL |
		  MPXARB_CONTROL_BUS_TENURE32;
	LREG_WRITE32(config->empiAddr, MPXARB_CONTROL, bits);

	/* expose the EMPI registers via region 5 (with no buffering) */
	LREG_WRITE32(config->empiAddr, EMPI_RBAR5, 
		     EMPI_RBAR_ADDR_MASK & (EMBX_UINT) config->empiAddr);
	LREG_WRITE32(config->empiAddr, EMPI_RLAR5, 
		     EMPI_RLAR_ADDR_MASK & (EMBX_UINT) config->empiAddr);
	/* no SPACE bits means we expose 64K */
	LREG_WRITE32(config->empiAddr, EMPI_RSR5,
		     EMPI_RSR_ENABLE | EMPI_RSR_READ_BYPASS | EMPI_RSR_WRITE_BYPASS);

	/* expose the hardware mailbox via region 6 (with no buffering) */
	LREG_WRITE32(config->empiAddr, EMPI_RBAR6, 
		     EMPI_RBAR_ADDR_MASK & (EMBX_UINT) config->mailboxAddr);
	LREG_WRITE32(config->empiAddr, EMPI_RLAR6, 
		     EMPI_RLAR_ADDR_MASK & (EMBX_UINT) config->mailboxAddr);
	/* no SPACE bits means we expose 64K */
	LREG_WRITE32(config->empiAddr, EMPI_RSR6,
		     EMPI_RSR_ENABLE | EMPI_RSR_READ_BYPASS | EMPI_RSR_WRITE_BYPASS);

	/* expose the memory we have allocated via region 7 (with buffering) */
	LREG_WRITE32(config->empiAddr, EMPI_RBAR7,
		     EMPI_RBAR_ADDR_MASK & (EMBX_UINT) tpshm->tcb);
	LREG_WRITE32(config->empiAddr, EMPI_RLAR7,
		     EMPI_RBAR_ADDR_MASK & (EMBX_UINT) tpshm->tcb);
	LREG_WRITE32(config->empiAddr, EMPI_RSR7,
		     EMPI_RSR_ENABLE |
		     (EMPI_RSR_SPACE_MASK & (config->mailbox.sharedSize-1)));
    }

    /* perform any system configuration required by the config structure */
    if (NULL != config->sysconfAddr)
    {
	EMBX_UINT bits;
	bits = LREG_READ32(config->sysconfAddr, SYSCONF_CONTROL2);
	bits |= config->sysconfMaskSet;
	bits &= ~config->sysconfMaskClear;
	LREG_WRITE32(config->sysconfAddr, SYSCONF_CONTROL2, bits);
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<empi_expose_shared_memory = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

#ifdef EMBXSHM_CACHED_HEAP
EMBX_Transport_t *EMBXSHMC_empi_mailbox_factory(void *param)
#else
EMBX_Transport_t *EMBXSHM_empi_mailbox_factory(void *param)
#endif
{
    EMBXSHM_Transport_t *tpshm;
    EMBXSHM_MailboxFactoryData_t *mfd;

    EMBX_Info(EMBX_INFO_FACTORY, (">>>EMBXSHM_empi_mailbox_factory(0x%08x)\n", (unsigned) param));
    
#ifdef EMBXSHM_CACHED_HEAP
    tpshm = (EMBXSHM_Transport_t *) EMBXSHMC_mailbox_factory(param);
#else
    tpshm = (EMBXSHM_Transport_t *) EMBXSHM_mailbox_factory(param);
#endif
    if (tpshm)
    {
	/* if we are not the master CPU then we need to installed an
	 * active buffer flushing function
	 */
	if (0 != tpshm->cpuID) {
	    tpshm->buffer_flush = empi_buffer_flush;
	}

	mfd = tpshm->factoryData;
	mfd->expose_shared_memory = empi_expose_shared_memory;
    }

    EMBX_Info(EMBX_INFO_FACTORY, ("<<<EMBXSHM_empi_mailbox_factory = 0x%08x\n", (unsigned) tpshm));
    return (EMBX_Transport_t *) tpshm;
}

#endif /* valid CPU/OS combo */


/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */
