/*
    oboskrnl/driverInterface/register_drive.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <utils/vector.h>

#include <driverInterface/struct.h>

#include <vfs/vfsNode.h>

#include <allocators/slab.h>

namespace obos
{
	namespace driverInterface
	{
		struct InputDevice
		{
			driverIdentity* driver = nullptr;
			utils::Vector<uint16_t> data;
			uint32_t id = 0;
			vfs::HandleList fileHandlesReferencing;
			InputDevice *next = nullptr, *prev = nullptr;

			void* operator new(size_t)
			{
				return ImplSlabAllocate(ObjectTypes::InputDevice);
			}
			void operator delete(void* ptr)
			{
				ImplSlabFree(ObjectTypes::InputDevice, ptr);
			}
			void* operator new[](size_t sz)
			{
				return ImplSlabAllocate(ObjectTypes::InputDevice, sz / sizeof(InputDevice));
			}
			void operator delete[](void* ptr, size_t sz)
			{
				ImplSlabFree(ObjectTypes::InputDevice, ptr, sz / sizeof(InputDevice));
			}
			[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
			[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
			void operator delete(void*, void*) noexcept {}
			void operator delete[](void*, void*) noexcept {}
		};
		struct InputDeviceList
		{
			InputDevice *head = nullptr, *tail = nullptr;
			size_t nNodes = 0;
		};
		extern InputDeviceList g_inputDevices;
	}
}