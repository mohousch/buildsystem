/*
 * LookupTable.c
 *
 * Copyright (C) STMicroelectronics Limited 2004. All rights reserved.
 *
 * 
 */

#include "mme_hostP.h"

#if defined __LINUX__ && !defined __KERNEL__
/* Until we have EMBX built in user space */
#include <stdlib.h>
#include <string.h>
#define EMBX_OS_MemAlloc(X)   (malloc(X))
#define EMBX_OS_MemFree(X)    (free(X))
#endif
    
struct MMELookupTable {
	int maxEntries;
	int temporallyUnique;
	int insertAt;
	void *lookup[1];
};

MMELookupTable_t * MME_LookupTable_Create(int max, int temporal)
{
	MMELookupTable_t *tbl;
        int size = sizeof(MMELookupTable_t) + (max-1) * sizeof(void *);

	MME_Assert(max > 0);
	MME_Assert(0 == temporal || 1 == temporal);

	/* allocation */
        tbl = EMBX_OS_MemAlloc(size);
        if (!tbl) {
                return NULL;
        }
        memset(tbl, 0, size);

	tbl->maxEntries = max;
	tbl->temporallyUnique = temporal;

	return tbl;
}

void MME_LookupTable_Delete(MMELookupTable_t *tbl)
{
	MME_Assert(tbl);
	EMBX_OS_MemFree(tbl);
}

MME_ERROR MME_LookupTable_Insert(MMELookupTable_t *tbl, void *data, int *id)
{
	int i;

	MME_Assert(tbl);
	MME_Assert(data);

	i = tbl->insertAt;
	do {
		MME_Assert(i <= tbl->maxEntries);

		if (NULL == tbl->lookup[i]) {
			tbl->lookup[i] = data;
			*id = i;

			i++;
			tbl->insertAt = (i == tbl->maxEntries)?0:i;
			return MME_SUCCESS;
		}
		
		if (++i == tbl->maxEntries) {
			i = 0;
		}
	} while (i != tbl->insertAt);

	return MME_NOMEM;
}

MME_ERROR MME_LookupTable_Remove(MMELookupTable_t *tbl, int id)
{
	MME_Assert(tbl);
	MME_Assert(id < tbl->maxEntries);

	if (id < 0 || id >= tbl->maxEntries) {
		return MME_INVALID_HANDLE;
	}

	tbl->lookup[id] = NULL;

	if (!tbl->temporallyUnique) {
		/* improves locality of reference at bit */
		tbl->insertAt = id;
	}

	return MME_SUCCESS;
}

MME_ERROR MME_LookupTable_Find(MMELookupTable_t *tbl, int id, void **data)
{
	MME_Assert(tbl);
	MME_Assert(data);

	if (id < 0 || id >= tbl->maxEntries) {
		return MME_INVALID_HANDLE;
	}

	*data = tbl->lookup[id];
	return (*data ? MME_SUCCESS : MME_INVALID_HANDLE);
}

int MME_LookupTable_IsEmpty(MMELookupTable_t *tbl)
{
	int i;

	MME_Assert(tbl);

	for (i=0; i<tbl->maxEntries; i++) {
		if (tbl->lookup[i]) {
			return 0;
		}
	}

	return 1;
}

