#pragma once

#define UACPI_ARCH_FLUSH_CPU_CACHE() asm volatile("wbinvd")