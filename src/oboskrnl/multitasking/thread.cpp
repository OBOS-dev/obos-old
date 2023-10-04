/*
	thread.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <multitasking/thread.h>
#include <multitasking/multitasking.h>

#include <inline-asm.h>
#include <error.h>

#include <memory_manager/paging/allocate.h>

#define RFLAGS_IF 0x200

namespace obos
{
	namespace multitasking
	{
		//extern void ThreadEntryPoint();

		UINT32_T g_nextThreadTid = 1;
		Thread::Thread(priority_t threadPriority, VOID(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus, SIZE_T stackSizePages)
		{
			CreateThread(threadPriority, entry, userData, threadStatus, stackSizePages);
		}

		DWORD Thread::CreateThread(priority_t threadPriority, VOID(*entry)(PVOID userData), PVOID userData, utils::RawBitfield threadStatus, SIZE_T stackSizePages)
		{
			list_t* priorityList = nullptr;

			switch (threadPriority)
			{
			case obos::multitasking::Thread::priority_t::IDLE:
				priorityList = g_threadPriorityList[0];
				break;
			case obos::multitasking::Thread::priority_t::LOW:
				priorityList = g_threadPriorityList[1];
				break;
			case obos::multitasking::Thread::priority_t::NORMAL:
				priorityList = g_threadPriorityList[2];
				break;
			case obos::multitasking::Thread::priority_t::HIGH:
				priorityList = g_threadPriorityList[3];
				break;
			default:
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
				break;
			}

			EnterKernelSection();

			status = threadStatus | (UINT32_T)status_t::RUNNING;
			status &= ~((UINT32_T)status_t::DEAD);
			status &= ~((UINT32_T)status_t::BLOCKED);

			priority = threadPriority;
			iterations = 0;

			tid = g_nextThreadTid++;
			exitCode = 0;

			if (!stackSizePages)
				stackSizePages = 2;

			lastTimeRan = g_timerTicks;

			// Setting up the registers is platform specific.

#if defined(__i686__)
			UINT32_T* stack = (UINT32_T*)memory::VirtualAlloc(nullptr, stackSizePages, memory::VirtualAllocFlags::WRITE_ENABLED | memory::VirtualAllocFlags::GLOBAL);
			if (!stack)
				return OBOS_ERROR_NO_MEMORY;
			stackBottom = stack;
			this->stackSizePages = stackSizePages;
			stack += stackSizePages * 1024;
			stack -= 1;
			*stack = (UINT32_T)userData;
			stack -= 1;
			*stack = (UINT32_T)entry;

			frame.esp = (UINTPTR_T)stack;
			frame.eip = (UINTPTR_T)entry;
			frame.ebp = 0;
			utils::setBitInBitfield(frame.eflags, 9);
#elif defined(__x86_64__)
			UINTPTR_T* stack = (UINTPTR_T*)memory::VirtualAlloc(nullptr, stackSizePages, memory::VirtualAllocFlags::WRITE_ENABLED | memory::VirtualAllocFlags::GLOBAL, true);
			if (!stack)
			{
				SetLastError(OBOS_ERROR_NO_MEMORY);
				return OBOS_ERROR_NO_MEMORY;
			}
			stackBottom = stack;
			this->stackSizePages = stackSizePages;
			stack += stackSizePages * 512;
			stack -= 1;
			*stack = (UINTPTR_T)nullptr;
			stack -= 1;
			*stack = (UINTPTR_T)entry;
			
			frame.rsp = (UINTPTR_T)stack;
			frame.rip = (UINTPTR_T)entry;
			frame.rdi = (UINTPTR_T)userData;
			frame.rbp = 0;
			frame.rflags.setBit(x86_64_flags::RFLAGS_INTERRUPT_ENABLE);
#endif
			// If our owner wasn't set before, then we'll use from the current thread's owner.
			if(!owner)
				owner = g_currentThread->owner;
			list_rpush(g_currentThread->owner->threads, list_node_new(this));

			list_rpush(g_threads, list_node_new(this));
			list_rpush(priorityList, list_node_new(this));

#if defined(__x86_64__)
			if(owner)
				if (owner->isUserMode)
					tssStackBottom = memory::VirtualAlloc(nullptr, 2, memory::VirtualAllocFlags::WRITE_ENABLED | memory::VirtualAllocFlags::GLOBAL, true);
#else
			if (owner)
				if (owner->isUserMode)
					tssStackBottom = memory::VirtualAlloc(nullptr, 2, memory::VirtualAllocFlags::WRITE_ENABLED | memory::VirtualAllocFlags::GLOBAL);
#endif
			LeaveKernelSection();
			return OBOS_ERROR_NO_ERROR;
		}
	}
}