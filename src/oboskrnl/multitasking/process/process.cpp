/*
	oboskrnl/process/process.h

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <multitasking/thread.h>
#include <multitasking/scheduler.h>
#include <multitasking/cpu_local.h>

#include <multitasking/process/process.h>
#include <multitasking/process/arch.h>

namespace obos
{
	namespace process
	{
		uint32_t g_nextPID = 1;
		Process::ProcessList g_processes;
		Process* CreateProcess(bool isUsermode)
		{
			Process* ret = new Process{};

			setupContextInfo(&ret->context);
			ret->pid = g_nextPID++;
			ret->isUsermode = isUsermode;
			ret->parent = (process::Process*)((thread::cpu_local*)thread::getCurrentCpuLocalPtr())->currentThread->owner;
			if (g_processes.tail)
			{
				g_processes.tail->next = ret;
				ret->prev = g_processes.tail;
			}
			if (!g_processes.head)
				g_processes.head = ret;
			g_processes.tail = ret;
			g_processes.size++;
			
			if (ret->parent->children.tail)
			{
				ret->parent->children.tail->next_child = ret;
				ret->prev_child = ret->parent->children.tail;
			}
			if (!ret->parent->children.head)
				ret->parent->children.head = ret;
			ret->parent->children.tail = ret;
			ret->parent->children.size++;
			
			return ret;
		}
	}
}