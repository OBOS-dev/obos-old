/*
	oboskrnl/process/process.h

	Copyright (c) 2023 Omar Berrow
*/

#include <process/process.h>

#include <memory_manager/paging/init.h>
#include <memory_manager/physical.h>

#include <multitasking/multitasking.h>
#include <multitasking/threadHandle.h>
#include <multitasking/thread.h>

#include <multitasking/mutex/mutexHandle.h>

#include <elf/elf.h>

#include <inline-asm.h>
#include <handle.h>
#include <types.h>
#include <error.h>
#include <klog.h>

#include <utils/memory.h>
#include <utils/bitfields.h>
#include <utils/list.h>

extern "C" UINTPTR_T boot_page_directory1;
extern "C" UINTPTR_T boot_page_level4_map;
extern "C" UINTPTR_T boot_page_table1;

namespace obos
{
	extern process::Process* g_kernelProcess;
	extern char kernelStart;
	namespace memory
	{
		extern UINTPTR_T* kmap_pageTable(PVOID physicalAddress);
		extern UINTPTR_T g_pageTableMapPageDirectory[512] alignas(4096);
	}
	namespace process
	{
		DWORD g_nextProcessId = 0;
		list_t* g_processList = nullptr;
		UINTPTR_T ProcEntryPointBase = 0;

		Process::Process()
		{}

		VOID attribute(section(".glb.text")) ProcEntryPoint(PVOID entry)
		{
			DWORD(*main)() = (DWORD(*)())entry;
			main();
			// Call syscall 1 (ExitProcess) as we cannot call functions located in the kernel in user mode.
#if defined (__i686__)
			asm volatile("push %eax; mov $1, %eax; mov %esp, %ebx; int $0x40;");
#elif defined (__x86_64__)
			asm volatile("push %rax; mov $1, %rax; mov %rsp, %rbx; int $0x40;");
#endif
		}
		VOID DriverEntryPoint(PVOID entry)
		{
			DWORD(*main)() = (DWORD(*)())entry;
			DWORD ret = main();
			if (ret != 0)
			{
				printf("\r\nDriver with pid %d failed to initialize with exit code %d.\r\n", multitasking::g_currentThread->owner->pid, ret);
				// In case the driver disabled interrupts and they didn't get enabled.
				asm volatile("sti" : : : "memory");
				multitasking::g_currentThread->owner->TerminateProcess(ret);
			}
			auto currentThread = multitasking::GetCurrentThreadHandle();
			currentThread->SetThreadPriority(multitasking::Thread::priority_t::IDLE);
			currentThread->closeHandle();
			delete currentThread;
			// In case the driver disabled interrupts and they didn't get enabled.
			asm volatile("sti" : : : "memory");
			// Do this if someone thinks they're funny and unpauses the thread
			while(1)
			{
				multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::PAUSED;
				_int(0x30);
			}
		}

		DWORD Process::CreateProcess(PBYTE file, SIZE_T size, PVOID _mainThread, bool isDriver)
		{
#if defined(__i686__)
			if (pageDirectory)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return OBOS_ERROR_NO_SUCH_OBJECT;
			}
#elif defined(__x86_64__)
			if (level4PageMap)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return OBOS_ERROR_NO_SUCH_OBJECT;
			}
#endif
			if (!g_processList)
				g_processList = list_new();
			EnterKernelSection();
			pid = g_nextProcessId++;
#if defined(__i686__)
			pageDirectory = new memory::PageDirectory{};
			UINTPTR_T* newPageDirectory = memory::kmap_pageTable(pageDirectory->getPageDirectory());
			// We need to make sure the page directory is empty.
			// Otherwise, you'll have a bad time.
			utils::memzero(newPageDirectory, 4096);
			UINTPTR_T* pBoot_page_directory1 = &boot_page_directory1;
			pBoot_page_directory1 += 0x30000000/*0xC0000000*/;
			// Clone the kernel page directory.
			for (int i = 0; i < 1024; i++)
				if (pBoot_page_directory1[i])
					newPageDirectory[i] = pBoot_page_directory1[i];

