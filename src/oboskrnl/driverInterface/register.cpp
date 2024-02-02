/*
    oboskrnl/driverInterface/register_drive.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <driverInterface/register.h>
#include <driverInterface/struct.h>
#include <driverInterface/input_device.h>

#include <utils/vector.h>

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
        InputDeviceList g_inputDevices;
        static uint32_t RegisterDrive()
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
        static void UnregisterDrive(uint32_t id)
        {
            // Find the drive.
            DriveEntry* cdrive = g_drives.head;
            DriveEntry* drive = nullptr;
            while (cdrive)
            {
                if (cdrive->driveId == id)
                {
                    drive = cdrive;
                    break;
                }

                cdrive = cdrive->next;
            }
            if (!drive)
                return;
            // Unlink the drive.
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
        static uint32_t RegisterUserInputDevice()
        {
            auto currentThread = thread::GetCurrentCpuLocalPtr()->currentThread;
            if (!currentThread->driverIdentity)
                return -1;
            auto _serviceType = ((driverIdentity*)currentThread->driverIdentity)->_serviceType;
            if (_serviceType != serviceType::OBOS_SERVICE_TYPE_USER_INPUT_DEVICE &&
                _serviceType != serviceType::OBOS_SERVICE_TYPE_VIRTUAL_USER_INPUT_DEVICE)
                return -1;
            uint32_t id = g_drives.nDrives;
            InputDevice* entry = new InputDevice;
            if (g_inputDevices.tail)
                g_inputDevices.tail->next = entry;
            if (!g_inputDevices.head)
                g_inputDevices.head = entry;
            entry->prev = g_inputDevices.tail;
            g_inputDevices.tail = entry;
            g_inputDevices.nNodes++;

            entry->id = id;
            entry->driver = (driverIdentity*)(currentThread->driverIdentity);
            return id;
        }
        static void UnregisterInputDevice(uint32_t id)
        {
            // Find the device.
            InputDevice* cdevice = g_inputDevices.head;
            InputDevice* device = nullptr;
            while (cdevice)
            {
                if (cdevice->id == id)
                {
                    device = cdevice;
                    break;
                }

                cdevice = cdevice->next;
            }
            if (!device)
                return;
            // Unlink the device.
            if (device->next)
                device->next->prev = device->prev;
            if (device->prev)
                device->prev->next = device->next;
            if (g_inputDevices.head == device)
                g_inputDevices.head = device->next;
            if (g_inputDevices.tail == device)
                g_inputDevices.tail = device->prev;
            g_drives.nDrives--;
            // Close any open file handles.
            for (auto handleNode = device->fileHandlesReferencing.head; handleNode;)
            {
                vfs::FileHandle* handle = (vfs::FileHandle*)handleNode->handle;
                auto next = handleNode->next;
                handle->Close();

                handleNode = next;
            }
            delete device;
            return;
        }
        uint32_t RegisterDevice(DeviceType type)
        {
            switch (type)
            {
            case obos::driverInterface::DeviceType::Drive:
                return RegisterDrive();
            case obos::driverInterface::DeviceType::UserInput:
                return RegisterUserInputDevice();
            default:
                break;
            }
            return (uint32_t)-1;
        }
        void UnregisterDevice(uint32_t id, DeviceType type)
        {
            switch (type)
            {
            case obos::driverInterface::DeviceType::Drive:
                UnregisterDrive(id);
                break;
            case obos::driverInterface::DeviceType::UserInput:
                UnregisterInputDevice(id);
                break;
            default:
                break;
            }
        }
        bool WriteByteToInputDeviceBuffer(uint32_t id, uint16_t exChar)
        {
            InputDevice* device = (InputDevice*)GetUserInputDevice(id);
            if (!device)
                return false;
            device->data.push_back(exChar);
            return true;
        }
        void* GetUserInputDevice(uint32_t id)
        {
            InputDevice* cdevice = g_inputDevices.head;
            InputDevice* device = nullptr;
            while (cdevice)
            {
                if (cdevice->id == id)
                {
                    device = cdevice;
                    break;
                }

                cdevice = cdevice->next;
            }
            return device;
        }
    }
}