#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "include/types.h"

#define WSIZE   sizeof(int)
#define DWSIZE  sizeof(long)
#define ALIGN   DWSIZE  // how many bytes to use with aligning 
#define MIN_BLOCK_SIZE  ALIGN + 2 * WSIZE       // minimum block size

#define GET(p)              (*(unsigned int *) (p))
#define PUT(p, val)         ((*(unsigned int *) (p)) = val)
#define PACK(size, afb)     (size << 3 | afb)

/* 
    29                             3
    11111111 1111111 1111111 11111 111 
    |____________________________| |_|
              BLOCK SIZE        INFO BITS
*/

#define GET_SIZE(p)         ( ( (unsigned int)(GET(p) & ~0x7) ) >> 3)
#define GET_INFO_BITS(p)    (GET(p) & 0x7)  // 0b111
#define GET_FA_STATUS(p)    (GET(p) & 0x1)  // is allocated or free


/* get header\footer pointer from payload  base */
#define HDPR(bp)    ((char *)(bp) - WSIZE)    
#define FTPR(bp)    (HDPR(bp) + GET_SIZE(HDPR(bp)) - WSIZE)

/* get pointer to next\prev payload block */
#define NEXT_BLKP(bp)   (FTPR(bp) + 2 * WSIZE) 
#define PREV_BLKP(bp)   (bp - GET_SIZE(HDPR(bp) - WSIZE))


#define EXT_NIGGA_CHUNK     512

/* for additional info */
#define DEBUG 1
#define NIGGA_PRINT 1

/* 
HEADER

   31               0
    +---------------+
    + BLOCK SIZE |A +
    +            |A +
    +---------------+
    + PAYLOAD       +
    +---------------+
    + PADDING       +
    +---------------+
    + FOOTER        +
    +---------------+ 

PAYLOAD IS DOUBLE WORD ALIGNET (DWSIZE)
*/

static char *mem_heap;
static char *mem_brk;

extern void nig_itoa(int val, char *dst);
extern void niga_print(char *format, int count, ...);


static void *nig_sbrk(intptr_t inc)
{
    void *p;

    p = sbrk(inc);
    
    if ( (*(int *)(p)) == -1){
        return (void *) -1;
    }

    mem_brk += inc;
    return p;
}


static void *extend_heap(size_t words)
{
    size_t awords, size;      // align words
    void *bp;

    if (words == 0){
        return (void *) -1;
    }

    awords = words;

    if ((awords % 2) != 0){
        awords += 1;
    }


    /* 
    *   MINIMUM EXTEND BLOCK SIZE WILL ALWAYS BE 16 BYTES
    *   EVERY EXTEND BLOCK WILL ALWAYS BE ALIGNED BY 8
    * 
    *   example 1: extend_heap(8);
    *   awords = 8
    *   size   = 8 * 4 + 2 * 4 = 40 bytes 
    *
    *   example 2: extend_heap(15);
    *   awords = 16
    *   size = 16 * 4 + 2 * 4 = 72 bytes  
    * 
    *   example 3: extend_heap(1);
    *   awords = 2
    *   size = 2 * 4 + 2 * 4 = 16 bytes 
    */
    
    size = awords * WSIZE + 2 * WSIZE;      // align words for payload + header + footer
    
    bp = nig_sbrk(size);

    if ( (*(int *)(bp)) ==  -1){
        return NULL;
    }
    coalesce(bp);

    PUT(HDPR(bp), PACK(size,  0));           //  mark current epilogue as free header
    PUT(FTPR(bp), PACK(size, 0));            //  mark new free block's footer
    PUT( HDPR(NEXT_BLKP(bp)), PACK(0, 1));   //  mark new epilogue
    
    #if NIGGA_PRINT
        niga_print("%d NIGGAbytes was allocated\n", 1, size);
    #endif
    return bp;
}



void mem_init()
{
    static int been_called = 0;

    if (been_called){
        return;
    }
    
    /* 
     *  allocate:
     *      1) align block
     *      2) prologue header
     *      3) prologue footer
     *      4) epilogue 
     */
    
    mem_heap = sbrk(4 * WSIZE);
    mem_brk = mem_heap + 4 * WSIZE;     //  init first time mem_break
    
    /* #if DEBUG
        printf("mem_heap: %p\n", mem_heap);
        printf("mem_brk1: %p\n", mem_brk);
    #endif */

    PUT(mem_heap, PACK(0, 1));                      //  alignment
    PUT(mem_heap + 1 * WSIZE, PACK(DWSIZE, 1));     //  prologue header
    PUT(mem_heap + 2 * WSIZE, PACK(DWSIZE, 1));     //  prologue footer
    PUT(mem_heap + 3 * WSIZE, PACK(0, 1));          //  epilogue

    extend_heap(EXT_NIGGA_CHUNK/WSIZE);
    
    /* #if DEBUG
        printf("mem_brk2: %p\n", mem_brk);
    #endif */
}


