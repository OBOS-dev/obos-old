/*
	oboskrnl/allocators/slab.h

	Copyright (c) 2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <new>

namespace obos
{
	extern bool g_slabAllocatorInitialized;
	enum class ObjectTypes
	{
		Invalid,
		Thread,
		Process,
		Mutex,
		MountPoint,
		DirectoryEntry,
		PartitionEntry,
		DriveEntry,
		InputDevice,
		DriverIdentity,
	};
	/// <summary>
	/// Initializes the slab allocator.<para></para>
	/// This doesn't need explicitly called.
	/// </summary>
	void SlabInitialize();
	/// <summary>
	/// Allocates an object of type 'type' with the slab allocator.
	/// </summary>
	/// <param name="type">The type of the object.</param>
	/// <returns>A pointer to the newly created object.</returns>
	void* ImplSlabAllocate(ObjectTypes type, size_t nObjects = 1);
	/// <summary>
	/// Frees an allocated object of type 'type'. This object must be allocated with ImplSlabAllocate
	/// </summary>
	/// <param name="type">The type of the object.</param>
	/// <param name="obj">The object to free.</param>
	void ImplSlabFree(ObjectTypes type, void* obj, size_t nObjects = 1);
	/// <summary>
	/// Checks if an object exists.
	/// </summary>
	/// <param name="type">The object type.</param>
	/// <param name="obj">The object to check.</param>
	/// <returns>Whether the object exists (true) or not (false).</returns>
	bool SlabHasObject(ObjectTypes type, void* obj);

	// Template madness!

	/*template<typename T>
	T* SlabAllocate()
	{
		return nullptr;
	}
	template<>
	thread::Thread* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::Thread)) thread::Thread{};
	}
	template<>
	process::Process* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::Process)) process::Process{};
	}
	template<>
	locks::Mutex* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::Mutex)) locks::Mutex{};
	}
	template<>
	vfs::MountPoint* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::MountPoint)) vfs::MountPoint{};
	}
	template<>
	vfs::DirectoryEntry* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::DirectoryEntry)) vfs::DirectoryEntry{};
	}
	template<>
	vfs::PartitionEntry* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::PartitionEntry)) vfs::PartitionEntry{};
	}
	template<>
	vfs::DriveEntry* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::DriveEntry)) vfs::DriveEntry{};
	}
	template<>
	driverInterface::InputDevice* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::InputDevice)) driverInterface::InputDevice{};
	}
	template<>
	driverInterface::driverIdentity* SlabAllocate()
	{
		return new (ImplSlabAllocate(ObjectTypes::DriverIdentity)) driverInterface::driverIdentity{};
	}
	template<typename T>
	void SlabFree(T*)
	{
	}
	template<>
	void SlabFree(thread::Thread* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::Thread, obj);
	}
	template<>
	void SlabFree(process::Process* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::Process, obj);
	}
	template<>
	void SlabFree(locks::Mutex* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::Mutex, obj);
	}
	template<>
	void SlabFree(vfs::MountPoint* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::MountPoint, obj);
	}
	template<>
	void SlabFree(vfs::DirectoryEntry* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::DirectoryEntry, obj);
	}
	template<>
	void SlabFree(vfs::PartitionEntry* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::PartitionEntry, obj);
	}
	template<>
	void SlabFree(vfs::DriveEntry* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::DriveEntry, obj);
	}
	template<>
	void SlabFree(driverInterface::InputDevice* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::InputDevice, obj);
	}
	template<>
	void SlabFree(driverInterface::driverIdentity* obj)
	{
		::operator delete(obj,obj);ImplSlabFree(ObjectTypes::DriverIdentity, obj);
	}*/
}