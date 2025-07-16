#include <gtest/gtest.h>

#include "CPU/Emulator.h"

#include <array>
#include <utility>
#include <map>
#include <iostream>

#define MORE_DEBUG

struct EmulatorTest : public testing::Test {
	Emulator* emu{};

	void SetUp() { emu = new Emulator(); }
	void TearDown() { delete emu; }
};

TEST_F(EmulatorTest, MemoryTest) {
	// No side effects test
	constexpr int num_addresses{ 6 };

	constexpr std::array<WORD, num_addresses> addresses{ 0xA000, 0xFEA0, 0xFF04,
		0xFF44, 0x4001, 0x8000 };
	constexpr std::array<BYTE, num_addresses> input{ 123, 123, 123,
		123, 123, 123 };
	std::array<BYTE, num_addresses> former{};

	for (int i{ 0 }; i < num_addresses; ++i) {
		former[i] = emu->read_memory(addresses[i]);
	}
	std::array<BYTE, num_addresses> expected_output{former[0], former[1], 0,
		0, former[4], 123};

	for (int i{ 0 }; i < num_addresses; ++i) {
		emu->write_memory(addresses[i], input[i]);
	}

	for (int i{ 0 }; i < num_addresses; ++i) {
		EXPECT_EQ(expected_output[i], emu->read_memory(addresses[i]));
	}

	// Testing side effects
	emu->m_MBC1 = true;
	emu->write_memory(0, 0xA);
	EXPECT_TRUE(emu->m_enable_RAM);

	emu->write_memory(0x2000, 0b10010);
	EXPECT_EQ(emu->m_current_ROM_bank & 0b11111, 0b10010);

	emu->write_memory(0x4000, 0b1000000);
	EXPECT_EQ(emu->m_current_ROM_bank & 0b11100000, 0b1000000);

	emu->write_memory(0x6000, 123);
	emu->write_memory(0x4000, 0b10);
	EXPECT_EQ(emu->m_current_RAM_bank, 0b10);
}

std::unordered_map<char, bool> get_flags(Emulator* emu) {
	constexpr int FLAG_Z{ 7 };
	constexpr int FLAG_N{ 6 };
	constexpr int FLAG_H{ 5 };
	constexpr int FLAG_C{ 4 };

	std::unordered_map<char, bool> ret{};

	ret['C'] = emu->m_registerAF.lo & (1 << FLAG_C);
	ret['H'] = emu->m_registerAF.lo & (1 << FLAG_H);
	ret['N'] = emu->m_registerAF.lo & (1 << FLAG_N);
	ret['Z'] = emu->m_registerAF.lo & (1 << FLAG_Z);

	return ret;
}

std::ostream& operator<<(std::ostream& out, std::unordered_map<char, bool> m) {
	out << std::boolalpha;
	for (const auto& [k, v] : m) {
		out << k << ": " << v << '\n';
	}
	return out;
}

