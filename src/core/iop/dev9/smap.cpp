#include "smap.hpp"
#include "dev9.hpp"
#include <cstdio>
#include <cstring>

SMAP::SMAP(DEV9& dev9) : dev9(dev9)
{
}

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
    if (address >= SMAP_REG(SMAP_BD_TX_BASE) && address < SMAP_REG(SMAP_BD_RX_BASE))
    {
        uint32_t index = (address - SMAP_REG(SMAP_BD_TX_BASE)) / 2;
        uint8_t bd_num = index / 4;
        printf("[DEV9] [SMAP] TX BD Read from BD %d(%08x) of %04x\n", bd_num, address, tx_bd.raw[index]);
        return tx_bd.raw[index];
    }
    if (address >= SMAP_REG(SMAP_BD_RX_BASE) && address < SMAP_REG(SMAP_BD_RX_BASE) + 0x200)
    {
        uint32_t index = (address - SMAP_REG(SMAP_BD_RX_BASE)) / 2;
        uint8_t bd_num = index / 4;
        printf("[DEV9] [SMAP] RX BD Read from BD %d(%08x) of %04x\n", bd_num, address, rx_bd.raw[index]);
        return rx_bd.raw[index];
    }

    switch (address)
    {
        case SMAP_REG(SMAP_R_TXFIFO_SIZE):
            // Misleading reg name? SMAP doesn't control fifo size
            // this seems to be used for DMA slice count
            printf("[DEV9] [SMAP] Read SMAP_R_TXFIFO_SIZE %04x\n", txdma_slice_count);
            return txdma_slice_count;
        case SMAP_REG(SMAP_R_TXFIFO_WR_PTR):
            // TODO: check if we need this to offset to the correct memory
            printf("[DEV9] [SMAP] Read SMAP_R_TXFIFO_WR_PTR %04x\n", txfifo.wpos());
            return txfifo.write;
        case SMAP_REG(SMAP_R_RXFIFO_SIZE):
            // Misleading reg name? SMAP doesn't control fifo size
            // this seems to be used for DMA slice count
            printf("[DEV9] [SMAP] Read SMAP_R_RXFIFO_SIZE %04x\n", rxdma_slice_count);
            return rxdma_slice_count;
        case SMAP_REG(SMAP_R_RXFIFO_RD_PTR):
            // TODO: check if we need this to offset to the correct memory
            printf("[DEV9] [SMAP] Read SMAP_R_RXFIFO_RD_PTR %04x\n", txfifo.rpos());
            return rxfifo.read;
    }

    printf("[DEV9] [SMAP] Unrecognized read 16 from %08x\n", address);
    return 0;
}

