/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxP.h                                                   */
/*                                                                 */
/* Description:                                                    */
/*         Private EMBX interfaces for use only by transports.     */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBXP_H
#define _EMBXP_H

#include "embx_typesP.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern EMBX_UINT	  EMBX_GetTuneable     (EMBX_Tuneable_t key);

extern EMBX_VOID          EMBX_EventListAdd    (EMBX_EventList_t **head, EMBX_EventList_t *node);
extern EMBX_VOID          EMBX_EventListRemove (EMBX_EventList_t **head, EMBX_EventList_t *node);
extern EMBX_EventList_t * EMBX_EventListFront  (EMBX_EventList_t **head);
extern EMBX_VOID          EMBX_EventListSignal (EMBX_EventList_t **head, EMBX_ERROR result);


#define EMBX_HANDLE_GETOBJ(m,h)                               \
(((EMBX_UINT)((h) & EMBX_HANDLE_INDEX_MASK) < (m)->size &&    \
  (m)->table[(h) & EMBX_HANDLE_INDEX_MASK].handle == (h)  ) ? \
        (m)->table[(h) & EMBX_HANDLE_INDEX_MASK].object : 0)



#define EMBX_HANDLE_ATTACHOBJ(m,h,pObj)                                    \
do {                                                                       \
  if((EMBX_UINT)((h) & EMBX_HANDLE_INDEX_MASK) < (m)->size &&              \
     (m)->table[(h) & EMBX_HANDLE_INDEX_MASK].handle == (h))               \
  {                                                                        \
    (m)->table[(h) & EMBX_HANDLE_INDEX_MASK].object = (EMBX_VOID *)(pObj); \
  }                                                                        \
} while(0);


#define EMBX_HANDLE_ISTYPEOF(h,t) (((h) & (EMBX_HANDLE_TYPE_MASK | EMBX_HANDLE_VALID_BIT)) == ((t) | EMBX_HANDLE_VALID_BIT))


#define EMBX_HANDLE_FREE(m,h)                                                                     \
do {                                                                                              \
  if((EMBX_UINT)((h) & EMBX_HANDLE_INDEX_MASK) < (m)->size &&                                     \
     (m)->table[(h) & EMBX_HANDLE_INDEX_MASK].handle == (h))                                      \
  {                                                                                               \
    (m)->lastFreed = ((m)->table[(h) & EMBX_HANDLE_INDEX_MASK].handle &= ~EMBX_HANDLE_VALID_BIT); \
    (m)->nAllocated--;                                                                            \
  }                                                                                               \
} while(0);



extern EMBX_HANDLE EMBX_HANDLE_CREATE (EMBX_HandleManager_t *pMan,void *obj, EMBX_UINT type);

extern EMBX_BOOL  EMBX_HandleManager_SetSize (EMBX_HandleManager_t *pMan,EMBX_UINT size);
extern EMBX_ERROR EMBX_HandleManager_Init    (EMBX_HandleManager_t *pMan);
extern void       EMBX_HandleManager_Destroy (EMBX_HandleManager_t *pMan);

/* take (and release) the highly contended global driver lock. this should only
 * ever be called from transport worker threads and even then only when strictly
 * required.
 */
extern void _EMBX_DriverMutexTake(void);
extern void _EMBX_DriverMutexRelease(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EMBXP_H */
