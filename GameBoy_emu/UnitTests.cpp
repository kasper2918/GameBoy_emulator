#include <gtest/gtest.h>

#include "Emulator/Emulator.h"

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

}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}