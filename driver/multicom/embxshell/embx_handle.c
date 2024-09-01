/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_handle.c                                             */
/*                                                                 */
/* Description:                                                    */
/*         EMBX API Public Handle manager                          */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "embxshell.h"
#include "debug_ctrl.h"

EMBX_HANDLE EMBX_HANDLE_CREATE(EMBX_HandleManager_t *pMan,void *obj, EMBX_UINT type)
{
EMBX_HANDLE handle;

    if(pMan->nAllocated == pMan->size)
        return EMBX_INVALID_HANDLE_VALUE;


    if(pMan->lastFreed)
    {
        handle = pMan->lastFreed;
        pMan->lastFreed = 0;
    }
    else
    {
    EMBX_UINT oldIndex = pMan->nextIndex;

        while(EMBX_TRUE)
        {
            handle = pMan->table[pMan->nextIndex].handle;

            pMan->nextIndex++;
            if(pMan->nextIndex == pMan->size)
            {
                pMan->nextIndex = 0;
            }

            if(!(handle & EMBX_HANDLE_VALID_BIT))
            {
                /* Found a free handle */
                break;
            }

            if(pMan->nextIndex == oldIndex)
            {
                EMBX_DebugMessage(("EMBX_HANDLE_CREATE: Search found no free handle, allocated count is wrong\n"));
                return EMBX_INVALID_HANDLE_VALUE;
            }
        }
    }

    /* Note that the handle issue number wraps around due to the application of the issue mask,
     * avoiding having to add extra test/branch code at the expense of some obfuscation.
     */
    handle = (handle & EMBX_HANDLE_INDEX_MASK) |
             (((handle & EMBX_HANDLE_ISSUE_MASK)+EMBX_HANDLE_ISSUE_INC) & EMBX_HANDLE_ISSUE_MASK) |
             (type & EMBX_HANDLE_TYPE_MASK) |
             EMBX_HANDLE_VALID_BIT;

    pMan->table[handle & EMBX_HANDLE_INDEX_MASK].handle = handle;
    pMan->table[handle & EMBX_HANDLE_INDEX_MASK].object = obj;

    pMan->nAllocated++;

    return handle;
}



EMBX_BOOL EMBX_HandleManager_SetSize(EMBX_HandleManager_t *pMan,EMBX_UINT size)
{
    if(size < EMBX_HANDLE_MIN_TABLE_SIZE || size > EMBX_HANDLE_INDEX_MASK)
        return EMBX_FALSE;


    pMan->size = size;
    return EMBX_TRUE;
}



EMBX_ERROR EMBX_HandleManager_Init(EMBX_HandleManager_t *pMan)
{
    if(pMan->table != 0)
        return EMBX_ALREADY_INITIALIZED;


    pMan->table = (EMBX_HandleEntry_t *)EMBX_OS_MemAlloc(pMan->size*sizeof(EMBX_HandleEntry_t));
    if(pMan->table != 0)
    {
    unsigned int i;

        for(i=0;i<pMan->size;i++)
        {
            /* We need to ensure that the handle value in the tables cannot be zero,
             * to prevent the obvious mistake of passing NULL to EMBX_HANDLE_GETOBJ
             * We do this by setting the type field to something other than zero and 
             * presetting the index bits to the correct index for the table slot.
             */
            pMan->table[i].handle = (i | EMBX_HANDLE_INITIAL_VALUE);
            pMan->table[i].object = 0;
        }

        pMan->nextIndex  = 0;
        pMan->nAllocated = 0;
        pMan->lastFreed  = 0;
    
        return EMBX_SUCCESS;
    }

    return EMBX_NOMEM;
}



void EMBX_HandleManager_Destroy(EMBX_HandleManager_t *pMan)
{
    if(pMan->table == 0)
        return;


#if defined(EMBX_VERBOSE)

    if(pMan->nAllocated != 0)
    {
        EMBX_DebugMessage(("EMBX_HandleManager_Destroy: %d open handles still exist\n",
                            pMan->nAllocated));
    }

#endif /* EMBX_VERBOSE */

    EMBX_OS_MemFree(pMan->table);
    pMan->table = 0;
}
