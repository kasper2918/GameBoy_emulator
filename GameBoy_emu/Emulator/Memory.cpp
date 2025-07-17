#include "Emulator.h"
#include "Misc/BitOps.h"

void Emulator::WriteMemory(WORD address, BYTE data)
{
    // dont allow any writing to the read only memory
    if (address < 0x8000)
    {
        HandleBanking(address, data);
    }

    else if ((address >= 0xA000) && (address < 0xC000))
    {
        if (m_EnableRAM)
        {
            WORD newAddress = address - 0xA000;
            m_RAMBanks[newAddress + (m_CurrentRAMBank * 0x2000)] = data;
        }
    }

    // writing to ECHO ram also writes in RAM
    else if ((address >= 0xE000) && (address < 0xFE00))
    {
        m_Rom[address] = data;
        WriteMemory(address - 0x2000, data);
    }

    // this area is restricted
    else if ((address >= 0xFEA0) && (address < 0xFEFF))
    {
        return;
    }

    //trap the divider register
    else if (0xFF04 == address)
        m_Rom[0xFF04] = 0;

    else if (TMC == address)
    {
        BYTE currentfreq = GetClockFreq();
        m_Rom[TMC] = data;
        BYTE newfreq = GetClockFreq();

        if (currentfreq != newfreq)
        {
            SetClockFreq();
        }
    }

    // reset the current scanline if the game tries to write to it
    else if (address == 0xFF44)
    {
        m_Rom[address] = 0;
    }

    else if (address == 0xFF46)
    {
        DoDMATransfer(data);
    }

    // no control needed over this area so write to memory
    else
    {
        m_Rom[address] = data;
    }
}

BYTE Emulator::ReadMemory(WORD address) const
{
    // are we reading from the rom memory bank?
    if ((address >= 0x4000) && (address <= 0x7FFF))
    {
        WORD newAddress = address - 0x4000;
        return m_CartridgeMemory[newAddress + (m_CurrentROMBank * 0x4000)];
    }

    // are we reading from ram memory bank?
    else if ((address >= 0xA000) && (address <= 0xBFFF))
    {
        WORD newAddress = address - 0xA000;
        return m_RAMBanks[newAddress + (m_CurrentRAMBank * 0x2000)];
    }

    else if (0xFF00 == address)
        return GetJoypadState();

    // else return memory
    return m_Rom[address];
}

void Emulator::HandleBanking(WORD address, BYTE data) 
{
    // do RAM enabling
    if (address < 0x2000)
    {
        if (m_MBC1 || m_MBC2)
        {
            DoRAMBankEnable(address, data);
        }
    }

    // do ROM bank change
    else if ((address >= 0x200) && (address < 0x4000))
    {
        if (m_MBC1 || m_MBC2)
        {
            DoChangeLoROMBank(data);
        }
    }

    // do ROM or RAM bank change
    else if ((address >= 0x4000) && (address < 0x6000))
    {
        // there is no rambank in mbc2 so always use rambank 0
        if (m_MBC1)
        {
            if (m_RomBanking)
            {
                DoChangeHiRomBank(data);
            }
            else
            {
                DoRAMBankChange(data);
            }
        }
    }

    // this will change whether we are doing ROM banking
    // or RAM banking with the above if statement
    else if ((address >= 0x6000) && (address < 0x8000))
    {
        if (m_MBC1)
            DoChangeROMRAMMode(data);
    }
}

void Emulator::DoRAMBankEnable(WORD address, BYTE data)
{
    if (m_MBC2)
    {
        if (TestBit(address, 4) == 1) return;
    }

    BYTE testData = data & 0xF;
    if (testData == 0xA)
        m_EnableRAM = true;
    else if (testData == 0x0)
        m_EnableRAM = false;
}

void Emulator::DoChangeLoROMBank(BYTE data)
{
    if (m_MBC2)
    {
        m_CurrentROMBank = data & 0xF;
        if (m_CurrentROMBank == 0) m_CurrentROMBank++;
        return;
    }

    BYTE lower5 = data & 31;
    m_CurrentROMBank &= 224; // turn off the lower 5
    m_CurrentROMBank |= lower5;
    if (m_CurrentROMBank == 0) m_CurrentROMBank++;
}

void Emulator::DoChangeHiRomBank(BYTE data)
{
    // turn off the upper 3 bits of the current rom
    m_CurrentROMBank &= 31;

    // turn off the lower 5 bits of the data
    data &= 224;
    m_CurrentROMBank |= data;
    if (m_CurrentROMBank == 0) m_CurrentROMBank++;
}

void Emulator::DoRAMBankChange(BYTE data)
{
    m_CurrentRAMBank = data & 0x3;
}

void Emulator::DoChangeROMRAMMode(BYTE data)
{
    BYTE newData = data & 0x1;
    m_RomBanking = (newData == 0) ? true : false;
    if (m_RomBanking)
        m_CurrentRAMBank = 0;
}