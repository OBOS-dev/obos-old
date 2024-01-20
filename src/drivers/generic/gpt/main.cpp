/*
    drivers/generic/gpt/main.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <vfs/devManip/driveHandle.h>

#include <driverInterface/struct.h>

#include <allocators/liballoc.h>
#include <allocators/vmm/vmm.h>

using namespace obos;

#ifdef __GNUC__
#define DEFINE_IN_SECTION __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME)))
#else
#define DEFINE_IN_SECTION
#endif

bool RegisterGPTPartitionsOnDrive(uint32_t driveId, size_t* oNPartitions, driverInterface::partitionInfo** oPartInfo);

driverInterface::driverHeader DEFINE_IN_SECTION g_driverHeader = {
	.magicNumber = driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 3, // The kernel __depends__ on this being three, do not change.
	.driverType = driverInterface::OBOS_SERVICE_TYPE_PARTITION_MANAGER,
	.requests = driverInterface::driverHeader::REQUEST_NO_MAIN_THREAD,
	.functionTable = {
		.GetServiceType = []()->driverInterface::serviceType { return driverInterface::serviceType::OBOS_SERVICE_TYPE_STORAGE_DEVICE; },
		.serviceSpecific = {
			.partitionManager = {
                .RegisterPartitionsOnDrive = RegisterGPTPartitionsOnDrive,
                .unused = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,}
            }
		}
	}
};

// "EFI PART"
#define GPT_HEADER_SIGNATURE 0x5452415020494645

struct gpt_header
{
    uint64_t signature;
    uint32_t revision; // Should be 0x00010000 (Version 1)
    uint32_t sizeofHeader; // Should >= 92 and <= sectorSize
    uint32_t headerCRC32;
    uint32_t resv1;
    uint64_t myLBA;
    uint64_t alternateLBA; // The LBA of the alternate GPT header.
    uint64_t firstUsableLBA;
    uint64_t lastUsableLBA;
    char diskGUID[16];
    uint64_t partitionEntryLBA;
    uint32_t nPartitionEntries;
    uint32_t szPartitionEntry; // Must be a multiple of 128.
    uint32_t partitionTableCRC32;
    // The rest of the sector is reserved.
} __attribute__((packed));
struct gpt_partition
{
    char partGUID[16];
    char uniquePartGUID[16];
    uint64_t startLBA;
    uint64_t endLBA;
    uint64_t attrib;
    char16_t partitionName[36];
    // From here to szPartitionEntry is reserved.
} __attribute__((packed));

extern "C" uint32_t crc32_bytes(void* data, size_t sz);
extern "C" uint32_t crc32_bytes_from_previous(void* data, size_t sz, uint32_t previousChecksum);
#if defined(__x86_64__) || defined(_WIN64)
#include <x86_64-utils/asm.h>
static bool _x86_64_initialized_crc32 = false;
extern "C" uint32_t crc32_bytes_from_previous_accelerated(void* data, size_t sz, uint32_t previousChecksum);
uint32_t crctab[256];
void crcInit()
{
    uint32_t crc = 0;
    for (uint16_t i = 0; i < 256; ++i)
    {
        crc = i;
        for (uint8_t j = 0; j < 8; ++j)
        {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        crctab[i] = crc;
    }
}
uint32_t crc(const char* data, size_t len, uint32_t result = ~0U)
{
    for (size_t i = 0; i < len; ++i)
        result = (result >> 8) ^ crctab[(result ^ data[i]) & 0xFF];
    return ~result;
}
extern "C" uint32_t crc32_bytes_from_previous(void* data, size_t sz, uint32_t previousChecksum)
{
    if (!_x86_64_initialized_crc32)
    {
        crcInit();
        _x86_64_initialized_crc32 = true;
    }
    return crc((char*)data, sz, ~previousChecksum);
}
extern "C" uint32_t crc32_bytes(void* data, size_t sz)
{
    if (!_x86_64_initialized_crc32)
    {
        crcInit();
        _x86_64_initialized_crc32 = true;
    }
    return crc((char*)data, sz);
}
asm(
    ".intel_syntax noprefix\n"
    "crc32_bytes_from_previous_accelerated:\n"
    "   push rbp\n"
    "   mov rbp, rsp\n"
    "   xor rax,rax\n"
    "   mov rcx, rsi\n"
    "   test rcx,rcx\n"
    "   jz crc32_bytesDone\n"
    "   mov rax,rdx\n"
    "   crc32_bytesLoop:\n"
    "       crc32 rax, byte ptr [rdi]\n"
    "       inc rdi\n"
    "       loop crc32_bytesLoop\n"
    "   crc32_bytesDone:\n"
    "   leave\n"
    "   ret\n"
    ".att_syntax prefix\n"
);
#endif

bool RegisterGPTPartitionsOnDrive(uint32_t driveId, size_t* oNPartitions, driverInterface::partitionInfo** oPartInfo)
{
    char* path = new char[logger::sprintf(nullptr, "D%d:/", driveId) + 1];
    logger::sprintf(path, "D%d:/", driveId);
    vfs::DriveHandle drive;
    drive.OpenDrive(path, vfs::DriveHandle::OpenOptions::OPTIONS_READ_ONLY);
    delete[] path;
    size_t sizeofSector;
    drive.QueryInfo(nullptr, &sizeofSector, nullptr);
    memory::VirtualAllocator allocator{ nullptr };
    byte* firstSector = (byte*)allocator.VirtualAlloc(nullptr, sizeofSector, 0);
    if (!drive.ReadSectors(firstSector, nullptr, 1, 1))
    {
        allocator.VirtualFree(firstSector, sizeofSector);
        drive.Close();
        return false;
    }
    // Verify the fact that this drive uses GPT.
    gpt_header* gptHeader = (gpt_header*)firstSector;
    if (gptHeader->signature != GPT_HEADER_SIGNATURE)
    {
        allocator.VirtualFree(firstSector, sizeofSector);
        drive.Close();
        return false;
    }
    // Verify the CRC32 checksum of the GPT header.
    bool retried = false;
    retry:
    uint32_t other_crc32 = gptHeader->headerCRC32;
    gptHeader->headerCRC32 = 0;
    uint32_t header_crc32 = crc32_bytes(gptHeader, sizeof(*gptHeader));
    gptHeader->headerCRC32 = other_crc32;
    if (header_crc32 != other_crc32)
    {
        // Oh no!
        if (retried)
        {
            // Corrupt disk...
            allocator.VirtualFree(firstSector, sizeofSector);
            drive.Close();
            return false;
        }
        // Try the alternate gpt...
        if (!drive.ReadSectors(firstSector, nullptr, gptHeader->alternateLBA, 1))
        {
            allocator.VirtualFree(firstSector, sizeofSector);
            drive.Close();
            return false;
        }
        retried = true;
        goto retry;
    }
    // Verify the CRC32 checksum of the GPT partition entries.
    const size_t nPartitionEntriesPerSector = sizeofSector / gptHeader->szPartitionEntry;
    const size_t nSectorsForPartitionTable = gptHeader->nPartitionEntries / nPartitionEntriesPerSector + (gptHeader->nPartitionEntries % nPartitionEntriesPerSector != 0);
    const uint64_t partitionTableLBA = gptHeader->partitionEntryLBA;
    byte* currentSector = (byte*)allocator.VirtualAlloc(nullptr, sizeofSector * nSectorsForPartitionTable, 0);
    uint32_t entriesCRC32 = 0;
    if (!drive.ReadSectors(currentSector, nullptr, partitionTableLBA, nSectorsForPartitionTable))
    {
        allocator.VirtualFree(firstSector, sizeofSector);
        allocator.VirtualFree(currentSector, sizeofSector);
        drive.Close();
        return false;
    }
    entriesCRC32 = crc32_bytes(currentSector, sizeofSector * nSectorsForPartitionTable);
    if (entriesCRC32 != gptHeader->partitionTableCRC32)
    {
        allocator.VirtualFree(firstSector, sizeofSector);
        allocator.VirtualFree(currentSector, sizeofSector);
        drive.Close();
        return false;
    }
    gpt_partition* const volatile table = (gpt_partition*)currentSector;
    driverInterface::partitionInfo* partitions = nullptr;
    size_t nPartitionEntries = 0;
    for (size_t i = 0, id = 0; i < nSectorsForPartitionTable; i++)
    {
        if (!drive.ReadSectors(currentSector, nullptr, partitionTableLBA + i, 1))
        {
            allocator.VirtualFree(firstSector, sizeofSector);
            allocator.VirtualFree(currentSector, sizeofSector);
            drive.Close();
            return false;
        }
        size_t nPartitionEntriesOnSector = 0;
        for (byte* iter = currentSector; iter < (currentSector + sizeofSector); iter += gptHeader->szPartitionEntry, nPartitionEntriesOnSector++)
            if (utils::memcmp(iter, (uint32_t)0, 16))
                break;
        nPartitionEntries += nPartitionEntriesOnSector;
        partitions = (driverInterface::partitionInfo*)krealloc(partitions, sizeof(driverInterface::partitionInfo) * nPartitionEntries);
        for (size_t partEntry = 0; partEntry < nPartitionEntriesOnSector; partEntry++, id++)
        {
            partitions[id].id = id;
            partitions[id].lbaOffset = table[partEntry].startLBA;
            partitions[id].sizeSectors = table[partEntry].endLBA - table[partEntry].startLBA;
        }
    }
    allocator.VirtualFree(firstSector, sizeofSector);
    allocator.VirtualFree(currentSector, sizeofSector);
    if (oNPartitions)
        *oNPartitions = nPartitionEntries;
    if (oPartInfo)
        *oPartInfo = partitions;
    return true;
}

extern "C" void _start()
{}