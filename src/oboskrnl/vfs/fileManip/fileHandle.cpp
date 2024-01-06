/*
	oboskrnl/vfs/fileHandle.cpp

	Copyright (c) 2023-2024 Omar Berrow
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
		// Do not make these next three functions static, as it's used in many places.
		bool strContains(const char* str, char ch)
		{
			size_t i = 0;
			for (; str[i] && str[i] != ch; i++);
			return str[i] == ch;
		}
		// If size == 0, it is determined automatically.
		uint32_t getMountId(const char* path, size_t size = 0)
		{
			uint32_t ret = 0;
			if (!size)
				size = utils::strCountToChar(path, ':');
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
			if (!(m_flags & FLAGS_CLOSED) || m_node)
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
			entry->fileHandlesReferencing.tail = node;
			entry->fileHandlesReferencing.size++;
			m_nodeInFileHandlesReferencing = node;

			if (options & OPTIONS_APPEND)
				SeekTo(0, SEEKPLACE_END);
			if (!(options & OPTIONS_READ_ONLY))
				m_flags |= FLAGS_ALLOW_WRITE;
			m_flags &= ~FLAGS_CLOSED;
			return true;
		}

		bool FileHandle::Read(char* data, size_t nToRead, bool peek)
		{
			if (m_flags & FLAGS_CLOSED || !m_node)
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
			if (__TestEof(m_currentFilePos + (nToRead - 1)))
			{
				SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
				return false;
			}
			if (!nToRead)
				return true;
			bool ret = true;

			if (data)
			{
				auto& functions = node->mountPoint->filesystemDriver->functionTable.serviceSpecific.filesystem;

				uint64_t driveId = node->mountPoint->partitionId >> 24;
				uint8_t drivePartitionId = node->mountPoint->partitionId & 0xff;

				ret = functions.ReadFile(
						driveId,
						drivePartitionId,
						node->path,
						m_currentFilePos,
						nToRead,
						data);
				if (!ret)
					SetLastError(OBOS_ERROR_VFS_READ_ABORTED);
				if (ret && !peek)
					m_currentFilePos += nToRead;
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
			// const DirectoryEntry* node = (DirectoryEntry*)m_node;
			// return m_currentFilePos + 1 >= node->filesize;
			return __TestEof(m_currentFilePos);
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
			case SEEKPLACE_CUR:
				m_currentFilePos += count;
				break;
			case SEEKPLACE_BEG:
				m_currentFilePos = count;
				break;
			case SEEKPLACE_END:
				m_currentFilePos = node->filesize + count;
				break;
			default:
				break;
			}
			return ret;
		}

		bool FileHandle::Close()
		{
			if (m_flags & FLAGS_CLOSED)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			if (!m_node || !m_nodeInFileHandlesReferencing)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			DirectoryEntry* node = (DirectoryEntry*)m_node;
			HandleListNode* nodeInFHR = (HandleListNode*)m_nodeInFileHandlesReferencing;
			if (nodeInFHR->next)
				nodeInFHR->next->prev = nodeInFHR->prev;
			if (nodeInFHR->prev)
				nodeInFHR->prev->next = nodeInFHR->next;
			if (node->fileHandlesReferencing.head == nodeInFHR)
				node->fileHandlesReferencing.head = nodeInFHR->next;
			if (node->fileHandlesReferencing.tail == nodeInFHR)
				node->fileHandlesReferencing.tail = nodeInFHR->prev;
			node->fileHandlesReferencing.size--;
			delete nodeInFHR;
			m_nodeInFileHandlesReferencing = m_pathNode = m_node = nullptr;
			m_currentFilePos = 0;

			m_flags = FLAGS_CLOSED;
			return true;
		}

		bool FileHandle::__TestEof(uoff_t pos) const
		{
			const DirectoryEntry* node = (DirectoryEntry*)m_node;
			return pos >= node->filesize;
		}
	}
}