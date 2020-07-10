#ifndef __DEV9_H_
#define __DEV9_H_

#include <cstdint>

#define DEV9_IDENT 0x32

class DEV9
{
    private:
    public:
    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);
    uint32_t read32(uint32_t address);

    void write8(uint32_t address, uint8_t value);
    void write16(uint32_t address, uint16_t value);
    void write32(uint32_t address, uint32_t value);
};


#endif // __DEV9_H_