TEST_F(EmulatorTest, CPUTest) {
	int cycles;
	WORD pc_copy;

	WORD& pc{ emu->m_program_counter };
	WORD& SP{ emu->m_stack_pointer.reg };
	BYTE& A{ emu->m_registerAF.hi };
	BYTE& F{ emu->m_registerAF.lo };
	BYTE& B{ emu->m_registerBC.hi };
	BYTE& C{ emu->m_registerBC.lo };
	BYTE& D{ emu->m_registerDE.hi };
	BYTE& E{ emu->m_registerDE.lo };
	BYTE& H{ emu->m_registerHL.hi };
	BYTE& L{ emu->m_registerHL.lo };
	WORD& AF{ emu->m_registerAF.reg };
	WORD& BC{ emu->m_registerBC.reg };
	WORD& DE{ emu->m_registerDE.reg };
	WORD& HL{ emu->m_registerHL.reg };

	BYTE* rom{ emu->m_rom };

	constexpr int FLAG_Z{ 7 };
	constexpr int FLAG_N{ 6 };
	constexpr int FLAG_H{ 5 };
	constexpr int FLAG_C{ 4 };

	//=============================================
	// 8 bit load
	// 
	// LD r8, r8
	pc_copy = pc;
	C = 213;
	cycles = emu->execute_opcode(0x41);
	EXPECT_EQ(B, 213);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 4);

	// LD r8, n
	pc_copy = pc;
	rom[pc_copy] = 0x1E;
	cycles = emu->execute_opcode(0x06);
	EXPECT_EQ(B, 0x1E);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 8);

	// LD r8, [HL]
	pc_copy = pc;
	rom[0xC000] = 0xAF;
	HL = 0xC000;
	cycles = emu->execute_opcode(0x46);
	EXPECT_EQ(B, 0xAF);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD [HL], r8
	pc_copy = pc;
	cycles = emu->execute_opcode(0x70);
	EXPECT_EQ(rom[HL], 0xAF);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD [HL], n
	pc_copy = pc;
	rom[pc] = 0x4D;
	cycles = emu->execute_opcode(0x36);
	EXPECT_EQ(rom[HL], 0x4D);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 12);

	// LD A, [BC]
	pc_copy = pc;
	BC = 0xC321;
	rom[0xC321] = 0xAD;
	cycles = emu->execute_opcode(0x0A);
	EXPECT_EQ(rom[BC], 0xAD);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD A, [DE]
	pc_copy = pc;
	DE = 0xC321;
	rom[0xC321] = 0xAD;
	cycles = emu->execute_opcode(0x1A);
	EXPECT_EQ(rom[DE], 0xAD);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD [BC], A
	pc_copy = pc;
	BC = 0xC321;
	A = 0xAD;
	cycles = emu->execute_opcode(0x02);
	EXPECT_EQ(rom[BC], 0xAD);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD [DE], A
	pc_copy = pc;
	DE = 0xC321;
	A = 0xAD;
	cycles = emu->execute_opcode(0x12);
	EXPECT_EQ(rom[DE], 0xAD);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD A, [n16]
	pc_copy = pc;
	rom[pc] = 0x54;
	rom[pc + 1] = 0xC0;
	rom[0xC054] = 0xA0;
	cycles = emu->execute_opcode(0xFA);
	EXPECT_EQ(A, 0xA0);
	EXPECT_EQ(pc_copy, pc - 2);
	EXPECT_EQ(cycles, 16);

	// LD [n16], A
	pc_copy = pc;
	cycles = emu->execute_opcode(0xEA);
	EXPECT_EQ(rom[0xC054], 0xA0);
	EXPECT_EQ(pc_copy, pc - 2);
	EXPECT_EQ(cycles, 16);

	// LDH A, [C]
	pc_copy = pc;
	rom[0xFF03] = 0x7D;
	C = 0x03;
	cycles = emu->execute_opcode(0xF2);
	EXPECT_EQ(A, 0x7D);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LDH [C], A
	pc_copy = pc;
	C = 0x03;
	A = 0x4F;
	cycles = emu->execute_opcode(0xE2);
	EXPECT_EQ(rom[0xFF03], 0x4F);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LDH A, [n16]
	pc_copy = pc;
	rom[pc] = 0x4F;
	rom[0xFF4F] = 0x4F;
	cycles = emu->execute_opcode(0xF0);
	EXPECT_EQ(A, 0x4F);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 12);

	// LDH [n16], A
	pc_copy = pc;
	rom[pc] = 0x4F;
	A = 0xAB;
	cycles = emu->execute_opcode(0xE0);
	EXPECT_EQ(rom[0xFF4F], 0xAB);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 12);

	// LD A, [HLD]
	pc_copy = pc;
	rom[0xC000] = 0xAF;
	HL = 0xC000;
	cycles = emu->execute_opcode(0x3A);
	EXPECT_EQ(A, 0xAF);
	EXPECT_EQ(HL, 0xBFFF);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD [HLD], A
	pc_copy = pc;
	HL = 0xC003;
	A = 0x7D;
	cycles = emu->execute_opcode(0x32);
	EXPECT_EQ(rom[0xC003], 0x7D);
	EXPECT_EQ(HL, 0xC002);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD A, [HLI]
	pc_copy = pc;
	rom[0xC000] = 0xAF;
	HL = 0xC000;
	cycles = emu->execute_opcode(0x2A);
	EXPECT_EQ(A, 0xAF);
	EXPECT_EQ(HL, 0xC001);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// LD [HLI], A
	pc_copy = pc;
	HL = 0xC003;
	A = 0x7D;
	cycles = emu->execute_opcode(0x22);
	EXPECT_EQ(rom[0xC003], 0x7D);
	EXPECT_EQ(HL, 0xC004);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	//=============================================
	// Interrupts test
	pc_copy = pc;
	cycles = emu->execute_opcode(0xFB);
	EXPECT_FALSE(emu->m_interupt_master);
	EXPECT_EQ(cycles, 4);
	rom[pc] = 0x22;
	emu->execute_next_opcode();
	EXPECT_TRUE(emu->m_interupt_master);
	cycles = emu->execute_opcode(0xF3);
	EXPECT_FALSE(emu->m_interupt_master);
	EXPECT_FALSE(emu->m_pending_interrupt_enabled);
	EXPECT_EQ(cycles, 4);
	//==============================================

	//==============================================
	// DAA test
	A = 0b00100011;
	emu->execute_opcode(0x87);
	emu->execute_opcode(0x27);
	EXPECT_EQ(A, 0b01000110);
	//==============================================

	//==============================================
	// 16 bit load
	// 
	// LD r16, n16
	pc_copy = pc;
	rom[pc] = 0x7D;
	rom[pc + 1] = 0x4D;
	cycles = emu->execute_opcode(0x01);
	EXPECT_EQ(BC, 0x4D7D);
	EXPECT_EQ(pc_copy, pc - 2);
	EXPECT_EQ(cycles, 12);

	// LD [n16], SP
	pc_copy = pc;
	SP = 0xFFFE;
	rom[pc] = 0x00;
	rom[pc + 1] = 0xC0;
	cycles = emu->execute_opcode(0x08);
	EXPECT_EQ(rom[0xC000], 0xFE);
	EXPECT_EQ(rom[0xC001], 0xFF);
	EXPECT_EQ(pc_copy, pc - 2);
	EXPECT_EQ(cycles, 20);

	// LD SP, HL
	pc_copy = pc;
	HL = 0xD8E9;
	cycles = emu->execute_opcode(0xF9);
	EXPECT_EQ(SP, 0xD8E9);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// PUSH r16
	pc_copy = pc;
	SP = 0xFFFE;
	BC = 0xD8E9;
	cycles = emu->execute_opcode(0xC5);
	EXPECT_EQ(rom[SP], 0xE9);
	EXPECT_EQ(rom[SP + 1], 0xD8);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 16);

	// POP r16
	pc_copy = pc;
	cycles = emu->execute_opcode(0xC1);
	EXPECT_EQ(BC, 0xD8E9);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 12);

	// LD HL, SP+e8
	pc_copy = pc;
	rom[pc] = 0xFE;
	cycles = emu->execute_opcode(0xF8);
	EXPECT_EQ(HL, 0xFFFC);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 12);
