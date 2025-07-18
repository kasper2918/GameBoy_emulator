#include "../Emulator.h"

void Emulator::PushWordOntoStack(WORD word)
{
	BYTE hi = word >> 8;
	BYTE lo = word & 0xFF;
	m_StackPointer.reg--;
	WriteMemory(m_StackPointer.reg, hi);
	m_StackPointer.reg--;
	WriteMemory(m_StackPointer.reg, lo);
}

WORD Emulator::PopWordOffStack()
{
	WORD word = ReadMemory(m_StackPointer.reg + 1) << 8;
	word |= ReadMemory(m_StackPointer.reg);
	m_StackPointer.reg += 2;

	return word;
}
