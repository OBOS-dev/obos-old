/*
	drivers/generic/test/main.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <driverInterface/struct.h>

obos::driverInterface::driverHeader __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME))) header = {
	.magicNumber = obos::driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 0,
	.driverType = obos::driverInterface::OBOS_SERVICE_TYPE_FILESYSTEM,
	.stackSize = 0x4000,
};

using namespace obos;

extern "C" uintptr_t Syscall(uintptr_t call, void* callPars);

static char* itoa(intptr_t value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	intptr_t tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

extern "C" void _start()
{
	driverInterface::DriverServer* conn = (driverInterface::DriverServer*)Syscall(2, nullptr);
	conn->Listen();
	uint32_t i = 69;
	conn->SendData((byte*)&i, sizeof(i));
	bool ret = conn->RecvData((byte*)&i, sizeof(i));
	const char* str = "Received ";

	if (!ret)
	{
		str = "Receive error!\n";
		Syscall(0, &str);
		conn->CloseConnection();
		goto finish;
	}

	{
		char strInteger[32] = {};
		itoa(i, strInteger, 10);
		str = "Received ";
		Syscall(0, &str);
		str = (char*)&strInteger;
		Syscall(0, &str);
		str = "\n";
		Syscall(0, &str);
		conn->CloseConnection();
	}

finish:
	uint32_t exitCode = 0;
	Syscall(1, &exitCode); // ExitThread
}

asm("Syscall: mov %rdi, %rax; mov %rsi, %rdi; int $0x31; ret");