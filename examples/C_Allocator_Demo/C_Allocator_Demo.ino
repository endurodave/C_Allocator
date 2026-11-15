// C_Allocator demo: fixed block allocations from static pools, no heap.
//
// fb_allocator hands out blocks of one size from a static pool. x_allocator
// combines several fb_allocators into a malloc/free style API that picks the
// smallest block that fits. Results are printed on Serial at 115200.
//
// @see https://github.com/endurodave/C_Allocator

#include <C_Allocator.h>

// fb_allocator: 8 blocks of 24 bytes in a static pool
ALLOC_DEFINE(packetAlloc, 24, 8)

// x_allocator: pools of 16, 64 and 128 byte blocks (plus each block's
// x_allocator header), smallest first
ALLOC_DEFINE(xAlloc16, 16 + XALLOC_BLOCK_META_DATA_SIZE, 8)
ALLOC_DEFINE(xAlloc64, 64 + XALLOC_BLOCK_META_DATA_SIZE, 4)
ALLOC_DEFINE(xAlloc128, 128 + XALLOC_BLOCK_META_DATA_SIZE, 2)

static ALLOC_Allocator* const xAllocators[] = { &xAlloc16Obj, &xAlloc64Obj, &xAlloc128Obj };
static XAllocData xData = { xAllocators, sizeof(xAllocators) / sizeof(xAllocators[0]) };

// malloc/free style API over the pools above
static void* MyMalloc(size_t size) { return XALLOC_Alloc(&xData, size); }
static void* MyRealloc(void* ptr, size_t size) { return XALLOC_Realloc(&xData, ptr, size); }
static void MyFree(void* ptr) { XALLOC_Free(ptr); }

static bool Demo()
{
    bool ok = true;

    // fb_allocator: a freed block is reused, never returned to a heap
    void* a = ALLOC_Alloc(packetAlloc, 24);
    ALLOC_Free(packetAlloc, a);
    void* b = ALLOC_Alloc(packetAlloc, 24);
    Serial.println(a == b ? "fb_allocator block reused: yes" : "fb_allocator block reused: no");
    ok = ok && a == b;
    ALLOC_Free(packetAlloc, b);

    // x_allocator: each request goes to the smallest pool that fits
    char* str = (char*)MyMalloc(12);
    strcpy(str, "C_Allocator");
    str = (char*)MyRealloc(str, 60);              // moves to a 64-byte block
    strcat(str, " static pools");
    Serial.print("x_allocator realloc: ");
    Serial.println(str);
    ok = ok && strcmp(str, "C_Allocator static pools") == 0;
    MyFree(str);

    // Many allocations of mixed sizes: every block must come back
    for (int loop = 0; loop < 100 && ok; loop++)
    {
        void* blocks[4];
        for (int i = 0; i < 4; i++)
            blocks[i] = MyMalloc(1 + (size_t)((loop * 7 + i * 29) % 60));
        for (int i = 0; i < 4; i++)
        {
            ok = ok && blocks[i] != NULL;
            MyFree(blocks[i]);
        }
    }

    Serial.print("Blocks in use: ");
    Serial.print(xAlloc16Obj.blocksInUse + xAlloc64Obj.blocksInUse + xAlloc128Obj.blocksInUse);
    Serial.print(", most at once: 16-byte ");
    Serial.print(xAlloc16Obj.maxBlocksInUse);
    Serial.print(", 64-byte ");
    Serial.println(xAlloc64Obj.maxBlocksInUse);
    ok = ok && xAlloc16Obj.blocksInUse == 0 && xAlloc64Obj.blocksInUse == 0 && xAlloc128Obj.blocksInUse == 0;
    return ok;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("C_Allocator demo");
    ALLOC_Init();
    Serial.println(Demo() ? "C_Allocator demo PASSED" : "C_Allocator demo FAILED");
}

void loop()
{
}
