#pragma once

#include "../Types.h"
#include <string_view>
#include <memory>
#include <functional>
#include <array>
#include <unordered_map>

#ifndef MY_NGTEST
#include <gtest/gtest.h>
#include <gtest/gtest_prod.h>
#endif // !MY_NGTEST

class Emulator {
public:
	Emulator() = default;

	void load_game(std::string_view path);
	void update();
	void initialize();

	Emulator(const Emulator&) = delete;
	Emulator& operator=(const Emulator&) = delete;
	Emulator(const Emulator&&) = delete;
	Emulator& operator=(const Emulator&&) = delete;
#ifndef MY_NGTEST
	FRIEND_TEST(EmulatorTest, MemoryTest);
	FRIEND_TEST(EmulatorTest, CPUTest);
	FRIEND_TEST(EmulatorTest, Rotates);
#endif // !MY_NGTEST

private:
	std::unique_ptr<BYTE[]> m_cartridge_memory{ std::make_unique<BYTE[]>(0x200000) };
	BYTE m_rom[0x10000];
	BYTE m_screen_data[160][144][3]{};

	union Register {
		WORD reg;
		struct {
			BYTE lo;
			BYTE hi;
		};
	};

	Register m_registerAF{};
	Register m_registerBC{};
	Register m_registerDE{};
	Register m_registerHL{};

	std::array<std::reference_wrapper<BYTE>, 8> m_registers{
		m_registerBC.hi, // 000 0
		m_registerBC.lo, // 001 1
		m_registerDE.hi, // 010 2
		m_registerDE.lo, // 011 3
		m_registerHL.hi, // 100 4
		m_registerHL.lo, // 101 5
		m_registerHL.lo, // 110 6
		m_registerAF.hi, // 111 7
	};

	std::array<std::reference_wrapper<WORD>, 4> m_registers16{
		m_registerAF.reg,
		m_registerBC.reg,
		m_registerDE.reg,
		m_registerHL.reg,
	};

	const int FLAG_Z{ 7 };
	const int FLAG_N{ 6 };
	const int FLAG_H{ 5 };
	const int FLAG_C{ 4 };

	WORD m_program_counter{};
	Register m_stack_pointer{};

	bool m_MBC1{ false };
	bool m_MBC2{ false };
	BYTE m_current_ROM_bank{ 1 };

	BYTE m_RAM_banks[0x8000]{};
	BYTE m_current_RAM_bank{};

	bool m_enable_RAM{};

	bool m_rom_banking{ true };

	const int TIMA{ 0xFF05 };
	const int TMA{ 0xFF06 };
	const int TMC{ 0xFF07 };

	int m_timer_counter{ 1024 };
	int m_divider_counter{ 0 };
	std::reference_wrapper<BYTE> m_divider_register{ std::ref(m_rom[0xFF04]) };

	bool m_interupt_master{}; // possible bug

	int m_scanline_counter{};

	enum COLOUR {
		WHITE,
		LIGHT_GRAY,
		DARK_GRAY,
		BLACK,
	};

	BYTE m_joypad_state{ 0xFF }; // possible bug

	bool m_halted{};

	bool m_pending_interrupt_disabled{}; // possible bug
	bool m_pending_interrupt_enabled{};

	int execute_next_opcode();
	int execute_opcode(BYTE opcode);
	int execute_extended_opcode();

	//CPUFunctions.cpp
	void CPU_8bit_load(BYTE& reg);
	void CPU_8bit_add(BYTE& reg, BYTE to_add,
		bool use_immediate, bool add_carry);
	void CPU_8bit_inc(BYTE& what);
	void CPU_8bit_incHL();
	void CPU_8bit_sub(BYTE& reg, BYTE subtracting,
		bool use_immediate, bool sub_carry);
	void CPU_8bit_cmp(BYTE reg, BYTE to_add,
		bool use_immediate);
	void CPU_8bit_dec(BYTE& reg);
	void CPU_8bit_decHL();
	void CPU_8bit_xor(BYTE& reg, BYTE to_xor,
		bool use_immediate);
	void CPU_8bit_and(BYTE& reg, BYTE to_and,
		bool use_immediate);
	void CPU_8bit_or(BYTE& reg, BYTE to_or,
		bool use_immediate);
	bool CPU_jump_immediate(bool use_condition, int flag, bool condition);
	bool CPU_jump_nn(bool use_condition, int flag, bool condition);
	bool CPU_call(bool use_condition, int flag, bool condition);
	bool CPU_return(bool use_condition, int flag, bool condition);
	void CPU_DAA();
	// For 0xF8 opcode
	void CPU_16bit_load();
	void CPU_16bit_add(WORD regis);
	void CPU_16bit_addSP();
	// Rotate
	BYTE CPU_RLC(BYTE data, bool isA);
	BYTE CPU_RRC(BYTE data, bool isA);
	BYTE CPU_RL(BYTE data, bool isA);
	BYTE CPU_RR(BYTE data, bool isA);
	// Shift
	BYTE CPU_SLA(BYTE data);
	BYTE CPU_SRA(BYTE data, bool isHL);
	BYTE CPU_swap(BYTE data);
	BYTE CPU_SRL(BYTE data);
	void CPU_bit(BYTE data, BYTE bit);


	void request_interupt(int id);
	void do_interrupts();
	void service_interupt(int interupt);

	void update_timers(int cycles);
	BYTE get_clock_freq() const;
	void set_clock_freq();
	bool is_clock_enabled() const;
	void do_divider_register(int cycles);

	void update_graphics(int cycles);
	bool is_LCD_enabled() const;
	void set_LCD_status();
	void draw_scan_line();
	void render_tiles();
	COLOUR get_colour(BYTE colour_num, WORD address) const;
	void render_sprites();

	void key_pressed(int key);
	void key_released(int key);
	BYTE get_joypad_state() const;

	// Misc.cpp
	void do_DMA_transfer(BYTE data);
	void push_word_onto_stack(WORD word);
	WORD pop_word_off_stack();
	WORD unsigned16(BYTE lsb, BYTE msb);
	WORD get_nn();

	BYTE read_memory(WORD address) const;
	void write_memory(WORD address, BYTE data);
	void handle_banking(WORD address, BYTE data);
	void do_RAM_bank_enable(WORD address, BYTE data);
	void do_change_loROM_bank(BYTE data);
	void do_change_hiRom_bank(BYTE data);
	void do_RAM_bank_change(BYTE data);
	void do_change_ROMRAM_mode(BYTE data);
};