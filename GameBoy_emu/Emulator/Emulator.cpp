#include "Emulator.h"
#include "Misc/BitOps.h"

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
#ifndef NDEBUG
        //std::cout << std::hex << PC << ": " << static_cast<int>(opcode) << '\n';
#endif // !NDEBUG

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
        case 0x00:
        {
            return 4;
        }
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
        // 
        // LD [n16], SP: 0x08
        case 0x08:
        {
            WORD nn{ get_nn() };
            WriteMemory(nn++, m_StackPointer.lo);
            WriteMemory(nn, m_StackPointer.hi);
        }
        return 20;

        // 8 bit indirect increment/decrement
        //
        // INC [HL]: 0x34
        case 0x34:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_INC(data);
            WriteMemory(HL, data);
        }
        return 12;

        // DEC [HL]: 0x35
        case 0x35:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_DEC(data);
            WriteMemory(HL, data);
        }
        return 12;

        // Carry Flag (CF)
        //
        // CCF: 0x3F
        case 0x3F:
        {
            F = BitReset(F, FLAG_N);
            F = BitReset(F, FLAG_H);

            if (BitGetVal(F, FLAG_C))
                F = BitReset(F, FLAG_C);
            else
                F = BitSet(F, FLAG_C);
        }
        return 4;

        // SCF: 0x37
        case 0x37:
        {
            F = BitReset(F, FLAG_N);
            F = BitReset(F, FLAG_H);

            F = BitSet(F, FLAG_C);
        }
        return 4;

        // Decimal Adjust Accumulator
        //
        // DAA: 0x27
        case 0x27:
        {
            CPU_DAA();
        }
        return 4;

        // Complement accumulator (A register)
        //
        // CPL: 0x2F
        case 0x2F:
        {
            A = ~A;
            F = BitSet(F, FLAG_N);
            F = BitSet(F, FLAG_H);
        }
        return 4;

        // Rotates
        //
        // RLCA: 0x07
        case 0x07:
        {
            CPU_RLC(A, true);
        }
        return 4;

        // RRCA: 0x0F
        case 0x0F:
        {
            CPU_RRC(A, true);
        }
        return 4;

        // RLA: 0x17
        case 0x17:
        {
            CPU_RL(A, true);
        }
        return 4;

        // RRA: 0x1F
        case 0x1F:
        {
            CPU_RR(A, true);
        }
        return 4;

        // Unconditional relative jump
        // JR n16: 0x18
        case 0x18:
        {
            CPU_JUMP_IMMEDIATE(false, 0, false);
        }
        return 8;

        // Conditional relative jump
        // JR cc, n16: 0b001'xx'000
        case 0x20:
        {
            CPU_JUMP_IMMEDIATE(true, FLAG_Z, false);
        }
        return 8;

        case 0x28:
        {
            CPU_JUMP_IMMEDIATE(true, FLAG_Z, true);
        }
        return 8;

        case 0x30:
        {
            CPU_JUMP_IMMEDIATE(true, FLAG_C, false);
        }
        return 8;

        case 0x38:
        {
            CPU_JUMP_IMMEDIATE(true, FLAG_C, true);
        }
        return 8;

        // STOP: 0x10
        case 0x10:
        {
            ++m_ProgramCounter;
        }
        return 4;
        }

        int last4{ opcode & (m3 | m2 | m1 | m0) };
        switch (last4)
        {
            // LD r16, n16: 0b00'xx'0001
        case 0b0001:
        {
            xx = get_nn();
        }
        return 12;

        // INC r16: 0b00'xx'0011
        case 0b0011:
        {
            ++xx;
        }
        return 8;

        // DEC r16: 0b00'xx'1011
        case 0b1011:
        {
            --xx;
        }
        return 8;

        // ADD HL, r16: 0b00'xx'1001
        case 0b1001:
        {
            CPU_16BIT_ADD(HL, xx);
        }
        return 8;
        }

        switch (m210)
        {
            // INC r8: 0b00'xxx'100
        case 0b100:
        {
            CPU_8BIT_INC(xxx);
        }
        return 4;

        // DEC r8: 0b00'xxx'101
        case 0b101:
        {
            CPU_8BIT_DEC(xxx);
        }
        return 4;

        // LD r8, n8: 0b00'xxx'110
        case 0b110:
        {
            CPU_8BIT_LOAD(xxx);
        }
        return 8;
        }
    }
    break;

    case 0b01:
    {
        // HALT: 0x76
        if (opcode == 0x76)
        {
            m_Halted = true;
            return 4;
        }

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
            // 
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
        // 
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

        // 8 bit arithmetic
        // 
        // ADD A, n8: 0xC6
        case 0xC6:
        {
            CPU_8BIT_ADD(A, 0, true, false);
        }
        return 8;

        // ADC A, n8: 0xCE
        case 0xCE:
        {
            CPU_8BIT_ADD(A, 0, true, true);
        }
        return 8;

        // SUB A, n8: 0xD6
        case 0xD6:
        {
            CPU_8BIT_SUB(A, 0, true, false);
        }
        return 8;

        // SBC A, n8: 0xDE
        case 0xDE:
        {
            CPU_8BIT_SUB(A, 0, true, true);
        }
        return 8;

        // CP A, n8: 0xFE
        case 0xFE:
        {
            CPU_8BIT_CP(A, 0, true);
        }
        return 8;

        // 8 bit logic
        // AND A, n8: 0xE6
        case 0xE6:
        {
            CPU_8BIT_AND(A, 0, true);
        }
        return 8;

        // OR A, n8: 0xF6
        case 0xF6:
        {
            CPU_8BIT_OR(A, 0, true);
        }
        return 8;

        // XOR A, n8: 0xEE
        case 0xEE:
        {
            CPU_8BIT_XOR(A, 0, true);
        }
        return 8;

        // 16 bit arithmetic
        // ADD SP, e8: 0xE8
        case 0xE8:
        {
            CPU_16BIT_ADD_SP();
        }
        return 16;

        // Extended opcode (0xCB)
        case 0xCB:
        {
            return ExecuteExtendedOpcode();
        }

        // Unconditional jumps
        // JP n16: 0xC3
        case 0xC3:
        {
            PC = get_nn();
        }
        return 12;

        // JP HL: 0xE9
        case 0xE9:
        {
            PC = HL;
        }
        return 4;

        // Conditional jumps
        // JP cc, n16: 0b110'xx'010
        case 0xC2:
        {
            CPU_JUMP(true, FLAG_Z, false);
        }
        return 12;

        case 0xCA:
        {
            CPU_JUMP(true, FLAG_Z, true);
        }
        return 12;

        case 0xD2:
        {
            CPU_JUMP(true, FLAG_C, false);
        }
        return 12;

        case 0xDA:
        {
            CPU_JUMP(true, FLAG_C, true);
        }
        return 12;

        // Unconditional call
        // CALL n16: 0xCD
        case 0xCD:
        {
            CPU_CALL(false, 0, false);
        }
        return 12;

        // Conditional calls
        // CALL cc, n16: 0b110'xx'100
        case 0xC4:
        {
            CPU_CALL(true, FLAG_Z, false);
        }
        return 12;

        case 0xCC:
        {
            CPU_CALL(true, FLAG_Z, true);
        }
        return 12;

        case 0xD4:
        {
            CPU_CALL(true, FLAG_C, false);
        }
        return 12;

        case 0xDC:
        {
            CPU_CALL(true, FLAG_C, true);
        }
        return 12;

        // Unconditional return
        // RET: 0xC9
        case 0xC9:
        {
            CPU_RETURN(false, 0, false);
        }
        return 8;

        // Conditional return
        // RET cc: 0b110xx000
        case 0xC0:
        {
            CPU_RETURN(true, FLAG_Z, false);
        }
        return 8;

        case 0xC8:
        {
            CPU_RETURN(true, FLAG_Z, true);
        }
        return 8;

        case 0xD0:
        {
            CPU_RETURN(true, FLAG_C, false);
        }
        return 8;

        case 0xD8:
        {
            CPU_RETURN(true, FLAG_C, true);
        }
        return 8;

        // Return from interrupt handler
        // RETI: 0xD9
        case 0xD9:
        {
            CPU_RETURN(false, 0, false);
            m_InteruptMaster = true;
        }
        return 8;

        // Interrupts
        // DI: 0xF3
        case 0xF3:
        {
            m_PendingInteruptDisabled = true;
        }
        return 4;

        // EI: 0xFB
        case 0xFB:
        {
            m_PendingInteruptEnabled = true;
        }
        return 4;
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

        // RST vec: 0b11'xxx'111
        if (m210 == 0b111)
        {
            std::array<BYTE, 8> vecs{ 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38 };
            PushWordOntoStack(PC);
            PC = vecs[m543];
            return 32;
        }
    }
    break;

    case 0b10:
    {
        switch (opcode)
        {
            // 8 bit arithmetic (0b10)
        // ADD A, [HL]: 0x86
        case 0x86:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_ADD(A, data, false, false);
        }
        return 8;

        // ADC A, [HL]: 0x8E
        case 0x8E:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_ADD(A, data, false, true);
        }
        return 8;

        // SUB A, [HL]: 0x96
        case 0x96:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_SUB(A, data, false, false);
        }
        return 8;

        // SBC A, [HL]: 0x9E
        case 0x9E:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_SUB(A, data, false, true);
        }
        return 8;

        // CP A, [HL]: 0xBE
        case 0xBE:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_CP(A, data, false);
        }
        return 8;

        // 8 bit logic (0b10)
        // AND A, [HL]: 0xA6
        case 0xA6:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_AND(A, data, false);
        }
        return 8;

        // OR A, [HL]: 0xB6
        case 0xB6:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_OR(A, data, false);
        }
        return 8;

        // XOR A, [HL]: 0xAE
        case 0xAE:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_8BIT_XOR(A, data, false);
        }
        return 8;
        }

        switch (m543)
        {
            // 8 bit arithmetic (0b10)
            // 
        // ADD A, r8: 0b10'000'xxx
        case 0b000:
        {
            CPU_8BIT_ADD(A, yyy, false, false);
        }
        return 4;

        // ADC A, r8: 0b10'001'xxx
        case 0b001:
        {
            CPU_8BIT_ADD(A, yyy, false, true);
        }
        return 4;

        // SUB A, r8: 0b10'010'xxx
        case 0b010:
        {
            CPU_8BIT_SUB(A, yyy, false, false);
        }
        return 4;

        // SBC A, r8: 0b10'011'xxx
        case 0b011:
        {
            CPU_8BIT_SUB(A, yyy, false, true);
        }
        return 4;

        // CP A, r8: 0b10'111'xxx
        case 0b111:
        {
            CPU_8BIT_CP(A, yyy, false);
        }
        return 4;

        // 8 bit logic
        //
        // AND A, r8: 0b10'100'xxx
        case 0b100:
        {
            CPU_8BIT_AND(A, yyy, false);
        }
        return 4;

        // OR A, r8: 0b10'110'xxx
        case 0b110:
        {
            CPU_8BIT_OR(A, yyy, false);
        }
        return 4;

        // XOR A, r8: 0b10'101'xxx
        case 0b101:
        {
            CPU_8BIT_XOR(A, yyy, false);
        }
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

int Emulator::ExecuteExtendedOpcode()
{
    BYTE opcode{ ReadMemory(m_ProgramCounter++) };

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

    BYTE& yyy{ m_Regs[m210].get() };

    switch ((opcode & (m7 | m6)) >> 6)
    {
    case 0b00:
    {
        switch (opcode)
        {
            // Indirect rotates (0xCB)
        // RLC [HL]: CB + 0x06
        case 0x06:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_RLC(data, false);
            WriteMemory(HL, data);
        }
        return 16;

        // RRC [HL]: CB + 0x0E
        case 0x0E:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_RRC(data, false);
            WriteMemory(HL, data);
        }
        return 16;

        // RL [HL]: CB + 0x16
        case 0x16:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_RL(data, false);
            WriteMemory(HL, data);
        }
        return 16;

        // RR [HL]: CB + 0x1E
        case 0x1E:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_RR(data, false);
            WriteMemory(HL, data);
        }
        return 16;

        // SLA [HL]: CB + 0x26
        case 0x26:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_SLA(data);
            WriteMemory(HL, data);
        }
        return 16;

        // SRA [HL]: CB + 0x2E
        case 0x2E:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_SRA(data);
            WriteMemory(HL, data);
        }
        return 16;

        // SWAP [HL]: CB + 0x36
        case 0x36:
        {
            BYTE data{ ReadMemory(HL) };

            data = (data >> 4) | ((data << 4) & 0xFF);
            F = 0;
            if (data == 0)
                F = BitSet(F, FLAG_Z);

            WriteMemory(HL, data);
        }
        return 16;

        // SRL [HL]: CB + 0x3E
        case 0x3E:
        {
            BYTE data{ ReadMemory(HL) };
            CPU_SRL(data);
            WriteMemory(HL, data);
        }
        return 16;
        }

        switch (m543)
        {
            // Rotates (0xCB)
        // RLC r8: CB + 0b00'000'xxx
        case 0b000:
        {
            CPU_RLC(yyy, false);
        }
        return 8;

        // RRC r8: CB + 0b00'001'xxx
        case 0b001:
        {
            CPU_RRC(yyy, false);
        }
        return 8;

        // RL r8: CB + 0b00'010'xxx
        case 0b010:
        {
            CPU_RL(yyy, false);
        }
        return 8;

        // RR r8: CB + 0b00'011'xxx
        case 0b011:
        {
            CPU_RR(yyy, false);
        }
        return 8;

        // SLA r8: CB + 0b00'100'xxx
        case 0b100:
        {
            CPU_SLA(yyy);
        }
        return 8;

        // SRA r8: CB + 0b00'101'xxx
        case 0b101:
        {
            CPU_SRA(yyy);
        }
        return 8;

        // SWAP r8: CB + 0b00'110'xxx
        case 0b110:
        {
            yyy = (yyy >> 4) | ((yyy << 4) & 0xFF);
            F = 0;
            if (yyy == 0)
                F = BitSet(F, FLAG_Z);
        }
        return 8;

        // SRL r8: CB + 0b00'111'xxx
        case 0b111:
        {
            CPU_SRL(yyy);
        }
        return 8;
        }
    }
    break;

    case 0b01:
    {
        // BIT u3, [HL]: 0b01'xxx'110
        if (m210 == 0b110)
        {
            F = BitReset(F, FLAG_N);
            F = BitSet(F, FLAG_H);
            BYTE data{ ReadMemory(HL) };
            if (TestBit(data, m543))
                F = BitReset(F, FLAG_Z);
            else
                F = BitSet(F, FLAG_Z);

            return 16;
        }

        // BIT u3, r8: 0b01'xxx'xxx
        F = BitReset(F, FLAG_N);
        F = BitSet(F, FLAG_H);
        if (TestBit(yyy, m543))
            F = BitReset(F, FLAG_Z);
        else
            F = BitSet(F, FLAG_Z);

        return 8;
    }
    break;

    case 0b10:
    {
        // RES u3, [HL]: 0b10'xxx'110
        if (m210 == 0b110)
        {
            BYTE data{ ReadMemory(HL) };
            data = BitReset(data, m543);
            WriteMemory(HL, data);
            return 16;
        }

        // RES u3, r8: 0b10'xxx'xxx
        yyy = BitReset(yyy, m543);
        return 8;
    }
    break;

    case 0b11:
    {
        // SET u3, [HL]: 0b11'xxx'110
        if (m210 == 0b110)
        {
            BYTE data{ ReadMemory(HL) };
            data = BitSet(data, m543);
            WriteMemory(HL, data);
            return 16;
        }

        // SET u3, r8: 0b11'xxx'xxx
        yyy = BitSet(yyy, m543);
        return 8;
    }
    break;
    }

    std::cerr << "Unknown opcode: CB"
        << std::hex << static_cast<int>(opcode) << '\n';
    assert(false);

    return -1;
}