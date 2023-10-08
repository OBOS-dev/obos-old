/*
	types.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#ifdef __GNUC__
#	define attribute(exp) __attribute__((exp))
#else
#	define attribute(exp)
#endif

#define NULLPTR nullptr
#ifndef NULL
#	define NULL ((DWORD)0)
#endif
#define NUL		((CHAR)0)

#define NUM_MODULES 4

#ifdef __cplusplus
typedef bool				BOOL;
#else
typedef _Bool				BOOL;
#define false 0
#define true 1
#endif
typedef unsigned char		UINT8_T;
typedef unsigned short		UINT16_T;
typedef unsigned int		UINT32_T;
typedef unsigned long long  UINT64_T;
typedef char				INT8_T;
typedef short				INT16_T;
typedef int					INT32_T;
typedef long long			INT64_T;
typedef UINT8_T				BYTE, *PBYTE;
typedef UINT16_T			WORD;
typedef UINT32_T			DWORD;
typedef UINT64_T			QWORD;
#ifdef __i686__
typedef INT32_T				INTPTR_T;
typedef UINT32_T			UINTPTR_T;
#elif __x86_64__
typedef INT64_T				INTPTR_T;
typedef UINT64_T			UINTPTR_T;
#endif
typedef int					INT, *PINT;
typedef short				SHORT, *PSHORT;
typedef char				CHAR, *PCHAR;
typedef	const CHAR			CCHAR, *PCCHAR;
typedef char			   *STRING;
typedef const char		   *CSTRING;
typedef void				VOID, *PVOID;
typedef const void		   *PCVOID;
typedef	UINTPTR_T			SIZE_T;
typedef UINTPTR_T			HANDLE;