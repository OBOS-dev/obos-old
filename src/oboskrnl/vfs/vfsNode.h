/*
	oboskrnl/vfs/vfsNode.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <driverInterface/struct.h>

#include <allocators/slab.h>

#define VFS_VFSVODE_H_INCLUDED 1

namespace obos
{
	namespace vfs
	{
		struct VFSString
		{
			char* str = nullptr;
			size_t strLen = 0;
			VFSString() = default;
			VFSString(const char* str) : str{ (char*)str }
			{
				if (!str)
					return;
				for (strLen = 0; str[strLen]; strLen++);
			}
			VFSString(char* str) : str{ str }
			{
				if (!str)
					return;
				for (strLen = 0; str[strLen]; strLen++);
			}
			operator char*() { return str; };
		};

		enum NodeType
		{
			VFS_INVALID_NODE_TYPE,
			VFS_NODE_MOUNTPOINT,
			VFS_NODE_DIRECTORY_ENTRY,
			VFS_NODE_DRIVE_ENTRY,
			VFS_NODE_PARTITION_ENTRY,
		};
		enum DirectoryEntryType
		{
			DIRECTORY_ENTRY_TYPE_INVALID = -1,
			DIRECTORY_ENTRY_TYPE_FILE,
			DIRECTORY_ENTRY_TYPE_SYMLINK,
			DIRECTORY_ENTRY_TYPE_DIRECTORY,
		};
		// General
		struct DirectoryEntryList
		{
			struct DirectoryEntry *head = nullptr, *tail = nullptr;
			size_t size = 0;
		};
		struct VFSNode
		{
			VFSNode() = default;
			VFSNode(NodeType _type) : type{ _type } {}
			NodeType type = VFS_INVALID_NODE_TYPE;
		};

		// Filesystem-Related nodes.

		struct GeneralFSNode : public VFSNode
		{
			GeneralFSNode() = delete;
			GeneralFSNode(NodeType type) : VFSNode{ type }
			{}
			struct DirectoryEntry *next = nullptr, *prev = nullptr; // used for the children.
			// If this is moved to DirectoryEntry, don't forget to change how SyscallDirectoryIteratorGetParent() works, or you won't have a good time.
			struct DirectoryEntry *parent = nullptr;
			DirectoryEntryList children{};
		};
		struct MountPoint : public GeneralFSNode
		{
			MountPoint() : GeneralFSNode{ VFS_NODE_MOUNTPOINT }
			{}
			uint32_t id = 0;
			struct PartitionEntry* partition;
			bool isInitrd = false;
			driverInterface::driverIdentity* filesystemDriver = nullptr; // The filesystem driver to invoke.
			uint32_t otherMountPointsReferencing = 0;

			void* operator new(size_t)
			{
				return ImplSlabAllocate(ObjectTypes::MountPoint);
			}
			void operator delete(void* ptr)
			{
				ImplSlabFree(ObjectTypes::MountPoint, ptr);
			}
			void* operator new[](size_t sz)
			{
				return ImplSlabAllocate(ObjectTypes::MountPoint, sz / sizeof(MountPoint));
			}
			void operator delete[](void* ptr, size_t sz)
			{
				ImplSlabFree(ObjectTypes::MountPoint, ptr, sz / sizeof(MountPoint));
			}
			[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
			[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
			void operator delete(void*, void*) noexcept {}
			void operator delete[](void*, void*) noexcept {}
		};
		struct HandleListNode
		{
			HandleListNode *next = nullptr, *prev = nullptr;
			void* handle = nullptr;
		};
		struct HandleList
		{
			HandleListNode *head = nullptr, *tail = nullptr;
			size_t size = 0;
		};
		struct AddressSpaceNode
		{
			AddressSpaceNode *next = nullptr, *prev = nullptr;
			void* addr;
			size_t off, countBytes;
			HandleListNode *node;
			void* owner;
		};
		struct AddressSpaceList
		{
			AddressSpaceNode *head, *tail;
			size_t nNodes;
		};
		struct DirectoryEntry : public GeneralFSNode
		{
			DirectoryEntry() : GeneralFSNode{ VFS_NODE_DIRECTORY_ENTRY }
			{}
			DirectoryEntry(DirectoryEntryType _direntType) : GeneralFSNode{ VFS_NODE_DIRECTORY_ENTRY }, direntType{ _direntType }
			{}
			DirectoryEntryType direntType = DIRECTORY_ENTRY_TYPE_INVALID;
			uint32_t fileAttrib = 0;
			size_t filesize = 0;
			VFSString path{}; // Never should be null.
			DirectoryEntry* linkedNode = nullptr; // Only non-null when direntType == DIRECTORY_ENTRY_TYPE_SYMLINK
			struct MountPoint* mountPoint = nullptr;
			HandleList fileHandlesReferencing{};
			void* operator new(size_t)
			{
				return ImplSlabAllocate(ObjectTypes::DirectoryEntry);
			}
			void operator delete(void* ptr)
			{
				ImplSlabFree(ObjectTypes::DirectoryEntry, ptr);
			}
			void* operator new[](size_t sz)
			{
				return ImplSlabAllocate(ObjectTypes::DirectoryEntry, sz / sizeof(DirectoryEntry));
			}
			void operator delete[](void* ptr, size_t sz)
			{
				ImplSlabFree(ObjectTypes::DirectoryEntry, ptr, sz / sizeof(DirectoryEntry));
			}
			[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
			[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
			void operator delete(void*, void*) noexcept {}
			void operator delete[](void*, void*) noexcept {}
			AddressSpaceList regionsMapped;
		};
		struct Directory : public DirectoryEntry
		{
			Directory() : DirectoryEntry{ DIRECTORY_ENTRY_TYPE_DIRECTORY }
			{}
		};

		// Drive-related nodes

		struct PartitionEntry : public VFSNode
		{
			PartitionEntry() : VFSNode{ VFS_NODE_PARTITION_ENTRY }
			{}
			uint32_t partitionId = 0;
			uint64_t lbaOffset = 0;
			size_t sizeSectors = 0;
			// Also add the handle to the DriveEntry's handle list.
			HandleList handlesReferencing;
			struct DriveEntry* drive;
			driverInterface::driverIdentity* filesystemDriver;
			const char* friendlyFilesystemName = "UNKNOWN";
			PartitionEntry *next = nullptr, *prev = nullptr;
			void* operator new(size_t)
			{
				return ImplSlabAllocate(ObjectTypes::PartitionEntry);
			}
			void operator delete(void* ptr)
			{
				ImplSlabFree(ObjectTypes::PartitionEntry, ptr);
			}
			void* operator new[](size_t sz)
			{
				return ImplSlabAllocate(ObjectTypes::PartitionEntry, sz / sizeof(PartitionEntry));
			}
			void operator delete[](void* ptr, size_t sz)
			{
				ImplSlabFree(ObjectTypes::PartitionEntry, ptr, sz / sizeof(PartitionEntry));
			}
			[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
			[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
			void operator delete(void*, void*) noexcept {}
			void operator delete[](void*, void*) noexcept {}
		};
		struct DriveEntry : public VFSNode
		{
			DriveEntry() : VFSNode{ VFS_NODE_DRIVE_ENTRY }
			{}
			uint32_t driveId = 0;
			bool isReadOnly = false;
			driverInterface::driverIdentity* storageDriver = nullptr; // The storage driver to invoke.
			PartitionEntry *firstPartition = nullptr,
						   *lastPartition  = nullptr;
			size_t nPartitions = 0;
			HandleList handlesReferencing;
			DriveEntry *next, *prev;

			void* operator new(size_t )
			{
				return ImplSlabAllocate(ObjectTypes::DriveEntry);
			}
			void operator delete(void* ptr)
			{
				ImplSlabFree(ObjectTypes::DriveEntry, ptr);
			}
			void* operator new[](size_t sz)
			{
				return ImplSlabAllocate(ObjectTypes::DriveEntry, sz / sizeof(DriveEntry));
			}
			void operator delete[](void* ptr, size_t sz)
			{
				ImplSlabFree(ObjectTypes::DriveEntry, ptr, sz / sizeof(DriveEntry));
			}
			[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
			[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
			void operator delete(void*, void*) noexcept {}
			void operator delete[](void*, void*) noexcept {}
		};
		struct DriveList
		{
			DriveEntry *head, *tail;
			size_t nDrives;
		};

		extern DriveList g_drives;
	}
}