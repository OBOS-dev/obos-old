/*
	oboskrnl/descriptors/idt/idt.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#if defined(__i686__)
#include <descriptors/idt/i686/idt.h>
#elif defined(__x86_64__)
#include <descriptors/idt/x86-64/idt.h>
#endif