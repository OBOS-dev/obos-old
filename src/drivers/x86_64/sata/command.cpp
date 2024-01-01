/*
	drivers/x86_64/sata/command.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <x86_64-utils/asm.h>

#include "command.h"
#include "structs.h"

extern volatile HBA_MEM* g_generalHostControl;

uint32_t FindCMDSlot(volatile HBA_PORT* pPort)
{
    uint32_t slots = (pPort->sact | pPort->ci);
    return obos::bsf(~slots);
}
void StopCommandEngine(volatile HBA_PORT* pPort)
{
    pPort->cmd &= ~(1<<0); // PxCMD.st

	// Clear FRE (bit 4)
	pPort->cmd &= ~(1<<4);

	// Wait until FR (bit 14), CR (bit 15) are cleared
	while (1)
	{
		if (pPort->cmd & (1<<14))
			continue;
		if (pPort->cmd & (1<<15))
			continue;
		break;
	}

}
void StartCommandEngine(volatile HBA_PORT* pPort)
{
    while (pPort->cmd & (1<<15))
    {}

	// Set FRE (bit 4) and ST (bit 0)
	pPort->cmd |= (1<<4);
	pPort->cmd |= (1<<0);
}