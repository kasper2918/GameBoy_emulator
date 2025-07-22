#pragma once

#include <memory>
#include <string_view>
#include <array>
#include <utility>

#ifndef MY_NGTEST
#include <gtest/gtest.h>
#include <map>
#endif // !MY_NGTEST

using BYTE = uint8_t;
using SIGNED_BYTE = int8_t;
using WORD = uint16_t;
using SIGNED_WORD = int16_t;

struct SDL_Renderer;

class Emulator 
{
public:
	int gameLoadStatus{};

	// PublicFunctions.cpp
	void Update();
	void LoadGame(std::string_view path);
	Emulator();
	void KeyPressed(int key);
	void KeyReleased(int key);
	friend void DrawGraphics(SDL_Renderer*& renderer, const Emulator& emu);

#ifndef MY_NGTEST
	FRIEND_TEST(EmulatorTest, Foo);
	FRIEND_TEST(EmulatorTest, CPUTest);
	friend std::map<char, bool> GetFlags(const Emulator& emu);
#endif // !MY_NGTEST

private:
	std::unique_ptr<BYTE[]> m_CartridgeMemory{};
	BYTE m_ScreenData[160][144][3] = { 255 };
	BYTE m_Rom[0x10000] = {0};

	union Register
	{
		WORD reg;
		struct
		{
			BYTE lo;
			BYTE hi;
		};
	};

	Register m_RegisterAF{};
	Register m_RegisterBC{};
	Register m_RegisterDE{};
	Register m_RegisterHL{};
	WORD m_ProgramCounter{};
	Register m_StackPointer{};

	BYTE& A{ m_RegisterAF.hi };
	BYTE& F{ m_RegisterAF.lo };
	BYTE& B{ m_RegisterBC.hi };
	BYTE& C{ m_RegisterBC.lo };
	BYTE& D{ m_RegisterDE.hi };
	BYTE& E{ m_RegisterDE.lo };
	BYTE& H{ m_RegisterHL.hi };
	BYTE& L{ m_RegisterHL.lo };

	WORD& AF{ m_RegisterAF.reg };
	WORD& BC{ m_RegisterBC.reg };
	WORD& DE{ m_RegisterDE.reg };
	WORD& HL{ m_RegisterHL.reg };
	WORD& PC{ m_ProgramCounter };
	WORD& SP{ m_StackPointer.reg };
	
	std::array<std::reference_wrapper<BYTE>, 8> m_Regs{
		B, // 0 = 000
		C, // 1 = 001
		D, // 2 = 010
		E, // 3 = 011
		H, // 4 = 100
		L, // 5 = 101
		L, // 6 = 110
		A, // 7 = 111
	};
	std::array<std::reference_wrapper<WORD>, 4> m_Regs16{
		BC,
		DE,
		HL,
		SP,
	};

	const int FLAG_Z{ 7 };
	const int FLAG_N{ 6 };
	const int FLAG_H{ 5 };
	const int FLAG_C{ 4 };

	int m_CurrentROMBank{ 1 };
	BYTE m_RAMBanks[0x8000] = {0};
	BYTE m_CurrentRAMBank{};

	bool m_MBC1{};
	bool m_MBC2{};
	bool m_EnableRAM{}; // possible bug
	bool m_RomBanking{ true };

	const WORD TIMA{ 0xFF05 };
	const WORD TMA{ 0xFF06 };
	const WORD TMC{ 0xFF07 };

	int m_TimerCounter{ 1024 };
	int m_DividerCounter{ 0 };

	bool m_InteruptMaster{};
	bool m_PendingInteruptDisabled{};
	bool m_PendingInteruptEnabled{};
	bool m_Halted{};

	int m_ScanlineCounter{ 456 };

	BYTE m_JoypadState{ 0xFF };

	enum COLOUR
	{
		WHITE,
		LIGHT_GRAY,
		DARK_GRAY,
		BLACK,
	};

	// Memory.cpp
	void WriteMemory(WORD address, BYTE data);
	BYTE ReadMemory(WORD address) const;
	void HandleBanking(WORD address, BYTE data);
	void DoRAMBankEnable(WORD address, BYTE data);
	void DoChangeLoROMBank(BYTE data);
	void DoChangeHiRomBank(BYTE data);
	void DoRAMBankChange(BYTE data);
	void DoChangeROMRAMMode(BYTE data);

	// Timers.cpp
	void UpdateTimers(int cycles);
	bool IsClockEnabled() const;
	BYTE GetClockFreq() const;
	void SetClockFreq();
	void DoDividerRegister(int cycles);

	// Interrupts.cpp
	// I'm too lazy to fix the typo
	void RequestInterupt(int id);
	void DoInterupts();
	void ServiceInterupt(int interupt);

	// Misc/Utils.cpp
	void PushWordOntoStack(WORD word);
	WORD PopWordOffStack();

	// LCD.cpp
	void SetLCDStatus();
	bool IsLCDEnabled() const;

	// Misc/Misc.cpp
	void DoDMATransfer(BYTE data);
	WORD get_nn();

	// Graphics.cpp
	void UpdateGraphics(int cycles);
	void DrawScanLine();
	void RenderTiles(BYTE lcdControl);
	void RenderSprites(BYTE lcdControl);
	COLOUR GetColour(BYTE colourNum, WORD address) const;

	// Joypad.cpp
	BYTE GetJoypadState() const;

	// Emulator.cpp
	int ExecuteNextOpcode();
	int ExecuteOpcode(BYTE opcode);
	int ExecuteExtendedOpcode();

	// CPUFunctions.cpp
	void CPU_8BIT_LOAD(BYTE& reg);

	void CPU_8BIT_ADD(BYTE& reg, BYTE toAdd,
		bool useImmediate, bool addCarry);
	void CPU_8BIT_SUB(BYTE& reg, BYTE subtracting,
		bool useImmediate, bool subCarry);
	void CPU_8BIT_CP(BYTE& reg, BYTE subtracting, bool useImmediate);
	void CPU_8BIT_INC(BYTE& reg);
	void CPU_8BIT_DEC(BYTE& reg);
	void CPU_8BIT_AND(BYTE& reg, BYTE toAnd, bool useImmediate);
	void CPU_8BIT_OR(BYTE& reg, BYTE toOr, bool useImmediate);
	void CPU_8BIT_XOR(BYTE& reg, BYTE toXOr, bool useImmediate);
	void CPU_DAA();

	// For LD HL, SP+e8
	void CPU_16BIT_LOAD();

	void CPU_16BIT_ADD(WORD& reg, WORD toAdd);
	void CPU_16BIT_ADD_SP();

	void CPU_RLC(BYTE& reg, bool isA);
	void CPU_RRC(BYTE& reg, bool isA);
	void CPU_RL(BYTE& reg, bool isA);
	void CPU_RR(BYTE& reg, bool isA);
	// shift left arithmetically (basically bit 0 gets set to 0) (bit 7 goes into carry)
	void CPU_SLA(BYTE& reg);
	// shift right. LSB into carry. bit 7 doesn't change
	void CPU_SRA(BYTE& reg);
	// shift right. bit 0 into carry
	void CPU_SRL(BYTE& reg);

	void CPU_JUMP(bool useCondition, int flag, bool condition);
	void CPU_JUMP_IMMEDIATE(bool useCondition, int flag, bool condition);
	void CPU_CALL(bool useCondition, int flag, bool condition);
	void CPU_RETURN(bool useCondition, int flag, bool condition);
};