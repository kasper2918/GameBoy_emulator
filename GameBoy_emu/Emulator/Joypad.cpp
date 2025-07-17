#include "Emulator.h"
#include "Misc/BitOps.h"

BYTE Emulator::GetJoypadState() const
{
    BYTE res = m_Rom[0xFF00];
    // flip all the bits
    res ^= 0xFF;

    // are we interested in the standard buttons?
    if (!TestBit(res, 4))
    {
        BYTE topJoypad = m_JoypadState >> 4;
        topJoypad |= 0xF0; // turn the top 4 bits on
        res &= topJoypad; // show what buttons are pressed
    }
    else if (!TestBit(res, 5))//directional buttons
    {
        BYTE bottomJoypad = m_JoypadState & 0xF;
        bottomJoypad |= 0xF0;
        res &= bottomJoypad;
    }
    return res;
}