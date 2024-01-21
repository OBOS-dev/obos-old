/*
	arch/x86_64/syscall/file.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/x86_64/syscall/handle.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t FileHandleSyscallHandler(uint64_t syscall, void* pars);

		/// <summary>
		/// Syscall Number: 13<para></para>
		/// Makes a raw file handle.
		/// </summary>
		/// <returns>The handle.</returns>
		user_handle SyscallMakeFileHandle();

		/// <summary>
		/// Syscall Number: 14<para></para>
		/// Opens a file on hnd.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <param name="path">The file path.</param>
		/// <param name="options">The open options. See vfs/fileManip/fileHandle.h for valid values for this parameter.</param>
		/// <returns>Whether the file could be opened (true) or not (false). If it fails, use GetLastError for an error code.</returns>
		bool SyscallFileOpen(user_handle hnd, const char* path, uint32_t options);

		/// <summary>
		/// Syscall Number: 15<para></para>
		/// Reads "nToRead" from the file handle into "data".
		/// If EOF is reached before all the data could be read, the function fails with OBOS_ERROR_VFS_READ_ABORTED.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <param name="data">The buffer to read into.</param>
		/// <param name="nToRead">The count of bytes to read.</param>
		/// <param name="peek">Whether to increment the stream position.</param>
		/// <returns>Whether the file could be read (true) or not (false). If it fails, use GetLastError for an error code.</returns>
		bool SyscallFileRead(user_handle hnd, char* data, size_t nToRead, bool peek);

		/// <summary>
		/// Syscall Number: 16<para></para>
		/// Checks if the file handle has reached the end of the file.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <returns>Whether EOF has been reached (true) or not (false). If this function fails, it returns true.</returns>
		bool SyscallFileEof(user_handle hnd);
		/// <summary>
		/// Syscall Number: 17<para></para>
		/// Gets the current file offset.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <returns>The current file offset, or UINTPTR_MAX on failure.</returns>
		uintptr_t SyscallFileGetPos(user_handle hnd);
		/// <summary>
		/// Syscall Number: 18<para></para>
		/// Gets the file handle's status/flags.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <returns>The handle's status/flags, or 0xffffffff on failure.</returns>
		uint32_t SyscallFileGetFlags(user_handle hnd);
		/// <summary>
		/// Syscall Number: 19<para></para>
		/// Gets the file's size.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <returns>The file's size, or -1 on failure.</returns>
		size_t SyscallFileGetFileSize(user_handle hnd);
		/// <summary>
		/// Syscall Number: 20<para></para>
		/// Gets the path of the parent directory of the file.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <param name="path">The buffer to put the path in. This can be nullptr.</param>
		/// <param name="sizePath">(output) The size of the path.</param>
		void SyscallFileGetParent(user_handle hnd, char* path, size_t* sizePath);

		/// <summary>
		/// Syscall Number: 21<para></para>
		/// Seeks to count based on the parameters.
		/// </summary>
		/// <param name="hnd">The file handle.</param>
		/// <param name="count">How much to adjust the position by</param>
		/// <param name="from">Where to add "count". See vfs/fileManip/fileHandle.h for valid values for this parameter.</param>
		/// <returns>The old file position.</returns>
		uintptr_t SyscallFileSeekTo(user_handle hnd, uintptr_t count, uint32_t from);

		/// <summary>
		/// Syscall Number: 22<para></para>
		/// Closes a file handle, allowing it to be used for another file.
		/// </summary>
		/// <returns>Whether the file could be closed (true) or not (false). If it fails, use GetLastError for an error code.</returns>
		bool SyscallFileClose(user_handle hnd);
	}
}