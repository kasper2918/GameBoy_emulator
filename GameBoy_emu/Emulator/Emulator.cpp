#include "Emulator.h"
#include <iostream>
#include <array>
#include <utility>

int Emulator::ExecuteNextOpcode()
{
    int cycles{};
    BYTE opcode = m_Rom[m_ProgramCounter];

    if ((m_ProgramCounter >= 0x4000 && m_ProgramCounter <= 0x7FFF) || (m_ProgramCounter >= 0xA000 && m_ProgramCounter <= 0xBFFF))
        opcode = ReadMemory(m_ProgramCounter);

    if (!m_Halted)
    {
        m_ProgramCounter++;
        cycles = ExecuteOpcode(opcode);
    }
    else
    {
        cycles = 4;
    }

    // we are trying to disable interupts, however
    // interupts get disabled after the next instruction
    // 0xF3 is the opcode for disabling interupt
    if (m_PendingInteruptDisabled)
    {
        if (ReadMemory(m_ProgramCounter - 1) != 0xF3)
        {
            m_PendingInteruptDisabled = false;
            m_InteruptMaster = false;
        }
    }

    if (m_PendingInteruptEnabled)
    {
        if (ReadMemory(m_ProgramCounter - 1) != 0xFB)
        {
            m_PendingInteruptEnabled = false;
            m_InteruptMaster = true;
        }
    }

    return cycles;
}

WORD unsigned16(BYTE lsb, BYTE msb)
{
    return (msb << 8) | lsb;
}

int Emulator::ExecuteOpcode(BYTE opcode)
{
    // bit masks
    int m0{ 0b0000'0001 };
    int m1{ 0b0000'0010 };
    int m2{ 0b0000'0100 };
    int m3{ 0b0000'1000 };
    int m4{ 0b0001'0000 };
    int m5{ 0b0010'0000 };
    int m6{ 0b0100'0000 };
    int m7{ 0b1000'0000 };

    int m543{ (opcode & (m5 | m4 | m3)) >> 3 };
    int m210{ opcode & (m2 | m1 | m0) };

    BYTE& xxx{ m_Regs[m543].get() };
    BYTE& yyy{ m_Regs[m210].get() };

    WORD& xx{ m_Regs16[(opcode & (m5 | m4)) >> 4].get() };

    switch ((opcode & (m7 | m6)) >> 6)
    {
    case 0b00:
    {
        switch (opcode)
        {
            // 8 bit indirect loads (0b00)
        // LD [HL], n8: 0x36
        case 0x36:
        {
            BYTE n = ReadMemory(PC++);
            WriteMemory(HL, n);
        }
        return 12;

        // LD A, [BC]: 0x0A
        case 0x0A:
        {
            A = ReadMemory(BC);
        }
        return 8;

        // LD A, [DE]: 0x1A
        case 0x1A:
        {
            A = ReadMemory(DE);
        }
        return 8;

        // LD [BC], A: 0x02
        case 0x02:
        {
            WriteMemory(BC, A);
        }
        return 8;

        // LD [DE], A: 0x12
        case 0x12:
        {
            WriteMemory(DE, A);
        }
        return 8;

        // LD A, [HL-]: 0x3A
        case 0x3A:
        {
            A = ReadMemory(HL--);
        }
        return 8;

        // LD [HL-], A: 0x32
        case 0x32:
        {
            WriteMemory(HL--, A);
        }
        return 8;

        // LD A, [HL+]: 0x2A
        case 0x2A:
        {
            A = ReadMemory(HL++);
        }
        return 8;

        // LD [HL+], A: 0x22
        case 0x22:
        {
            WriteMemory(HL++, A);
        }
        return 8;

        // 16 bit load
        // LD [n16], SP: 0x08
        case 0x08:
        {
            WORD nn{ get_nn() };
            WriteMemory(nn++, m_StackPointer.lo);
            WriteMemory(nn, m_StackPointer.hi);
        }
        return 20;
        }

        // LD r8, n8: 0b00'xxx'110
        if (m210 == 0b110)
        {
            CPU_8BIT_LOAD(xxx);
            return 8;
        }

        // LD r16, n16: 0b00'xx'0001
        if ((opcode & (m3 | m2 | m1 | m0)) == 1)
        {
            xx = get_nn();
            return 12;
        }
    }
    break;

    case 0b01:
    {
        // LD [HL], r8: 0b01'110'xxx
        if (m543 == 0b110)
        {
            WriteMemory(HL, yyy);
            return 8;
        }

        // LD r8, [HL]: 0b01'xxx'110
        if (m210 == 0b110)
        {
            xxx = ReadMemory(HL);
            return 8;
        }

        // LD r8, r8: 0b01'xxx'yyy
        xxx = yyy;
        return 4;
    }
    break;

    case 0b11:
    {
        std::array<std::reference_wrapper<WORD>, 4> regs16Stack{
            BC,
            DE,
            HL,
            AF,
        };
        WORD& xxStack{ regs16Stack[(opcode & (m5 | m4)) >> 4].get() };

        switch (opcode)
        {
            // 8 bit indirect loads (0b11)
        // LD A, [n16]: 0xFA
        case 0xFA:
        {
            WORD nn = get_nn();
            A = ReadMemory(nn);
        }
        return 16;

        // LD [n16], A: 0xEA
        case 0xEA:
        {
            WORD nn = get_nn();
            WriteMemory(nn, A);
        }
        return 16;

        // LDH A, [C]: 0xF2
        case 0xF2:
        {
            A = ReadMemory(unsigned16(C, 0xFF));
        }
        return 8;

        // LDH [C], A: 0xE2
        case 0xE2:
        {
            WriteMemory(unsigned16(C, 0xFF), A);
        }
        return 8;

        // LDH A, [n16]: 0xF0
        case 0xF0:
        {
            BYTE n{ ReadMemory(PC++) };
            A = ReadMemory(unsigned16(n, 0xFF));
        }
        return 12;

        // LDH [n16], A: 0xE0
        case 0xE0:
        {
            BYTE n{ ReadMemory(PC++) };
            WriteMemory(unsigned16(n, 0xFF), A);
        }
        return 12;

        // 16 bit loads
        // LD SP, HL: 0xF9
        case 0xF9:
        {
            SP = HL;
        }
        return 8;

        // LD HL, SP+e8: 0xF8
        case 0xF8:
        {
            CPU_16BIT_LOAD();
        }
        return 12;
        }

        int last4{ opcode & (m3 | m2 | m1 | m0) };
        // PUSH r16: 0b11'xx'0101
        if (last4 == 0b0101)
        {
            PushWordOntoStack(xxStack);
            return 16;
        }

        // POP r16: 0b11'xx'0001
        if (last4 == 0b0001)
        {
            WORD word{ PopWordOffStack() };
            xxStack = word;
            return 12;
        }
    }
    break;

    case 0b10:
    {
        // ADD A, r8: 0x10'000'xxx
        if (m543 == 0b000)
        {

            return 4;
        }
    }
    break;
    }

    std::cerr << "Unknown opcode: "
        << std::hex << static_cast<int>(opcode) << '\n';
    assert(false);

    return -1;
}