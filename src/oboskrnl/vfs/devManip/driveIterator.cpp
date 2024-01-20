/*
	oboskrnl/vfs/devManip/driveIterator.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <vfs/devManip/driveIterator.h>

#include <vfs/vfsNode.h>

namespace obos
{
	namespace vfs
	{
		DriveIterator::DriveIterator()
		{
			m_currentNode = g_drives.head;
		}

		const char* DriveIterator::operator*()
		{
			if (!m_currentNode)
				return nullptr;
			DriveEntry* entry = (DriveEntry*)m_currentNode;
			uint32_t driveId = entry->driveId;
			size_t szPath = logger::sprintf(nullptr, "D%d:/", driveId);
			char* path = new char[szPath + 1];
			utils::memzero(path, szPath + 1);
			logger::sprintf(path, "D%d:/", driveId);
			return path;
		}
		const char* DriveIterator::operator++()
		{
			m_currentNode = ((DriveEntry*)m_currentNode)->next;
			return operator*();
		}
		const char* DriveIterator::operator++(int)
		{
			auto ret = operator*();
			m_currentNode = ((DriveEntry*)m_currentNode)->next;
			return ret;
		}
		const char* DriveIterator::operator--()
		{
			m_currentNode = ((DriveEntry*)m_currentNode)->prev;
			return operator*();
		}
		const char* DriveIterator::operator--(int)
		{
			auto ret = operator*();
			m_currentNode = ((DriveEntry*)m_currentNode)->prev;
			return ret;
		}
	}
}