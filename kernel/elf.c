/*
    elf.c

    Copyright (c) 2023 Omar Berrow
*/
#include "elf.h"
#include "elfStructures.h"
#include "paging.h"

#include "liballoc/liballoc_1_1.h"

static PVOID memset(PVOID buf, CHAR ch, SIZE_T size)
{
    for (int i = 0; i < size; ((PCHAR)buf)[i++] = ch); 
    return buf;
}
static PVOID memcpy(PVOID destination, PVOID source, SIZE_T count)
{
    for (int i = 0; i < count; i++)
        ((PBYTE)destination)[i] = ((PBYTE)source)[i];
    return destination;
}
static BOOL checkMachine(Elf32_Ehdr* elfHeader)
{
    return  elfHeader->e_ident[EI_CLASS] == ELFCLASS32 &&
            elfHeader->e_ident[EI_DATA] == ELFDATA2LSB &&
            elfHeader->e_machine == EM_386;
}

static BOOL checkIdent(Elf32_Ehdr* elfHeader)
{
    return  elfHeader->e_ident[EI_MAG0] == ELFMAG0 &&
            elfHeader->e_ident[EI_MAG1] == ELFMAG1 &&
            elfHeader->e_ident[EI_MAG2] == ELFMAG2 &&
            elfHeader->e_ident[EI_MAG3] == ELFMAG3 &&
            elfHeader->e_ident[EI_VERSION] == EV_CURRENT;
}

#define PAGE_SIZE 4096

// Taken from https://github.com/malisal/loaders/blob/master/elf/elf.c, and slightly modified.
// Please see the license and copyright, as this isn't technically my code: https://github.com/malisal/loaders/blob/master/LICENSE.
BOOL elf_load(char* elf_start, void* stack, size_t* base_addr, size_t* entry)
{
    Elf32_Ehdr* hdr;
    Elf32_Phdr* phdr;

    int x;
    BOOL elf_prot = FALSE;
    size_t base;

    hdr = (Elf32_Ehdr*)elf_start;
    phdr = (Elf32_Phdr*)(elf_start + hdr->e_phoff);

    if (hdr->e_type == ET_DYN)
    {
        // If this is a DYNAMIC ELF (can be loaded anywhere), set a random base address
        base = (size_t)kalloc_pages(1, FALSE, TRUE);
        kfree_pages((void*)base, 1);
    }
    else
        base = 0;

    if (base_addr != NULL)
        *base_addr = -1;

    if (entry != NULL)
        *entry = base + hdr->e_entry;

    for (x = 0; x < hdr->e_phnum; x++)
    {

        if (phdr[x].p_type != PT_LOAD)
            continue;

        if (!phdr[x].p_filesz)
            continue;

        void* map_start = (void*)ROUND_DOWN(phdr[x].p_vaddr, PAGE_SIZE);
        extern char endKernel;
        if ((UINTPTR_T)map_start < (UINTPTR_T)&endKernel)
            return FALSE;
        int round_down_size = (void*)phdr[x].p_vaddr - map_start;

        int map_size = ROUND_UP(phdr[x].p_memsz + round_down_size, PAGE_SIZE);

        kalloc_pagesAt(base + map_start, map_size / 4096, FALSE, TRUE);
        memcpy((void*)base + phdr[x].p_vaddr, elf_start + phdr[x].p_offset, phdr[x].p_filesz);

        // Zero-out BSS, if it exists
        if (phdr[x].p_memsz > phdr[x].p_filesz)
            memset((void*)(base + phdr[x].p_vaddr + phdr[x].p_filesz), 0, phdr[x].p_memsz - phdr[x].p_filesz);

        // Set proper protection on the area
        if (phdr[x].p_flags & PF_R)
            elf_prot = TRUE;

        if (phdr[x].p_flags & PF_W)
            elf_prot = FALSE;

        /*if (phdr[x].p_flags & PF_X)
            elf_prot |= PROT_EXEC;*/

        ksetPagesProtection((unsigned char*)(base + map_start), map_size, elf_prot);

        // Clear cache on this area
        //cacheflush(base + map_start, (size_t)(map_start + map_size), 0);

        // Is this the lowest memory area we saw. That is, is this the ELF base address?
        if (base_addr != NULL && (*base_addr == -1 || *base_addr > (size_t)(base + map_start)))
            *base_addr = (size_t)(base + map_start);
    }
    return TRUE;
}


UINTPTR_T LoadElfFile(PBYTE elfImage, SIZE_T imageSize)
{
    if (imageSize < sizeof(Elf32_Ehdr))
        return 0;
    Elf32_Ehdr elfHeader;
    memset(&elfHeader, 0, sizeof(Elf32_Ehdr));
    memcpy(&elfHeader, elfImage, sizeof(Elf32_Ehdr));
    if (!checkIdent(&elfHeader))
        return 0;
    if (!checkMachine(&elfHeader))
        return 0;
   if(elfHeader.e_type != ET_EXEC)
        return 0;
   // Loads the elf file at the base address, if it has one, otherwise, at a random address.
   return elf_load((PCHAR)elfImage, NULLPTR, NULLPTR, NULLPTR) ? elfHeader.e_entry : 0;
}
