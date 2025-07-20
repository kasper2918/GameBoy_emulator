#include <gtest/gtest.h>
#include <map>

#include "Emulator/Emulator.h"
#include "Emulator/Misc/BitOps.h"

#define MORE_DEBUG

struct EmulatorTest : public testing::Test 
{
	Emulator emu{};

	void SetUp() { }
	void TearDown() { }
};

TEST_F(EmulatorTest, Foo) 
{
	EXPECT_EQ(emu.A, 0x01);
	EXPECT_EQ(emu.F, 0xB0);
	EXPECT_EQ(emu.B, 0x00);
	EXPECT_EQ(emu.C, 0x13);
	EXPECT_EQ(emu.D, 0x00);
	EXPECT_EQ(emu.E, 0xD8);
	EXPECT_EQ(emu.H, 0x01);
	EXPECT_EQ(emu.L, 0x4D);
}

std::map<char, bool> GetFlags(const Emulator& emu)
{
	std::map<char, bool> ret{};

	ret['Z'] = emu.F & (1 << emu.FLAG_Z);
	ret['N'] = emu.F & (1 << emu.FLAG_N);
	ret['H'] = emu.F & (1 << emu.FLAG_H);
	ret['C'] = emu.F & (1 << emu.FLAG_C);

	return ret;
}

