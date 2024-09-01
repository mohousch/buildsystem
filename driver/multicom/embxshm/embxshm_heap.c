/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_heap.c                                      */
/*                                                            */
/* Description:                                               */
/*    EMBX shared memory heap manager                         */
/*                                                            */
/**************************************************************/

#include "embxshmP.h"

static EMBXSHM_Buffer_t *assertCache;

/*
 * Constants describing the nature of the largest data cache line on any participant
 */
#define CACHE_LINE_SIZE        EMBXSHM_MAX_CACHE_LINE
#define CACHE_SINGLE_LINE_ALLOC (CACHE_LINE_SIZE / 2)

/*
 * Alignment management macros
 */
#define ALIGN_UP(uint, align)   ((((uintptr_t) uint - 1) | ((align) - 1)) + 1)
#define ALIGN_DOWN(uint, align) (((uintptr_t) uint) & ~((align) - 1))
#define ALIGN_TEST(uint, align) (0 == ((uintptr_t) uint & ((align) - 1)))

/*
 * Examine a buffer and check it meets the alignment requirements (which
 * are the for large buffers buf->data is cache line aligned and for
 * small buffers buf->data is half cache line aligned)
 */
#define BUFFER_ALIGN_TEST(buf, size) \
        (ALIGN_TEST (&((buf)->data), ((size) <= CACHE_SINGLE_LINE_ALLOC ? CACHE_SINGLE_LINE_ALLOC : CACHE_LINE_SIZE)))

/*
 * Examine a buffer and check it meets the address requirements of a buffer
 * allocated from this heap.
 */
#define BUFFER_ADDRESS_TEST(buf, tpshm) \
	( ((uintptr_t) (buf) >=   (uintptr_t) ((tpshm)->heap)) && \
          ((uintptr_t) (buf) <= ( (uintptr_t) ((tpshm)->heap) + (tpshm)->heapSize)) \
	)

#define FREELIST_LOCK(tpshm, hcb, id) EMBXSHM_takeSpinlock(tpshm, &(hcb->lockPartner[(id)]), hcb->lockCache[(id)])
#define FREELIST_UNLOCK(tpshm, hcb, id) EMBXSHM_releaseSpinlock(tpshm, &(hcb->lockPartner[(id)]), hcb->lockCache[(id)])

/*
 * Allocates and initializes a small block from the break value
 *
 * memory:   2 shared reads and 3 shared writes plus one lock overhead
 * calls:    nothing
 */
static EMBXSHM_Buffer_t *allocateFromSmallBreak (EMBXSHM_Transport_t *tpshm,
                                                 size_t                      size)
{
    EMBXSHM_HeapControlBlock_t *hcb;
    EMBXSHM_Buffer_t *block;
    uintptr_t         smallBreak, largeBreak;

    EMBX_Info(EMBX_INFO_HEAP, (">>>allocateFromSmallBreak(0x%08x, %d)\n", (unsigned) tpshm, size));

    EMBX_Assert (tpshm);
    EMBX_Assert ((0 < size) && (size <= EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE));

    hcb = tpshm->hcb;
    EMBX_Assert (hcb);
    EMBX_Assert (hcb->hcbShared);

    FREELIST_LOCK (tpshm, hcb, FREELIST_BREAK_VALUES);

    EMBXSHM_READS(&hcb->hcbShared->freelist[FREELIST_BREAK_VALUES]);
    
    smallBreak = hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].smallBlockBreak;
    EMBX_Assert (ALIGN_TEST (smallBreak, CACHE_LINE_SIZE));

    largeBreak = hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].largeBlockBreak;
    EMBX_Assert (ALIGN_TEST (largeBreak, CACHE_LINE_SIZE));

    /*
     * There are different alignment contraints on very small blocks (a very small block
     * is one which can fit in a single cache line alongside the buffer header)
     */
    if (size < CACHE_SINGLE_LINE_ALLOC)
    {
        smallBreak -= ALIGN_UP (size, CACHE_SINGLE_LINE_ALLOC);
        block       = BUS_TO_BUFFER (smallBreak - SIZEOF_EMBXSHM_BUFFER, tpshm->pointerWarp);
        smallBreak -= ALIGN_UP (SIZEOF_EMBXSHM_BUFFER, CACHE_SINGLE_LINE_ALLOC);
    }
    else
    {
        smallBreak -= ALIGN_UP (size, CACHE_LINE_SIZE);
        block       = BUS_TO_BUFFER (smallBreak - SIZEOF_EMBXSHM_BUFFER, tpshm->pointerWarp);
        smallBreak -= ALIGN_UP (SIZEOF_EMBXSHM_BUFFER, CACHE_LINE_SIZE);
    }

    /*
     * Check that the block and the break are correctly aligned
     */
    EMBX_Assert (ALIGN_TEST (smallBreak, CACHE_LINE_SIZE));
    EMBX_Assert (BUFFER_ADDRESS_TEST (block, tpshm));
    EMBX_Assert (BUFFER_ALIGN_TEST (block, size));

    /*
     * Populate the block providing we have not run out of memory (and smallBreak
     * has not overflowed).
     */
    if ((uintptr_t) tpshm->heapSize >= smallBreak - largeBreak)
    {
        hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].smallBlockBreak = smallBreak;

        block->next = EMBXSHM_BUFFER_ALLOCATED;
        block->size = size;

	EMBXSHM_WROTE(&hcb->hcbShared->freelist[FREELIST_BREAK_VALUES]);
	EMBXSHM_WROTE(block);
    }
    else
    {
        block = 0;
    }
    
    FREELIST_UNLOCK (tpshm, hcb, FREELIST_BREAK_VALUES);

    EMBX_Info(EMBX_INFO_HEAP, ("<<<allocateFromSmallBreak = 0x%08x\n", (unsigned) block));
    return block;
}

