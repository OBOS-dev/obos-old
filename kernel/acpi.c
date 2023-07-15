/*
	acpi.c

	Copyright (c) 2023 Omar Berrow
*/

#include <stddef.h>

#include "types.h"
#include "klog.h"
#include "terminal.h"
#include "inline-asm.h"

DWORD *SMI_CMD;
BYTE ACPI_ENABLE;
BYTE ACPI_DISABLE;
DWORD *PM1a_CNT;
DWORD *PM1b_CNT;
WORD SLP_TYPa;
WORD SLP_TYPb;
WORD SLP_EN;
WORD SCI_EN;
BYTE PM1_CNT_LEN;

#define sleep(x) { for(int i = 0; i < x / 4; i++) outb(0x80, 0); }

struct RSDPtr
{
	BYTE Signature[8];
	BYTE CheckSum;
	BYTE OemID[6];
	BYTE Revision;
	DWORD *RsdtAddress;
};

typedef struct _FACP
{
	BYTE Signature[4];
	DWORD Length;
	BYTE unneded1[40 - 8];
	DWORD *DSDT;
	BYTE unneded2[48 - 44];
	DWORD *SMI_CMD;
	BYTE ACPI_ENABLE;
	BYTE ACPI_DISABLE;
	BYTE unneded3[64 - 54];
	DWORD *PM1a_CNT_BLK;
	DWORD *PM1b_CNT_BLK;
	BYTE unneded4[89 - 72];
	BYTE PM1_CNT_LEN;
} FACP;

extern UINT16_T inw(UINT16_T port);

int memcmp(const void* mem1, const void* mem2, unsigned int size)
{
	const char* buf1_c = mem1;
	const char* buf2_c = mem2;
	for(int i = 0; i < size; i++)
		if(buf1_c[i] < buf2_c[i])
			return -1;
		else if (buf1_c[i] > buf2_c[i])
			return 1;
		else
			continue;
	return 0;
}

UINT32_T *acpiCheckRSDPtr(UINT32_T* ptr)
{
	char *sig = "RSD PTR ";
	struct RSDPtr* rsdp = (struct RSDPtr*)ptr;
	BYTE* bptr;
	BYTE check = 0;
	int i;

	if (memcmp(sig, rsdp, 8) == 0)
	{
		// check checksum rsdpd
		bptr = (BYTE*)ptr;
		for (i=0; i < sizeof(struct RSDPtr); i++)
		{
			check += *bptr;
			bptr++;
		}

		// found valid rsdpd   
		if (check == 0)
			return rsdp->RsdtAddress;
	}

	return NULL;
}

UINT32_T *acpiGetRSDPtr(void)
{
	unsigned int* addr = (PVOID)0x000E0000;
	unsigned int* rsdp = NULLPTR;	

	for (; addr < (UINT32_T*)0x00100000; addr += 0x10 / sizeof(addr))
	{
	   rsdp = acpiCheckRSDPtr(addr);
	   if (rsdp != NULL)
	      return rsdp;
	}

	INT32_T _ebda = *((short*)0x40E);
	_ebda = _ebda * 0x10 & 0x000FFFFF;
	INTPTR_T ebda = _ebda;

	for (addr = (unsigned int*)ebda; addr < (UINT32_T*)(ebda + 1024); addr += 0x10 / sizeof(addr))
	{
	   rsdp = acpiCheckRSDPtr(addr);
	   if (rsdp != NULL)
	      return rsdp;
	}	

	return NULLPTR;
}

int acpiCheckHeader(unsigned int *ptr, char *sig)
{
	if (memcmp(ptr, sig, 4) == 0)
	{
		char *checkPtr = (char *) ptr;
		int len = *(ptr + 1);
		char check = 0;
		while (0 < len--)
		{
			check += *checkPtr;
			checkPtr++;
		}
		if (check == 0)
			return 0;
	}
	return -1;
}



