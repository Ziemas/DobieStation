#ifndef __EEPROM_H_
#define __EEPROM_H_
#include <cstdint>

/*  EEPROM on network adapter, stores the MAC address.
 *  This is a serial eeprom, you control it using the enable
 *  the clock and a single data line
 *
 *  Doesn't seem to be one of the common protocols.
 *  */

/* the 3 most significant bits of the rigest represents:
 *         __________________________________________________________
 * enable /                                                          \
 *          __    __    __    __    __    __    __    __    __    __
 * clock   /  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \__/  \
 *          _____    ______
 * data    /     \__/      \___________
 *          1     0     1     0
 *
 *        ... you get the idea, but maybe not exactly like this sketch
 *
 *        2 command bits bits -> 6 address bits -> one bit skipped -> data
 *
 *        once the address has been set you can fire away,
 *        the address automatically increments
 *
 *   */

/*  There seems to be some xor action going on in the dev9ghz implementation
 *  I don't understand it so tried to copy the functionality while
 *  making the implementation more sane, we'll se if this works at all later */

class EEPROM
{
    private:
        enum STATE
        {
            READ_COMMAND = 0,
            READ_ADDRESS,
            ACK,
            TRANSMIT,
        };

        enum COMMAND
        {
            unk1 = 0,
            unk2 = 1,
            WRITE = 2,
            READ = 3,
        };

        // Pins
        uint8_t clock = 0;
        uint8_t data = 0;

        // Internal state
        uint8_t state = READ_COMMAND;
        uint8_t command = 0;
        uint8_t sequence = 0;
        uint8_t address = 0;

        // This is probably not part of the eeprom?
        uint8_t pio_dir = 0;

        void step();
    public:
        void set_dir(uint8_t direction);
        uint8_t get_dir();
        void write(uint8_t cmd);
        uint8_t read();

};



#endif // __EEPROM_H_
