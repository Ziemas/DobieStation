#include "eeprom.hpp"
#include <cstdio>
#include <array>

//std::array<uint8_t, 64> eeprom = {
uint8_t hardcoded_eeprom[] = {
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

EEPROM::EEPROM(uint16_t* eeprom_data) : eeprom(eeprom_data)
{
    if (eeprom == nullptr)
    {
        eeprom = (uint16_t*)hardcoded_eeprom;
    }
}

void EEPROM::step()
{
    uint8_t line = (data >> 5) & 1;
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
            // TODO: need to use brain to figure out what the correct thing is
            //address = (address & (63 ^ (1 << sequence))) | ((line >> sequence) & (0x20 >> sequence));
            //dev9.eeprom_address =
            //    (dev9.eeprom_address & (63 ^ (1 << sequence))) |
            //    ((value >> sequence) & (0x20 >> sequence)));

            sequence++;
            if (sequence == 6)
            {
                printf("eeprom addr %x\n", address);
                state = ACK;
                sequence = 0;
            }
            break;
        case ACK:
            printf("eeprom ack\n");
            data = 0;
            sequence = 0;
            state = TRANSMIT;
            break;
        case TRANSMIT:
            if (command == READ)
                data = ((eeprom[address] << sequence) & 0x8000) >> 11;

            // TODO: figure this out
            if (command == WRITE)
            {
                //eeprom[address] = eeprom[address] | (line << (15 - sequence));
                //dev9.eeprom[dev9.eeprom_address] =
                //    (dev9.eeprom[dev9.eeprom_address] & (63 ^ (1 << dev9.eeprom_bit))) |
                //    ((value >> dev9.eeprom_bit) & (0x8000 >> dev9.eeprom_bit));
                //
                // eeprom[address] = (eeprom[address] & (63 ^ (1 << bit))) | ((bit) & (0x8000 >> bit));
            }

            sequence++;
            if (sequence == 16)
            {
                printf("eeprom transmit advanced addres to [%x] value read was %04x\n", address, eeprom[address]);
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
