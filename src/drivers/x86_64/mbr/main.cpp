/*
    drivers/x86_64/mbr/main.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

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
	.driverId = 2, // The kernel __depends__ on this being two, do not change.
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

struct mbr_part
{
    // Bit 7: Active partition.
    // Any other bits should be zero.
    uint8_t attribs;
    uint8_t startHead;
    uint8_t startSector;
    uint8_t startTrack;
    uint8_t osIndicator;
    uint8_t endHead;
    uint8_t endSector;
    uint8_t endTrack;
    uint32_t startingLBA;
    uint32_t endLBA;
} __attribute__((packed));
struct mbr
{
    uint8_t  bootCode[440];
    uint32_t uniqueMBRSignature;
    uint8_t  unknown[2];
    mbr_part partitions[4];
    uint16_t signature; // If != 0x55AA, invalid MBR.
} __attribute__((packed));
static_assert(sizeof(mbr) == 512, "struct mbr is not 512 bytes in length.");

bool RegisterMBRPartitionsOnDrive(uint32_t driveId, size_t* oNPartitions, driverInterface::partitionInfo** oPartInfo)
{
    char* path = new char[logger::sprintf(nullptr, "D%d:/", driveId) + 1];
    logger::sprintf(path, "D%d:/", driveId);
    vfs::DriveHandle drive;
    drive.OpenDrive(path, vfs::DriveHandle::OpenOptions::OPTIONS_READ_ONLY);
    delete[] path;
    size_t sizeofSector;
    drive.QueryInfo(nullptr, &sizeofSector, nullptr);
    byte* firstSector = new byte[sizeofSector];
    if (!drive.ReadSectors(firstSector, nullptr, 0, 1))
    {
        delete[] firstSector;
        drive.Close();
        return false;
    }
    mbr* driveMbr = (mbr*)firstSector;
    if (driveMbr->signature != 0xAA55)
        return false;
    size_t nPartitions = 0;
    driverInterface::partitionInfo* partitionInfo = nullptr;
    for (size_t part = 0; part < 4; part++)
    {
        // Active partition.
        if (driveMbr->partitions[part].attribs & 0x80)
        {
            partitionInfo = (driverInterface::partitionInfo*)krealloc(partitionInfo, ++nPartitions);
            partitionInfo[nPartitions - 1].id = nPartitions - 1;
            partitionInfo[nPartitions - 1].lbaOffset = driveMbr->partitions[part].startingLBA;
            partitionInfo[nPartitions - 1].sizeSectors = driveMbr->partitions[part].endLBA - driveMbr->partitions[part].startingLBA;
        }
    }
    if (oNPartitions)
        *oNPartitions = nPartitions;
    if (oPartInfo)
        *oPartInfo = partitionInfo;
    else
        delete[] oPartInfo;
    return true;
}

extern "C" void _start()
{}