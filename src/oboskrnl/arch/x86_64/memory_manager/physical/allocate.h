/*
	oboskrnl/arch/x86_64/memory_manager/allocate.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace memory
	{
		void InitializePhysicalMemoryManager();
	
		/// <summary>
		/// Allocates a physical page.
		/// </summary>
		/// <returns>The address of the page. Make sure to map this page before using it.</returns>
		uintptr_t allocatePhysicalPage();
		/// <summary>
		/// Marks a physical page as freed.
		/// </summary>
		/// <param name="addr">The address of the page to free</param>
		/// <returns>true on success, otherwise false.</returns>
		bool freePhysicalPage(uintptr_t addr);
	}
}