/*
	oboskrnl/arch/x86_64/syscall/vfs/dir.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/x86_64/syscall/handle.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t DirectorySyscallHandler(uint64_t syscall, void* args);

		/// <summary>
		/// Syscall number: 61<para></para>
		/// Makes a directory iterator.
		/// </summary>
		/// <returns>The handle, or USER_HANDLE_MAX on failure.</returns>
		user_handle SyscallMakeDirectoryIterator();
		
		/// <summary>
		/// Syscall number: 66<para></para>
		/// Opens a directory iterator at 'path'.
		/// </summary>
		/// <param name="hnd">The handle to open.</param>
		/// <param name="path">The path of the directory to iterate over.</param>
		/// <returns>Whether the handle was successfully opened (true) or not (false).</returns>
		bool SyscallDirectoryIteratorOpen(user_handle hnd, const char* path);

		/// <summary>
		/// Syscall number: 62<para></para>
		/// Makes the directory iterator's position increase by one.
		/// </summary>
		/// <param name="hnd">The iterator to modify.</param>
		/// <returns>Whether the function succeeded (true) or not (false).</returns>
		bool SyscallDirectoryIteratorNext(user_handle hnd);
		/// <summary>
		/// Syscall number: 63<para></para>
		/// Makes the directory iterator's position decrease by one.
		/// </summary>
		/// <param name="hnd">The iterator to modify.</param>
		/// <returns>Whether the function succeeded (true) or not (false).</returns>
		bool SyscallDirectoryIteratorPrevious(user_handle hnd);
		/// <summary>
		/// Syscall number: 64<para></para>
		/// Gets the current file path of the directory iterator.
		/// </summary>
		/// <param name="hnd">The handle to query.</param>
		/// <param name="path">[out] The buffer to store the path in. This can be nullptr.</param>
		/// <param name="size">[in,out] A pointer to the size of the path, and a pointer to the actual size of the path. This must not be nullptr if path is not nullptr.</param>
		/// <returns>Whether the function succeeded (true) or not (false).</returns>
		bool SyscallDirectoryIteratorGet(user_handle hnd, char* path, size_t* size);
		/// <summary>
		/// Syscall number: 65<para></para>
		/// Queries whether the directory iterator has any more entries left to iterator, as if you queried EOF on it.
		/// </summary>
		/// <param name="hnd">The handle to query.</param>
		/// <param name="status">[out,opt] Whether the function succeeded (true) or not (false).</param>
		/// <returns>Whether the directory iterator reached past the last entry (true) or not (false). This returns false if the function fails, always check *status.</returns>
		bool SyscallDirectoryIteratorEnd(user_handle hnd, bool* status);

		/// <summary>
		/// Syscall number: 67<para></para>
		/// Closes a directory handle.
		/// </summary>
		/// <param name="hnd">The handle to close.</param>
		/// <returns>Whether the handle was successfully closed (true) or not (false).</returns>
		bool SyscallDirectoryIteratorClose(user_handle hnd);

		/// <summary>
		/// Syscall number: 68<para></para>
		/// Gets the path of the parent directory of a directory iterator.<para></para>
		/// In case anyone is wondering why I (Omar Berrow) decided to not add a direct syscall to get the parent of a path, it was because I was too lazy to do that.
		/// </summary>
		/// <param name="hnd">The handle to query.</param>
		/// <param name="path">[out] The buffer to store the path in. This can be nullptr.</param>
		/// <param name="size">[in,out] A pointer to the size of the path, and a pointer to the actual size of the path. This must not be nullptr if path is not nullptr.</param>
		/// <returns>Whether the function succeeded (true) or not (false).</returns>
		bool SyscallDirectoryIteratorGetParent(user_handle hnd, char* path, size_t* size);
	}
}