#include "Emulator.h"
#include "Misc/BitOps.h"

void Emulator::UpdateTimers(int cycles)
{
    DoDividerRegister(cycles);

    // the clock must be enabled to update the clock
    if (IsClockEnabled())
    {
        m_TimerCounter -= cycles;

        // enough cpu clock cycles have happened to update the timer
        if (m_TimerCounter <= 0)
        {
            // reset m_TimerTracer to the correct value
            SetClockFreq();

            // timer about to overflow
            if (ReadMemory(TIMA) == 255)
            {
                WriteMemory(TIMA, ReadMemory(TMA));
                RequestInterupt(2);
            }
            else
            {
                WriteMemory(TIMA, ReadMemory(TIMA) + 1);
            }
        }
    }
}

bool Emulator::IsClockEnabled() const
{
	return TestBit(ReadMemory(TMC), 2) ? true : false;
}

BYTE Emulator::GetClockFreq()const
{
	return ReadMemory(TMC) & 0x3;
}

void Emulator::SetClockFreq()
{
    BYTE freq = GetClockFreq();
    switch (freq)
    {
    case 0: m_TimerCounter = 1024; break; // freq 4096
    case 1: m_TimerCounter = 16; break;// freq 262144
    case 2: m_TimerCounter = 64; break;// freq 65536
    case 3: m_TimerCounter = 256; break;// freq 16382
    }
}

void Emulator::DoDividerRegister(int cycles)
{
    m_DividerCounter += cycles;
    if (m_DividerCounter >= 255)
    {
        m_DividerCounter = 0;
        m_Rom[0xFF04]++;
    }
}