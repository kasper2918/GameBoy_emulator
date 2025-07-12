#include "Emulator.h"

void Emulator::key_pressed(int key) {
    // if setting from 1 to 0 we may have to request an interupt
    bool previously_unset{ !(m_joypad_state & (1 << key)) }; // possible bug

    // remember if a key pressed its bit is 0 not 1
    m_joypad_state &= ~(1 << key);

    // button pressed
    bool button = true;

    // is this a standard button or a directional button?
    if (key > 3)
        button = true;
    else // directional button pressed
        button = false;

    BYTE key_req = m_rom[0xFF00];
    bool requestInterupt = false;

    // only request interupt if the button just pressed is
    // the style of button the game is interested in
    if ( (button && !(key_req & (1 << 5))) 
        || (!button && !(key_req & (1 << 4))) )
        requestInterupt = true;

    // request interupt
    if (requestInterupt && !previously_unset)
        request_interupt(4);
}

void Emulator::key_released(int key) {
    m_joypad_state |= 1 << key;
}

BYTE Emulator::get_joypad_state() const {
    BYTE res = m_rom[0xFF00];
    // flip all the bits
    res ^= 0xFF;

    // are we interested in the standard buttons?
    if (!(res & (1 << 4)))
    {
        BYTE top_joypad = m_joypad_state >> 4;
        top_joypad |= 0xF0; // turn the top 4 bits on
        res &= top_joypad; // show what buttons are pressed
    }
    else if (!(res & (1 << 5)))//directional buttons
    {
        BYTE bottom_joypad = m_joypad_state & 0xF;
        bottom_joypad |= 0xF0;
        res &= bottom_joypad;
    }
    return res;
}