int acpiEnable(void)
{
	// Check if ACPI is enabled on the current computer.
	if ((inw((UINTPTR_T)PM1a_CNT), &SCI_EN) == 0)
	{
		if (SMI_CMD != 0 && ACPI_ENABLE != 0)
		{
			outb((UINTPTR_T)SMI_CMD, ACPI_ENABLE);
			int i;
			for (i = 0; i < 300; i++)
			{
				if (inw((UINTPTR_T)PM1a_CNT & SCI_EN) == 1)
					break;
				sleep(10);
			}
			if (PM1b_CNT != 0)
				for (; i < 300; i++)
				{
					if (inw((UINTPTR_T)PM1b_CNT & SCI_EN) == 1)
						break;
					sleep(10);
				}
			kassert(i < 300, "Could not enable ACPI.", 23);
		}
		else
			kpanic(KSTR_LITERAL("ACPI cannot be enabled on the current computer.\r\n"));
	}
	return 0;
}

int initAcpi(void)
{
	UINT32_T* ptr = acpiGetRSDPtr();

	if (ptr != NULL && acpiCheckHeader(ptr, "RSDT") == 0)
	{
		int entrys = *(ptr + 1);
		entrys = (entrys - 36) / 4;
		ptr += 36 / 4;

		while (0 < entrys--)
		{
			if (acpiCheckHeader((UINT32_T*)((UINTPTR_T)(*ptr)), "FACP") == 0)
			{
				entrys = -2;
				FACP* facp = (FACP*)((UINTPTR_T)*ptr);
				if (acpiCheckHeader(facp->DSDT, "DSDT") == 0)
				{
					char* S5Addr = (char*)facp->DSDT + 36;
					int dsdtLength = *(facp->DSDT + 1) - 36;
					while (0 < dsdtLength--)
					{
						if (memcmp(S5Addr, "_S5_", 4) == 0)
							break;
						S5Addr++;
					}
					kassert(dsdtLength > 0, "\\_S5 parse error.\r\n", 20);
					// check if \_S5 was found
					if (dsdtLength > 0)
					{
						// check for valid AML structure
						if ((*(S5Addr - 1) == 0x08 || (*(S5Addr - 2) == 0x08 && *(S5Addr - 1) == '\\')) && *(S5Addr + 4) == 0x12)
						{
							S5Addr += 5;
							S5Addr += ((*S5Addr & 0xC0) >> 6) + 2;   // calculate PkgLength size
							if (*S5Addr == 0x0A)
								S5Addr++;   // skip byteprefix
							SLP_TYPa = *(S5Addr) << 10;
							S5Addr++;
							if (*S5Addr == 0x0A)
								S5Addr++;   // skip byteprefix
							SLP_TYPb = *(S5Addr) << 10;
							SMI_CMD = facp->SMI_CMD;
							ACPI_ENABLE = facp->ACPI_ENABLE;
							ACPI_DISABLE = facp->ACPI_DISABLE;
							PM1a_CNT = facp->PM1a_CNT_BLK;
							PM1b_CNT = facp->PM1b_CNT_BLK;
							PM1_CNT_LEN = facp->PM1_CNT_LEN;
							SLP_EN = 1 << 13;
							SCI_EN = 1;
							return 0;
						}
					}
					else
						kassert(0, "\\_S5 not present.\n", 19);
				}
				else
					kassert(0, "DSDT invalid.\r\n", 15);
			}
			ptr++;
		}
		kassert(0, "No valid FACP present.\r\n", 24);
	}
	else
		kassert(0, "Computer doesn't support ACPI.\r\n", 32);
	kpanic(KSTR_LITERAL("Unspecified ACPI error\r\n"));
	return -1;
}

void acpiPowerOff(void)
{
	resetOnKernelPanic();
	kassert(SCI_EN != 0, "ACPI poweroff failed.\r\n", 24);

	acpiEnable();

	// Send the shutdown command,
	outw((UINTPTR_T)PM1a_CNT, SLP_TYPa | SLP_EN);
	if (PM1b_CNT != 0)
		outw((UINTPTR_T)PM1b_CNT, SLP_TYPb | SLP_EN);
	io_wait();
	io_wait();
	io_wait();
	io_wait();
	io_wait();
	kpanic("ACPI power off failed.\r\n", 24);
}