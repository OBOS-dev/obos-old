/*
    paging.c

    Copyright (c) 2023 Omar Berrow
*/

#include "paging.h"
#include "klog.h"
#include "terminal.h"
#include "types.h"
#include "inline-asm.h"
#include "bitfields.h"
#include "interrupts.h"
#include "error.h"
#include "mutexes.h"

#include "multiboot.h"

#include <stddef.h>

extern multiboot_info_t* g_multibootInfo;

memory_block* g_kMemoryTable = NULLPTR;
SIZE_T g_kMemoryTableBlockSize = 0;
SIZE_T g_kMemoryTableSize = 0;
SIZE_T g_kPhysicalMemorySize = 0;
SIZE_T g_kAvailablePhysicalMemorySize = 0;
SIZE_T g_kSystemPhysicalMemorySize = 0;

#define MAX_COUNT_PAGES 1048576
#define BITFIELD_SIZE 32768
#define inRange(rStart, rEnd, num) ((BOOL)(num >= rStart && num <= rEnd))

UINT32_T __attribute__((aligned(4096))) g_pageDirectory[1024];
UINT32_T __attribute__((aligned(4096))) g_pageTable[1024][1024];
// A bit field.
static UINT32_T g_usedPages[BITFIELD_SIZE];
static HANDLE s_liballocMutex = OBOS_INVALID_HANDLE_VALUE;

//static int strlen(const char* str)
//{
//    int i = 0;
//    for (; str[i]; i++);
//    return i;
//}

// "out" should be 11 bytes + null character (12 bytes)
void itoa(INT32_T num, char* out)
{
    if (num < 10)
    {
        out[0] = num + '0';
        return;
    }
    int _strlen = 0;
    for (DWORD divisor = num, quotient = num % 10, i = 0;
        divisor != 0 && i < 11;
        divisor /= 10, quotient = divisor % 10, i++, _strlen++)
        out[i] = quotient + '0';
    int _sp = 11;
    char stack[12] = { 0 };
    for (int i = 0; i < sizeof(stack); stack[i++] = 0);

    for (int i = 0; i < _strlen; i++, _sp--)
        stack[_sp] = out[i];
    for (int i = 0; i < _strlen + 1; i++, _sp++)
        out[i - 1] = stack[_sp];
    out[_strlen] = 0;
}

static void updatePageTable()
{
    disablePICInterrupt(0);
    for (int i = 96; i < BITFIELD_SIZE; i++)
        for (unsigned int j = 0, address = ((i / 32 * 1024 + j + i * 32) * 4096); j < 32; j++, address = ((i / 32 * 1024 + j + i * 32) * 4096))
            if (getBitFromBitfield(g_usedPages[i], j))
            {
                int pageTableIndex = address / 4194304;
                int pageIndex = (address - (address / 4194304 * 4194304)) / 4096;
                UINT32_T* pageTableEntry = &g_pageTable[pageTableIndex][pageIndex];
                address = (pageTableIndex * 1024 + pageIndex) * 4096;
                BOOL isMemoryReserved = FALSE;
                for (int i2 = 0; i2 < g_kMemoryTableSize && !isMemoryReserved; i2++)
                    isMemoryReserved = inRange((UINTPTR_T)g_kMemoryTable[i2].start, (UINTPTR_T)g_kMemoryTable[i2].end, address);
                if (!isMemoryReserved)
                    *pageTableEntry = address | 3;
                else
                    *pageTableEntry = 2;
            }
            else
            {
                int pageTableIndex = address / 4194304;
                int pageIndex = (address - (address / 4194304 * 4194304)) / 4096;
                UINT32_T* pageTableEntry = &g_pageTable[pageTableIndex][pageIndex];
                *pageTableEntry = 2;
            }
    enablePICInterrupt(0);
}
static void updatePageDirectory()
{
    disablePICInterrupt(0);
    for (int i = 0; i < 1024; i++)
    {
        BOOL hasAllocatedPages = FALSE;
        for (int j = 0; j < 1024 && !hasAllocatedPages; j++)
            if (g_pageTable[i][j] != 3)
                hasAllocatedPages = TRUE;
        if (hasAllocatedPages)
            g_pageDirectory[i] = ((UINTPTR_T)g_pageTable[i]) | 0x00000003;
        else
            g_pageDirectory[i] = 0x00000002;
    }
    enablePICInterrupt(0);
}

