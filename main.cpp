#include "stdafx.h"
#include "fb_allocator.h"
#include "my_allocator.h"
#include "bm_allocator.h"
#include "Fault.h"
#include <iostream>
#include <string.h>

using namespace std;

// Benchmark allocators defs
typedef void* (*AllocFunc)(int size);
typedef void(*DeallocFunc)(void* ptr);
static const int MAX_BLOCKS = 10000;
static const int MAX_BLOCK_SIZE = 4096;
void* memoryPtrs[MAX_BLOCKS];
void* memoryPtrs2[MAX_BLOCKS];

void Benchmark(const char* name, AllocFunc allocFunc, DeallocFunc deallocFunc);
void* AllocHeap(int size);
void DeallocHeap(void* ptr);
void* AllocFbAllocator(int size);
void DeallocFbAllocator(void* ptr);
void* AllocXAllocator(int size);
void DeallocXAllocator(void* ptr);

struct MyData
{
    CHAR data[128];
};

ALLOC_DEFINE(TestAlloc, 16, 5)
ALLOC_DEFINE(BenchmarkFbAlloc, MAX_BLOCK_SIZE, MAX_BLOCKS*2)

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

    Benchmark("Heap (Run 1)", AllocHeap, DeallocHeap);
    Benchmark("Heap (Run 2)", AllocHeap, DeallocHeap);
    Benchmark("Heap (Run 3)", AllocHeap, DeallocHeap);

    Benchmark("fb_allocator (Run 1)", AllocFbAllocator, DeallocFbAllocator);
    Benchmark("fb_allocator (Run 2)", AllocFbAllocator, DeallocFbAllocator);
    Benchmark("fb_allocator (Run 3)", AllocFbAllocator, DeallocFbAllocator);

    Benchmark("bm_allocator (Run 1)", AllocXAllocator, DeallocXAllocator);
    Benchmark("bm_allocator (Run 2)", AllocXAllocator, DeallocXAllocator);
    Benchmark("bm_allocator (Run 3)", AllocXAllocator, DeallocXAllocator);

    return 0;
}

//------------------------------------------------------------------------------
// AllocHeap
//------------------------------------------------------------------------------
void* AllocHeap(int size)
{
    return malloc(size);
}

//------------------------------------------------------------------------------
// DeallocHeap
//------------------------------------------------------------------------------
void DeallocHeap(void* ptr)
{
    free(ptr);
}

//------------------------------------------------------------------------------
// AllocFbAllocator
//------------------------------------------------------------------------------
void* AllocFbAllocator(int size)
{
    return ALLOC_Alloc(BenchmarkFbAlloc, size);
}

//------------------------------------------------------------------------------
// DeallocFbAllocator
//------------------------------------------------------------------------------
void DeallocFbAllocator(void* ptr)
{
    ALLOC_Free(BenchmarkFbAlloc, ptr);
}

//------------------------------------------------------------------------------
// AllocXAllocator
//------------------------------------------------------------------------------
void* AllocXAllocator(int size)
{
    return BMALLOC_Alloc(size);
}

//------------------------------------------------------------------------------
// DeallocXAllocator
//------------------------------------------------------------------------------
void DeallocXAllocator(void* ptr)
{
    return BMALLOC_Free(ptr);
}

//------------------------------------------------------------------------------
// Benchmark
//------------------------------------------------------------------------------
void Benchmark(const char* name, AllocFunc allocFunc, DeallocFunc deallocFunc)
{
#if WIN32
    LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds, TotalElapsedMicroseconds = { 0 };
    LARGE_INTEGER Frequency;

    SetProcessPriorityBoost(GetCurrentProcess(), true);

    QueryPerformanceFrequency(&Frequency);

    // Allocate MAX_BLOCKS blocks MAX_BLOCK_SIZE / 2 sized blocks
    QueryPerformanceCounter(&StartingTime);
    for (int i = 0; i<MAX_BLOCKS; i++)
        memoryPtrs[i] = allocFunc(MAX_BLOCK_SIZE / 2);
    QueryPerformanceCounter(&EndingTime);
    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    ElapsedMicroseconds.QuadPart *= 1000000;
    ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
    std::cout << name << " allocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
    TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

    // Deallocate MAX_BLOCKS blocks (every other one)
    QueryPerformanceCounter(&StartingTime);
    for (int i = 0; i<MAX_BLOCKS; i += 2)
        deallocFunc(memoryPtrs[i]);
    QueryPerformanceCounter(&EndingTime);
    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    ElapsedMicroseconds.QuadPart *= 1000000;
    ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
    std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
    TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

    // Allocate MAX_BLOCKS blocks MAX_BLOCK_SIZE sized blocks
    QueryPerformanceCounter(&StartingTime);
    for (int i = 0; i<MAX_BLOCKS; i++)
        memoryPtrs2[i] = allocFunc(MAX_BLOCK_SIZE);
    QueryPerformanceCounter(&EndingTime);
    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    ElapsedMicroseconds.QuadPart *= 1000000;
    ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
    std::cout << name << " allocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
    TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

    // Deallocate MAX_BLOCKS blocks (every other one)
    QueryPerformanceCounter(&StartingTime);
    for (int i = 1; i<MAX_BLOCKS; i += 2)
        deallocFunc(memoryPtrs[i]);
    QueryPerformanceCounter(&EndingTime);
    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    ElapsedMicroseconds.QuadPart *= 1000000;
    ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
    std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
    TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

    // Deallocate MAX_BLOCKS blocks 
    QueryPerformanceCounter(&StartingTime);
    for (int i = MAX_BLOCKS - 1; i >= 0; i--)
        deallocFunc(memoryPtrs2[i]);
    QueryPerformanceCounter(&EndingTime);
    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    ElapsedMicroseconds.QuadPart *= 1000000;
    ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
    std::cout << name << " deallocate time: " << ElapsedMicroseconds.QuadPart << std::endl;
    TotalElapsedMicroseconds.QuadPart += ElapsedMicroseconds.QuadPart;

    std::cout << name << " TOTAL TIME: " << TotalElapsedMicroseconds.QuadPart << "\n" << std::endl;

    SetProcessPriorityBoost(GetCurrentProcess(), false);
#endif
}

