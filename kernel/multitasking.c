/*
	multitasking.c

	Copyright (c) 2023 Omar Berrow
*/

#include "multitasking.h"
#include "interrupts.h"
#include "error.h"
#include "inline-asm.h"
#include "bitfields.h"
#include "klog.h"
#include "paging.h"

#include "liballoc/liballoc_1_1.h"

#include "list/list.h"

// Warning: this function does not return.

struct thr_registers
{
	// +0
	UINT32_T ebp;
	// +4
	UINT32_T ds;
	// +8,+12,+16,+20
	UINT32_T eip, cs, eflags, ss;
	// +24,+28,+32,+36,+40,+44,+48
	UINT32_T edi, esi, esp, ebx, edx, ecx, eax;
};
struct kThreadStructure
{
	/// <summary>
	/// The thread id of the thread.
	/// </summary>
	DWORD tid;
	/// <summary>
	/// The process that owns the thread. If it the kernel this will be zero.
	/// </summary>
	DWORD owner;
	/// <summary>
	/// The exit code of the thread. If the thread didn't exit, this value is undefined.
	/// </summary>
	DWORD exitCode;
	/// <summary>
	/// The status of the thread. This is a bitfield.
	/// </summary>
	THREADSTATUS status;
	/// <summary>
	/// The priority of the thread.
	/// </summary>
	PRIORITYLEVEL priority;
	/// <summary>
	/// How much times the thread has been run. This gets reset once all the threads have been run.
	/// If it is equal to or greater than the priority, the thread will no longer get run.
	/// If there is only one thread in the entire system (only at early boot time), this will be ignored.
	/// </summary>
	BYTE iterations;
	/// <summary>
	/// The saved registers.
	/// </summary>
	struct thr_registers registers;
	/// <summary>
	/// The top of the thread's stack. This is used in DestroyThread so the thread's stack can be destroyed.
	/// </summary>
	UINTPTR_T stackStart;
	/// <summary>
	/// When to wake up the thread. This used for the Sleep function.
	/// </summary>
	QWORD timerEnd;
	/// <summary>
	/// The function to check whether the thread should continue running.
	/// </summary>
	BOOL(*checkIfBlocked)(HANDLE hThread, PVOID userData);
	/// <summary>
	/// The user data for the callback.
	/// </summary>
	PVOID checkIfBlockedUserData;
	/// <summary>
	/// Whether the thread was made in user mode.
	/// </summary>
	BOOL isUserMode;
	/// <summary>
	/// The kernel stack.
	/// </summary>
	UINTPTR_T kernelStack;
};
static list_t* s_kThreadHandles;
/// <summary>
/// s_kThreadPriorities[0] are PRIORITY_IDLE threads.
/// s_kThreadPriorities[1] are PRIORITY_LOW threads.
/// s_kThreadPriorities[2] are PRIORITY_NORMAL threads.
/// s_kThreadPriorities[3] are PRIORITY_HIGH threads.
/// </summary>
static list_t* s_kThreadPriorities[4];
static HANDLE s_nextThreadTidValue = 0;
static struct kThreadStructure* s_currentThread;
static BOOL skipThreadSet = FALSE;
static HANDLE skipThreadHandle = OBOS_INVALID_HANDLE_VALUE;

extern void changeKernelStack(UINTPTR_T stack);

static PVOID memset(PVOID ptr, CHAR ch, SIZE_T size)
{
	PCHAR tmp = (PCHAR)ptr;
	for (int i = 0; i < size; tmp[i++] = ch);
	return ptr;
}

static struct thr_registers isr_registers_To_thr_registers(isr_registers registers)
{
	struct thr_registers ret;
	ret.cs = registers.cs;
	ret.ds = registers.ds;
	ret.ss = registers.ss;
	ret.eax = registers.eax;
	ret.edx = registers.edx;
	ret.esi = registers.esi;
	ret.esp = registers.esp;
	ret.edi = registers.edi;
	ret.ebx = registers.ebx;
	ret.ecx = registers.ecx;
	ret.eip = registers.eip;
	ret.ebp = registers.ebp;
	ret.eflags = registers.eflags;
	return ret;
}

