/*
	driver_api/syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <driver_api/driver_interface.h>
#include <driver_api/syscalls.h>
#include <driver_api/interrupts.h>
#include <driver_api/enums.h>

#include <memory_manager/liballoc/liballoc.h>
#include <memory_manager/physical.h>
#include <memory_manager/paging/allocate.h>

#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

#include <console.h>
#include <klog.h>

#include <boot/boot.h>

#include <utils/memory.h>

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

#define CURRENT_DRIVER_IDENTITY reinterpret_cast<obos::driverAPI::driverIdentification*>(obos::multitasking::g_currentThread->owner->driverIdentity)

namespace obos
{
	namespace memory
	{
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress);
	}
	extern char kernelStart;
	namespace driverAPI
	{
		static exitStatus RegisterDriver();
		static exitStatus RegisterInterruptHandler(DEFINE_RESERVED_PARAMETERS);
		static exitStatus PicSendEoi(DEFINE_RESERVED_PARAMETERS);
		static exitStatus EnableIRQ(DEFINE_RESERVED_PARAMETERS);
		static exitStatus DisableIRQ(DEFINE_RESERVED_PARAMETERS);
		static exitStatus PrintChar(DEFINE_RESERVED_PARAMETERS);
		static exitStatus GetMultibootModule(DEFINE_RESERVED_PARAMETERS);
		static exitStatus MapPhysicalTo(DEFINE_RESERVED_PARAMETERS);
		static exitStatus UnmapPhysicalTo(DEFINE_RESERVED_PARAMETERS);
		static exitStatus Printf(CSTRING format, ...);
		static exitStatus GetPhysicalAddress(DEFINE_RESERVED_PARAMETERS);
		static exitStatus ListenForConnections(DEFINE_RESERVED_PARAMETERS);
		static exitStatus ConnectionSendData(DEFINE_RESERVED_PARAMETERS);
		static exitStatus ConnectionRecvData(DEFINE_RESERVED_PARAMETERS);
		static exitStatus ConnectionClose(DEFINE_RESERVED_PARAMETERS);
		static exitStatus HeapAllocate(DEFINE_RESERVED_PARAMETERS);
		static exitStatus HeapFree(DEFINE_RESERVED_PARAMETERS);
		
		static void interruptHandler(const obos::interrupt_frame* frame);

		/*struct driverIdentification
		{
			DWORD driverId = 0;
			serviceType m_service = serviceType::OBOS_SERVICE_TYPE_INVALID;
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
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(PrintChar));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(GetMultibootModule));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(MapPhysicalTo));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(UnmapPhysicalTo));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(Printf));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(GetPhysicalAddress));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(ListenForConnections));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(ConnectionSendData));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(ConnectionRecvData));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(ConnectionClose));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(HeapAllocate));
			RegisterSyscallHandler(currentSyscall++, GET_FUNC_ADDR(HeapFree));
		}

		static exitStatus RegisterDriver()
		{
			return exitStatus::EXIT_STATUS_NOT_IMPLEMENTED;
		}
		static exitStatus RegisterInterruptHandler(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				alignas(STACK_SIZE) UINTPTR_T interruptId;
				alignas(STACK_SIZE) void(*handler)();
			} attribute(aligned(8)) *pars = (_par*)parameters;
			if (pars->interruptId < 0x21)
				return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
			if (!multitasking::g_currentThread->owner->driverIdentity)
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			serviceType type = CURRENT_DRIVER_IDENTITY->service_type;
			switch (type)
			{
			case serviceType::OBOS_SERVICE_TYPE_INVALID:
				return exitStatus::EXIT_STATUS_NO_SUCH_DRIVER;
			case serviceType::OBOS_SERVICE_TYPE_STORAGE_DEVICE:
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
			case serviceType::OBOS_SERVICE_TYPE_USER_INPUT_DEVICE:
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
			case serviceType::OBOS_SERVICE_TYPE_GRAPHICS_DEVICE:
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
			case serviceType::OBOS_SERVICE_TYPE_MONITOR:
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
			case serviceType::OBOS_SERVICE_TYPE_KERNEL_EXTENSION:
				if(pars->interruptId >= 0x21 && pars->interruptId < 0x30)
					break;
				return exitStatus::EXIT_STATUS_ACCESS_DENIED; // No.
			case serviceType::OBOS_SERVICE_TYPE_COMMUNICATION:
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

			case serviceType::OBOS_SERVICE_TYPE_FILESYSTEM:
			case serviceType::OBOS_SERVICE_TYPE_VIRTUAL_STORAGE_DEVICE:
			case serviceType::OBOS_SERVICE_TYPE_VIRTUAL_USER_INPUT_DEVICE:
			case serviceType::OBOS_SERVICE_TYPE_VIRTUAL_COMMUNICATION:
				return exitStatus::EXIT_STATUS_ACCESS_DENIED;
			default:
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			}
			obos::RegisterInterruptHandler(pars->interruptId, interruptHandler);
			g_registeredInterruptHandlers[pars->interruptId - 0x20].handler = pars->handler;
			g_registeredInterruptHandlers[pars->interruptId - 0x20].driver = CURRENT_DRIVER_IDENTITY;
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
		static exitStatus ListenForConnections(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				HANDLE* newHandle;
			} *pars = (_par*)parameters;
			*pars->newHandle = reinterpret_cast<HANDLE>(Listen());
			return *pars->newHandle ? exitStatus::EXIT_STATUS_SUCCESS : exitStatus::EXIT_STATUS_CHECK_LAST_ERROR;
		}
		static exitStatus ConnectionSendData(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				DriverConnectionHandle* handle;
				PBYTE data;
				SIZE_T size;
				BOOL failIfMutexLocked;
			} *pars = (_par*)parameters;
			if (GET_FUNC_ADDR(pars->handle->GetDriverIdentification()) < 0xfffffffff0000000)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			return pars->handle->SendData(pars->data, pars->size, pars->failIfMutexLocked) ? exitStatus::EXIT_STATUS_SUCCESS : exitStatus::EXIT_STATUS_CHECK_LAST_ERROR;
		}
		static exitStatus ConnectionRecvData(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				DriverConnectionHandle* handle;
				PBYTE data;
				SIZE_T size;
				BOOL waitForData;
				BOOL peek;
				BOOL failIfMutexLocked;
			} *pars = (_par*)parameters;
			if (GET_FUNC_ADDR(pars->handle->GetDriverIdentification()) < 0xfffffffff0000000)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			return pars->handle->RecvData(pars->data, pars->size, pars->waitForData, pars->peek, pars->failIfMutexLocked) ? exitStatus::EXIT_STATUS_SUCCESS : exitStatus::EXIT_STATUS_CHECK_LAST_ERROR;
		}
		static exitStatus ConnectionClose(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				DriverConnectionHandle* handle;
			} *pars = (_par*)parameters;
			if (GET_FUNC_ADDR(pars->handle->GetDriverIdentification()) < 0xfffffffff0000000)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			bool ret = pars->handle->CloseConnection();
			delete pars->handle;
			return ret ? exitStatus::EXIT_STATUS_SUCCESS : exitStatus::EXIT_STATUS_CHECK_LAST_ERROR;
		}
		static exitStatus HeapAllocate(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				PVOID* ret;
				SIZE_T size;
			} *pars = (_par*)parameters;
			if(!pars->size)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			*pars->ret = new BYTE[pars->size];
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
		static exitStatus HeapFree(DEFINE_RESERVED_PARAMETERS)
		{
			struct _par
			{
				PBYTE block;
			} *pars = (_par*)parameters;
			if (pars->block < (PVOID)0xfffffffff0000000)
				return exitStatus::EXIT_STATUS_INVALID_PARAMETER;
			delete[] pars->block;
			return exitStatus::EXIT_STATUS_SUCCESS;
		}
			
		static void interruptHandler(const obos::interrupt_frame* frame)
		{
			if (!(Pic((frame->intNumber - 32) < 8 ? Pic::PIC1_CMD : Pic::PIC2_CMD, (frame->intNumber - 32) < 8 ? Pic::PIC1_DATA : Pic::PIC2_DATA).issuedInterrupt(frame->intNumber - 32)))
				return;
			if (g_registeredInterruptHandlers[frame->intNumber - 32].driver)
			{
#ifdef __x86_64__
				memory::PageMap* pageMap = memory::g_level4PageMap;
				memory::g_level4PageMap = g_registeredInterruptHandlers[frame->intNumber - 32].driver->process->level4PageMap;
				UINTPTR_T* _pageMap = memory::g_level4PageMap->getPageMap();
#define commitMemory , true
#elif defined(__i686__)
				memory::PageDirectory* pageDirectory = memory::g_pageDirectory;
				memory::g_pageDirectory = g_registeredInterruptHandlers[frame->intNumber - 32].driver->process->pageDirectory;
				UINTPTR_T* _pageMap = memory::g_pageDirectory->getPageDirectory();
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
				obos::SendEOI(frame->intNumber - 32);
			}
		}
	}
}