void kmeminit()
{
    // Allocate the memory for the table
    const multiboot_memory_map_t* table = (PVOID)g_multibootInfo->mmap_addr;
    const multiboot_memory_map_t* mmap = NULLPTR;
    int tableSize = 0;
    for(int i = 0; i < g_multibootInfo->mmap_length / sizeof(multiboot_memory_map_t); i++)
    {
        mmap = &table[i];
        // The memory is available only if it doesn't fall in the stack and mmap->type is MULTIBOOT_MEMORY_AVAILABLE and the address is within a 32-bit address space.
        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->addr <= 0xFFFFFFFF)
            tableSize++;
        g_kPhysicalMemorySize += mmap->len;
    }
    kassert(tableSize > 0, KSTR_LITERAL("No available memory in the system."));
    tableSize *= sizeof(memory_block);
    int i = 0;
    BOOL foundMemory = FALSE;
    for(; i < g_multibootInfo->mmap_length / sizeof(multiboot_memory_map_t); i++)
    {
        mmap = &table[i];
        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->len >= tableSize && mmap->addr <= 0xFFFFFFFF)
        {
            g_kMemoryTable = (memory_block*)((UINTPTR_T)mmap->addr);
            g_kMemoryTableBlockSize = mmap->len;
            foundMemory = TRUE;
            break;
        }
    }
    kassert(foundMemory, KSTR_LITERAL("Could not find a block of memory for the table.\r\n"));
    // Make the table.
    int i2 = 0;
    for(int i3 = 0; i3 < g_multibootInfo->mmap_length / sizeof(multiboot_memory_map_t); i3++)
    {
        mmap = &table[i3];
        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->addr <= 0xFFFFFFFF)
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
            g_kMemoryTable[i2].start = (memory_block*)mmap->addr;
            g_kMemoryTable[i2].end = (memory_block*)(mmap->addr + mmap->len);
#pragma GCC diagnostic pop
            g_kMemoryTable[i2].size = mmap->len;
            g_kMemoryTable[i2].isInUse = i == i3;
            g_kAvailablePhysicalMemorySize += mmap->len;
            g_kMemoryTableSize++;
            i2++;
        }
        else
            g_kSystemPhysicalMemorySize += mmap->len;
    }
}
PVOID kfindmemblock(SIZE_T size, SIZE_T* real_size)
{
    if (!g_kMemoryTableSize)
        return (PVOID)0xFFFFFFFF;
    void* ret = (void*)0xFFFFFFFF;
    if(size == -1)
    {
        //register char* sp asm("sp");
        struct size
        {
            SIZE_T len;
            SIZE_T index;
        };
        struct size sizes[g_kMemoryTableSize];
        for(int i = 0; i < g_kMemoryTableSize; i++)
        {
            memory_block* block = &g_kMemoryTable[i];
            if(!block->isInUse)
            {
                struct size temp;
                temp.len = block->size;
                temp.index = i;
                sizes[i] = temp;
                break;
            }
        }
        struct size* biggestSize = &sizes[0];
        // Store a pointer to the biggest element in biggestSize.
        for(int i = 1; i < g_kMemoryTableSize - 1; i++)
            if(biggestSize->len < sizes[i].len)
                biggestSize = &sizes[i];
        TerminalOutputString("\r\n");
        outb(0x3F8, '\r');
        outb(0x3F8, '\n');
        outb(0x2F8, '\r');
        outb(0x2F8, '\n');
        SIZE_T biggestBlockIndex = biggestSize->index;
        memory_block* block = g_kMemoryTable + biggestBlockIndex;
        ret = (PVOID)block->start;
        block->isInUse = TRUE;
        *real_size = block->size;
        return ret;
    }
    for(int i = 0; i < g_kMemoryTableSize; i++)
    {
        memory_block* block = &g_kMemoryTable[i];
        if(block->size >= size && !block->isInUse)
        {
            if (real_size)
                *real_size = block->size;
            ret = (PVOID)block->start;
            block->isInUse = TRUE;
            break;
        }
    }
    return ret;
}

