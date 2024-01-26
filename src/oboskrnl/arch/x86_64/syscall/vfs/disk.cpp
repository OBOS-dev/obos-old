/*
    oboskrnl/arch/x86_64/syscall/vfs/disk.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/verify_pars.h>

#include <arch/x86_64/syscall/vfs/disk.h>

#include <allocators/vmm/vmm.h>

#include <vfs/devManip/driveHandle.h>

namespace obos
{
    namespace syscalls
    {
        uintptr_t DriveSyscallHandler(uint64_t syscall, void* args)
        {
            using basic_io_t = bool(*)(user_handle, void*, size_t*, uintptr_t, size_t);
            uint32_t(*general_query_u32)(user_handle) = nullptr;
            bool(*userhandle_bool)(user_handle) = nullptr;
            basic_io_t basic_io = nullptr;
            switch (syscall)
            {
            case 54:
                return SyscallMakeDriveHandle();
            case 45:
            {
                struct _pars
                {
                    alignas(0x10) user_handle hnd;
                    alignas(0x10) const char* path;
                    alignas(0x10) uint32_t options;
                } *pars = (_pars*)args;
                if (!canAccessUserMemory(pars, sizeof(*pars), false))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return false;
                }
                if (!ProcessVerifyHandle(nullptr, pars->hnd, ProcessHandleType::DRIVE_HANDLE))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return false;
                }
                return SyscallOpenDrive(pars->hnd, pars->path, (vfs::DriveHandle::OpenOptions)pars->options);
            }
            case 48:
                general_query_u32 = SyscallDriveGetId;
                goto _general_query_u32;
            case 49:
                general_query_u32 = SyscallDriveGetPartitionId;
            {
                _general_query_u32:
                struct _pars
                {
                    alignas(0x10) user_handle hnd;
                } *pars = (_pars*)args;
                if (!canAccessUserMemory(pars, sizeof(*pars), false))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return 0xffffffff;
                }
                return general_query_u32(pars->hnd);
            }
            case 52:
                userhandle_bool = SyscallDriveIsPartitionHandle;
                goto _userhandle_bool;
            case 53:
                userhandle_bool = SyscallCloseDriveHandle;
            {
                _userhandle_bool:
                struct _pars
                {
                    alignas(0x10) user_handle hnd;
                } *pars = (_pars*)args;
                if (!canAccessUserMemory(pars, sizeof(*pars), false))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return 0xffffffff;
                }
                return userhandle_bool(pars->hnd);
            }
            case 46:
                basic_io = SyscallReadSectors;
                goto _basic_io;
            case 47:
                basic_io = (basic_io_t)SyscallWriteSectors;
            _basic_io:
            {
                struct _pars
                {
                    alignas(0x10) user_handle hnd;
                    alignas(0x10) void* buff;
                    alignas(0x10) size_t* pPar1;
                    alignas(0x10) uintptr_t lbaOffset;
                    alignas(0x10) size_t nSectors;
                } *pars = (_pars*)args;
                if (!canAccessUserMemory(pars, sizeof(*pars), false))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return false;
                }
                return basic_io(pars->hnd, pars->buff, pars->pPar1, pars->lbaOffset, pars->nSectors);
            }
            case 50:
            {
                struct _pars
                {
                    alignas(0x10) user_handle hnd;
                    alignas(0x10) size_t* nSectors;
                    alignas(0x10) size_t* bytesPerSector;
                    alignas(0x10) size_t* nPartitions;
                } *pars = (_pars*)args;
                if (!canAccessUserMemory(pars, sizeof(*pars), false))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return false;
                }
                return SyscallDriveQueryInfo(pars->hnd, pars->nSectors, pars->bytesPerSector, pars->nPartitions);
            }
            case 51:
            {
                struct _pars
                {
                    alignas(0x10) user_handle hnd;
                    alignas(0x10) size_t* nSectors;
                    alignas(0x10) size_t* bytesPerSector;
                    alignas(0x10) char* filesystemName; 
                    alignas(0x10) size_t* szFilesystemName;
                } *pars = (_pars*)args;
                if (!canAccessUserMemory(pars, sizeof(*pars), false))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return false;
                }
                return SyscallDriveQueryPartitionInfo(pars->hnd, pars->nSectors, pars->bytesPerSector, pars->filesystemName, pars->szFilesystemName);
            }
            default:
                break;
            }
            return 0;
        }

        user_handle SyscallMakeDriveHandle()
        {
            return ProcessRegisterHandle(nullptr, new vfs::DriveHandle{}, ProcessHandleType::DRIVE_HANDLE);
        }

        bool SyscallOpenDrive(user_handle hnd, const char* path, uint32_t options)
        {
            auto pageSize = memory::VirtualAllocator::GetPageSize();
            if (!canAccessUserMemory(path, pageSize, false))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            size_t szPath = 0;
            while (szPath >= (pageSize - ((uintptr_t)path % pageSize)))
            {
                for (szPath = 0; path[szPath] && szPath < (pageSize - ((uintptr_t)path % pageSize)); szPath++);
                if (!canAccessUserMemory(path + szPath, pageSize, false))
                {
                    SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                    return false;
                }
            }
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            return handle->OpenDrive(path, (vfs::DriveHandle::OpenOptions)options);
        }

        bool SyscallReadSectors(user_handle hnd, void* buff, size_t* nSectorsRead, uintptr_t lbaOffset, size_t nSectors)
        {
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            size_t bytesPerSector = 0;
            if (!handle->QueryInfo(nullptr, &bytesPerSector, nullptr))
                return false;
            if (!canAccessUserMemory(buff, bytesPerSector * nSectors, true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            size_t _nSectorsRead = 0;
            bool ret = handle->ReadSectors(buff, &_nSectorsRead, lbaOffset, nSectors);
            if (nSectorsRead && canAccessUserMemory(nSectorsRead, sizeof(*nSectorsRead), true))
                *nSectorsRead = _nSectorsRead;
            return ret;
        }
        bool SyscallWriteSectors(user_handle hnd, const void* buff, size_t* nSectorsWritten, uintptr_t lbaOffset, size_t nSectors)
        {
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            size_t bytesPerSector = 0;
            if (!handle->QueryInfo(nullptr, &bytesPerSector, nullptr))
                return false;
            if (!canAccessUserMemory(buff, bytesPerSector * nSectors, false))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            size_t _nSectorsWritten = 0;
            bool ret = handle->WriteSectors(buff, &_nSectorsWritten, lbaOffset, nSectors);
            if (nSectorsWritten && canAccessUserMemory(nSectorsWritten, sizeof(*nSectorsWritten), true))
                *nSectorsWritten = _nSectorsWritten;
            return ret;
        }

        uint32_t SyscallDriveGetId(user_handle hnd)
        {
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            return handle->GetDriveId();
        }
        uint32_t SyscallDriveGetPartitionId(user_handle hnd)
        {
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            return handle->GetPartitionId();
        }

        bool SyscallDriveQueryInfo(user_handle hnd, size_t* nSectors, size_t* bytesPerSector, size_t* nPartitions)
        {
            if (nSectors && !canAccessUserMemory(nSectors, sizeof(*nSectors), true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            if (bytesPerSector && !canAccessUserMemory(bytesPerSector, sizeof(*bytesPerSector), true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            if (nPartitions && !canAccessUserMemory(nPartitions, sizeof(*nPartitions), true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            return handle->QueryInfo(nSectors, bytesPerSector, nPartitions);
        }
        bool SyscallDriveQueryPartitionInfo(user_handle hnd, size_t* nSectors, uint64_t* lbaOffset, char* filesystemName, size_t* szFilesystemName)
        {
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            if (nSectors && !canAccessUserMemory(nSectors, sizeof(*nSectors), true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            if (lbaOffset && !canAccessUserMemory(lbaOffset, sizeof(*lbaOffset), true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            if (szFilesystemName && !canAccessUserMemory(szFilesystemName, sizeof(*szFilesystemName), true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            if (filesystemName && !canAccessUserMemory(filesystemName, *szFilesystemName, true))
            {
                SetLastError(OBOS_ERROR_INVALID_PARAMETER);
                return false;
            }
            const char* fsName = nullptr;
            bool ret = handle->QueryPartitionInfo(nSectors, lbaOffset, (filesystemName != nullptr) ? &fsName : nullptr);
            if (fsName)
            {
                size_t szFsName = utils::strlen(fsName);
                utils::memcpy(filesystemName, fsName, *szFilesystemName < szFsName ? *szFilesystemName : szFsName);
            }
            return ret;
        }

        bool SyscallDriveIsPartitionHandle(user_handle hnd)
        {
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            return handle->IsPartitionHandle();
        }

        bool SyscallCloseDriveHandle(user_handle hnd)
        {
            vfs::DriveHandle* handle = (vfs::DriveHandle*)ProcessGetHandleObject(nullptr, hnd);
            return handle->Close();
        }
    }
}