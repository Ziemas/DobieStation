#include "dev9.hpp"
#include <cstdio>

DEV9::DEV9(IOP_INTC* intc) : intc(intc) {}

void DEV9::reset()
{
    eeprom = {};
    irq_mask = 0;
    irq_stat = 0;
}

uint8_t DEV9::read8(uint32_t address)
{
    if (address >= SPD_REGBASE && address < SMAP_REGBASE)
    {
        switch (address)
        {
            case SPD_REG(SPD_R_PIO_DIR):
                printf("[DEV9] [SPD] Read8 PIO_DIR: %02x\n ", eeprom.get_dir());
                return eeprom.get_dir();

            // Writing to PIO_DATA apparently also toggles the LED on the network adapter?
            case SPD_REG(SPD_R_PIO_DATA):
                return eeprom.read();
        }

        printf("[DEV9] [SPD] Unrecognized read8 from %08x\n", address);
        return 0;
    }
    if (address >= SMAP_REGBASE && address < FLASH_REGBASE)
    {
        printf("[DEV9] [SMAP] unrecognized read8 from %08x\n", address);
        return 0;
    }

    printf("Unrecognized DEV9 read8 from $%08x\n", address);
    return 0;
}

uint16_t DEV9::read16(uint32_t address)
{
    if (address >= SPD_REGBASE && address < SMAP_REGBASE)
    {
        switch (address)
        {
            case SPD_REG(SPD_R_REV_1):
                printf("[DEV9] [SPD] Read speed chip version\n");
                return SPEED_CHIP_VER;

            case SPD_REG(SPD_R_REV_3):
                printf("[DEV9] [SPD] Read caps\n");
                return spd_caps;

            case SPD_REG(SPD_R_INTR_STAT):
                printf("[DEV9] [SPD] Read INTR_STAT: %04x\n", irq_stat);
                return irq_stat;

            case SPD_REG(SPD_R_INTR_MASK):
                printf("[DEV9] [SPD] Read INTR_MASK: %04x\n", irq_mask);
                return irq_mask;
        }

        printf("[DEV9] [SPD] Unrecognized read16 from %08x\n", address);
        return 0;
    }

    if (address >= SMAP_REGBASE && address < FLASH_REGBASE)
    {
        printf("[DEV9] [SMAP] Unrecognized read 16 from %08x\n", address);
        // SMAP
        return 0;
    }

    switch (address)
    {
        case DEV9_R_REV:
            printf("[DEV9] Read DEV9 type\n");
            if (connected)
                return (uint16_t)DEV9_TYPE::EXPBAY;
            return 0;

        case DEV9_R_POWER:
            printf("[DEV9] Read DEV9 R_POWER\n");
            return power;

    }

    printf("[DEV9] Unrecognized read16 from $%08x\n", address);
    return 0;
}

uint32_t DEV9::read32(uint32_t address)
{
    if (address >= SPD_REGBASE && address < SMAP_REGBASE)
    {
        printf("[DEV9] [SPD] Unrecognized read32 from %08x\n", address);
        return 0;
    }
    if (address >= SMAP_REGBASE && address < FLASH_REGBASE)
    {
        printf("[DEV9] [SMAP] Read32 unrecognized SMAP reg %08x\n", address);
        return 0;
    }

    printf("[DEV9] Unrecognized read32 from $%08x\n", address);
    return 0;
}

void DEV9::write8(uint32_t address, uint8_t value)
{
    if (address >= SPD_REGBASE && address < SMAP_REGBASE)
    {
        switch (address)
        {
            case SPD_REG(SPD_R_PIO_DIR):
                printf("[DEV9] [SPD] Write8 PIO_DIR: %02x\n", value);
                eeprom.set_dir(value);
                return;

            case SPD_REG(SPD_R_PIO_DATA):
                eeprom.write(value);
                return;
        }

        printf("[DEV9] [SPD] Unrecognized write8 to %08x of %02x\n", address, value);
        return;
    }

    if (address >= SMAP_REGBASE && address < FLASH_REGBASE)
    {
        printf("[DEV9] [SMAP] Unrecognized write8 to $%08x of %02x\n", address, value);
        return;
    }


    printf("[DEV0] Unrecognized DEV9 write8 to $%08x of $%02x\n", address, value);
}

void DEV9::write16(uint32_t address, uint16_t value)
{
    if (address >= SPD_REGBASE && address < SMAP_REGBASE)
    {
        switch (address)
        {
            case SPD_REG(SPD_R_INTR_MASK):
                printf("[DEV9] [SPD] Write16 INTR_MASK: %04x\n", value);
                irq_mask = value;
                return;
        }

        printf("[DEV9] [SPD] Unrecognized write16 to %08x of %04x\n", address, value);
        return;
    }

    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
        printf("[DEV9] [SMAP] Unrecognized SMAP write16 to $%08x of %04x\n", address, value);
        return;
    }

    printf("[DEV9] Unrecognized write16 to $%08x of $%02x\n", address, value);
}

void DEV9::write32(uint32_t address, uint32_t value)
{
    if (address >= SPD_REGBASE && address < SMAP_REGBASE)
    {
        printf("[DEV9] [SPD] Unrecognized write32 to %08x of %08x\n", address, value);
        return;
    }
    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
        printf("[DEV9] [SMAP] Unrecognized SMAP write32 to $%08x of %08x\n", address, value);
        return;
    }
    printf("[DEV9] Unrecognized DEV9 write32 to $%08x of $%08x\n", address, value);
}
