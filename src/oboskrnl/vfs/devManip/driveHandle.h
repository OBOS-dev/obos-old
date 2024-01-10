/*
    oboskrnl/vfs/devManip/driveHandle.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

#include <vfs/fileManip/fileHandle.h>

namespace obos
{
    namespace vfs
    {
        class DriveHandle
        {
        public:
            enum OpenOptions
            {
                OPTIONS_DEFAULT = 0,
                OPTIONS_READ_ONLY = 1,
            };
            enum Flags
            {
                FLAGS_CAN_WRITE = 1,
                FLAGS_CLOSED = 2,
            };
        public:
            OBOS_EXPORT DriveHandle() = default;

            // Path format: Dd[Pp]:/
            // With 'd' being the drive id, and 'p' being the partition id (optional).
            // Everything after the initial path is ignored.
            // Examples:
            // To open drive 0, raw: D0:/
            // To open drive 0, partition 1, D0P1:/
            OBOS_EXPORT bool OpenDrive(const char* path, OpenOptions options = OPTIONS_DEFAULT);

            OBOS_EXPORT bool ReadSectors(void* buff, size_t* nSectorsRead, uoff_t lbaOffset, size_t nSectors); 
            OBOS_EXPORT bool WriteSectors(void* buff, size_t* nSectorsWritten, uoff_t lbaOffset, size_t nSectors); 
        
            OBOS_EXPORT uint32_t GetDriveId();
            OBOS_EXPORT uint32_t GetPartitionId();

            OBOS_EXPORT bool QueryInfo(size_t *nSectors, size_t *bytesPerSector);
        
            OBOS_EXPORT bool Close();

            OBOS_EXPORT ~DriveHandle() { if (m_node && !(m_flags & FLAGS_CLOSED)) Close(); }
        private:
            void* m_node = nullptr;
            void* m_handleNode = nullptr;
            void* m_driveNode = nullptr;
            void* m_driveHandleNode = nullptr;
            Flags m_flags = FLAGS_CLOSED;
        };
    }
}