// Warning: Function doesn't return.
void attribute(noreturn) switchTaskToAsm();
// Warning: Function doesn't return.
void attribute(noreturn) switchTaskToAsmUserMode();

static QWORD s_millisecondsSinceInitialization = 0;

static BOOL canRunThread(struct kThreadStructure* thr)
{
	return !getBitFromBitfieldBitmask(thr->status, THREAD_BLOCKED) &&
		   !getBitFromBitfieldBitmask(thr->status, THREAD_PAUSED) &&
		   !getBitFromBitfieldBitmask(thr->status, THREAD_DEAD) && 
		   !getBitFromBitfieldBitmask(thr->status, THREAD_SLEEPING) &&
			getBitFromBitfieldBitmask(thr->status, THREAD_RUNNING) &&
			thr->iterations < thr->priority;
}

static void attribute(noreturn) switchTaskTo(int newTask)
{
	kassert(((struct kThreadStructure*)newTask)->tid != 1, KSTR_LITERAL("Attempt to switch to the idle task. See multitasking.c:kinitmultitasking()\r\n"));
	s_currentThread = (PVOID)newTask;
	changeKernelStack(s_currentThread->kernelStack);
	struct thr_registers* tmp = &s_currentThread->registers;
	if (s_currentThread->isUserMode)
	{
		asm volatile ("mov %%eax, %0" :
									  : "r"(tmp));
		switchTaskToAsmUserMode();
	}
	asm volatile ("mov %%eax, %0" :
								  : "r"(tmp));
	switchTaskToAsm();
}
static void findNewTask(isr_registers registers)
{
	// If only the kernel main thread exists, continue execution.
	if (s_nextThreadTidValue == 1)
		return;
	if(s_currentThread)
		s_currentThread->registers = isr_registers_To_thr_registers(registers);
label1:
	list_node_t* newTask = NULLPTR;
	// Check if all the tasks have been run.
	BOOL ranAllTasks = FALSE;
	{
		INT realIndex = 0;
		for (int i = 0; i < 4; i++)
		{
			list_iterator_t* iter = list_iterator_new(s_kThreadPriorities[i], LIST_HEAD);
			BOOL shouldBreak = FALSE;
			for (list_node_t* node = list_iterator_next(iter); node; node = list_iterator_next(iter), realIndex++)
			{
				if (realIndex != 1)
					continue;
				struct kThreadStructure* currentHandle = (struct kThreadStructure*)node->val;
				if (!(!getBitFromBitfieldBitmask(currentHandle->status, THREAD_BLOCKED) &&
					  !getBitFromBitfieldBitmask(currentHandle->status, THREAD_PAUSED) &&
					  !getBitFromBitfieldBitmask(currentHandle->status, THREAD_DEAD) &&
					  !getBitFromBitfieldBitmask(currentHandle->status, THREAD_SLEEPING) &&
					   getBitFromBitfieldBitmask(currentHandle->status, THREAD_RUNNING)))
				{
					realIndex--;
					continue;
				}
				ranAllTasks = currentHandle->iterations >= currentHandle->priority;
				shouldBreak = TRUE;
				break;
			}
			list_iterator_destroy(iter);
			if (shouldBreak)
				break;
		}
	}
	// If all tasks have been run, clear the amount of iterations for all tasks.
	if (ranAllTasks)
	{
		list_iterator_t* iter = list_iterator_new(s_kThreadHandles, LIST_HEAD);
		for (list_node_t* currentNode = list_iterator_next(iter); currentNode; currentNode = list_iterator_next(iter))
			((struct kThreadStructure*)currentNode->val)->iterations = 0;
		list_iterator_destroy(iter);
	}
	// Find a new task to run.
	for (int i = 3; i >= 0 && !newTask; i--)
	{
		list_iterator_t* iter = list_iterator_new(s_kThreadPriorities[i], LIST_TAIL);
		for (list_node_t* currentNode = list_iterator_next(iter); currentNode; currentNode = list_iterator_next(iter))
		{
			struct kThreadStructure* currentHandle = (struct kThreadStructure*)currentNode->val;
			if (getBitFromBitfieldBitmask(currentHandle->status, THREAD_SLEEPING) && currentHandle->timerEnd >= s_millisecondsSinceInitialization)
				currentHandle->status = THREAD_RUNNING;
			if (getBitFromBitfieldBitmask(currentHandle->status, THREAD_BLOCKED))
				if (currentHandle->checkIfBlocked)
					if (currentHandle->checkIfBlocked((HANDLE)currentNode->val, currentHandle->checkIfBlockedUserData))
						currentHandle->status = THREAD_RUNNING;
					else {}
				else
					currentHandle->status = THREAD_DEAD; // Kill the thread.
			else {}
			if (canRunThread(currentHandle))
			{
				if ((UINTPTR_T)currentNode->val == skipThreadHandle && skipThreadSet)
				{
					skipThreadSet = FALSE;
					continue;
				}
				newTask = currentNode;
				currentHandle->iterations++;
				break;
			}
		}
		list_iterator_destroy(iter);
	}
	kassert(newTask != NULLPTR, KSTR_LITERAL("The new task is nullptr."));
	if (((struct kThreadStructure*)newTask->val)->tid == 1)
	{
		// Note: Do not use the sti() function, as if the thread was interrupted in a kernel section (see kernel/inline-asm.c), sti() will do nothing.
		asm volatile("sti");
		goto label1;
	}
	if (newTask->val == s_currentThread)
		return;
	switchTaskTo((UINTPTR_T)newTask->val);
}

