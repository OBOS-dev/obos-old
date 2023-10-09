#ifdef _DEBUG
/*
	oboskrnl/stack_canary.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <klog.h>

#include <cpuid.h>

UINTPTR_T __stack_chk_guard = 0x15E4F6DABD6F8A;

extern "C" [[noreturn]] void __stack_chk_fail(void)
{
	obos::kpanic(nullptr, __builtin_return_address(0), "Stack smashing detected!\n");
}
//
//namespace obos
//{
//	void srand(unsigned int seed);
//	int rand();
//	inline UINT64_T rdtsc();
//}
//
//void InitializeStackCanary()
//{
//}
#endif