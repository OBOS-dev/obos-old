#pragma once

#ifdef OBOS_DRIVER
#	ifndef __INTELLISENSE__
#		define OBOS_EXPORT __attribute__((weak))
#	else
#		define OBOS_EXPORT
#	endif
#elif defined(OBOS_KERNEL)
#	define OBOS_EXPORT
#else
#	error You should not be including kernel headers.
#endif