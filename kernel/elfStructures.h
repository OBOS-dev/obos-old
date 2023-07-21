#ifndef __OBOS_ELFSTRUCTURES_H
#define __OBOS_ELFSTRUCTURES_H

#include "types.h"

#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6
#define EI_PAD     7
#define EI_NIDENT  16

#define ELFMAG0 (BYTE)0x7f
#define ELFMAG1 (BYTE)'E'
#define ELFMAG2 (BYTE)'L'
#define ELFMAG3 (BYTE)'F'

#define ET_NONE 0 /*No file type*/
#define ET_REL 1 /*Relocatable file*/
#define ET_EXEC 2 /*Executable file*/
#define ET_DYN 3 /*Shared object file*/
#define ET_CORE 4 /*Core file*/
#define ET_LOPROC 0xff00 /*Processor-specific*/
#define ET_HIPROC 0xffff /*Processor-specific*/

#define EV_NONE 0
#define EV_CURRENT 1

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EM_NONE 0 /*No machine*/
#define EM_M32 1 /*AT&T WE 32100*/
#define EM_SPARC 2 /*SPARC*/
#define EM_386 3 /*Intel 80386 (x86 architecture)*/
#define EM_68K 4 /*Motorola 68000*/
#define EM_88K 5 /*Motorola 88000*/
#define EM_860 7 /*Intel 80860*/
#define EM_MIPS 8 /*MIPS RS3000*/

// Taken from https://github.com/malisal/loaders/blob/master/elf/elf.c 
#define ROUND_UP(v, s) ((v + s - 1) & -s)
#define ROUND_DOWN(v, s) (v & -s)

// Taken from the linux kernel.
#define PF_R 0x4
#define PF_W 0x2

typedef UINT32_T Elf32_Addr;
typedef UINT16_T Elf32_Half;
typedef UINT32_T Elf32_Off;
typedef INT32_T Elf32_Sword;
typedef UINT32_T Elf32_Word;

typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	Elf32_Word p_type;
	Elf32_Word p_offset;
	Elf32_Word p_vaddr; 
	Elf32_Word p_paddr; 
	Elf32_Word p_filesz;
	Elf32_Word p_memsz; 
	Elf32_Word p_flags;
	Elf32_Word p_align; 
} Elf32_Phdr;

enum
{
	PT_NULL,
	PT_LOAD,
	PT_DYNAMIC,
	PT_INTERP,
	PT_NOTE,
	PT_SHLIB,
	PT_PHDR,
	PT_LOPROC = 0x70000000, //reserved
	PT_HIPROC = 0x7FFFFFFF  //reserved
};

#endif