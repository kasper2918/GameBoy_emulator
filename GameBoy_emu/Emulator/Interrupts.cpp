#include "Emulator.h"
#include "Misc/BitOps.h"

void Emulator::RequestInterupt(int id)
{
	BYTE req = ReadMemory(0xFF0F);
	req = BitSet(req, id);
	WriteMemory(0xFF0F, id);
}

void Emulator::DoInterupts()
{
	// I'm too lazy to refactor this
	if (m_InteruptMaster == true)
	{
		BYTE req = ReadMemory(0xFF0F);
		BYTE enabled = ReadMemory(0xFFFF);
		if (req > 0)
		{
			for (int i = 0; i < 5; i++)
			{
				if (TestBit(req, i) == true)
				{
					if (TestBit(enabled, i))
						ServiceInterupt(i);
				}
			}
		}
	}
}

void Emulator::ServiceInterupt(int interupt)
{
	m_InteruptMaster = false;
	BYTE req = ReadMemory(0xFF0F);
	req = BitReset(req, interupt);
	WriteMemory(0xFF0F, req);

	/// we must save the current execution address by pushing it onto the stack
	PushWordOntoStack(m_ProgramCounter);

	switch (interupt)
	{
	case 0: m_ProgramCounter = 0x40; break;
	case 1: m_ProgramCounter = 0x48; break;
	case 2: m_ProgramCounter = 0x50; break;
	case 4: m_ProgramCounter = 0x60; break;
	}
}