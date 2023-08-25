/*
	oboskrnl/memory_manager/x86-64/allocate.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/allocate.h>
#include <memory_manager/paging/init.h>

#include <types.h>

namespace obos
{
	namespace memory
	{
		PVOID VirtualAlloc(PVOID _base, SIZE_T nPages, UINTPTR_T flags)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base);
			if (utils::testBitInBitfield(flags, 63))
				utils::clearBitInBitfield(flags, 63);
			else
				utils::setBitInBitfield(flags, 63);
			if (!base)
			{
				// Determine the address for the caller.
				for (base += 0x200000; base > 0x200000; base += 4096)
				{
					if (!g_level4PageMap->getLevel3PageMapAddress(PageMap::computeIndexAtAddress(base, 3)))
						break; // We found available memory.
					for (int level3 = 0; level3 < 512; level3++)
					{
						if (!g_level4PageMap->getLevel3PageMap(PageMap::computeIndexAtAddress(base, 3)))
							break;

					}
				}
			}
			return ;
		}
	}
}