#include "eeprom.hpp"
#include <cstdio>
#include <array>

// TODO: lets just take a MAC address string and generate the eeprom content
// it's just the bytes of the address and a simple checksum (adding 3 shorts together)

uint16_t hardcoded_eeprom[32] = {
    0x6D76, 0x6361, 0x3130, 0x0207,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x1000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0010, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x1000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000,
};

EEPROM::EEPROM(uint16_t* eeprom_data) : eeprom(eeprom_data)
{
    if (eeprom == nullptr)
    {
        eeprom = hardcoded_eeprom;
    }
}

void EEPROM::step()
{
    uint8_t line = (data >> 5) & 1;
    switch (state)
    {
        case READ_COMMAND: // read two bits of command
            command |= line << (1 - sequence);
            sequence++;
            if (sequence == 2)
            {
                sequence = 0;
                state = READ_ADDRESS;
            }
            break;
        case READ_ADDRESS: // read 6 bits of address
            // TODO: is this correct? i only have a address 0 test case
            address |= line << (5 - sequence);

            sequence++;
            if (sequence == 6)
            {
                state = ACK;
                sequence = 0;
            }
            break;
        case ACK: // wait one clock pulse
            data = 0;
            sequence = 0;
            state = TRANSMIT;
            break;
        case TRANSMIT: // fire away, bit position increments every pulse
            if (command == READ)
                data = ((eeprom[address] << sequence) & 0x8000) >> 11;

            if (command == WRITE) // TODO: untested
                eeprom[address] = eeprom[address] | (line << (15 - sequence));

            sequence++;
            if (sequence == 16)
            {
                sequence = 0;
                address++;
            }
            break;
    }
    return;
}

void EEPROM::write(uint8_t value)
{
    printf("[DEV9] [EEPROM] Write PIO_DATA %02x\n", value);
    uint8_t enable = (value >> 7) & 1;
    uint8_t new_clock = (value >> 6) & 1;

    if (enable == 0)
    {
        sequence = 0;
        address = 0;
        state = 0;
        clock = 0;
        return;
    }

    data = value;
    if (new_clock == 1)
    {
        // The eeprom works on the rising clock edge
        if (clock == 0)
        {
            step();
        }
    }
    clock = new_clock;

}

uint8_t EEPROM::read()
{
    printf("[DEV9] [EEPROM] Read PIO_DATA %02x\n", data);
    return data;
}
