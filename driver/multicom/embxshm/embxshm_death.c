/**************************************************************/
/* Copyright STMicroelectronics 2006, all rights reserved     */
/*                                                            */
/* File: embxshm_death.c                                      */
/*                                                            */
/* Description:                                               */
/*    Some firmware authors believe it is not possible to     */
/*    prevent their firmware from crashing and demanded that  */
/*    a recovery mechanism be added to Multicom to allow it   */
/*    to cope with the random 'disconnection' of one of the   */
/*    physically attached processors.                         */
/*                                                            */
/*    This file is part of that recovery mechanism.           */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"
#include "embxP.h"

extern EMBX_HandleManager_t _embx_handle_manager;

EMBX_ERROR EMBXSHM_RemoveProcessor(EMBX_TRANSPORT htp, EMBX_UINT cpuID)
{
	EMBX_TransportHandle_t     *tph;
	EMBXSHM_Transport_t        *tpshm;
	EMBXSHM_TCB_t *tcb;

	if (0 == cpuID) {
		EMBX_DebugMessage(("Failed 'cannot remove host processor'\n"));
		return EMBX_INVALID_ARGUMENT;

	}
	
	if (cpuID >= EMBXSHM_MAX_CPUS) {
		EMBX_DebugMessage(("Failed 'CPU ID is too large'\n"));
		return EMBX_INVALID_ARGUMENT;
	}

	if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT)) {
		EMBX_DebugMessage(("Failed 'transport handle parameter is not a transport handle'\n"));
		return EMBX_INVALID_TRANSPORT;
	}

	tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
	if( (tph == 0) || (tph->state != EMBX_HANDLE_VALID) ) {
		EMBX_DebugMessage(("Failed 'transport handle is not valid'\n"));
		return EMBX_INVALID_TRANSPORT;
	}

	tpshm = (EMBXSHM_Transport_t *)tph->transport;
	tcb = tpshm->tcb;
	EMBXSHM_READS(tcb);

	/* mark the processor as inactive */
	tcb->activeCPUs[cpuID].marker = 0;
	EMBXSHM_WROTE(&tcb->activeCPUs[cpuID]);	

	/* update the local participants map used to determine when to send port
	 * open/close notifications. for remote processors this will be performed
	 * automatically [see update updateLocalParticipantsMap()] but we don't
	 * want to needlessly deadlock if the dead processors comms. pipe
	 * overflows.
	 */
	tpshm->participants[cpuID] = 0;

	return EMBX_SUCCESS;
}

#if defined(__LINUX__) && defined(__KERNEL__) && defined(MODULE)
EXPORT_SYMBOL( EMBXSHM_RemoveProcessor );
#endif
