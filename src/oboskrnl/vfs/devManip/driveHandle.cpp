/*
    oboskrnl/vfs/devManip/driveHandle.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>

#include <vfs/devManip/driveHandle.h>
#include <vfs/devManip/memcpy.h>

#include <vfs/vfsNode.h>

#include <driverInterface/struct.h>

#include <allocators/vmm/vmm.h>

namespace obos
{
    namespace vfs
    {
		bool strContains(const char* str, char ch);
		uint32_t getMountId(const char* path, size_t size = 0);
        bool DriveHandle::OpenDrive(const char* path, DriveHandle::OpenOptions options)
        {
            if (m_node || !(m_flags & FLAGS_CLOSED))
            {
                logger::debug("%s: m_node = %p, m_flags = %p", __func__, m_node, m_flags);
                SetLastError(OBOS_ERROR_ALREADY_EXISTS);
                return false;
            }
            if (!path)
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            if (*path != 'D')
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            bool openPartition = strContains(path, 'P');
            size_t sizeDID = openPartition ? utils::strCountToChar(path + 1, 'P') : utils::strCountToChar(path + 1, ':');
            uint32_t driveId = getMountId(path + 1, sizeDID);
            uint32_t partitionId = 0;
            if (openPartition)
                partitionId = getMountId(path + 1 + sizeDID + 1, utils::strCountToChar(path + 1 + sizeDID + 1, ':'));
            // Find the drive.
            DriveEntry* cdrive = g_drives.head;
            DriveEntry* drive = nullptr;
            while (cdrive)
            {
                if (cdrive->driveId == driveId)
                {
                    drive = cdrive;
                    break;
                }

                cdrive = cdrive->next;
            }
            if (!drive)
            {
                SetLastError(OBOS_ERROR_VFS_FILE_NOT_FOUND);
                return false;
            }
            HandleListNode* dnode = new HandleListNode;
            utils::memzero(dnode, sizeof(*dnode));
            dnode->handle = this;
            if (drive->handlesReferencing.tail)
                drive->handlesReferencing.tail->next = dnode;
            if(!drive->handlesReferencing.head)
                drive->handlesReferencing.head = dnode;
            dnode->prev = drive->handlesReferencing.tail;
            drive->handlesReferencing.tail = dnode;
            drive->handlesReferencing.size++;
            m_driveHandleNode = dnode;
            m_driveNode = drive;
            if (!openPartition)
            {
                m_node = drive;
                m_driveHandleNode = dnode;
                m_flags = (options == OPTIONS_READ_ONLY ? (Flags)0 : FLAGS_CAN_WRITE); 
                m_node = drive;
                return true;
            }
            // Look for the partition.
            PartitionEntry *cpart = drive->firstPartition, *part = nullptr;
            while (cpart)
            {
                if (cpart->partitionId == partitionId)
                {
                    part = cpart;
                    break;
                }

                cpart = cpart->next;
            }
            if (!part)
            {
                SetLastError(OBOS_ERROR_VFS_FILE_NOT_FOUND);
                return false;
            }        
            HandleListNode* pnode = new HandleListNode;
            dnode->handle = this;
            if (part->handlesReferencing.tail)
                part->handlesReferencing.tail->next = pnode;
            if(!part->handlesReferencing.head)
                part->handlesReferencing.head = pnode;
            pnode->prev = part->handlesReferencing.tail;
            part->handlesReferencing.tail = pnode;
            part->handlesReferencing.size++;
            m_flags = (options == OPTIONS_READ_ONLY ? (Flags)0 : FLAGS_CAN_WRITE); 
            m_node = part;
            m_handleNode = pnode;
            return true;
        }
        bool DriveHandle::ReadSectors(void* obuff, size_t* _nSectorsRead, uoff_t _lbaOffset, size_t nSectorsToRead) const
        {
            if (!m_node || m_flags & FLAGS_CLOSED)
            {
                SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
                return false;
            }
            if (!obuff || !nSectorsToRead)
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            driverInterface::driverIdentity* identity = ((DriveEntry*)m_driveNode)->storageDriver;
            uintptr_t lbaOffset = _lbaOffset;
            uint32_t driveId = ((DriveEntry*)m_driveNode)->driveId;
            size_t partitionSizeSectors = 0;
            uint64_t partitionLBAOffset = 0;
            bool isPartitionRead = false;
            if (m_driveNode != m_node)
            {
                PartitionEntry* part = (PartitionEntry*)m_node;
                partitionLBAOffset = part->lbaOffset;
                lbaOffset += part->lbaOffset;
                partitionSizeSectors = part->sizeSectors;
                isPartitionRead = true;
            }
            auto ftable = identity->functionTable.serviceSpecific.storageDevice;
            // Get the sector count.
            size_t nSectors = 0, sizeofSector = 0;
            ftable.QueryDiskInfo(driveId, &nSectors, &sizeofSector);
            if (lbaOffset + nSectorsToRead > (nSectors + 1))
            {
                SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
                return false;
            }
            if (isPartitionRead)
            {
                if (lbaOffset + nSectorsToRead > (partitionLBAOffset + partitionSizeSectors + 1))
                {
                    SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
                    return false;
                }
            }
            // Attempt the read...
            void* buff = nullptr;
            size_t nSectorsRead = 0;
            if (!ftable.ReadSectors(driveId, lbaOffset, nSectorsToRead, &buff, &nSectorsRead))
            {
                SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
                return false;
            }
            if (_VectorizedMemcpy64B)
                _VectorizedMemcpy64B(obuff, buff, (nSectorsRead * sizeofSector) / 64);
            else
                utils::memcpy(obuff, buff, nSectorsRead * sizeofSector);
            memory::VirtualAllocator{nullptr}.VirtualFree(buff, nSectorsRead * sizeofSector);
            if (_nSectorsRead)
                *_nSectorsRead = nSectorsRead;
            return true;
        }
        bool DriveHandle::WriteSectors(const void* buff, size_t* _nSectorsWritten, uoff_t _lbaOffset, size_t nSectorsToWrite)
        {
            if (!m_node || m_flags & FLAGS_CLOSED)
            {
                SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
                return false;
            }
            if (!buff)
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            if (!(m_flags & FLAGS_CAN_WRITE))
            {
                SetLastError(OBOS_ERROR_VFS_READ_ONLY);
                return false;
            }
            driverInterface::driverIdentity* identity = ((DriveEntry*)m_driveNode)->storageDriver;
            uintptr_t lbaOffset = _lbaOffset;
            uint32_t driveId = ((DriveEntry*)m_driveNode)->driveId;
            size_t partitionSizeSectors = 0;
            uint64_t partitionLBAOffset = 0;
            bool isPartitionWrite = false;
            if (m_driveNode != m_node)
            {
                PartitionEntry* part = (PartitionEntry*)m_node;
                partitionLBAOffset = part->lbaOffset;
                lbaOffset += part->lbaOffset;
                partitionSizeSectors = part->sizeSectors;
                isPartitionWrite = true;
            }
            auto ftable = identity->functionTable.serviceSpecific.storageDevice;
            // Get the sector count.
            size_t nSectors = partitionSizeSectors, sizeofSector = 0;
            ftable.QueryDiskInfo(driveId, &nSectors, &sizeofSector);
            if (lbaOffset + nSectorsToWrite > (nSectors + 1))
            {
                SetLastError(OBOS_ERROR_VFS_WRITE_ABORTED);
                return false;
            }
            if (isPartitionWrite)
            {
                if (lbaOffset + nSectorsToWrite > (partitionLBAOffset + partitionSizeSectors + 1))
                {
                    SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
                    return false;
                }
            }
            // Attempt the write...
            size_t nSectorsWrote = 0;
            if (!ftable.WriteSectors(driveId, lbaOffset, nSectorsToWrite, (char*)buff, &nSectorsWrote))
            {
                SetLastError(OBOS_ERROR_VFS_WRITE_ABORTED);
                return false;
            }
            if (_nSectorsWritten)
                *_nSectorsWritten = nSectorsWrote;
            return true;
        }
        uint32_t DriveHandle::GetDriveId() const
        {
            if (!m_node || m_flags & FLAGS_CLOSED)
            {
                SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
                return 0xffffffff;
            }
            return ((DriveEntry*)m_driveNode)->driveId;
        }
        uint32_t DriveHandle::GetPartitionId() const
        {
            if (!m_node || m_flags & FLAGS_CLOSED)
            {
                SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
                return 0xffffffff;
            }
            if (((VFSNode*)m_node)->type != VFS_NODE_PARTITION_ENTRY)
            {
                SetLastError(OBOS_ERROR_VFS_HANDLE_NOT_REFERRING_TO_PARTITION);
                return false;
            }
            return ((PartitionEntry*)m_node)->partitionId;
        }

        bool DriveHandle::QueryInfo(size_t *_nSectors, size_t *_bytesPerSector, size_t *_nPartitions) const
        {
            if (!m_node || m_flags & FLAGS_CLOSED)
            {
                SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
                return false;
            }
            DriveEntry* drive = (DriveEntry*)m_driveNode;
            auto ftable = drive->storageDriver->functionTable.serviceSpecific.storageDevice;
            size_t nSectors = 0, sizeofSector = 0;
            if (!ftable.QueryDiskInfo(drive->driveId, &nSectors, &sizeofSector))
            {
                SetLastError(OBOS_ERROR_VFS_DRIVER_FAILURE);
                return false;
            }
            if (_nSectors)
               *_nSectors = nSectors;
            if (_bytesPerSector)
               *_bytesPerSector = sizeofSector;
            if (!IsPartitionHandle() && _nPartitions)
                *_nPartitions = drive->nPartitions;
            return true;
        }
        bool DriveHandle::QueryPartitionInfo(size_t* nSectors, uint64_t* lbaOffset, const char** filesystemName) const
        {
            if (!m_node || m_flags & FLAGS_CLOSED)
            {
                SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
                return false;
            }
            if (!IsPartitionHandle())
            {
                SetLastError(OBOS_ERROR_VFS_HANDLE_NOT_REFERRING_TO_PARTITION);
                return false;
            }
            PartitionEntry* part = (PartitionEntry*)m_node;
            if (nSectors)
                *nSectors = part->sizeSectors;
            if (lbaOffset)
                *lbaOffset = part->lbaOffset;
            if (filesystemName)
                *filesystemName = part->friendlyFilesystemName;
            return true;
        }
        
        bool DriveHandle::Close()
        {
            if (!m_node || m_flags & FLAGS_CLOSED)
            {
                SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
                return false;
            }
            DriveEntry* drive = (DriveEntry*)m_driveNode;
            HandleListNode* driveHandleNode = (HandleListNode*)m_driveHandleNode;
            if (driveHandleNode->next)
                driveHandleNode->next->prev = driveHandleNode->prev;
            if (driveHandleNode->prev)
                driveHandleNode->prev->next = driveHandleNode->next;
            if (drive->handlesReferencing.head == driveHandleNode)
                drive->handlesReferencing.head =  driveHandleNode->next;
            if (drive->handlesReferencing.tail == driveHandleNode)
                drive->handlesReferencing.tail =  driveHandleNode->prev;
            drive->handlesReferencing.size--;
            if (m_node != m_driveNode)
            {
                PartitionEntry* partition = (PartitionEntry*)m_node;
                HandleListNode* handleNode = (HandleListNode*)m_handleNode;
                if (handleNode->next)
                    handleNode->next->prev = handleNode->prev;
                if (handleNode->prev)
                    handleNode->prev->next = handleNode->next;
                if (partition->handlesReferencing.head == handleNode)
                    partition->handlesReferencing.head =  handleNode->next;
                if (partition->handlesReferencing.tail == handleNode)
                    partition->handlesReferencing.tail =  handleNode->prev;
                partition->handlesReferencing.size--;
                delete handleNode;
            }
            delete driveHandleNode;
            m_node = nullptr;
            m_handleNode = nullptr;
            m_flags = FLAGS_CLOSED;
            m_driveNode = nullptr;
            m_driveHandleNode = nullptr;
            return true;
        }
    }
}