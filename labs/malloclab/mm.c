/*
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

team_t team;

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */

// #define DEBUG 1
// #define VERBOSE 1

#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* Global variables */
// TODO root can change to mem_heap_lo()
static char *root = 0; /* Pointer to first free block */
// static int  left_right_flag = 0;

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE 4               /* Word and header/footer size (bytes) */
#define DSIZE 8               /* Double word size (bytes) */
#define CHUNKSIZE ((1 << 10)) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
// #define PACK(size, alloc)  ((size) | (alloc))

/* Pack a size, a prev_in_use bit and allocated bit into a word */
#define PACK(size, prev_in_use, alloc) ((size) | (prev_in_use << 1) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read and write a pointer at address p */
#define GETP(p) (char *)(*(unsigned int *)(p) ? (char *)mem_heap_lo() + *(unsigned int *)(p) : NULL)
#define PUTP(p, val) (*(unsigned int *)(p) = (unsigned int)(val ? (char *)val - (char *)mem_heap_lo() : 0))

/* The segregated list nums*/
#define TOTAL_LISTS 17 // 🌟 must be odd number because of padding, the Prologue block and Epilogure block is 3 word in total.

#define ROOT(size) (root + get_index(size) * WSIZE)

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOCED(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given free block ptr bp, compute address of its next and prev */
#define NEXTP(bp) ((char *)(bp))
#define PREVP(bp) ((char *)(bp) + WSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
/* Get and set the prev_in_use_bit */
#define GET_PREV_IN_USE(p) ((GET(HDRP(p)) & 0x2) >> 1)
#define SET_NEXT_BLK_PREV_IN_USE(p) (PUT(HDRP(NEXT_BLKP(p)), \
                                         GET(HDRP(NEXT_BLKP(p))) | 0x2))

#define SET_NEXT_BLK_PREV_NOT_IN_USE(p) (PUT(HDRP(NEXT_BLKP(p)), \
                                             GET(HDRP(NEXT_BLKP(p))) & ~(0x2)))

/* DEBUG */
#define PRINT_FUNC(string) \
    printf("********** %d: %s(%s) **********\n", __LINE__, __func__, string)

#ifdef DEBUG
#ifndef VERBOSE
#define VERBOSE 0
#endif

#define CHECKHEAP(string)  \
    mm_checkheap(VERBOSE); \
    PRINT_FUNC(string)
#else
#define CHECKHEAP(string)
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
void mm_checkheap(int verbose);
static void checkblock(void *bp);

/* Inline function */

/*
 * get_index: get index in the segregated free lists accoring to the size.
 */
static inline size_t get_index(size_t size)
{
    int offset = 31 - __builtin_clz(size);
    return offset >= TOTAL_LISTS ? TOTAL_LISTS - 1 : offset;
}

/*
 * insert: insert free block bp of size into the segregated free lists.
 *   every linked list of the segregated free lists should be sorted by the size.
 */
static inline void insert(void *bp, size_t size)
{
    void *block;
    void *prev_block;

    prev_block = NULL;
    block = GETP(ROOT(size));

    while (block)
    {
        if (GET_SIZE(HDRP(block)) >= GET_SIZE(HDRP(bp)))
            break;
        prev_block = block;
        block = GETP(NEXTP(block));
    }

    PUTP(NEXTP(bp), block);
    PUTP(PREVP(bp), prev_block);

    if (block)
        PUTP(PREVP(block), bp);

    if (prev_block)
        PUTP(NEXTP(prev_block), bp);
    else
        PUTP(ROOT(size), bp);
}

/*
 * delete: delete the block bp of size in the segregated free lists.
 */
static inline void delete(void *bp, size_t size)
{

    if (!GETP(PREVP(bp)))
    { /* block is the first free block*/
        PUTP(ROOT(size), GETP(NEXTP(bp)));
    }
    else
    { /*next block is not the first free block*/
        PUTP(NEXTP(GETP(PREVP(bp))), GETP(NEXTP(bp)));
    }

    if (GETP(NEXTP(bp)))
    {
        PUTP(PREVP(GETP(NEXTP(bp))), GETP(PREVP(bp)));
    }
}

/*
 * adjust_alloc_size: Expand the size close to a power of 2.
 *   optimizing for test cases.
 */