/*
 * Allocates and initializes a large block from the break value
 *
 * memory:   2 shared reads and 1 shared write plus one lock overhead
 * calls:    nothing
 */
static EMBXSHM_Buffer_t *allocateFromLargeBreak (EMBXSHM_Transport_t        *tpshm,
                                                 size_t                      size)
{
    EMBXSHM_HeapControlBlock_t *hcb;
    EMBXSHM_Buffer_t *block;
    uintptr_t         smallBreak, largeBreak;

    EMBX_Info(EMBX_INFO_HEAP, (">>>allocateFromLargeBreak(0x%08x, %d)\n", (unsigned) tpshm, size));

    EMBX_Assert(tpshm);
    EMBX_Assert (EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE < size && size <= 0x80000000);
    EMBX_Assert (ALIGN_TEST (size, CACHE_LINE_SIZE));

    hcb = tpshm->hcb;
    EMBX_Assert (hcb);
    EMBX_Assert (hcb->hcbShared);

    FREELIST_LOCK (tpshm, hcb, FREELIST_BREAK_VALUES);

    EMBXSHM_READS(&hcb->hcbShared->freelist[FREELIST_BREAK_VALUES]);

    smallBreak = hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].smallBlockBreak;
    EMBX_Assert (ALIGN_TEST (smallBreak, CACHE_LINE_SIZE));

    largeBreak = hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].largeBlockBreak;
    EMBX_Assert (ALIGN_TEST (largeBreak, CACHE_LINE_SIZE));

    /*
     * Alter the break value and perform the allocation
     */
    largeBreak += ALIGN_UP (SIZEOF_EMBXSHM_BUFFER, CACHE_LINE_SIZE);
    block       = BUS_TO_BUFFER (largeBreak - SIZEOF_EMBXSHM_BUFFER, tpshm->pointerWarp);
    largeBreak += size;

    /*
     * Check that the block and the break are correctly aligned
     */
    EMBX_Assert (ALIGN_TEST (smallBreak, CACHE_LINE_SIZE));
    EMBX_Assert (BUFFER_ADDRESS_TEST (block, tpshm));
    EMBX_Assert (BUFFER_ALIGN_TEST (block, size));

    /*
     * Populate the block providing we have not run out of memory (and largeBreak
     * has not overflowed).
     */
    if ((uintptr_t) tpshm->heapSize >= smallBreak - largeBreak)
    {
        hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].largeBlockBreak = largeBreak;

        block->next = EMBXSHM_BUFFER_ALLOCATED;
        block->size = size;

	EMBXSHM_WROTE(&hcb->hcbShared->freelist[FREELIST_BREAK_VALUES]);
	EMBXSHM_WROTE(block);
    }
    else
    {
        block = 0;
    }

    FREELIST_UNLOCK (tpshm, hcb, FREELIST_BREAK_VALUES);

    EMBX_Info(EMBX_INFO_HEAP, ("<<<allocateFromLargeBreak = 0x%08x\n", (unsigned) block));
    return block;
}

/*
 * Allocates a small block from the supplied list
 */
static EMBXSHM_Buffer_t * allocateSmallBlock (EMBXSHM_Transport_t        *tpshm,
                                              EMBXSHM_BufferList_t      *list,
                                              int                         listId,
                                              size_t                      size)
{
    EMBXSHM_HeapControlBlock_t *hcb;
    EMBXSHM_Buffer_t *head;

    EMBX_Info(EMBX_INFO_HEAP, (">>>allocateSmallBlock(0x%08x, 0x%08x, %d, %d)\n",
                               (unsigned) tpshm, (unsigned) list, listId, size));

    EMBX_Assert (tpshm);
    EMBX_Assert (list && EMBXSHM_ALIGNED_TO_CACHE_LINE(list));
    EMBX_Assert ((0 < size) && (size <= EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE));

    hcb = tpshm->hcb;
    EMBX_Assert (hcb);
    EMBX_Assert (hcb->hcbShared);

    FREELIST_LOCK (tpshm, hcb, listId);

    EMBXSHM_READS(list);

    head = list->head;

    if (0 == (void *) head)
    {
	FREELIST_UNLOCK (tpshm, hcb, listId);
	EMBX_Info(EMBX_INFO_HEAP, ("<<<allocateSmallBlock = allocateFromSmallBreak()\n"));
        return allocateFromSmallBreak (tpshm, size);
    }

    EMBX_Assert (BUFFER_ALIGN_TEST (head, size));
    head = BUS_TO_BUFFER (head, tpshm->pointerWarp);
    EMBX_Assert (BUFFER_ADDRESS_TEST (head, tpshm));

    EMBXSHM_READS(head);
    EMBX_Assert (head->size == size);
    EMBX_Assert (0 == head->next || BUFFER_ALIGN_TEST (head->next, size));
    list->head = head->next;
    head->next = EMBXSHM_BUFFER_ALLOCATED;


    EMBXSHM_WROTE(list);
    EMBXSHM_WROTE(head);

    FREELIST_UNLOCK (tpshm, hcb, listId);

    EMBX_Info(EMBX_INFO_HEAP, ("<<<allocateSmallBlock = 0x%08x\n", (unsigned) head));
    return head;
}

