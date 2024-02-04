/*
	oboskrnl/arch/x86_64/syscall/vfs/dir.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/verify_pars.h>

#include <arch/x86_64/syscall/vfs/dir.h>

#include <vfs/fileManip/directoryIterator.h>

#include <vfs/vfsNode.h>

#include <allocators/vmm/vmm.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t DirectorySyscallHandler(uint64_t syscall, void* args)
		{
			bool(*rB_h)(user_handle) = nullptr;
			bool(*rB_h_cP_sP)(user_handle hnd, char* path, size_t* size) = nullptr;
			switch (syscall)
			{
			case 61:
				return SyscallMakeDirectoryIterator();
			case 66:
			{
				struct _par
				{
					alignas(0x10) user_handle hnd;
					alignas(0x10) const char* path;
				} *par = (_par*)args;
				if (!canAccessUserMemory(par, sizeof(*par), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return SyscallDirectoryIteratorOpen(par->hnd, par->path);
			}
			case 62:
				rB_h = SyscallDirectoryIteratorNext;
				goto _rB_h;
			case 63:
				rB_h = SyscallDirectoryIteratorPrevious;
				goto _rB_h;
			case 67:
				rB_h = SyscallDirectoryIteratorClose;
			{
				_rB_h:
				struct _par
				{
					alignas(0x10) user_handle hnd;
				} *par = (_par*)args;
				if (!canAccessUserMemory(par, sizeof(*par), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return rB_h(par->hnd);
			}
			case 64:
				rB_h_cP_sP = SyscallDirectoryIteratorGet;
				goto _rB_h_cP_sP;
			case 68:
				rB_h_cP_sP = SyscallDirectoryIteratorGetParent;
			{
				_rB_h_cP_sP:
				struct _par
				{
					alignas(0x10) user_handle hnd;
					alignas(0x10) char* path;
					alignas(0x10) size_t* size;
				} *par = (_par*)args;
				if (!canAccessUserMemory(par, sizeof(*par), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return rB_h_cP_sP(par->hnd, par->path, par->size);
			}
			case 65:
			{
				struct _par
				{
					alignas(0x10) user_handle hnd;
					alignas(0x10) bool* oStatus;
				} *par = (_par*)args;
				if (!canAccessUserMemory(par, sizeof(*par), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return SyscallDirectoryIteratorEnd(par->hnd, par->oStatus);
			}
			default:
				break;
			}
			return 0;
		}

		user_handle SyscallMakeDirectoryIterator()
		{
			return ProcessRegisterHandle(nullptr, new vfs::DirectoryIterator{}, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE);
		}
		
		bool SyscallDirectoryIteratorOpen(user_handle hnd, const char* path)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
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
			vfs::DirectoryIterator* handle = (vfs::DirectoryIterator*)ProcessGetHandleObject(nullptr, hnd);
			return handle->OpenAt(path);
		}

		bool SyscallDirectoryIteratorNext(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::DirectoryIterator* handle = (vfs::DirectoryIterator*)ProcessGetHandleObject(nullptr, hnd);
			delete[] ((*handle)++);
			return true;
		}
		bool SyscallDirectoryIteratorPrevious(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::DirectoryIterator* handle = (vfs::DirectoryIterator*)ProcessGetHandleObject(nullptr, hnd);
			delete[] ((*handle)--);
			return true;
		}
		bool SyscallDirectoryIteratorGet(user_handle hnd, char* path, size_t* size)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!path && !size)
				return true; // We don't need to do anything but verify the handle so it seems like we did something.
			// We need to handle three cases with path && size:
			// path && !size: Fail
			//  path && size: Copy size bytes to path, and set *size to the size of the full path.
			// !path && size: Copy no bytes, and set *size to the size of the full path.
			if (path && !size)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (size && !canAccessUserMemory(size, sizeof(*size), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (path && !canAccessUserMemory(path, *size, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::DirectoryIterator& handle = *(vfs::DirectoryIterator*)ProcessGetHandleObject(nullptr, hnd);
			const char* res = *handle;
			if (path)
				utils::memcpy(path, res, *size);
			if (size)
				*size = utils::strlen(res);
			delete[] res;
			return true;
		}
		bool SyscallDirectoryIteratorEnd(user_handle hnd, bool* status)
		{
			if (status && !canAccessUserMemory(status, sizeof(*status), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				*status = false;
				return false;
			}
			if (status)
				*status = true;
			vfs::DirectoryIterator& handle = *(vfs::DirectoryIterator*)ProcessGetHandleObject(nullptr, hnd);
			return !handle;
		}

		bool SyscallDirectoryIteratorClose(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::DirectoryIterator& handle = *(vfs::DirectoryIterator*)ProcessGetHandleObject(nullptr, hnd);
			return handle.Close();
		}
	
		static const char* GetDirectoryIteratorParent(vfs::DirectoryIterator& handle)
		{
			using namespace vfs;
			// There is no need to handle the case where the directory node is a mount point or directory entry because
			// both MountPoint and DirectoryEntry inherit from GeneralFSNode, which contains the parent member.
			DirectoryEntry* parent = ((GeneralFSNode*)handle.GetDirectoryNode())->parent;
			if (!parent)
				return nullptr;
			return parent->path;
		}
		bool SyscallDirectoryIteratorGetParent(user_handle hnd, char* path, size_t* size)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::DIRECTORY_ITERATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!path && !size)
				return true; // We don't need to do anything but verify the handle so it seems like we did something.
			// We need to handle three cases with path && size:
			// path && !size: Fail
			//  path && size: Copy size bytes to path, and set *size to the size of the full path.
			// !path && size: Copy no bytes, and set *size to the size of the full path.
			if (path && !size)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (size && !canAccessUserMemory(size, sizeof(*size), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (path && !canAccessUserMemory(path, *size, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::DirectoryIterator& handle = *(vfs::DirectoryIterator*)ProcessGetHandleObject(nullptr, hnd);
			const char* res = GetDirectoryIteratorParent(handle);
			if (!res)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return false;
			}
			if (path)
				utils::memcpy(path, res, *size);
			if (size)
				*size = utils::strlen(res);
			return true;
		}
	}
}