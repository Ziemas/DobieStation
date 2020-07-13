#include "smap.hpp"
#include <cstdio>

uint8_t SMAP::read8(uint32_t address)
{
    switch (address)
    {
        case SMAP_REG(SMAP_R_BD_MODE):
            printf("[DEV9] [SMAP] Read BD_MODE 0%2x\n", bd_mode);
            return bd_mode;
        case SMAP_REG(SMAP_R_TXFIFO_CTRL):
            printf("[DEV9] [SMAP] Read8 TXFIFO_CTRL\n");
            return txfifo_ctrl;
        case SMAP_REG(SMAP_R_RXFIFO_CTRL):
            printf("[DEV9] [SMAP] Read8 RXFIFO_CTRL\n");
            return rxfifo_ctrl;
    }
    printf("[DEV9] [SMAP] unrecognized read8 from %08x\n", address);
    return 0;
}

uint16_t SMAP::read16(uint32_t address)
{
    printf("[DEV9] [SMAP] Unrecognized read 16 from %08x\n", address);
    return 0;
}

uint32_t SMAP::read32(uint32_t address)
{
    switch (address)
    {
        case EMAC3_REG(SMAP_EMAC3_MODE0):
            printf("[DEV9] [SMAP] Read EMAC3_MODE0 %08x\n", emac3_mode0);
            return emac3_mode0;
    }
    printf("[DEV9] [SMAP] Read32 unrecognized SMAP reg %08x\n", address);
    return 0;
}

void SMAP::write8(uint32_t address, uint8_t value)
{
    switch (address)
    {
        case SMAP_REG(SMAP_R_BD_MODE):
            printf("[DEV9] [SMAP] Write BD_MODE %02x\n", value);
            bd_mode = value;
            return;
        case SMAP_REG(SMAP_R_TXFIFO_CTRL):
            printf("[DEV9] [SMAP] Write TXFIFO_CTRL %02x\n", value);
            if (value == CTRL_RESET) // Reset the fifo?
                txfifo_ctrl = 0;
            return;
        case SMAP_REG(SMAP_R_RXFIFO_CTRL):
            printf("[DEV9] [SMAP] Write RXFIFO_CTRL %02x\n", value);
            if (value == CTRL_RESET) // Reset the fifo?
                txfifo_ctrl = 0;
            return;
    }
    printf("[DEV9] [SMAP] Unrecognized write8 to $%08x of %02x\n", address, value);
    return;
}

void SMAP::write16(uint32_t address, uint16_t value)
{
    printf("[DEV9] [SMAP] Unrecognized SMAP write16 to $%08x of %04x\n", address, value);

    return;
}

void SMAP::write32(uint32_t address, uint32_t value)
{
    switch (address)
    {
        case EMAC3_REG(SMAP_EMAC3_MODE0):
            printf("[DEV9] [SMAP] Write EMAC3_MODE0 %08x\n", value);
            if (value == SMAP_E3_SOFT_RESET)
            {
                printf("[DEV9] [SMAP] Reset emac3 mode0\n");
                emac3_mode0 = 0;
            }
            return;
    }
    printf("[DEV9] [SMAP] Unrecognized SMAP write32 to $%08x of %08x\n", address, value);
    return;
}
