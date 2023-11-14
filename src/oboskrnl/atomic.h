/*
	oboskrnl/atomic.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	void atomic_set(bool* val);
	void atomic_clear(bool* val);
	bool atomic_test(bool* val);

	void atomic_inc(uint64_t& val);
	void atomic_dec(uint64_t& val);
}