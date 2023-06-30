#ifndef __OBOS_TYPES_H
#define __OBOS_TYPES_H

#ifdef __GNUC__
#define attribute(exp) __attribute__((exp))
#else
#define attribute(exp)
#endif

#define NULLPTR ((PVOID)0)
#ifndef NULL
#	define NULL    NULLPTR
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
typedef	 UINT32_T			SIZE_T;
typedef UINT8_T				BYTE;
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
#endif