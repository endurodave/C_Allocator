#ifndef _BM_ALLOCATOR_H
#define _BM_ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* BMALLOC_Alloc(size_t size);
void BMALLOC_Free(void* ptr);
void* BMALLOC_Realloc(void *ptr, size_t new_size);
void* BMALLOC_Calloc(size_t num, size_t size);

#ifdef __cplusplus
}
#endif

#endif // _BM_ALLOCATOR_H
