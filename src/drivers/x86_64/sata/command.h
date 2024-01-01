/*
	drivers/x86_64/sata/command.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

uint32_t FindCMDSlot(volatile struct HBA_PORT* pPort);
void StopCommandEngine(volatile struct HBA_PORT* pPort);
void StartCommandEngine(volatile struct HBA_PORT* pPort);