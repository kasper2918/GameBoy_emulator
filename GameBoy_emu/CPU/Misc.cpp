#include "Emulator.h"

void Emulator::do_DMA_transfer(BYTE data) {
	WORD address = data << 8; // source address is data * 0x100
	for (int i = 0; i < 0xA0; i++) {
		write_memory(0xFE00 + i, read_memory(address + i));
	}
}

void Emulator::push_word_onto_stack(WORD word) {
	BYTE hi( word >> 8 );
	BYTE lo( word & 0xFF );
	write_memory(--m_stack_pointer.reg, hi);
	write_memory(--m_stack_pointer.reg, lo);
}

WORD Emulator::pop_word_off_stack() {
	WORD word = read_memory(m_stack_pointer.reg + 1) << 8;
	word |= read_memory(m_stack_pointer.reg);
	m_stack_pointer.reg += 2;

	return word;
}

WORD Emulator::unsigned16(BYTE lsb, BYTE msb) {
	return (msb << 8) | lsb;
}

WORD Emulator::get_nn() {
	BYTE lsb{ read_memory(m_program_counter++) }; // least significant byte
	BYTE msb{ read_memory(m_program_counter++) }; // most significant byte
	return unsigned16(lsb, msb);
}