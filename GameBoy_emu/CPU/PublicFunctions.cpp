#include "Emulator.h"

#include <string_view>
#include <algorithm>
#include <fstream>

void Emulator::update() {
	constexpr int MAX_CYCLES{ 69905 };
	int cycles_this_update = 0;

	while (cycles_this_update < MAX_CYCLES) {
		int cycles = execute_next_opcode();
		cycles_this_update += cycles;
		update_timers(cycles);
		update_graphics(cycles);
		do_interrupts();
	}

	// TODO: add rendering
}

void Emulator::initialize() {
	m_program_counter = 0x100;
	m_registerAF.reg = 0x01B0;
	m_registerBC.reg = 0x0013;
	m_registerDE.reg = 0x00D8;
	m_registerHL.reg = 0x014D;
	m_stack_pointer.reg = 0xFFFE;

	m_current_RAM_bank = 0;
	std::fill(m_RAM_banks, m_RAM_banks + 0x8000, 0);

	m_rom[0xFF05] = 0x00;
	m_rom[0xFF06] = 0x00;
	m_rom[0xFF07] = 0x00;
	m_rom[0xFF10] = 0x80;
	m_rom[0xFF11] = 0xBF;
	m_rom[0xFF12] = 0xF3;
	m_rom[0xFF14] = 0xBF;
	m_rom[0xFF16] = 0x3F;
	m_rom[0xFF17] = 0x00;
	m_rom[0xFF19] = 0xBF;
	m_rom[0xFF1A] = 0x7F;
	m_rom[0xFF1B] = 0xFF;
	m_rom[0xFF1C] = 0x9F;
	m_rom[0xFF1E] = 0xBF;
	m_rom[0xFF20] = 0xFF;
	m_rom[0xFF21] = 0x00;
	m_rom[0xFF22] = 0x00;
	m_rom[0xFF23] = 0xBF;
	m_rom[0xFF24] = 0x77;
	m_rom[0xFF25] = 0xF3;
	m_rom[0xFF26] = 0xF1;
	m_rom[0xFF40] = 0x91;
	m_rom[0xFF42] = 0x00;
	m_rom[0xFF43] = 0x00;
	m_rom[0xFF45] = 0x00;
	m_rom[0xFF47] = 0xFC;
	m_rom[0xFF48] = 0xFF;
	m_rom[0xFF49] = 0xFF;
	m_rom[0xFF4A] = 0x00;
	m_rom[0xFF4B] = 0x00;
	m_rom[0xFFFF] = 0x00;

	m_divider_register.get() = 0;
}

void Emulator::load_game(std::string_view path) {
	std::fill(m_cartridge_memory.get(), m_cartridge_memory.get() + 0x200000, 0);
	int i{};
	for (std::ifstream file{ path.data(), std::ios::binary }; file.good(); )
	{
		file >> m_cartridge_memory[i++];
	}

	switch (m_cartridge_memory[0x147])
	{
	case 1: m_MBC1 = true; break;
	case 2: m_MBC1 = true; break;
	case 3: m_MBC1 = true; break;
	case 5: m_MBC2 = true; break;
	case 6: m_MBC2 = true; break;
	default: break;
	}
}