/*
 * Deallocates a small block and places it back on the supplied list
 */
static void deallocateSmallBlock (EMBXSHM_Transport_t        *tpshm,
                                  EMBXSHM_BufferList_t       *list,
                                  int                         listId,
                                  EMBXSHM_Buffer_t           *block,
                                  size_t                      size)
{
    EMBXSHM_HeapControlBlock_t *hcb;

    EMBX_Info(EMBX_INFO_HEAP, (">>>deallocateSmallBlock(0x%08x, 0x%08x, %d, 0x%08x, %d)\n",
                               (unsigned) tpshm, (unsigned) list, listId, (unsigned) block, size));

    EMBX_Assert (tpshm);
    EMBX_Assert (list && EMBXSHM_ALIGNED_TO_CACHE_LINE(list));
    EMBX_Assert (block);
    EMBX_Assert ((0 < size) && size <= EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE);
    EMBX_Assert (block->size == size);
    EMBX_Assert (block->next == EMBXSHM_BUFFER_ALLOCATED);
    EMBX_Assert (BUFFER_ADDRESS_TEST (block, tpshm));
    EMBX_Assert (BUFFER_ALIGN_TEST (block, size));

    hcb = tpshm->hcb;
    EMBX_Assert(hcb);
    EMBX_Assert(hcb->hcbShared);

    FREELIST_LOCK (tpshm, hcb, listId);

    EMBXSHM_READS(block);
    EMBXSHM_READS(list);

    EMBX_Assert(0 == list->head || BUFFER_ALIGN_TEST (list->head, size));
    block->next = list->head;
    list->head  = BUFFER_TO_BUS (block, tpshm->pointerWarp);

    EMBXSHM_WROTE(block);
    EMBXSHM_WROTE(list);

    FREELIST_UNLOCK (tpshm, hcb, listId);

    EMBX_Info(EMBX_INFO_HEAP, ("<<<deallocateSmallBlock\n"));
}

/*
 * Prepare a block before it is returned to the user. this might involve spliting
 * the block or simple removing it from the free list
 *
 * memory:   TBD
 * calls:    
 * oddities: we take currSize as an argument (dispite the fact we could obtain it
 *           be dereferencing curr) since our callers will already have copied this
 *           value from shared memory to local memory
 */
static EMBXSHM_Buffer_t *prepareLargeBlock ( size_t             size, 
                                             EMBXSHM_Buffer_t **link, 
                                             EMBXSHM_Buffer_t  *curr, 
                                             size_t             currSize)
{
    EMBXSHM_Buffer_t *buf;

    EMBX_Info(EMBX_INFO_HEAP, (">>>prepareLargeBlock()\n"));

    /*
     * Assert our pre-conditions
     */
    EMBX_Assert (size > EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE);
    EMBX_Assert (ALIGN_TEST (size, CACHE_LINE_SIZE));
    EMBX_Assert (curr);
    EMBX_Assert (BUFFER_ALIGN_TEST (curr, size));
    EMBX_Assert (ALIGN_TEST (currSize, CACHE_LINE_SIZE));

    EMBXSHM_READS(curr);
    EMBX_Assert(curr->size == currSize);
    EMBX_Assert(curr->next != EMBXSHM_BUFFER_ALLOCATED);

    /*
     * TODO: we may want to split blocks earlier since although they cannot
     * be allocated then can be coalesced with the blocks on the other side
     */
    if (currSize < (size + EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE + ALIGN_UP (SIZEOF_EMBXSHM_BUFFER, CACHE_LINE_SIZE)))
    {
        /*
         * This block cannot be split so we must remove it from the list
         */
        *link = curr->next;
        buf   = curr;

        /*
         * Do *not* update size or we'll leak memory
         */

	EMBXSHM_WROTE(link);
    }
    else
    {
        /*
         * Allocate some space from the top of this block (we allocate from
         * the top so we do not have to remove the block from the free list)
         */
        currSize -= size;

        /*
         * We do not need to compensate using SIZEOF_EMBXSHM_BUFFER here since the
         * value of curr effectively does that for us
         */
        buf = (EMBXSHM_Buffer_t *) ((uintptr_t) curr + currSize);

        currSize -= ALIGN_UP (SIZEOF_EMBXSHM_BUFFER, CACHE_LINE_SIZE);

        EMBX_Assert (BUFFER_ALIGN_TEST (buf, size));
        EMBX_Assert (ALIGN_TEST (currSize, CACHE_LINE_SIZE));
        EMBX_Assert ((((uintptr_t) &(curr->data)) + currSize) == ALIGN_DOWN (buf, CACHE_LINE_SIZE));

        buf->size = size;    
        curr->size = currSize;

	EMBXSHM_WROTE(&curr->size);
    }

    /*
     * Mark the buffer as allocated
     */
    buf->next = EMBXSHM_BUFFER_ALLOCATED;

    EMBXSHM_WROTE(buf);

    EMBX_Info(EMBX_INFO_HEAP, ("<<<prepareLargeBlock = 0x%08x\n", (unsigned) buf));
    return buf;
}