uint32_t SMAP::read32(uint32_t address)
{
    switch (address)
    {
        case EMAC3_REG(SMAP_R_EMAC3_MODE0):
            printf("[DEV9] [SMAP] Read EMAC3_MODE0 %08x\n", emac3_mode0);
            return EMAC3_WSWAP(emac3_mode0);
        case EMAC3_REG(SMAP_R_EMAC3_MODE1):
            printf("[DEV9] [SMAP] Read EMAC3_MODE0 %08x\n", emac3_mode1);
            return EMAC3_WSWAP(emac3_mode1);
        case EMAC3_REG(SMAP_R_EMAC3_TxMODE0):
            printf("[DEV9] [SMAP] Read EMAC3_TXMODE0 %08x\n", 0);
            return 0;
        case EMAC3_REG(SMAP_R_EMAC3_TxMODE1):
            printf("[DEV9] [SMAP] Read EMAC3_TXMODE1 %08x\n", emac3_txmode1);
            return EMAC3_WSWAP(emac3_txmode1);
        case EMAC3_REG(SMAP_R_EMAC3_RxMODE):
            printf("[DEV9] [SMAP] Read EMAC3_RXMODE %08x\n", emac3_rxmode);
            return EMAC3_WSWAP(emac3_rxmode);
        case EMAC3_REG(SMAP_R_EMAC3_INTR_STAT):
            printf("[DEV9] [SMAP] Read EMAC3_INTR_STAT %08x\n", emac3_intr_stat);
            return EMAC3_WSWAP(emac3_intr_stat);
        case EMAC3_REG(SMAP_R_EMAC3_INTR_ENABLE):
            printf("[DEV9] [SMAP] Read EMAC3_INTR_ENABLE %08x\n", emac3_intr_enable);
            return EMAC3_WSWAP(emac3_intr_enable);
        case EMAC3_REG(SMAP_R_EMAC3_ADDR_HI):
            printf("[DEV9] [SMAP] Read EMAC3_ADDR_HI %08x\n", (uint32_t)(mac_address >> 32));
            return EMAC3_WSWAP(mac_address >> 32);
        case EMAC3_REG(SMAP_R_EMAC3_ADDR_LO):
            printf("[DEV9] [SMAP] Read EMAC3_ADDR_LO %08x\n", (uint32_t)(mac_address & 0xFFFFFFFF));
            return EMAC3_WSWAP(mac_address & 0xFFFFFFFF);
        case EMAC3_REG(SMAP_R_EMAC3_PAUSE_TIMER):
            printf("[DEV9] [SMAP] Read EMAC3_PAUSE_TIMER %08x\n", emac3_pause_timer);
            return EMAC3_WSWAP(emac3_pause_timer);
        case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH1):
            printf("[DEV9] [SMAP] Read EMAC3_GROUP_HASH1 %08x\n", emac3_group_hash1);
            return EMAC3_WSWAP(emac3_group_hash1);
        case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH2):
            printf("[DEV9] [SMAP] Read EMAC3_GROUP_HASH1 %08x\n", emac3_group_hash2);
            return EMAC3_WSWAP(emac3_group_hash2);
        case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH3):
            printf("[DEV9] [SMAP] Read EMAC3_GROUP_HASH1 %08x\n", emac3_group_hash3);
            return EMAC3_WSWAP(emac3_group_hash3);
        case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH4):
            printf("[DEV9] [SMAP] Read EMAC3_GROUP_HASH1 %08x\n", emac3_group_hash4);
            return EMAC3_WSWAP(emac3_group_hash4);
        case EMAC3_REG(SMAP_R_EMAC3_INTER_FRAME_GAP):
            printf("[DEV9] [SMAP] Read EMAC3_INTR_FRAME_GAP %08x\n", emac3_inter_frame_gap);
            return EMAC3_WSWAP(emac3_inter_frame_gap);
        case EMAC3_REG(SMAP_R_EMAC3_TX_THRESHOLD):
            printf("[DEV9] [SMAP] Read EMAC3_TX_THRESHOLD %08x\n", emac3_tx_threshold);
            return EMAC3_WSWAP(emac3_tx_threshold);
        case EMAC3_REG(SMAP_R_EMAC3_RX_WATERMARK):
            printf("[DEV9] [SMAP] Read EMAC3_RX_WATERMARK %08x\n", emac3_rx_watermark);
            return EMAC3_WSWAP(emac3_rx_watermark);
        case EMAC3_REG(SMAP_R_EMAC3_STA_CTRL):
            printf("[DEV9] [SMAP] Read EMAC3_STA_CTRL %08x\n", emac3_sta_ctrl);
            return EMAC3_WSWAP(emac3_sta_ctrl);
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
            printf("[DEV9] [SMAP] Write8 TXFIFO_CTRL %02x\n", value);
            if (value & SMAP_TXFIFO_RESET) // Reset the fifo?
            {
                txfifo.reset(); // maybe
                txfifo_ctrl = 0;
            }
            if (value & SMAP_TXFIFO_DMAEN)
            {
                // Enable DMA TODO: should reset after completed dma?
                printf("[DEV9] [SMAP] TXFIFO ENABLE DMA\n");
                dev9.request_dma();
            }
            return;
        case SMAP_REG(SMAP_R_TXFIFO_FRAME_CNT):
            printf("[DEV9] [SMAP] Write SMAP_R_TXFIFO_FRAME_CNT %02x\n", value);
            return;
        case SMAP_REG(SMAP_R_TXFIFO_FRAME_INC):
            printf("[DEV9] [SMAP] Write SMAP_R_TXFIFO_FRAME_INC %02x\n", value);
            return;
        case SMAP_REG(SMAP_R_RXFIFO_CTRL):
            printf("[DEV9] [SMAP] Write RXFIFO_CTRL %02x\n", value);
            if (value == SMAP_RXFIFO_RESET) // Reset the fifo?
            {
                rxfifo.reset(); // maybe
                rxfifo_ctrl = 0;
            }
            if (value & SMAP_TXFIFO_DMAEN)
            {
                // Enable DMA TODO: should reset after completed dma?
                printf("[DEV9] [SMAP] RXFIFO ENABLE DMA\n");
                dev9.request_dma();
            }
            return;
    }
    printf("[DEV9] [SMAP] Unrecognized write8 to $%08x of %02x\n", address, value);
    return;
}

