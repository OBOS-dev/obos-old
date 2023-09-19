/*
	vfs/mount.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <inline-asm.h>

#include <vfs/mount.h>
#include <vfs/node.h>
#include <vfs/error.h>

#include <driver_api/enums.h>
#include <driver_api/syscalls.h>

#include <memory_manager/paging/allocate.h>

#include <utils/memory.h>

extern "C" void callDriverIterateCallback(PBYTE newRsp,
	UINTPTR_T * l4PageMap,
	void(*iterateCallback)(bool(*appendEntry)(CSTRING filename, SIZE_T bufSize, BYTE attrib)),
	bool(*fPar1)(CSTRING filename, SIZE_T bufSize, BYTE attrib));

namespace obos
{
	namespace vfs
	{
		bool g_mountedInitRD = false;
		struct
		{
			mountNode* head = nullptr;
			mountNode* tail = nullptr;
			SIZE_T count = 0;
		} mountedDrives;

		/*static SIZE_T countDirectoryIterations(CSTRING path)
		{
			SIZE_T nAppeared = 0;
			for (SIZE_T i = 0; path[i]; i++)
				if (path[i] == '/')
					nAppeared++;
			return nAppeared;
		}*/
		static driverAPI::driverIdentification* s_currentDriver = nullptr;
		static mountNode* s_newMountNode = nullptr;
		static bool s_filesystemError = false;
		static bool addFilePath(CSTRING filename, SIZE_T bufSize, BYTE attrib)
		{
			// Parse the file path.
			//SIZE_T dirIterations = 0; // How much directories does the file path recurse?
			//SIZE_T fileSize = 0;
			if ((attrib & driverAPI::fileExistsReturn::FILE_EXISTS_DIRECTORY) && (attrib & driverAPI::fileExistsReturn::FILE_EXISTS_FILE))
			{
				s_filesystemError = true;
				return false;
			}
			// If any memory is allocated here, change the clean up in MountDrive after the call to callDriverIterateCallback(), and in UnmountDrive.
			if (attrib & driverAPI::fileExistsReturn::FILE_EXISTS_DIRECTORY)
			{
				directoryNode* node = new directoryNode{};
				node->mountPoint = s_newMountNode;
				node->path = new CHAR[bufSize + 1];
				utils::memzero(node->path, bufSize + 1);
				for (SIZE_T i = 0; i < bufSize; i++)
					node->path[i] = filename[i];
				if (!s_newMountNode->entryList.head)
					s_newMountNode->entryList.tail = s_newMountNode->entryList.head = (commonNode*)node;
				else
				{
					s_newMountNode->entryList.tail->next = node;
					node->prev = mountedDrives.tail;
					s_newMountNode->entryList.tail = node;
				}
			}
			else if (attrib & driverAPI::fileExistsReturn::FILE_EXISTS_FILE)
			{
				fileNode* node = new fileNode{};
				node->path = new CHAR[bufSize + 1];
				utils::memzero(node->path, bufSize + 1);
				for (SIZE_T i = 0; i < bufSize; i++)
					node->path[i] = filename[i];
				node->attrib = attrib;
				if (!s_newMountNode->entryList.head)
					s_newMountNode->entryList.tail = s_newMountNode->entryList.head = (commonNode*)node;
				else
				{
					s_newMountNode->entryList.tail->next = node;
					node->prev = mountedDrives.tail;
					s_newMountNode->entryList.tail = node;
				}
			}
			return true;
		}

		DWORD MountDrive(UINT16_T mountPoint, mountFlags flags, SIZE_T filesystemDriverId, UINTPTR_T drive)
		{
			for (mountNode* current = mountedDrives.head; current != nullptr; )
			{
				if (current->userMountPoint == mountPoint)
					return OBOS_VFS_ALREADY_EXISTS;

				current = (mountNode*)current->next;
			}
			if (g_mountedInitRD && flags.isInitRD)
				return OBOS_VFS_ALREADY_EXISTS;
			if (driverAPI::g_registeredDriversCapacity > filesystemDriverId)
				return OBOS_VFS_DRIVER_NOT_LOADED;
			if (driverAPI::g_registeredDrivers[filesystemDriverId]->service_type != driverAPI::serviceType::SERVICE_TYPE_FILESYSTEM)
				return OBOS_VFS_NOT_A_FILESYSTEM_DRIVER;
			mountNode* newNode = new mountNode{};
			newNode->userMountPoint = mountPoint;
			newNode->flags = flags;
			newNode->filesystemDriverId = filesystemDriverId;
			newNode->drive = drive;
			if (!mountedDrives.head)
				mountedDrives.tail = mountedDrives.head = newNode;
			else
			{
				mountedDrives.tail->next = newNode;
				newNode->prev = mountedDrives.tail;
				mountedDrives.tail = newNode;
			}
			mountedDrives.count++;
			driverAPI::driverIdentification* driver = driverAPI::g_registeredDrivers[filesystemDriverId];
			/*char(*existsCallback)(CSTRING filename, SIZE_T * size) =
				(char(*)(CSTRING filename, SIZE_T * size))driver->existsCallback;
			void(*readCallback)(CSTRING filename, STRING output, SIZE_T size) =
				(void(*)(CSTRING filename, STRING output, SIZE_T size))driver->readCallback;*/
			void(*iterateCallback)(bool(*appendEntry)(CSTRING,SIZE_T,BYTE)) =
				(void(*)(bool(*appendEntry)(CSTRING,SIZE_T,BYTE)))driver->iterateCallback;

			EnterKernelSection();

#ifdef __x86_64__
			memory::PageMap* pageMap = memory::g_level4PageMap;
			memory::g_level4PageMap = driver->process->level4PageMap;
			UINTPTR_T* _pageMap = memory::g_level4PageMap->getPageMap();
#define commitMemory , true
#elif defined(__i686__)
			memory::PageDirectory* pageDirectory = memory::g_pageDirectory;
			memory::g_pageDirectory = driver->process->pageDirectory;
			UINTPTR_T* _pageMap = memory::g_pageDirectory->getPageDirectory();
#define commitMemory
#endif

			s_newMountNode = newNode;
			s_currentDriver = driver;

			PBYTE newRsp = (PBYTE)memory::VirtualAlloc(nullptr, 1, memory::VirtualAllocFlags::GLOBAL | memory::VirtualAllocFlags::WRITE_ENABLED commitMemory);

			callDriverIterateCallback(newRsp + 4096, _pageMap, iterateCallback, addFilePath);

			memory::VirtualFree(newRsp, 1);

#ifdef __x86_64__
			memory::g_level4PageMap = pageMap;
#elif defined(__i686__)
			memory::g_pageDirectory = pageDirectory;
#endif

#undef commitMemory

			LeaveKernelSection();

			if (s_filesystemError)
			{
				// The filesystem shit itself, we must now clean up after ourselves.
				for (commonNode* node = newNode->entryList.head; node != nullptr; )
				{
					delete[] node->path;

					node = node->next;
					delete node->prev;
				}
				delete newNode;
				return OBOS_VFS_FILESYSTEM_ERROR;
			}



			return OBOS_VFS_NO_ERROR;
		}

		DWORD UnmountDrive(UINT16_T mountPoint)
		{
			mountNode* current = mountedDrives.head;
			for (; current != nullptr; )
			{
				if (current->userMountPoint == mountPoint)
					break;

				current = (mountNode*)current->next;
			}
			if (!current)
				return OBOS_VFS_NOT_FOUND;
			// TODO: Close all file handles.
			mountNode* previous = (mountNode*)current->prev;
			mountNode* next = (mountNode*)current->next;
			if(previous)
				previous->next = current->next;
			if (next)
				next->prev = previous;
			if (mountedDrives.head == current)
				mountedDrives.head = next;
			if (mountedDrives.tail == current)
				mountedDrives.tail = previous;
			mountedDrives.count--;
			for (commonNode* node = current->entryList.head; node != nullptr; )
			{
				delete[] node->path;

				node = node->next;
				delete node->prev;
			}
			delete current;
			return OBOS_VFS_NO_ERROR;
		}

	}
}