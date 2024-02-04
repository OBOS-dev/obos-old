/*
	arch/x86_64/syscall/handle.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <utils/hashmap.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/verify_pars.h>

#include <multitasking/process/process.h>

#include <multitasking/cpu_local.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <vfs/devManip/driveHandle.h>

#include <vfs/fileManip/fileHandle.h>
#include <vfs/fileManip/directoryIterator.h>

namespace obos
{
	namespace syscalls
	{
		user_handle ProcessRegisterHandle(void* _proc, void* objectAddress, ProcessHandleType type)
		{
			if (!_proc)
				_proc = thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			if (type == ProcessHandleType::INVALID ||
				!objectAddress)
				return 0xffffffffffffffff;
			process::Process* proc = (process::Process*)_proc;
			locks::SafeMutex lock_guard{ &proc->context.handleTableLock };
			lock_guard.Lock();
			auto &handleTable = proc->context.handleTable;
			user_handle handleValue = proc->context.nextHandleValue++;
			handleTable.emplace_at(handleValue, { objectAddress, type });
			return handleValue;
		}
		void* ProcessReleaseHandle(void* _proc, user_handle handle)
		{
			if (!_proc)
				_proc = thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			if (!ProcessVerifyHandle(_proc, handle, ProcessHandleType::INVALID))
				return nullptr;
			void* ret = ProcessGetHandleObject(_proc, handle);
			process::Process* proc = (process::Process*)_proc;
			locks::SafeMutex lock_guard{ &proc->context.handleTableLock };
			auto& handleTable = proc->context.handleTable;
			handleTable.remove(handle);
			return ret;
		}
		bool ProcessVerifyHandle(void* _proc, user_handle handle, ProcessHandleType type)
		{
			if (!_proc)
				_proc = thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			process::Process* proc = (process::Process*)_proc;
			auto& handleTable = proc->context.handleTable;
			if (type == ProcessHandleType::INVALID)
				return handleTable.contains(handle);
			bool has = handleTable.contains(handle);
			if (!has)
				return false;
			return handleTable.at(handle).second == type;
		}
		void* ProcessGetHandleObject(void* _proc, user_handle handle)
		{
			if (!_proc)
				_proc = thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			if (!ProcessVerifyHandle(_proc, handle, ProcessHandleType::INVALID))
				return nullptr;
			process::Process* proc = (process::Process*)_proc;
			auto& handleTable = proc->context.handleTable;
			return handleTable.at(handle).first;
		}
		ProcessHandleType ProcessGetHandleType(void* _proc, user_handle handle)
		{
			if (!_proc)
				_proc = thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			if (!ProcessVerifyHandle(_proc, handle, ProcessHandleType::INVALID))
				return ProcessHandleType::INVALID;
			process::Process* proc = (process::Process*)_proc;
			auto& handleTable = proc->context.handleTable;
			return handleTable.at(handle).second;
		}

		bool SyscallInvalidateHandle(uint64_t, user_handle* _handle)
		{
			if (!canAccessUserMemory(_handle, sizeof(*_handle), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			auto handle = *_handle;
			if (!ProcessVerifyHandle(nullptr, handle))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			auto type = ProcessGetHandleType(nullptr, handle);
			void* val = ProcessGetHandleObject(nullptr, handle);
			switch (type)
			{
			case obos::syscalls::ProcessHandleType::FILE_HANDLE:
				delete (vfs::FileHandle*)val;
				break;
			case obos::syscalls::ProcessHandleType::DRIVE_HANDLE:
				delete (vfs::DriveHandle*)val;
				break;
			case obos::syscalls::ProcessHandleType::THREAD_HANDLE:
				delete (thread::ThreadHandle*)val;
				break;
			case obos::syscalls::ProcessHandleType::VALLOCATOR_HANDLE:
				delete (memory::VirtualAllocator*)val;
				break;
			case obos::syscalls::ProcessHandleType::DIRECTORY_ITERATOR_HANDLE:
				delete (vfs::DirectoryIterator*)val;
				break;
			default:
				break;
			}
			ProcessReleaseHandle(nullptr, handle);
			return true;
		}
	}
}