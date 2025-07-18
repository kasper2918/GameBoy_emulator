#include "Emulator.h"
#include "Misc/BitOps.h"

void Emulator::CPU_8BIT_LOAD(BYTE& reg)
{
	BYTE n = ReadMemory(m_ProgramCounter);
	m_ProgramCounter++;
	reg = n;
}

void Emulator::CPU_8BIT_ADD(BYTE& reg, BYTE toAdd,
	bool useImmediate, bool addCarry)
{
	BYTE before = reg;
	BYTE adding = 0;

	// are we adding immediate data or the second param?
	if (useImmediate)
	{
		BYTE n = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
		adding = n;
	}
	else
	{
		adding = toAdd;
	}

	// are we also adding the carry flag?
	if (addCarry)
	{
		if (TestBit(m_RegisterAF.lo, FLAG_C))
			adding++;
	}

	reg += adding;

	// set the flags
	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	WORD htest = (before & 0xF);
	htest += (adding & 0xF);

	if (htest > 0xF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);

	if ((before + adding) > 0xFF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
}

void Emulator::CPU_16BIT_LOAD()
{
	SIGNED_BYTE n = ReadMemory(m_ProgramCounter++);
	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);
	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);


	WORD value = (m_StackPointer.reg + n) & 0xFFFF;

	m_RegisterHL.reg = value;
	unsigned int v = m_StackPointer.reg + n;

	if (v > 0xFFFF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_C);

	if ((m_StackPointer.reg & 0xF) + (n & 0xF) > 0xF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}