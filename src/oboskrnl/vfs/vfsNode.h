/*
	oboskrnl/vfs/vfsNode.h

	Copyright (c) 2023 Omar Berrow
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
		};

		enum NodeType
		{
			VFS_INVALID_NODE_TYPE,
			VFS_NODE_MOUNTPOINT,
			VFS_NODE_DIRECTORY_ENTRY,
		};
		enum DirectoryEntryType
		{
			DIRECTORY_ENTRY_TYPE_FILE,
			DIRECTORY_ENTRY_TYPE_SYMLINK,
			DIRECTORY_ENTRY_TYPE_DIRECTORY,
		};
		struct VFSNode
		{
			VFSNode() = default;
			VFSNode(NodeType _type) : type{ _type } {}
			struct DirectoryEntry *next, *prev;
			struct DirectoryEntry *parent, *children;
			size_t countChildren;
			struct MountPoint* mountPoint;
			NodeType type = VFS_INVALID_NODE_TYPE;
		};

		struct MountPoint : public VFSNode
		{
			MountPoint() : VFSNode{ VFS_NODE_MOUNTPOINT }
			{}
			uint32_t id;
			uint32_t partitionId;
			driverInterface::driverIdentity* filesystemDriver; // The filesystem driver to invoke.
			uint32_t otherMountPointsReferencing;
			struct Directory *head, *tail;
			size_t nDirectories;
		};
		struct DirectoryEntry : public VFSNode
		{
			DirectoryEntry() : VFSNode{ VFS_NODE_DIRECTORY_ENTRY }
			{}
			DirectoryEntry(DirectoryEntryType _direntType) : VFSNode{ VFS_NODE_DIRECTORY_ENTRY }, direntType{ _direntType }
			{}
			DirectoryEntryType direntType;
			driverInterface::fileAttributes fileAttrib;
			VFSString path; // Never should be null.
			VFSString linkedPath; // Only non-null if direntType == DIRECTORY_ENTRY_TYPE_SYMLINK
			uint32_t fileHandlesReferencing;
			// TODO: Add a list of file handles.
		};
		struct Directory : public DirectoryEntry
		{
			Directory() : DirectoryEntry{ DIRECTORY_ENTRY_TYPE_DIRECTORY }
			{}
			DirectoryEntry *head, *tail;
			size_t nFiles;
		};
	}
}