			pageDirectory->switchToThis();
#elif defined(__x86_64__)
			level4PageMap = new memory::PageMap{};
			UINTPTR_T* newPageMap = memory::kmap_pageTable(level4PageMap->getPageMap());
			utils::dwMemset((DWORD*)newPageMap, 0, 1024);
			UINTPTR_T* pBoot_page_directory1 = &boot_page_level4_map;
			pBoot_page_directory1 += 0x1ffffffff0000000/*0xffffffff80000000*/;
			// Clone the page map.
			for (int l4 = 1; l4 < 512; l4++)
				if (pBoot_page_directory1[l4])
					newPageMap[l4] = pBoot_page_directory1[l4];
			UINTPTR_T l3PageMap = (UINTPTR_T)memory::kalloc_physicalPage();
			utils::memzero(memory::kmap_pageTable((PVOID)l3PageMap), 4096);
			newPageMap = memory::kmap_pageTable(level4PageMap->getPageMap());
			newPageMap[0] = l3PageMap | 7;/*
			UINTPTR_T* l3PageMapPtr = (UINTPTR_T*)memory::kmap_pageTable((PVOID)l3PageMap);
			UINTPTR_T* pageDirectory = (UINTPTR_T*)memory::kalloc_physicalPage();
			memory::kmap_pageTable((PVOID)l3PageMap);
			l3PageMapPtr[0] = (UINTPTR_T)pageDirectory;
			l3PageMapPtr[0] |= 7;
			UINTPTR_T* pageDirectoryPtr = memory::kmap_pageTable(pageDirectory);
			pageDirectoryPtr[0] = (UINTPTR_T)&boot_page_table1;
			pageDirectoryPtr[0] |= 3;
			utils::memzero(pageDirectoryPtr + 1, 511 * sizeof(UINTPTR_T));*/
			{
				UINTPTR_T* pmap = memory::kmap_pageTable(level4PageMap->getPageMap());
				UINTPTR_T flags = (static_cast<UINTPTR_T>(memory::CPUSupportsExecuteDisable()) << 63) | 3;
				reinterpret_cast<UINTPTR_T*>(pmap[0] & 0xFFFFFFFFFF000)[3]
					= (reinterpret_cast<UINTPTR_T>(memory::g_pageTableMapPageDirectory) - reinterpret_cast<UINTPTR_T>(&kernelStart)) | flags;
			}
			level4PageMap->switchToThis();

			UINTPTR_T glbTextPos = memory::g_kernelPageMap.getPageTableEntry(
				memory::PageMap::computeIndexAtAddress(ProcEntryPointBase, 3),
				memory::PageMap::computeIndexAtAddress(ProcEntryPointBase, 2),
				memory::PageMap::computeIndexAtAddress(ProcEntryPointBase, 1),
				memory::PageMap::computeIndexAtAddress(ProcEntryPointBase, 0));
			memory::kmap_physical((PVOID)ProcEntryPointBase, 6, (PVOID)glbTextPos, true);
#endif

			children = list_new();
			threads = list_new();
			abstractHandles = list_new();
			allocatedBlocks = list_new();
			
			allocatedBlocks->match = [](PVOID _block1, PVOID _block2)->int {
				allocatedBlock* block1 = reinterpret_cast<allocatedBlock*>(_block1);
				allocatedBlock* block2 = reinterpret_cast<allocatedBlock*>(_block2);
				return block1->start == block2->start &&
					   block1->size == block2->size;
				};

			Process* oldThreadOwner = multitasking::g_currentThread->owner;
			multitasking::g_currentThread->owner = this;

			UINTPTR_T entry = 0;
			UINTPTR_T base = 0;

			DWORD loaderRet = elfLoader::LoadElfFile(file, size, entry, base);
			if (loaderRet)
			{
				UncommitProcessMemory();
				multitasking::g_currentThread->owner = oldThreadOwner;
				multitasking::g_currentThread->owner->doContextSwitch();
				LeaveKernelSection();
				return loaderRet;
			}

			isUserMode = !isDriver;

			multitasking::Thread* mainThread = new multitasking::Thread{};
			
			DWORD ret = mainThread->CreateThread(multitasking::Thread::priority_t::NORMAL,
				!isDriver ? (VOID(*)(PVOID))ProcEntryPointBase : DriverEntryPoint,
				(PVOID)entry,
				(DWORD)multitasking::Thread::status_t::PAUSED,
				0);
			if (ret)
			{
				multitasking::g_currentThread->owner = oldThreadOwner;
				multitasking::g_currentThread->owner->doContextSwitch();
				LeaveKernelSection();
				return ret;
			}

			mainThread->owner = this;
			parent = oldThreadOwner;

			list_rpush(threads, list_node_new(mainThread));
			list_rpush(oldThreadOwner->children, list_node_new(this));

			if (_mainThread)
			{
				multitasking::ThreadHandle* mainThreadHandle = (multitasking::ThreadHandle*)_mainThread;
				mainThreadHandle->OpenThread(mainThread);
			}
			
			multitasking::g_currentThread->owner = oldThreadOwner;
			// Switch back the parent's page directory.
			multitasking::g_currentThread->owner->doContextSwitch();

			list_rpush(g_processList, list_node_new(this));

			LeaveKernelSection();

			mainThread->status &= ~((DWORD)multitasking::Thread::status_t::PAUSED);

			return OBOS_ERROR_NO_ERROR;
		}
		/*static BYTE s_temporaryStack[8192] attribute(aligned(4096));
		static multitasking::MutexHandle* s_temporaryStackMutex = nullptr;*/
		static DWORD s_exitCode = 0;
		DWORD Process::TerminateProcess(DWORD exitCode, bool canExitThread)
		{
#if defined(__i686__)
			if (!pageDirectory)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return OBOS_ERROR_NO_SUCH_OBJECT;
			}