/* Allocates a large block from the supplied list using best-fit to
 * minimise fragmentation
 *
 * TODO: we could perhaps solve the readers/writers problem with less
 * contention by scanning the list without locking out other readers.
 * Doing so could be very hard!
 *
 * memory:   unbounded number of accesses to shared memory
 * calls:    allocateFromLargeBreak
 */
static EMBXSHM_Buffer_t *allocateLargeBlock (EMBXSHM_Transport_t        *tpshm,
                                             EMBXSHM_BufferList_t       *list,
                                             int                         listId,
                                             size_t                      size)
{
    EMBXSHM_HeapControlBlock_t *hcb;
    EMBXSHM_Buffer_t  *buf, *curr, *head;
    EMBXSHM_Buffer_t **bestLink, **link;
    size_t             bestSize; /* local memory cache of best size */

    EMBX_Info(EMBX_INFO_HEAP, (">>>allocateLargeBlock(0x%08x, 0x%08x, %d, %d)\n",
                               (unsigned) tpshm, (unsigned) list, listId, size));

    EMBX_Assert (tpshm);
    EMBX_Assert (list && EMBXSHM_ALIGNED_TO_CACHE_LINE(list));
    EMBX_Assert (size > EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE);
    EMBX_Assert (ALIGN_TEST (size, CACHE_LINE_SIZE));

    hcb = tpshm->hcb;
    EMBX_Assert (hcb);
    EMBX_Assert (hcb->hcbShared);

    FREELIST_LOCK (tpshm, hcb, listId);

    EMBXSHM_READS(list);

    head = list->head;

    if (head == 0)
    {
	FREELIST_UNLOCK (tpshm, hcb, listId);
	EMBX_Info(EMBX_INFO_HEAP, ("<<<allocateLargeBlock = allocateFromLargeBreak()\n"));
        return allocateFromLargeBreak (tpshm, size);
    }

    EMBX_Assert (BUFFER_ALIGN_TEST (head, size));

    /*
     * Examine every block in the freelist looking for the best fit
     */
    bestLink = 0;
    bestSize = INT_MAX;
    for (link = &(list->head), curr = BUS_TO_BUFFER (list->head, tpshm->pointerWarp);
         curr != BUS_TO_BUFFER (0, tpshm->pointerWarp);
         link = &(curr->next), curr = BUS_TO_BUFFER (curr->next, tpshm->pointerWarp))
    {
        size_t currSize;

	EMBX_Assert (BUFFER_ADDRESS_TEST (curr, tpshm));
        EMBX_Assert (BUFFER_ALIGN_TEST (curr, size));

	EMBXSHM_READS(curr);

        currSize = curr->size;
        
        if ((size <= currSize) && (currSize < bestSize))
        {
            /*
             * We have a new best friend^H^H^H^H^H^H block
             */
            bestLink = link;
            bestSize = currSize;

            /*
             * If we have the perfect match we might as well abort
             * abort the search
             */
            if (currSize == size)
            {
                break;
            }
        }
    }

    /*
     * Did we find a block suitable for allocation - if not we can
     * allocate from the large break value
     */
    if (0 == (void *) bestLink)
    {
        FREELIST_UNLOCK (tpshm, hcb, listId);
        return allocateFromLargeBreak (tpshm, size);
    }

    /*
     * If we reach this point we must have a block to allocate from
     * TODO: this is a needless shared memory access!
     */
    buf = prepareLargeBlock (size, bestLink, BUS_TO_BUFFER (*bestLink, tpshm->pointerWarp), bestSize);

    FREELIST_UNLOCK (tpshm, hcb, listId);

    EMBX_Info(EMBX_INFO_HEAP, ("<<<allocateLargeBlock = 0x%08x\n", (unsigned) buf));
    return buf;
}

/*
 * Compare the current and deallocated block to determine if we can coalesce
 *
 * returns:   the number of blocks coalesced (an integer between 0 and 1)
 */
