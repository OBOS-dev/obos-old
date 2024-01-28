/*
	drivers/generic/acpi/main.cpp

	Copyright (c) 2023-2024 Omar Berrow		
*/

#include <int.h>
#include <klog.h>
#include <export.h>
#include <limine.h>

#include <driverInterface/struct.h>

#include <uacpi/internal/tables.h>
#include <uacpi/uacpi.h>

#include <x86_64-utils/asm.h>
#include <arch/x86_64/memory_manager/virtual/initialize.h>

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

extern "C" uacpi_status
uacpi_table_find(struct uacpi_table_identifiers* id,
	struct uacpi_table** out_table);

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
			{ UACPI_LOG_TRACE, 0 }
		};
		uacpi_status st = uacpi_initialize(&params);
		verify_status(st);

		st = uacpi_namespace_load();
		verify_status(st);

		st = uacpi_namespace_initialize();
		verify_status(st);

		uacpi_object* ret = nullptr;
		uacpi_eval(nullptr, "_S5_", nullptr, &ret);
		// grab the first and the second entry of the package

		auto* obj = uacpi_create_object(UACPI_OBJECT_INTEGER);
		obj->integer = 5;
		auto args = uacpi_args{ &obj, 1 };
		uacpi_eval(nullptr, "_PTS", &args, nullptr);
		verify_status(st);

		uacpi_table* t = nullptr;
		uacpi_table_identifiers id{};
		id.signature.text[0] = 'F';
		id.signature.text[1] = 'A';
		id.signature.text[2] = 'C';
		id.signature.text[3] = 'P';
		uacpi_table_find(&id, &t);
		auto fadt = (acpi_fadt*)memory::mapPageTable((uintptr_t*)t->phys_addr);
		outw(fadt->pm1a_cnt_blk, (ret->package->objects[0]->integer << 10) | ((uint64_t)1 << 13));
		if (fadt->pm1b_cnt_blk)
			outw(fadt->pm1b_cnt_blk, (ret->package->objects[1]->integer << 10) | ((uint64_t)1 << 13));
	}
	done:
	thread::ExitThread(0);
}