#include <gtest/gtest.h>

#include "CPU/Emulator.h"

#include <array>
#include <utility>

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

TEST_F(EmulatorTest, CPUTest) {
	BYTE n = emu->read_memory(emu->m_program_counter + 1);
	emu->execute_opcode(0x06);
	EXPECT_EQ(emu->m_registerBC.hi, n);

	emu->m_registerDE.lo = 237;
	emu->execute_opcode(0x7B);
	EXPECT_EQ(emu->m_registerAF.hi, 237);
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