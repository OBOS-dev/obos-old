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

using namespace obos;

#ifdef __GNUC__
#define DEFINE_IN_SECTION __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME)))
#else
#define DEFINE_IN_SECTION
#endif

bool RegisterMBRPartitionsOnDrive(uint32_t driveId, size_t* oNPartitions, driverInterface::partitionInfo** oPartInfo);

driverInterface::driverHeader DEFINE_IN_SECTION g_driverHeader = {
	.magicNumber = driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 3, // The kernel __depends__ on this being three, do not change.
	.driverType = driverInterface::OBOS_SERVICE_TYPE_PARTITION_MANAGER,
	.requests = driverInterface::driverHeader::REQUEST_NO_MAIN_THREAD,
	.functionTable = {
		.GetServiceType = []()->driverInterface::serviceType { return driverInterface::serviceType::OBOS_SERVICE_TYPE_STORAGE_DEVICE; },
		.serviceSpecific = {
			.partitionManager = {
                .RegisterPartitionsOnDrive = RegisterMBRPartitionsOnDrive,
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
};
struct gpt_partition
{
    char partGUID[16];
    char uniquePartGUID[16];
    uint64_t startLBA;
    uint64_t endLBA;
    uint64_t attrib;
    char partitionName[72];
    // From here to szPartitionEntry is reserved.
};

uint32_t crc32_bytes(void* data, size_t sz);
#if defined(__x86_64__)
asm (
    ".intel_syntax noprefix\n"
    "crc32_bytes:\n"
    "   push rbp\n"
    "   mov rbp, rsp\n"
    "   mov rcx, rsi\n"
    "   cmp rcx,rcx\n"
    "   jz crc32_bytesDone\n"
    "   crc32_bytesLoop:\n"
    "       crc32 rax, [rdi]\n"
    "       inc rdi\n"
    "       loop crc32_bytesLoop\n"
    "   crc32_bytesDone:\n"
    "   leave\n"
    "   ret\n"
    ".att_syntax prefix\n"
);
#elif defined(__i386__)
asm (
    ".intel_syntax noprefix\n"
    "crc32_bytes:\n"
    "   push ebp\n"
    "   mov ebp, esp\n"
    "   mov ecx, [ebp+0x10]\n"
    "   cmp ecx,ecx\n"
    "   jz crc32_bytesDone\n"
    "   crc32_bytesLoop:\n"
    "       crc32 eax, [edi]\n"
    "       inc edi\n"
    "       loop crc32_bytesLoop\n"
    "   crc32_bytesDone:\n"
    "   leave\n"
    "   ret\n"
    ".att_syntax prefix\n"
);
#endif

bool RegisterMBRPartitionsOnDrive(uint32_t driveId, size_t* oNPartitions, driverInterface::partitionInfo** oPartInfo)
{
    char* path = new char[logger::sprintf(nullptr, "D%d:/", driveId) + 1];
    logger::sprintf(path, "D%d:/", driveId);
    vfs::DriveHandle drive;
    drive.OpenDrive(path, vfs::DriveHandle::OpenOptions::OPTIONS_READ_ONLY);
    delete[] path;
    size_t sizeofSector;
    drive.QueryInfo(nullptr, &sizeofSector);
    byte* firstSector = new byte[sizeofSector];
    if (!drive.ReadSectors(firstSector, nullptr, 1, 1))
    {
        delete[] firstSector;
        drive.Close();
        return false;
    }
    gpt_header* gptHeader = (gpt_header*)firstSector;
    if (gptHeader->signature != GPT_HEADER_SIGNATURE)
    {
        delete[] firstSector;
        drive.Close();
        return false;
    }
    uint32_t header_crc32 = crc32_bytes(gptHeader, sizeof(*gptHeader));
    if (header_crc32 != gptHeader->headerCRC32)
    {
        delete[] firstSector;
        drive.Close();
        return false;
    }
    return true;
}

void _start()
{}