#include "Emulator.h"

BYTE Emulator::read_memory(WORD address) const
{
	// are we reading from the rom memory bank?
	if ((address >= 0x4000) && (address <= 0x7FFF))
	{
		WORD new_address = address - 0x4000;
		return m_cartridge_memory[new_address + (m_current_ROM_bank * 0x4000)];
	}

	// are we reading from ram memory bank?
	else if ((address >= 0xA000) && (address <= 0xBFFF))
	{
		WORD new_address = address - 0xA000;
		return m_RAM_banks[new_address + (m_current_RAM_bank * 0x2000)];
	}

	else if (address == 0xFF00)
		return get_joypad_state();

	// else return memory
	return m_rom[address];
}

void Emulator::write_memory(WORD address, BYTE data)
{
	if (address < 0x8000)
	{
		handle_banking(address, data);
	}

	else if ((address >= 0xA000) && (address < 0xC000))
	{
		if (m_enable_RAM)
		{
			WORD newAddress = address - 0xA000;
			m_RAM_banks[newAddress + (m_current_RAM_bank * 0x2000)] = data;
		}
	}

	// writing to ECHO ram also writes in RAM
	else if ((address >= 0xE000) && (address < 0xFE00))
	{
		m_rom[address] = data;
		write_memory(address - 0x2000, data);
	}

	// this area is restricted
	else if ((address >= 0xFEA0) && (address < 0xFEFF))
	{
	}

	else if (TMC == address)
	{
		BYTE currentfreq = get_clock_freq();
		m_rom[TMC] = data;
		BYTE newfreq = get_clock_freq();

		if (currentfreq != newfreq)
		{
			set_clock_freq();
		}
	}

	// reset the divider register
	else if (address == 0xFF04) {
		m_rom[0xFF04] = 0;
	}
		

	else if (address == 0xFF44)
	{
		m_rom[address] = 0;
	}

	else if (address == 0xFF46)
	{
		do_DMA_transfer(data);
	}

	// no control needed over this area so write to memory
	else
	{
		m_rom[address] = data;
	}
}

void Emulator::handle_banking(WORD address, BYTE data)
{
	// do RAM enabling
	if (address < 0x2000)
	{
		if (m_MBC1 || m_MBC2)
		{
			do_RAM_bank_enable(address, data);
		}
	}

	// do ROM bank change
	else if ((address >= 0x200) && (address < 0x4000))
	{
		if (m_MBC1 || m_MBC2)
		{
			do_change_loROM_bank(data);
		}
	}

	// do ROM or RAM bank change
	else if ((address >= 0x4000) && (address < 0x6000))
	{
		// there is no rambank in mbc2 so always use rambank 0
		if (m_MBC1)
		{
			if (m_rom_banking)
			{
				do_change_hiRom_bank(data);
			}
			else
			{
				do_RAM_bank_change(data);
			}
		}
	}

	// this will change whether we are doing ROM banking
	// or RAM banking with the above if statement
	else if ((address >= 0x6000) && (address < 0x8000))
	{
		if (m_MBC1)
			do_change_ROMRAM_mode(data);
	}

}

void Emulator::do_RAM_bank_enable(WORD address, BYTE data)
{
	if (m_MBC2)
	{
		if (((address >> 4) & 1) == 1) return; // Possible bug
	}

	BYTE test_data = data & 0xF;
	if (test_data == 0xA)
		m_enable_RAM = true;
	else if (test_data == 0x0)
		m_enable_RAM = false;
}

void Emulator::do_change_loROM_bank(BYTE data)
{
	if (m_MBC2)
	{
		m_current_ROM_bank = data & 0xF;
		if (m_current_ROM_bank == 0) ++m_current_ROM_bank;
		return;
	}

	BYTE lower5 = data & 31;
	m_current_ROM_bank &= 224; // turn off the lower 5
	m_current_ROM_bank |= lower5;
	if (m_current_ROM_bank == 0) ++m_current_ROM_bank;
}

void Emulator::do_change_hiRom_bank(BYTE data) {
	// turn off the upper 3 bits of the current rom
	m_current_ROM_bank &= 31;

	// turn off the lower 5 bits of the data
	data &= 224;
	m_current_ROM_bank |= data;
	if (m_current_ROM_bank == 0) ++m_current_ROM_bank;
}

void Emulator::do_RAM_bank_change(BYTE data) {
	m_current_RAM_bank = data & 0x3;
}

void Emulator::do_change_ROMRAM_mode(BYTE data) {
	BYTE newData = data & 0x1;
	m_rom_banking = (newData == 0) ? true : false;
	if (m_rom_banking)
		m_current_RAM_bank = 0;
}