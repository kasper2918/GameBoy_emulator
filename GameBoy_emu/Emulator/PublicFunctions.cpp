#include "Emulator.h"
#include "Misc/BitOps.h"

#include <fstream>

void Emulator::Update()
{
    constexpr int MAXCYCLES{ 69905 * 4 };
    int cyclesThisUpdate = 0;

    while (cyclesThisUpdate < MAXCYCLES)
    {
        int cycles = ExecuteNextOpcode();
        cyclesThisUpdate += cycles;
        UpdateTimers(cycles);
        UpdateGraphics(cycles);
        DoInterupts();
    }
}

void Emulator::LoadGame(std::string_view path) 
{
    m_CartridgeMemory = std::make_unique<BYTE[]>(0x200000);
    
    int i{};
    for (std::ifstream file{ path.data(), std::ios::binary }; file.good();)
        m_CartridgeMemory[i++] = file.get();

    switch (m_CartridgeMemory[0x147])
    {
    case 1: m_MBC1 = true; break;
    case 2: m_MBC1 = true; break;
    case 3: m_MBC1 = true; break;
    case 5: m_MBC2 = true; break;
    case 6: m_MBC2 = true; break;
    default: break;
    }

    std::copy_n(m_CartridgeMemory.get(), 0x8000, m_Rom);

    gameLoadStatus = 1;
}

Emulator::Emulator() {
    m_ProgramCounter = 0x100;
    m_RegisterAF.reg = 0x01B0;
    m_RegisterBC.reg = 0x0013;
    m_RegisterDE.reg = 0x00D8;
    m_RegisterHL.reg = 0x014D;
    m_StackPointer.reg = 0xFFFE;
    m_Rom[0xFF05] = 0x00;
    m_Rom[0xFF06] = 0x00;
    m_Rom[0xFF07] = 0x00;
    m_Rom[0xFF10] = 0x80;
    m_Rom[0xFF11] = 0xBF;
    m_Rom[0xFF12] = 0xF3;
    m_Rom[0xFF14] = 0xBF;
    m_Rom[0xFF16] = 0x3F;
    m_Rom[0xFF17] = 0x00;
    m_Rom[0xFF19] = 0xBF;
    m_Rom[0xFF1A] = 0x7F;
    m_Rom[0xFF1B] = 0xFF;
    m_Rom[0xFF1C] = 0x9F;
    m_Rom[0xFF1E] = 0xBF;
    m_Rom[0xFF20] = 0xFF;
    m_Rom[0xFF21] = 0x00;
    m_Rom[0xFF22] = 0x00;
    m_Rom[0xFF23] = 0xBF;
    m_Rom[0xFF24] = 0x77;
    m_Rom[0xFF25] = 0xF3;
    m_Rom[0xFF26] = 0xF1;
    m_Rom[0xFF40] = 0x91;
    m_Rom[0xFF42] = 0x00;
    m_Rom[0xFF43] = 0x00;
    m_Rom[0xFF45] = 0x00;
    m_Rom[0xFF47] = 0xFC;
    m_Rom[0xFF48] = 0xFF;
    m_Rom[0xFF49] = 0xFF;
    m_Rom[0xFF4A] = 0x00;
    m_Rom[0xFF4B] = 0x00;
    m_Rom[0xFFFF] = 0x00;
}

// I'm too lazy to refactor this
void Emulator::KeyPressed(int key)
{
    bool previouslyUnset = false;

    // if setting from 1 to 0 we may have to request an interupt
    if (TestBit(m_JoypadState, key) == false)
        previouslyUnset = true;

    // remember if a keypressed its bit is 0 not 1
    m_JoypadState = BitReset(m_JoypadState, key);

    // button pressed
    bool button = true;

    // is this a standard button or a directional button?
    if (key > 3)
        button = true;
    else // directional button pressed
        button = false;

    BYTE keyReq = m_Rom[0xFF00];
    bool requestInterupt = false;

    // only request interupt if the button just pressed is
    // the style of button the game is interested in
    if (button && !TestBit(keyReq, 5))
        requestInterupt = true;

    // same as above but for directional button
    else if (!button && !TestBit(keyReq, 4))
        requestInterupt = true;

    // request interupt
    if (requestInterupt && !previouslyUnset)
        RequestInterupt(4);
}

void Emulator::KeyReleased(int key)
{
    m_JoypadState = BitSet(m_JoypadState, key);
}