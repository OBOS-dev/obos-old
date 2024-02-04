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
		void dividePathToTokens(const char* filepath, const char**& tokens, size_t& nTokens, bool useOffset = true);
		bool pathStrcmp(const char* p1, const char* p2)
		{
			for (; *p1 == '/'; p1++);
			for (; *p2 == '/'; p2++);
			const char** p1Tok = nullptr;
			size_t nP1Tok = 0;
			const char** p2Tok = nullptr;
			size_t nP2Tok = 0;
			vfs::dividePathToTokens(p1, p1Tok, nP1Tok, false);
			vfs::dividePathToTokens(p2, p2Tok, nP2Tok, false);
			bool ret = true;
			if (nP1Tok != nP2Tok)
			{
				ret = false;
				goto finish;
			}
			for (size_t i = 0; i < nP1Tok; i++)
			{
				if (!utils::strcmp(p1Tok[i], p2Tok[i]))
				{
					ret = false;
					goto finish;
				}
			}
		finish:
			for (size_t i = 0; i < nP1Tok; i++)
				delete[] p1Tok[i];
			for (size_t i = 0; i < nP2Tok; i++)
				delete[] p2Tok[i];
			delete[] p1Tok;
			delete[] p2Tok;
			return ret;
		}
		extern bool strContains(const char* str, char ch);
		extern uint32_t getMountId(const char* path, size_t size = 0);
		extern DirectoryEntry* SearchForNode(DirectoryEntry* root, void* userdata, bool(*compare)(DirectoryEntry* current, void* userdata));
		bool DirectoryIterator::OpenAt(const char* path)
		{
			if (m_currentNode || m_directoryNode)
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
					return pathStrcmp(current->path.str, (const char*)userdata);
				});
				if (!entry)
				{
					SetLastError(OBOS_ERROR_VFS_FILE_NOT_FOUND);
					return false;
				}
				if (entry->direntType != DIRECTORY_ENTRY_TYPE_DIRECTORY)
				{
					SetLastError(OBOS_ERROR_VFS_INVALID_OPERATION_ON_OBJECT);
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
		bool DirectoryIterator::Close()
		{
			if (!m_currentNode || !m_directoryNode)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			m_currentNode = nullptr; 
			m_directoryNode = nullptr;
			return true;
		}
	}
}