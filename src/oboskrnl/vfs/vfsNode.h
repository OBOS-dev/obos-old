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
			VFSString() = default;
			VFSString(const char* str) : str{ (char*)str }
			{
				for (strLen = 0; str[strLen]; strLen++);
			}
			VFSString(char* str) : str{ str }
			{
				for (strLen = 0; str[strLen]; strLen++);
			}
		};

		enum NodeType
		{
			VFS_INVALID_NODE_TYPE,
			VFS_NODE_MOUNTPOINT,
			VFS_NODE_DIRECTORY_ENTRY,
		};
		enum DirectoryEntryType
		{
			DIRECTORY_ENTRY_TYPE_INVALID = -1,
			DIRECTORY_ENTRY_TYPE_FILE,
			DIRECTORY_ENTRY_TYPE_SYMLINK,
			DIRECTORY_ENTRY_TYPE_DIRECTORY,
		};
		struct DirectoryEntryList
		{
			struct DirectoryEntry *head, *tail;
			size_t size;
		};
		struct VFSNode
		{
			VFSNode() = default;
			VFSNode(NodeType _type) : type{ _type } {}
			struct DirectoryEntry *next, *prev; // used for the children.
			struct DirectoryEntry *parent;
			DirectoryEntryList children;
			struct MountPoint* mountPoint;
			NodeType type = VFS_INVALID_NODE_TYPE;
		};

		struct MountPoint : public VFSNode
		{
			MountPoint() : VFSNode{ VFS_NODE_MOUNTPOINT }
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
		struct DirectoryEntry : public VFSNode
		{
			DirectoryEntry() : VFSNode{ VFS_NODE_DIRECTORY_ENTRY }
			{}
			DirectoryEntry(DirectoryEntryType _direntType) : VFSNode{ VFS_NODE_DIRECTORY_ENTRY }, direntType{ _direntType }
			{}
			DirectoryEntryType direntType = DIRECTORY_ENTRY_TYPE_INVALID;
			uint32_t fileAttrib;
			size_t filesize;
			VFSString path; // Never should be null.
			DirectoryEntry* linkedNode; // Only non-null when direntType == DIRECTORY_ENTRY_TYPE_SYMLINK
			HandleList fileHandlesReferencing;
		};
		struct Directory : public DirectoryEntry
		{
			Directory() : DirectoryEntry{ DIRECTORY_ENTRY_TYPE_DIRECTORY }
			{}
		};
	}
}