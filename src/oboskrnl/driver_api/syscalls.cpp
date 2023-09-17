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
#include <multitasking/threadHandle.h>
#include <multitasking/thread.h>

#include <process/process.h>

#define DEFINE_RESERVED_PARAMETERS volatile PVOID parameters
#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && ((UINTPTR_T)(val)) <= ((UINTPTR_T)(rEnd)))

#if defined(__i686__)
#define STACK_SIZE 4
#elif defined(__x86_64__)
#define STACK_SIZE 8
#endif

extern "C" void callDriverIrqHandler(PBYTE newRsp, UINTPTR_T* l4PageMap, void(*irqHandler)());

namespace obos
{
	namespace memory
	{
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress);
	}
	extern char kernelStart;
	namespace driverAPI
	{
		static exitStatus RegisterDriver(DEFINE_RESERVED_PARAMETERS);
		static exitStatus RegisterInterruptHandler(DEFINE_RESERVED_PARAMETERS);
		static exitStatus PicSendEoi(DEFINE_RESERVED_PARAMETERS);
		static exitStatus EnableIRQ(DEFINE_RESERVED_PARAMETERS);
		static exitStatus DisableIRQ(DEFINE_RESERVED_PARAMETERS);
		static exitStatus RegisterReadCallback(DEFINE_RESERVED_PARAMETERS);
		static exitStatus PrintChar(DEFINE_RESERVED_PARAMETERS);
		static exitStatus GetMultibootModule(DEFINE_RESERVED_PARAMETERS);
		static exitStatus RegisterFileReadCallback(DEFINE_RESERVED_PARAMETERS);
		static exitStatus RegisterFileExistsCallback(DEFINE_RESERVED_PARAMETERS);
		static exitStatus MapPhysicalTo(DEFINE_RESERVED_PARAMETERS);
		static exitStatus UnmapPhysicalTo(DEFINE_RESERVED_PARAMETERS);
		static exitStatus Printf(CSTRING format, ...);
		static exitStatus GetPhysicalAddress(DEFINE_RESERVED_PARAMETERS);
		static exitStatus RegisterRecursiveFileIterateCallback(DEFINE_RESERVED_PARAMETERS);
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
			VOID(*handler)();
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
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(RegisterRecursiveFileIterateCallback));
		}

		static exitStatus RegisterDriver(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T driverID;
				alignas(STACK_SIZE) serviceType type;
			} *pars = (_par*)parameters;
			if(!g_registeredDrivers[pars->driverID])
			{
				if (pars->driverID > g_registeredDriversCapacity)
				{
					// g_registeredDriversCapacity + g_registeredDriversCapacity / 4 should be the same as g_registeredDriversCapacity * 1.25f, but truncated.
					g_registeredDriversCapacity += g_registeredDriversCapacity / 4;
					g_registeredDrivers = (driverIdentification**)krealloc(g_registeredDrivers, g_registeredDriversCapacity);
				}
				if (pars->type == serviceType::SERVICE_TYPE_INVALID)
					return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
				if (pars->type > serviceType::SERVICE_TYPE_MAX_VALUE)
					return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
				driverIdentification* identity = new driverIdentification {
					(BYTE)pars->driverID,
					multitasking::g_currentThread->owner,
					pars->type,
					nullptr,
					nullptr
				};
				g_registeredDrivers[pars->driverID] = identity;
				return exitStatus::EXIT_STATUS_SUCCESS;
			}
			return exitStatus::EXIT_STATUS_ALREADY_REGISTERED;
		}
		static exitStatus RegisterInterruptHandler(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T driverId;
				alignas(STACK_SIZE) UINTPTR_T interruptId;
				alignas(STACK_SIZE) void(*handler)();
			} attribute(aligned(8)) *pars = (_par*)parameters;
			if (pars->driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[pars->driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (pars->interruptId < 0x21)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
			serviceType type = g_registeredDrivers[pars->driverId]->service_type;
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
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i]; i++);
				if(i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i - 1])
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
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i - 1])
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
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i - 1])
					return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
				break;
			}
			case serviceType::SERVICE_TYPE_MONITOR:
			{
				BYTE allowedInterrupts[] = {
					0x20 + 8,
				};
				SIZE_T i = 0;
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i])
					return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
				break;
			}
			case serviceType::SERVICE_TYPE_KERNEL_EXTENSION:
				if(pars->interruptId >= 0x21 && pars->interruptId < 0x30)
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
				for (; i < sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i]; i++);
				if (i == sizeof(allowedInterrupts) / sizeof(allowedInterrupts[0]) && pars->interruptId != allowedInterrupts[i - 1])
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
			obos::RegisterInterruptHandler(pars->interruptId, interruptHandler);
			g_registeredInterruptHandlers[pars->interruptId - 0x20].handler = pars->handler;
			g_registeredInterruptHandlers[pars->interruptId - 0x20].driver = g_registeredDrivers[pars->driverId];
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus PicSendEoi(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) BYTE irq;
			} *pars = (_par*)parameters;
			SendEOI(pars->irq);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus DisableIRQ(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) BYTE irq;
			} *pars = (_par*)parameters;
			if (pars->irq == 0)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			Pic pic{ (pars->irq > 7) ? Pic::PIC2_CMD : Pic::PIC1_CMD, (pars->irq > 7) ? Pic::PIC2_DATA : Pic::PIC2_DATA };
			pic.disableIrq(pars->irq);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus EnableIRQ(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) BYTE irq;
			} *pars = (_par*)parameters;
			if (pars->irq == 0)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			Pic pic{ (pars->irq > 7) ? Pic::PIC2_CMD : Pic::PIC1_CMD, (pars->irq > 7) ? Pic::PIC2_DATA : Pic::PIC2_DATA };
			pic.enableIrq(pars->irq);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus RegisterReadCallback(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T driverId;
				alignas(STACK_SIZE) void(*callback)(STRING outputBuffer, SIZE_T sizeBuffer);
			} *pars = (_par*)parameters;
			if (pars->driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[pars->driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (g_registeredDrivers[pars->driverId]->service_type != serviceType::SERVICE_TYPE_USER_INPUT_DEVICE)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			g_registeredDrivers[pars->driverId]->readCallback = (PVOID)pars->callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus PrintChar(DEFINE_RESERVED_PARAMETERS) 
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T ch;
				alignas(STACK_SIZE) UINTPTR_T flush;
			} *pars = (_par*)parameters;
			ConsoleOutputCharacter(pars->ch, pars->flush);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus GetMultibootModule(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T moduleIndex; 
				alignas(STACK_SIZE) UINTPTR_T* moduleStart; 
				alignas(STACK_SIZE) SIZE_T* size;
			} *pars = (_par*)parameters;
			if (pars->moduleIndex >= NUM_MODULES)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			multiboot_module_t* mod = ((multiboot_module_t*)g_multibootInfo->mods_addr) + pars->moduleIndex;
			*pars->moduleStart = mod->mod_start;
			*pars->size = mod->mod_end - mod->mod_start;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus RegisterFileReadCallback(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T driverId;
				alignas(STACK_SIZE) void(*callback)(CSTRING filename, STRING outputBuffer, SIZE_T sizeBuffer);
			} *pars = (_par*)parameters;
			if (pars->driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[pars->driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (g_registeredDrivers[pars->driverId]->service_type != serviceType::SERVICE_TYPE_FILESYSTEM)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			g_registeredDrivers[pars->driverId]->readCallback = (PVOID)pars->callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus RegisterFileExistsCallback(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T driverId;
				alignas(STACK_SIZE) void(*callback)(CSTRING filename, SIZE_T* size);
			} *pars = (_par*)parameters;
			if (pars->driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[pars->driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (g_registeredDrivers[pars->driverId]->service_type != serviceType::SERVICE_TYPE_FILESYSTEM)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			g_registeredDrivers[pars->driverId]->existsCallback = (PVOID)pars->callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus MapPhysicalTo(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T physicalAddress;
				alignas(STACK_SIZE) PVOID virtualAddress; 
				alignas(STACK_SIZE) UINTPTR_T flags;
			} *pars = (_par*)parameters;
			if (inRange(pars->virtualAddress, &kernelStart, &memory::endKernel))
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			if(!memory::kmap_physical(pars->virtualAddress, pars->flags, (PVOID)pars->physicalAddress))
				return exitStatus::EXIT_STATUS_ADDRESS_NOT_AVAILABLE;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus UnmapPhysicalTo(DEFINE_RESERVED_PARAMETERS)
		{
			PVOID* virtualAddress = (PVOID*)parameters;
			if (inRange(virtualAddress, 0xC0000000, 0xE0000000))
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			if (memory::VirtualFree(*virtualAddress, 1) == 1)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus Printf(CSTRING format, ...)
		{
			va_list list;
			va_start(list, format);

			vprintf(format, list);

			va_end(list);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}

#if defined(__i686__)
		static exitStatus GetPhysicalAddress(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) PVOID linearAddress;
				alignas(STACK_SIZE) PVOID* physicalAddress;
			} *pars = (_par*)parameters;
			if (!memory::g_pageDirectory->getPageTableAddress(memory::PageDirectory::addressToIndex((UINTPTR_T)pars->linearAddress)))
				return exitStatus::EXIT_STATUS_ADDRESS_NOT_AVAILABLE;
			*pars->physicalAddress = reinterpret_cast<PVOID>(memory::kmap_pageTable(memory::g_pageDirectory->getPageTable(memory::PageDirectory::addressToIndex((UINTPTR_T)pars->linearAddress)))
				[memory::PageDirectory::addressToPageTableIndex((UINTPTR_T)pars->linearAddress)] & 0xFFFFF000);
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
#elif defined(__x86_64__)
		static exitStatus GetPhysicalAddress(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) PVOID _virtualAddress;
				alignas(STACK_SIZE) PVOID* physicalAddress;
			} *pars = (_par*)parameters;
			PBYTE virtualAddress = (PBYTE)pars->_virtualAddress;
			if (!memory::HasVirtualAddress(virtualAddress, 1))
				return exitStatus::EXIT_STATUS_ADDRESS_NOT_AVAILABLE;
			UINTPTR_T addr = memory::g_level4PageMap->getPageTableEntry(
																		  memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 2),
																		  memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 1),
																		  memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 0),
																		  memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 0));
			if (utils::testBitInBitfield(addr, 9))
			{
				*virtualAddress = 0; // Allocate the page, as it would be a problem if give them the zero page and a device writes to it when the driver issues a write to that device.
				addr = memory::g_level4PageMap->getPageTableEntry(
					memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 2),
					memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 1),
					memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 0),
					memory::PageMap::computeIndexAtAddress((UINTPTR_T)virtualAddress, 0));
			}
			UINTPTR_T* phys = (UINTPTR_T*)pars->physicalAddress;
			*phys = addr;
			*phys &= 0xFFFFFFFFFF000;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
