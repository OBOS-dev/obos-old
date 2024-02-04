/*
	oboskrnl/arch/x86_64/memory_manager/allocate.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

namespace obos
{
	namespace memory
	{
		void InitializePhysicalMemoryManager();
	
		/// <summary>
		/// Allocates a physical page.
		/// </summary>
		/// <returns>The address of the page. Make sure to map this page before using it.</returns>
		OBOS_EXPORT uintptr_t allocatePhysicalPage(size_t nPages = 1);
		/// <summary>
		/// Marks a physical page as freed.
		/// </summary>
		/// <param name="addr">The address of the page to free</param>
		/// <returns>true on success, otherwise false.</returns>
		OBOS_EXPORT bool freePhysicalPage(uintptr_t addr, size_t nPages = 1);
		/// <summary>
		/// Queries whether a page is in the HHDM or not.
		/// </summary>
		/// <param name="addr">The address of the page to check.</param>
		/// <returns>true if the page is in the HHDM, otherwise false.</returns>
		OBOS_EXPORT bool PageInHHDM(uintptr_t addr);
	}
}