void SMAP::checkBD(smap_bd bd)
{
    // Just for fun we can check what it wants to transfer
    if (bd.ctrl_stat & (1 << 15))
    {
        printf("DEV9 bd ready: DST ");
        FrameHeader fh = *(FrameHeader*)&txfifo.array[bd.pointer - 0x1000];

        for (int i = 0; i < 6; i++)
        {
            printf("%02x ", fh.dst_mac[i]);
        }

        printf(" | SRC: ");

        for (int i = 0; i < 6; i++)
        {
            printf("%02x ", fh.src_mac[i]);
        }
        printf("| tag: %08x ", fh.tag);
        printf("| len: %04x\n", fh.len);
    }
}

void SMAP::write16(uint32_t address, uint16_t value)
{
    if (address >= SMAP_REG(SMAP_BD_TX_BASE) && address < SMAP_REG(SMAP_BD_RX_BASE))
    {
        uint32_t index = (address - SMAP_REG(SMAP_BD_TX_BASE)) / 2;
        uint8_t bd_num = index / 4;

        printf("[DEV9] [SMAP] TX BD Write to BD %d(%08x) of %04x\n", bd_num, index, value);

        tx_bd.raw[index] = value;
        //checkBD(tx_bd.bd[bd_num]);
        return;
    }
    if (address >= SMAP_REG(SMAP_BD_RX_BASE) && address < SMAP_REG(SMAP_BD_RX_BASE) + 0x200)
    {
        uint32_t index = (address - SMAP_REG(SMAP_BD_RX_BASE)) / 2;
        uint8_t bd_num = index / 4;
        printf("[DEV9] [SMAP] RX BD Write to BD %d(%08x) of %04x\n", bd_num, address, value);
        rx_bd.raw[index] = value;
        return;
    }
    switch (address)
    {
        case SMAP_REG(SMAP_R_INTR_CLR):
            printf("[DEV9] [SMAP] Write INTR_CLR %04x\n", value);
            return;
        case SMAP_REG(SMAP_R_TXFIFO_SIZE):
            // Misleading reg name? SMAP doesn't control fifo size
            // this seems to be used for DMA slice count
            printf("[DEV9] [SMAP] Write SMAP_R_TXFIFO_SIZE %04x\n", value);
            //txfifo_size = value; DMA Slice count?
            txdma_slice_count = value;
            return;
        case SMAP_REG(SMAP_R_TXFIFO_WR_PTR):
            printf("[DEV9] [SMAP] Write SMAP_R_TXFIFO_WR_PTR %04x\n", value);
            txfifo.write = value;
            return;
        case SMAP_REG(SMAP_R_RXFIFO_SIZE):
            // Misleading reg name? SMAP doesn't control fifo size
            // this seems to be used for DMA slice count
            printf("[DEV9] [SMAP] Write SMAP_R_RXFIFO_SIZE %04x\n", value);
            //rxfifo_size = value; DMA slice count?
            rxdma_slice_count = value;
            return;
        case SMAP_REG(SMAP_R_RXFIFO_RD_PTR):
            printf("[DEV9] [SMAP] Write SMAP_R_RXFIFO_RD_PTR %04x\n", value);
            rxfifo.read = value;
            return;
    }
    printf("[DEV9] [SMAP] Unrecognized SMAP write16 to $%08x of %04x\n", address, value);
    return;
}