void nigga_free(void *bp)
{
    PUT(HDPR(bp),  PACK( GET_SIZE(HDPR(bp)), 0) );    // mark header as free
    PUT(FTPR(bp),  PACK( GET_SIZE(FTPR(bp)), 0) );    // mark footer as free
    
    #if NIGGA_PRINT
        niga_print("free: %d niggabytes\n", 1, GET_SIZE(HDPR(bp)));
    #endif

    coalesce(bp);
}

void coalesce(void *bp)
{
    int prev_fa, next_fa;
    int prev_size, cur_size, next_size, ttl_size;

    next_fa = GET_FA_STATUS( HDPR( NEXT_BLKP(bp) ) );
    prev_fa = GET_FA_STATUS( HDPR( PREV_BLKP(bp) ) );

    /* 
    *   00  (case 0)  -> prev and next are free
    *   01  (case 1)  -> prev is free,  next is allocated
    *   10  (case 2)  -> next is free,  prev is allocated
    *   11  (case 3)  -> prev and next are allocated
    *  */

    switch ((prev_fa << 1) | next_fa){
        case 0:        

            next_size = GET_SIZE( HDPR(NEXT_BLKP(bp)) );
            cur_size  = GET_SIZE( HDPR(bp) );
            prev_size = GET_SIZE( HDPR(PREV_BLKP(bp)) );
            ttl_size = next_size + cur_size + prev_size;

            PUT( HDPR(PREV_BLKP(bp)), PACK(ttl_size, 0) );
            
            #if DEBUG
                niga_print("prev and next block are coalesced together."
                    " New free block size: %d\n", 1, ttl_size);
            #endif

            break;
    
        case 1:        

            cur_size  = GET_SIZE( HDPR(bp) );
            prev_size = GET_SIZE( HDPR(PREV_BLKP(bp)) );
            ttl_size = cur_size + prev_size;    

            PUT( HDPR(PREV_BLKP(bp)), PACK(ttl_size, 0) );

            #if DEBUG
                niga_print("prev block is coalesced."
                    " New free block size: %d\n",1, ttl_size);
            #endif
            break;

        case 2:
            cur_size  = GET_SIZE( HDPR(bp) );
            next_size = GET_SIZE( HDPR(NEXT_BLKP(bp)) );
            ttl_size = cur_size + next_size;    

            PUT( HDPR(bp), PACK(ttl_size, 0) );
            
            #if DEBUG
                niga_print("next block is coalesced."
                    " New free block size: %d\n", 1, ttl_size);
            #endif

            break;
        case 3:
            #if DEBUG 
                niga_print("nothing to coalesce\n", 0);
            #endif
            
            break;
    }
    return;
}


static void *first_fit(int size)
{
    char *p;

    p = mem_heap + 4 * WSIZE; // point to first block
    
    while(1){
        if (GET_SIZE(HDPR(p)) == 0){
            return (void *) -1;
        }

        if (GET_SIZE(HDPR(p)) >= size && !GET_FA_STATUS(HDPR(p))){
            return p;
        } 

        p = NEXT_BLKP(p);
    }
}


void *niggalloc(int size)
{
    int wsize;
    int fsize;
    char *p;


    wsize = ALIGN;

    if (size > ALIGN){
        wsize = size;
    }
    
    if ((wsize % ALIGN) != 0){
        wsize += ALIGN - (wsize % ALIGN);
    }

    p = first_fit(wsize);
    
    if (p == (void *) -1){
        p = extend_heap(wsize / WSIZE);
    }

    /* 
        SPLITTING
            1) if free block size - allocated request size > min block size ->
                CUT INTO 2 CHUNKS
            2) if free block size - allocated request size > min block size ->
                ALLOCATE WHOLE FREE BLOCK
    */
    
    fsize = GET_SIZE(HDPR(p));

    if (fsize - (wsize + 2 * WSIZE) > MIN_BLOCK_SIZE){
        PUT( HDPR(p), PACK(wsize + 2 * WSIZE, 1));
        PUT( FTPR(p), PACK(wsize + 2 * WSIZE, 1));

        PUT(HDPR(NEXT_BLKP(p)), PACK(fsize - (wsize + 2 * WSIZE), 0));
        PUT(FTPR(NEXT_BLKP(p)), PACK(fsize - (wsize + 2 * WSIZE), 0));

    } else {
        PUT( HDPR(p), PACK( fsize, 1 ));
        PUT( FTPR(p), PACK( fsize, 1 ));
    }
    return p;

}


int main(int argc, char *argv[])
{
    mem_init();

    char *p = niggalloc(1024);
    memcpy(p, "nigger", 7);
    char *p2 = niggalloc(500);
    printf("p2: %p\n", p2);
    char *p3 = niggalloc(120);
    nigga_free(p);
    char *p4 = niggalloc(1000);
    printf("p4: %p\t%s\n", p4, p4);
    return 0;
}