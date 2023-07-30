/*
	klog.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define kpanic(message) _kpanic("Kernel panic at line " TOSTRING(__LINE__) ", file " __FILE__ ", message:\r\n\t" message "\r\n")

namespace obos
{
	
	void _kpanic(CSTRING message);
}