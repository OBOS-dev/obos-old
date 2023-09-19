/*
	
*/

#pragma once

#include <types.h>

#define OBOS_VFS_MAX_PATH_SIZE (0x7FFF)

namespace obos
{
	namespace vfs
	{
		enum entryType
		{
			FILE,
			DIRECTORY,
			MOUNT_POINT,
			TMP_FILE, // A file that exists until the computer shutdowns or restarts.
		};

		struct commonNode
		{
			STRING path;
			commonNode* next;
			commonNode* prev;
			virtual entryType getType() = 0;

			virtual ~commonNode(){}
		};

		struct mountFlags
		{
			bool isReadOnly : 1; // If true, no write operations can happen on this drive.
			bool isRamDisk : 1; // If true, 'drive' is a pointer to the beginning of the ram disk.
			bool isInitRD : 1; // If true, 'drive' does nothing.
		};

		struct mountNode : public commonNode
		{
			UINT16_T userMountPoint;
			UINTPTR_T drive; // todo: make this do something.
			SIZE_T filesystemDriverId;
			mountFlags flags;
			commonNode* rootDirectory;
			struct
			{
				commonNode* head;
				commonNode* tail;
				SIZE_T count;
			} entryList;
			virtual entryType getType() override { return entryType::MOUNT_POINT; };
			virtual ~mountNode() {}
		};
		struct directoryNode : public commonNode
		{
			SIZE_T nFiles;
			directoryNode* parentDirectory;
			mountNode* mountPoint;
			commonNode* head; // Can be a file or a child directory.
			commonNode* tail; // Can be a file or a child directory.

			virtual entryType getType() override { return entryType::DIRECTORY; };
			virtual ~directoryNode() {}
		};
		struct fileNode : public commonNode
		{
			SIZE_T fileSize;
			BYTE attrib;
			directoryNode* parentDirectory;

			virtual entryType getType() override { return entryType::FILE; };
			virtual ~fileNode() {}
		};
		struct vfileNode : public fileNode
		{
			PBYTE data;

			virtual entryType getType() override { return entryType::TMP_FILE; };
			virtual ~vfileNode() {}
		};
	}
}