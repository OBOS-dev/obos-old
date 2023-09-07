#include <iostream>

#include <cstdint>

static uint16_t computeIndexAtAddress(uintptr_t address, uint8_t level) { return (address >> (9 * level + 12)) & 0x1FF; }

int main()
{
    uintptr_t addr = 0;
    for (; !computeIndexAtAddress(addr, 2); addr += 4096);
    std::cout << std::hex << "0x" << addr << ": 0x" << computeIndexAtAddress(addr, 0) << '\n';
}