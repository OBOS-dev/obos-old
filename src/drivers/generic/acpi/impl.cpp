/*
	drivers/generic/acpi/impl.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <export.h>

#include <multitasking/scheduler.h>
#include <multitasking/cpu_local.h>

#include <multitasking/locks/mutex.h>

#if defined(__x86_64__) || defined(_WIN64)
#include <driverInterface/x86_64/enumerate_pci.h>
#include <arch/x86_64/irq/irq.h>
#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <x86_64-utils/asm.h>
#endif

#include <allocators/liballoc.h>

#include <allocators/vmm/vmm.h>
#include <allocators/vmm/arch.h>

#include <uacpi/kernel_api.h>

extern "C" uacpi_status
uacpi_table_find(struct uacpi_table_identifiers* id,
	struct uacpi_table** out_table);

using namespace obos;

namespace obos
{
#if defined(__x86_64__) || defined(_WIN64)
	namespace thread
	{
		OBOS_EXPORT uint64_t configureHPET(uint64_t freq);
	}
	namespace memory
	{
		OBOS_EXPORT void* MapPhysicalAddress(PageMap* pageMap, uintptr_t phys, void* to, uintptr_t cpuFlags);
		OBOS_EXPORT void UnmapAddress(PageMap* pageMap, void* _addr);
		OBOS_EXPORT uintptr_t DecodeProtectionFlags(uintptr_t _flags);
	}
#endif
}

extern "C" {
	uacpi_status uacpi_kernel_pci_read(
		uacpi_pci_address* address, uacpi_size offset,
		uacpi_u8 byte_width, uacpi_u64* value
	)
	{
#if defined(__x86_64__) || defined(_WIN64)
		if (address->segment)
			return UACPI_STATUS_UNIMPLEMENTED;
		switch (byte_width)
		{
		case 1:
			*value = driverInterface::pciReadByteRegister(address->bus, address->device, address->function, offset);
			return UACPI_STATUS_OK;
		case 2:
			*value = driverInterface::pciReadWordRegister(address->bus, address->device, address->function, offset);
			return UACPI_STATUS_OK;
		case 4:
			*value = driverInterface::pciReadDwordRegister(address->bus, address->device, address->function, offset);
			return UACPI_STATUS_OK;
		case 8:
			*value = driverInterface::pciReadDwordRegister(address->bus, address->device, address->function, offset) | 
				driverInterface::pciReadDwordRegister(address->bus, address->device, address->function, offset + 4);
			return UACPI_STATUS_OK;
		default:
			break;
		}
		return UACPI_STATUS_INVALID_ARGUMENT;
#else
		return UACPI_STATUS_UNIMPLEMENTED;
#endif
	}
	uacpi_status uacpi_kernel_pci_write(
		uacpi_pci_address* address, uacpi_size offset,
		uacpi_u8 byte_width, uacpi_u64 value
	)
	{
#if defined(__x86_64__) || defined(_WIN64)
		if (address->segment)
			return UACPI_STATUS_UNIMPLEMENTED;
		switch (byte_width)
		{
		case 1:
			driverInterface::pciWriteByteRegister(address->bus, address->device, address->function, offset, value & 0xff);
			return UACPI_STATUS_OK;
		case 2:
			driverInterface::pciWriteWordRegister(address->bus, address->device, address->function, offset, value & 0xffff);
			return UACPI_STATUS_OK;
		case 4:
			driverInterface::pciWriteDwordRegister(address->bus, address->device, address->function, offset, value & 0xffff'ffff);
			return UACPI_STATUS_OK;
		case 8:
			driverInterface::pciWriteDwordRegister(address->bus, address->device, address->function, offset, value & 0xffff'ffff);
			driverInterface::pciWriteDwordRegister(address->bus, address->device, address->function, offset + 4, value >> 32);
			return UACPI_STATUS_OK;
		default:
			break;
		}
		return UACPI_STATUS_INVALID_ARGUMENT;
#else
		return UACPI_STATUS_UNIMPLEMENTED;
#endif
	}
	void* uacpi_kernel_alloc(uacpi_size size)
	{
		return kmalloc(size);
	}
	void* uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
	{
		return kcalloc(count, size);
	}
	void uacpi_kernel_free(void* mem)
	{
		if (!mem)
			return;
		kfree(mem);
	}
	void uacpi_kernel_log(enum uacpi_log_level level, const char* format, ...)
	{
		va_list list;
		va_start(list, format);
		uacpi_kernel_vlog(level, format, list);
		va_end(list);
	}
	void uacpi_kernel_vlog(enum uacpi_log_level level, const char* format, uacpi_va_list list)
	{
		const char* prefix = "UNKNOWN";
		switch (level)
		{
		case UACPI_LOG_TRACE:
			prefix = "TRACE";
			break;
		case UACPI_LOG_INFO:
			prefix = "INFO";
			break;
		case UACPI_LOG_WARN:
			prefix = "WARN";
			break;
		case UACPI_LOG_ERROR:
			prefix = "ERROR";
			break;
		case UACPI_LOG_FATAL:
			prefix = "FATAL";
			break;
		default:
			break;
		}
		logger::printf("uACPI, %s: ", prefix);
		logger::vprintf(format, list);
	}
	uacpi_u64 uacpi_kernel_get_ticks(void)
	{
		return 10000 /* 1ms is 1000000ns, so we divide that to get 10000 ticks. */;
	}
#if defined(__x86_64__) || defined(_WIN64)
#pragma GCC push_options
#pragma GCC target("sse")
	void uacpi_kernel_stall(uacpi_u8 usec)
	{
		byte fpuState[512] = {};
		asm volatile("fxsave (%0)" : : "r"(fpuState):);
		for (uint64_t comparatorValue = thread::configureHPET(1/(usec/1000000.f)); g_HPETAddr->mainCounterValue < comparatorValue;)
			pause();
		asm volatile("fxrstor (%0)" : : "r"(fpuState):);
	}
