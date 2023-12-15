/*
	drivers/generic/initrd/parse.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include "parse.h"

#include <int.h>

#include <driverInterface/struct.h>

#include <driverInterface/x86_64/call_interface.h>

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
	if (!g_driverHeader.memoryManipFunctionsResponse.memcmp(&entry->indication, "ustar", 6))
		kpanic("[DRIVER 0, FATAL] %s: Invalid or empty initrd image!\n", __func__);
	while (g_driverHeader.memoryManipFunctionsResponse.memcmp(&entry->indication, "ustar", 6))
	{
		size_t filesize = oct2bin(entry->filesizeOctal, 11);
		ustarEntryCacheNode* node = (ustarEntryCacheNode*)Malloc(sizeof(ustarEntryCacheNode));
		ustarEntryCache* cache = node->cache = (ustarEntryCache*)Malloc(sizeof(ustarEntryCache));
		g_driverHeader.memoryManipFunctionsResponse.memzero(node, sizeof(*node));
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

void GetFileAttribute(const char* filepath, size_t* size, uint32_t* _attrib)
{
	ustarEntryCache* entry = GetCacheForPath(filepath);
	if (_attrib)
		*_attrib = entry->entryAttributes;
	if (size)
		*size = entry->entryFilesize;
	return;
}

void ReadFile(const char* filepath, size_t, size_t* szRead, char* dataRead)
{
	ustarEntryCache* entry = GetCacheForPath(filepath);
	if (!entry)
		return;
	if (szRead)
		*szRead = entry->entryFilesize;
	if (dataRead)
		g_driverHeader.memoryManipFunctionsResponse.memcpy(dataRead, entry->dataStart, entry->entryFilesize);
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
		if (g_driverHeader.memoryManipFunctionsResponse.strcmp(path, entry->cache->entry->path))
			break;

		entry = entry->next;
	}

	return entry->cache;
}
