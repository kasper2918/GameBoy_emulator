#include "Emulator.h"
#include <cassert>
#include <array>
#include <utility>

int Emulator::execute_next_opcode() {
	int res = 0;
	BYTE opcode = read_memory(m_program_counter++);
	res = execute_opcode(opcode);
	return res;
}

int Emulator::execute_opcode(BYTE opcode) {
	// bit masks
	int m0{ 0b00000001 };
	int m1{ 0b00000010 };
	int m2{ 0b00000100 };
	int m3{ 0b00001000 };
	int m4{ 0b00010000 };
	int m5{ 0b00100000 };
	int m6{ 0b01000000 };
	int m7{ 0b10000000 };

	BYTE m543((opcode & (m5 | m4 | m3)) >> 3);
	BYTE m210((opcode & (m2 | m1 | m0)));

	BYTE& xxx{ m_registers[m543].get() };
	BYTE& yyy{ m_registers[m210].get() };

	switch ((opcode & (m7 | m6)) >> 6) {
	case 0x00: {
		BYTE m_last5(opcode & (m0 | m1 | m2 | m3 | m4 | m5));
		
		if (m543 == 0b110 && m210 == 0b110) {
			BYTE n{ read_memory(m_program_counter++) };
			write_memory(m_registerHL.reg, n);
			return 2;
		}
		else if (m210 == 0b110) {
			CPU_8bit_load(xxx);
			return 2;
		}

		else switch (m_last5) {
		case 0b1010:
			m_registerAF.hi = read_memory(m_registerBC.reg);
			return 2;
		case 0b11010:
			m_registerAF.hi = read_memory(m_registerDE.reg);
			return 2;
		case 0b10:
			write_memory(m_registerBC.reg, m_registerAF.hi);
			return 2;
		case 0b10010:
			write_memory(m_registerDE.reg, m_registerAF.hi);
			return 2;
		}

		if (opcode & 1) {
			std::array<std::reference_wrapper<WORD>, 4> regs16{
				m_registerBC.reg,
				m_registerDE.reg,
				m_registerHL.reg,
				m_stack_pointer.reg,
			};
			WORD& reg16{ regs16[(opcode & (m5 | m4)) >> 4].get() };
			
			reg16 = get_nn();
			return 3;
		}
		if (opcode == 0x08) {
			WORD nn{ get_nn() };
			write_memory(nn++, m_stack_pointer.lo); // possible bug
			write_memory(nn, m_stack_pointer.hi);
			return 5;
		}
		
		
	}
			 break;
	case 0x01:
		if (m543 == 0b110) {
			write_memory(m_registerHL.reg, yyy);
			return 2;
		}
		else if (m210 == 0b110) {
			xxx = read_memory(m_registerHL.reg);
			return 2;
		}
		else {
			xxx = yyy;
			return 1;
		}
		break;
	case 0x11:
		switch (opcode) {
		case 0xFA: {
			m_registerAF.hi = read_memory(get_nn());
		}
				 return 4;
		case 0xEA: {
			write_memory(get_nn(), m_registerAF.hi);
		}
				 return 4;
		case 0xF2:
			m_registerAF.hi = read_memory(unsigned16(m_registerBC.lo, 0xFF));
			return 2;
		case 0xE2:
			write_memory(unsigned16(m_registerBC.lo, 0xFF), m_registerAF.hi);
			return 2;
		case 0xF0: {
			BYTE n{ read_memory(m_program_counter++) };
			m_registerAF.hi = read_memory(unsigned16(n, 0xFF));
		}
				 return 3;
		case 0xE0: {
			BYTE n{ read_memory(m_program_counter++) };
			write_memory(unsigned16(n, 0xFF), m_registerAF.hi);
		}
				 return 3;
		case 0x3A: {
			m_registerAF.hi = read_memory(m_registerHL.reg--);
		}
				 return 2;
		case 0x32: {
			write_memory(m_registerHL.reg--, m_registerAF.hi);
		}
				 return 2;
		case 0x2A: {
			m_registerAF.hi = read_memory(m_registerHL.reg++);
		}
				 return 2;
		case 0x22: {
			write_memory(m_registerHL.reg++, m_registerAF.hi);
		}
				 return 2;
		case 0xF9: {
			m_stack_pointer.reg = m_registerHL.reg;
		}
				 return 2;
		}

		if ((opcode & (m0 | m1 | m2 | m3)) == 0b101) {
			WORD& xx{ m_registers16[(opcode & (m5 | m4)) >> 4].get() };
			push_word_onto_stack(xx);
			return 4;
		}
		if (opcode & 1) {
			WORD& xx{ m_registers16[(opcode & (m5 | m4)) >> 4].get() };
			xx = pop_word_off_stack();
			return 3;
		}
		if (opcode == 0xF8) {

			return 3;
		}
		break;
	default:
		std::cerr << "Unknown opcode: " << std::hex << opcode << '\n';
		assert(false);
		break;
	}

	return -1;
}

