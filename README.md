# A Fixed Block Memory Allocator in C

A C language fixed block memory allocator improve performance and protect against heap fragmentation faults on any C or C++ project.

# Table of Contents

- [A Fixed Block Memory Allocator in C](#a-fixed-block-memory-allocator-in-c)
- [Table of Contents](#table-of-contents)
- [Preface](#preface)
- [Introduction](#introduction)
- [Background](#background)
- [C Language Allocators](#c-language-allocators)
  - [fb\_allocator](#fb_allocator)
  - [x\_allocator](#x_allocator)
  - [my\_allocator](#my_allocator)
- [Implementation Details](#implementation-details)
  - [fb\_allocator Free-List](#fb_allocator-free-list)
  - [fb\_allocator Memory Alignment](#fb_allocator-memory-alignment)
  - [x\_allocator Meta Data](#x_allocator-meta-data)
- [Benchmarking](#benchmarking)
- [Thread Safety](#thread-safety)
- [Reference Articles](#reference-articles)
- [Conclusion](#conclusion)

# Preface 

Originally published on CodeProject at: <a href="https://www.codeproject.com/Articles/1272619/A-Fixed-Block-Memory-Allocator-in-C"><strong>A Fixed Block Memory Allocator in C</strong></a>

<p><a href="https://www.cmake.org/">CMake</a>&nbsp;is used to create the build files. CMake is free and open-source software. Windows, Linux and other toolchains are supported. See the <strong>CMakeLists.txt </strong>file for more information.</p>

# Introduction

<p>In 1998, I wrote an article on fixed block memory allocators for Embedded Systems Programming magazine. I received $750 for the piece. Now, articles are written for free on websites such as CodeProject. Oh, how times keep a-changin&rsquo;.</p>

<p>One thing that hasn&rsquo;t changed is the usefulness of fixed block allocators. It&rsquo;s a no-no using the global heap on some devices. Throughout my career, I&rsquo;ve written numerous fixed block memory allocators to provide high-speed dynamic-like allocation in memory constrained systems. A generic, well-designed fixed block allocator opens up application implementation possibilities by offering dynamic memory allocations in systems where heap usage is forbidden.</p>

<p>On CodeProject, I&rsquo;ve documented various C++ allocator implementations (see the Reference Articles section). This time, I&rsquo;ll present a C language version with unique features suitable for embedded devices or otherwise.</p>

<p>The solution presented here will:</p>

<ul>
	<li>Be faster than the global heap</li>
	<li>Be easy to use</li>
	<li>Be thread safe</li>
	<li>Support fixed block versions of malloc, free, realloc and calloc</li>
	<li>Use minimal code space</li>
	<li>Execute in fixed time</li>
	<li>Eliminate heap fragmentation memory faults</li>
	<li>Require no additional storage overhead (except for a few bytes of static memory)</li>
	<li>Handle memory alignment</li>
	<li>Automatically dispense variable block size based on demand (a la malloc)</li>
</ul>

<p>Two simple C modules that dispense and reclaim memory will provide all of the aforementioned benefits, as I&#39;ll show.</p>

# Background

<p>Custom fixed block memory allocators are used to solve at least two types of memory related problems. First, global heap allocations/deallocations can be slow and nondeterministic. You never know how long the memory manager is going to take. Secondly, to eliminate the possibility of a memory allocation fault caused by a fragmented heap &ndash; a valid concern, especially on mission-critical type systems.</p>

<p>Even if the system isn&#39;t considered mission-critical, some embedded systems are designed to run for weeks or years without a reboot. Depending on allocation patterns and heap implementation, long-term heap use can lead to heap faults.</p>

# C Language Allocators

## fb_allocator

<p>Each <code>fb_allocator</code> instance handles a single block size. The interface is shown below:</p>

<pre lang="C++">
void ALLOC_Init(void);
void ALLOC_Term(void);
void* ALLOC_Alloc(ALLOC_HANDLE hAlloc, size_t size);
void* ALLOC_Calloc(ALLOC_HANDLE hAlloc, size_t num, size_t size);
void ALLOC_Free(ALLOC_HANDLE hAlloc, void* pBlock);</pre>

<p><code>ALLOC_Init()</code> is called one time at startup and <code>ALLOC_Term()</code> at shutdown. Each of the remaining APIs operate in the same manner as the CRT counterparts; <code>ALLOC_Alloc()</code> allocates memory and <code>ALLOC_Free() </code>deallocates.</p>

<p>An <code>fb_allocator</code> instance is created using the <code>ALLOC_DEFINE</code> macro at file scope. The <code>TestAlloc </code>allocator below defines a fixed block allocator with a maximum of five 16-byte blocks.</p>

<pre lang="C++">
ALLOC_DEFINE(TestAlloc, 16, 5);</pre>

<p>Allocating a 16-byte block is simple.</p>

<pre lang="C++">
void* data = ALLOC_Alloc(TestAlloc, 16);</pre>

<p>Deallocate the block when done.</p>

<pre lang="C++">
ALLOC_Free(TestAlloc, data);</pre>

## x_allocator

<p>The <code>x_allocator</code> module handles multiple memory block sizes using two or more <code>fb_allocator</code> instances; one <code>fb_allocator</code> per block size. During allocation, <code>x_allocator</code> returns a block sized from one of the <code>fb_allocator</code> instances based on the caller&rsquo;s requested size. The <code>x_allocator</code> API is shown below:</p>

<pre lang="C++">
void* XALLOC_Alloc(XAllocData* self, size_t size);
void XALLOC_Free(void* ptr);
void* XALLOC_Realloc(XAllocData* self, void *ptr, size_t new_size);
void* XALLOC_Calloc(XAllocData* self, size_t num, size_t size);</pre>

<p>Users of <code>x_allocator</code> typically create a thin wrapper module that (a) defines two or more <code>fb_allocator</code> instances and (b) provides a custom API to access the <code>x_allocator</code> memory. It&rsquo;s easier to explain with a simple example.</p>

## my_allocator

<p>Let&rsquo;s say we want a fixed block allocator to dispense two block sizes: 32 and 128. We&rsquo;ll call it <code>my_allocator</code> and the API is shown below:</p>

<pre lang="C++">
void* MYALLOC_Alloc(size_t size);
void MYALLOC_Free(void* ptr);
void* MYALLOC_Realloc(void *ptr, size_t new_size);
void* MYALLOC_Calloc(size_t num, size_t size);</pre>

<p>The implementation creates multiple <code>fb_allocator</code> instances; one to handle each desired block size. In this case, we&rsquo;ll allow at most ten 32-byte blocks and five 128-byte blocks.</p>

<pre lang="C++">
#include &quot;my_allocator.h&quot;
#include &quot;x_allocator.h&quot;

// Maximum number of blocks for each size
#define MAX_32_BLOCKS   10
#define MAX_128_BLOCKS  5

// Define size of each block including meta data overhead
#define BLOCK_32_SIZE     32 + XALLOC_BLOCK_META_DATA_SIZE
#define BLOCK_128_SIZE    128 + XALLOC_BLOCK_META_DATA_SIZE

// Define individual fb_allocators
ALLOC_DEFINE(myDataAllocator32, BLOCK_32_SIZE, MAX_32_BLOCKS)
ALLOC_DEFINE(myDataAllocator128, BLOCK_128_SIZE, MAX_128_BLOCKS)

// An array of allocators sorted by smallest block first
static ALLOC_Allocator* allocators[] = {
    &amp;myDataAllocator32Obj,
    &amp;myDataAllocator128Obj
};

#define MAX_ALLOCATORS   (sizeof(allocators) / sizeof(allocators[0]))

static XAllocData self = { allocators, MAX_ALLOCATORS };</pre>

<p>Now, simple one line wrapper functions provide access to the underlying <code>x_allocator</code> module.</p>

<pre lang="C++">
void* MYALLOC_Alloc(size_t size)
{
    return XALLOC_Alloc(&amp;self, size);
}

void MYALLOC_Free(void* ptr)
{
    XALLOC_Free(ptr);
}

void* MYALLOC_Realloc(void *ptr, size_t new_size)
{
    return XALLOC_Realloc(&amp;self, ptr, new_size);
}

void* MYALLOC_Calloc(size_t num, size_t size)
{
    return XALLOC_Calloc(&amp;self, num, size);
}</pre>

<p>When the caller calls <code>MYALLOC_Alloc()</code> with a size between 1 to 32, a 32-byte block is returned. If the requested size is between 33 and 128, a 128-byte block is provided. <code>MYALLOC_Free()</code> returns the block to the originating <code>fb_allocator</code> instance. In this way, a collection of fixed block memory allocators are grouped together providing variable sized memory blocks at runtime based on application demand. The sample wrapper pattern is used again and again offering groups of memory blocks for specific purposes within the system.</p>

# Implementation Details

<p>Most of the allocator implementation is relatively straight forward. However, I&rsquo;ll explain a few details to assist with key concepts.</p>

## fb_allocator Free-List

<p>This is a handy technique for linking blocks together in the free-list without consuming any extra storage for the pointers. After the user calls <code>ALLOC_Free()</code>, a fixed memory block is no longer being utilized and is freed to be used for other things, like a next pointer. Since the <code>fb_allocator</code> module needs to keep the deleted blocks around, we put the list&#39;s next pointer in that currently unused block space. When the block is reused by the application, the pointer is no longer needed and will be overwritten by the user object. This way, there is no per-instance storage overhead incurred linking blocks together.</p>

<p>The free-list is actually implemented as a singly linked list, but the code only adds/removes from the head so the behavior is that of a stack. Using a stack makes the allocation/deallocations really fast and deterministic. No loop searching for a free block &ndash; just push or pop a block and go.</p>

<p>Using freed object space as the memory to link blocks together means the object must be large enough to hold a pointer. The <code>ALLOC_BLOCK_SIZE </code>macro ensures that the minimum size is met.</p>

## fb_allocator Memory Alignment

<p>Some embedded systems require memory to be aligned on a particular byte boundary. Since the allocator&rsquo;s memory is a contiguous <code>static</code> byte array, having blocks start on an unaligned boundary could cause a hardware exception on some CPUs. For instance, 13-byte blocks will cause a problem if 4-byte alignment is required. Change <code>ALLOC_MEM_ALIGN </code>to the byte boundary desired. The block size will be rounded up to the next nearest aligned boundary.</p>

## x_allocator Meta Data

<p>The <code>x_allocator</code> implementation adds 4-bytes of meta data per block. For instance, if 32-byte blocks are required by the user, the <code>x_allocator</code> actually uses 36-byte blocks. The extra 4-bytes are used to hide an <code>fb_allocator</code> pointer inside the block (assuming the pointer is 4-bytes in size).</p>

<p>When deleting memory, <code>x_allocator</code> needs the original <code>fb_allocator</code> instance so the deallocation request can be routed to the correct <code>fb_allocator</code> instance for processing. Unlike <code>XALLOC_Alloc()</code>, <code>XALLOC_Free()</code> does not take a size and only uses a <code>void*</code> argument. Therefore, <code>XALLOC_Alloc()</code> actually hides a pointer to the <code>fb_allocator</code> within an unused portion of the memory block by adding an additional 4-bytes to the request. The caller gets a pointer to the block&rsquo;s client region where the hidden <code>fb_allocator</code> pointer is not overwritten.</p>

<pre lang="C++">
void* XALLOC_Alloc(XAllocData* self, size_t size)
{
    ALLOC_Allocator* pAllocator;
    void* pBlockMemory = NULL;
    void* pClientMemory = NULL;

    ASSERT_TRUE(self);

    // Get an allocator instance to handle the memory request
    pAllocator = XALLOC_GetAllocator(self, size);

    // An allocator found to handle memory request?
    if (pAllocator)
    {
        // Get a fixed memory block from the allocator instance
        pBlockMemory = ALLOC_Alloc(pAllocator, size + XALLOC_BLOCK_META_DATA_SIZE);
        if (pBlockMemory)
        {
            // Set the block ALLOC_Allocator* ptr within the raw memory block region
            pClientMemory = XALLOC_PutAllocatorPtrInBlock(pBlockMemory, pAllocator);
        }
    }
    else
    {
        // Too large a memory block requested
        ASSERT();
    }

    return pClientMemory;
} </pre>

<p>When <code>XALLOC_Free()</code> is called, the allocator pointer is extracted from the memory block so the correct <code>fb_allocator</code> instance can be called to deallocate the block.</p>

<pre lang="C++">
void XALLOC_Free(void* ptr)
{
    ALLOC_Allocator* pAllocator = NULL;
    void* pBlock = NULL;

    if (!ptr)
        return;

    // Extract the original allocator instance from the caller&#39;s block pointer
    pAllocator = XALLOC_GetAllocatorPtrFromBlock(ptr);
    if (pAllocator)
    {
        // Convert the client pointer into the original raw block pointer
        pBlock = XALLOC_GetBlockPtr(ptr);

        // Deallocate the fixed memory block
        ALLOC_Free(pAllocator, pBlock);
    }
}</pre>

# Benchmarking

<p>Benchmarking the allocator performance vs. the global heap on a Windows PC shows just how fast the code&nbsp;is. A&nbsp;basic test of allocating and deallocating 20000 4096 and 2048 sized blocks in a somewhat interleaved fashion&nbsp;tests the speed improvement. See the attached source code for the exact algorithm.&nbsp;</p>

<p><strong>Windows Allocation Times in Milliseconds</strong></p>

<table class="ArticleTable">
	<thead>
		<tr>
			<td>Allocator</td>
			<td>Mode</td>
			<td>Run</td>
			<td>Benchmark Time (mS)</td>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>Global Heap</td>
			<td>Release</td>
			<td>1</td>
			<td>36.3</td>
		</tr>
		<tr>
			<td>Global Heap</td>
			<td>Release&nbsp;</td>
			<td>2</td>
			<td>33.8</td>
		</tr>
		<tr>
			<td>Global Heap</td>
			<td>Release&nbsp;</td>
			<td>3</td>
			<td>32.8</td>
		</tr>
		<tr>
			<td>fb_allocator</td>
			<td>Static Pool</td>
			<td>1</td>
			<td>22.6</td>
		</tr>
		<tr>
			<td>fb_allocator</td>
			<td>Static Pool</td>
			<td>2</td>
			<td>3.7</td>
		</tr>
		<tr>
			<td>fb_allocator</td>
			<td>Static Pool</td>
			<td>3</td>
			<td>4.9</td>
		</tr>
		<tr>
			<td>x_allocator</td>
			<td>Static Pool</td>
			<td>1</td>
			<td>33.9</td>
		</tr>
		<tr>
			<td>x_allocator</td>
			<td>Static Pool</td>
			<td>2</td>
			<td>6.9</td>
		</tr>
		<tr>
			<td>x_allocator</td>
			<td>Static Pool</td>
			<td>3</td>
			<td>7.7</td>
		</tr>
	</tbody>
</table>

<p>Windows uses a debug heap when executing within the debugger. The debug heap adds extra safety checks slowing its performance. The release heap is much faster as the checks are disabled. The debug heap can be disabled within&nbsp;Visual Studio by setting&nbsp;<strong>_NO_DEBUG_HEAP=1</strong> in the <strong>Debugging &gt; Environment&nbsp;</strong>project option.&nbsp;</p>

<p>This benchmark&nbsp;test is very simplistic and a more realistic scenario with varying blocks sizes and random new/delete intervals might produce different results. However, the basic point is illustrated nicely; the memory manager is slower than allocator and highly dependent on the platform&#39;s implementation.</p>

<p>The <code>fb_allocator </code>uses a static memory pool and doesn&#39;t rely upon the heap. This has a fast&nbsp;execution time of around 4mS once the free-list is populated with blocks. The 22.6mS on fb_allocator Run 1&nbsp;accounts for dicing up the fixed memory pool into individual blocks on the first run.</p>

<p>The <code>x_allocator </code>used within the bm_allocator module is a bit slower at ~7mS since it has overhead to allocate/deallocate&nbsp;mutiple sized blocks wherease <code>fb_allocator</code> only supports one block size.&nbsp;</p>

<p>In comparison to the Windows global heap, the <code>fb_allocator </code>is about 8 times faster and <code>x_allocator </code>is about 5 times faster. On embedded devices, I&#39;ve seen as high as a 15x speed increase over the global heap.&nbsp;</p>

# Thread Safety

<p>The <code>LK_LOCK </code>and <code>LK_UNLOCK </code>macros within the <code>LockGuard</code> module implement the software locks needed for thread safety. Update the lock implementation as required for your platforms operating system.</p>

# Reference Articles

<ul>
	<li><a href="https://github.com/endurodave/Allocator">An Efficient C++ Fixed Block Memory Allocator</a> - by David Lafreniere</li>
	<li><a href="https://github.com/endurodave/xallocator">Replace malloc/free with a Fast Fixed Block Memory Allocator</a> - by David Lafreniere</li>
	<li><a href="https://github.com/endurodave/stl_allocator">A Custom STL std::allocator Replacement Improves Performance</a> - by David Lafreniere</li>
</ul>

# Conclusion

<p>The C-based fixed block memory allocator presented here is suitable for any C or C++ system. For a C++ specific implementation with its own unique features, see the referenced articles.</p>

<p>Use the <code>fb_allocator</code> whenever you need to allocate a single block size. Use <code>x_allocator</code> when dispensing multiple block sizes is desired. Create multiple <code>x_allocator</code> wrappers to segregate memory pools depending on intended usage.</p>

<p>If you have an application that really hammers the heap and is causing slow performance, or if you&rsquo;re worried about a fragmented heap fault, integrating <code>fb_allocator</code> and <code>x_allocator</code> may help solve those problems. The implementation was kept to a minimum facilitating use on even the smallest of embedded systems.</p>


