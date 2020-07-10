#include "dev9.hpp"
#include <cstdio>

uint8_t DEV9::read8(uint32_t address)
{
    printf("Unrecognized DEV9 read8 from $%08x\n", address);
    return 0;
}

uint16_t DEV9::read16(uint32_t address)
{
    switch (address)
    {
        case 0x1F80146E:
            return DEV9_IDENT;
    }
    printf("Unrecognized DEV9 read16 from $%08x\n", address);
    return 0;
}

uint32_t DEV9::read32(uint32_t address)
{
    printf("Unrecognized DEV9 read32 from $%08x\n", address);
    return 0;
}

void DEV9::write8(uint32_t address, uint8_t value)
{
    printf("Unrecognized DEV9 write8 to $%08x of $%01x\n", address, value);
}

void DEV9::write16(uint32_t address, uint16_t value)
{
    printf("Unrecognized DEV9 write16 to $%08x of $%02x\n", address, value);
}

void DEV9::write32(uint32_t address, uint32_t value)
{
    printf("Unrecognized DEV9 write32 to $%08x of $%04x\n", address, value);
}
