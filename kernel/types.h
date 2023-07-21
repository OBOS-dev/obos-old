/*
	types.h

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_TYPES_H
#define __OBOS_TYPES_H

#ifdef __GNUC__
#	define attribute(exp) __attribute__((exp))
#else
#	define attribute(exp)
#endif

#define NULLPTR ((PVOID)0)
#ifndef NULL
#	define NULL 0
#endif
#define NUL		((CHAR)0)

typedef enum attribute(__packed__)
{ FALSE, TRUE }				BOOL;
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

typedef _Atomic BOOL				ATOMIC_BOOL;
typedef _Atomic unsigned char		ATOMIC_UINT8_T;
typedef _Atomic unsigned short		ATOMIC_UINT16_T;
typedef _Atomic unsigned int		ATOMIC_UINT32_T;
typedef _Atomic unsigned long long  ATOMIC_UINT64_T;
typedef _Atomic char				ATOMIC_INT8_T;
typedef _Atomic short				ATOMIC_INT16_T;
typedef _Atomic int					ATOMIC_INT32_T;
typedef _Atomic long long			ATOMIC_INT64_T;
typedef	_Atomic UINT32_T			ATOMIC_SIZE_T;
typedef _Atomic UINT8_T				ATOMIC_BYTE;
typedef _Atomic UINT16_T			ATOMIC_WORD;
typedef _Atomic UINT32_T			ATOMIC_DWORD;
typedef _Atomic UINT64_T			ATOMIC_QWORD;
typedef _Atomic INT32_T				ATOMIC_INTPTR_T;
typedef _Atomic UINT32_T			ATOMIC_UINTPTR_T;
typedef _Atomic int					ATOMIC_INT, *ATOMIC_PINT;
typedef _Atomic short				ATOMIC_SHORT, *ATOMIC_PSHORT;
typedef _Atomic char				ATOMIC_CHAR, *ATOMIC_PCHAR;
typedef	_Atomic const CHAR			ATOMIC_CCHAR, *ATOMIC_PCCHAR;
typedef _Atomic char*				ATOMIC_STRING;
typedef _Atomic const char*			ATOMIC_CSTRING;
typedef _Atomic void			   *ATOMIC_PVOID;
typedef _Atomic const void*			ATOMIC_PCVOID;

#endif