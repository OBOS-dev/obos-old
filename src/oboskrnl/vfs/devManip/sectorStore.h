/*
	Copyright (c) 2024 Omar Berrow

	oboskrnl/vfs/devManip/sectorStore.h
*/

#pragma once

#include <int.h>

#include <allocators/vmm/vmm.h>

// Contains a class to store drive sectors that uses the VMM instead of the kernel heap to allocate memory.
// This will help prevent immense heap fragmentation.

namespace obos
{
	namespace utils
	{
		class SectorStorage
		{
		public:
			SectorStorage() = default;
			SectorStorage(size_t size)
			{
				if (!size)
					return;
				m_ptr = (byte*)m_valloc.VirtualAlloc(nullptr, size, 0);
				m_realSize = size;
				const size_t pageSize = m_valloc.GetPageSize();
				m_size = size / pageSize * pageSize + ((size % pageSize) != 0);
			}

			// This pointer should not be stored, as it's subject to relocation.
			// If you end up storing it, after every resize, update that pointer.
			byte* data() const { return m_ptr; }
			void resize(size_t size)
			{
				if (!size)
				{
					free();
					return;
				}
				const size_t pageSize = m_valloc.GetPageSize();
				size_t oldSize = m_size;
				size = m_size = size / pageSize * pageSize + ((size % pageSize) != 0);
				m_realSize = size;
				if (size == oldSize)
					return;
				if (size < oldSize)
				{
					// Truncuate the block.
					m_valloc.VirtualFree(m_ptr + size, oldSize - size);
					return;
				}
				// Grow the block.
				// Find out if there's more space after the block.
				const size_t nPages = size / pageSize + ((size % pageSize) != 0);
				uintptr_t* flags = new uintptr_t[nPages];
				m_valloc.VirtualGetProtection(m_ptr + oldSize, oldSize - size, flags);
				size_t i = 0;
				for (; i < nPages && !(flags[i] & memory::PROT_IS_PRESENT); i++);
				if (flags[i] & memory::PROT_IS_PRESENT)
				{
					// If so, allocate the pages after m_ptr
					m_valloc.VirtualAlloc(m_ptr + oldSize, oldSize - size, 0);
					return;
				}
				// If not, relocate m_ptr.
				_ImplRelocatePtr(oldSize);
			}

			void free()
			{
				if (m_ptr)
					m_valloc.VirtualFree(m_ptr, m_size);
				m_ptr = nullptr;
				m_size = 0;
				m_realSize = 0;
			}
			size_t length() const { return m_realSize; }
			size_t length_pageSizeAligned() const { return m_size; }

			~SectorStorage()
			{
				free();
			}
		private:
			void _ImplRelocatePtr(size_t oldSize)
			{
				void* newPtr = m_valloc.VirtualAlloc(nullptr, m_size, 0);
				utils::memcpy(newPtr, m_ptr, m_size);
				m_valloc.VirtualFree(m_ptr, oldSize);
				m_ptr = (byte*)newPtr;
			}
			byte* m_ptr = nullptr;
			size_t m_size = 0;
			size_t m_realSize = 0;
			memory::VirtualAllocator m_valloc{ nullptr };
		};
	}
}