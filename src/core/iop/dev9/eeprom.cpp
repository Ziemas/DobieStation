#include "eeprom.hpp"
#include <cstdio>
#include <array>

std::array<uint8_t, 64> eeprom = {
    //0x6D, 0x76, 0x63, 0x61, 0x31, 0x30, 0x08, 0x01,
    0x76, 0x6D, 0x61, 0x63, 0x30, 0x31, 0x07, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void EEPROM::set_dir(uint8_t value)
{
    printf("[DEV9] [EEPROM] write PIO_DIR: %02x\n", value);
    direction = (value >> 4) & 3;
    sequence = 0;
    address = 0;
    state = 0;
    clock = 0;
}

uint8_t EEPROM::get_dir()
{
    printf("[DEV9] [EEPROM] read PIO_DIR: %02x\n", direction);
    return direction;
}

void EEPROM::step()
{
    uint8_t line = (latch >> 5) & 1;
    printf("eeprom clock state: %d, command: %d, sequence %d\n", state, command, sequence);
    switch (state)
    {
        case READ_COMMAND:
            command |= line << (1 - sequence);
            sequence++;
            if (sequence == 2)
            {
                printf("eeprom command %d\n", command);
                sequence = 0;
                state = READ_ADDRESS;
            }
            break;
        case READ_ADDRESS:
            address |= line << (5 - sequence);
            // TODO
            //address = (address & (63 ^ (1 << sequence))) | ((line >> sequence) & (0x20 >> sequence));
            sequence++;
            if (sequence == 6)
            {
                printf("eeprom addr %x\n", address);
                state = TRANSMIT;
                sequence = 0;
            }
            break;
        case TRANSMIT:
            // TODO
            if (command == READ)
                latch = ((eeprom[address] << sequence) & 0x8000) >> 11;

            // TODO
            if (command == WRITE)
            {
                // eeprom[address] = (eeprom[address] & (63 ^ (1 << bit))) | ((bit) & (0x8000 >> bit));
            }

            sequence++;
            if (sequence == 16)
            {
                sequence = 0;
                address++;
                printf("eeprom transmit advanced address to [%x] value there is %02x\n", address, eeprom[address]);
            }
            break;
    }
    return;
}

void EEPROM::write(uint8_t value)
{
    printf("[DEV9] [EEPROM] Write PIO_DATA %02x\n", value);
    uint8_t new_clock = (value >> 6) & 1;

    latch = value;
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
    printf("[DEV9] [EEPROM] Read PIO_DATA %02x\n", latch);
    return latch;
}
