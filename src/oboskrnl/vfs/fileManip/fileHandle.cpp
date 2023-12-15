/*
	oboskrnl/vfs/fileHandle.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>
#include <memory_manipulation.h>

#include <vfs/fileManip/fileHandle.h>

#include <vfs/vfsNode.h>

#include <vfs/mount/mount.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace vfs
	{
		bool strContains(const char* str, char ch)
		{
			size_t i = 0;
			for (; str[i] && str[i] != ch; i++);
			return str[i] == ch;
		}
		uint32_t getMountId(const char* path)
		{
			uint32_t ret = 0;
			size_t size = utils::strCountToChar(path, ':');
			path += *path == '\n';
			for (size_t i = 0; i < size; i++)
			{
				ret *= 10;
				ret += path[i] - '0';
			}
			return ret;
		}
		DirectoryEntry* SearchForNode(DirectoryEntry* root, void* userdata, bool(*compare)(DirectoryEntry* current, void* userdata))
		{
			if (!root)
				return nullptr;

			DirectoryEntry* node1 = root;
			while (node1)
			{
				if (compare(node1, userdata))
					return node1;

				node1 = node1->next;
			}
			node1 = root;
			
			DirectoryEntry* node2 = nullptr;
			while (!node2 && node1)
			{
				node2 = SearchForNode(node1->children.head, userdata, compare);
				
				node1 = node1->next;
			}
			return node2;
		}
		bool FileHandle::Open(const char* path, OpenOptions options)
		{
			if ((m_flags & FLAGS_CLOSED))
			{
				SetLastError(OBOS_ERROR_ALREADY_EXISTS);
				return 0;
			}
			if (!path)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!strContains(path, ':'))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			// Get the mount point id from the path.
			uint32_t mountId = getMountId(path);
			MountPoint* point = nullptr;
			for (size_t i = 0; i < g_mountPoints.length(); i++)
				if (g_mountPoints[i]->id == mountId)
					point = g_mountPoints[i];
			if (!point)
			{
				SetLastError(OBOS_ERROR_VFS_FILE_NOT_FOUND);
				return false;
			}
			// Find the file's node.
			const char* realPath = path;
			realPath += utils::strCountToChar(path, ':');
			realPath += utils::strCountToChar(path, '/');
			// If a file is in the root, the file size is zero, whether it actually is or not? TODO: Fix.
			DirectoryEntry* entry = SearchForNode(point->children.head, (void*)realPath, [](DirectoryEntry* current, void* userdata)->bool {
				return utils::strcmp(current->path.str, (const char*)userdata);
				});
			if (!entry)
			{
				SetLastError(OBOS_ERROR_VFS_FILE_NOT_FOUND);
				return false;
			}
			m_pathNode = entry;
			while (entry->direntType == DIRECTORY_ENTRY_TYPE_SYMLINK)
				entry = entry->linkedNode;
			m_node = entry;

			HandleListNode* node = new HandleListNode{};
			node->handle = this;
			if (entry->fileHandlesReferencing.tail)
				entry->fileHandlesReferencing.tail->next = node;
			if(!entry->fileHandlesReferencing.head)
				entry->fileHandlesReferencing.head = node;
			node->prev = entry->fileHandlesReferencing.tail;
			entry->fileHandlesReferencing.size++;
			
			if (options & OPTIONS_APPEND)
				SeekTo(0, SEEKPLACE_END);
			if (!(options & OPTIONS_READ_ONLY))
				m_flags |= FLAGS_ALLOW_WRITE;
			return true;
		}
		bool ReadFile(driverInterface::DriverClient& client, const char* path, uint32_t partitionId, void** data, size_t* nRead, size_t nToSkip, size_t nToRead);

		bool FileHandle::Read(char* data, size_t nToRead, bool peek)
		{
			if (m_flags & FLAGS_CLOSED)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			DirectoryEntry* node = (DirectoryEntry*)m_node;
			if (Eof())
			{
				SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
				return false;
			}
			if (!nToRead)
				return true;
			driverInterface::DriverClient driver;
			bool(*ReadCharacter)(driverInterface::DriverClient&, char*, FileHandle*) =
				[](driverInterface::DriverClient& driver, char* dest, FileHandle* _this)->bool
				{
					char* tempDest = nullptr;
					DirectoryEntry* node = (DirectoryEntry*)_this->m_node;
					if (_this->Eof())
					{
						SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
						return false;
					}
					bool ret = ReadFile(driver, node->path.str, node->mountPoint->partitionId, (void**)&tempDest, nullptr, _this->m_currentFilePos++, 1);
					*dest = *tempDest;
					delete[] tempDest;
					return ret;
				};
			process::Process* proc = (process::Process*)node->mountPoint->filesystemDriver->process;

			uint32_t pid = proc->pid;
			bool ret = true;

			if (data)
			{
				driver.OpenConnection(pid, 15000);

				volatile auto originalFilePos = m_currentFilePos;

				for (size_t i = 0; i < nToRead && ret; ret = ReadCharacter(driver, &data[i++], this));

				if (peek)
					m_currentFilePos = originalFilePos;

				driver.CloseConnection();
			}
			return ret;
		}
		bool FileHandle::Write()
		{
			SetLastError(OBOS_ERROR_UNIMPLEMENTED_FEATURE);
			return false;
		}

		bool FileHandle::Eof() const
		{
			const DirectoryEntry* node = (DirectoryEntry*)m_node;
			return m_currentFilePos + 1 >= node->filesize;
		}
		size_t FileHandle::GetFileSize() const
		{
			if (m_flags & FLAGS_CLOSED || !m_node)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return 0;
			}
			return ((DirectoryEntry*)m_node)->filesize;
		}
		void FileHandle::GetParent(char* path, size_t* sizePath)
		{
			if (m_flags & FLAGS_CLOSED)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return;
			}
			DirectoryEntry* node = (DirectoryEntry*)m_node;
			node = node->parent;
			if (node)
			{
				if (path)
					utils::memcpy(path, node->path.str, node->path.strLen);
				if (*sizePath)
					*sizePath = node->path.strLen;
			}
		}

		uoff_t FileHandle::SeekTo(off_t count, SeekPlace from)
		{
			if (m_flags & FLAGS_CLOSED)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return 0;
			}
			DirectoryEntry* node = (DirectoryEntry*)m_node;
			uoff_t ret = GetPos();
			switch (from)
			{
			case obos::vfs::SEEKPLACE_CUR:
				m_currentFilePos += count;
				break;
			case obos::vfs::SEEKPLACE_BEG:
				m_currentFilePos = count;
				break;
			case obos::vfs::SEEKPLACE_END:
				m_currentFilePos = node->filesize + count;
				break;
			default:
				break;
			}
			return ret;
		}

		bool FileHandle::Close()
		{
			return false;
		}

	}
}