static int coalesceLargeBlock (EMBXSHM_Buffer_t **link,
                               EMBXSHM_Buffer_t  *curr,
                               EMBXSHM_Buffer_t **pBuf,
                               size_t            *pBufSize)
{
    size_t    currSize;
    uintptr_t currBase, currEnd, bufBase, bufEnd;

    EMBX_Info(EMBX_INFO_HEAP, (">>>coalesceLargeBlock()\n"));

    EMBX_Assert (link);
    EMBX_Assert (curr);
    EMBX_Assert (pBuf && *pBuf);
    EMBX_Assert (BUFFER_ALIGN_TEST (*pBuf, *pBufSize));
    EMBX_Assert (pBufSize && *pBufSize > EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE);
    EMBX_Assert (ALIGN_TEST (*pBufSize, CACHE_LINE_SIZE));

    EMBXSHM_READS(curr);
    EMBXSHM_READS(*pBuf);

    currSize = curr->size;
    EMBX_Assert (ALIGN_TEST (currSize, CACHE_LINE_SIZE));
    EMBX_Assert (currSize >= EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE);

    currBase = (uintptr_t) ALIGN_DOWN (curr, CACHE_LINE_SIZE);
    currEnd  = ((uintptr_t) &(curr->data)) + currSize;

    bufBase  = (uintptr_t) ALIGN_DOWN (*pBuf, CACHE_LINE_SIZE);
    bufEnd   = ((uintptr_t) &((*pBuf)->data)) + *pBufSize;

    /*
     * Check that our blocks are not nested (detects some forms of illegal
     * free)
     */
    EMBX_Assert (( (currBase < bufBase) && (currEnd <= bufBase) ) ||
                 ( (bufBase < currBase) && (bufEnd <= currBase) ) );

    /*
     * If these blocks can be coaleced then remove the current element from the list
     * and increase the size of buffer in the appropriate way
     */
    if ((bufBase == currEnd) || (bufEnd == currBase))
    {
        /*
         * Remove curr from the list
         */
        *link = curr->next;

        /*
         * If we coalesced downwards then alter the buffer pointer
         */
        if (bufBase == currEnd)
        {
            *pBuf = curr;
        }

        /*
         * Calculate the new buffer size (and update the buffer)
         */
        *pBufSize += currSize + ALIGN_UP (SIZEOF_EMBXSHM_BUFFER, CACHE_LINE_SIZE);
        (*pBuf)->size = *pBufSize;

        EMBX_Assert (BUFFER_ALIGN_TEST (*pBuf, *pBufSize));
        EMBX_Assert (*pBufSize > EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE);
        EMBX_Assert (ALIGN_TEST (*pBufSize, CACHE_LINE_SIZE));

	EMBXSHM_WROTE(*pBuf);
	EMBXSHM_WROTE(link);

        /*
         * Record the fact we merged
         */
	EMBX_Info(EMBX_INFO_HEAP, ("<<<coalesceLargeBlock = 1\n"));
        return 1;
    }

    EMBX_Info(EMBX_INFO_HEAP, ("<<<coalesceLargeBlock = 0\n"));
    return 0;
}

/*
 * Deallocates a small block and places it back on the supplied list
 */
static void deallocateLargeBlock (EMBXSHM_Transport_t        *tpshm,
                                  EMBXSHM_BufferList_t       *list,
                                  int                         listId,
                                  EMBXSHM_Buffer_t           *buf,
                                  size_t                      size)
{
    EMBXSHM_HeapControlBlock_t *hcb;
    int               coalesceCount;
    EMBXSHM_Buffer_t *curr, **link;

    EMBX_Info(EMBX_INFO_HEAP, (">>>deallocateLargeBlock(0x%08x, 0x%08x, %d, 0x%08x, %d)\n",
                               (unsigned) tpshm, (unsigned) list, listId, (unsigned) buf, size));

    EMBX_Assert (tpshm);
    EMBX_Assert (list && EMBXSHM_ALIGNED_TO_CACHE_LINE(list));
    EMBX_Assert (buf);
    EMBX_Assert (BUFFER_ADDRESS_TEST (buf, tpshm));
    EMBX_Assert (BUFFER_ALIGN_TEST (buf, size));
    EMBX_Assert (size > EMBXSHM_MINIMUM_LARGE_BLOCK_SIZE);
    EMBX_Assert (ALIGN_TEST (size, CACHE_LINE_SIZE));

    hcb = tpshm->hcb;
    EMBX_Assert (hcb);
    EMBX_Assert (hcb->hcbShared);

    FREELIST_LOCK (tpshm, hcb, listId);

    EMBXSHM_READS(list);
    EMBXSHM_READS(buf);

    /*
     * Try to coalesce with every block on the free list (we abort the search
     * once we have coalesced with two blocks because we cannot possible
     * coalesce with more blocks
     */

    coalesceCount = 0;
    link = &(list->head);
    curr = BUS_TO_BUFFER (list->head, tpshm->pointerWarp);
    
    while ((curr != BUS_TO_BUFFER (0, tpshm->pointerWarp)) && (coalesceCount < 2))
    {
	EMBX_Assert (BUFFER_ADDRESS_TEST (curr, tpshm));
        EMBX_Assert (BUFFER_ALIGN_TEST (curr, size));

        if (0 != coalesceLargeBlock (link, curr, &buf, &size))
        {
            coalesceCount++;
        }
        else
        {
            link = &(curr->next);
	    EMBX_Assert (0 == *link || BUFFER_ALIGN_TEST (*link, curr->size));
	    EMBXSHM_WROTE(link);
        }

        curr = BUS_TO_BUFFER (*link, tpshm->pointerWarp);
        
    }

    /*
     * We can now simple place the block at the front of the free list
     */
    buf->next  = list->head;
    list->head = BUFFER_TO_BUS (buf, tpshm->pointerWarp);

    EMBXSHM_WROTE(buf);
    EMBXSHM_WROTE(list);

    FREELIST_UNLOCK (tpshm, hcb, listId);

    EMBX_Info(EMBX_INFO_HEAP, ("<<<deallocateLargeBlock\n"));
}

