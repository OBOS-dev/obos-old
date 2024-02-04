/*
	arch/x86_64/syscall/handle.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <utils/pair.h>

namespace obos
{
	namespace syscalls
	{
		enum class ProcessHandleType
		{
			INVALID = (-1),
			FILE_HANDLE,
			DRIVE_HANDLE,
			THREAD_HANDLE,
			VALLOCATOR_HANDLE,
			DIRECTORY_ITERATOR_HANDLE,
		};
		using user_handle = uint64_t;
		using handle = utils::pair<void*, ProcessHandleType>;
		constexpr user_handle USER_HANDLE_MAX = UINT64_MAX;
		/// <summary>
		/// Registers a handle for a process at a certain address with a certain type.
		/// </summary>
		/// <param name="proc">The owner of the handle. This should be a process::Process*</param>
		/// <param name="objectAddress">The address of the object.</param>
		/// <param name="type">The type of the object.</param>
		/// <returns>The newly made handle, or UINT64_MAX on failure.</returns>
		user_handle ProcessRegisterHandle(void* proc, void* objectAddress, ProcessHandleType type);
		/// <summary>
		/// Frees a handle for a process.
		/// </summary>
		/// <param name="proc">The owner of the handle. This should be a process::Process*</param>
		/// <param name="handle">The handle to release.</param>
		/// <returns>The object the handle represented, or nullptr on failure.</returns>
		void* ProcessReleaseHandle(void* proc, user_handle handle);
		/// <summary>
		/// Checks if a handle exists or not.
		/// </summary>
		/// <param name="proc">The owner of the handle. This should be a process::Process*</param>
		/// <param name="handle">The handle value to check.</param>
		/// <param name="type">The type of the handle. This can be ProcessHandleType::INVALID to specify any handle type.</param>
		/// <returns>true if the handle exists, and the type matches, otherwise false.</returns>
		bool ProcessVerifyHandle(void* proc, user_handle handle, ProcessHandleType type = ProcessHandleType::INVALID);
		/// <summary>
		/// Gets the object represented by a handle.
		/// </summary>
		/// <param name="proc">The owner of the handle. This should be a process::Process*</param>
		/// <param name="handle">The handle value to retrieve.</param>
		/// <returns>The object represented by the handle on success, or nullptr on failure.</returns>
		void* ProcessGetHandleObject(void* proc, user_handle handle);
		/// <summary>
		/// Gets the type of a handle.
		/// </summary>
		/// <param name="proc">The owner of the handle. This should be a process::Process*</param>
		/// <param name="handle">The handle value to retrieve.</param>
		/// <returns>The type of thehandle on success, or ProcessHandleType::INVALID on failure.</returns>
		ProcessHandleType ProcessGetHandleType(void* proc, user_handle handle);

		/// <summary>
		/// Syscall Number: 24<para></para>
		/// Frees a handle. After this function is called on a handle, the handle can no longer be used for any purpose.
		/// You should call this function after calling Syscall*CloseHandle.
		/// </summary>
		/// <code>uint64_t thrHandle = SyscallMakeThreadHandle();</code>
		/// <code>SyscallCreateThread(thrHandle, 0,0,ThreadEntry,0, nullptr, false);</code>
		/// <code>// Do stuff with the handle.</code>
		/// <code>SyscallThreadCloseHandle(thrHandle);</code>
		/// <code>SyscallInvalidateHandle(thrHandle);</code>
		/// 
		/// <param name="syscall">Only here because of the syscall abi on OBOS x86-64</param>
		/// <param name="handle">The handle to free. This is a pointer because of the syscall abi on OBOS x86-64, when 'syscall'ing, you just pass a user_handle.</param>
		/// <returns>Whether the handle was freed properly (true) or false on failure.</returns>
		bool SyscallInvalidateHandle(uint64_t syscall, user_handle* handle);
	}
}