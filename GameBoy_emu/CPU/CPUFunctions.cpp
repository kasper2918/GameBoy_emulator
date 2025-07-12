#include "Emulator.h"

void Emulator::CPU_8bit_load(BYTE& reg) {
	BYTE n = read_memory(m_program_counter++);
	reg = n;
}

void Emulator::CPU_8bit_add(BYTE& reg, BYTE to_add,
	bool use_immediate, bool add_carry)
{
	BYTE before = reg;
	BYTE adding = 0;

	// are we adding immediate data or the second param?
	if (use_immediate)
	{
		BYTE n = read_memory(m_program_counter++);
		adding = n;
	}
	else
	{
		adding = to_add;
	}

	// are we also adding the carry flag?
	if (add_carry)
	{
		if (m_registerAF.lo & (1 << FLAG_C))
			adding++;
	}

	reg += adding;

	// set the flags
	m_registerAF.lo = 0;

	if (reg == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	WORD htest = (before & 0xF);
	htest += (adding & 0xF);

	if (htest > 0xF)
		m_registerAF.lo |= 1 << FLAG_H;

	if ((before + adding) > 0xFF)
		m_registerAF.lo |= 1 << FLAG_C;
}

void Emulator::CPU_8bit_sub(BYTE& reg, BYTE subtracting,
	bool use_immediate, bool sub_carry)
{
	BYTE before = reg;
	BYTE to_subtract = 0;

	if (use_immediate)
	{
		BYTE n = read_memory(m_program_counter++);
		to_subtract = n;
	}
	else
	{
		to_subtract = subtracting;
	}

	if (sub_carry)
	{
		if (m_registerAF.lo & (1 << FLAG_C))
			to_subtract++;
	}

	reg -= to_subtract;

	m_registerAF.lo = 0;

	if (reg == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	m_registerAF.lo |= 1 << FLAG_N;

	// set if no borrow
	if (before < to_subtract)
		m_registerAF.lo |= 1 << FLAG_C;

	SIGNED_WORD htest = (before & 0xF);
	htest -= (to_subtract & 0xF);

	if (htest < 0)
		m_registerAF.lo |= 1 << FLAG_H;
}

void Emulator::CPU_8bit_xor(BYTE& reg, BYTE to_xor,
	bool use_immediate)
{
	if (use_immediate) {
		BYTE imm{ read_memory(m_program_counter++) };
		to_xor = imm;
	}

	reg ^= to_xor;

	m_registerAF.lo = 0;

	if (reg == 0)
		m_registerAF.lo |= 1 << FLAG_Z;
}

void Emulator::CPU_8bit_and(BYTE& reg, BYTE to_and,
	bool use_immediate)
{
	if (use_immediate) {
		BYTE imm{ read_memory(m_program_counter++) };
		to_and = imm;
	}

	reg &= to_and;

	m_registerAF.lo = 0;
	m_registerAF.lo |= 1 << FLAG_H;

	if (reg == 0)
		m_registerAF.lo |= 1 << FLAG_Z;
}

void Emulator::CPU_8bit_or(BYTE& reg, BYTE to_or,
	bool use_immediate)
{
	if (use_immediate) {
		BYTE imm{ read_memory(m_program_counter++) };
		to_or = imm;
	}

	reg |= to_or;

	m_registerAF.lo = 0;

	if (reg == 0)
		m_registerAF.lo |= 1 << FLAG_Z;
}

void Emulator::CPU_jump_immediate(bool use_condition, int flag, bool condition) {
	SIGNED_BYTE n{ static_cast<SIGNED_BYTE>(read_memory(m_program_counter)) };

	if (!use_condition)
	{
		m_program_counter += n;
	}
	else if ((m_registerAF.lo & (1 << flag)) == condition)
	{
		m_program_counter += n;
	}

	m_program_counter++;
}

void Emulator::CPU_jump_nn(bool use_condition, int flag, bool condition) {
	WORD nn((read_memory(m_program_counter + 1) << 8) | read_memory(m_program_counter));
	m_program_counter += 2;

	if (!use_condition)
		m_program_counter = nn;
	else if ((m_registerAF.lo & (1 << flag)) == condition)
		m_program_counter = nn;
}

void Emulator::CPU_call(bool use_condition, int flag, bool condition) {
	WORD nn((read_memory(m_program_counter + 1) << 8) | read_memory(m_program_counter));
	m_program_counter += 2;

	if (!use_condition || (use_condition && (m_registerAF.lo & (1 << flag)) == condition)) {
		push_word_onto_stack(m_program_counter);
		m_program_counter = nn;
	}
}

void Emulator::CPU_return(bool use_condition, int flag, bool condition) {
	if (!use_condition || (use_condition && (m_registerAF.lo & (1 << flag)) == condition)) {
		m_program_counter = pop_word_off_stack();
	}
}

/**
	possible bug
*/
void Emulator::CPU_16bit_load() {
	m_registerAF.lo = 0;
	SIGNED_BYTE e{ static_cast<SIGNED_BYTE>(read_memory(m_program_counter++)) };

	if (e >= 0) {
		WORD htest( m_stack_pointer.reg & 0xF );
		htest += e & 0xF;
		if (htest > 0xF)
			m_registerAF.lo |= 1 << FLAG_H;

		if ((m_stack_pointer.reg + e) > 0xFF)
			m_registerAF.lo |= 1 << FLAG_C;
	}
	else {
		SIGNED_WORD htest(m_stack_pointer.reg & 0xF);
		htest -= e & 0xF;
		if (htest < 0)
			m_registerAF.lo |= 1 << FLAG_H;

		if (m_stack_pointer.reg < e)
			m_registerAF.lo |= 1 << FLAG_C;
	}

	m_registerHL.reg = m_stack_pointer.reg + e;
}