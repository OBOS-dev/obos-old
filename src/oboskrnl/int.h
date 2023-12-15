#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t byte;
#ifdef __cplusplus
template<typename T>
using ptr = T*;
#endif