void SMAP::write32(uint32_t address, uint32_t value)
{
    if (address >= SMAP_REGBASE && address < SMAP_EMAC3_REGBASE)
    {
        switch (address)
        {
            case SMAP_REG(SMAP_R_TXFIFO_DATA):
            {
                printf("[DEV9] [SMAP] write32 SMAP_R_TXFIFO_DATA pos: %x, %08x\n", txfifo.write, value);
                txfifo.push(value);
                return;
            }
        }

        printf("[DEV9] [SMAP] Write to unknown SMAP register %08x of %08x\n", address, value);
        return;
    }

    if (address >= SMAP_EMAC3_REGBASE && address < SMAP_REG(SMAP_BD_REGBASE))
    {
        switch (address)
        {
            case EMAC3_REG(SMAP_R_EMAC3_MODE0):
                printf("[DEV9] [SMAP] Write EMAC3_MODE0($%08x) %08x\n", address, EMAC3_WSWAP(value));
                if (value == EMAC3_WSWAP(SMAP_E3_SOFT_RESET))
                {
                    printf("[DEV9] [SMAP] EMAC3 SOFT RESET\n");
                    // TODO: I guess everything should go...
                    // maybe...
                    rxfifo.reset();
                    txfifo.reset();
                    emac3_mode0 = 0;

                    // on second thought, emac3 reset probably wouldn't do anything to the bds?
                    for (auto& bd : rx_bd.bd)
                    {
                        bd.ctrl_stat = 0;
                        bd.reserved = 0;
                        bd.length = 0;
                        bd.pointer = 0;
                    }

                    for (auto& bd : tx_bd.bd)
                    {
                        bd.ctrl_stat = 0;
                        bd.reserved = 0;
                        bd.length = 0;
                        bd.pointer = 0;
                    }
                }
                return;
            case EMAC3_REG(SMAP_R_EMAC3_MODE1):
                printf("[DEV9] [SMAP] Write EMAC3_MODE1 %08x\n", EMAC3_WSWAP(value));
                emac3_mode1 = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_TxMODE0):
                // TODO: This is what gets poked when the driver wants to transmit a frame
                printf("[DEV9] [SMAP] Write EMAC3_TxMODE0 %08x\n", EMAC3_WSWAP(value));
                return;
            case EMAC3_REG(SMAP_R_EMAC3_TxMODE1):
                printf("[DEV9] [SMAP] Write EMAC3_TxMODE1 %08x\n", EMAC3_WSWAP(value));
                emac3_txmode1 = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_RxMODE):
                printf("[DEV9] [SMAP] Write EMAC3_RXMODE %08x\n", EMAC3_WSWAP(value));
                emac3_rxmode = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_INTR_STAT):
                printf("[DEV9] [SMAP] Write EMAC3_INTR_STAT %08x\n", EMAC3_WSWAP(value));
                emac3_intr_stat = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_INTR_ENABLE):
                printf("[DEV9] [SMAP] Write EMAC3_INTR_ENABLE %08x\n", EMAC3_WSWAP(value));
                emac3_intr_enable = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_ADDR_HI):
                printf("[DEV9] [SMAP] Write EMAC3_ADDR_HI %08x\n", EMAC3_WSWAP(value));
                mac_address &= mac_address & 0xFFFFFFFF;
                mac_address |= (uint64_t)(EMAC3_WSWAP(value)) << 32;
                return;
            case EMAC3_REG(SMAP_R_EMAC3_ADDR_LO):
                printf("[DEV9] [SMAP] Write EMAC3_ADDR_LO %08x\n", EMAC3_WSWAP(value));
                mac_address &= 0xFFFF00000000;
                mac_address |= EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_PAUSE_TIMER):
                printf("[DEV9] [SMAP] Write EMAC3_PAUSE_TIMER %08x\n", EMAC3_WSWAP(value));
                emac3_pause_timer = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH1):
                printf("[DEV9] [SMAP] Write EMAC3_GROUP_HASH1 %08x\n", EMAC3_WSWAP(value));
                emac3_group_hash1 = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH2):
                printf("[DEV9] [SMAP] Write EMAC3_GROUP_HASH2 %08x\n", EMAC3_WSWAP(value));
                emac3_group_hash2 = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH3):
                printf("[DEV9] [SMAP] Write EMAC3_GROUP_HASH3 %08x\n", EMAC3_WSWAP(value));
                emac3_group_hash3 = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_GROUP_HASH4):
                printf("[DEV9] [SMAP] Write EMAC3_GROUP_HASH4 %08x\n", EMAC3_WSWAP(value));
                emac3_group_hash4 = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_INTER_FRAME_GAP):
                printf("[DEV9] [SMAP] Write EMAC3_INTR_FRAME_GAP %08x\n", EMAC3_WSWAP(value));
                emac3_inter_frame_gap = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_TX_THRESHOLD):
                printf("[DEV9] [SMAP] Write EMAC3_TX_THRESHOLD %08x\n", EMAC3_WSWAP(value));
                emac3_tx_threshold = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_RX_WATERMARK):
                printf("[DEV9] [SMAP] Write EMAC3_RX_WATERMARK %08x\n", EMAC3_WSWAP(value));
                emac3_rx_watermark = EMAC3_WSWAP(value);
                return;
            case EMAC3_REG(SMAP_R_EMAC3_STA_CTRL):
                write_sta(value);
                return;
        }

        printf("[DEV9] [SMAP] Write to unknown EMAC3 register %08x of %08x\n", address, value);
        return;
    }

    printf("[DEV9] [SMAP] Unrecognized SMAP write32 to $%08x of %08x\n", address, value);
    return;
}

