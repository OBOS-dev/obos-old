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
#include "multitasking.h"

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
                int pageIndex = (address >> 12) % 1024;
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
                // address / 4096 % 1024
                int pageIndex = (address >> 12) % 1024;
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
        BOOL hasUserModePages = FALSE;
        for (int j = 0; j < 1024 && !hasAllocatedPages; j++)
        {
            // Is the page present?
            if (getBitFromBitfield(g_pageTable[i][j], 0))
                hasAllocatedPages = TRUE; // If so, set 'hasAllocatedPages' to true.
            // Is the page supervisor?
            if (getBitFromBitfield(g_pageTable[i][j], 2))
                hasUserModePages = TRUE; // If not, set 'hasUserModePages' to true
            // For performance reasons, exit if both 'hasAllocatedPages' and 'hasUserModePages' are true, exit the loop.
            if (hasUserModePages && hasAllocatedPages)
                break;
        }
        if (hasAllocatedPages)
            g_pageDirectory[i] = ((UINTPTR_T)g_pageTable[i]) | 3 | (hasUserModePages << 2);
        else
            g_pageDirectory[i] = 2;
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
    if (!block)
        return NULL;
    PCHAR _block = block;
    for (int i = 0; i < size; _block[i++] = value);
    return block;
}

// The end of .rodata
extern char __erodata;

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
        g_pageDirectory[i] = ((UINTPTR_T)&g_pageTable[i]) | 3;
        
        BOOL isAvailableOne = FALSE;
        BOOL isAvailableTwo = FALSE;

        for (int i2 = 0; i2 < g_kMemoryTableSize; i2++)
        {
            if (inRange((UINTPTR_T)g_kMemoryTable[i2].start, (UINTPTR_T)g_kMemoryTable[i2].end, ((0 * 1024 + i) * 4096)))
                isAvailableOne = TRUE;
            if (inRange((UINTPTR_T)g_kMemoryTable[i2].start, (UINTPTR_T)g_kMemoryTable[i2].end, ((1 * 1024 + i) * 4096)))
                isAvailableTwo = TRUE;
            if (isAvailableOne && isAvailableTwo)
                break;
        }

        // TODO: And an else condition that finds a 4096-byte block of memory at a physical address that isn't being used.

        if(isAvailableOne && ((0 * 1024 + i) * 4096) > 0x5000)
        {
            UINTPTR_T address = ((0 * 1024 + i) * 4096);
            if(inRange((UINTPTR_T)0x100000, (UINTPTR_T)&__erodata, address))
                g_pageTable[0][i] = address | 1;
            else
                g_pageTable[0][i] = address | 3;
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
            if (inRange((UINTPTR_T)0x100000, (UINTPTR_T)&__erodata, address))
                g_pageTable[1][i] = address | 1;
            else
                g_pageTable[1][i] = address | 3;
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
    /*g_pageDirectory[0] = ((UINTPTR_T)g_pageTable[0]) | 3;
    g_pageDirectory[1] = ((UINTPTR_T)g_pageTable[1]) | 3;
    g_pageDirectory[2] = ((UINTPTR_T)g_pageTable[2]) | 3;*/

    loadPageDirectory(&g_pageDirectory[0]);
    enablePaging();

    s_liballocMutex = MakeMutex();

    extern void OBOS_ThreadEntryPoint();
    // Allocate the thread entry point at 0x800000.
    kalloc_pagesAt((PVOID)0x800000, 1, TRUE, TRUE);
    PBYTE threadEntryPoint = (PBYTE)0x800000;
    PBYTE realThreadEntryPoint = (PBYTE)OBOS_ThreadEntryPoint;
    // c3 is ret instruction.
    for (i = 0; realThreadEntryPoint[i] != 0xc3; i++)
        threadEntryPoint[i] = realThreadEntryPoint[i];
    threadEntryPoint[i] = 0xc3;
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

static int cachedPageTableIndex = 2;
static int cachedPageTable = 1;
void* kalloc_pages(SIZE_T nPages, BOOL isReadOnly, BOOL canBeAccessedUserMode)
{
    // TODO: Change g_pageTable[pageTableIndex][(index)] to use the page directory instead to access page tables.
    int tries = 0;
    int x = cachedPageTableIndex;
    int y = cachedPageTable;
    for(; tries < 6; tries++)
    {
        UINTPTR_T startAddress = 0;
        for (; x < 1024 && startAddress == 0; x++)
        {
            for (; y < 1024; y++)
            {
                if (g_pageTable[x][y] == 2)
                {
                    // startAddress = x * 4194304 + y * 4096
                    startAddress = (x << 22) + (y << 12);
                    break;
                }
                else
                    continue;
            }
        }
        BOOL foundEnoughSpace = TRUE;
        for(UINTPTR_T vAddress = startAddress + 4096, i = 0; i < nPages; vAddress += 4096, i++)
        {
            // vAddress / 4194304
            int pageTableIndex = vAddress >> 22;
            // address / 4096 % 1024
            int pageIndex = (vAddress >> 12) % 1024;
            if (g_pageTable[pageTableIndex][pageIndex] != 2)
            {
                foundEnoughSpace = FALSE;
                break;
            }
        }
        if(!foundEnoughSpace)
            continue;
        PVOID ret = kalloc_pagesAt((PVOID)startAddress, nPages, isReadOnly, canBeAccessedUserMode);
        if(!ret)
            continue;
        return ret;
    }
    // We ran out of tries.
    SetLastError(OBOS_ERROR_NO_MEMORY);
    return NULLPTR;
}
void* kalloc_pagesAt(PVOID virtualAddress, SIZE_T nPages, BOOL isReadOnly, BOOL canBeAccessedUserMode)
{
    // Is the virtual address available?
    UINTPTR_T address = (UINTPTR_T)virtualAddress;
    for (int i = 0; i < nPages; i++, address += 4096)
    {
        // address / 4194304
        int pageTableIndex = address >> 22;
        // address / 4096 % 1024
        int pageIndex = (address >> 12) % 1024;
        if (g_pageTable[pageTableIndex][pageIndex] != 2)
        {
            // The address is in use.
            SetLastError(OBOS_ERROR_ADDRESS_NOT_AVAILABLE);
            return NULLPTR;
        }
        //// address / 4096
        //SIZE_T usedPagesIndex = address >> 12;
        //SIZE_T bit = ((address >> 12) - 32) % 32;
        //if (getBitFromBitfield(g_usedPages[usedPagesIndex], bit))
        //{
        //    SetLastError(OBOS_ERROR_ADDRESS_NOT_AVAILABLE);
        //    return NULLPTR;
        //}
    }
    // If so, find physical pages to map them to, and map the pages.
    address = (UINTPTR_T)virtualAddress;
    UINTPTR_T vAddress = address;
    UINTPTR_T physicalAddress = 0;
    UINTPTR_T physicalAddresses[nPages];
    memset(physicalAddresses, 0, sizeof(UINTPTR_T) * nPages);
    for (int i = 0; i < nPages; i++, vAddress += 4096)
    {
        //// address / 262144
        //SIZE_T usedPagesIndex = address >> 18;
        //// (address / 4096) % 32
        //SIZE_T bit = (address >> 12) % 32;
        for(int i2 = 96; i2 < BITFIELD_SIZE; i2++)
        {
            for (int j = 0; j < 32; j++)
            {
                if (!getBitFromBitfield(g_usedPages[i2], j))
                {
                    // i * 262144 + j * 4096
                    // TODO: Add a for loop to check all the new allocated pages before this one.
                    /*if (address == ((i << 18) + (j << 12)))
                        continue;*/
                    BOOL continueOut = FALSE;
                    for(int x = 0; x <= i; x++)
                        if(physicalAddresses[x] == (i2 << 18) + (j << 12))
                        {
                            continueOut = TRUE;
                            break;
                        }
                    if (continueOut)
                        continue;
                    physicalAddresses[i] = address = physicalAddress = (i2 << 18) + (j << 12);
                    goto foundPhysicalAddress;
                }
            }
        }
        foundPhysicalAddress:
        if (!physicalAddress || physicalAddress > g_kAvailablePhysicalMemorySize)
        {
            SetLastError(OBOS_ERROR_NO_MEMORY);
            return NULLPTR;
        }
        // vAddress / 4194304
        int pageTableIndex = vAddress >> 22;
        // address / 4096 % 1024
        int pageIndex = (vAddress >> 12) % 1024;
        g_pageTable[pageTableIndex][pageIndex] = physicalAddress | (1 | ((!isReadOnly) << 1) | (canBeAccessedUserMode << 2));
        SIZE_T usedPagesIndex = address >> 18;
        SIZE_T bit = ((address >> 12) - 32) % 32;
        setBitInBitfield(&g_usedPages[usedPagesIndex], bit);
        physicalAddress = 0;
    }
    updatePageDirectory();
    return (PVOID)virtualAddress;
}
int kfree_pages(PVOID start, SIZE_T nPages)
{
    if ((UINTPTR_T)start % 4096 != 0)
        start -= ((UINTPTR_T)start % 4096);
    UINTPTR_T block = (UINTPTR_T)start;
    for(UINTPTR_T i2 = 0, cblock = block; i2 < nPages; i2++, cblock += 4096)
    {
        // cblock / 4194304
        int pageTableIndex = cblock >> 22;
        // cblock / 4096 % 1024
        int pageIndex = (cblock >> 12) % 1024;
        if (getBitFromBitfield(g_pageTable[pageTableIndex][pageIndex], 0))
            return -1;
        
    }
    memset(start, 0, nPages * 4096 - 1);
    for (int index = 0; index < nPages; index++)
    {
        UINTPTR_T address = block + index * 4096;
        // address / 4194304
        int pageTableIndex = address >> 22;
        // address / 4096 % 1024
        int pageIndex = (address >> 12) % 1024;
        SIZE_T usedPagesIndex = g_pageTable[pageTableIndex][pageIndex] >> 18;
        SIZE_T bit = ((g_pageTable[pageTableIndex][pageIndex] >> 12) - 32) % 32;
        setBitInBitfield(&g_usedPages[usedPagesIndex], bit);
        g_pageTable[pageTableIndex][pageIndex] = 2;
    }
    cachedPageTableIndex = block >> 22;
    updatePageDirectory();
    return 0;
}

void ksetPagesProtection(PVOID start, SIZE_T nPages, BOOL isReadOnly)
{
    for (int i = 0; i < nPages; i++)
    {
        UINTPTR_T address = (UINTPTR_T)start + i * 4096;
        // address / 4194304
        int pageTableIndex = address >> 22;
        // address / 4096 % 1024
        int pageIndex = (address >> 12) % 1024;
        if (g_pageTable[pageTableIndex][pageIndex] == 2)
            return;
    }
    for(int i = 0; i < nPages; i++)
    {
        UINTPTR_T address = (UINTPTR_T)start + i * 4096;
        // address / 4194304
        int pageTableIndex = address >> 22;
        // address / 4096 % 1024
        int pageIndex = (address >> 12) % 1024;
        BOOL isSupervisorPage = getBitFromBitfield(g_pageTable[pageTableIndex][pageIndex], 2);
        UINTPTR_T physicalAddress = g_pageTable[pageTableIndex][pageIndex] & 0x7FFFF000;
        g_pageTable[pageTableIndex][pageIndex] = physicalAddress | 1 | ((!isReadOnly) << 1) | (isSupervisorPage << 2);
    }
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
    PVOID block = kalloc_pages(nPages, FALSE, FALSE);
    if (block)
        memset(block, 0, nPages * 4096);
    else
        if (GetTid() != 0)
            ExitThread((DWORD)(-((INT)OBOS_ERROR_NO_MEMORY)));
        else
            kpanic(NULLPTR, 0);
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