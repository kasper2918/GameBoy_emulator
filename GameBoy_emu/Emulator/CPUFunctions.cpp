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

void Emulator::CPU_8BIT_SUB(BYTE& reg, BYTE subtracting,
	bool useImmediate, bool subCarry)
{
	BYTE before = reg;
	BYTE toSubtract = 0;

	if (useImmediate)
	{
		BYTE n = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
		toSubtract = n;
	}
	else
	{
		toSubtract = subtracting;
	}

	if (subCarry)
	{
		if (TestBit(m_RegisterAF.lo, FLAG_C))
			toSubtract++;
	}

	reg -= toSubtract;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_N);

	// set if no borrow
	if (before < toSubtract)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	SIGNED_WORD htest = (before & 0xF);
	htest -= (toSubtract & 0xF);

	if (htest < 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_CP(BYTE& reg, BYTE subtracting, bool useImmediate)
{
	BYTE toSubtract = 0;

	if (useImmediate)
	{
		BYTE n = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
		toSubtract = n;
	}
	else
	{
		toSubtract = subtracting;
	}

	m_RegisterAF.lo = 0;

	if (reg - toSubtract == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_N);

	// set if no borrow
	if (reg < toSubtract)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	SIGNED_WORD htest = (reg & 0xF);
	htest -= (toSubtract & 0xF);

	if (htest < 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_INC(BYTE& reg)
{
	BYTE before = reg;

	reg++;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);

	if ((before & 0xF) == 0xF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_DEC(BYTE& reg)
{
	BYTE before = reg;

	reg--;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);

	m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_N);

	if ((before & 0x0F) == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_8BIT_AND(BYTE& reg, BYTE toAnd, bool useImmediate)
{
	BYTE myand = 0;

	if (useImmediate)
	{
		BYTE n = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
		myand = n;
	}
	else
	{
		myand = toAnd;
	}

	reg &= myand;

	m_RegisterAF.lo = 0;

	F = BitSet(F, FLAG_H);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_8BIT_OR(BYTE& reg, BYTE toOr, bool useImmediate)
{
	BYTE myor = 0;

	if (useImmediate)
	{
		BYTE n = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
		myor = n;
	}
	else
	{
		myor = toOr;
	}

	reg |= myor;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_8BIT_XOR(BYTE& reg, BYTE toXOr, bool useImmediate)
{
	BYTE myxor = 0;

	if (useImmediate)
	{
		BYTE n = ReadMemory(m_ProgramCounter);
		m_ProgramCounter++;
		myxor = n;
	}
	else
	{
		myxor = toXOr;
	}

	reg ^= myxor;

	m_RegisterAF.lo = 0;

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

// STOLEN FROM A GUY WHO ALSO STOLE IT
void Emulator::CPU_DAA()
{
	if (TestBit(m_RegisterAF.lo, FLAG_N))
	{
		if ((m_RegisterAF.hi & 0x0F) > 0x09 || m_RegisterAF.lo & 0x20)
		{
			m_RegisterAF.hi -= 0x06; //Half borrow: (0-1) = (0xF-0x6) = 9
			if ((m_RegisterAF.hi & 0xF0) == 0xF0)
				m_RegisterAF.lo |= 0x10; 
			else 
				m_RegisterAF.lo &= ~0x10;
		}

		if ((m_RegisterAF.hi & 0xF0) > 0x90 || m_RegisterAF.lo & 0x10)
			m_RegisterAF.hi -= 0x60;
	}
	else
	{
		if ((m_RegisterAF.hi & 0x0F) > 9 || m_RegisterAF.lo & 0x20)
		{
			m_RegisterAF.hi += 0x06; //Half carry: (9+1) = (0xA+0x6) = 10
			if ((m_RegisterAF.hi & 0xF0) == 0)
				m_RegisterAF.lo |= 0x10;
			else m_RegisterAF.lo &= ~0x10;
		}

		if ((m_RegisterAF.hi & 0xF0) > 0x90 || m_RegisterAF.lo & 0x10)
			m_RegisterAF.hi += 0x60;
	}

	if (m_RegisterAF.hi == 0)
		m_RegisterAF.lo |= 0x80;
	else m_RegisterAF.lo &= ~0x80;
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

void Emulator::CPU_16BIT_ADD(WORD& reg, WORD toAdd)
{
	WORD before = reg;

	reg += toAdd;

	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);

	if ((before + toAdd) > 0xFFFF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_C);


	if ((before & 0xFFF) + (toAdd & 0xFFF) > 0xFFF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);
}

void Emulator::CPU_16BIT_ADD_SP()
{
	SIGNED_BYTE n = ReadMemory(m_ProgramCounter++);
	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_Z);
	m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_N);

	unsigned int v = m_StackPointer.reg + n;

	if (v > 0xFFFF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_C);

	if ((m_StackPointer.reg & 0xF) + (n & 0xF) > 0xF)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_H);
	else
		m_RegisterAF.lo = BitReset(m_RegisterAF.lo, FLAG_H);

	SP += n;
}

void Emulator::CPU_RLC(BYTE& reg, bool isA)
{
	bool isMSBSet = TestBit(reg, 7);

	m_RegisterAF.lo = 0;

	reg <<= 1;

	if (isMSBSet)
	{
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
		reg = BitSet(reg, 0);
	}

	if (!isA && reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

}

void Emulator::CPU_RRC(BYTE& reg, bool isA)
{
	bool isLSBSet = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (isLSBSet)
	{
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);
		reg = BitSet(reg, 7);
	}

	if (!isA && reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_RL(BYTE& reg, bool isA)
{
	bool isCarrySet = TestBit(m_RegisterAF.lo, FLAG_C);
	bool isMSBSet = TestBit(reg, 7);

	m_RegisterAF.lo = 0;

	reg <<= 1;

	if (isMSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (isCarrySet)
		reg = BitSet(reg, 0);

	if (!isA && reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_RR(BYTE& reg, bool isA)
{
	bool isCarrySet = TestBit(m_RegisterAF.lo, FLAG_C);
	bool isLSBSet = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (isLSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (isCarrySet)
		reg = BitSet(reg, 7);

	if (!isA && reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_SLA(BYTE& reg)
{
	bool isMSBSet = TestBit(reg, 7);

	reg <<= 1;

	m_RegisterAF.lo = 0;

	if (isMSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_SRA(BYTE& reg)
{
	bool isLSBSet = TestBit(reg, 0);
	bool isMSBSet = TestBit(reg, 7);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (isMSBSet)
		reg = BitSet(reg, 7);
	if (isLSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);
}

void Emulator::CPU_SRL(BYTE& reg)
{
	bool isLSBSet = TestBit(reg, 0);

	m_RegisterAF.lo = 0;

	reg >>= 1;

	if (isLSBSet)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_C);

	if (reg == 0)
		m_RegisterAF.lo = BitSet(m_RegisterAF.lo, FLAG_Z);

}

void Emulator::CPU_JUMP(bool useCondition, int flag, bool condition)
{
	WORD nn = get_nn();

	if (!useCondition)
	{
		m_ProgramCounter = nn;
		return;
	}

	if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		m_ProgramCounter = nn;
	}

}

void Emulator::CPU_JUMP_IMMEDIATE(bool useCondition, int flag, bool condition)
{
	SIGNED_BYTE n = (SIGNED_BYTE)ReadMemory(m_ProgramCounter);

	if (!useCondition)
	{
		m_ProgramCounter += n;
	}
	else if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		m_ProgramCounter += n;
	}

	m_ProgramCounter++;
}

void Emulator::CPU_CALL(bool useCondition, int flag, bool condition)
{
	WORD nn = get_nn();

	if (!useCondition)
	{
		PushWordOntoStack(m_ProgramCounter);
		m_ProgramCounter = nn;
		return;
	}

	if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		PushWordOntoStack(m_ProgramCounter);
		m_ProgramCounter = nn;
	}
}

void Emulator::CPU_RETURN(bool useCondition, int flag, bool condition)
{
	if (!useCondition)
	{
		m_ProgramCounter = PopWordOffStack();
		return;
	}

	if (TestBit(m_RegisterAF.lo, flag) == condition)
	{
		m_ProgramCounter = PopWordOffStack();
	}
}