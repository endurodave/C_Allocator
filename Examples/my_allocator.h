#ifndef _MY_ALLOCATOR_H
#define _MY_ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* MYALLOC_Alloc(size_t size);
void MYALLOC_Free(void* ptr);
void* MYALLOC_Realloc(void *ptr, size_t new_size);
void* MYALLOC_Calloc(size_t num, size_t size);

#ifdef __cplusplus
}
#endif

#endif // _MY_ALLOCATOR_H