/*
 * Lookup the appropriate freelist and lock for a given block size.
 *
 * Returns true if the returned freelist is *not* of fixed size.
 */
static int size2list (EMBXSHM_Transport_t              *tpshm,
                      EMBXSHM_BufferList_t            **pList,
                      int                              *pListId,
                      size_t                           *pSize)
{
    EMBXSHM_HeapControlBlockShared_t *hcbShared;
    int id;
    size_t size;

    EMBX_Info(EMBX_INFO_HEAP, (">>>size2list(0x%08x, ...)\n", (unsigned) tpshm));
    
    EMBX_Assert (tpshm);
    EMBX_Assert (tpshm->hcb);
    EMBX_Assert (pList);
    EMBX_Assert (pListId);
    EMBX_Assert (pSize);

    /* this functions makes no access to the shared memory although it does
     * generate pointers to portions of it.
     */

    hcbShared = tpshm->hcb->hcbShared;
    EMBX_Assert (hcbShared && EMBXSHM_ALIGNED_TO_CACHE_LINE(hcbShared));

    size = *pSize;

    if (size > 512)
    {
        *pList   = &(hcbShared->freelist[FREELIST_LARGE_BLOCKS]);
        *pListId = FREELIST_LARGE_BLOCKS;
        *pSize   = ALIGN_UP (size, CACHE_LINE_SIZE);
	EMBX_Info(EMBX_INFO_HEAP, ("<<<size2list(..., 0x%08x, %d, %d) = 1\n",
	                           (unsigned) *pList, *pListId, *pSize));
        return 1;
    }

    /*
     * For our switch table to work correctly we must align size to an
     * 8 byte boundary
     */
    size = ALIGN_UP (size, 8);

    switch (size)
    {
    case 8:
        id = FREELIST_8_BYTE;
        break;

    case 16:
        id = FREELIST_16_BYTE;
        break;

    case 24:
        id = FREELIST_24_BYTE;
        break;

    case 32:
        id = FREELIST_32_BYTE;
        break;

    default:
        if (size <= 128)
        {
            if (size <= 64)
            {
                id = FREELIST_64_BYTE;
                size = 64;
            }
            else
            {
                id = FREELIST_128_BYTE;
                size = 128;
            }
        }
        else
        {
            if (size <= 256)
            {
                id = FREELIST_256_BYTE;
                size = 256;
            }
            else
            {
                id = FREELIST_512_BYTE;
                size = 512;
            }
        }
        break;
    }

    *pList   = &(hcbShared->freelist[id]);
    *pListId = id;
    *pSize   = size;

    EMBX_Info(EMBX_INFO_HEAP, ("<<<size2list(..., 0x%08x, %d, %d) = 0\n",
			       (unsigned) *pList, *pListId, *pSize));
    return 0;
}

/*
 * Entry point to EMBXSHM memory allocator
 */
EMBX_ERROR EMBXSHM_allocateMemory (EMBX_Transport_t           *tp,
                                   EMBX_UINT                   size,
				   EMBX_Buffer_t             **pBuf)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *) tp;
    EMBXSHM_BufferList_t  *list;
    EMBXSHM_Buffer_t      *buf;
    int                    freelistId;
    int                    mode;

    EMBX_Info(EMBX_INFO_HEAP, (">>>EMBXSHM_allocateMemory(0x%08x, %d)\n", (unsigned) tpshm, size));

    EMBX_Assert(tpshm);

    /* automatically fail for every large allocations since we are likely
     * to experience problems if the sign bit is set
     */
#if !defined (EMBX_LEAN_AND_MEAN)
    if ((unsigned long) size >= 0x80000000)
    {
	EMBX_Info(EMBX_INFO_HEAP, ("<<<EMBXSHM_allocateMemory = EMBX_NOMEM\n"));
	return EMBX_NOMEM;
    }
