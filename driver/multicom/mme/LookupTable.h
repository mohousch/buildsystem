/*
 * MMELookupTable.h
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * 
 */

#ifndef MME_MME_LOOKUP_TABLE_H
#define MME_MME_LOOKUP_TABLE_H

typedef struct MMELookupTable MMELookupTable_t;

MMELookupTable_t * MME_LookupTable_Create(int max, int temporal);
void MME_LookupTable_Delete(MMELookupTable_t *tbl);
MME_ERROR MME_LookupTable_Insert(MMELookupTable_t *tbl, void *data, int *id);
MME_ERROR MME_LookupTable_Remove(MMELookupTable_t *tbl, int id);
MME_ERROR MME_LookupTable_Find(MMELookupTable_t *tbl, int id, void **data);
int MME_LookupTable_IsEmpty(MMELookupTable_t *tbl);

#endif /* MME_MME_LOOKUP_TABLE_H */
