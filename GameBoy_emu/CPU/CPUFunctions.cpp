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

void Emulator::CPU_8bit_inc(BYTE& reg) {
	if (reg + 1 == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	WORD htest = (reg & 0xF);
	++htest;

	if (htest > 0xF)
		m_registerAF.lo |= 1 << FLAG_H;

	m_registerAF.lo &= ~(1 << FLAG_N);

	++reg;
}

void Emulator::CPU_8bit_incHL() {
	BYTE data{ read_memory(m_registerHL.reg) };
	if (data + 1 == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	WORD htest = (data & 0xF);
	++htest;

	if (htest > 0xF)
		m_registerAF.lo |= 1 << FLAG_H;

	m_registerAF.lo &= ~(1 << FLAG_N);

	write_memory(m_registerHL.reg, ++data);
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

void Emulator::CPU_8bit_cmp(BYTE reg, BYTE subtracting,
	bool use_immediate)
{
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

	m_registerAF.lo = 0;

	if (reg - to_subtract == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	m_registerAF.lo |= 1 << FLAG_N;

	// set if no borrow
	if (reg < to_subtract)
		m_registerAF.lo |= 1 << FLAG_C;

	SIGNED_WORD htest = (reg & 0xF);
	htest -= (to_subtract & 0xF);

	if (htest < 0)
		m_registerAF.lo |= 1 << FLAG_H;
}

void Emulator::CPU_8bit_dec(BYTE& reg) {
	if (reg - 1 == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	m_registerAF.lo |= 1 << FLAG_N;

	SIGNED_WORD htest = (reg & 0xF);
	--htest;

	if (htest < 0)
		m_registerAF.lo |= 1 << FLAG_H;

	--reg;
}

void Emulator::CPU_8bit_decHL() {
	BYTE data{ read_memory(m_registerHL.reg) };

	if (data - 1 == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	m_registerAF.lo |= 1 << FLAG_N;

	SIGNED_WORD htest = (data & 0xF);
	--htest;
	if (htest < 0)
		m_registerAF.lo |= 1 << FLAG_H;

	write_memory(m_registerHL.reg, ++data);
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

bool Emulator::CPU_jump_immediate(bool use_condition, int flag, bool condition) {
	SIGNED_BYTE n{ static_cast<SIGNED_BYTE>(read_memory(m_program_counter)) };

	if (!use_condition)
	{
		m_program_counter += n;
	}
	else if ((m_registerAF.lo & (1 << flag)) == condition)
	{
		m_program_counter += n;
		return true;
	}

	m_program_counter++;

	return false;
}

bool Emulator::CPU_jump_nn(bool use_condition, int flag, bool condition) {
	WORD nn((read_memory(m_program_counter + 1) << 8) | read_memory(m_program_counter));
	m_program_counter += 2;

	if (!use_condition)
		m_program_counter = nn;
	else if ((m_registerAF.lo & (1 << flag)) == condition) {
		m_program_counter = nn;
		return true;
	}
	
	return false;
}

bool Emulator::CPU_call(bool use_condition, int flag, bool condition) {
	WORD nn((read_memory(m_program_counter + 1) << 8) | read_memory(m_program_counter));
	m_program_counter += 2;

	if (!use_condition || (use_condition && (m_registerAF.lo & (1 << flag)) == condition)) {
		push_word_onto_stack(m_program_counter);
		m_program_counter = nn;
		return true;
	}

	return false;
}

bool Emulator::CPU_return(bool use_condition, int flag, bool condition) {
	if (!use_condition || (use_condition && (m_registerAF.lo & (1 << flag)) == condition)) {
		m_program_counter = pop_word_off_stack();
		return true;
	}

	return false;
}

// STOLEN FROM GUY WHO ALSO STOLE IT
void Emulator::CPU_DAA()
{

	if (m_registerAF.lo & (1 << FLAG_N))
	{
		if ((m_registerAF.hi & 0x0F) > 0x09 || m_registerAF.lo & 0x20)
		{
			m_registerAF.hi -= 0x06; //Half borrow: (0-1) = (0xF-0x6) = 9
			if ((m_registerAF.hi & 0xF0) == 0xF0) 
				m_registerAF.lo |= 0x10;
			else
				m_registerAF.lo &= ~0x10;
		}

		if ((m_registerAF.hi & 0xF0) > 0x90 || m_registerAF.lo & 0x10)
			m_registerAF.hi -= 0x60;
	}
	else
	{
		if ((m_registerAF.hi & 0x0F) > 9 || m_registerAF.lo & 0x20)
		{
			m_registerAF.hi += 0x06; //Half carry: (9+1) = (0xA+0x6) = 10
			if ((m_registerAF.hi & 0xF0) == 0)
				m_registerAF.lo |= 0x10;
			else
				m_registerAF.lo &= ~0x10;
		}

		if ((m_registerAF.hi & 0xF0) > 0x90 || m_registerAF.lo & 0x10)
			m_registerAF.hi += 0x60;
	}

	if (m_registerAF.hi == 0) 
		m_registerAF.lo |= 0x80;
	else
		m_registerAF.lo &= ~0x80;
}

// possible bug
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

void Emulator::CPU_16bit_add(WORD regis) {
	WORD htest = (m_registerHL.reg & 0xFFF);
	htest += (regis & 0xFFF);
	if (htest > 0xFFF)
		m_registerAF.lo |= 1 << FLAG_H;

	if ((m_registerHL.reg + regis) > 0xFFFF)
		m_registerAF.lo |= 1 << FLAG_C;
	
	m_registerAF.lo &= ~(1 << FLAG_N);

	m_registerHL.reg += regis;
}

// possible bug
void Emulator::CPU_16bit_addSP() {
	m_registerAF.lo = 0;
	SIGNED_BYTE e{ static_cast<SIGNED_BYTE>(read_memory(m_program_counter++)) };

	if (e >= 0) {
		WORD htest(m_stack_pointer.reg & 0xF);
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

	m_stack_pointer.reg += e;
}

BYTE Emulator::CPU_RLC(BYTE data, bool isA) {
	m_registerAF.lo = 0;

	auto bit7{ (data & 0x80) >> 7 };
	m_registerAF.lo |= bit7 << FLAG_C;
	data = bit7 | (data << 1);

	if (!isA && data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	return data;
}

// possible bug
BYTE Emulator::CPU_RRC(BYTE data, bool isA) {
	m_registerAF.lo = 0;

	auto bit0{ data & 1 };
	m_registerAF.lo |= bit0 << FLAG_C;
	data = (bit0 << 7) | (data >> 1);

	if (!isA && data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	return data;
}

BYTE Emulator::CPU_RL(BYTE data, bool isA) {
	m_registerAF.lo = 0;

	auto bit7{ (data & 0x80) >> 7 };
	data = (data << 1) | ((m_registerAF.lo & (1 << FLAG_C)) >> FLAG_C);
	m_registerAF.lo |= bit7 << FLAG_C;

	if (!isA && data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	return data;
}

BYTE Emulator::CPU_RR(BYTE data, bool isA) {
	m_registerAF.lo = 0;

	auto bit0{ data & 1 };
	data = (data >> 1) | ((m_registerAF.lo & (1 << FLAG_C)) << (7 - FLAG_C));
	m_registerAF.lo |= bit0 << FLAG_C;

	if (!isA && data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	return data;
}

BYTE Emulator::CPU_SLA(BYTE data) {
	m_registerAF.lo = 0;

	auto bit7{ (data & 0x80) >> 7 };
	m_registerAF.lo |= bit7 << FLAG_C;
	data <<= 1;

	if (data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	return data;
}

// possible bug
BYTE Emulator::CPU_SRA(BYTE data, bool isHL) {
	m_registerAF.lo = 0;

	auto bit0{ data & 1 };
	auto bit7{ data & 0x80 };
	m_registerAF.lo |= bit0 << FLAG_C;
	data >>= 1;
	if (!isHL)
		data |= bit7;

	if (data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	return data;
}

BYTE Emulator::CPU_swap(BYTE data) {
	BYTE lo{ data & 0x0F };
	BYTE hi{ data & 0xF0 };
	data = lo << 4 & hi >> 4;

	m_registerAF.lo = 0;
	if (data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;
}

BYTE Emulator::CPU_SRL(BYTE data) {
	m_registerAF.lo = 0;

	auto bit0{ data & 1 };
	m_registerAF.lo |= bit0 << FLAG_C;
	data >>= 1;

	if (data == 0)
		m_registerAF.lo |= 1 << FLAG_Z;

	return data;
}

void Emulator::CPU_bit(BYTE data, BYTE bit) {
	m_registerAF.lo = ((data & (1 << bit)) >> bit) << FLAG_Z;
	m_registerAF.lo |= 1 << FLAG_H;
	m_registerAF.lo &= ~(1 << FLAG_N);
}