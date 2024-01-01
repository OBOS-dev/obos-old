/*
	drivers/generic/initrd/interface.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <memory_manipulation.h>

#include <driverInterface/struct.h>

#include "interface.h"
#include "parse.h"

#include <allocators/liballoc.h>

struct
{
	fileIterator* head, * tail;
	size_t size;
} g_iterators;

namespace initrdInterface
{
	bool QueryFileProperties(
		const char* path,
		uint64_t, uint8_t,
		size_t* oFsizeBytes,
		uint64_t* oLBAOffset,
		obos::driverInterface::fileAttributes* oFAttribs)
	{
		if (oLBAOffset)
			*oLBAOffset = 0;
		uint32_t fAttribs = 0;
		if (!::GetFileAttribute(path, oFsizeBytes, &fAttribs))
		{
			if (oFAttribs)
				*oFAttribs = obos::driverInterface::FILE_DOESNT_EXIST;
			if (oFsizeBytes)
				*oFsizeBytes = 0;
			if (oLBAOffset)
				*oLBAOffset = 0;
			return false;
		}
		if (oFAttribs)
			*oFAttribs = (obos::driverInterface::fileAttributes)fAttribs;
		return true;

	}
	bool IterCreate(
		uint64_t, uint8_t,
		uintptr_t* oIter)
	{
		fileIterator* iter = (fileIterator*)kcalloc(1, sizeof(fileIterator));
		if (!iter)
			return false;
		iter->currentNode = g_filesystemCache.head;
		if (g_iterators.tail)
			g_iterators.tail->next = iter;
		if (!g_iterators.head)
			g_iterators.head = iter;
		iter->prev = g_iterators.tail;
		g_iterators.tail = iter;
		g_iterators.size++;
		*oIter = (uintptr_t)iter;
		return true;
	}
	bool IterNext(
		uintptr_t _iter,
		const char** oFilepath,
		void(**freeFunction)(void* buf),
		size_t* oFsizeBytes,
		uint64_t* oLBAOffset,
		obos::driverInterface::fileAttributes* oFAttribs)
	{
		fileIterator* iter = (fileIterator*)_iter;
		fileIterator* _node = g_iterators.head;
		while (_node)
		{
			if (_node == iter)
				break;

			_node = _node->next;
		}
		if (_node != iter)
			return false;
		if (!iter->currentNode)
		{
			if (oFAttribs)
				*oFAttribs = obos::driverInterface::FILE_DOESNT_EXIST;
			if (oFsizeBytes)
				*oFsizeBytes = 0;
			if (oLBAOffset)
				*oLBAOffset = 0;
			return true;
		}
		bool ret = QueryFileProperties(iter->currentNode->cache->entry->path, 0,0, oFsizeBytes, oLBAOffset, oFAttribs);
		if (!ret)
			return false;
		size_t szFilepath = obos::utils::strlen(iter->currentNode->cache->entry->path);
		if (oFilepath)
			*oFilepath = (const char*)obos::utils::memcpy(kcalloc(szFilepath + 1, 1), iter->currentNode->cache->entry->path, szFilepath);
		if (freeFunction)
			*freeFunction = kfree;
		iter->currentNode = iter->currentNode->next;
		return true;
	}
	bool IterClose(uintptr_t _iter)
	{
		fileIterator* iter = (fileIterator*)_iter;
		fileIterator* _node = g_iterators.head;
		while (_node)
		{
			if (_node == iter)
				break;
			_node = _node->next;
		}
		if (_node != iter)
			return false;
		if (iter->next)
			iter->next->prev = iter->prev;
		if (iter->prev)
			iter->prev->next = iter->next;
		if (g_iterators.head == iter)
			g_iterators.head = iter->next;
		if (g_iterators.tail == iter)
			g_iterators.tail = iter->prev;
		g_iterators.size--;
		kfree(iter);
		return true;
	}
	bool ReadFile(
		uint64_t, uint8_t,
		const char* path,
		size_t nToSkip,
		size_t nToRead,
		char* buff)
	{
		auto cache = GetCacheForPath(path);
		if (!cache)
			return false;
		if ((cache->dataStart + nToSkip) >= cache->dataEnd)
			return false;
		if ((cache->dataStart + nToSkip + nToRead) >= cache->dataEnd)
			return false;
		if (buff)
			obos::utils::memcpy(buff, cache->dataStart + nToSkip, nToRead);
		return true;
	}
}