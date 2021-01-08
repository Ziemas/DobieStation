#include "eeprom.hpp"
#include <array>
#include <cstdio>

// TODO: lets just take a MAC address string and generate the eeprom content
// it's just the bytes of the address and a simple checksum (adding 3 shorts together)

// clang-format off
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
// clang-format on

EEPROM::EEPROM(uint16_t* eeprom_data) : m_eeprom(eeprom_data)
{
    if (m_eeprom == nullptr)
    {
        m_eeprom = hardcoded_eeprom;
    }
}

void EEPROM::step()
{
    uint8_t line = (m_data >> PP_DIN_SHIFT) & 1;

    switch (m_state)
    {
        case State::cmd_start: // wait for data to go high
            m_data = 0;
            m_sequence = 0;
            if (line)
                m_state = State::read_cmd;
            break;
        case State::read_cmd: // read two bits of command
            m_command |= line << (1 - m_sequence);
            m_sequence++;
            if (m_sequence == 2)
            {
                m_sequence = 0;
                m_state = State::read_address;
            }
            break;
        case State::read_address: // read 6 bits of address
            // Address read in from highest bit to lowest bit
            m_address |= line << (5 - m_sequence);

            m_sequence++;
            if (m_sequence == 6)
            {
                m_state = State::transmit;
                m_sequence = 0;
            }
            break;
        case State::transmit: // fire away, bit position increments every pulse
            if (m_command == PP_OP_READ)
                m_data = ((m_eeprom[m_address] >> (15 - m_sequence) ) & 1) << PP_DOUT_SHIFT;

            if (m_command == PP_OP_WRITE) // TODO: untested
                m_eeprom[m_address] = m_eeprom[m_address] | (line << (15 - m_sequence));

            m_sequence++;
            if (m_sequence == 16)
            {
                m_sequence = 0;
                m_address++;
            }
            break;
    }
    return;
}

void EEPROM::write(uint8_t value)
{
    printf("[DEV9] [EEPROM] Write PIO_DATA %02x\n", value);
    uint8_t cselect = (value >> PP_CSEL_SHIFT) & 1;
    uint8_t new_clock = (value >> PP_SCLK_SHIFT) & 1;

    if (cselect == 0)
    {
        m_sequence = 0;
        m_address = 0;
        m_state = State::cmd_start;
        m_clock = 0;
        return;
    }

    m_data = value;
    if (new_clock == 1)
    {
        // The eeprom works on the rising clock edge
        if (m_clock == 0)
        {
            step();
        }
    }
    m_clock = new_clock;
}

uint8_t EEPROM::read()
{
    printf("[DEV9] [EEPROM] Read PIO_DATA %02x\n", m_data);
    return m_data;
}