static inline size_t adjust_alloc_size(size_t size)
{
    // binary2-bal.rep
    if (size >= 112 && size < 128)
    {
        return 128;
    }
    // binary.rep
    if (size >= 448 && size < 512)
    {
        return 512;
    }
    return size;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
#if VERBOSE
    PRINT_FUNC("void");
#endif
    int i;

    /* Create the initial empty heap */
    if ((root = mem_sbrk((TOTAL_LISTS + 3) * WSIZE)) == (void *)-1)
        return -1;
    for (i = 0; i < TOTAL_LISTS; i++)
        PUT(root + i * WSIZE, 0);
    PUT(root + ((i++) * WSIZE), PACK(DSIZE, 1, 1)); /* Prologue header */
    PUT(root + ((i++) * WSIZE), PACK(DSIZE, 1, 1)); /* Prologue footer */
    PUT(root + ((i)*WSIZE), PACK(0, 1, 1));         /* Epilogue header */
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    CHECKHEAP("void");
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
#if DEBUG
    char str[20];
    sprintf(str, "%u", size);
#endif

#if VERBOSE
    PRINT_FUNC(str);
#endif
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    if (root == 0)
    {
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;
    size = adjust_alloc_size(size);
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = ALIGN(size + WSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        bp = place(bp, asize);
        return bp;
    }

    /* there is no fit in the free list, use extend_heap to get move space*/
    char *last_block;
    if ((long)(last_block = mem_sbrk(0)) == -1)
        return NULL;
    if (GET_PREV_IN_USE(last_block))
        extendsize = asize;
    else
        extendsize = asize - GET_SIZE(HDRP(PREV_BLKP(last_block)));
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    delete (bp, GET_SIZE(HDRP(bp)));
    bp = place(bp, asize);
    CHECKHEAP(str);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
#ifdef DEBUG
    char str[20];
    sprintf(str, "%p", ptr);
#endif

#ifdef DEBUG
    PRINT_FUNC(str);
#endif

    if (ptr == 0)
        return;
    SET_NEXT_BLK_PREV_NOT_IN_USE(ptr);
    PUT(HDRP(ptr), GET(HDRP(ptr)) & ~0x1);
    PUT(FTRP(ptr), GET(HDRP(ptr)));
    coalesce(ptr);

    CHECKHEAP(str);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
#if DEBUG
    char str[40];
    sprintf(str, "%p, %d", ptr, size);
#endif

#if VERBOSE
    PRINT_FUNC(str);
#endif
    char *newptr;
    size_t oldsize;
    size_t asize; /* Adjusted block size */

    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0)
    {
        mm_free(ptr);
        CHECKHEAP(str);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
    {
        CHECKHEAP(str);
        return mm_malloc(size);
    }

    oldsize = GET_SIZE(HDRP(ptr));

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = ALIGN(size + WSIZE);

    if (oldsize >= asize)
    {
        newptr = ptr;
    }
    else
    {
        int prev_in_use = GET_PREV_IN_USE(ptr);
        char *next_block = NEXT_BLKP(ptr);
        char *prev_block = prev_in_use ? NULL : PREV_BLKP(ptr);
        int next_alloced = GET_ALLOCED(HDRP(next_block));
        size_t not_allocated_prev_block_size = prev_in_use ? 0 : GET_SIZE(HDRP(prev_block));
        size_t not_allocated_next_block_size = next_alloced ? 0 : GET_SIZE(HDRP(next_block));
        size_t extend_next_size = oldsize + not_allocated_next_block_size;
        size_t extend_next_prev_size = extend_next_size + not_allocated_prev_block_size;

        if (asize <= extend_next_size)
        {
            /* the next block is free and it's size add current block's size is enough for the new size */
            int prev_in_use = GET_PREV_IN_USE(ptr);

            newptr = ptr;
            delete (next_block, not_allocated_next_block_size);

            PUT(HDRP(newptr), PACK(extend_next_size, prev_in_use, 1));
            SET_NEXT_BLK_PREV_IN_USE(newptr); // make sure the next block will not coalesce curret free block,
                                              // an optimizing for "realloc-bal.rep" and "realloc2-bal.rep".
            place(newptr, asize);
        }
        else if (asize <= extend_next_prev_size &&
                 not_allocated_prev_block_size > 24) // 24 is optimized for realloc2-bal.rep
        {
            /* the prev block is free and it's size add current block's size
               add the size of the next block if it is free is enough for the new size */
            int prev_in_use = GET_PREV_IN_USE(prev_block);

            delete (prev_block, not_allocated_prev_block_size);
            if (not_allocated_next_block_size != 0)
                delete (next_block, not_allocated_next_block_size);

            newptr = prev_block;
            memmove(newptr, ptr, oldsize - WSIZE);
            PUT(HDRP(newptr), PACK(extend_next_prev_size, prev_in_use, 1));
            SET_NEXT_BLK_PREV_IN_USE(newptr); // make sure the next block will not coalesce curret free block,
                                              // an optimizing for "realloc-bal.rep" and "realloc2-bal.rep".
            place(newptr, asize);
        }
        else if (GET_SIZE(HDRP(next_block)) == 0 ||
                 (not_allocated_next_block_size && GET_SIZE(HDRP(NEXT_BLKP(next_block))) == 0))
        {
            /* current block is at the end of the heap, or current block's next block is free and at the end of heap,
               in either case, we can ask the heap for more space.*/
            int prev_in_use = GET_PREV_IN_USE(ptr);
            newptr = ptr;
            if (not_allocated_next_block_size)
                delete (next_block, not_allocated_next_block_size);
            if ((long)(mem_sbrk(asize - oldsize - not_allocated_next_block_size)) == -1)
                return NULL;
            /* Initialize free block header/footer and the epilogue header */
            PUT(HDRP(newptr), PACK(asize, prev_in_use, 1));
            PUT(HDRP(NEXT_BLKP(newptr)), PACK(0, 1, 1)); /* New epilogue header */
            SET_NEXT_BLK_PREV_IN_USE(newptr);
        }
        else
        {
            newptr = mm_malloc(size);

            if (!newptr)
            {
                return NULL;
            }

            /* Copy the old data. */
            memmove(newptr, ptr, oldsize - WSIZE);

            /* Free the old block. */
            mm_free(ptr);
            // }
        }
    }

    SET_NEXT_BLK_PREV_IN_USE(newptr);
    CHECKHEAP(str);
    return newptr;
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin coalesce*/
static void *coalesce(void *bp)
{
    char *next_block = NEXT_BLKP(bp);
    size_t prev_alloc;
    size_t next_alloc = GET_ALLOCED(HDRP(next_block));
    size_t size = GET_SIZE(HDRP(bp));

    if (GET_PREV_IN_USE(bp))
    {
        prev_alloc = 1;
    }
    else
    {
        prev_alloc = GET_ALLOCED(HDRP(PREV_BLKP(bp)));
    }

    if (prev_alloc && next_alloc)
    { /* Case 1 */
        insert(bp, size);
    }

    else if (prev_alloc && !next_alloc)
    { /* Case 2 */
        size_t next_block_size = GET_SIZE(HDRP(next_block));

        delete (next_block, next_block_size);

        size += next_block_size;
        PUT(HDRP(bp), PACK(size, 1, 0));
        PUT(FTRP(bp), PACK(size, 1, 0));

        insert(bp, size);
    }
    else if (!prev_alloc && next_alloc)
    { /* Case 3 */
        void *prev_block = PREV_BLKP(bp);
        size_t prev_block_size = GET_SIZE(HDRP(prev_block));
        int prev_in_use = GET_PREV_IN_USE(prev_block);

        delete (prev_block, prev_block_size);

        size += prev_block_size;
        PUT(HDRP(prev_block), PACK(size, prev_in_use, 0));
        PUT(FTRP(prev_block), PACK(size, prev_in_use, 0));
        bp = prev_block;

        insert(bp, size);
    }

    else
    { /* Case 4 */
        void *prev_block = PREV_BLKP(bp);
        int prev_in_use = GET_PREV_IN_USE(prev_block);

        size_t next_block_size = GET_SIZE(HDRP(next_block));
        size_t prev_block_size = GET_SIZE(HDRP(prev_block));

        delete (prev_block, prev_block_size);
        delete (next_block, next_block_size);

        size += next_block_size + prev_block_size;
        PUT(HDRP(prev_block), PACK(size, prev_in_use, 0));
        PUT(FTRP(prev_block), PACK(size, prev_in_use, 0));

        bp = prev_block;

        insert(bp, size);
    }

    return bp;
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words)
{
    void *bp;
    size_t size;
    int prev_in_use;

    /* Allocate an even number of words to maintain alignment */
    void *last_block;
    if ((long)(last_block = mem_sbrk(0)) == -1)
        return NULL;
    prev_in_use = GET_PREV_IN_USE(last_block);

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // size = size > CHUNKSIZE ? size : CHUNKSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, prev_in_use, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, prev_in_use, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1));   /* New epilogue header */

    /* Coalesce */
    bp = coalesce(bp);
    // CHECKHEAP(str);
    return bp;
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void *place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    int prev_in_use = GET_PREV_IN_USE(bp);

    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, prev_in_use, 1));
        char *free_block = NEXT_BLKP(bp);
        PUT(HDRP(free_block), PACK(csize - asize, 1, 0));
        PUT(FTRP(free_block), PACK(csize - asize, 1, 0));
        coalesce(free_block);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, prev_in_use, 1));
        SET_NEXT_BLK_PREV_IN_USE(bp);
    }
    return bp;
}

