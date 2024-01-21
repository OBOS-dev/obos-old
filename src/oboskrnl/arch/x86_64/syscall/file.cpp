/*
	arch/x86_64/syscall/file.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/file.h>
#include <arch/x86_64/syscall/verify_pars.h>

#include <vfs/fileManip/fileHandle.h>

#include <allocators/vmm/vmm.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t FileHandleSyscallHandler(uint64_t syscall, void* args)
		{
			uintptr_t(*basicGetterFunction)(user_handle hnd) = nullptr;
			uintptr_t getterFailureReturnValue = 0;
			switch (syscall)
			{
			case 13:
				return SyscallMakeFileHandle();
			case 14:
			{
				struct _par
				{
					alignas(0x10) user_handle hnd;
					alignas(0x10) const char* path;
					alignas(0x10) uint32_t options;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(args, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				// TODO: Test
				auto pageSize = memory::VirtualAllocator::GetPageSize();
				if (!canAccessUserMemory(pars->path, pageSize, false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				size_t szPath = 0;
				while (szPath >= (pageSize - ((uintptr_t)pars->path % pageSize)))
				{
					for (szPath = 0; pars->path[szPath] && szPath < (pageSize - ((uintptr_t)pars->path % pageSize)); szPath++);
					if (!canAccessUserMemory(pars->path + szPath, pageSize, false))
					{
						SetLastError(OBOS_ERROR_INVALID_PARAMETER);
						return false;
					}
				}
				return SyscallFileOpen(pars->hnd, pars->path, pars->options);
			}
			case 15:
			{
				struct _par
				{
					alignas(0x10) user_handle hnd;
					alignas(0x10) char* data;
					alignas(0x10) size_t nToRead;
					alignas(0x10) bool peek;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof * pars, false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				if (!canAccessUserMemory(pars->data, pars->nToRead, true))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return SyscallFileRead(pars->hnd, pars->data, pars->nToRead, pars->peek);
			}
			case 16:
				basicGetterFunction = reinterpret_cast<uintptr_t(*)(user_handle)>((void*)SyscallFileEof);
				getterFailureReturnValue = true;
				goto _basicGetterFunction_call;
			case 17:
				basicGetterFunction = reinterpret_cast<uintptr_t(*)(user_handle)>((void*)SyscallFileGetPos);
				getterFailureReturnValue = UINTPTR_MAX;
				goto _basicGetterFunction_call;
			case 18:
				basicGetterFunction = reinterpret_cast<uintptr_t(*)(user_handle)>((void*)SyscallFileGetFlags);
				getterFailureReturnValue = UINT32_MAX;
				goto _basicGetterFunction_call;
			case 22:
				basicGetterFunction = reinterpret_cast<uintptr_t(*)(user_handle)>((void*)SyscallFileClose);
				getterFailureReturnValue = false;
				goto _basicGetterFunction_call;
			case 19:
				basicGetterFunction = reinterpret_cast<uintptr_t(*)(user_handle)>((void*)SyscallFileGetFileSize);
				getterFailureReturnValue = ((size_t)-1);
			_basicGetterFunction_call:
				{
					struct _pars
					{
						user_handle hnd;
					} *pars = (_pars*)args;
					if (!canAccessUserMemory(pars, sizeof(*pars), false))
					{
						SetLastError(OBOS_ERROR_INVALID_PARAMETER);
						return getterFailureReturnValue;
					}
					return basicGetterFunction(pars->hnd);
				}
			case 20:
			{
				// TODO: Test.
				struct _pars
				{
					user_handle hnd;
					char* oPath;
					size_t* oSzPath;
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return 0;
				}
				if (!pars->oSzPath && pars->oPath)
					return 0; // Nothing is needed to be done.
				if (pars->oSzPath)
				{
					if (!canAccessUserMemory(pars->oSzPath, sizeof(*pars->oSzPath), true))
					{
						SetLastError(OBOS_ERROR_INVALID_PARAMETER);
						return 0;
					}
				}
				char* path = nullptr;
				size_t szPath = 0;
				SyscallFileGetParent(pars->hnd, path, &szPath);
				if (pars->oSzPath)
					*pars->oSzPath = szPath;
				if (pars->oPath)
				{
					path = new char[szPath + 1];
					SyscallFileGetParent(pars->hnd, path, nullptr);
					if (!canAccessUserMemory(pars->oPath, szPath, true))
					{
						SetLastError(OBOS_ERROR_INVALID_PARAMETER);
						return 0;
					}
					utils::memcpy(pars->oPath, path, szPath);
					delete[] path;
				}
				break;
			}
			case 21:
			{
				// SyscallFileSeekTo
				struct _pars
				{
					alignas(0x10) user_handle hnd;
					alignas(0x10) uintptr_t count; 
					alignas(0x10) uint32_t from;
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return SyscallFileSeekTo(pars->hnd, pars->count, pars->from);
			}
			// Syscall 22 is handled in basic getter syscalls, as it has the same return value and parameters overall.
			}
			return 0;
		}
		
		user_handle SyscallMakeFileHandle()
		{
			return ProcessRegisterHandle(nullptr, new vfs::FileHandle, ProcessHandleType::FILE_HANDLE);
		}

		bool SyscallFileOpen(user_handle hnd, const char* path, uint32_t options)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->Open(path, (vfs::FileHandle::OpenOptions)options);
		}
		bool SyscallFileRead(user_handle hnd, char* data, size_t nToRead, bool peek)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->Read(data, nToRead, peek);
		}

		bool SyscallFileEof(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return true;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->Eof();
		}
		uintptr_t SyscallFileGetPos(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return true;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->GetPos();
		}
		uint32_t SyscallFileGetFlags(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return true;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->GetFlags();
		}
		size_t SyscallFileGetFileSize(user_handle hnd) 
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return true;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->GetFileSize();
		}
		void SyscallFileGetParent(user_handle hnd, char* path, size_t* sizePath)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			handle->GetParent(path, sizePath);
		}

		uintptr_t SyscallFileSeekTo(user_handle hnd, uintptr_t count, uint32_t from)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return UINTPTR_MAX;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->SeekTo(count, (vfs::FileHandle::SeekPlace)from);
		}

		bool SyscallFileClose(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::FILE_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			vfs::FileHandle* handle = (vfs::FileHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->Close();
		}
	}
}