#ifdef MORE_DEBUG
	std::cout << get_flags(emu);
#endif // MORE_DEBUG
	//==============================================
	
	//==============================================
	// 8 bit arithmetic
	// 
	// ADD A, r8
	pc_copy = pc;
	A = 15;
	B = 7;
	cycles = emu->execute_opcode(0x80);
	EXPECT_EQ(A, 22);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 4);

	// ADD A, [HL]
	pc_copy = pc;
	HL = 0xC013;
	rom[HL] = 0xA4;
	cycles = emu->execute_opcode(0x86);
	EXPECT_EQ(A, 0xBA);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 8);

	// CCF
	cycles = emu->execute_opcode(0x3F);
	auto f{ get_flags(emu) };
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 4);

	// SCF
	cycles = emu->execute_opcode(0x37);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 4);

	// CPL
	A = 0b10101010;
	cycles = emu->execute_opcode(0x2F);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], true);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(A, 0b1010101);
	EXPECT_EQ(pc_copy, pc);
	EXPECT_EQ(cycles, 4);
	//==============================================

	//==============================================
	// bit flag

	// bit u3, r8
	pc_copy = pc;
	rom[pc] = 0b01011000;
	B = 0b10001111;
	cycles = emu->execute_extended_opcode();
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 8);

	pc_copy = pc;
	rom[pc] = 0b01011000;
	B = 0b10000111;
	cycles = emu->execute_extended_opcode();
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 8);

	// bit u3, [HL]
	pc_copy = pc;
	rom[pc] = 0b01011110;
	rom[HL] = 0b10001111;
	cycles = emu->execute_extended_opcode();
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 16);

	pc_copy = pc;
	rom[pc] = 0b01011110;
	rom[HL] = 0b10000111;
	cycles = emu->execute_extended_opcode();
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], true);
	EXPECT_EQ(f['Z'], true);
	EXPECT_EQ(pc_copy, pc - 1);
	EXPECT_EQ(cycles, 16);

	// TODO: res u3, [HL]
	//==============================================

	//==============================================
	// Control flow
	//
	// CALL n16
	pc_copy = pc;
	rom[pc] = 0x03;
	rom[pc + 1] = 0xC0;
	SP = 0xFFFE;
	cycles = emu->execute_opcode(0xCD);
	EXPECT_EQ(pc, 0xC003);
	EXPECT_EQ(cycles, 12);

	cycles = emu->execute_opcode(0xC9);
	EXPECT_EQ(pc, pc_copy + 2);
	EXPECT_EQ(cycles, 8);

	// CALL cc, n16
	F &= ~(1 << FLAG_Z);
	pc_copy = pc;
	rom[pc] = 0x03;
	rom[pc + 1] = 0xC0;
	SP = 0xFFFE;
	cycles = emu->execute_opcode(0xC4);
	EXPECT_EQ(pc, 0xC003);
	EXPECT_EQ(cycles, 12);

	cycles = emu->execute_opcode(0xC0);
	EXPECT_EQ(pc, pc_copy + 2);
	EXPECT_EQ(cycles, 8);

	cycles = emu->execute_opcode(0xDF);
	EXPECT_EQ(pc, 0x18);
	EXPECT_EQ(cycles, 32);

	//==============================================

	//==============================================
	// Bit shifts
	//
	// RLCA
	pc_copy = pc;
	A = 0b10010100;
	cycles = emu->execute_opcode(0x07);
	EXPECT_EQ(A, 0b101001);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc, pc_copy);
	EXPECT_EQ(cycles, 4);

	pc_copy = pc;
	A = 0b10100;
	cycles = emu->execute_opcode(0x07);
	EXPECT_EQ(A, 0b101000);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc, pc_copy);
	EXPECT_EQ(cycles, 4);

	// RRCA
	pc_copy = pc;
	A = 0b10101;
	cycles = emu->execute_opcode(0x0F);
	EXPECT_EQ(A, 0b10001010);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc, pc_copy);
	EXPECT_EQ(cycles, 4);

	pc_copy = pc;
	A = 0b10100;
	cycles = emu->execute_opcode(0x0F);
	EXPECT_EQ(A, 0b1010);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc, pc_copy);
	EXPECT_EQ(cycles, 4);

	// RLA
	pc_copy = pc;
	A = 0b10101;
	cycles = emu->execute_opcode(0x17);
	EXPECT_EQ(A, 0b101010);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], false);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc, pc_copy);
	EXPECT_EQ(cycles, 4);

	pc_copy = pc;
	A = 0b11101000;
	cycles = emu->execute_opcode(0x17);
	EXPECT_EQ(A, 0b11010000);
	f = get_flags(emu);
	EXPECT_EQ(f['C'], true);
	EXPECT_EQ(f['N'], false);
	EXPECT_EQ(f['H'], false);
	EXPECT_EQ(f['Z'], false);
	EXPECT_EQ(pc, pc_copy);
	EXPECT_EQ(cycles, 4);

	//==============================================
}

TEST_F(EmulatorTest, Rotates) {
	BYTE data{ 0b10010111 };
	data = emu->CPU_RLC(data, false);
	EXPECT_EQ(data, 0b101111);
	BYTE F{ emu->m_registerAF.lo };
	EXPECT_TRUE(F & (1 << emu->FLAG_C));
	EXPECT_FALSE(F & (1 << emu->FLAG_Z));
	EXPECT_FALSE(F & (1 << emu->FLAG_H));
	EXPECT_FALSE(F & (1 << emu->FLAG_N));
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}