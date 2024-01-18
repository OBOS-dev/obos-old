/*
	drivers/generic/fat/cache.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <export.h>

#include <vfs/devManip/driveHandle.h>

#include <driverInterface/struct.h>

#include "cache.h"
#include "fat_structs.h"

using namespace obos;

namespace obos
{
	namespace vfs
	{
		OBOS_EXPORT void dividePathToTokens(const char* filepath, const char**& tokens, size_t& nTokens, bool useOffset = true);
	}
}

namespace fatDriver
{
	struct fileIterator
	{
		cacheEntry* currentNode;
		fileIterator* next, *prev; // The next file iterator in the file iterator list.
	};
	struct
	{
		fileIterator *head, *tail;
		size_t size;
	} g_iterators;
	static bool pathStrcmp(const char* p1, const char* p2)
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
	static cacheEntry* FindCacheEntry(partition& part, const char* path)
	{
		for (auto* ret = part.head; ret; )
		{
			if (pathStrcmp(ret->path, path))
				return ret;
			ret = ret->next;
		}
		return nullptr;
	}
	bool QueryFileProperties(
		const char* path,
		uint32_t driveId, uint32_t partitionIdOnDrive,
		size_t* oFsizeBytes,
		driverInterface::fileAttributes* oFAttribs)
	{
		partitionIdPair p;
		utils::memzero(&p, sizeof(p));
		p.first = driveId;
		p.second = partitionIdOnDrive;
		if (!g_partitionToIndex.contains(p))
			return false;
		auto& partition = g_partitions[g_partitionToIndex.at(p)];
		auto entry = FindCacheEntry(partition, path);
		if (!entry)
		{
			if (oFAttribs)
				*oFAttribs = driverInterface::FILE_DOESNT_EXIST;
			if (oFsizeBytes)
				*oFsizeBytes = 0;
			return true;
		}
		if (oFAttribs)
			*oFAttribs = (driverInterface::fileAttributes)entry->fileAttributes;
		if (oFsizeBytes)
			*oFsizeBytes = entry->filesize;
		return true;
	}
	bool FileIteratorCreate(
		uint32_t driveId, uint32_t partitionIdOnDrive,
		uintptr_t* oIter)
	{
		partitionIdPair p;
		utils::memzero(&p, sizeof(p));
		p.first = driveId;
		p.second = partitionIdOnDrive;
		if (!g_partitionToIndex.contains(p))
			return false;
		auto& partition = g_partitions[g_partitionToIndex.at(p)];
		fileIterator* newIter = new fileIterator;
		newIter->currentNode = partition.head;
		if (g_iterators.tail)
			g_iterators.tail->next = newIter;
		if (!g_iterators.head)
			g_iterators.head = newIter;
		newIter->prev = g_iterators.tail;
		g_iterators.tail = newIter;
		g_iterators.size++;
		*oIter = (uintptr_t)newIter;
		return true;
	}
	bool FileIteratorNext(
		uintptr_t _iter,
		const char** oFilepath,
		void(**freeFunction)(void* buf),
		size_t* oFsizeBytes,
		driverInterface::fileAttributes* oFAttribs)
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
			return true;
		}
		bool ret = QueryFileProperties(iter->currentNode->path, iter->currentNode->owner->driveId, iter->currentNode->owner->partitionId, oFsizeBytes, oFAttribs);
		if (!ret)
			return false;
		size_t szFilepath = obos::utils::strlen(iter->currentNode->path);
		if (oFilepath)
			*oFilepath = (const char*)obos::utils::memcpy(kcalloc(szFilepath + 1, 1), iter->currentNode->path, szFilepath);
		if (freeFunction)
			*freeFunction = kfree;
		iter->currentNode = iter->currentNode->next;
		return true;
	}
	bool FileIteratorClose(uintptr_t _iter)
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
		uint32_t driveId, uint32_t partitionIdOnDrive,
		const char* path,
		size_t nToSkip,
		size_t nToRead,
		char* buff)
	{
		partitionIdPair p;
		utils::memzero(&p, sizeof(p));
		p.first = driveId;
		p.second = partitionIdOnDrive;
		if (!g_partitionToIndex.contains(p))
			return false;
		auto& partition = g_partitions[g_partitionToIndex.at(p)];
		auto entry = FindCacheEntry(partition, path);
		if (!entry)
			return false;
		if (nToSkip >= entry->filesize)
			return false;
		if ((nToSkip + nToRead) > entry->filesize)
			return false;
		auto bpb = partition.bpb;
		// Read the appropriate clusters.
		size_t bytesPerCluster = ((size_t)bpb->sectorsPerCluster * bpb->bytesPerSector);
		size_t nClusters = nToRead / bytesPerCluster + ((nToRead % bytesPerCluster) != 0);
		uint32_t clusterIndex = nToSkip / bytesPerCluster;
		vfs::DriveHandle drv;
		size_t drvPathSz = logger::sprintf(nullptr, "D%dP%d:/", driveId, partitionIdOnDrive) + 1;
		char* drvPath = new char[drvPathSz];
		utils::memzero(drvPath, drvPathSz);
		logger::sprintf(drvPath, "D%dP%d:/", driveId, partitionIdOnDrive);
		drv.OpenDrive(drvPath);
		delete[] drvPath;
		size_t sectorSize = 0;
		drv.QueryInfo(nullptr, &sectorSize, nullptr);
		char* fbuffer = new char[bytesPerCluster * nClusters];
		switch (partition.fat_type)
		{
		case fatType::FAT32:
		{
			char* currentCluster = fbuffer;
			for (size_t i = clusterIndex; i < nClusters; i++, currentCluster += bytesPerCluster)
			{
				uint64_t sector = fat32FirstSectorOfCluster(entry->clusters[i], *bpb, partition.FirstDataSec);
				if (!drv.ReadSectors(currentCluster, nullptr, sector, bpb->sectorsPerCluster))
				{
					delete[] currentCluster;
					return false;
				}
			}
			break;
		}
		// TODO: Implement FAT12 and FAT16
		default:
			break;
		}
		vfs::uoff_t initialBufferOffset = (nToSkip % bytesPerCluster);
		size_t nToCopy = nToRead;
		utils::memcpy(buff, fbuffer + initialBufferOffset, nToCopy);
		return true;
	}
}