#pragma GCC pop_options
#else
#error Implement uacpi_kernel_stall for the current architecture!
#endif
	void uacpi_kernel_sleep(uacpi_u64 msec)
	{
		auto currentThread = thread::GetCurrentCpuLocalPtr()->currentThread;
		currentThread->blockCallback.callback = [](thread::Thread*, void* userdata)
			{
				size_t wakeTime = (size_t)userdata;
				return thread::g_timerTicks >= wakeTime;
			};
		currentThread->blockCallback.userdata = (void*)(thread::g_timerTicks + msec);
		currentThread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_BLOCKED;
		thread::callScheduler(false);
	}
	void* uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size)
	{
		/*const size_t pageSize = memory::VirtualAllocator::GetPageSize();
		const size_t nPages = size / pageSize + ((size % pageSize) != 0);
		void* ret = memory::_Impl_FindUsableAddress(nullptr, nPages);
		auto _addr = addr - (addr % pageSize);
#if defined(__x86_64__) || defined(_WIN64)
		for (size_t i = 0; i < nPages; i++)
			memory::MapPhysicalAddress(memory::getCurrentPageMap(), _addr + i * 4096, (byte*)ret + i * 4096, memory::DecodeProtectionFlags(memory::PROT_IS_PRESENT | memory::PROT_DISABLE_CACHE));
#endif
		return (byte*)ret + addr % pageSize;*/
		return memory::mapPageTable((uintptr_t*)addr);
	}
	// Does nothing, as we don't need to un-map from the hhdm.
	void uacpi_kernel_unmap(void*, uacpi_size)
	{
	}
	uacpi_handle uacpi_kernel_create_mutex(void)
	{
		return new locks::Mutex{ false };
	}
	void uacpi_kernel_free_mutex(uacpi_handle m)
	{
		delete (locks::Mutex*)m;
	}

	uacpi_bool uacpi_kernel_acquire_mutex(uacpi_handle _m, uacpi_u16 t)
	{
		locks::Mutex* m = (locks::Mutex*)_m;
		return m->Lock(t == 0xffff ? 0 : t, true);
	}
	void uacpi_kernel_release_mutex(uacpi_handle _m)
	{
		locks::Mutex* m = (locks::Mutex*)_m;
		m->Unlock();
	}

	uacpi_handle uacpi_kernel_create_event(void)
	{
		return new size_t{};
	}
	void uacpi_kernel_free_event(uacpi_handle e)
	{
		delete (size_t*)e;
	}

	uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle _e, uacpi_u16 t)
	{
		size_t* e = (size_t*)_e;
		if (t == 0xffff)
		{
			while (*e > 0);
			return UACPI_TRUE;
		}
		uint64_t wakeTime = thread::g_timerTicks + t;
		while (*e > 0 && thread::g_timerTicks >= wakeTime);
		bool ret = *e > 0;
		*e -= ret;
		return ret;
	}
	void uacpi_kernel_signal_event(uacpi_handle _e)
	{
		size_t* e = (size_t*)_e;
		__atomic_fetch_add(e, 1, __ATOMIC_SEQ_CST);
	}
	void uacpi_kernel_reset_event(uacpi_handle _e)
	{
		size_t* e = (size_t*)_e;
		__atomic_store_n(e, 0, __ATOMIC_SEQ_CST);
	}

	struct io_region
	{
		uacpi_io_addr base;
		uacpi_size len;
	};

	uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle* out_handle)
	{
		io_region* hnd = new io_region{};
		if (!hnd)
			return UACPI_STATUS_OUT_OF_MEMORY;
		hnd->base = base;
		hnd->len = len;
		*out_handle = hnd;
		return UACPI_STATUS_OK;
	}
	void uacpi_kernel_io_unmap(uacpi_handle handle)
	{
		io_region* hnd = (io_region*)handle;
		delete hnd;
	}

	uacpi_status uacpi_kernel_io_read(
		uacpi_handle _hnd, uacpi_size offset,
		uacpi_u8 byte_width, uacpi_u64* value
	)
	{
		io_region* hnd = (io_region*)_hnd;
		uint16_t port = hnd->base + offset;
		switch (byte_width)
		{
		case 1:
			*value = inb(port);
			return UACPI_STATUS_OK;
		case 2:
			*value = inw(port);
			return UACPI_STATUS_OK;
		case 4:
			*value = ind(port);
			return UACPI_STATUS_OK;
		case 8:
			*value = ind(port) | ind(port + 4);
			return UACPI_STATUS_OK;
		default:
			break;
		}
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	uacpi_status uacpi_kernel_io_write(
		uacpi_handle _hnd, uacpi_size offset,
		uacpi_u8 byte_width, uacpi_u64 value
	)
	{
		io_region* hnd = (io_region*)_hnd;
		uint16_t port = hnd->base + offset;
		switch (byte_width)
		{
		case 1:
			outb(port, value);
			return UACPI_STATUS_OK;
		case 2:
			outw(port, value);
			return UACPI_STATUS_OK;
		case 4:
			outd(port, value);
			return UACPI_STATUS_OK;
		case 8:
			outd(port, value);
			outd(port + 4, value >> 32);
			return UACPI_STATUS_OK;
		default:
			break;
		}
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request* req)
	{
		switch (req->type)
		{
		case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
			break;
		case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
			logger::panic(nullptr,
				"Your bios fucked up, so now you have to deal with the consequences, also known as possible data loss. Firmware Error Code: 0x%016x, argument: %x016x\n",
				req->fatal.code, req->fatal.arg);
			break;
		default:
			break;
		}
	}
}