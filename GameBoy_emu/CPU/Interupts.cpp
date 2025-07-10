#include "Emulator.h"

void Emulator::request_interupt(int id) {
    BYTE req = read_memory(0xFF0F);
    req |= 1 << id; // possible bug
    write_memory(0xFF0F, req);
}

void Emulator::do_interupts() {
    if (m_interupt_master == true)
    {
        BYTE req = read_memory(0xFF0F);
        BYTE enabled = read_memory(0xFFFF);
        if (req > 0)
        {
            for (int i = 0; i < 5; i++)
            {
                if ((req & (1 << i)) && (enabled & (1 << i))) // possible bug
                {
                    service_interupt(i);
                }
            }
        }
    }
}

void Emulator::service_interupt(int interupt) {
    m_interupt_master = false;
    BYTE req = read_memory(0xFF0F);
    req &= ~(1 << interupt);
    write_memory(0xFF0F, req);

    /// we must save the current execution address by pushing it onto the stack
    m_rom[m_stack_pointer.reg--] = m_program_counter;

    switch (interupt)
    {
    case 0: m_program_counter = 0x40; break;
    case 1: m_program_counter = 0x48; break;
    case 2: m_program_counter = 0x50; break; // 2 = 10
    case 4: m_program_counter = 0x60; break; // 4 = 100
    }
}