/*
	drivers/generic/acpi/main.cpp

	Copyright (c) 2023-2024 Omar Berrow		
*/

#include <int.h>
#include <klog.h>
#include <export.h>
#include <limine.h>

#include <multitasking/thread.h>

#ifdef __x86_64__
#include <x86_64-utils/asm.h>
#endif

extern "C"
{
	#include <uacpi/uacpi.h>

	#include <uacpi/internal/tables.h>

	#include <uacpi/kernel_api.h>
	#include <uacpi/sleep.h>
}

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

#define verify_status(st) \
if (st != UACPI_STATUS_OK)\
	obos::logger::panic(nullptr, "uACPI Failed! Status code: %d, error message: %s\n", st, uacpi_status_to_string(st));

namespace obos
{
	void InitializeUACPI()
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
	}
	bool EnterSleepState(int sleepState)
	{
		if (sleepState < 0 || sleepState > 5)
			return false;
		uacpi_status ret;

		ret = uacpi_prepare_for_sleep_state((uacpi_sleep_state)sleepState);
		if (uacpi_unlikely_error(ret))
			return false;

#ifdef __x86_64__
		cli();
#endif
		ret = uacpi_enter_sleep_state((uacpi_sleep_state)sleepState);
#ifdef __x86_64__
		sti();
#endif
		return ret == UACPI_STATUS_OK;
		// Keep this code as a souvenir of when uACPI didn't have sleep helpers when I (Omar Berrow) had implemented it.
		/*char objectName[5] = {};
		logger::sprintf(objectName, "_S%d_", sleepState);
		uacpi_object* ret = nullptr;
		uacpi_status st = uacpi_eval(nullptr, objectName, nullptr, &ret);
		if (st != UACPI_STATUS_OK)
			return false;

		auto* obj = uacpi_create_object(UACPI_OBJECT_INTEGER);
		obj->integer = sleepState;
		uacpi_args args{ &obj, 1 };
		st = uacpi_eval(nullptr, "_PTS", &args, nullptr);
		if (st != UACPI_STATUS_OK && st != UACPI_STATUS_NOT_FOUND)
		{
			uacpi_object_unref(obj);
			return false;
		}

		uacpi_table* t = nullptr;
		st = uacpi_table_find_by_type(UACPI_TABLE_TYPE_FADT, &t);
		if (st != UACPI_STATUS_OK)
		{
			uacpi_object_unref(obj);
			return false;
		}
		acpi_fadt* fadt = (acpi_fadt*)t->hdr;
		uacpi_handle pm1a_cnt_blk_hnd{}, pm1b_cnt_blk_hnd{};
		if (uacpi_kernel_io_map(fadt->pm1a_cnt_blk, 2, &pm1a_cnt_blk_hnd) != UACPI_STATUS_OK)
			return false;
		if (fadt->pm1b_cnt_blk)
		{
			if (uacpi_kernel_io_map(fadt->pm1b_cnt_blk, 2, &pm1b_cnt_blk_hnd) != UACPI_STATUS_OK)
			{
				uacpi_kernel_io_unmap(pm1b_cnt_blk_hnd);
				return false;
			}
		}
		uacpi_kernel_io_write(pm1a_cnt_blk_hnd, 0, 2, (ret->package->objects[0]->integer << 10) | ((uint64_t)1 << 13));
		if (fadt->pm1b_cnt_blk)
			uacpi_kernel_io_write(pm1b_cnt_blk_hnd, 0, 2, (ret->package->objects[1]->integer << 10) | ((uint64_t)1 << 13));
		uacpi_object_unref(obj);
		uacpi_kernel_io_unmap(pm1a_cnt_blk_hnd);
		if (fadt->pm1b_cnt_blk)
			uacpi_kernel_io_unmap(pm1b_cnt_blk_hnd);
		return true;*/
	}

}