/*
	 oboskrnl/arch/x86_64/memory_manager/virtual/mapFile.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <arch/x86_64/memory_manager/virtual/internal.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <allocators/vmm/arch.h>

#include <multitasking/cpu_local.h>

#include <vfs/vfsNode.h>

namespace obos
{
	namespace memory
	{
		void* _Impl_ProcMapFileNodeToAddress(
			process::Process* proc,
			void* _base,
			size_t size,
			uintptr_t protFlags,
			[[maybe_unused]] vfs::DirectoryEntry* entry,
			[[maybe_unused]] uintptr_t off,
			uint32_t* status)
		{
			// Don't actually map anything until a page fault happens on these pages.
			// Set bit 10 which is avaliable for the kernel to use to indicate to PagesAllocated() and other functions that the page is uncommitted.
			// On page-fault, the page is mapped with the correct protection flags and is loaded into memory.
			// If the file is written to through a file handle (unimplemented as of February 10th, 2024), the region should be updated accordingly.
			
			// TODO: Look for (committed) memory mapped regions of the same directory entry, offset, and size and use some sort of shared memory.
			// TODO: When file writing is implemented, allow this to write 
			if (!_Impl_IsValidAddress(_base))
			{
				*status = VALLOC_INVALID_PARAMETER;
				return nullptr;
			}
			if (!(protFlags & PROT_READ_ONLY))
			{
				*status = MAPFILESTATUS_UNIMPLEMENTED;
				return nullptr;
			}
			auto [pageMap, isUserProcess] = GetPageMapFromProcess(proc);
			if (!proc)
				proc = thread::GetCurrentCpuLocalPtr() ? (process::Process*)thread::GetCurrentCpuLocalPtr()->currentThread->owner : nullptr;
			if (!proc)
			{
				*status = MAPFILESTATUS_INVALID_PARAMETER;
				return nullptr;
			}
			const size_t nPages = size / 4096 + ((size % 4096) != 0);
			if ((uintptr_t)_base > 0xffff800000000000 && isUserProcess)
			{
				*status = VALLOC_ACCESS_DENIED;
				return nullptr;
			}
			if (!memory::CanAllocatePages(_base, nPages, pageMap))
			{
				*status = MAPFILESTATUS_BASE_ADDRESS_USED;
				return nullptr;
			}
			uintptr_t base = (uintptr_t)_base;
			size_t sz = size;
			for (uintptr_t addr = base, offset = 0; addr < (base + nPages * 4096); addr += 4096, sz -= 4096, offset += 4096)
			{
				proc->context.memoryMappedFiles.emplace_at((void*)addr, { _base, (void*)addr, entry, offset, sz, true });
				uintptr_t* pageTable = allocatePagingStructures(addr, pageMap, 1);
				pageTable[PageMap::addressToIndex(addr, 0)] = ((uintptr_t)1<<10) | (protFlags << 52);
			}
			return _base;
		}
		bool mapFilePFHandler(uintptr_t addr, memory::PageMap* pageMap, uintptr_t errorCode)
		{
			process::Process* proc = (process::Process*)thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			if (!proc->context.memoryMappedFiles.contains((void*)addr))
				return false;
			uintptr_t l1Entry = (uintptr_t)pageMap->getL1PageMapEntryAt(addr);
			uintptr_t protFlags = l1Entry >> 52;
			uintptr_t flags = DecodeProtectionFlags(protFlags) | 1;
			if (errorCode & ((uintptr_t)1 << 4) /* execution fault */ && !(protFlags & PROT_CAN_EXECUTE) /* and the protection flags don't say we can execute... */)
				return false; // Fail.
			if (errorCode & ((uintptr_t)1 << 1) /* write fault */ && (protFlags & PROT_READ_ONLY) /* and the protection flags say that this is a RO page... */)
				return false; // Fail.
			if (errorCode & ((uintptr_t)1 << 2) /* write fault */ && (protFlags & PROT_USER_MODE_ACCESS) /* and the protection flags say that this is a RO page... */)
				return false; // Fail.
			// A valid operation was done on this page, map it in.
			auto& node = proc->context.memoryMappedFiles.at((void*)addr);
			if (node.isDirent)
			{
				vfs::DirectoryEntry* dirent = (vfs::DirectoryEntry*)node.dirent;
				if (dirent->fileAttrib & driverInterface::FILE_ATTRIBUTES_READ_ONLY && errorCode & ((uintptr_t)1 << 1))
					return false;
				auto &ftable = dirent->mountPoint->filesystemDriver->functionTable.serviceSpecific.filesystem;
				uint64_t driveId = dirent->mountPoint->partition ? dirent->mountPoint->partition->drive->driveId : 0;
				uint8_t drivePartitionId = dirent->mountPoint->partition ? dirent->mountPoint->partition->partitionId : 0;
				[[maybe_unused]] uint32_t status = 0;
				uintptr_t page = allocatePhysicalPage();
				l1Entry = page | flags;
				char* fileData = (char*)MapPhysicalAddress(pageMap, page, _Impl_FindUsableAddress(nullptr, 1), 3);
				ftable.ReadFile(driveId, drivePartitionId, dirent->path, node.off & ~0xfff, dirent->filesize < 4096 ? dirent->filesize : 4096, (char*)fileData);
				UnmapAddress(pageMap, fileData);
			}
			else
			{
				vfs::PartitionEntry* part = (vfs::PartitionEntry*)node.dirent;
				if (part->filesystemDriver)
					return false;
				auto &ftable = part->filesystemDriver->functionTable.serviceSpecific.storageDevice;
				uint64_t driveId = part->drive->driveId;
				[[maybe_unused]] uint32_t status = 0;
				uintptr_t page = allocatePhysicalPage();
				l1Entry = page | flags;
				size_t sectorSize = 0;
				ftable.QueryDiskInfo(driveId, nullptr, &sectorSize);
				size_t lbaOffset = node.off / sectorSize;
				void* buff = nullptr;
				ftable.ReadSectors(driveId, part->lbaOffset + lbaOffset, 4096/sectorSize, &buff, nullptr);
				_Impl_ProcVirtualFree(nullptr, buff, 1, &status);
			}
			MapEntry(pageMap, l1Entry, (void*)(addr & ~0xfff));
			return true;
		}
	}
}