#elif defined(__x86_64__)
			if (!level4PageMap)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return OBOS_ERROR_NO_SUCH_OBJECT;
			}
#endif
			EnterKernelSection();
			doContextSwitch();
			// Iterate over the children.
			list_iterator_t* iter = list_iterator_new(children, LIST_HEAD);
			for (list_node_t* _child = list_iterator_next(iter); _child != nullptr; _child = list_iterator_next(iter))
			{
				Process* child = (Process*)_child->val;
				// The kernel is very nice and is adopting the children.
				child->parent = g_kernelProcess;
			}
			list_iterator_destroy(iter);
			// As for the threads, they die.
			iter = list_iterator_new(threads, LIST_HEAD);
			for (list_node_t* _thread = list_iterator_next(iter); _thread != nullptr; _thread = list_iterator_next(iter))
			{
				multitasking::ThreadHandle* threadHandle = new multitasking::ThreadHandle{};
				threadHandle->OpenThread((multitasking::Thread*)_thread->val);
				threadHandle->TerminateThread(0);
				threadHandle->closeHandle();
				delete threadHandle;
			}
			list_iterator_destroy(iter);
			iter = list_iterator_new(abstractHandles, LIST_HEAD);
			// Same for the rest of the handles.
			for (list_node_t* _handle = list_iterator_next(iter); _handle != nullptr; _handle = list_iterator_next(iter))
			{
				Handle* handle = (Handle*)_handle->val;
				handle->closeHandle();
			}
			list_iterator_destroy(iter);
			list_destroy(children);
			list_destroy(threads);
			list_destroy(abstractHandles);
			list_remove(g_processList, list_find(g_processList, this));
			s_exitCode = exitCode;

			// Free everything.
			UncommitProcessMemory();

#if defined(__i686__)
			delete pageDirectory;
#elif defined(__x86_64__)
			delete level4PageMap;
#endif
			LeaveKernelSection();
			this->exitCode = s_exitCode;
			if (this == multitasking::g_currentThread->owner && canExitThread)
			{
				//s_temporaryStackMutex->Unlock();
				multitasking::ExitThread(s_exitCode);
			}
			return OBOS_ERROR_NO_ERROR;
		}

		void Process::doContextSwitch()
		{
#if defined(__i686__)
			pageDirectory->switchToThis();
#elif defined(__x86_64__)
			level4PageMap->switchToThis();
#endif
		}
#if defined(__x86_64__)
		void Process::UncommitProcessMemory()
		{
			if (!this->allocatedBlocks)
				return;
			list_iterator_t* iter = list_iterator_new(allocatedBlocks, LIST_HEAD);
			for (list_node_t* node = list_iterator_next(iter); node != nullptr; node = list_iterator_next(iter))
			{
				allocatedBlock* block = (allocatedBlock*)node->val;
				for (SIZE_T page = 0; page < block->size; page++)
				{
					UINTPTR_T phys = memory::g_level4PageMap->getPageTableEntry(
						memory::PageMap::computeIndexAtAddress(reinterpret_cast<UINTPTR_T>(block->start + (page << 12)), 3),
						memory::PageMap::computeIndexAtAddress(reinterpret_cast<UINTPTR_T>(block->start + (page << 12)), 2),
						memory::PageMap::computeIndexAtAddress(reinterpret_cast<UINTPTR_T>(block->start + (page << 12)), 1),
						memory::PageMap::computeIndexAtAddress(reinterpret_cast<UINTPTR_T>(block->start + (page << 12)), 0)
					);
					if (phys != reinterpret_cast<UINTPTR_T>(memory::g_zeroPage))
						memory::kfree_physicalPage((PVOID)phys);
				}
			}
		}
#elif defined(__i686__)
		void Process::UncommitProcessMemory()
		{
			if (!this->allocatedBlocks)
				return;
			list_iterator_t* iter = list_iterator_new(allocatedBlocks, LIST_HEAD);
			for (list_node_t* node = list_iterator_next(iter); node != nullptr; node = list_iterator_next(iter))
			{
				allocatedBlock* block = (allocatedBlock*)node->val;
				for (SIZE_T page = 0; page < block->size; page++)
				{
					UINTPTR_T phys = memory::g_pageDirectory->getPageTableAddress(memory::PageDirectory::addressToIndex(reinterpret_cast<UINTPTR_T>(block->start + (page << 12))))[
						memory::PageDirectory::addressToPageTableIndex(memory::PageDirectory::addressToPageTableIndex(reinterpret_cast<UINTPTR_T>(block->start + (page << 12))))];
					memory::kfree_physicalPage((PVOID)phys);
				}
			}
		}
#endif
	}
}