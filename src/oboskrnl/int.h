#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t byte;
#ifdef __cplusplus
template<typename T>
using ptr = T*;
#endif
#ifdef __INTELLISENSE__
// It doesn't matter what the type actually is for intellisense.
// This is only so intellisense gets out of my way.
using __uint128_t = uint64_t;
#endif