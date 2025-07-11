#pragma once

#include "../Types.h"
#include <string_view>
#include <memory>

#ifndef NDEBUG
#include <gtest/gtest.h>
#include <gtest/gtest_prod.h>
#endif // !NDEBUG

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
#ifndef NDEBUG
	FRIEND_TEST(EmulatorTest, MemoryTest);
#endif // !NDEBUG

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

	int execute_next_opcode();

	void request_interupt(int id);
	void do_interupts();
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

	void do_DMA_transfer(BYTE data);

	BYTE read_memory(WORD address) const;
	void write_memory(WORD address, BYTE data);
	void handle_banking(WORD address, BYTE data);
	void do_RAM_bank_enable(WORD address, BYTE data);
	void do_change_loROM_bank(BYTE data);
	void do_change_hiRom_bank(BYTE data);
	void do_RAM_bank_change(BYTE data);
	void do_change_ROMRAM_mode(BYTE data);
};