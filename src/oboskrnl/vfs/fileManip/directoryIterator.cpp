/*
	oboskrnl/vfs/fileManip/directoryIterator.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <vfs/fileManip/directoryIterator.h>

#include <vfs/mount/mount.h>

#include <vfs/vfsNode.h>

namespace obos
{
	namespace vfs
	{
		extern bool strContains(const char* str, char ch);
		extern uint32_t getMountId(const char* path, size_t size = 0);
		extern DirectoryEntry* SearchForNode(DirectoryEntry* root, void* userdata, bool(*compare)(DirectoryEntry* current, void* userdata));
		bool DirectoryIterator::OpenAt(const char* path)
		{
			if (m_currentNode)
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
			if (*realPath)
			{
				DirectoryEntry* entry = SearchForNode(point->children.head, (void*)realPath, [](DirectoryEntry* current, void* userdata)->bool {
					return utils::strcmp(current->path.str, (const char*)userdata);
				});
				if (!entry)
				{
					SetLastError(OBOS_ERROR_VFS_FILE_NOT_FOUND);
					return false;
				}
				while (entry->direntType == DIRECTORY_ENTRY_TYPE_SYMLINK)
					entry = entry->linkedNode;
				m_directoryNode = entry;
				m_currentNode = entry->children.head;
			}
			else
			{
				m_directoryNode = point;
				m_currentNode = point->children.head;
			}
			return true;
		}

		const char* DirectoryIterator::operator*()
		{
			if (!m_currentNode)
				return nullptr;
			DirectoryEntry* entry = (DirectoryEntry*)m_currentNode;
			uint32_t mountId = entry->mountPoint->id;
			size_t szPath = logger::sprintf(
				nullptr,
				"%d:/%s",
				mountId,
				entry->path.str
			);
			char* path = new char[szPath + 1];
			logger::sprintf(
				path,
				"%d:/%s",
				mountId,
				entry->path.str
			);
			return path;
		}
		// ++iter;
		const char* DirectoryIterator::operator++()
		{
			if (!m_currentNode)
				return nullptr;
			DirectoryEntry* entry = (DirectoryEntry*)m_currentNode;
			m_currentNode = entry->next;
			return operator*();
		}
		// iter++;
		const char* DirectoryIterator::operator++(int)
		{
			if (!m_currentNode)
				return nullptr;
			DirectoryEntry* entry = (DirectoryEntry*)m_currentNode;
			const char* ret = operator*();
			m_currentNode = entry->next;
			if (!m_currentNode)
				return nullptr;
			return ret;
		}
		// --iter;
		const char* DirectoryIterator::operator--()
		{
			if (!m_currentNode)
				return nullptr;
			DirectoryEntry* entry = (DirectoryEntry*)m_currentNode;
			m_currentNode = entry->prev;
			if (!m_currentNode)
				return nullptr;
			const char* ret = operator*();
			return ret;
		}
		// iter--;
		const char* DirectoryIterator::operator--(int)
		{
			if (!m_currentNode)
				return nullptr;
			DirectoryEntry* entry = (DirectoryEntry*)m_currentNode;
			const char* ret = operator*();
			m_currentNode = entry->prev;
			return ret;
		}
	}
}