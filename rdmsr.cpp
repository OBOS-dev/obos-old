#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

extern "C" long __stdcall ReadMsr(
	unsigned long Msr,
	unsigned long long* Value
);

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: %s address", argv[0]);
		return 1;
	}
	uint32_t msr = atoi(argv[1]);
	uint64_t msrValue = 0;
	ReadMsr(msr, &msrValue);
	printf("0x%x", msr);
	return 0;
}
