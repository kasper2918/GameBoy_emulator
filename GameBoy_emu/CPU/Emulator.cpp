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
	case 0b00: {
		if (opcode == 0) {
			return 1;
		}
		
		BYTE m_last5(opcode & (m0 | m1 | m2 | m3 | m4 | m5));

		std::array<std::reference_wrapper<WORD>, 4> regs16{
				m_registerBC.reg,
				m_registerDE.reg,
				m_registerHL.reg,
				m_stack_pointer.reg,
		};
		WORD& reg16{ regs16[(opcode & (m5 | m4)) >> 4].get() };

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
			reg16 = get_nn();
			return 3;
		}
		if (opcode == 0x08) {
			WORD nn{ get_nn() };
			write_memory(nn++, m_stack_pointer.lo); // possible bug
			write_memory(nn, m_stack_pointer.hi);
			return 5;
		}

		switch (m210) {
		case 0b100:
			if (m543 == 110) {
				CPU_8bit_incHL();
				return 3;
			}
			CPU_8bit_inc(xxx);
			return 1;
		case 0b101:
			if (m543 == 110) {
				CPU_8bit_decHL();
				return 3;
			}
			CPU_8bit_dec(xxx);
			return 1;
		}

		switch (opcode) {
		case 0x3F:
			m_registerAF.lo &= ~(1 << FLAG_N);
			m_registerAF.lo &= ~(1 << FLAG_H);
			m_registerAF.lo ^= 1 << FLAG_C;
			return 1;
		case 0x37:
			m_registerAF.lo &= ~(1 << FLAG_N);
			m_registerAF.lo &= ~(1 << FLAG_H);
			m_registerAF.lo |= 1 << FLAG_C;
			return 1;
		case 0x27:
			CPU_DAA();
			return 1;
		case 0x2F:
			m_registerAF.hi = ~m_registerAF.hi;
			m_registerAF.lo |= 1 << FLAG_N;
			m_registerAF.lo |= 1 << FLAG_H;
			return 1;
		case 0x08:
			WORD nn = get_nn();
			write_memory(nn++, m_stack_pointer.lo);
			write_memory(nn, m_stack_pointer.lo);
			return 5;

			// Rotate
		case 0x07:
			m_registerAF.hi = CPU_RLC(m_registerAF.hi, true);
			return 1;
		case 0x0F:
			m_registerAF.hi = CPU_RRC(m_registerAF.hi, true);
			return 1;
		case 0x17:
			m_registerAF.hi = CPU_RL(m_registerAF.hi, true);
			return 1;
		case 0x1F:
			m_registerAF.hi = CPU_RR(m_registerAF.hi, true);
			return 1;

			// unconditional immediate (e/n) jump
		case 0x18:
			CPU_jump_immediate(false, 0, false);
			return 3;

			// conditional immediate (e/n) jumps
		case 0x20:
			return CPU_jump_immediate(true, FLAG_Z, false) ? 3 : 2;
		case 0x28:
			return CPU_jump_immediate(true, FLAG_Z, true) ? 3 : 2;
		case 0x30:
			return CPU_jump_immediate(true, FLAG_C, false) ? 3 : 2;
		case 0x38:
			return CPU_jump_immediate(true, FLAG_C, true) ? 3 : 2;
		}

		switch (opcode & (m0 | m1 | m2 | m3)) {
		case 0b0011:
			++reg16;
			return 2;
		case 0b1011:
			--reg16;
			return 2;
		case 0b1001:
			CPU_16bit_add(reg16);
			return 2;
		}

		if (opcode == 0x10) {
			++m_program_counter;
			return 0;
		}
	}
			 break;
	case 0b01:
		if (opcode == 0x76) {
			m_halted = true;
			return 0;
		}
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
	case 0b11:
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

				 // 8 bit arithmetic
		case 0xC6: {
			CPU_8bit_add(m_registerAF.hi, 0, true, false);
		}
				 return 2;
		case 0xCE: {
			CPU_8bit_add(m_registerAF.hi, 0, true, true);
		}
				 return 2;
		case 0xD6: {
			CPU_8bit_sub(m_registerAF.hi, 0, true, false);
		}
				 return 2;
		case 0xDE: {
			CPU_8bit_sub(m_registerAF.hi, 0, true, true);
		}
				 return 2;
		case 0xFE: {
			CPU_8bit_cmp(m_registerAF.hi, 0, true);
		}
				 return 2;

				 // 8 bit logic
		case 0xE6: {
			CPU_8bit_and(m_registerAF.hi, 0, true);
		}
				 return 2;
		case 0xF6: {
			CPU_8bit_or(m_registerAF.hi, 0, true);
		}
				 return 2;
		case 0xEE: {
			CPU_8bit_xor(m_registerAF.hi, 0, true);
		}
				 return 2;
		case 0xE8:
			CPU_16bit_addSP();
			return 4;
		case 0xF8:
			CPU_16bit_load();
			return 3;

			// CB prefix
		case 0xCB:
			return execute_extended_opcode();

			// jumps (control flow)
			// 2 unconditional jumps
		case 0xC3:
			CPU_jump_nn(false, 0, false);
			return 4;
		case 0xE9:
			m_program_counter = m_registerHL.reg;
			return 1;

			// conditional nn jumps
		case 0xC2:
			return CPU_jump_nn(true, FLAG_Z, false) ? 4 : 3;
		case 0xCA:
			return CPU_jump_nn(true, FLAG_Z, true) ? 4 : 3;
		case 0xD2:
			return CPU_jump_nn(true, FLAG_C, false) ? 4 : 3;
		case 0xDA:
			return CPU_jump_nn(true, FLAG_C, true) ? 4 : 3;

			// unconditional call
		case 0xCD:
			CPU_call(false, 0, false);
			return 6;

			// conditional calls
		case 0xC4:
			return CPU_call(true, FLAG_Z, false) ? 6 : 3;
		case 0xCC:
			return CPU_call(true, FLAG_Z, true) ? 6 : 3;
		case 0xD4:
			return CPU_call(true, FLAG_C, false) ? 6 : 3;
		case 0xDC:
			return CPU_call(true, FLAG_C, true) ? 6 : 3;

			// unconditional return from a function
		case 0xC9:
			CPU_return(false, 0, false);
			return 4;

			// conditional returns from a function
		case 0xC0:
			return CPU_return(true, FLAG_Z, false) ? 5 : 2;
		case 0xC8:
			return CPU_return(true, FLAG_Z, true) ? 5 : 2;
		case 0xD0:
			return CPU_return(true, FLAG_C, false) ? 5 : 2;
		case 0xD8:
			return CPU_return(true, FLAG_C, true) ? 5 : 2;

			// RETI: Return from interrupt handler
			// Unconditional return from a function. 
			// Also enables interrupts by setting IME=1 (Interrupt Master Enable)
		case 0xD9:
			CPU_return(false, 0, false);
			m_interupt_master = true;
			return 4;

			// DI: disable interrupts
		case 0xF3:
			m_interupt_master = false;
			m_pending_interrupt_disabled = true;
			return 1;
		case 0xFB:
			m_pending_interrupt_enabled = true;
			return 1;
		}

		if ((opcode & (m0 | m1 | m2 | m3)) == 0b101) {
			WORD& xx{ m_registers16[(opcode & (m5 | m4)) >> 4].get() };
			push_word_onto_stack(xx);
			return 4;
		}
		// possible bug
		if ((opcode & (m0 | m1 | m2)) == 0b111) {
			std::array<BYTE, 8> rst{
				0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
			};
			
			auto vec{ rst[m543] };
			push_word_onto_stack(m_program_counter);
			m_program_counter = unsigned16(vec, 0x00);
			return 4;
		}
		if (opcode & 1) {
			WORD& xx{ m_registers16[(opcode & (m5 | m4)) >> 4].get() };
			xx = pop_word_off_stack();
			return 3;
		}
		break;
	case 0b10:
		switch (m543) {
		case 0:
			switch (m210) {
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_add(m_registerAF.hi, data, false, false);
			}
					  return 2;
			default:
				CPU_8bit_add(m_registerAF.hi, yyy, false, false);
				return 1;
			}
			break;
		case 1:
			switch (m210) {
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_add(m_registerAF.hi, data, false, true);
			}
					  return 2;
			default:
				CPU_8bit_add(m_registerAF.hi, yyy, false, true);
				return 1;
			}
			break;
		case 0b010:
			switch (m210) {
				// SUB A, [HL]	0b10010110/0x96
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_sub(m_registerAF.hi, data, false, false);
			}
					  return 2;
					  // SUB r	0b10010xxx/various
			default: {
				CPU_8bit_sub(m_registerAF.hi, yyy, false, false);
			}
				   return 1;
			}
			break;
		case 0b011:
			switch (m210) {
				// SBC A, [HL]	0b10011110/0x9E
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_sub(m_registerAF.hi, data, false, true);
			}
					  return 2;
					  // SBC r	0b10011xxx/various
			default: {
				CPU_8bit_sub(m_registerAF.hi, yyy, false, true);
			}
				   return 1;
			}
			break;
		case 0b111:
			switch (m210) {
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_cmp(m_registerAF.hi, data, false);
			}
					  return 2;
			default: {
				CPU_8bit_cmp(m_registerAF.hi, yyy, false);
			}
				   return 1;
			}
			break;
		case 0b100:
			if (m210 == 0b110) {
				BYTE to_and{ read_memory(m_registerHL.reg) };
				CPU_8bit_and(m_registerAF.hi, to_and, false);
				return 2;
			}
			CPU_8bit_and(m_registerAF.hi, yyy, 0);
			return 1;
		case 0b110:
			if (m210 == 0b110) {
				BYTE to_or{ read_memory(m_registerHL.reg) };
				CPU_8bit_or(m_registerAF.hi, to_or, false);
				return 2;
			}
			CPU_8bit_or(m_registerAF.hi, yyy, 0);
			return 1;
		case 0b101:
			if (m210 == 0b110) {
				BYTE to_xor{ read_memory(m_registerHL.reg) };
				CPU_8bit_xor(m_registerAF.hi, to_xor, false);
				return 2;
			}
			CPU_8bit_xor(m_registerAF.hi, yyy, 0);
			return 1;
		}
		break;
	default:
		std::cerr << "Unknown opcode: 0x" << std::hex << opcode << '\n';
		assert(false);
		break;
	}

	return -1;
}

