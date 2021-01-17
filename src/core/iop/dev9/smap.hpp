#ifndef __SMAP_H_
#define __SMAP_H_

#include <cstdint>
#include <memory>
#include <cstring>

/* SMAP is the network chip of the PS2's network adaptor.
 * It is hooked up to an EMAC very similar to the one on the PPC 405GP
 * embedded processor. The PHY is a National Semiconductor DP83846A */

#define SMAP_REGBASE 0x10000100
#define SMAP_REG(offset) (SMAP_REGBASE + (offset))

#define SMAP_R_BD_MODE 0x02

#define SMAP_R_INTR_CLR 0x28

#define SMAP_R_TXFIFO_CTRL 0xf00
#define SMAP_TXFIFO_RESET (1 << 0)
#define SMAP_TXFIFO_DMAEN (1 << 1)
#define SMAP_R_TXFIFO_WR_PTR 0xf04
#define SMAP_R_TXFIFO_SIZE 0xf08
#define SMAP_R_TXFIFO_DATA 0x1000

#define SMAP_R_RXFIFO_CTRL 0xf30
#define SMAP_RXFIFO_RESET (1 << 0)
#define SMAP_RXFIFO_DMAEN (1 << 1)
#define SMAP_R_RXFIFO_RD_PTR 0xf34
#define SMAP_R_RXFIFO_SIZE 0xf38
#define SMAP_R_RXFIFO_DATA 0x1100

#define SMAP_EMAC3_REGBASE 0x1f00
#define EMAC3_REG(offset) (SMAP_REGBASE + SMAP_EMAC3_REGBASE + (offset))
#define SMAP_R_EMAC3_MODE0 0x0

#define SMAP_E3_RXMAC_IDLE (1 << 31)
#define SMAP_E3_TXMAC_IDLE (1 << 30)
#define SMAP_E3_SOFT_RESET (1 << 29)
#define SMAP_E3_TXMAC_ENABLE (1 << 28)
#define SMAP_E3_RXMAC_ENABLE (1 << 27)
#define SMAP_E3_WAKEUP_ENABLE (1 << 26)

#define SMAP_R_EMAC3_MODE1 0x04

#define SMAP_R_EMAC3_TxMODE1 0x0C

#define SMAP_R_EMAC3_RxMODE 0x10

#define SMAP_R_EMAC3_INTR_STAT 0x14

#define SMAP_R_EMAC3_INTR_ENABLE 0x18

#define SMAP_R_EMAC3_ADDR_HI 0x1C
#define SMAP_R_EMAC3_ADDR_LO 0x20

#define SMAP_R_EMAC3_PAUSE_TIMER 0x2C

#define SMAP_R_EMAC3_GROUP_HASH1 0x40
#define SMAP_R_EMAC3_GROUP_HASH2 0x44
#define SMAP_R_EMAC3_GROUP_HASH3 0x48
#define SMAP_R_EMAC3_GROUP_HASH4 0x4C

#define SMAP_R_EMAC3_INTER_FRAME_GAP 0x58

#define SMAP_R_EMAC3_STA_CTRL 0x5C
#define SMAP_E3_PHY_OP_COMP (1 << 15)

#define SMAP_R_EMAC3_TX_THRESHOLD 0x60
#define SMAP_R_EMAC3_RX_WATERMARK 0x64

#define SMAP_BD_REGBASE 0x2f00
#define SMAP_BD_TX_BASE (SMAP_BD_REGBASE + 0x0000)
#define SMAP_BD_RX_BASE (SMAP_BD_REGBASE + 0x0200)

// SMAP driver word swaps before writing, swapping backs makes it easier to reason about
#define EMAC3_WSWAP(val) ((((val) >> 16) & 0xffff) | (((val)&0xffff) << 16))

#define SMAP_DsPHYTER_BMCR 0x00
#define SMAP_PHY_BMCR_RST (1 << 15)
#define SMAP_DsPHYTER_BMSR 0x01
#define SMAP_PHY_BMSR_LINK_STAT (1 << 2)
#define SMAP_PHY_BMSR_ANEN_AVAIL (1 << 3)
#define SMAP_PHY_BMSR_ANEN_COMPLETE (1 << 5)
#define SMAP_PHY_BMSR_100btxfd (1 << 15)

struct smap_bd
{
    uint16_t ctrl_stat;
    /** must be zero */
    uint16_t reserved;
    /** number of bytes in pkt */
    uint16_t length;
    uint16_t pointer;
};

class DEV9;

class SMAP
{
  public:
    SMAP(DEV9& dev9);

