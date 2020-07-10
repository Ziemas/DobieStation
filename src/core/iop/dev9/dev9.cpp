#include "dev9.hpp"
#include <cstdio>

uint8_t DEV9::read8(uint32_t address)
{
    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
      //printf("[DEV9] Read16 unrecognized SMAP reg\n");
      // SMAP
      return 0;
    }

    switch (address)
    {
        case SPD_REG(SPD_R_PIO_DIR):
        {
            printf("[DEV9] Read8 PIO_DIR: %04x\n", 0);
            return 0;
        }
        case SPD_REG(SPD_R_PIO_DATA):
        {
            printf("[DEV9] Read8 PIO_DATA: %04x\n", 0);
            return 0;
        }
    }
    printf("Unrecognized DEV9 read8 from $%08x\n", address);
    return 0;
}

uint16_t DEV9::read16(uint32_t address)
{
    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
      //printf("[DEV9] Read16 unrecognized SMAP reg\n");
      // SMAP
      return 0;
    }
    switch (address)
    {
        case 0x1F80146E:
        {
            printf("[DEV9] Read ident magic\n");
            return DEV9_IDENT; // magic number
        }
        case SPD_REG(SPD_R_REV_1):
        {
            printf("[DEV9] Read magic number the second coming\n");
            return 0x0011; // magic number
        }
        case SPD_REG(SPD_R_REV_3):
        {
            printf("[DEV9] Read caps\n");
            return SPD_CAPS_SMAP; // lets lie a litle
        }
        case SPD_REG(SPD_R_INTR_STAT):
        {
            printf("[DEV9] Read INTR_STAT: %04x\n", irq_stat);
            return irq_stat;
        }
        case SPD_REG(SPD_R_INTR_MASK):
        {
            printf("[DEV9] Read INTR_MASK: %04x\n", irq_mask);
            return irq_mask;
        }
    }

    printf("[DEV9] Unrecognized read16 from $%08x\n", address);
    return 0;
}

uint32_t DEV9::read32(uint32_t address)
{
    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
      //printf("[DEV9] Read16 unrecognized SMAP reg\n");
      // SMAP
      return 0;
    }

    printf("[DEV9] Unrecognized read32 from $%08x\n", address);
    return 0;
}

void DEV9::write8(uint32_t address, uint8_t value)
{
    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
      printf("[DEV9] Unrecognized SMAP write8 to $%04x of %01x\n", address, value);
      // SMAP
      return;
    }

    switch (address)
    {
        case SPD_REG(SPD_R_PIO_DIR):
        {
            printf("[DEV9] Write8 PIO_DIR: %04x\n", value);
            return;
        }
        case SPD_REG(SPD_R_PIO_DATA):
        {
            printf("[DEV9] Write8 PIO_DATA: %04x\n", value);
            return;
        }
    }
    printf("[DEV0] Unrecognized DEV9 write8 to $%08x of $%01x\n", address, value);
}

void DEV9::write16(uint32_t address, uint16_t value)
{
    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
      printf("[DEV9] Unrecognized SMAP write16 to $%04x of %02x\n", address, value);
      // SMAP
      return;
    }

    switch (address)
    {
        case SPD_REG(SPD_R_INTR_MASK):
        {
            printf("[DEV9] Write16 INTR_MASK: %04x\n", value);
            irq_mask = value;
            return;
        }
    }

    printf("[DEV9] Unrecognized write16 to $%08x of $%02x\n", address, value);
}

void DEV9::write32(uint32_t address, uint32_t value)
{
    if (address >= SMAP_REGBASE && address <= FLASH_REGBASE)
    {
      printf("[DEV9] Unrecognized SMAP write32 to $%04x of %04x\n", address, value);
      // SMAP
      return;
    }
    printf("[DEV9] Unrecognized DEV9 write32 to $%08x of $%04x\n", address, value);
}
