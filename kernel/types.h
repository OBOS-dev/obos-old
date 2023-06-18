#ifndef __OBOS_INTEGER_H
#define __OBOS_INTEGER_H

#ifdef _GNU_C
#define _cdecl __attribute__((__cdecl__))
#else
#define _cdecl
#endif

#define NULLPTR ((PVOID)0)

typedef enum { FALSE, TRUE } BOOL;
typedef unsigned long SIZE_T;
typedef unsigned char UINT8_T;
typedef unsigned short UINT16_T;
typedef unsigned int UINT32_T;
typedef unsigned long long int UINT64_T;
typedef char INT8_T;
typedef short INT16_T;
typedef int INT32_T;
typedef long long int INT64_T;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned long long int QWORD;
typedef int INT;
typedef char CHAR, *PCHAR;
typedef const PCHAR PCCHAR;
typedef char *STRING;
typedef const char *CSTRING;
typedef void VOID, *PVOID;
typedef const void *PCVOID;
#ifdef __x86_64__
typedef INT64_T INTPTR_T;
typedef UINT64_T UINTPTR_T;
#else
typedef INT32_T INTPTR_T;
typedef UINT32_T UINTPTR_T;
#endif
#endif