#endif

    /*
     * size2list will return true if we must allocate from the
     * large block pool, additionally it will alter each of its
     * arguments:
     *   list will be the EMBXSHM_Buffer_list_t to allocate from
     *   freelistId will be the list identifier (for locking)
     *   size will be altered to meet alignment and other 
     *     requirements
     */
    mode = size2list (tpshm, &list, &freelistId, &size);

    /*
     * Allocate the memory using the appropriate mechanism
     */
    buf = (mode ?
           allocateLargeBlock (tpshm, list, freelistId, size) :
           allocateSmallBlock (tpshm, list, freelistId, size));

    if (0 == buf)
    {
	EMBX_Info(EMBX_INFO_HEAP, ("<<<EMBXSHM_allocateMemory = EMBX_NOMEM\n"));
	return EMBX_NOMEM;
    }

    /*
     * Check that our memory meets the alignment and status requirements
     */
    EMBX_Assert (EMBXSHM_BUFFER_ALLOCATED == (assertCache = buf->next));
    EMBX_Assert (BUFFER_ADDRESS_TEST (buf, tpshm));
    EMBX_Assert (BUFFER_ALIGN_TEST (buf, size));

    EMBX_Info(EMBX_INFO_HEAP, ("<<<EMBXSHM_allocateMemory(..., 0x%08x) = EMBX_SUCCESS\n",
                               (unsigned) buf));
    *pBuf = (EMBX_Buffer_t *) buf;
    return EMBX_SUCCESS;
}

/*
 * Entry point to EMBXSHM memory allocator
 */