int Emulator::execute_extended_opcode() {
	BYTE opcode = read_memory(m_program_counter++);

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

	// BYTE& xxx{ m_registers[m543].get() };
	BYTE& yyy{ m_registers[m210].get() };

	switch (opcode & (m7 | m6) >> 6) {
	case 0b00:
		BYTE dataHL{ read_memory(m_registerHL.reg) };
		switch (opcode) {
		case 0x06: {
			write_memory(m_registerHL.reg, CPU_RLC(dataHL, false));
		}
				 return 4;
		case 0x0E: {
			write_memory(m_registerHL.reg, CPU_RRC(dataHL, false));
		}
				 return 4;
		case 0x16: {
			write_memory(m_registerHL.reg, CPU_RL(dataHL, false));
		}
				 return 4;
		case 0x1E: {
			write_memory(m_registerHL.reg, CPU_RR(dataHL, false));
		}
				 return 4;
		case 0x2E: {
			write_memory(m_registerHL.reg, CPU_SRA(dataHL, true));
		}
				 return 4;
		case 0x36: {
			write_memory(m_registerHL.reg, CPU_swap(dataHL));
		}
				 return 4;
		case 0x3E: {
			write_memory(m_registerHL.reg, CPU_SRL(dataHL));
		}
				 return 4;
		}

		switch (m543) {
		case 0b000: {
			yyy = CPU_RLC(yyy, false);
		}
				  return 2;
		case 0b001: {
			yyy = CPU_RRC(yyy, false);
		}
				  return 2;
		case 0b010: {
			yyy = CPU_RL(yyy, false);
		}
				  return 2;
		case 0b011: {
			yyy = CPU_RR(yyy, false);
		}
				  return 2;
		case 0b100: {
			yyy = CPU_SLA(yyy);
		}
				  return 2;
		case 0b101: {
			yyy = CPU_SRA(yyy, false);
		}
				  return 2;
		case 110: {
			yyy = CPU_swap(yyy);
		}
				return 2;
		case 111: {
			yyy = CPU_SRL(yyy);
		}
				return 2;
		}
	case 0b01:
		if (m210 == 0b110) {
			CPU_bit(dataHL, m543);
			return 3;
		}
		CPU_bit(yyy, m543);
		return 2;
	case 0b10:
		if (m210 == 0b110) {
			write_memory(m_registerHL.reg, dataHL & ~(1 << m543));
			return 4;
		}
		yyy &= ~(1 << m543);
		return 2;
	case 0b11:				
		if (m210 == 0b110) {
			write_memory(m_registerHL.reg, dataHL | (1 << m543));
			return 4;
		}
		yyy |= 1 << m543;
		return 2;
	default:
		std::cerr << "Unknown opcode: 0xCB" << std::hex << opcode << '\n';
		assert(false);
		break;
	}

	return -1;
}