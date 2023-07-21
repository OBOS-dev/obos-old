/*
    elf.h

    Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_ELF_H
#define __OBOS_ELF_H

#include "types.h"

/// <summary>
/// Loads a elf file to memory.
/// </summary>
/// <param name="read">A callback to read from the file. Returns FALSE when data can't be read, otherwise TRUE.</param>
/// <returns>A pointer to entry point</returns>
UINTPTR_T LoadElfFile(PBYTE elfImage, SIZE_T imageSize);

#endif