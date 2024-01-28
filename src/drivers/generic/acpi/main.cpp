/*
	drivers/generic/acpi/main.cpp

	Copyright (c) 2023-2024 Omar Berrow		
*/

#include <int.h>
#include <klog.h>
#include <export.h>
#include <limine.h>

#include <driverInterface/struct.h>

#include <uacpi/uacpi.h>

using namespace obos;

#ifdef __GNUC__
#define DEFINE_IN_SECTION __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME)))
#else
#define DEFINE_IN_SECTION
#endif

namespace obos
{
	extern OBOS_EXPORT volatile limine_rsdp_request rsdp_request;
	extern OBOS_EXPORT volatile limine_hhdm_request hhdm_offset;
}

driverInterface::driverHeader DEFINE_IN_SECTION g_driverHeader = {
	.magicNumber = obos::driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 6,
	.driverType = obos::driverInterface::OBOS_SERVICE_TYPE_KERNEL_EXTENSION,
	.requests = driverInterface::driverHeader::REQUEST_SET_STACK_SIZE,
	.stackSize = 0x10000,
	.functionTable = {
		.GetServiceType = []()->driverInterface::serviceType { return driverInterface::serviceType::OBOS_SERVICE_TYPE_KERNEL_EXTENSION; },
	}
};

#define verify_status(st) \
if (st != UACPI_STATUS_OK)\
{\
	obos::logger::error("UACPI Failed! Status code: %d, error message: %s\n", st, uacpi_status_to_string(st));\
	goto done;\
}

extern "C" void _start()
{
	g_driverHeader.driver_initialized = true;
	while (!g_driverHeader.driver_finished_loading)
		;

	{
		uacpi_init_params params = {
			(uintptr_t)rsdp_request.response->address - hhdm_offset.response->offset,
			{ UACPI_LOG_INFO, 0 }
		};
		uacpi_status st = uacpi_initialize(&params);
		verify_status(st);

		st = uacpi_namespace_load();
		verify_status(st);

		st = uacpi_namespace_initialize();
		verify_status(st);

		uacpi_object* ret = nullptr;
		st = uacpi_eval(nullptr, "\\MAIN", nullptr, &ret);
		verify_status(st);
	}
	done:
	thread::ExitThread(0);
}