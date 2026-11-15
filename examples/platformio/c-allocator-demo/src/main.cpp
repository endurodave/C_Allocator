// C_Allocator PlatformIO example: fixed block allocations from static pools,
// with no heap. Runs on Arduino boards and the desktop (native).
//
// fb_allocator handles one block size. x_allocator combines several
// fb_allocators into a malloc/free style API that picks the smallest block
// that fits. Both are C; this example is C++ only for Arduino's setup()/loop().
//
// @see https://github.com/endurodave/C_Allocator

#include "fb_allocator.h"
#include "x_allocator.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

#if defined(ALLOC_LOCK_FREERTOS) && defined(ARDUINO_ARCH_STM32)
// ALLOC_LOCK_FREERTOS: the allocators guard their free-lists with a FreeRTOS
// critical section, so tasks can share them (see platformio.ini)
#include <STM32FreeRTOS.h>
#define USE_FREERTOS_TASKS
#endif

static void Log(const char* msg)
{
#ifdef ARDUINO
    Serial.println(msg);
#else
    printf("%s\n", msg);
#endif
}

// fb_allocator: 8 blocks of 24 bytes in a static pool
ALLOC_DEFINE(packetAlloc, 24, 8)

// x_allocator: three pools of 16, 64 and 256 byte blocks (plus each block's
// x_allocator header), smallest first
ALLOC_DEFINE(xAlloc16, 16 + XALLOC_BLOCK_META_DATA_SIZE, 16)
ALLOC_DEFINE(xAlloc64, 64 + XALLOC_BLOCK_META_DATA_SIZE, 16)
ALLOC_DEFINE(xAlloc256, 256 + XALLOC_BLOCK_META_DATA_SIZE, 4)

static ALLOC_Allocator* const xAllocators[] = { &xAlloc16Obj, &xAlloc64Obj, &xAlloc256Obj };
static XAllocData xData = { xAllocators, sizeof(xAllocators) / sizeof(xAllocators[0]) };

// malloc/free style API over the pools above
static void* MyMalloc(size_t size) { return XALLOC_Alloc(&xData, size); }
static void* MyRealloc(void* ptr, size_t size) { return XALLOC_Realloc(&xData, ptr, size); }
static void MyFree(void* ptr) { XALLOC_Free(ptr); }

static bool IsAligned(const void* p) { return ((uintptr_t)p % sizeof(void*)) == 0; }

static bool Demo()
{
    char buf[96];
    bool ok = true;

    // fb_allocator: freed blocks are reused, never returned to a heap
    void* a = ALLOC_Alloc(packetAlloc, 24);
    ALLOC_Free(packetAlloc, a);
    void* b = ALLOC_Alloc(packetAlloc, 24);
    Log(a == b ? "fb_allocator block reused: yes" : "fb_allocator block reused: no");
    ok = ok && a == b && IsAligned(b);
    ALLOC_Free(packetAlloc, b);

    // x_allocator: each request goes to the smallest pool that fits
    char* str = (char*)MyMalloc(10);
    strcpy(str, "C_Allocator");
    str = (char*)MyRealloc(str, 100);
    strcat(str, " static pools");
    snprintf(buf, sizeof(buf), "x_allocator realloc: \"%s\"", str);
    Log(buf);
    ok = ok && strcmp(str, "C_Allocator static pools") == 0 && IsAligned(str);
    MyFree(str);

    snprintf(buf, sizeof(buf), "Pools in use: 16-byte %u, 64-byte %u, 256-byte %u (max %u, %u, %u)",
        xAlloc16Obj.blocksInUse, xAlloc64Obj.blocksInUse, xAlloc256Obj.blocksInUse,
        xAlloc16Obj.maxBlocksInUse, xAlloc64Obj.maxBlocksInUse, xAlloc256Obj.maxBlocksInUse);
    Log(buf);
    ok = ok && xAlloc16Obj.blocksInUse == 0 && xAlloc64Obj.blocksInUse == 0;

    return ok;
}

// Allocate, fill, check and free blocks of several sizes
static bool Stress(uint8_t pattern, int loops)
{
    const int COUNT = 6;
    void* blocks[COUNT];
    for (int loop = 0; loop < loops; loop++)
    {
        for (int i = 0; i < COUNT; i++)
        {
            size_t size = 1 + (size_t)((loop * 7 + i * 29) % 64);
            blocks[i] = MyMalloc(size);
            memset(blocks[i], pattern, size);
        }
        for (int i = 0; i < COUNT; i++)
        {
            size_t size = 1 + (size_t)((loop * 7 + i * 29) % 64);
            const uint8_t* p = (const uint8_t*)blocks[i];
            for (size_t n = 0; n < size; n++)
                if (p[n] != pattern)
                    return false;
            MyFree(blocks[i]);
        }
    }
    return true;
}

#ifdef USE_FREERTOS_TASKS
static volatile bool g_taskOk[2] = { true, true };
static volatile int g_taskDone = 0;

// Two of these run at once, sharing the x_allocator pools
static void StressTask(void* arg)
{
    int id = (int)(intptr_t)arg;
    for (int round = 0; round < 50; round++)
    {
        if (!Stress((uint8_t)(0xA0 + id), 10))
            g_taskOk[id] = false;
        vTaskDelay(1);
    }
    taskENTER_CRITICAL();
    g_taskDone = g_taskDone + 1;
    taskEXIT_CRITICAL();
    vTaskDelete(NULL);
}

static void MainTask(void*)
{
    bool ok = Demo();
    xTaskCreate(StressTask, "Stress0", 512, (void*)0, 1, NULL);
    xTaskCreate(StressTask, "Stress1", 512, (void*)1, 1, NULL);
    while (g_taskDone < 2)
        vTaskDelay(10);
    bool tasksOk = g_taskOk[0] && g_taskOk[1];
    Log(tasksOk ? "Two task stress test: passed" : "Two task stress test: FAILED");
    Log(ok && tasksOk ? "C_Allocator demo PASSED" : "C_Allocator demo FAILED");
    vTaskDelete(NULL);
}
#endif

#ifdef ARDUINO
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Log("C_Allocator demo");
    ALLOC_Init();
#ifdef USE_FREERTOS_TASKS
    xTaskCreate(MainTask, "Main", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
#else
    bool ok = Demo() && Stress(0x5A, 100);
    Log(ok ? "C_Allocator demo PASSED" : "C_Allocator demo FAILED");
#endif
}

void loop()
{
}
#else
int main()
{
    Log("C_Allocator demo");
    ALLOC_Init();
    bool ok = Demo() && Stress(0x5A, 100);
    Log(ok ? "C_Allocator demo PASSED" : "C_Allocator demo FAILED");
    ALLOC_Term();
    return ok ? 0 : 1;
}
#endif
