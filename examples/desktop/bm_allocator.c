// BMALLOC allocates either a 2048 or 4096 byte blocks depending 
// on the requested size. 

#include "my_allocator.h"
#include "x_allocator.h"

// Maximum number of blocks for each size
#define MAX_2048_BLOCKS     10000
#define MAX_4096_BLOCKS     10000


// Define size of each block including meta data overhead
#define BLOCK_2048_SIZE     2048 + XALLOC_BLOCK_META_DATA_SIZE
#define BLOCK_4096_SIZE     4096 + XALLOC_BLOCK_META_DATA_SIZE

// Define individual fb_allocators
ALLOC_DEFINE(bmDataAllocator2048, BLOCK_2048_SIZE, MAX_2048_BLOCKS)
ALLOC_DEFINE(bmDataAllocator4096, BLOCK_4096_SIZE, MAX_4096_BLOCKS)

// An array of allocators sorted by smallest block first
static ALLOC_Allocator* allocators[] = {
    &bmDataAllocator2048Obj,
    &bmDataAllocator4096Obj
};

#define MAX_ALLOCATORS   (sizeof(allocators) / sizeof(allocators[0]))

static XAllocData self = { allocators, MAX_ALLOCATORS };

//----------------------------------------------------------------------------
// BMALLOC_Alloc
//----------------------------------------------------------------------------
void* BMALLOC_Alloc(size_t size)
{
    return XALLOC_Alloc(&self, size);
}

//----------------------------------------------------------------------------
// BMALLOC_Free
//----------------------------------------------------------------------------
void BMALLOC_Free(void* ptr)
{
    XALLOC_Free(ptr);
}

//----------------------------------------------------------------------------
// BMALLOC_Realloc
//----------------------------------------------------------------------------
void* BMALLOC_Realloc(void *ptr, size_t new_size)
{
    return XALLOC_Realloc(&self, ptr, new_size);
}

//----------------------------------------------------------------------------
// BMALLOC_Calloc
//----------------------------------------------------------------------------
void* BMALLOC_Calloc(size_t num, size_t size)
{
    return XALLOC_Calloc(&self, num, size);
}