static void scheduler(int interrupt, isr_registers registers)
{
	if(interrupt != 48)
		s_millisecondsSinceInitialization++;
	findNewTask(registers);
}

void kinitmultitasking()
{
	EnterKernelSection();
	s_kThreadPriorities[0] = list_new();
	s_kThreadPriorities[1] = list_new();
	s_kThreadPriorities[2] = list_new();
	s_kThreadPriorities[3] = list_new();

	s_kThreadHandles = list_new();

	extern void kmain_thread();
	extern char stack_top_thread;
	struct kThreadStructure* current = list_rpush(s_kThreadHandles, list_node_new(kcalloc(1, sizeof(struct kThreadStructure))))->val;
	list_rpush(s_kThreadPriorities[2], list_node_new(current));
	current->tid = s_nextThreadTidValue++;
	current->priority = PRIORITY_NORMAL;
	current->status = THREAD_RUNNING;
	current->registers.eip = (UINTPTR_T)kmain_thread;
	current->registers.esp = (UINTPTR_T)&stack_top_thread;
	current->registers.ebp = 0;
	current->stackStart	   = (UINTPTR_T)&stack_top_thread;
	current->kernelStack   = (UINTPTR_T)kalloc_pages(2, FALSE, TRUE) + 8192;
	current->isUserMode = FALSE;
	//s_currentThread = current;
	extern void idleTask();
	// The idle task doesn't actually exist, the scheduler uses it for reasons. Do not remove it.
	current = list_rpush(s_kThreadHandles, list_node_new(kcalloc(1, sizeof(struct kThreadStructure))))->val;
	list_rpush(s_kThreadPriorities[0], list_node_new(current));
	current->tid = s_nextThreadTidValue++;
	current->priority = PRIORITY_IDLE;
	current->status = THREAD_RUNNING;
	current->registers.eip = (UINTPTR_T)0;
	current->registers.esp = (UINTPTR_T)0;
	current->registers.ebp = (UINTPTR_T)0;
	current->stackStart    = (UINTPTR_T)0;
	current->isUserMode = FALSE;
	LeaveKernelSection();
	{
		int _irq0 = 0;
		setPICInterruptHandlers(&_irq0, 1, scheduler);

		// 1000 hz, 1 ms. (1/1000 * 1000)
		UINT32_T divisor = 1193180 / 1000;

		outb(0x43, 0x36);

		UINT8_T l = (UINT8_T)(divisor & 0xFF);
		UINT8_T h = (UINT8_T)((divisor >> 8) & 0xFF);

		// Send the frequency divisor.
		outb(0x40, l);
		outb(0x40, h);

		enablePICInterrupt(0);
	}
	switchTaskTo((int)list_at(s_kThreadHandles, 0)->val);
}