// Sony's network driver uses 16bit writes for STA :(
void SMAP::write_sta(uint32_t reg)
{
    uint32_t sta = EMAC3_WSWAP(reg);
    printf("[DEV9] [SMAP] Write EMAC3_STA_CTRL %08x\n", sta);
    emac3_sta_ctrl = sta;
    uint8_t command = (sta >> 12) & 0x3;

    if (command == 0)
        return;

    uint8_t address = sta & 0x1f;
    uint16_t data = sta >> 16;
    printf("[DEV9] [SMAP] STA WRITE cmd: %d, addr: %x, data: %x\n", command, address, data);

    switch (command)
    {
        case 1:
        {
            uint16_t response = read_phy(address, data);
            emac3_sta_ctrl = (emac3_sta_ctrl & 0xFFFF) | (response << 16);
            break;
        }
        case 2:
            write_phy(address, data);
            break;
        default:
            printf("[DEV9] [SMAP] UNKNOWN STA_CTRL WRITE\n");
    }

    emac3_sta_ctrl |= SMAP_E3_PHY_OP_COMP;

    return;
}

uint16_t SMAP::read_phy(uint8_t address, uint16_t value)
{
    switch (address)
    {
        case SMAP_DsPHYTER_BMCR: // Basic mode control
            printf("[DEV9] [SMAP] Read PHY BMCR\n");
            return 0;
        case SMAP_DsPHYTER_BMSR:
            printf("[DEV9] [SMAP] Write to PHY BMSR of %04x\n", value);
            return 0 | SMAP_PHY_BMSR_100btxfd | SMAP_PHY_BMSR_LINK_STAT | SMAP_PHY_BMSR_ANEN_COMPLETE | SMAP_PHY_BMSR_ANEN_AVAIL;
    }
    printf("[DEV9] [SMAP] Read from PHY %02x\n", address);
    return 0;
}

void SMAP::write_phy(uint8_t address, uint16_t value)
{
    switch (address)
    {
        case SMAP_DsPHYTER_BMCR: // Basic mode control
            printf("[DEV9] [SMAP] Write to PHY BMCR of %04x\n", value);
            if (value == SMAP_PHY_BMCR_RST)
                printf("[DEV9] [SMAP] PHY Reset\n");
            return;
        case SMAP_DsPHYTER_BMSR:
            printf("[DEV9] [SMAP] Write to PHY BMSR of %04x\n", value);
            return;
    }
    printf("[DEV9] [SMAP] Write to PHY %02x of %04x\n", address, value);
}

void SMAP::write_DMA(uint32_t value)
{
    printf("[DEV9] [SMAP] DMA Write of %08x\n", value);
    txfifo.push(value);
}

uint32_t SMAP::read_DMA()
{
    printf("[DEV9] [SMAP] DMA read\n");
    return 0;
}
