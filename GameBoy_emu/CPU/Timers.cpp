#include "Emulator.h"

void Emulator::update_timers(int cycles) {
	do_divider_register(cycles);

	// the clock must be enabled to update the clock
	if (is_clock_enabled())
	{
		m_timer_counter -= cycles;

		// enough cpu clock cycles have happened to update the timer
		if (m_timer_counter <= 0)
		{
			// reset m_TimerTracer to the correct value
			set_clock_freq();

			// timer about to overflow
			if (read_memory(TIMA) == 255)
			{
				write_memory(TIMA, read_memory(TMA));
				request_interupt(2);
			}
			else
			{
				write_memory(TIMA, read_memory(TIMA) + 1);
			}
		}
	}
}

bool Emulator::is_clock_enabled() const {
	return read_memory((TMC >> 2) & 1); // possible bug
}

BYTE Emulator::get_clock_freq() const {
	return read_memory(TMC) & 0x3;
}

void Emulator::set_clock_freq() {
	BYTE freq = get_clock_freq();
	switch (freq)
	{
	case 0: m_timer_counter = 1024; break; // freq 4096
	case 1: m_timer_counter = 16; break;// freq 262144
	case 2: m_timer_counter = 64; break;// freq 65536
	case 3: m_timer_counter = 256; break;// freq 16382
	}
}

void Emulator::do_divider_register(int cycles)
{
	m_divider_register += cycles; // possible bug
	if (m_divider_counter >= 255)
	{
		m_divider_counter = 0;
		++m_divider_register;
	}
}