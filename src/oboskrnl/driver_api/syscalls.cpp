/*
	driver_api/syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <driver_api/syscalls.h>
#include <driver_api/interrupts.h>
#include <driver_api/enums.h>

#include <memory_manager/liballoc/liballoc.h>
#include <memory_manager/physical.h>
#include <memory_manager/paging/allocate.h>

#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

#include <console.h>

#include <boot/boot.h>

#include <utils/memory.h>

#include <klog.h>

#include <multitasking/multitasking.h>

#include <process/process.h>

#define DEFINE_RESERVED_PARAMETERS volatile UINT32_T, volatile UINT32_T, volatile UINT32_T, volatile UINT32_T
#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && ((UINTPTR_T)(val)) <= ((UINTPTR_T)(rEnd)))

namespace obos
{
	namespace memory
	{
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress);
	}
	namespace driverAPI
	{
		static exitStatus RegisterDriver(DEFINE_RESERVED_PARAMETERS, DWORD driverID, serviceType type);
		static exitStatus RegisterInterruptHandler(DEFINE_RESERVED_PARAMETERS, DWORD driverId, BYTE interruptId, void(*handler)(const obos::interrupt_frame* frame));
		static exitStatus PicSendEoi(DEFINE_RESERVED_PARAMETERS, BYTE irq);
		static exitStatus EnableIRQ(DEFINE_RESERVED_PARAMETERS, BYTE irq);
		static exitStatus DisableIRQ(DEFINE_RESERVED_PARAMETERS, BYTE irq);
		static exitStatus RegisterReadCallback(DEFINE_RESERVED_PARAMETERS, DWORD driverId, void(*callback)(STRING outputBuffer, SIZE_T sizeBuffer));
		static exitStatus PrintChar(DEFINE_RESERVED_PARAMETERS, CHAR ch, bool flush);
		static exitStatus GetMultibootModule(DEFINE_RESERVED_PARAMETERS, DWORD moduleIndex, UINTPTR_T* moduleAddress, SIZE_T* size);
		static exitStatus RegisterFileReadCallback(DEFINE_RESERVED_PARAMETERS, DWORD driverId, void(*callback)(CSTRING filename, STRING outputBuffer, SIZE_T sizeBuffer));
		static exitStatus RegisterFileExistsCallback(DEFINE_RESERVED_PARAMETERS, DWORD driverId, char(*callback)(CSTRING filename, SIZE_T* size));
		static exitStatus MapPhysicalTo(DEFINE_RESERVED_PARAMETERS, UINTPTR_T physicalAddress, PVOID virtualAddress, UINT32_T flags);
		static exitStatus UnmapPhysicalTo(DEFINE_RESERVED_PARAMETERS, PVOID virtualAddress);
		static exitStatus Printf(DEFINE_RESERVED_PARAMETERS, CSTRING format, ...);
		static exitStatus GetPhysicalAddress(DEFINE_RESERVED_PARAMETERS, PVOID linearAddress, PVOID* physicalAddress);
		static void interruptHandler(const obos::interrupt_frame* frame);

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
		driverIdentification** g_registeredDrivers = nullptr;
		SIZE_T g_registeredDriversCapacity = 128;

		struct
		{
			VOID(*handler)(const obos::interrupt_frame* frame);
			driverIdentification* driver;
		} g_registeredInterruptHandlers[16];

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
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(GetMultibootModule));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(RegisterFileReadCallback));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(RegisterFileExistsCallback));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(MapPhysicalTo));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(UnmapPhysicalTo));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(Printf));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(GetPhysicalAddress));
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
				if (type > serviceType::SERVICE_TYPE_MAX_VALUE)
					return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
				driverIdentification* identity = new driverIdentification {
					driverID,
					multitasking::g_currentThread->owner,
					type,
					nullptr,
					nullptr
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
				if(i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i - 1])
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
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i - 1])
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
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i - 1])
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
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && interruptId != allowedInterrupts[i - 1])
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
			obos::RegisterInterruptHandler(interruptId, interruptHandler);
			g_registeredInterruptHandlers[interruptId - 0x20].handler = handler;
			g_registeredInterruptHandlers[interruptId - 0x20].driver = g_registeredDrivers[driverId];
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
			if (g_registeredDrivers[driverId]->service_type != serviceType::SERVICE_TYPE_USER_INPUT_DEVICE)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			g_registeredDrivers[driverId]->readCallback = (PVOID)callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus PrintChar(DEFINE_RESERVED_PARAMETERS, CHAR ch, bool flush) 
		{
			ConsoleOutputCharacter(ch, flush);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus GetMultibootModule(DEFINE_RESERVED_PARAMETERS, DWORD moduleIndex, UINTPTR_T* moduleStart, SIZE_T* size)
		{
			if (moduleIndex >= NUM_MODULES)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			multiboot_module_t* mod = ((multiboot_module_t*)g_multibootInfo->mods_addr) + moduleIndex;
			*moduleStart = mod->mod_start;
			*size = mod->mod_end - mod->mod_start;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus RegisterFileReadCallback(DEFINE_RESERVED_PARAMETERS, DWORD driverId, void(*callback)(CSTRING filename, STRING outputBuffer, SIZE_T sizeBuffer))
		{
			if (driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (g_registeredDrivers[driverId]->service_type != serviceType::SERVICE_TYPE_FILESYSTEM)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			g_registeredDrivers[driverId]->readCallback = (PVOID)callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus RegisterFileExistsCallback(DEFINE_RESERVED_PARAMETERS, DWORD driverId, char(*callback)(CSTRING filename, SIZE_T* size))
		{
			if (driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (g_registeredDrivers[driverId]->service_type != serviceType::SERVICE_TYPE_FILESYSTEM)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			g_registeredDrivers[driverId]->existsCallback = (PVOID)callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus MapPhysicalTo(DEFINE_RESERVED_PARAMETERS, UINTPTR_T physicalAddress, PVOID virtualAddress, UINT32_T flags)
		{
			if (inRange(virtualAddress, 0xC0000000, 0xE0000000))
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			if(!memory::kmap_physical(virtualAddress, 1, flags, (PVOID)physicalAddress))
				return exitStatus::EXIT_STATUS_ADDRESS_NOT_AVAILABLE;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus UnmapPhysicalTo(DEFINE_RESERVED_PARAMETERS, PVOID virtualAddress)
		{
			if (inRange(virtualAddress, 0xC0000000, 0xE0000000))
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			if (memory::VirtualFree(virtualAddress, 1) == 1)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus Printf(DEFINE_RESERVED_PARAMETERS, CSTRING format, ...)
		{
			va_list list;
			va_start(list, format);

			vprintf(format, list);

			va_end(list);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}

#if defined(__i686__)
		static exitStatus GetPhysicalAddress(DEFINE_RESERVED_PARAMETERS, PVOID linearAddress, PVOID* physicalAddress)
		{
			if (!memory::g_pageDirectory->getPageTableAddress(memory::PageDirectory::addressToIndex((UINTPTR_T)linearAddress)))
				return exitStatus::EXIT_STATUS_ADDRESS_NOT_AVAILABLE;
			*physicalAddress = reinterpret_cast<PVOID>(memory::kmap_pageTable(memory::g_pageDirectory->getPageTable(memory::PageDirectory::addressToIndex((UINTPTR_T)linearAddress)))
				[memory::PageDirectory::addressToPageTableIndex((UINTPTR_T)linearAddress)] & 0xFFFFF000);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
#elif defined(__x86_64__)
		static exitStatus GetPhysicalAddress(DEFINE_RESERVED_PARAMETERS, PVOID, PVOID*)
		{
			return exitStatus::EXIT_STATUS_NOT_IMPLEMENTED;
		}
#endif

		static void interruptHandler(const obos::interrupt_frame* frame)
		{
			if (g_registeredInterruptHandlers[frame->intNumber - 32].driver)
			{
				g_registeredInterruptHandlers[frame->intNumber - 32].driver->process->doContextSwitch();
				g_registeredInterruptHandlers[frame->intNumber - 32].handler(frame);
				multitasking::g_currentThread->owner->doContextSwitch();
			}
		}
	}
}