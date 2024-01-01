/*
	drivers/generic/initrd/parse.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include "parse.h"

#include <driverInterface/struct.h>

#include <allocators/liballoc.h>


using namespace obos;



extern driverInterface::driverHeader g_driverHeader;

int oct2bin(char* str, int size)
{
	int n = 0;
	char* c = str;
	while (size-- > 0) {
		n *= 8;
		n += *c - '0';
		c++;
	}
	return n;
}

[[noreturn]] extern void kpanic(const char* format, ...);

filesystemCache g_filesystemCache = {};

void InitializeFilesystemCache()
{
	ustarEntry* entry = (ustarEntry*)g_driverHeader.initrdLocationResponse.addr;
	if (!obos::utils::memcmp(&entry->indication, "ustar", 6))
		obos::logger::panic(nullptr, "DRIVER 0, %s: Invalid or empty initrd image!\n", __func__);
	while (obos::utils::memcmp(&entry->indication, "ustar", 6))
	{
		size_t filesize = oct2bin(entry->filesizeOctal, 11);
		ustarEntryCacheNode* node = (ustarEntryCacheNode*)kcalloc(1, sizeof(ustarEntryCacheNode));
		ustarEntryCache* cache = node->cache = (ustarEntryCache*)kcalloc(1, sizeof(ustarEntryCache));
		obos::utils::memzero(node, sizeof(*node));
		cache->entry = entry;
		cache->entryFilesize = filesize;
		cache->dataStart = (uint8_t*)(entry + 1);
		if (filesize)
			cache->dataEnd = cache->dataStart + ((filesize / 512 + 1) * 512);
		else
			cache->dataEnd = cache->dataStart;
		node->cache = cache;
		switch (entry->type)
		{
		case ustarEntry::NORMAL_FILE:
			cache->entryAttributes = driverInterface::FILE_ATTRIBUTES_FILE | driverInterface::FILE_ATTRIBUTES_READ_ONLY;
			cache->entryFilesize++; // Add one to the filesize.
			break;
		case ustarEntry::DIRECTORY:
			cache->entryAttributes = driverInterface::FILE_ATTRIBUTES_DIRECTORY;
			break;
		case ustarEntry::HARD_LINK:
			cache->entryAttributes = driverInterface::FILE_ATTRIBUTES_HARDLINK;
			break;
		default:
			break;
		}
		if (g_filesystemCache.tail)
			g_filesystemCache.tail->next = node;
		if (!g_filesystemCache.head)
			g_filesystemCache.head = node;
		node->prev = g_filesystemCache.tail;
		g_filesystemCache.tail = node;
		g_filesystemCache.size++;
		entry = (ustarEntry*)cache->dataEnd;
	}
}

bool GetFileAttribute(const char* filepath, size_t* size, uint32_t* _attrib)
{
	ustarEntryCache* entry = GetCacheForPath(filepath);
	if (!entry)
	{
		if (_attrib)
			*_attrib = driverInterface::fileAttributes::FILE_DOESNT_EXIST;
		if (size)
			*size = 0;
		return false;
	}
	if (_attrib)
		*_attrib = entry->entryAttributes;
	if (size)
		*size = entry->entryFilesize;
	return true;
}
bool FileExists(const char* filepath)
{
	return GetCacheForPath(filepath) != nullptr;
}

ustarEntryCache* GetCacheForPath(const char* path)
{
	ustarEntryCacheNode* entry = g_filesystemCache.head;
	while (entry)
	{
		if (obos::utils::strcmp(path, entry->cache->entry->path))
			break;

		entry = entry->next;
	}

	return entry->cache;
}