EMBX_ERROR EMBXSHM_freeMemory (EMBX_Transport_t        *tp,
                               EMBX_Buffer_t           *ptr)
{
    EMBXSHM_Transport_t *tpshm = (EMBXSHM_Transport_t *) tp;
    EMBXSHM_Buffer_t      *buf = (EMBXSHM_Buffer_t *) ptr; /* re-type ptr */
    size_t                 size;
    EMBXSHM_BufferList_t  *list;
    int                    freelistId;

    EMBX_Info(EMBX_INFO_HEAP, (">>>EMBXSHM_freeMemory(0x%08x, 0x%08x)\n", (unsigned) tpshm, (unsigned) ptr));

    EMBX_Assert (tpshm);
    EMBX_Assert (buf);
    EMBX_Assert (BUFFER_ADDRESS_TEST (buf, tpshm));
    EMBX_Assert (EMBXSHM_BUFFER_ALLOCATED == buf->next);

    EMBXSHM_WROTE(buf);

    size = buf->size;
    EMBX_Assert (BUFFER_ALIGN_TEST (buf, size));
    
    /* once memory is returned to the pool if must not be dirty in the current
     * CPU's cache since the memory is now owned by both processors.
     *
     * TODO: each CPU needs a private (reclaimable) pool to avoid this.
     */
    EMBXSHM_WROTE_ARRAY((char *) &buf->data, size);

    if (size2list (tpshm, &list, &freelistId, &size))
    {
        deallocateLargeBlock (tpshm, list, freelistId, buf, size);
    }
    else
    {
        deallocateSmallBlock (tpshm, list, freelistId, buf, size);
    }

    EMBX_Info(EMBX_INFO_HEAP, ("<<<EMBXSHM_freeMemory = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}

EMBX_ERROR EMBXSHM_memoryInit (EMBXSHM_Transport_t *tpshm,
						EMBX_VOID *base,
                                                EMBX_UINT  size)
{
    int                         i;
    EMBXSHM_Spinlock_t         *bootLock;
    EMBXSHM_HeapControlBlock_t *hcb;

    EMBX_Info(EMBX_INFO_INIT, (">>>EMBXSHM_memoryInit(0x%08x, 0x%08x, %d)\n", (unsigned) tpshm, (unsigned) base, size));

    EMBX_Assert (tpshm);
    EMBX_Assert (base);

    /*
     * Check that EMBX_Buffer_t and EMBXSHM_Buffer_t are equivalent.
     *
     * Each of these asserts tests a constant expression and thus can be
     * determined at compile time. This causes warnings about useless
     * statements. For that reason they are all rolled into one expression
     * with a previous assertion restated.
     */
    EMBX_Assert (tpshm &&
		 sizeof  (EMBX_Buffer_t)         == sizeof  (EMBXSHM_Buffer_t) &&
		 offsetof (EMBX_Buffer_t, size)  == offsetof (EMBXSHM_Buffer_t, size) &&
		 offsetof (EMBX_Buffer_t, htp)   == offsetof (EMBXSHM_Buffer_t, htp) &&
		 offsetof (EMBX_Buffer_t, state) == offsetof (EMBXSHM_Buffer_t, next) &&
		 offsetof (EMBX_Buffer_t, data)  == offsetof (EMBXSHM_Buffer_t, data));

    hcb = (EMBXSHM_HeapControlBlock_t *)EMBX_OS_MemAlloc (sizeof (EMBXSHM_HeapControlBlock_t));
    if (NULL == hcb)
    {
        EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_memoryInit = EMBX_NOMEM\n")); 
        return EMBX_NOMEM;
    }

    /*
     * we must record the heap control block in the transport structure now otherwise our
     * later calls to the allocation functions will fail
     */
    tpshm->hcb = hcb;

    /*
     * Our heap control block is the first item held in 'shared' memory
     */
    hcb->hcbShared   = (EMBXSHM_HeapControlBlockShared_t *)base;
    EMBX_Assert(EMBXSHM_ALIGNED_TO_CACHE_LINE(hcb->hcbShared));

    /*
     * Initialize the local mutex 'partners'
     */
    for (i=0; i<FREELIST_MAX_INDEX; i++)
    {
        (void)EMBX_OS_MUTEX_INIT (&(hcb->lockPartner[i]));
    }

    /*
     * A size of zero means that we do not need to initialize our memory pool
     */
    if (0 != size)
    {
        uintptr_t smallBreak, largeBreak;

        /*
         * Ensure the heap control block is zeroed
         */
        memset (hcb->hcbShared, 0, sizeof (EMBXSHM_HeapControlBlockShared_t));

        /*
         * Calculate the break values
         */
        smallBreak = ALIGN_DOWN (((uintptr_t) hcb->hcbShared) + size, CACHE_LINE_SIZE);
        largeBreak = ALIGN_UP  ((uintptr_t) (hcb->hcbShared + 1),    CACHE_LINE_SIZE);

        /*
         * Store the calculated values in shared memory
         */
        hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].smallBlockBreak = 
			(uintptr_t) LOCAL_TO_BUS (smallBreak, tpshm->pointerWarp);
        hcb->hcbShared->freelist[FREELIST_BREAK_VALUES].largeBlockBreak = 
			(uintptr_t) LOCAL_TO_BUS (largeBreak, tpshm->pointerWarp);

        bootLock = (EMBXSHM_Spinlock_t*)largeBreak;
        memset (bootLock, 0, sizeof (EMBXSHM_Spinlock_t));

        /*
         * Set every heap lock to the bootstrap lock
         */
        for (i=0; i<FREELIST_MAX_INDEX; i++)
        {
            hcb->lockCache[i] = bootLock;
        }

        /*
         * It is now safe to allocate the spinlocks for real
         */
        for (i=0; i<FREELIST_MAX_INDEX; i++)
        {
            hcb->lockCache[i]       = EMBXSHM_createSpinlock (tpshm);
	    hcb->hcbShared->freelist[i].lock = EMBXSHM_serializeSpinlock (tpshm, hcb->lockCache[i]);
        }

	EMBXSHM_READS(bootLock); /* otherwise the boot lock will be dirty in the cache */
	EMBXSHM_WROTE(hcb->hcbShared);
    }
    else
    {
	EMBXSHM_READS(hcb->hcbShared);

        /*
         * Populate the local lock cache from the shared memory
         */
        for (i=0; i<FREELIST_MAX_INDEX; i++)
        {
	    hcb->lockCache[i] = EMBXSHM_deserializeSpinlock (tpshm, 
							     hcb->hcbShared->freelist[i].lock);
        }
    }

    assertCache = 0; /* warning suppression - does nothing useful */

    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBXSHM_memoryInit = EMBX_SUCCESS\n")); 
    return EMBX_SUCCESS;
}

/*
 * Routine to shutdown the heap manager.
 * We just free the control block.
 */
void EMBXSHM_memoryDeinit (EMBXSHM_Transport_t *tpshm)
{
    EMBX_Info(EMBX_INFO_INIT, (">>>EMBX_memoryDeinit(0x%08x)\n", (unsigned) tpshm));
    EMBX_Assert (tpshm);

    if (tpshm->hcb)
    {
        int i;
        
        /*
         * Destroy the local mutex 'partners'
         */
        for (i=0; i<FREELIST_MAX_INDEX; i++)
        {
            EMBX_OS_MUTEX_DESTROY (&(tpshm->hcb->lockPartner[i]));
        }

        EMBX_OS_MemFree (tpshm->hcb);
        tpshm->hcb = 0;
    }

    EMBX_Info(EMBX_INFO_INIT, ("<<<EMBX_memoryDeinit\n"));
}

/*
 * Veneer routine to provide a malloc like allocator for
 * shared memory
 */
void *EMBXSHM_malloc (EMBXSHM_Transport_t        *tpshm,
                      size_t                      size)
{
    EMBX_Buffer_t *buf;
    
    if (EMBX_SUCCESS != EMBXSHM_allocateMemory ((EMBX_Transport_t *) tpshm, size, &buf))
    {
	return 0;
    }
	
    return (void *) (buf->data);
}

/*
 * Veneer routine to provide a free like routine to free
 * off shared memory
 */
void EMBXSHM_free (EMBXSHM_Transport_t        *tpshm,
                   void                       *ptr)
{
    EMBX_Buffer_t *buf = (EMBX_Buffer_t *) (((size_t)ptr) - SIZEOF_EMBXSHM_BUFFER);
    EMBX_ERROR err;

    EMBX_Assert (ptr);
    EMBX_Assert (buf);

    err = EMBXSHM_freeMemory ((EMBX_Transport_t *) tpshm, buf);
    EMBX_Assert(EMBX_SUCCESS == err);
}
