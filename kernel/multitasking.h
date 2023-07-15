/*
	multitasking.h
	
	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_MULTITASKING_H
#define __OBOS_MULTITASKING_H

#include "types.h"

typedef enum __priorityLevel
{
	/// <summary>
	/// Use if a thread is going to be waiting for a long, long, time, or if it isn't doing anything important.
	/// This type of thread gets 1 ms and is at the bottom of the thread queue.
	/// </summary>
	PRIORITY_IDLE = 1,
	/// <summary>
	/// Use if a thread is just running in the background polling data.
	/// This type of thread gets 2 ms and is the third in the thread queue.
	/// </summary>
	PRIORITY_LOW = 2,
	/// <summary>
	/// Use if a thread is actively running, but doesn't require special timing requirements.
	/// This type of thread gets 4 ms and is the second in the thread queue.
	/// </summary>
	PRIORITY_NORMAL = 4,
	/// <summary>
	/// Use if a thread is doing time-critical stuff, where waiting isn't acceptable. This type of thread gets twice as much time than normal priority, so use it carefully.
	/// Great power comes with great responsibilty!
	/// This type of thread gets 8 ms and is the first in the thread queue.
	/// </summary>
	PRIORITY_HIGH = 8
} attribute(packed) PRIORITYLEVEL;
typedef enum __threadStatus
{
	/// <summary>
	/// The thread called ExitThread or TerminateThread was called on this thread's handle.
	/// </summary>
	THREAD_DEAD = 1,
	/// <summary>
	/// The thread is paused.
	/// </summary>
	THREAD_PAUSED = 2,
	/// <summary>
	/// The thread called a blocking function.
	/// </summary>
	THREAD_BLOCKED = 4,
	/// <summary>
	/// The thread called Sleep().
	/// </summary>
	THREAD_SLEEPING = 8,
	/// <summary>
	/// The thread is running.
	/// </summary>
	THREAD_RUNNING = 16
} attribute(packed) THREADSTATUS;

/// <summary>
/// Starts a new thread.
/// </summary>
/// <param name="tid">The thread id. (output)</param>
/// <param name="priority">The priority of the thread.</param>
/// <param name="function">The function to run on startup.</param>
/// <param name="userdata">The data to pass into the function. This is not used while making the thread.</param>
/// <param name="startPaused">Whether to start the thread paused or running.</param>
/// <returns>A handle to the new thread, or OBOS_INVALID_HANDLE_VALUE on failure. If the function fails, call GetLastError for the error code.</returns>
HANDLE MakeThread(DWORD* tid,
				  PRIORITYLEVEL priority,
				  DWORD(*function)(PVOID userdata),
				  PVOID userdata,
				  BOOL startPaused);
/// <summary>
/// Gets the thread id that is held by the thread.
/// </summary>
/// <param name="hThread">The thread.</param>
/// <returns>The thread id, or 0xFFFFFFFF on failure. If the function fails, call GetLastError for the error code.</returns>
DWORD GetHandleTid(HANDLE hThread);
/// <summary>
/// Terminates the thread. Do not use this function unless you know exactly what the thread is doing, or if you know it's doing nothing critical. No resources that this thread has
/// allocated will be freed.
/// </summary>
/// <param name="hThread">The thread to terminate.</param>
/// <param name="exitCode">The exit code.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL TerminateThread(HANDLE hThread, DWORD exitCode);
/// <summary>
/// Gets the exit code of the thread. (The value passed to ExitThread or TerminateThread)
/// </summary>
/// <param name="hThread">The thread to get the exit code of.</param>
/// <param name="pExitCode">The output of the function. If the function fails, this parameter would not be modified.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL GetThreadExitCode(HANDLE hThread, DWORD* pExitCode);
/// <summary>
/// Gets the status of a thread.
/// </summary>
/// <param name="hThread">The thread to get the status.</param>
/// <returns>On failure, 0xFFFFFFFF, otherwise a bitfield with the status.</returns>
UINT32_T GetThreadStatus(HANDLE hThread);
/// <summary>
/// Resumes a thread after it was paused.
/// </summary>
/// <param name="hThread">The thread to resume.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL ResumeThread(HANDLE hThread);
/// <summary>
/// Pauses a thread.
/// </summary>
/// <param name="hThread">The thread to pause.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL PauseThread(HANDLE hThread);
/// <summary>
/// Destroys a thread object. If the thread is still running, it will be terminated.
/// </summary>
/// <param name="hThread">The thread object to destroy.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL DestroyThread(HANDLE hThread);

/// <summary>
/// The next functions operate on the current thread.
/// </summary>

/// <summary>
/// Gets the tid of the current thread.
/// </summary>
/// <returns>The tid of the current thread. This value is useless for the end-user, except for logging</returns>
DWORD GetTid();
/// <summary>
/// Gets the handle of the current thread.
/// </summary>
/// <returns>The handle of the current thread.</returns>
DWORD GetThreadHandle();
/// <summary>
/// Gives up execution time to another thread.
/// </summary>
void YieldExecution();
/// <summary>
/// Waits for the specified amount of milliseconds.
/// </summary>
/// <param name="milliseconds">The amount of milliseconds to sleep for.</param>
void Sleep(DWORD milliseconds);
/// <summary>
/// Exits the current thread.<para/>
/// Does not release any resources allocated by the thread.
/// </summary>
/// <param name="exitCode">The exit code.</param>
void ExitThread(DWORD exitCode);
/// <summary>
/// Waits for the threads to exit passed to the function. This a blocking function.
/// </summary>
/// <param name="nThreads">The amount of threads passed in the array.</param>
/// <param name="handles">The threads.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL WaitForThreads(SIZE_T nThreads, HANDLE* handles);
/// <summary>
/// Waits for the threads to exit passed to the function. This a blocking function.
/// </summary>
/// <param name="nThreads">The amount of threads passed as variadic arguments.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL WaitForThreadsVariadic(SIZE_T nThreads, ...);

/// <summary>
/// For internal use for the kernel. Cannot be accessed outside the kernel (I hope, or bad stuff can happen).
/// </summary>
/// <param name="hThread"></param>
DWORD ksetblockstate(HANDLE hThread);
/// <summary>
/// For internal use for the kernel. Cannot be accessed outside the kernel (I hope, or bad stuff can happen).
/// </summary>
/// <param name="hThread"></param>
DWORD kclearblockstate(HANDLE hThread);
/// <summary>
/// For internal use for the kernel. Cannot be accessed outside the kernel (I hope, or bad stuff can happen).
/// </summary>
/// <param name="hThread"></param>
DWORD ksetblockcallback(HANDLE hThread, BOOL(*callback)(HANDLE hThread, PVOID userData), PVOID userData);

#endif