#endif
		static exitStatus RegisterRecursiveFileIterateCallback(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T driverId;
				alignas(STACK_SIZE) void(*callback)(void(*appendEntry)(CSTRING,SIZE_T));
			} *pars = (_par*)parameters;
			if (pars->driverId > g_registeredDriversCapacity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (!g_registeredDrivers[pars->driverId])
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			if (g_registeredDrivers[pars->driverId]->service_type != serviceType::SERVICE_TYPE_FILESYSTEM)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			g_registeredDrivers[pars->driverId]->iterateCallback = (PVOID)pars->callback;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}

		static void interruptHandler(const obos::interrupt_frame* frame)
		{
			if (g_registeredInterruptHandlers[frame->intNumber - 32].driver)
			{
				struct par
				{
					DWORD irq;
				} _par;
				_par.irq = frame->intNumber - 32;
				DisableIRQ(&_par);

#ifdef __x86_64__
				memory::PageMap* pageMap = memory::g_level4PageMap;
				memory::g_level4PageMap = g_registeredInterruptHandlers[frame->intNumber - 32].driver->process->level4PageMap;
				UINTPTR_T* _pageMap = memory::g_level4PageMap->getPageMap();
#define commitMemory , true
#elif defined(__i686__)
				memory::PageDirectory* pageDirectory = memory::g_pageDirectory;
				memory::g_pageDirectory = g_registeredInterruptHandlers[frame->intNumber - 32].driver->process->pageDirectory;
				UINTPTR_T* _pageMap = memory::g_pageDirectory->getPageMap();
#define commitMemory
#endif

				PBYTE newRsp = (PBYTE)memory::VirtualAlloc(nullptr, 1, memory::VirtualAllocFlags::GLOBAL | memory::VirtualAllocFlags::WRITE_ENABLED commitMemory);

				cli();

				EnterKernelSection();

				// Sets the new stack, 
				callDriverIrqHandler(newRsp + 4096, _pageMap, g_registeredInterruptHandlers[frame->intNumber - 32].handler);
				
				memory::VirtualFree(newRsp, 1);

#ifdef __x86_64__
				memory::g_level4PageMap = pageMap;
#elif defined(__i686__)
				memory::g_pageDirectory = pageDirectory;
#endif

#undef commitMemory

				LeaveKernelSection();
				PicSendEoi(&_par);
				EnableIRQ(&_par);
			}
		}
	}
}