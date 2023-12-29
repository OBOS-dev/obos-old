/*
	drivers/generic/initrd/parse.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <driverInterface/struct.h>

struct ustarEntry
{
	char path[100];
	byte unused1[24];
	char filesizeOctal[12];
	byte unused2[12];
	uint64_t unused3;
	enum __type
	{
		NORMAL_FILE = '0',
		HARD_LINK = '1',
		SYM_LINK = '2',
		CHAR_DEVICE = '3',
		BLOCK_DEVICE = '4',
		DIRECTORY = '5',
		NAMED_PIPE = '6',
	} __attribute__((packed));
	__type type;
	char linkedFile[100];
	char indication[6]; // should be "ustar\0"
	char version[2];
	char unused4[80];
	char filenamePrefix[155];
	char padding[12];
} __attribute__((packed));

struct ustarEntryCache
{
	ustarEntry* entry;
	size_t entryFilesize;
	uint32_t entryAttributes;
	byte* dataStart;
	byte* dataEnd;
};
struct ustarEntryCacheNode 
{
	ustarEntryCacheNode *next, *prev;
	ustarEntryCache* cache;
};
struct fileIterator
{
	ustarEntryCacheNode* currentNode;
	fileIterator *next, *prev; // The next file iterator in the file iterator list.
};
struct filesystemCache
{
	ustarEntryCacheNode *head, *tail;
	size_t size;
} extern g_filesystemCache;

void InitializeFilesystemCache();

bool GetFileAttribute(const char* filepath, size_t* size, uint32_t* attrib); // OBOS_SERVICE_QUERY_FILE_DATA
bool FileExists(const char* path);
ustarEntryCache* GetCacheForPath(const char* path);