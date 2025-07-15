#include "Emulator.h"
#include <cassert>
#include <array>
#include <utility>
#include <iostream>

int Emulator::execute_next_opcode() {
	static bool enable_interrups{};
	if (m_pending_interrupt_enabled)
		enable_interrups = true;
	else
		enable_interrups = false;

	int res{};
	if (!m_halted) {
		BYTE opcode = read_memory(m_program_counter++);
		res = execute_opcode(opcode);
#ifndef NDEBUG
		if (res == -1) {
			std::cerr << "Unknown opcode: " << std::hex << opcode << '\n';
			assert(false);
		}
		std::cout << std::hex << static_cast<int>(opcode) << '\n';
#endif // !NDEBUG
		if (m_pending_interrupt_enabled && enable_interrups)
			m_interupt_master = true;
	}
	else {
		res = 4;
	}

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
		switch (opcode) {
		// NOP: no operation
		case 0x00:
			return 4;

		case 0x36: {
			BYTE n = read_memory(m_program_counter++);
			write_memory(m_registerHL.reg, n);
		}
				 return 12;

		case 0x3A: {
			m_registerAF.hi = read_memory(m_registerHL.reg--);
		}
				 return 8;
		case 0x32: {
			write_memory(m_registerHL.reg--, m_registerAF.hi);
		}
				 return 8;
		case 0x2A: {
			m_registerAF.hi = read_memory(m_registerHL.reg++);
		}
				 return 8;
		case 0x22: {
			write_memory(m_registerHL.reg++, m_registerAF.hi);
		}
				 return 8;

		case 0x3F:
			m_registerAF.lo &= ~(1 << FLAG_N);
			m_registerAF.lo &= ~(1 << FLAG_H);
			m_registerAF.lo ^= 1 << FLAG_C;
			return 4;
		case 0x37:
			m_registerAF.lo &= ~(1 << FLAG_N);
			m_registerAF.lo &= ~(1 << FLAG_H);
			m_registerAF.lo |= 1 << FLAG_C;
			return 4;
		case 0x27:
			CPU_DAA();
			return 4;
		case 0x2F:
			m_registerAF.hi = ~m_registerAF.hi;
			m_registerAF.lo |= 1 << FLAG_N;
			m_registerAF.lo |= 1 << FLAG_H;
			return 4;
		// LD [n16], SP: possible bug
		case 0x08: {
			WORD nn = get_nn();
			write_memory(nn++, m_stack_pointer.lo);
			write_memory(nn, m_stack_pointer.hi);
		}
				 return 20;

				 // Rotate
		case 0x07:
			m_registerAF.hi = CPU_RLC(m_registerAF.hi, true);
			return 4;
		case 0x0F:
			m_registerAF.hi = CPU_RRC(m_registerAF.hi, true);
			return 4;
		case 0x17:
			m_registerAF.hi = CPU_RL(m_registerAF.hi, true);
			return 4;
		case 0x1F:
			m_registerAF.hi = CPU_RR(m_registerAF.hi, true);
			return 4;

			// unconditional immediate (e/n) jump
		case 0x18:
			CPU_jump_immediate(false, 0, false);
			return 8;

			// conditional immediate (e/n) jumps
		case 0x20:
			return CPU_jump_immediate(true, FLAG_Z, false);
			return 8;
			//return CPU_jump_immediate(true, FLAG_Z, false) ? 12 : 8;
		case 0x28:
			return CPU_jump_immediate(true, FLAG_Z, true);
			return 8;
			//return CPU_jump_immediate(true, FLAG_Z, true) ? 12 : 8;
		case 0x30:
			return CPU_jump_immediate(true, FLAG_C, false);
			return 8;
			//return CPU_jump_immediate(true, FLAG_C, false) ? 12 : 8;
		case 0x38:
			return CPU_jump_immediate(true, FLAG_C, true);
			return 8;
			//return CPU_jump_immediate(true, FLAG_C, true) ? 12 : 8;

			// STOP (unimplemented in my realization)
		case 0x10:
			++m_program_counter;
			return 4;
		}

		BYTE m_last5(opcode & (m0 | m1 | m2 | m3 | m4));

		std::array<std::reference_wrapper<WORD>, 4> regs16{
				m_registerBC.reg,
				m_registerDE.reg,
				m_registerHL.reg,
				m_stack_pointer.reg,
		};
		WORD& reg16{ regs16[(opcode & (m5 | m4)) >> 4].get() };

		switch (m_last5) {
		case 0b1010:
			m_registerAF.hi = read_memory(m_registerBC.reg);
			return 8;
		case 0b11010:
			m_registerAF.hi = read_memory(m_registerDE.reg);
			return 8;
		case 0b10:
			write_memory(m_registerBC.reg, m_registerAF.hi);
			return 8;
		case 0b10010:
			write_memory(m_registerDE.reg, m_registerAF.hi);
			return 8;
		}

		switch (opcode & (m0 | m1 | m2 | m3)) {
		case 0b0011:
			++reg16;
			return 8;
		case 0b1011:
			--reg16;
			return 8;
		case 0b1001:
			CPU_16bit_add(reg16);
			return 8;
		}

		switch (m210) {
		case 0b100:
			if (m543 == 110) {
				CPU_8bit_incHL();
				return 12;
			}
			CPU_8bit_inc(xxx);
			return 4;
		case 0b101:
			if (m543 == 110) {
				CPU_8bit_decHL();
				return 12;
			}
			CPU_8bit_dec(xxx);
			return 4;
		}

		if (m543 == 0b110 && m210 == 0b110) {
			BYTE n{ read_memory(m_program_counter++) };
			write_memory(m_registerHL.reg, n);
			return 12;
		}
		else if (m210 == 0b110) {
			CPU_8bit_load(xxx);
			return 8;
		}

		if (opcode & 1) {
			reg16 = get_nn();
			return 12;
		}
	}
			 break;
	case 0b01:
		if (opcode == 0x76) {
			m_halted = true;
			return 4;
		}
		if (m543 == 0b110) {
			write_memory(m_registerHL.reg, yyy);
			return 8;
		}
		else if (m210 == 0b110) {
			xxx = read_memory(m_registerHL.reg);
			return 8;
		}
		else {
			xxx = yyy;
			return 4;
		}
		break;
	case 0b11:
		switch (opcode) {
		case 0xFA: {
			m_registerAF.hi = read_memory(get_nn());
		}
				 return 16;
		case 0xEA: {
			write_memory(get_nn(), m_registerAF.hi);
		}
				 return 16;
		case 0xF2:
			m_registerAF.hi = read_memory(unsigned16(m_registerBC.lo, 0xFF));
			return 8;
		case 0xE2:
			write_memory(unsigned16(m_registerBC.lo, 0xFF), m_registerAF.hi);
			return 8;
		case 0xF0: {
			BYTE n{ read_memory(m_program_counter++) };
			m_registerAF.hi = read_memory(unsigned16(n, 0xFF));
		}
				 return 12;
		case 0xE0: {
			BYTE n{ read_memory(m_program_counter++) };
			write_memory(unsigned16(n, 0xFF), m_registerAF.hi);
		}
				 return 12;
		case 0xF9: {
			m_stack_pointer.reg = m_registerHL.reg;
		}
				 return 8;

				 // 8 bit arithmetic
		case 0xC6: {
			CPU_8bit_add(m_registerAF.hi, 0, true, false);
		}
				 return 8;
		case 0xCE: {
			CPU_8bit_add(m_registerAF.hi, 0, true, true);
		}
				 return 8;
		case 0xD6: {
			CPU_8bit_sub(m_registerAF.hi, 0, true, false);
		}
				 return 8;
		case 0xDE: {
			CPU_8bit_sub(m_registerAF.hi, 0, true, true);
		}
				 return 8;
		case 0xFE: {
			CPU_8bit_cmp(m_registerAF.hi, 0, true);
		}
				 return 8;

				 // 8 bit logic
		case 0xE6: {
			CPU_8bit_and(m_registerAF.hi, 0, true);
		}
				 return 8;
		case 0xF6: {
			CPU_8bit_or(m_registerAF.hi, 0, true);
		}
				 return 8;
		case 0xEE: {
			CPU_8bit_xor(m_registerAF.hi, 0, true);
		}
				 return 8;
		case 0xE8:
			CPU_16bit_addSP();
			return 16;
		case 0xF8:
			CPU_16bit_load();
			return 12;

			// CB prefix
		case 0xCB:
			return execute_extended_opcode();

			// jumps (control flow)
			// 2 unconditional jumps
		case 0xC3:
			CPU_jump_nn(false, 0, false);
			return 12;
		case 0xE9:
			m_program_counter = m_registerHL.reg;
			return 4;

			// conditional nn jumps
		case 0xC2:
			return CPU_jump_nn(true, FLAG_Z, false) ? 16 : 12;
		case 0xCA:
			return CPU_jump_nn(true, FLAG_Z, true) ? 16 : 12;
		case 0xD2:
			return CPU_jump_nn(true, FLAG_C, false) ? 16 : 12;
		case 0xDA:
			return CPU_jump_nn(true, FLAG_C, true) ? 16 : 12;

			// unconditional call
		case 0xCD:
			CPU_call(false, 0, false);
			return 12;

			// conditional calls
		case 0xC4:
			CPU_call(true, FLAG_Z, false);
			return 12;
			//return CPU_call(true, FLAG_Z, false) ? 24 : 12;
		case 0xCC:
			CPU_call(true, FLAG_Z, true);
			return 12;
			//return CPU_call(true, FLAG_Z, true) ? 24 : 12;
		case 0xD4:
			CPU_call(true, FLAG_C, false);
			return 12;
			//return CPU_call(true, FLAG_C, false) ? 24 : 12;
		case 0xDC:
			CPU_call(true, FLAG_C, true);
			return 12;
			//return CPU_call(true, FLAG_C, true) ? 24 : 12;

			// unconditional return from a function
		case 0xC9:
			CPU_return(false, 0, false);
			return 8;

			// conditional returns from a function
		case 0xC0:
			CPU_return(true, FLAG_Z, false);
			return 8;
			//return CPU_return(true, FLAG_Z, false) ? 20 : 8;
		case 0xC8:
			CPU_return(true, FLAG_Z, true);
			return 8;
			//return CPU_return(true, FLAG_Z, true) ? 20 : 8;
		case 0xD0:
			CPU_return(true, FLAG_C, false);
			return 8;
			//return CPU_return(true, FLAG_C, false) ? 20 : 8;
		case 0xD8:
			CPU_return(true, FLAG_C, true);
			return 8;
			//return CPU_return(true, FLAG_C, true) ? 20 : 8;

			// RETI: Return from interrupt handler
			// Unconditional return from a function. 
			// Also enables interrupts by setting IME=1 (Interrupt Master Enable)
		case 0xD9:
			CPU_return(false, 0, false);
			m_interupt_master = true;
			return 8;

			// DI: disable interrupts
		case 0xF3:
			m_interupt_master = false;
			m_pending_interrupt_enabled = false;
			return 4;
		case 0xFB:
			m_pending_interrupt_enabled = true;
			return 4;
		}

		if ((opcode & (m0 | m1 | m2 | m3)) == 0b101) {
			WORD& xx{ m_registers16[(opcode & (m5 | m4)) >> 4].get() };
			push_word_onto_stack(xx);
			return 16;
		}
		// possible bug
		if ((opcode & (m0 | m1 | m2)) == 0b111) {
			std::array<BYTE, 8> rst{
				0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
			};

			auto vec{ rst[m543] };
			push_word_onto_stack(m_program_counter);
			m_program_counter = unsigned16(vec, 0x00);
			return 32;
		}
		if (opcode & 1) {
			WORD& xx{ m_registers16[(opcode & (m5 | m4)) >> 4].get() };
			xx = pop_word_off_stack();
			return 12;
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
					  return 8;
			default:
				CPU_8bit_add(m_registerAF.hi, yyy, false, false);
				return 4;
			}
			break;
		case 1:
			switch (m210) {
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_add(m_registerAF.hi, data, false, true);
			}
					  return 8;
			default:
				CPU_8bit_add(m_registerAF.hi, yyy, false, true);
				return 4;
			}
			break;
		case 0b010:
			switch (m210) {
				// SUB A, [HL]	0b10010110/0x96
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_sub(m_registerAF.hi, data, false, false);
			}
					  return 8;
					  // SUB r	0b10010xxx/various
			default: {
				CPU_8bit_sub(m_registerAF.hi, yyy, false, false);
			}
				   return 4;
			}
			break;
		case 0b011:
			switch (m210) {
				// SBC A, [HL]	0b10011110/0x9E
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_sub(m_registerAF.hi, data, false, true);
			}
					  return 8;
					  // SBC r	0b10011xxx/various
			default: {
				CPU_8bit_sub(m_registerAF.hi, yyy, false, true);
			}
				   return 4;
			}
			break;
		case 0b111:
			switch (m210) {
			case 0b110: {
				BYTE data{ read_memory(m_registerHL.reg) };
				CPU_8bit_cmp(m_registerAF.hi, data, false);
			}
					  return 8;
			default: {
				CPU_8bit_cmp(m_registerAF.hi, yyy, false);
			}
				   return 4;
			}
			break;
		case 0b100:
			if (m210 == 0b110) {
				BYTE to_and{ read_memory(m_registerHL.reg) };
				CPU_8bit_and(m_registerAF.hi, to_and, false);
				return 8;
			}
			CPU_8bit_and(m_registerAF.hi, yyy, false);
			return 4;
		case 0b110:
			if (m210 == 0b110) {
				BYTE to_or{ read_memory(m_registerHL.reg) };
				CPU_8bit_or(m_registerAF.hi, to_or, false);
				return 8;
			}
			CPU_8bit_or(m_registerAF.hi, yyy, 0);
			return 4;
		case 0b101:
			if (m210 == 0b110) {
				BYTE to_xor{ read_memory(m_registerHL.reg) };
				CPU_8bit_xor(m_registerAF.hi, to_xor, false);
				return 8;
			}
			CPU_8bit_xor(m_registerAF.hi, yyy, 0);
			return 4;
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

	BYTE dataHL{ read_memory(m_registerHL.reg) };

	switch (opcode & (m7 | m6) >> 6) {
	case 0b00:
		switch (opcode) {
		case 0x06: {
			write_memory(m_registerHL.reg, CPU_RLC(dataHL, false));
		}
				 return 16;
		case 0x0E: {
			write_memory(m_registerHL.reg, CPU_RRC(dataHL, false));
		}
				 return 16;
		case 0x16: {
			write_memory(m_registerHL.reg, CPU_RL(dataHL, false));
		}
				 return 16;
		case 0x1E: {
			write_memory(m_registerHL.reg, CPU_RR(dataHL, false));
		}
				 return 16;
		case 0x2E: {
			write_memory(m_registerHL.reg, CPU_SRA(dataHL, true));
		}
				 return 16;
		case 0x36: {
			write_memory(m_registerHL.reg, CPU_swap(dataHL));
		}
				 return 16;
		case 0x3E: {
			write_memory(m_registerHL.reg, CPU_SRL(dataHL));
		}
				 return 16;
		}

		switch (m543) {
		case 0b000: {
			yyy = CPU_RLC(yyy, false);
		}
				  return 8;
		case 0b001: {
			yyy = CPU_RRC(yyy, false);
		}
				  return 8;
		case 0b010: {
			yyy = CPU_RL(yyy, false);
		}
				  return 8;
		case 0b011: {
			yyy = CPU_RR(yyy, false);
		}
				  return 8;
		case 0b100: {
			yyy = CPU_SLA(yyy);
		}
				  return 8;
		case 0b101: {
			yyy = CPU_SRA(yyy, false);
		}
				  return 8;
		case 110: {
			yyy = CPU_swap(yyy);
		}
				return 8;
		case 111: {
			yyy = CPU_SRL(yyy);
		}
				return 8;
		}
		break;
	case 0b01:
		if (m210 == 0b110) {
			CPU_bit(dataHL, m543);
			return 16;
		}
		CPU_bit(yyy, m543);
		return 8;
	case 0b10:
		if (m210 == 0b110) {
			write_memory(m_registerHL.reg, dataHL & ~(1 << m543));
			return 16;
		}
		yyy &= ~(1 << m543);
		return 8;
	case 0b11:
		if (m210 == 0b110) {
			write_memory(m_registerHL.reg, dataHL | (1 << m543));
			return 16;
		}
		yyy |= 1 << m543;
		return 8;
	default:
		std::cerr << "Unknown opcode: 0xCB" << std::hex << opcode << '\n';
		assert(false);
		break;
	}

	return -1;
}