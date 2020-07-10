#ifndef __DEV9_H_
#define __DEV9_H_

#include <cstdint>

#define SPEED_CHIP_VER 0x0011

#define SPD_REGBASE 0x10000000
#define SMAP_REGBASE (SPD_REGBASE + 0x100)
#define FLASH_REGBASE 0x10004800

#define SPD_REG(offset) (SPD_REGBASE + (offset))

#define SPD_CAPS_SMAP (1 << 0)
#define SPD_CAPS_ATA (1 << 1)
#define SPD_CAPS_UART (1 << 3)
#define SPD_CAPS_DVR (1 << 4)
#define SPD_CAPS_FLASH (1 << 5)

#define SPD_R_DMA_CTRL 0x24
#define SPD_R_INTR_STAT 0x28
#define SPD_R_INTR_MASK 0x2a
#define SPD_R_PIO_DIR 0x2c
#define SPD_R_PIO_DATA 0x2e

#define SPD_R_REV 0x00
#define SPD_R_REV_1 0x02
#define SPD_R_REV_3 0x04
#define SPD_R_REV_8 0x0e

#define DEV9_R_POWER 0x1F80146C
#define DEV9_R_REV 0x1F80146E

class DEV9
{
    private:
        uint16_t irq_stat = 0;
        uint16_t irq_mask = 0;

        enum DEV9_TYPE
        {
            PCMCIA = 0x20,
            EXPBAY = 0x30,
        };

    public:
        uint8_t read8(uint32_t address);
        uint16_t read16(uint32_t address);
        uint32_t read32(uint32_t address);

        void write8(uint32_t address, uint8_t value);
        void write16(uint32_t address, uint16_t value);
        void write32(uint32_t address, uint32_t value);
};


#endif // __DEV9_H_
