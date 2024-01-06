/*
    oboskrnl/driverInterface/register_drive.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <driverInterface/register_drive.h>

#include <vector.h>

#include <allocators/liballoc.h>

#include <vfs/vfsNode.h>

#include <multitasking/thread.h>
#include <multitasking/cpu_local.h>

#include <vfs/devManip/driveHandle.h>

namespace obos
{
    namespace vfs
    {
        DriveList g_drives;
    }
	namespace driverInterface
	{
        using vfs::g_drives;
        using vfs::DriveEntry;
        uint32_t RegisterDrive()
        {
            auto currentThread = thread::GetCurrentCpuLocalPtr()->currentThread;
            if (!currentThread->driverIdentity)
                return -1;
            auto _serviceType = ((driverIdentity*)currentThread->driverIdentity)->_serviceType;
            if (_serviceType != serviceType::OBOS_SERVICE_TYPE_STORAGE_DEVICE &&
                _serviceType != serviceType::OBOS_SERVICE_TYPE_VIRTUAL_STORAGE_DEVICE)
                return -1;
            uint32_t id = g_drives.nDrives;
            DriveEntry* entry = new DriveEntry;
            if (g_drives.tail)
                g_drives.tail->next = entry;
            if(!g_drives.head)
                g_drives.head = entry;
            entry->prev = g_drives.tail;
            g_drives.tail = entry;
            g_drives.nDrives++;
            
            entry->driveId = id;
            entry->storageDriver = (driverIdentity*)(currentThread->driverIdentity);   
            return id;
        }
		void UnregisterDrive(uint32_t id)
        {
            // Find the drive.
            DriveEntry* cdrive = g_drives.head;
            DriveEntry* drive = nullptr;
            if (!cdrive)
                return;
            while (cdrive)
            {
                if (cdrive->driveId == id)
                {
                    drive = cdrive;
                    break;
                }

                cdrive = drive->next;
            }
            if (!drive)
                return;
            // Unlink the driver.
            if (drive->next)
                drive->next->prev = drive->prev;
            if (drive->prev)
                drive->prev->next = drive->next;
            if (g_drives.head == drive)
                g_drives.head = drive->next;
            if (g_drives.tail == drive)
                g_drives.tail = drive->prev;
            g_drives.nDrives--;
            // Close any open handles, partition handles for this driver will also be closed.
            for (auto handleNode = drive->handlesReferencing.head; handleNode;)
            {
                vfs::DriveHandle* handle = (vfs::DriveHandle*)handleNode->handle;
                auto next = handleNode->next;
                handle->Close();
                
                handleNode = next;
            }
            // Delete any partition handles.
            for (auto part = drive->firstPartition; part;)
            {
                auto next = part->next;
                delete part;
                part = next;
            }
            delete drive;
            return;
        }
	}
}