TEST_F(EmulatorTest, CPUTest)
{
	BYTE& A{ emu.A };
	BYTE& B{ emu.B };
	BYTE& C{ emu.C };
	BYTE& D{ emu.D };
	BYTE& E{ emu.E };
	BYTE& H{ emu.H };
	BYTE& L{ emu.L };
	BYTE& F{ emu.F };

	WORD& PC{ emu.m_ProgramCounter };
	WORD& SP{ emu.m_StackPointer.reg };

	WORD& AF{ emu.AF };
	WORD& BC{ emu.BC };
	WORD& DE{ emu.DE };
	WORD& HL{ emu.HL };

	BYTE* rom{ emu.m_Rom };
	
	//==============================================
	// 8 bit loads

	// LD r8, r8: 0b01'xxx'yyy
	B = 1;
	emu.ExecuteOpcode(0b01'000'000);
	EXPECT_EQ(B, 1);

	C = 2;
	emu.ExecuteOpcode(0b01'000'001);
	EXPECT_EQ(B, 2);

	H = 7;
	emu.ExecuteOpcode(0b01'000'100);
	EXPECT_EQ(B, 7);
	
	A = 9;
	emu.ExecuteOpcode(0b01'000'111);
	EXPECT_EQ(B, 9);

	// LD r8, n8: 0b00'xxx'110
	rom[PC] = 0x4D;
	emu.ExecuteOpcode(0b00'001'110);
	EXPECT_EQ(C, 0x4D);

	// LD [HL], r8: 0b01'110'xxx
	HL = 0xC000;
	B = 0x7D;
	emu.ExecuteOpcode(0b01'110'000);
	EXPECT_EQ(rom[0xC000], 0x7D);

	// LD r8, [HL]: 0b01'xxx'110
	emu.ExecuteOpcode(0b01'010'110);
	EXPECT_EQ(D, 0x7D);

	// LD [HL], n8: 0x36
	rom[PC] = 0xE7;
	emu.ExecuteOpcode(0x36);
	EXPECT_EQ(rom[0xC000], 0xE7);

	// LD A, [BC]: 0x0A
	BC = 0xC123;
	rom[BC] = 0xDE;
	emu.ExecuteOpcode(0x0A);
	EXPECT_EQ(A, 0xDE);

	// LD A, [DE]: 0x1A
	DE = 0xC123;
	rom[DE] = 0xF6;
	emu.ExecuteOpcode(0x1A);
	EXPECT_EQ(A, 0xF6);

	// LD [BC], A: 0x02
	A = 0x2D;
	emu.ExecuteOpcode(0x02);
	EXPECT_EQ(rom[BC], 0x2D);

	// LD [DE], A: 0x12
	A = 0x2D;
	emu.ExecuteOpcode(0x12);
	EXPECT_EQ(rom[DE], 0x2D);

	// LD A, [n16]: 0xFA
	rom[PC] = 0x23;
	rom[PC + 1] = 0xC1;
	rom[0xC123] = 0x3C;
	emu.ExecuteOpcode(0xFA);
	EXPECT_EQ(A, 0x3C);

	// LD [n16], A: 0xEA
	rom[PC] = 0x23;
	rom[PC + 1] = 0xC1;
	A = 0x1B;
	emu.ExecuteOpcode(0xEA);
	EXPECT_EQ(rom[0xC123], 0x1B);

	// LDH A, [C]: 0xF2
	C = 0x3A;
	rom[0xFF3A] = 0x36;
	emu.ExecuteOpcode(0xF2);
	EXPECT_EQ(A, 0x36);

	// LDH [C], A: 0xE2
	emu.ExecuteOpcode(0xE2);
	EXPECT_EQ(rom[0xFF3A], 0x36);

	// LDH A, [n16]: 0xF0
	rom[PC] = 0x23;
	rom[0xFF23] = 0x13;
	emu.ExecuteOpcode(0xF0);
	EXPECT_EQ(A, 0x13);

	// LDH [n16], A: 0xE0
	rom[PC] = 0x23;
	emu.ExecuteOpcode(0xE0);
	EXPECT_EQ(rom[0xFF23], 0x13);

	// LD A, [HL-]: 0x3A
	HL = 0xC123;
	rom[0xC123] = 0x15;
	emu.ExecuteOpcode(0x3A);
	EXPECT_EQ(A, 0x15);
	EXPECT_EQ(HL, 0xC122);

	// LD [HL-], A: 0x32
	HL = 0xC123;
	emu.ExecuteOpcode(0x32);
	EXPECT_EQ(rom[0xC123], 0x15);
	EXPECT_EQ(HL, 0xC122);

	// LD A, [HL+]: 0x2A
	HL = 0xC123;
	rom[0xC123] = 0x15;
	emu.ExecuteOpcode(0x2A);
	EXPECT_EQ(A, 0x15);
	EXPECT_EQ(HL, 0xC124);

	// LD [HL+], A: 0x22
	HL = 0xC123;
	emu.ExecuteOpcode(0x22);
	EXPECT_EQ(rom[0xC123], 0x15);
	EXPECT_EQ(HL, 0xC124);

	// LD r16, n16: 0b00'xx'0001
	rom[PC] = 0xDE;
	rom[PC + 1] = 0x73;
	emu.ExecuteOpcode(0b00'10'0001);
	EXPECT_EQ(HL, 0x73DE);

	// LD [n16], SP: 0x08
	rom[PC] = 0xA0;
	rom[PC + 1] = 0xC6;
	SP = 0xFFFE;
	emu.ExecuteOpcode(0x08);
	EXPECT_EQ(rom[0xC6A0], 0xFE);
	EXPECT_EQ(rom[0xC6A1], 0xFF);

	// LD SP, HL: 0xF9
	HL = 0xFF73;
	emu.ExecuteOpcode(0xF9);
	EXPECT_EQ(SP, 0xFF73);

	// PUSH r16: 0b11'xx'0101
	AF = 0x5AD2;
	emu.ExecuteOpcode(0b11'11'0101);
	EXPECT_EQ(rom[SP], 0xD2);
	EXPECT_EQ(rom[SP + 1], 0x5A);

	// POP r16: 0b11'xx'0001
	AF = 0x3211;
	emu.ExecuteOpcode(0b11'11'0001);
	EXPECT_EQ(AF, 0x5AD2);

	// LD HL, SP+e8: 0xF8
	SP = 0xFFFE;
	rom[PC] = 0xFE;
	emu.ExecuteOpcode(0xF8);
	EXPECT_EQ(HL, 0xFFFC);

	SP = 0xFF73;
	rom[PC] = 0x05;
	emu.ExecuteOpcode(0xF8);
	EXPECT_EQ(HL, 0xFF78);
	//||||||||||||||||||||||||||||||||||||||||||||||

	//==============================================
	// 8 bit arithmetic

	// ADD A, r8: 0x10'000'xxx
	A = 2;
	emu.ExecuteOpcode(0b10'000'111);
	EXPECT_EQ(A, 4);

	// ADD A, [HL]: 0x86
	HL = 0xCFB7;
	rom[HL] = 7;
	A = 3;
	emu.ExecuteOpcode(0x86);
	EXPECT_EQ(A, 10);

	// ADD A, n8: 0xC6
	A = 15;
	rom[PC] = 17;
	emu.ExecuteOpcode(0xC6);
	EXPECT_EQ(A, 32);

	// ADC A, r8: 0x10'001'xxx
	A = 2;
	F = BitSet(F, emu.FLAG_C);
	emu.ExecuteOpcode(0b10'001'111);
	EXPECT_EQ(A, 5);

	// ADC A, [HL]: 0x8E
	HL = 0xCFB7;
	rom[HL] = 7;
	A = 3;
	F = BitSet(F, emu.FLAG_C);
	emu.ExecuteOpcode(0x8E);
	EXPECT_EQ(A, 11);

	// ADC A, n8: 0xCE
	A = 15;
	rom[PC] = 17;
	F = BitSet(F, emu.FLAG_C);
	emu.ExecuteOpcode(0xCE);
	EXPECT_EQ(A, 33);

	// SUB A, r8: 0x10'010'xxx
	A = 7;
	emu.ExecuteOpcode(0b10'010'111);
	EXPECT_EQ(A, 0);
	EXPECT_TRUE(F & (1 << emu.FLAG_Z));

	// SUB A, [HL]: 0x96
	HL = 0xCFB7;
	rom[HL] = 7;
	A = 10;
	emu.ExecuteOpcode(0x96);
	EXPECT_EQ(A, 3);

	// SUB A, n8: 0xD6
	A = 15;
	rom[PC] = 9;
	emu.ExecuteOpcode(0xD6);
	EXPECT_EQ(A, 6);

	// SBC A, r8: 0x10'011'xxx
	A = 63;
	B = 3;
	F = BitSet(F, emu.FLAG_C);
	emu.ExecuteOpcode(0b10'011'000);
	EXPECT_EQ(A, 59);

	// SBC A, [HL]: 0x9E
	HL = 0xCFB7;
	rom[HL] = 7;
	A = 14;
	F = BitSet(F, emu.FLAG_C);
	emu.ExecuteOpcode(0x9E);
	EXPECT_EQ(A, 6);

	// SBC A, n8: 0xDE
	A = 17;
	rom[PC] = 15;
	F = BitSet(F, emu.FLAG_C);
	emu.ExecuteOpcode(0xDE);
	EXPECT_EQ(A, 1);

	// CP A, r8: 0x10'111'xxx
	A = 5;
	emu.ExecuteOpcode(0b10'111'111);
	auto f{ GetFlags(emu) };
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], true);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 5;
	B = 7;
	emu.ExecuteOpcode(0b10'111'000);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], true);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], true);

	A = 7;
	B = 5;
	emu.ExecuteOpcode(0b10'111'000);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], true);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// CP A, [HL]: 0xBE
	A = 112;
	HL = 0xC123;
	rom[HL] = 112;
	emu.ExecuteOpcode(0xBE);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], true);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// CP A, n8: 0xFE
	A = 203;
	rom[PC] = 203;
	emu.ExecuteOpcode(0xFE);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], true);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// INC r8: 0b00'xxx'100
	A = 7;
	emu.ExecuteOpcode(0b00'111'100);
	EXPECT_EQ(A, 8);

	// INC [HL]: 0x34
	HL = 0xC6A3;
	rom[HL] = 15;
	emu.ExecuteOpcode(0x34);
	EXPECT_EQ(rom[HL], 16);

	// DEC r8: 0b00'xxx'101
	A = 7;
	emu.ExecuteOpcode(0b00'111'101);
	EXPECT_EQ(A, 6);

	// DEC [HL]: 0x35
	HL = 0xC6A3;
	rom[HL] = 15;
	emu.ExecuteOpcode(0x35);
	EXPECT_EQ(rom[HL], 14);

	// AND A, r8: 0b10'100'xxx
	A = 0b1001'1011;
	B = 0b1101'0101;
	emu.ExecuteOpcode(0b10'100'000);
	EXPECT_EQ(A, 0b1001'0001);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'1011;
	B = 0b0100'0100;
	emu.ExecuteOpcode(0b10'100'000);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	// AND A, [HL]: 0xA6
	A = 0b1001'1011;
	HL = 0xC123;
	rom[HL] = 0b1101'0101;
	emu.ExecuteOpcode(0xA6);
	EXPECT_EQ(A, 0b1001'0001);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'1011;
	HL = 0xC123;
	rom[HL] = 0b0100'0100;
	emu.ExecuteOpcode(0xA6);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	// AND A, n8: 0xE6
	A = 0b1001'1011;
	rom[PC] = 0b1101'0101;
	emu.ExecuteOpcode(0xE6);
	EXPECT_EQ(A, 0b1001'0001);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'1011;
	rom[PC] = 0b0100'0100;
	emu.ExecuteOpcode(0xE6);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	// OR A, r8: 0b10'110'xxx
	A = 0b1001'1011;
	B = 0b1101'0101;
	emu.ExecuteOpcode(0b10'110'000);
	EXPECT_EQ(A, 0b1101'1111);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0;
	B = 0;
	emu.ExecuteOpcode(0b10'110'000);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// OR A, [HL]: 0xB6
	A = 0b1001'1011;
	HL = 0xC123;
	rom[HL] = 0b1101'0101;
	emu.ExecuteOpcode(0xB6);
	EXPECT_EQ(A, 0b1101'1111);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0;
	HL = 0xC123;
	rom[HL] = 0;
	emu.ExecuteOpcode(0xB6);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// OR A, n8: 0xF6
	A = 0b1001'1011;
	rom[PC] = 0b1101'0101;
	emu.ExecuteOpcode(0xF6);
	EXPECT_EQ(A, 0b1101'1111);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0;
	rom[PC] = 0;
	emu.ExecuteOpcode(0xF6);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// XOR A, r8: 0b10'101'xxx
	A = 0b1001'1011;
	B = 0b1101'0101;
	emu.ExecuteOpcode(0b10'101'000);
	EXPECT_EQ(A, 0b0100'1110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'1011;
	B = 0b1001'1011;
	emu.ExecuteOpcode(0b10'101'000);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// XOR A, [HL]: 0xAE
	A = 0b1001'1011;
	rom[HL] = 0b1101'0101;
	emu.ExecuteOpcode(0xAE);
	EXPECT_EQ(A, 0b0100'1110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'1011;
	rom[HL] = 0b1001'1011;
	emu.ExecuteOpcode(0xAE);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// XOR A, n8: 0xEE
	A = 0b1001'1011;
	rom[PC] = 0b1101'0101;
	emu.ExecuteOpcode(0xEE);
	EXPECT_EQ(A, 0b0100'1110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'1011;
	rom[PC] = 0b1001'1011;
	emu.ExecuteOpcode(0xEE);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// CCF: 0x3F
	emu.ExecuteOpcode(0x3F);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	emu.ExecuteOpcode(0x3F);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// SCF: 0x37
	emu.ExecuteOpcode(0x37);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// DAA: 0x27
	BYTE x{ 0b0000'1000 };
	BYTE y{ 0b0000'0111 };
	A = x + y;
	emu.ExecuteOpcode(0x27);
	EXPECT_EQ(A, 0b0001'0101);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0;
	emu.ExecuteOpcode(0x27);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// CPL: 0x2F
	A = 0b0011'0110;
	emu.ExecuteOpcode(0x2F);
	EXPECT_EQ(A, 0b1100'1001);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], true);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);
	
	//||||||||||||||||||||||||||||||||||||||||||||||

	//==============================================
	// 16 bit arithmetic
	
	// INC r16: 0b00'xx'0011
	SP = 0xFFFD;
	emu.ExecuteOpcode(0b00'11'0011);
	EXPECT_EQ(SP, 0xFFFE);

	// DEC r16: 0b00'xx'1011
	SP = 0xFFFF;
	emu.ExecuteOpcode(0b00'11'1011);
	EXPECT_EQ(SP, 0xFFFE);

	// ADD HL, r16: 0b00'xx'1001
	HL = 29041;
	DE = 7319;
	emu.ExecuteOpcode(0b00'01'1001);
	EXPECT_EQ(HL, 36'360);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	HL = 0xF00;
	emu.ExecuteOpcode(0b00'10'1001);
	EXPECT_EQ(HL, 0xF00 + 0xF00);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	// ADD SP, e8: 0xE8
	SP = 0xFFFE;
	rom[PC] = -2;
	emu.ExecuteOpcode(0xE8);
	EXPECT_EQ(SP, 0xFFFC);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	SP = 0xFFFC;
	rom[PC] = 2;
	emu.ExecuteOpcode(0xE8);
	EXPECT_EQ(SP, 0xFFFE);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	//||||||||||||||||||||||||||||||||||||||||||||||

	//==============================================
	// Rotate and shift operation instructions

	// RLCA: 0x07
	A = 0b1001'0110;
	emu.ExecuteOpcode(0x07);
	EXPECT_EQ(A, 0b101101);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	A = 0b0001'0110;
	emu.ExecuteOpcode(0x07);
	EXPECT_EQ(A, 0b101100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// RRCA: 0x0F
	A = 0b1001'0110;
	emu.ExecuteOpcode(0x0F);
	EXPECT_EQ(A, 0b1001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'0111;
	emu.ExecuteOpcode(0x0F);
	EXPECT_EQ(A, 0b11001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RLA: 0x17
	F = BitSet(F, emu.FLAG_C);
	A = 0b0001'0110;
	emu.ExecuteOpcode(0x17);
	EXPECT_EQ(A, 0b101101);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	F = BitReset(F, emu.FLAG_C);
	A = 0b1001'0111;
	emu.ExecuteOpcode(0x17);
	EXPECT_EQ(A, 0b101110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RRA: 0x1F
	F = BitSet(F, emu.FLAG_C);
	A = 0b1001'0110;
	emu.ExecuteOpcode(0x1F);
	EXPECT_EQ(A, 0b11001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	F = BitReset(F, emu.FLAG_C);
	A = 0b1001'0111;
	emu.ExecuteOpcode(0x1F);
	EXPECT_EQ(A, 0b1001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RLC r8: CB + 0b00'000'xxx
	A = 0b1001'0110;
	rom[PC] = 0b00'000'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b101101);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	A = 0b0001'0110;
	rom[PC] = 0b00'000'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b101100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// RLC [HL]: CB + 0x06
	HL = 0xC123;
	rom[HL] = 0b1001'0110;
	rom[PC] = 0x06;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b101101);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	rom[HL] = 0b0001'0110;
	rom[PC] = 0x06;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b101100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// RRC r8: CB + 0b00'001'xxx
	A = 0b1001'0110;
	rom[PC] = 0b00'001'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0b1001'0111;
	rom[PC] = 0b00'001'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b11001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RRC [HL]: CB + 0x0E
	rom[HL] = 0b1001'0110;
	rom[PC] = 0x0E;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	rom[HL] = 0b1001'0111;
	rom[PC] = 0x0E;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b11001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RL r8: CB + 0b00'010'xxx
	F = BitSet(F, emu.FLAG_C);
	A = 0b0001'0110;
	rom[PC] = 0x17;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b101101);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	F = BitReset(F, emu.FLAG_C);
	A = 0b1001'0111;
	rom[PC] = 0x17;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b101110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RL [HL]: CB + 0x16
	F = BitSet(F, emu.FLAG_C);
	rom[HL] = 0b0001'0110;
	rom[PC] = 0x16;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b101101);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	F = BitReset(F, emu.FLAG_C);
	rom[HL] = 0b1001'0111;
	rom[PC] = 0x16;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b101110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RR r8: CB + 0b00'011'xxx
	F = BitSet(F, emu.FLAG_C);
	A = 0b1001'0110;
	rom[PC] = 0x1F;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b11001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	F = BitReset(F, emu.FLAG_C);
	A = 0b1001'0111;
	rom[PC] = 0x1F;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// RR [HL]: CB + 0x1E
	F = BitSet(F, emu.FLAG_C);
	rom[HL] = 0b1001'0110;
	rom[PC] = 0x1E;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b11001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	F = BitReset(F, emu.FLAG_C);
	rom[HL] = 0b1001'0111;
	rom[PC] = 0x1E;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1001011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// SLA r8: CB + 0b00'100'xxx
	A = 0b1001'0110;
	rom[PC] = 0b00'100'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b10'1100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	A = 0b1001'0111;
	rom[PC] = 0b00'100'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b10'1110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// SLA [HL]: CB + 0x26
	rom[HL] = 0b1001'0110;
	rom[PC] = 0x26;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b10'1100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	rom[HL] = 0b1001'0111;
	rom[PC] = 0x26;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b10'1110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	// SRA r8: CB + 0b00'101'xxx
	A = 0b1001'0111;
	rom[PC] = 0b00'101'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1100'1011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	A = 0b1001'0110;
	rom[PC] = 0b00'101'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1100'1011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0b0001'0110;
	rom[PC] = 0b00'101'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b0000'1011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// SRA [HL]: CB + 0x2E
	rom[HL] = 0b1001'0111;
	rom[PC] = 0x2E;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1100'1011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	rom[HL] = 0b1001'0110;
	rom[PC] = 0x2E;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1100'1011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	rom[HL] = 0b0001'0110;
	rom[PC] = 0x2E;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b0000'1011);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// SWAP r8: CB + 0b00'110'xxx
	A = 0b1001'0110;
	rom[PC] = 0b00'110'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b0110'1001);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	A = 0;
	rom[PC] = 0b00'110'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// SWAP [HL]: CB + 0x36
	rom[HL] = 0b1001'0110;
	rom[PC] = 0x36;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b0110'1001);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	rom[HL] = 0;
	rom[PC] = 0x36;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	// SRL r8: CB + 0b00'111'xxx
	A = 0b1010'1101;
	rom[PC] = 0b00'111'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b101'0110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	A = 1;
	rom[PC] = 0b00'111'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], true);

	A = 0b1010'1100;
	rom[PC] = 0b00'111'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b101'0110);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['C'], false);

	//||||||||||||||||||||||||||||||||||||||||||||||

	//==============================================
	// Bit operations

	// BIT u3, r8: 0b01'xxx'xxx
	A = 0b1010'1100;
	rom[PC] = 0b01'011'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1010'1100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	A = 0b1010'1100;
	rom[PC] = 0b01'100'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1010'1100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	// BIT u3, [HL]: 0b01'xxx'110
	rom[HL] = 0b1010'1100;
	rom[PC] = 0b01'011'110;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1010'1100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	rom[HL] = 0b1010'1100;
	rom[PC] = 0b01'100'110;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1010'1100);
	f = GetFlags(emu);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['C'], false);

	// RES u3, r8: 0b10'xxx'xxx
	A = 0b1010'1100;
	rom[PC] = 0b10'011'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1010'0100);

	// RES u3, [HL]: 0b10'xxx'110
	rom[HL] = 0b1010'1100;
	rom[PC] = 0b10'101'110;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1000'1100);

	// SET u3, r8: 0b11'xxx'xxx
	A = 0b1010'1100;
	rom[PC] = 0b11'100'111;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(A, 0b1011'1100);

	// SET u3, [HL]: 0b11'xxx'110
	rom[HL] = 0b1010'1100;
	rom[PC] = 0b11'110'110;
	emu.ExecuteOpcode(0xCB);
	EXPECT_EQ(rom[HL], 0b1110'1100);

	//||||||||||||||||||||||||||||||||||||||||||||||

	//==============================================
	// Control flow
	
	// JP n16: 0xC3
	rom[PC] = 0x34;
	rom[PC + 1] = 0x12;
	emu.ExecuteOpcode(0xC3);
	EXPECT_EQ(PC, 0x1234);

	// JP HL: 0xE9
	HL = 0x100;
	emu.ExecuteOpcode(0xE9);
	EXPECT_EQ(PC, 0x100);

	// TODO: JP cc, n16: 0b110'xx'010

	// JR n16: 0x18
	rom[PC] = 5;
	emu.ExecuteOpcode(0x18);
	EXPECT_EQ(PC, 0x106);

	// TODO: JR cc, n16: 0b001'xx'000

	// CALL n16: 0xCD
	WORD PC_next = PC + 2;
	rom[PC] = 0x34;
	rom[PC + 1] = 0x12;
	SP = 0xFFFE;
	emu.ExecuteOpcode(0xCD);
	EXPECT_EQ(PC, 0x1234);
	EXPECT_EQ(rom[SP], (PC_next & 0xF));
	EXPECT_EQ(rom[SP + 1], PC_next >> 8);

	// TODO: CALL cc, n16: 0b110'xx'100

	// RET: 0xC9
	emu.ExecuteOpcode(0xC9);
	EXPECT_EQ(PC, PC_next);

	// TODO: RET cc: 0b110xx000

	// RETI: 0xD9
	emu.PushWordOntoStack(0x100);
	emu.ExecuteOpcode(0xD9);
	EXPECT_EQ(PC, 0x100);
	EXPECT_EQ(emu.m_InteruptMaster, true);

	// RST vec: 0b11'xxx'111
	emu.ExecuteOpcode(0b11'100'111);
	EXPECT_EQ(PC, 0x20);

	// HALT: 0x76
	/*emu.ExecuteOpcode(0x76);
	EXPECT_TRUE(emu.m_Halted);*/
	
	// STOP: 0x10
	emu.ExecuteOpcode(0x10);
	EXPECT_EQ(PC, 0x21);

	
	//||||||||||||||||||||||||||||||||||||||||||||||
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}