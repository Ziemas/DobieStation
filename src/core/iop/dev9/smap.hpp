#ifndef __SMAP_H_
#define __SMAP_H_

#include <cstdint>

#define SMAP_REGBASE 0x10000100
#define SMAP_REG(offset) (SMAP_REGBASE + (offset))

#define SMAP_R_BD_MODE 0x02

#define SMAP_R_TXFIFO_CTRL 0xf00
#define SMAP_R_TXFIFO_DATA 0x1000

#define SMAP_R_RXFIFO_CTRL 0xf30

#define SMAP_EMAC3_REGBASE 0x1f00
#define EMAC3_REG(offset) (SMAP_REGBASE + SMAP_EMAC3_REGBASE + (offset))
#define SMAP_EMAC3_MODE0 0x0

#define SMAP_E3_RXMAC_IDLE (1<<31)
#define SMAP_E3_TXMAC_IDLE (1<<30)
#define SMAP_E3_SOFT_RESET (1<<29)
#define SMAP_E3_TXMAC_ENABLE (1<<28)
#define SMAP_E3_RXMAC_ENABLE (1<<27)
#define SMAP_E3_WAKEUP_ENABLE (1<<26)

#define SMAP_BD_REGBASE 0x2f00
#define SMAP_BD_TX_BASE (SMAP_BD_REGBASE + 0x0000)
#define SMAP_BD_RX_BASE (SMAP_BD_REGBASE + 0x0200)

struct smap_bd
{
    uint16_t ctrl_stat;
    /** must be zero */
    uint16_t reserved;
    /** number of bytes in pkt */
    uint16_t length;
    uint16_t pointer;
};

class SMAP
{
    private:
        enum CTRL
        {
            CTRL_RESET = 0x1,
            CTRL_DMA_ENABLE= 0x2,
        };

        uint8_t bd_mode;
        uint32_t emac3_mode0;
        uint32_t mac_address;

        // 64 buffer descriptors, 512 kb
        smap_bd rx_bd[64] = {};
        smap_bd tx_bd[64] = {};

        uint32_t rx_bd_index = 0;
        // Seems big... 4kb on ppc405gp emac
        uint8_t rxfifo[16*1024] = {};
        uint16_t rxfifo_write_ptr = 0;

        uint32_t tx_bd_index = 0;
        // Seems big... 2kb on ppc405gp emac
        uint8_t txfifo[16*1024] = {};
        uint16_t txfifo_write_ptr = 0;

        //uint8_t bd[512];

        uint8_t txfifo_ctrl = 0;
        uint8_t rxfifo_ctrl = 0;

    public:

        uint8_t read8(uint32_t address);
        uint16_t read16(uint32_t address);
        uint32_t read32(uint32_t address);
        void write8(uint32_t address, uint8_t value);
        void write16(uint32_t address, uint16_t value);
        void write32(uint32_t address, uint32_t value);
};

#endif // __SMAP_H_
