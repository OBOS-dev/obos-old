/*
	oboskrnl/vfs/vfsNode.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <driverInterface/struct.h>

namespace obos
{
	namespace vfs
	{
		struct VFSString
		{
			char* str;
			size_t strLen;
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
			struct DirectoryEntry *head, *tail;
			size_t size;
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
			struct DirectoryEntry *next, *prev; // used for the children.
			struct DirectoryEntry *parent;
			DirectoryEntryList children;
		};
		struct MountPoint : public GeneralFSNode
		{
			MountPoint() : GeneralFSNode{ VFS_NODE_MOUNTPOINT }
			{}
			uint32_t id;
			uint32_t partitionId = ((uint32_t)-1);
			driverInterface::driverIdentity* filesystemDriver; // The filesystem driver to invoke.
			uint32_t otherMountPointsReferencing;
		};
		struct HandleListNode
		{
			HandleListNode *next, *prev;
			void* handle;
		};
		struct HandleList
		{
			HandleListNode *head, *tail;
			size_t size;
		};
		struct DirectoryEntry : public GeneralFSNode
		{
			DirectoryEntry() : GeneralFSNode{ VFS_NODE_DIRECTORY_ENTRY }
			{}
			DirectoryEntry(DirectoryEntryType _direntType) : GeneralFSNode{ VFS_NODE_DIRECTORY_ENTRY }, direntType{ _direntType }
			{}
			DirectoryEntryType direntType = DIRECTORY_ENTRY_TYPE_INVALID;
			uint32_t fileAttrib;
			size_t filesize;
			VFSString path; // Never should be null.
			DirectoryEntry* linkedNode; // Only non-null when direntType == DIRECTORY_ENTRY_TYPE_SYMLINK
			struct MountPoint* mountPoint;
			HandleList fileHandlesReferencing;
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
		};
		struct DriveList
		{
			DriveEntry *head, *tail;
			size_t nDrives;
		};

		extern DriveList g_drives;
	}
}