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



int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}