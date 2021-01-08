#ifndef __EEPROM_H_
#define __EEPROM_H_
#include <cstdint>

/*  EEPROM on the network adapter, stores the MAC address.
 *  This is a serial eeprom, you control it using the chip select,
 *  the clock, and the two data lines.
 **/

/* the 4 most significant bits of the pio data register represents:
 *         ____|___________|___________________________________|____________
 * enable /    |           |                                   |
 *          __ |   __   __ |   __    __    __    __    __   __ |  __    __
 * clock   /  \|__/  \_/  \|__/  \__/  \__/  \__/  \__/  \_/  \|_/  \__/  \
 *          ___|_____      |                                   |
 * din     /   |     \_____|___________________________________|____________
 *             |           |                                   |  _______
 * dout    ____|___________|___________________________________|_/       \__
 *             |           |                                   |
 * din       1 |  1    0   |   0     0     0     0     0    0  |   0    0
 * dout      0 |  0    0   |   0     0     0     0     0    0  |   1    1
 *        start     cmd                   addr                      data
 *
 *  1 bit to signal the start -> 2 bits of command -> 6 bits of address -> data
 *
 * */

class EEPROM
{
  private:
    static constexpr uint8_t PP_DOUT_SHIFT = 4; /* Data output */
    static constexpr uint8_t PP_DIN_SHIFT = 5;  /* Data input  */
    static constexpr uint8_t PP_SCLK_SHIFT = 6; /* Clock       */
    static constexpr uint8_t PP_CSEL_SHIFT = 7; /* Chip select */

    static constexpr uint8_t PP_OP_READ = 2;  /* 2b'10 */
    static constexpr uint8_t PP_OP_WRITE = 1; /* 2b'01 */

    // Write enable and disable, presumably controlled by using the address bits.
    static constexpr uint8_t PP_OP_EWEN = 0; /* 2b'00 */
    static constexpr uint8_t PP_OP_EWDS = 0; /* 2b'00 */

    enum class State
    {
        cmd_start,
        read_cmd,
        read_address,
        transmit,
    };

    // Pins
    uint8_t m_clock = 0;
    uint8_t m_data = 0;

    // Internal state
    State m_state = State::cmd_start;
    uint8_t m_command = 0;
    uint8_t m_sequence = 0;
    uint8_t m_address = 0;
    uint16_t* m_eeprom;

    void step();

  public:
    EEPROM(uint16_t* eeprom);
    void write(uint8_t cmd);
    uint8_t read();
};

#endif // __EEPROM_H_
