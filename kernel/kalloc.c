#include "kalloc.h"
#include "klog.h"
#include "types.h"
#include "terminal.h"
#include "inline-asm.h"

#include "multiboot.h"

extern multiboot_info_t* g_multibootInfo;

memory_block* g_kMemoryTable = NULLPTR;
SIZE_T g_kMemoryTableBlockSize = 0;
SIZE_T g_kMemoryTableSize = 0;
SIZE_T g_kPhysicalMemorySize = 0;

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
        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && (mmap->addr > 0x106000 || mmap->addr < 0x104000) && mmap->addr <= 0xFFFFFFFF)
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
        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && (mmap->addr > 0x106000 || mmap->addr < 0x104000) && mmap->len >= tableSize && mmap->addr <= 0xFFFFFFFF)
        {
            klog_info("Found memory block for the table.\r\n");
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
        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE && (mmap->addr > 0x106000 || mmap->addr < 0x104000) && i3 != i && mmap->addr <= 0xFFFFFFFF)
        {
            klog_info("Found valid memory block.\r\n");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
            g_kMemoryTable[i2].start = (memory_block*)mmap->addr;
            g_kMemoryTable[i2].end = (memory_block*)(mmap->addr + mmap->len);
#pragma GCC diagnostic pop
            g_kMemoryTable[i2].size = mmap->len;
            g_kMemoryTable[i2].isInUse = FALSE;
            g_kMemoryTableSize++;
            i2++;
        }
    }
    klog_info("Found all memory blocks.\r\n");
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
                klog_info("sizeof(block %d) = 0x%X\r\n", i, temp.len);
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
        klog_info("The size of the biggest block is 0x%X and its index is %d.\r\n", biggestSize->len, biggestSize->index);
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

void loadPageDirectory(UINT32_T* address);
// A page directory MUST be loaded before calling, otherwise, bad stuff could happen.
void enablePaging();

PVOID memset(PVOID block, BYTE value, SIZE_T size)
{
    PCHAR _block = block;
    for (int i = 0; i < size; _block[i++] = value);
}

#define MAX_COUNT_PAGES 1048576
#define BITFIELD_SIZE 32768

static UINT32_T s_pageDirectory[1024] __attribute__((aligned(4096)));
static UINT32_T s_pageTable[1024][1024] __attribute__((aligned(4096)));
// A bit field.
static UINT32_T g_usedPages[BITFIELD_SIZE];

void kInitializePaging()
{
    extern void* endKernel;

    memset(g_usedPages, 0, BITFIELD_SIZE * sizeof(int));

    for (int i = 0; i < 1024; i++)
    {
        for (int j = 0; j < 1024; j++)
            s_pageTable[i][j] = 0x00000002;
        s_pageDirectory[i] = ((UINT32_T)s_pageTable[i]) | 3;
    }

    SIZE_T i;

    for (i = 0; i < 1024; i++)
    {
        // Supervisor level, read/write, not present.
        s_pageDirectory[i] = 0x00000002;
        // Supervisor level, read/write, present.
        s_pageTable[0][i] = (i * 0x1000) | 3;
    }

    // Supervisor level, read/write, present.
    s_pageDirectory[0] = ((UINT32_T)s_pageTable[0]) | 3;

    memset(g_usedPages, 1, 32 * sizeof(int));

    loadPageDirectory(s_pageDirectory);
    enablePaging();
}

/** This function is supposed to lock the memory data structures. It
 * could be as simple as disabling interrupts or acquiring a spinlock.
 * It's up to you to decide.
 *
 * \return 0 if the lock was acquired successfully. Anything else is
 * failure.
 */
int liballoc_lock() { return 0; }

/** This function unlocks what was previously locked by the liballoc_lock
 * function.  If it disabled interrupts, it enables interrupts. If it
 * had acquiried a spinlock, it releases the spinlock. etc.
 *
 * \return 0 if the lock was successfully released.
 */
int liballoc_unlock() { return 0; }

void* alloc_pages(SIZE_T nPages)
{
    unsigned short usedPagesIndex = 0;
    for (; usedPagesIndex < BITFIELD_SIZE; usedPagesIndex++)
    {
        if (g_usedPages[usedPagesIndex] == 0xFFFFFFFF)
            continue;
        else
            break;
    }
    if (usedPagesIndex == BITFIELD_SIZE - 1 && g_usedPages[usedPagesIndex] == 0xFFFFFFFF)
        return NULLPTR;
    UINT32_T* usedPageBitfield = &g_usedPages[usedPagesIndex];
    // TODO: Count available pages in *usedPageBitfield and check if it has enough space to store nPages continuously. If not, check
    // there is enough space to get to the end and continue the allocation in g_usedPages[usedPagesIndex + i].
    // If all that fails, find a new place to store the pages.
    BYTE bitmask = 1;
    for (; bitmask != 0xFFFFFFFF && *usedPageBitfield & bitmask == bitmask; bitmask <<= 1);
    *usedPageBitfield | bitmask;
}

/** This is the hook into the local system which allocates pages. It
 * accepts an integer parameter which is the number of pages
 * required.  The page size was set up in the liballoc_init function.
 *
 * \return NULL if the pages were not allocated.
 * \return A pointer to the allocated memory.
 */
void* liballoc_alloc(SIZE_T nPages)
{
    return alloc_pages(nPages);
}

/** This frees previously allocated memory. The void* parameter passed
 * to the function is the exact same value returned from a previous
 * liballoc_alloc call.
 *
 * The integer value is the number of pages to free.
 *
 * \return 0 if the memory was successfully freed.
 */
int liballoc_free(void* _block, SIZE_T nPages)
{
    return 0;
}