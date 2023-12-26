/*
	oboskrnl/allocators/vmm/vmm.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <allocators/vmm/vmm.h>
#include <allocators/vmm/arch.h>

#include <multitasking/process/process.h>

#include <multitasking/cpu_local.h>

#include <memory_manipulation.h>

namespace obos
{
	namespace memory
	{
#define ROUND_ADDRESS_DOWN(addr, size) ((uintptr_t)(addr) / (size) * (size))
		size_t VirtualAllocator::m_pageSize;
		VirtualAllocator::VirtualAllocator(process::Process* owner)
			:m_owner{ owner }
		{
			if (!m_pageSize)
				m_pageSize = _Impl_GetPageSize();
		}

		void VirtualAllocator::Initialize(process::Process* owner)
		{
			m_owner = owner;
			if (!m_pageSize)
				m_pageSize = _Impl_GetPageSize();
		}

		void* VirtualAllocator::VirtualAlloc(void* _base, size_t size, uintptr_t flags)
		{
			if (!m_pageSize)
				m_pageSize = _Impl_GetPageSize();
			_base = (void*)ROUND_ADDRESS_DOWN(_base, m_pageSize);
			if (!_Impl_IsValidAddress(_base))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return nullptr;
			}
			const size_t nPages = (size % m_pageSize) ? (size / m_pageSize + 1) : (size / m_pageSize);
			if (!_base)
			{
				_base = _Impl_FindUsableAddress(m_owner, nPages);
				if (!_base)
				{
					SetLastError(OBOS_ERROR_NO_FREE_REGION);
					return nullptr;
				}
			}
			uint32_t status = 0;
			void* ret = _Impl_ProcVirtualAlloc(m_owner, (void*)_base, nPages, flags, &status);
			switch (status)
			{
			case VALLOC_SUCCESS:
				break;
			case VALLOC_BASE_ADDRESS_USED:
				SetLastError(OBOS_ERROR_BASE_ADDRESS_USED);
				ret = nullptr;
				break;
			case VALLOC_INVALID_PARAMETER:
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				ret = nullptr;
				break;
			case VALLOC_ACCESS_DENIED:
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				ret = nullptr;
				break;
			default:
				break;
			}
			return ret;
		}
		bool VirtualAllocator::VirtualFree(void* _base, size_t size)
		{
			if (!m_pageSize)
				m_pageSize = _Impl_GetPageSize();
			_base = (void*)ROUND_ADDRESS_DOWN(_base, m_pageSize);
			if (!_base)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			const size_t nPages = (size % m_pageSize) ? (size / m_pageSize + 1) : (size / m_pageSize);
			uint32_t status = 0;
			_Impl_ProcVirtualFree(m_owner, _base, nPages, &status);
			bool ret = false;
			switch (status)
			{
			case VFREE_SUCCESS:
				ret = true;
				break;
			case VFREE_ACCESS_DENIED:
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				break;
			case VFREE_INVALID_PARAMETER:
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				break;
			case VFREE_MEMORY_UNMAPPED:
				SetLastError(OBOS_ERROR_ADDRESS_UNMAPPED);
				break;
			default:
				break;
			}
			return ret;
		}
		bool VirtualAllocator::VirtualProtect(void* _base, size_t size, uintptr_t flags)
		{
			if (!m_pageSize)
				m_pageSize = _Impl_GetPageSize();
			_base = (void*)ROUND_ADDRESS_DOWN(_base, m_pageSize);
			if (!_base)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			const size_t nPages = (size % m_pageSize) ? (size / m_pageSize + 1) : (size / m_pageSize);
			uint32_t status = 0;
			_Impl_ProcVirtualProtect(m_owner, _base, nPages, flags, &status);
			bool ret = false;
			switch (status)
			{
			case VPROTECT_SUCCESS:
				ret = true;
				break;
			case VPROTECT_ACCESS_DENIED:
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				break;
			case VPROTECT_INVALID_PARAMETER:
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				break;
			case VPROTECT_MEMORY_UNMAPPED:
				SetLastError(OBOS_ERROR_ADDRESS_UNMAPPED);
				break;
			default:
				break;
			}
			return ret;
		}
		bool VirtualAllocator::VirtualGetProtection(void* _base, size_t size, uintptr_t* flags)
		{
			if (!m_pageSize)
				m_pageSize = _Impl_GetPageSize();
			_base = (void*)ROUND_ADDRESS_DOWN(_base, m_pageSize);
			if (!_base)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			const size_t nPages = (size % m_pageSize) ? (size / m_pageSize + 1) : (size / m_pageSize);
			uint32_t status = 0;
			_Impl_ProcVirtualGetProtection(m_owner, _base, nPages, flags, &status);
			bool ret = false;
			switch (status)
			{
			case VGETPROT_SUCCESS:
				ret = true;
				break;
			case VGETPROT_ACCESS_DENIED:
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				break;
			case VGETPROT_INVALID_PARAMETER:
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				break;
			default:
				break;
			}
			return ret;
		}

		void* VirtualAllocator::Memcpy(void* remoteDest, void* localSrc, size_t size)
		{
			if (!m_pageSize)
				m_pageSize = _Impl_GetPageSize();
			if (!m_owner)
				return utils::memcpy(remoteDest, localSrc, size);
			uint32_t status = 0;
			void* ret = _Impl_Memcpy(m_owner, remoteDest, localSrc, size, &status);
			switch (status)
			{
			case MEMCPY_SUCCESS:
				break;
			case MEMCPY_DESTINATION_PFAULT:
			case MEMCPY_SOURCE_PFAULT:
				SetLastError(OBOS_ERROR_PAGE_FAULT);
				ret = nullptr;
				break;
			}
			return ret;
		}
		bool VirtualAllocator::IsUsermodeAllocator()
		{
			process::Process* proc = m_owner;
			if (m_owner)
				proc = m_owner;
			else
				if (thread::getCurrentCpuLocalPtr())
					proc = (process::Process*)thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			return proc ? proc->isUsermode : false;
		}
	}
}