HANDLE MakeThread(DWORD* tid,
	PRIORITYLEVEL priority,
	DWORD(*function)(PVOID userdata),
	PVOID userdata,
	BOOL startPaused)
{
	switch (priority)
	{
	case PRIORITY_IDLE:
	case PRIORITY_LOW:
	case PRIORITY_NORMAL:
	case PRIORITY_HIGH:
		break;
	default:
		SetLastError(OBOS_ERROR_INVALID_PARAMETER);
		return OBOS_INVALID_HANDLE_VALUE;
		break;
	}
	EnterKernelSection();
	struct kThreadStructure* ret = kcalloc(1, sizeof(struct kThreadStructure));
	if (!ret)
	{
		SetLastError(OBOS_ERROR_NO_MEMORY);
		return OBOS_INVALID_HANDLE_VALUE;
	}
	ret->priority = priority;
	ret->status = THREAD_RUNNING | ((startPaused) ? THREAD_PAUSED : THREAD_RUNNING);
	ret->tid = s_nextThreadTidValue++;
	if (tid) *tid = ret->tid;
	ret->owner = /*GetPid()*/0;
	// See paging.c:kInitializePaging.
	ret->registers.eip = 0x800000;
	ret->stackStart = ret->registers.esp = (UINTPTR_T)kalloc_pages(2, FALSE, TRUE) + 2 * 4096;
	ret->kernelStack = (UINTPTR_T)kalloc_pages(2, FALSE, TRUE) + 8192;
	ret->registers.ebp = 0;
	ret->registers.eax = (UINTPTR_T)function;
	ret->registers.edi = (UINTPTR_T)userdata;
	ret->isUserMode = TRUE;
	list_t* list = NULLPTR;
	switch (priority)
	{
	case PRIORITY_IDLE:
		list = s_kThreadPriorities[0];
		break;
	case PRIORITY_LOW:
		list = s_kThreadPriorities[1];
		break;
	case PRIORITY_NORMAL:
		list = s_kThreadPriorities[2];
		break;
	case PRIORITY_HIGH:
		list = s_kThreadPriorities[3];
		break;
	default:
		break;
	}
	list_rpush(list			   , list_node_new(ret));
	list_rpush(s_kThreadHandles, list_node_new(ret));
	disablePICInterrupt(0);
	LeaveKernelSection();
	enablePICInterrupt(0);
	return (HANDLE)ret;
}

DWORD GetHandleTid(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return 0xFFFFFFFF;
	}
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
	{
		SetLastError(OBOS_ERROR_ACCESS_DENIED);
		return 0xFFFFFFFF;
	}
	return thread->tid;
}

