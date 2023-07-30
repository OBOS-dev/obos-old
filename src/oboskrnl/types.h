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

#ifndef NULL
#	define NULL 0
#endif
#define NUL		((CHAR)0)

typedef bool				BOOL;
typedef unsigned char		UINT8_T;
typedef unsigned short		UINT16_T;
typedef unsigned int		UINT32_T;
typedef unsigned long long  UINT64_T;
typedef char				INT8_T;
typedef short				INT16_T;
typedef int					INT32_T;
typedef long long			INT64_T;
typedef	UINT32_T			SIZE_T;
typedef UINT8_T				BYTE, *PBYTE;
typedef UINT16_T			WORD;
typedef UINT32_T			DWORD;
typedef UINT64_T			QWORD;
typedef INT32_T				INTPTR_T;
typedef UINT32_T			UINTPTR_T;
typedef int					INT, *PINT;
typedef short				SHORT, *PSHORT;
typedef char				CHAR, *PCHAR;
typedef	const CHAR			CCHAR, *PCCHAR;
typedef char			   *STRING;
typedef const char		   *CSTRING;
typedef void				VOID, *PVOID;
typedef const void		   *PCVOID;
typedef UINTPTR_T			HANDLE;