extern void loadPageDirectory(UINT32_T* address);
// A page directory MUST be loaded before calling, otherwise, bad stuff could happen.
extern void enablePaging();

PVOID memset(PVOID block, BYTE value, SIZE_T size)
{
    PCHAR _block = block;
    for (int i = 0; i < size; _block[i++] = value);
    return block;
}

// The end of .text
extern char __etext;

void kInitializePaging()
{
    memset(g_usedPages, 0, BITFIELD_SIZE * sizeof(int));

    for (int i = 0; i < 1024; i++)
    {
        for (int j = 0; j < 1024; j++)
            g_pageTable[i][j] = 0x00000002;
        g_pageDirectory[i] = 0x00000002;
    }

    // TODO: Map 16384 pages (65536 kib, 64 mib) at 0xFBFFFFFF (page directory: 1008-1024) to the initrd's address space.

    SIZE_T i;

    for (i = 0; i < 1024; i++)
    {
        // Supervisor level, read-only, not present.
        g_pageDirectory[i] = 0x00000002;
        
        BOOL isAvailableOne = FALSE;
        BOOL isAvailableTwo = FALSE;
        BOOL isAvailableThree = FALSE;

        for (int i2 = 0; i2 < g_kMemoryTableSize; i2++)
        {
            if (inRange((UINTPTR_T)g_kMemoryTable[i2].start, (UINTPTR_T)g_kMemoryTable[i2].end, ((0 * 1024 + i) * 4096)))
                isAvailableOne = TRUE;
            if (inRange((UINTPTR_T)g_kMemoryTable[i2].start, (UINTPTR_T)g_kMemoryTable[i2].end, ((1 * 1024 + i) * 4096)))
                isAvailableTwo = TRUE;
            if (inRange((UINTPTR_T)g_kMemoryTable[i2].start, (UINTPTR_T)g_kMemoryTable[i2].end, ((2 * 1024 + i) * 4096)))
                isAvailableThree = TRUE;
            if (isAvailableOne && isAvailableTwo && isAvailableThree)
                break;
        }

        // TODO: And an else condition that finds a 4096-byte block of memory at a physical address that isn't being used.

        if(isAvailableOne)
        {
            UINTPTR_T address = ((0 * 1024 + i) * 4096);
            if(inRange((UINTPTR_T)0x100000, (UINTPTR_T)&__etext, address))
                g_pageTable[0][i] = address | 1;
            else
                g_pageTable[0][i] = address | 3;
            setBitInBitfield(&g_usedPages[i / 32], i % 32);
        }
        else
            setBitInBitfield(&g_usedPages[i / 32], i % 32); // If the page is reserved, mark it as allocated so it doesn't get allocated.
        /*else
        {
            UINTPTR_T foundAddress1 = 0;
            for (int i2 = 0; i2 < g_kMemoryTableSize; i2++);
            g_pageTable[0][i] = foundAddress1 | 3;
            setBitInBitfield(&g_usedPages[i / 32], i % 32);
        }*/
        if(isAvailableTwo)
        {
            UINTPTR_T address = ((1 * 1024 + i) * 4096);
            if (inRange((UINTPTR_T)0x100000, (UINTPTR_T)&__etext, address))
                g_pageTable[1][i] = address | 1;
            else
                g_pageTable[1][i] = address | 3;
            setBitInBitfield(&g_usedPages[i / 32 + 32], i % 32);
        }
        else
            setBitInBitfield(&g_usedPages[i / 32], i % 32); // If the page is reserved, mark it as allocated so it doesn't get allocated.
        if(isAvailableThree)
        {
            UINTPTR_T address = ((2 * 1024 + i) * 4096);
            if (inRange((UINTPTR_T)0x100000, (UINTPTR_T)&__etext, address))
                g_pageTable[2][i] = address | 1;
            else
                g_pageTable[2][i] = address | 3;
            setBitInBitfield(&g_usedPages[i / 32 + 64], i % 32);
        }
        else
            setBitInBitfield(&g_usedPages[i / 32], i % 32); // If the page is reserved, mark it as allocated so it doesn't get allocated.
    }

    // Allocate two pages for the video memory.
    g_pageTable[0][184] = 0xB8003;
    g_pageTable[0][185] = 0xB9003;
    setBitInBitfield(&g_usedPages[5], 24);
    setBitInBitfield(&g_usedPages[5], 25);

    // Supervisor level, read/write, present.
    g_pageDirectory[0] = ((UINTPTR_T)g_pageTable[0]) | 3;
    g_pageDirectory[1] = ((UINTPTR_T)g_pageTable[1]) | 3;
    g_pageDirectory[2] = ((UINTPTR_T)g_pageTable[2]) | 3;

    s_liballocMutex = MakeMutex();

    loadPageDirectory(&g_pageDirectory[0]);
    enablePaging();
}