BOOL TerminateThread(HANDLE hThread, DWORD exitCode)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
	{
		SetLastError(OBOS_ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (getBitFromBitfieldBitmask(thread->status, THREAD_DEAD))
	{
		SetLastError(OBOS_ERROR_THREAD_DEAD);
		return FALSE;
	}
	EnterKernelSection();
	thread->status = generateBitfieldBitmask(1, THREAD_DEAD);
	thread->exitCode = exitCode;
	thread->owner = 0;
	LeaveKernelSection();
	if (hThread == (UINTPTR_T)s_currentThread)
		YieldExecution(); // Might not get hit.
	return TRUE;
}

BOOL GetThreadExitCode(HANDLE hThread, DWORD* pExitCode)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid*/0)
	{
		SetLastError(OBOS_ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (!getBitFromBitfieldBitmask(thread->status, THREAD_DEAD))
	{
		SetLastError(OBOS_ERROR_THREAD_NOT_DEAD);
		return FALSE;
	}
	*pExitCode = thread->exitCode;
	return TRUE;
}

UINT32_T GetThreadStatus(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
	{
		SetLastError(OBOS_ERROR_ACCESS_DENIED);
		return FALSE;
	}
	return thread->status;
}

BOOL ResumeThread(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
	{
		SetLastError(OBOS_ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (getBitFromBitfieldBitmask(thread->status, THREAD_DEAD))
	{
		SetLastError(OBOS_ERROR_THREAD_DEAD);
		return FALSE;
	}
	EnterKernelSection();
	thread->status = generateBitfieldBitmask(1, THREAD_RUNNING);
	LeaveKernelSection();
	return TRUE;
}

BOOL PauseThread(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
	{
		SetLastError(OBOS_ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if (getBitFromBitfieldBitmask(thread->status, THREAD_DEAD))
	{
		SetLastError(OBOS_ERROR_THREAD_DEAD);
		return FALSE;
	}
	EnterKernelSection();
	thread->status = generateBitfieldBitmask(2, THREAD_RUNNING, THREAD_PAUSED);
	LeaveKernelSection();
	return TRUE;
}

BOOL DestroyThread(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
	{
		SetLastError(OBOS_ERROR_ACCESS_DENIED);
		return FALSE;
	}
	if ((UINTPTR_T)s_currentThread == hThread)
	{
		SetLastError(OBOS_ERROR_CANNOT_DESTROY_SELF);
		return FALSE;
	}
	EnterKernelSection();
	if (!getBitFromBitfieldBitmask(thread->status, THREAD_DEAD))
		TerminateThread(hThread, -1);
	list_t* list = NULLPTR;
	switch (thread->priority)
	{
	case PRIORITY_IDLE:
		list = s_kThreadPriorities[0];
		break;
	case PRIORITY_LOW:
		list = s_kThreadPriorities[1];
		break;
	case PRIORITY_NORMAL:
		list = s_kThreadPriorities[2];
		break;
	case PRIORITY_HIGH:
		list = s_kThreadPriorities[3];
		break;
	default:
		break;
	}
	list_remove(s_kThreadHandles, list_find(s_kThreadHandles, thread));
	list_remove(list, list_find(list, thread));
	memset(thread, 0, sizeof(struct kThreadStructure));
	kfree_pages((PVOID)(thread->stackStart - 2 * 4096), 2);
	kfree_pages((PVOID)(thread->kernelStack - 2 * 4096), 2);
	kfree(thread);
	LeaveKernelSection();
	return TRUE;
}

DWORD GetTid()
{
	if (s_currentThread) 
		return s_currentThread->tid; 
	return (DWORD)-1;
}
DWORD GetThreadHandle()
{ return (UINTPTR_T)s_currentThread; }
void YieldExecution()
{
	skipThreadHandle = (UINTPTR_T)s_currentThread;
	skipThreadSet = TRUE;
	// Call the scheduler.
	_int(48);
	/*isr_registers reg;
	reg.eax = 0;
	{ PCHAR e = (PCHAR)&reg; for (int i = 0; i < sizeof(reg); e[i++] = 0); }
	scheduler(0, reg);*/
}
void Sleep(DWORD milliseconds)
{
	EnterKernelSection();
	s_currentThread->status = generateBitfieldBitmask(2, THREAD_RUNNING, THREAD_SLEEPING);
	s_currentThread->timerEnd = s_millisecondsSinceInitialization + milliseconds;
	LeaveKernelSection();
	YieldExecution();
}
void attribute(noreturn) ExitThread(DWORD exitCode)
{
	TerminateThread((UINTPTR_T)s_currentThread, exitCode);
	while (1)
		__builtin_ia32_pause();
}
static BOOL WaitForThreadsCallback(HANDLE hThread, PVOID userData)
{
	HANDLE* handles = *(HANDLE**)userData;
	if (!handles)
		return FALSE; // We were lied to...
	SIZE_T blockSize = *(SIZE_T*)(((UINTPTR_T)userData) + 4);
	BOOL ret = TRUE;
	for (int i = 0; i < blockSize / sizeof(HANDLE); i++)
	{
		HANDLE hThread = handles[i];
		if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
		{
			ret = FALSE;
			break;
		}
		struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
		if (thread->owner != /*GetPid()*/0)
		{
			ret = FALSE;
			break;
		}
		if (!getBitFromBitfieldBitmask(GetThreadStatus(hThread), THREAD_DEAD))
		{
			ret = FALSE;
			break;
		}
	}
	if (ret)
	{
		kfree(handles);
		kfree(userData);
	}
	return ret;
}
BOOL WaitForThreads(SIZE_T nThreads, HANDLE* handles)
{
	// Check if the handles are valid.
	{
		for (SIZE_T i = 0; i < nThreads; i++)
		{
			HANDLE hThread = handles[i];
			if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return FALSE;
			}
			struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
			if (thread->owner != /*GetPid()*/0)
			{
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return FALSE;
			}
		}
	}
	EnterKernelSection();
	HANDLE* _handles = kcalloc(nThreads, sizeof(HANDLE));
	if (!handles)
	{
		SetLastError(OBOS_ERROR_NO_MEMORY);
		return FALSE;
	}
	for (int i = 0; i < nThreads; i++)
		_handles[i] = handles[i];
	UINTPTR_T* userData = kcalloc(2, sizeof(UINTPTR_T));
	userData[0] = (UINTPTR_T)_handles;
	userData[1] = nThreads * sizeof(HANDLE);
	ksetblockcallback((HANDLE)s_currentThread, WaitForThreadsCallback, (PVOID)userData);
	ksetblockstate((HANDLE)s_currentThread);
	_int(48);
	LeaveKernelSection();
	return TRUE;
}
BOOL WaitForThreadsVariadic(SIZE_T nThreads, ...)
{
	HANDLE* array = kcalloc(nThreads, sizeof(HANDLE));
	va_list list; va_start(list, nThreads);
	for (int i = 0; i < nThreads; i++)
		array[i] = va_arg(list, HANDLE);
	va_end(list);
	BOOL ret = WaitForThreads(nThreads, array);
	kfree(array);
	return ret;
}

DWORD ksetblockstate(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
		return OBOS_ERROR_INVALID_HANDLE;
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
		return OBOS_ERROR_ACCESS_DENIED;
	EnterKernelSection();
	thread->status = generateBitfieldBitmask(2, THREAD_RUNNING, THREAD_BLOCKED);
	LeaveKernelSection();
	return 0;
}

DWORD kclearblockstate(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
		return OBOS_ERROR_INVALID_HANDLE;
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
		return OBOS_ERROR_ACCESS_DENIED;
	EnterKernelSection();
	thread->status = generateBitfieldBitmask(1, THREAD_RUNNING);
	LeaveKernelSection();
	return 0;
}

DWORD ksetblockcallback(HANDLE hThread, BOOL(*callback)(HANDLE hThread, PVOID userData), PVOID userData)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
		return OBOS_ERROR_INVALID_HANDLE;
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
		return OBOS_ERROR_ACCESS_DENIED;
	EnterKernelSection();
	thread->checkIfBlocked = callback;
	thread->checkIfBlockedUserData = userData;
	LeaveKernelSection();
	return 0;
}

DWORD kMakeThreadKernelMode(HANDLE hThread)
{
	if (!list_find(s_kThreadHandles, (struct kThreadStructure*)hThread))
		return OBOS_ERROR_INVALID_HANDLE;
	struct kThreadStructure* thread = (struct kThreadStructure*)hThread;
	if (thread->owner != /*GetPid()*/0)
		return OBOS_ERROR_ACCESS_DENIED;
	EnterKernelSection();
	thread->isUserMode = FALSE;
	thread->registers.ds = 0x10;
	LeaveKernelSection();
	return 0;
}
