/*
    oboskrnl/arch/x86_64/syscall/power_management.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <arch/x86_64/syscall/power_management.h>

#include <multitasking/arch.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <x86_64-utils/asm.h>

extern "C"
{
#include <uacpi/uacpi.h>
#include <uacpi/internal/tables.h>
#include <uacpi/kernel_api.h>
    void reset_idt();
}

namespace obos
{
    void EarlyKPanic();
	bool EnterSleepState(int sleepState);
    namespace syscalls
    {
        uintptr_t PMSyscallHandler(uint64_t syscall, void*)
        {
            switch (syscall)
            {
            case 59:
                SyscallShutdown();
                break;
            case 60:
                SyscallReboot();
                break;
            default:
                break;
            }
            return 0;
        }

        void SyscallShutdown()
        {
            EnterSleepState(5);
            thread::StopCPUs(true);
        }
        void SyscallReboot()
        {
            //EarlyKPanic();
            thread::stopTimer();
            thread::StopCPUs(false);
            uacpi_table* t1 = nullptr;
            uacpi_status st = uacpi_table_find_by_type(UACPI_TABLE_TYPE_FADT, &t1);
            acpi_fadt* fadt = (acpi_fadt*)t1->virt_addr;
            acpi_gas &gas = fadt->reset_reg;
            void(*write_callback)(uint64_t address, uint8_t val, uint8_t wordSize) = nullptr;
            static auto write_address_callback = [](uint64_t address, uint8_t val, uint8_t wordSize) {
                switch (wordSize)
                {
                case 1:
                {
                    uint8_t* v = (uint8_t*)memory::mapPageTable((uintptr_t*)address);
                    *v = val;
                    break;
                }
                case 2:
                {
                    uint16_t* v = (uint16_t*)memory::mapPageTable((uintptr_t*)address);
                    *v = val;
                    break;
                }
                case 3:
                {
                    uint32_t* v = (uint32_t*)memory::mapPageTable((uintptr_t*)address);
                    *v = val;
                    break;
                }
                case 4:
                {
                    uint64_t* v = (uint64_t*)memory::mapPageTable((uintptr_t*)address);
                    *v = val;
                    break;
                }
                default:
                    break;
                }
                };;
            if (st != UACPI_STATUS_OK)
                goto acpi_reboot_failure; // Abort.
            switch (gas.address_space_id)
            {
            case 0:
            {
                write_callback = write_address_callback;
                break;
            }
            case 1:
            {
                write_callback = [](uint64_t address, uint8_t val, uint8_t wordSize) {
                    switch (wordSize)
                    {
                    case 1:
                    {
                        outb(address, val);
                        break;
                    }
                    case 2:
                    {
                        outw(address, val);
                        break;
                    }
                    case 3:
                    {
                        outd(address, val);
                        break;
                    }
                    case 4:
                    {
                        outd(address, val);
                        outd(address + 4, val);
                        break;
                    }
                    default:
                        break;
                    }
                    };
                break;
            }
            case 2:
            {
                write_callback = [](uint64_t address, uint8_t val, uint8_t wordSize) {
                    uacpi_pci_address addr{};
                    uint16_t offset = 0;
                    addr.bus = 0;
                    addr.segment = 0;
                    addr.device = (address >> 32) & 0xffff;
                    addr.function = (address >> 16) & 0xffff;
                    offset = address & 0xffff;
                    switch (wordSize)
                    {
                    case 1:
                    {
                        uacpi_kernel_pci_write(&addr, offset, 1, val);
                        break;
                    }
                    case 2:
                    {
                        uacpi_kernel_pci_write(&addr, offset, 2, val);
                        break;
                    }
                    case 3:
                    {
                        uacpi_kernel_pci_write(&addr, offset, 4, val);
                        break;
                    }
                    case 4:
                    {
                        uacpi_kernel_pci_write(&addr, offset, 8, val);
                        break;
                    }
                    default:
                        break;
                    }
                    };
                break;
            }
            case 6:
            {
                write_callback = [](uint64_t address, uint8_t val, uint8_t wordSize) {
                    uacpi_pci_address addr{};
                    addr.bus = (address >> 48) & 0xff;
                    addr.segment = (address >> 56) & 0xffff;
                    addr.device = (address >> 43) & 0xf;
                    addr.function = (address >> 40) & 0x7;
                    uint64_t barOffset = address & 0xF'FFFF'FFFF;
                    uint8_t barIndex = (address >> 36) & 0x7;
                    uint64_t bar = 0;
                    uacpi_kernel_pci_read(&addr, (uacpi_size)barIndex * 4 + 0x10, 4, &bar);
                    bar += barOffset;
                    write_address_callback(bar, val, wordSize);
                    };
                break;
            }
            default:
                break;
            }
            if (write_callback)
                write_callback(gas.address, fadt->reset_value, gas.access_size);
        acpi_reboot_failure:

            EarlyKPanic();
            thread::StopCPUs(true);
            while (1);
        }
    } // namespace syscall
    
} // namespace obos
