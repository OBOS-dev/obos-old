#pragma once

#include <types.h>

namespace obos
{
	namespace vfs
	{
		enum entryType
		{
			FILE,
			DIRECTORY,
			MOUNT_POINT
		};

		struct commonNode
		{
			STRING name;
			void* next;
			void* prev;
			virtual entryType getType() = 0;

		};

		struct mountNode : public commonNode
		{
			UINT16_T id;
			UINTPTR_T drive; // todo: make this do something.
			SIZE_T filesystemDriverId;
			struct
			{
				BOOL isReadOnly : 1;
				BOOL isRamDisk : 1;
			} attribute(packed) flags;
			virtual entryType getType() { return entryType::MOUNT_POINT; };
		};
		struct directoryNode : public commonNode
		{
			SIZE_T nFiles;
			directoryNode* parentDirectory;
			mountNode* mountPoint;

			virtual entryType getType() { return entryType::DIRECTORY; };
		};
		struct fileNode : public commonNode
		{
			SIZE_T fileSize;
			BYTE flags;
			directoryNode* parentDirectory;

			virtual entryType getType() { return entryType::FILE; };
		};
	}
}