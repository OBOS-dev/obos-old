/*
	driver_api/syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <driver_api/syscalls.h>
#include <driver_api/interrupts.h>
#include <driver_api/enums.h>

#include <utils/hashTable.h>

#include <memory_manager/liballoc/liballoc.h>

#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

#include <console.h>

#define DEFINE_RESERVED_PARAMETERS volatile UINT32_T, volatile UINT32_T, volatile UINT32_T, volatile UINT32_T

namespace obos
{
	extern void(*readFromKeyboard)(STRING outputBuffer, SIZE_T sizeBuffer);
	namespace driverAPI
	{
		static exitStatus RegisterDriver(DEFINE_RESERVED_PARAMETERS, DWORD driverID, serviceType type);
		static exitStatus RegisterInterruptHandler(DEFINE_RESERVED_PARAMETERS, DWORD driverId, BYTE interruptId, void(*handler)(const obos::interrupt_frame* frame));
		static exitStatus PicSendEoi(DEFINE_RESERVED_PARAMETERS, BYTE irq);
		static exitStatus EnableIRQ(DEFINE_RESERVED_PARAMETERS, BYTE irq);
		static exitStatus DisableIRQ(DEFINE_RESERVED_PARAMETERS, BYTE irq);
		static exitStatus RegisterReadCallback(DEFINE_RESERVED_PARAMETERS, DWORD driverId, void(*callback)(STRING outputBuffer, SIZE_T sizeBuffer));
		static exitStatus PrintChar(DEFINE_RESERVED_PARAMETERS, CHAR ch, bool flush);

		/*struct driverIdentification
		{
			DWORD driverId = 0;
			serviceType m_service = serviceType::SERVICE_TYPE_INVALID;
			static UINT32_T hash(driverIdentification* key)
			{
				return key->driverId;
			}
		};
		utils::HashTable<DWORD, driverIdentification*, driverIdentification> g_registeredDrivers;*/
		struct driverIdentification
		{
			DWORD driverId = 0;
			serviceType service_type = serviceType::SERVICE_TYPE_INVALID;
		};
		driverIdentification** g_registeredDrivers = nullptr;
		SIZE_T g_registeredDriversCapacity = 128;

		void RegisterSyscalls()
		{
			g_registeredDrivers = (driverIdentification**)kcalloc(sizeof(g_registeredDriversCapacity), sizeof(driverIdentification*));
			int currentSyscall = 0;
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(RegisterDriver));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(RegisterInterruptHandler));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(PicSendEoi));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(EnableIRQ));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(DisableIRQ));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(RegisterReadCallback));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(PrintChar));
		}

		static exitStatus RegisterDriver(DEFINE_RESERVED_PARAMETERS, DWORD driverID, serviceType type)
		{
			if(!g_registeredDrivers[driverID])
			{
				if (driverID > g_registeredDriversCapacity)
				{
					// g_registeredDriversCapacity + g_registeredDriversCapacity / 4 should be the same as g_registeredDriversCapacity * 1.25f, but truncated.
					g_registeredDriversCapacity += g_registeredDriversCapacity / 4;
					g_registeredDrivers = (driverIdentification**)krealloc(g_registeredDrivers, g_registeredDriversCapacity);
				}
				if (type == serviceType::SERVICE_TYPE_INVALID)
					return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
				driverIdentification* identity = new driverIdentification {
					driverID,
					type
				};
				g_registeredDrivers[driverID] = identity;
				return exitStatus::EXIT_STATUS_SUCCESS;
			}
			return exitStatus::EXIT_STATUS_ALREADY_REGISTERED;
		}
		static exitStatus RegisterInterruptHandler(DEFINE_RESERVED_PARAMETERS, DWORD driverId, BYTE interruptId, void(*handler)(const obos::interrupt_frame* frame))
		{
			if (driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (interruptId < 0x21)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
			serviceType type = g_registeredDrivers[driverId]->service_type;
			switch (type)
			{
			case serviceType::SERVICE_TYPE_INVALID:
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			case serviceType::SERVICE_TYPE_STORAGE_DEVICE:
			{
				BYTE allowedInterrupts[] = {
					0x20 + 6,
					0x20 + 9,
					0x20 + 10,
					0x20 + 11,
					0x20 + 14,
					0x20 + 15,
				};
				SIZE_T i = 0;
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i]; i++);
				if(i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i])
					return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
				break;
			}
			case serviceType::SERVICE_TYPE_USER_INPUT_DEVICE:
			{
				BYTE allowedInterrupts[] = {
					0x20 + 1,
					0x20 + 9,
					0x20 + 10,
					0x20 + 11,
					0x20 + 12,
				};
				SIZE_T i = 0;
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i])
					return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
				break;
			}
			case serviceType::SERVICE_TYPE_GRAPHICS_DEVICE:
			{
				BYTE allowedInterrupts[] = {
					0x20 + 9,
					0x20 + 10,
					0x20 + 11,
				};
				SIZE_T i = 0;
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i])
					return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
				break;
			}
			case serviceType::SERVICE_TYPE_MONITOR:
			{
				BYTE allowedInterrupts[] = {
					0x20 + 8,
				};
				SIZE_T i = 0;
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i])
					return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
				break;
			}
			case serviceType::SERVICE_TYPE_KERNEL_EXTENSION:
				if(interruptId >= 0x21 && interruptId < 0x30)
					break;
				return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
			case serviceType::SERVICE_TYPE_COMMUNICATION:
			{
				BYTE allowedInterrupts[] = {
					0x20 + 3,
					0x20 + 4,
					0x20 + 5,
					0x20 + 7,
					0x20 + 9,
					0x20 + 10,
					0x20 + 11,
				};
				SIZE_T i = 0;
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i])
					return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
				break;
			}

			case serviceType::SERVICE_TYPE_FILESYSTEM:
			case serviceType::SERVICE_TYPE_VIRTUAL_STORAGE_DEVICE:
			case serviceType::SERVICE_TYPE_VIRTUAL_USER_INPUT_DEVICE:
			case serviceType::SERVICE_TYPE_VIRTUAL_COMMUNICATION:
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			default:
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			}
			obos::RegisterInterruptHandler(interruptId, handler);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus PicSendEoi(DEFINE_RESERVED_PARAMETERS, BYTE irq)
		{
			SendEOI(irq);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus DisableIRQ(DEFINE_RESERVED_PARAMETERS, BYTE irq)
		{
			if (irq == 0)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			Pic pic{ (irq > 7) ? Pic::PIC2_CMD : Pic::PIC1_CMD, (irq > 7) ? Pic::PIC2_DATA : Pic::PIC2_DATA };
			pic.disableIrq(irq);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus EnableIRQ(DEFINE_RESERVED_PARAMETERS, BYTE irq)
		{
			if (irq == 0)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			Pic pic{ (irq > 7) ? Pic::PIC2_CMD : Pic::PIC1_CMD, (irq > 7) ? Pic::PIC2_DATA : Pic::PIC2_DATA };
			pic.enableIrq(irq);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus RegisterReadCallback(DEFINE_RESERVED_PARAMETERS, DWORD driverId, void(*callback)(STRING outputBuffer, SIZE_T sizeBuffer))
		{
			if (driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			// TODO: Make this syscall do something.
			readFromKeyboard = callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus PrintChar(DEFINE_RESERVED_PARAMETERS, CHAR ch, bool flush) 
		{
			ConsoleOutputCharacter(ch, flush);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
	}
}