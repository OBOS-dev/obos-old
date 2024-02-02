/*
	oboskrnl/allocators/slab.cpp

	Copyright (c) 2024 Omar Berrow
*/

#include <int.h>
#include <new>

#include <allocators/vmm/vmm.h>
#include <allocators/slab.h>

#include <multitasking/thread.h>
#include <multitasking/process/process.h>
#include <multitasking/locks/mutex.h>
#include <vfs/vfsNode.h>
#include <driverInterface/struct.h>
#include <driverInterface/input_device.h>

#define DEFAULT_OBJECT_COUNT 32
extern obos::memory::VirtualAllocator g_liballocVirtualAllocator;

namespace obos
{
	locks::Mutex g_slabAllocatorLock;
	bool g_slabAllocatorInitialized;
	struct FreeListNode
	{
		alignas(0x10) struct FreeList* parent;
		alignas(0x10) FreeListNode *next, *prev;
		void Remove(FreeListNode*& head, FreeListNode*& tail, size_t &nNodes)
		{
			if (next)
				next->prev = prev;
			if (prev)
				prev->next = next;
			if (head == this)
				head = next;
			if (tail == this)
				tail = prev;
			nNodes--;
		}
		void PushBack(FreeListNode*& head, FreeListNode*& tail, size_t& nNodes)
		{
			if (tail)
				tail->next = this;
			if(!head)
				head = this;
			prev = tail;
			tail = this;
			nNodes++;
		}
		void PushFront(FreeListNode*& head, FreeListNode*& tail, size_t& nNodes)
		{
			if (head)
				head->prev = this;
			if (!tail)
				tail = this;
			next = head;
			head = this;
			nNodes++;
		}
	};
	struct FreeList
	{
		ObjectTypes type;
		FreeListNode *fhead, *ftail;
		size_t nFNodes;
		FreeListNode *ahead, *atail;
		size_t nANodes;
	};
	constexpr size_t g_objectTypesToSize[] = {
		sizeof(thread::Thread),
		sizeof(process::Process),
		sizeof(locks::Mutex),
		sizeof(vfs::MountPoint),
		sizeof(vfs::DirectoryEntry),
		sizeof(vfs::PartitionEntry),
		sizeof(vfs::DriveEntry),
		sizeof(driverInterface::InputDevice),
		sizeof(driverInterface::driverIdentity),
	};
	static void ExpandFreeList(FreeList* list, size_t nObjects, size_t szObject, size_t alignment = 0)
	{
		size_t realSzObject = szObject + sizeof(FreeListNode);
		if (alignment)
			realSzObject += (realSzObject % alignment);
		void* newBlock = g_liballocVirtualAllocator.VirtualAlloc(nullptr, nObjects * realSzObject, 0);
		for (size_t i = 0; i < nObjects; i++)
		{
			FreeListNode* currentNode = (FreeListNode*)((byte*)newBlock + i * realSzObject);
			new (currentNode) FreeListNode{};
			currentNode->parent = list;
			currentNode->PushBack(list->fhead, list->ftail, list->nFNodes);
		}
	}
	FreeList g_freeLists[9];
	void SlabInitialize()
	{
		new (&g_slabAllocatorLock) locks::Mutex{};
		for (size_t i = 0; i < sizeof(g_freeLists) / sizeof(g_freeLists[0]); i++)
		{
			ExpandFreeList(g_freeLists + i, DEFAULT_OBJECT_COUNT, g_objectTypesToSize[i], i == 0 ? 0x10 : 0);
			g_freeLists[i].type = (ObjectTypes)(i+1);
		}
		g_slabAllocatorInitialized = true;
	}
	void* ImplSlabAllocate(ObjectTypes _type, size_t nObjects)
	{
		if (!g_slabAllocatorInitialized)
			SlabInitialize();
		uint32_t type = (uint32_t)_type - 1;
		if (_type == ObjectTypes::Invalid || type > 8)
			return nullptr;
		const size_t sz = g_objectTypesToSize[type];
		FreeList* const list = &g_freeLists[type];
		if (!list->fhead)
			ExpandFreeList(list, DEFAULT_OBJECT_COUNT, sz);
		if (list->nFNodes < nObjects)
			ExpandFreeList(list, nObjects, sz);
		auto node = list->ftail;
		void* ret = node + 1;
		for (size_t i = 0; i < nObjects; i++)
		{
			node->Remove(list->fhead, list->ftail, list->nFNodes);
			node->PushBack(list->ahead, list->atail, list->nANodes);
		}
		return ret;
	}
	static bool HasNode(FreeList* list, void* obj)
	{
		for (auto node = list->ahead; node;)
		{
			if (node == obj)
				return true;

			node = node->next;
		}
		return true;
	}
	bool SlabHasObject(ObjectTypes _type, void* obj)
	{
		uint32_t type = (uint32_t)_type - 1;
		if (_type == ObjectTypes::Invalid || type > 8)
			return false;
		return HasNode(&g_freeLists[type], obj);
	}
	void ImplSlabFree(ObjectTypes _type, void* obj, size_t nObjects)
	{
		if (!g_slabAllocatorInitialized)
			return;
		uint32_t type = (uint32_t)_type - 1;
		if (_type == ObjectTypes::Invalid || type > 8)
			return;
		obj = (void*)((uintptr_t)obj - sizeof(FreeListNode));
		const size_t sz = g_objectTypesToSize[type];
		FreeList* const list = &g_freeLists[type];
		for (size_t i = 0; i < nObjects; i++)
			if (!HasNode(list, (FreeListNode*)( (byte*)obj + i * (sz + sizeof(FreeListNode)) ) ) )
				return;
		for (size_t i = 0; i < nObjects; i++)
		{
			FreeListNode* node = (FreeListNode*)((byte*)obj + i * (sz + sizeof(FreeListNode)));
			node->Remove(list->ahead, list->atail, list->nANodes);
			node->PushBack(list->fhead, list->ftail, list->nFNodes);
			utils::memzero(node + 1, sz);
		}
	}
}