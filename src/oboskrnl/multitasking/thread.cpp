/*
	thread.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <multitasking/thread.h>
#include <multitasking/multitasking.h>

#include <inline-asm.h>

#include <memory_manager/paging/allocate.h>

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
				return 1;
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

			UINT32_T* stack = (UINT32_T*)memory::VirtualAlloc(nullptr, stackSizePages, memory::VirtualAllocFlags::WRITE_ENABLED | memory::VirtualAllocFlags::GLOBAL);
			if (!stack)
				return 2;
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
			// If our owner wasn't set before, then we'll use from the current thread's owner.
			if(!owner)
				owner = g_currentThread->owner;
			list_rpush(g_currentThread->owner->threads, list_node_new(this));

			utils::setBitInBitfield(frame.eflags, 9);

			list_rpush(g_threads, list_node_new(this));
			list_rpush(priorityList, list_node_new(this));

			if(owner)
				if(owner->isUserMode)
					tssStackBottom = memory::VirtualAlloc(nullptr, 2, memory::VirtualAllocFlags::WRITE_ENABLED | memory::VirtualAllocFlags::GLOBAL);

			LeaveKernelSection();
			return 0;
		}
	}
}