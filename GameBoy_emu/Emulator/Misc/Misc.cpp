#include "../Emulator.h"

void Emulator::DoDMATransfer(BYTE data)
{
    WORD address = data << 8; // source address is data * 100
    for (int i = 0; i < 0xA0; i++)
    {
        WriteMemory(0xFE00 + i, ReadMemory(address + i));
    }
}

WORD Emulator::get_nn()
{
    WORD nn = ReadMemory(PC++);
    nn |= ReadMemory(PC++) << 8;
    return nn;
}