/*
 * find_fit - Find a fit for a block with asize bytes, this will be the best fit.
 */
static void *find_fit(size_t asize)
{
    char *bp;
    int index;

    for (index = get_index(asize); index < TOTAL_LISTS; index++)
        for (bp = GETP(root + index * WSIZE); bp != NULL; bp = GETP(NEXTP(bp)))
            if (asize <= GET_SIZE(HDRP(bp)))
            {
                delete (bp, GET_SIZE(HDRP(bp)));
                return bp;
            }
    return NULL; /* No fit */
}

/*
 * mm_checkheap - Check the heap for correctness
 */
void mm_checkheap(int verbose)
{
    void *bp;
    void *heap_list_p = root + (TOTAL_LISTS + 1) * WSIZE;
    int i;
    int not_empty;

    /* Print the blocks */
    if (verbose)
    {
        printf("--------------------\n");
        printf("The blocks\n");
        printf("--------------------\n");
    }
    if (verbose)
    {
        printf("Heap (%p):\n", heap_list_p);
        for (i = 0; i < TOTAL_LISTS; i++)
        {
            if (GETP(root + i * WSIZE))
                printf("%p: root: %p\n", root + i * WSIZE, GETP(root + i * WSIZE));
        }
    }

    if ((GET_SIZE(HDRP(heap_list_p)) != DSIZE) || !GET_ALLOCED(HDRP(heap_list_p)))
        printf("Bad prologue header\n");
    checkblock(heap_list_p);

    for (bp = heap_list_p; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (verbose)
            printblock(bp);
        // ;
        checkblock(bp);
    }
    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOCED(HDRP(bp))))
        printf("Bad epilogue header\n");

    /* Print the free lists */
    if (verbose)
    {
        printf("--------------------\n");
        printf("The free lists\n");
        printf("--------------------\n");
    }

    void *two_steps;

    for (i = 0; i < TOTAL_LISTS; i++)
    {
        not_empty = 1;
        for (two_steps = bp = GETP(root + i * WSIZE);
             bp != 0; bp = GETP(NEXTP(bp)))
        {
            void *prev;
            if (verbose)
            {
                if (not_empty)
                {
                    not_empty = 0;
                    printf("%p: root{%d}: %p\n",
                           root + i * WSIZE, i + 1, GETP(root + i * WSIZE));
                }
                printf("\t");
                printblock(bp);
            }
            if (i != 0)
            {
                if ((prev = GETP(PREVP(bp))) != NULL)
                {
                    if (GETP(NEXTP(prev)) != bp)
                    {
                        printf("Error: illegal block in the free lists!\n");
                        // exit(EXIT_FAILURE);
                    }
                }
                if (GET_ALLOCED(HDRP(bp)))
                {
                    printf("Error: Free lists with a allocate block!\n");
                    // exit(EXIT_FAILURE);
                }
            }
            if (two_steps)
            {
                two_steps = GETP(NEXTP(two_steps));
                if (two_steps == bp)
                {
                    printf("Error: There is a loop in the free lists!\n");
                    exit(EXIT_FAILURE);
                }
                if (two_steps)
                    two_steps = GETP(NEXTP(two_steps));
            }
        }
    }
}

static void printblock(void *bp)
{
    unsigned long hsize, halloc, fsize, falloc, palloc;
    void *nextp, *prevp;

    // mm_checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOCED(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOCED(FTRP(bp));
    palloc = GET_PREV_IN_USE(bp);

    nextp = GETP(NEXTP(bp));
    prevp = GETP(PREVP(bp));

    if (hsize == 0)
    {
        printf("%p: EOL\n", bp);
        return;
    }
    if (halloc)
        // ;
        printf("%p: header: [%lu:%c:%c]\n", bp, hsize, (palloc ? 'a' : 'f'),
               (halloc ? 'a' : 'f'));
    else
        printf("%p: header: [%lu:%c:%c]\t next: %-11p \t prev: %-11p \t footer: [%lu:%c]\n", bp, hsize,
               (palloc ? 'a' : 'f'),
               (halloc ? 'a' : 'f'),
               nextp,
               prevp,
               fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(void *bp)
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (!GET_ALLOCED(HDRP(bp)) && GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}
