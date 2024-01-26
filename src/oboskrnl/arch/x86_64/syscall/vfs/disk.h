/*
	oboskrnl/arch/x86_64/syscall/vfs/disk.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/x86_64/syscall/handle.h>

namespace obos
{
	namespace syscalls
	{
        uintptr_t DriveSyscallHandler(uint64_t syscall, void* args);

        /// <summary>
        /// Syscall number: 54<para></para>
        /// Makes a drive handle.
        /// </summary>
        /// <returns>The new handle.</returns>
        user_handle SyscallMakeDriveHandle();

        /// <summary>
        /// Syscall number: 45<para></para>
        /// Opens a drive.
        /// <para></para>Path format: Dd[Pp]:/
        /// <para></para>With 'd' being the drive id, and 'p' being the partition id (optional).
        /// <para></para>Everything after the initial path is ignored.
        /// <para></para>Examples:
        /// <para></para>To open drive 0, raw: D0:/
        /// <para></para>To open drive 0, partition 1, D0P1:/
        /// </summary>
        /// <param name="path">The handle to open the drive on.</param>
        /// <param name="path">The path to open. See above for how to format it.</param>
        /// <param name="options">The options. Look in vfs/devManip/driveHandle.h for valid values for this parameter.</param>
        /// <returns>If the function succeeded, true, otherwise false.</returns>
        bool SyscallOpenDrive(user_handle hnd, const char* path, uint32_t options);

        /// <summary>
        /// Syscall number: 46<para></para>
        /// Reads nSectors at lbaOffset.
        /// </summary>
        /// <param name="hnd">The drive handle to read from.</param>
        /// <param name="buff">The buffer to read into.</param>
        /// <param name="nSectorsRead">The amount of sectors read.</param>
        /// <param name="lbaOffset">The lba offset of the first sector to read.</param>
        /// <param name="nSectors">The amount of sectors to read.</param>
        /// <returns>If the function succeeded, true, otherwise false.</returns>
        bool SyscallReadSectors(user_handle hnd, void* buff, size_t* nSectorsRead, uintptr_t lbaOffset, size_t nSectors);
        /// <summary>
        /// Syscall number: 47<para></para>
        /// Writes nSectors at lbaOffset.
        /// </summary>
        /// <param name="hnd">The drive handle to write to.</param>
        /// <param name="buff">The buffer with the data to write.</param>
        /// <param name="nSectorsWritten">The amount of sectors written.</param>
        /// <param name="lbaOffset">The lba offset of the first sector to write.</param>
        /// <param name="nSectors">The amount of sectors to write.</param>
        /// <returns>If the function succeeded, true, otherwise false.</returns>
        bool SyscallWriteSectors(user_handle hnd, const void* buff, size_t* nSectorsWritten, uintptr_t lbaOffset, size_t nSectors);

        /// <summary>
        /// Syscall number: 48<para></para>
        /// Gets the drive id of the handle.
        /// </summary>
        /// <param name="hnd">The handle to query.</param>
        /// <returns>The drive id, or 0xffffffff on failure.</returns>
        uint32_t SyscallDriveGetId(user_handle hnd);
        /// <summary>
        /// Syscall number: 49<para></para>
        /// Gets the partition id of the handle.
        /// </summary>
        /// <param name="hnd">The handle to query.</param>
        /// <returns>The partition id, or 0xffffffff on failure.</returns>
        uint32_t SyscallDriveGetPartitionId(user_handle hnd);

        /// <summary>
        /// Syscall number: 50<para></para>
        /// Queries the info of the drive.
        /// </summary>
        /// <param name="hnd">The handle to query.</param>
        /// <param name="nSectors">[out] The amount of sectors of the drive.</param>
        /// <param name="bytesPerSector">[out] The amount of bytes per sectors for the drive.</param>
        /// <param name="nPartitions">[out] The amount of partitions on the drive.</param>
        /// <returns>If the function succeeded, true, otherwise false.</returns>
        bool SyscallDriveQueryInfo(user_handle hnd, size_t* nSectors, size_t* bytesPerSector, size_t* nPartitions);
        /// <summary>
        /// Syscall number: 51<para></para>
        /// Queries the info of the partition.
        /// </summary>
        /// <param name="hnd">The handle to query.</param>
        /// <param name="nSectors">[out] The amount of sectors of the partition.</param>
        /// <param name="lbaOffset">[out] The lba offset of the partition.</param>
        /// <param name="filesystemName">[out, *szFilesystemName] The name of the filesystem.</param>
        /// <param name="szFilesystemName">[in, out] The size of the filesystem name.</param>
        /// <returns>If the function succeeded, true, otherwise false.</returns>
        bool SyscallDriveQueryPartitionInfo(user_handle hnd, size_t* nSectors, uint64_t* lbaOffset, char* filesystemName, size_t* szFilesystemName);

        /// <summary>
        /// Syscall number: 52<para></para>
        /// Queries whether the handle is a drive or a partition handle.
        /// </summary>
        /// <param name="hnd">The handle to query.</param>
        /// <returns>Whether the handle is a drive or a partition handle.</returns>
        bool SyscallDriveIsPartitionHandle(user_handle hnd);
        
        /// <summary>
        /// Syscall number: 53<para></para>
        /// Closes the drive.
        /// </summary>
        /// <param name="hnd">The drive to close.</param>
        /// <returns>If the function succeeded, true, otherwise false.</returns>
        bool SyscallCloseDriveHandle(user_handle hnd);
	}
}