/** This function is supposed to lock the memory data structures. It
 * could be as simple as disabling interrupts or acquiring a spinlock.
 * It's up to you to decide.
 *
 * \return 0 if the lock was acquired successfully. Anything else is
 * failure.
 */
int liballoc_lock() 
{
    SetLastError(0);
    if (!LockMutex(s_liballocMutex))
        return GetLastError();
    cli();
    return 0;
}

/** This function unlocks what was previously locked by the liballoc_lock
 * function.  If it disabled interrupts, it enables interrupts. If it
 * had acquiried a spinlock, it releases the spinlock. etc.
 *
 * \return 0 if the lock was successfully released.
 */
int liballoc_unlock()
{
    SetLastError(0);
    if (!UnlockMutex(s_liballocMutex))
        return GetLastError();
    sti();
    return 0;
}

void* kalloc_pages(SIZE_T nPages)
{
    int tries = 0;
    unsigned short usedPagesIndex = 0;
    for(; tries < 6; tries++)
    {
        for (; usedPagesIndex < BITFIELD_SIZE; usedPagesIndex++)
        {
            if (g_usedPages[usedPagesIndex] != 0xFFFFFFFF)
                break;
        }
        if (usedPagesIndex == BITFIELD_SIZE - 1 && g_usedPages[usedPagesIndex] == 0xFFFFFFFF)
            break;
        UINT32_T* usedPageBitfield = &g_usedPages[usedPagesIndex];
        // Count available pages in *usedPageBitfield and check if it has enough space to store nPages continuously. If not, check
        // there is enough space to get to the end and continue the allocation in g_usedPages[usedPagesIndex + i].
        // If all that fails, find a new place to store the pages.
        BOOL foundEnoughSpace = FALSE;
        int amountIncreased = 0;
        int i;
        for (i = 0; i < 32 && !foundEnoughSpace; i++)
        {
            if (!getBitFromBitfield(*usedPageBitfield, i))
            {
                int i2 = i;
                BOOL foundEmptyPages = TRUE;
                for (int j = 0; j < nPages; j++, i2++)
                {
                    if (i2 == 31)
                    {
                        amountIncreased++;
                        i2 = 0;
                        usedPageBitfield = &g_usedPages[usedPagesIndex++];
                    }
                    foundEmptyPages = foundEmptyPages && !getBitFromBitfield(*usedPageBitfield, i2);
                    if (!foundEmptyPages)
                        break;
                }
                foundEnoughSpace = foundEmptyPages;
            }
        }
        if (!foundEnoughSpace)
            continue;
        UINTPTR_T startAddress = ((usedPagesIndex / 32 * 1024 + i + usedPagesIndex * 32) * 4096);
        // There is no more physical memory, so exit the loop
        if (startAddress > g_kPhysicalMemorySize)
            break;
        usedPageBitfield = &g_usedPages[usedPagesIndex -= amountIncreased];
        setBitInBitfield(usedPageBitfield, i);
        for (int x = i, currentPage = 0; currentPage < nPages; x++, currentPage++)
        {
            if (x == 31)
            {
                x = 0;
                usedPageBitfield = &g_usedPages[++usedPagesIndex];
            }
            setBitInBitfield(usedPageBitfield, x);
        }
        BOOL isAvailable = FALSE;
        for (int index = 0; index < nPages; index++)
        {
            UINTPTR_T address = startAddress + index * 4096;
            for (int j = 0; j < g_kMemoryTableSize && !isAvailable; j++)
                isAvailable = inRange((UINTPTR_T)g_kMemoryTable[j].start, (UINTPTR_T)g_kMemoryTable[j].end, address);
            if (!isAvailable)
                break;
            int pageTableIndex = address / 4194304;
            int pageIndex = (address - (address / 4194304 * 4194304)) / 4096;
            g_pageTable[pageTableIndex][pageIndex] = address | 3;
        }
        if (!isAvailable)
            continue;
        updatePageDirectory();
        usedPagesIndex -= amountIncreased;
        return (PVOID)startAddress;
    }
    // We ran out of tries.
    SetLastError(OBOS_ERROR_NO_MEMORY);
    return NULLPTR;
}
int kfree_pages(PVOID start, SIZE_T nPages)
{
    if ((UINTPTR_T)start % 4096 != 0)
        start -= ((UINTPTR_T)start % 4096);
    UINTPTR_T block = (UINTPTR_T)start;
    SIZE_T usedPagesIndex = block / 262144;
    SIZE_T i = (block / 4096 - usedPagesIndex * 32) % 32;
    UINT32_T* usedPageBitfield = &g_usedPages[usedPagesIndex];
    {
        int pageTableIndex = block / 4194304;
        int pageIndex = (block - (block / 4194304 * 4194304)) / 4096;
        if ((g_pageTable[pageTableIndex][pageIndex] & 3) != 3)
            return -1;
    }
    memset(start, 0, nPages * 4096 - 1);
    for (int x = i; x - i < nPages; x++)
    {
        if (x == 31)
        {
            x = 0;
            usedPageBitfield = &g_usedPages[++usedPagesIndex];
        }
        clearBitInBitfield(usedPageBitfield, i);
    }
    for (int index = 0; index < nPages; index++)
    {
        UINTPTR_T address = block + index * 4096;
        int pageTableIndex = address / 4194304;
        int pageIndex = (address - (address / 4194304 * 4194304)) / 4096;
        g_pageTable[pageTableIndex][pageIndex] = 2;
    }
    updatePageDirectory();
    return 0;
}

void reloadPages()
{
    updatePageTable();
    updatePageDirectory();
}

/** This is the hook into the local system which allocates pages. It
 * accepts an integer parameter which is the number of pages
 * required.  The page size was set up in the liballoc_init function.
 *
 * \return NULL if the pages were not allocated.
 * \return A pointer to the allocated memory.
 */
void* liballoc_alloc(size_t nPages)
{
    PVOID block = kalloc_pages(nPages);
    memset(block, 0, nPages * 4096);
    return block;
}

/** This frees previously allocated memory. The void* parameter passed
 * to the function is the exact same value returned from a previous
 * liballoc_alloc call.
 *
 * The integer value is the number of pages to free.
 *
 * \return 0 if the memory was successfully freed.
 */
int liballoc_free(void* block, size_t nPages)
{
    return kfree_pages(block, nPages);
}