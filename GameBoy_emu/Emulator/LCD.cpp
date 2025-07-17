#include "Emulator.h"
#include "Misc/BitOps.h"

void Emulator::SetLCDStatus()
{
    BYTE status = ReadMemory(0xFF41);
    if (false == IsLCDEnabled())
    {
        // set the mode to 1 during lcd disabled and reset scanline
        m_ScanlineCounter = 456;
        m_Rom[0xFF44] = 0;
        status &= 252;
        status = BitSet(status, 0);
        WriteMemory(0xFF41, status);
        return;
    }

    BYTE currentline = ReadMemory(0xFF44);
    BYTE currentmode = status & 0x3;

    BYTE mode = 0;
    bool reqInt = false;

    // in vblank so set mode to 1
    if (currentline >= 144)
    {
        mode = 1;
        status = BitSet(status, 0);
        status = BitReset(status, 1);
        reqInt = TestBit(status, 4);
    }
    else
    {
        int mode2bounds = 456 - 80;
        int mode3bounds = mode2bounds - 172;

        // mode 2
        if (m_ScanlineCounter >= mode2bounds)
        {
            mode = 2;
            status = BitSet(status, 1);
            status = BitReset(status, 0);
            reqInt = TestBit(status, 5);
        }
        // mode 3
        else if (m_ScanlineCounter >= mode3bounds)
        {
            mode = 3;
            status = BitSet(status, 1);
            status = BitSet(status, 0);
        }
        // mode 0
        else
        {
            mode = 0;
            status = BitReset(status, 1);
            status = BitReset(status, 0);
            reqInt = TestBit(status, 3);
        }
    }

    // just entered a new mode so request interupt
    if (reqInt && (mode != currentmode))
        RequestInterupt(1);

    // check the conincidence flag
    if (currentline == ReadMemory(0xFF45))
    {
        status = BitSet(status, 2);
        if (TestBit(status, 6))
            RequestInterupt(1);
    }
    else
    {
        status = BitReset(status, 2);
    }
    WriteMemory(0xFF41, status);
}

bool Emulator::IsLCDEnabled() const
{
	return TestBit(ReadMemory(0xFF40), 7);
}