    uint32_t read_DMA();
    void write_DMA(uint32_t value);

    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);
    uint32_t read32(uint32_t address);
    void write8(uint32_t address, uint8_t value);
    void write16(uint32_t address, uint16_t value);
    void write32(uint32_t address, uint32_t value);

  private:
    DEV9& dev9;

    enum class CTRL
    {
        RESET = 0x1,
        DMA_ENABLE = 0x2,
    };

#pragma pack(push, 1)
    union emac3_regs
    {
        uint16_t raw16[27 * 2];
        uint32_t raw32[27];

        struct
        {
            /* 0x0  */ uint32_t mode0;
            /* 0x4  */ uint32_t mode1;
            /* 0x8  */ uint32_t txmode0;
            /* 0xc  */ uint32_t txmode1;
            /* 0x10 */ uint32_t rxmode;
            /* 0x14 */ uint32_t intr_stat;
            /* 0x18 */ uint32_t intr_enable;
            /* 0x1c */ uint64_t mac_address;
            /* 0x24 */ uint32_t vlan_tpid;
            /* 0x28 */ uint32_t vlan_tci;
            /* 0x3c */ uint32_t pause_timer;
            /* 0x30 */ uint32_t indidual_hash[4];
            /* 0x40 */ uint32_t group_hash[4];
            /* 0x50 */ uint64_t last_sa;
            /* 0x58 */ uint32_t inter_frame_gap;
            /* 0x5C */ uint32_t sta_ctrl;
            /* 0x60 */ uint32_t tx_threshold;
            /* 0x64 */ uint32_t rx_watermark;
            /* 0x68 */ uint32_t tx_octets;
            /* 0x6c */ uint32_t rx_octets;
        };
    };
#pragma pack(pop)

    emac3_regs emac3reg = {};

    // TODO: parse all the bit fields in these regs
    // FIFO size etc
    uint8_t bd_mode = 0;
    uint32_t emac3_mode0 = 0;
    uint32_t emac3_mode1 = 0;
    uint32_t emac3_txmode1 = 0;
    uint32_t emac3_rxmode = 0;
    uint32_t emac3_intr_stat = 0;
    uint32_t emac3_intr_enable = 0;
    uint32_t emac3_pause_timer = 0;
    // Probably handle these differently later
    uint32_t emac3_group_hash1 = 0;
    uint32_t emac3_group_hash2 = 0;
    uint32_t emac3_group_hash3 = 0;
    uint32_t emac3_group_hash4 = 0;
    uint32_t emac3_inter_frame_gap = 0;
    uint32_t emac3_sta_ctrl = 0;
    uint32_t emac3_tx_threshold = 0;
    uint32_t emac3_rx_watermark = 0;
    uint64_t mac_address = 0;

    // 64 buffer descriptors, 512 kb
    //std::unique_ptr<smap_bd[64]> rx_bd;
    //std::unique_ptr<smap_bd[64]> tx_bd;

    union bufdesc
    {
        uint16_t raw[64 * 4];
        smap_bd bd[64];
    };

    bufdesc tx_bd = {};
    bufdesc rx_bd = {};

    uint16_t rxdma_slice_count = 0;
    uint32_t rx_bd_index = 0;

    struct Fifo
    {
        uint32_t array[1024] = {0};
        uint16_t capacity = 1024;
        uint16_t read = 0;
        uint16_t write = 0;

        uint16_t mask(uint32_t val) { return val & (capacity - 1); }

        uint16_t rpos() { return mask(read); }
        uint16_t wpos() { return mask(write); }

        uint32_t& front() { return array[mask(read)]; }
        uint32_t& back() { return array[mask(write)]; }
        void push(uint32_t val) { array[mask(write++)] = val; }
        uint32_t pop() { return array[mask(read++)]; }
        uint32_t size() { return write - read; }
        bool full() { return size() == capacity; }
        bool empty() { return read == write; }

        void reset()
        {
            std::memset(array, 0, 64);
            read = 0;
            write = 0;
        }
    };

    // Fifo size is configurable by the driver
    // TODO: get fifo size from EMAC3 MODE1 reg
    // RXFIFO options are 512, 1kb, 2kb.
    Fifo rxfifo = {};
    // TXFIFO options are 512, 1kb, 2kb, 4kb
    Fifo txfifo = {};

    uint16_t txdma_slice_count = 0;
    uint32_t tx_bd_index = 0;
    //uint16_t txfifo_size = 512;
    //uint8_t txfifo[2 * 1024] = {};
    //uint16_t txfifo_write_ptr = 0;
    //uint16_t txfifo_read_ptr = 0;

    uint8_t txfifo_ctrl = 0;
    uint8_t rxfifo_ctrl = 0;

    void write_phy(uint8_t address, uint16_t value);
    uint16_t read_phy(uint8_t address, uint16_t value);
    void write_sta(uint32_t reg);
};

#endif // __SMAP_H_
