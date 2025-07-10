#include "Emulator.h"

void Emulator::update_graphics(int cycles) {
    // TODO: implement this
}

void Emulator::draw_scan_line() {

}

void Emulator::set_LCD_status() {
    BYTE status = read_memory(0xFF41);
    if (!is_LCD_enabled())
    {
        // set the mode to 1 during lcd disabled and reset scanline
        m_scanline_counter = 456;
        m_rom[0xFF44] = 0;
        status &= 252;
        status |= 1; // possible bug
        write_memory(0xFF41, status);
        return;
    }

    BYTE currentline = read_memory(0xFF44);
    BYTE currentmode = status & 0x3;

    BYTE mode = 0;
    bool req_int = false;

    // in vblank so set mode to 1
    if (currentline >= 144)
    {
        mode = 1;
        status |= 1;
        status &= ~(1u << 1u); // possible bug
        req_int = status & (1u << 4u);
    }
    else
    {
        int mode2bounds = 456 - 80;
        int mode3bounds = mode2bounds - 172;

        // mode 2
        if (m_scanline_counter >= mode2bounds)
        {
            mode = 2;
            status |= 0b10;
            status &= ~1u; // po
            req_int = status & (1 << 5);
        }
        // mode 3
        else if (m_scanline_counter >= mode3bounds)
        {
            mode = 3;
            status |= 0b10;
            status |= 1;
        }
        // mode 0
        else
        {
            mode = 0;
            status &= ~(0b10u);
            status &= ~(0b1u);
            req_int = status & (1 << 3);
        }
    }

    // just entered a new mode so request interupt
    if (req_int && (mode != currentmode))
        request_interupt(1);

    // check the conincidence flag
    if (currentline == read_memory(0xFF45))
    {
        status |= 1 << 2;
        if (status & (1 << 6))
            request_interupt(1);
    }
    else
    {
        status &= ~(1u << 2u);
    }
    write_memory(0xFF41, status);
}

bool Emulator::is_LCD_enabled() const {
	return read_memory(0xFF40) & (1 << 7); // possible bug
}