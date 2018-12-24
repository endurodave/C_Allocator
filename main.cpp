#include "stdafx.h"
#include "fb_allocator.h"
#include "my_allocator.h"
#include "Fault.h"
#include <iostream>
#include <string.h>

using namespace std;

struct MyData
{
    CHAR data[128];
};

ALLOC_DEFINE(TestAlloc, 16, 5);

int main()
{
    // Initialize allocator module
    ALLOC_Init();

    // Allocate and free memory using TestAlloc allocator
    void* data = ALLOC_Alloc(TestAlloc, 16);
    memset(data, 0, 16);
    ALLOC_Free(TestAlloc, data);

    // Allocate memory using MYALLOC allocator
    void* mem1 = MYALLOC_Alloc(28);
    void* mem2 = MYALLOC_Calloc(1, 32);
    mem2 = MYALLOC_Realloc(mem2, 128);

    memset(mem1, 0, 28);
    memset(mem2, 0, 128);

    MYALLOC_Free(mem1);
    MYALLOC_Free(mem2);

    MyData* myData = (MyData*)MYALLOC_Alloc(sizeof(MyData));
    memset(myData->data, 0, 128);
    MYALLOC_Free(myData);

    ASSERT_TRUE(TestAllocObj.blocksInUse == 0);

    return 0;
}

