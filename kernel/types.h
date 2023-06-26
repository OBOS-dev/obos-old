#ifndef __OBOS_TYPES_H
#define __OBOS_TYPES_H

#define NULLPTR ((PVOID)0)

typedef enum { FALSE, TRUE } BOOL;
typedef unsigned char UINT8_T;
typedef unsigned short UINT16_T;
typedef unsigned int UINT32_T;
typedef UINT32_T SIZE_T;
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
typedef INT32_T INTPTR_T;
typedef UINT32_T UINTPTR_T;
#endif