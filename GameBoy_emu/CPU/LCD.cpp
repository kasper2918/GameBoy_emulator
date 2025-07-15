#include "Emulator.h"

template< typename T >
T bit_get_val(T in_data, size_t in_bit_position)
{
	T l_msk = 1 << in_bit_position;
	return (in_data & l_msk) ? 1 : 0;
}

void Emulator::update_graphics(int cycles) {
	set_LCD_status();

	if (is_LCD_enabled())
		m_scanline_counter -= cycles;
	else
		return;

	if (m_scanline_counter <= 0)
	{
		// time to move onto next scanline
		m_rom[0xFF44]++;
		BYTE currentline = read_memory(0xFF44);

		m_scanline_counter = 456;

		// we have entered vertical blank period
		if (currentline == 144)
			request_interupt(0);

		// if gone past scanline 153 reset to 0
		else if (currentline > 153)
			m_rom[0xFF44] = 0;

		// draw the current scanline
		else if (currentline < 144)
			draw_scan_line();
	}
}

void Emulator::draw_scan_line() {
	BYTE control = read_memory(0xFF40);
	if (control & 1)
		render_tiles();
	if (control & 0b10)
		render_sprites();
}

void Emulator::render_tiles() {
	WORD tile_data = 0;
	WORD background_memory = 0;
	bool unsig = true;

	// where to draw the visual area and the window
	BYTE scrollY = read_memory(0xFF42);
	BYTE scrollX = read_memory(0xFF43);
	BYTE windowY = read_memory(0xFF4A);
	BYTE windowX = read_memory(0xFF4B) - 7;

	bool using_window = false;

	BYTE lcd_control{ read_memory(0xFF40) };

	// is the window enabled?
	if (lcd_control & (1 << 5)) // possible bug
	{
		// is the current scanline we're drawing
		// within the windows Y pos?,
		if (windowY <= read_memory(0xFF44))
			using_window = true;
	}

	// which tile data are we using?
	if (lcd_control & (1 << 4))
	{
		tile_data = 0x8000;
	}
	else
	{
		// IMPORTANT: This memory region uses signed
		// bytes as tile identifiers
		tile_data = 0x8800;
		unsig = false;
	}

	// which background mem?
	if (!using_window)
	{
		if (lcd_control & (1 << 3))
			background_memory = 0x9C00;
		else
			background_memory = 0x9800;
	}
	else
	{
		// which window memory?
		if (lcd_control & (1 << 6))
			background_memory = 0x9C00;
		else
			background_memory = 0x9800;
	}

	BYTE y_pos{ 0 };

	// yPos is used to calculate which of 32 vertical tiles the
	// current scanline is drawing
	if (!using_window)
		y_pos = scrollY + read_memory(0xFF44);
	else
		y_pos = read_memory(0xFF44) - windowY;

	// which of the 8 vertical pixels of the current
	// tile is the scanline on?
	WORD tileRow(static_cast<BYTE>(y_pos / 8) * 32);

	// time to start drawing the 160 horizontal pixels
	// for this scanline
	for (int pixel = 0; pixel < 160; pixel++)
	{
		BYTE x_pos = pixel + scrollX;

		// translate the current x pos to window space if necessary
		if (using_window)
		{
			if (pixel >= windowX)
			{
				x_pos = pixel - windowX;
			}
		}

		// which of the 32 horizontal tiles does this xPos fall within?
		WORD tileCol = x_pos / 8;
		SIGNED_WORD tileNum;

		// get the tile identity number. Remember it can be signed
		// or unsigned
		WORD tileAddrss = background_memory + tileRow + tileCol;
		if (unsig)
			tileNum = (BYTE)read_memory(tileAddrss);
		else
			tileNum = (SIGNED_BYTE)read_memory(tileAddrss);

		// deduce where this tile identifier is in memory. Remember i
		// shown this algorithm earlier
		WORD tileLocation = tile_data;

		if (unsig)
			tileLocation += (tileNum * 16);
		else
			tileLocation += ((tileNum + 128) * 16);

		// find the correct vertical line we're on of the
		// tile to get the tile data
		// from in memory
		BYTE line = y_pos % 8;
		line *= 2; // each vertical line takes up two bytes of memory
		BYTE data1 = read_memory(tileLocation + line);
		BYTE data2 = read_memory(tileLocation + line + 1);

		// pixel 0 in the tile is it 7 of data 1 and data2.
		// Pixel 1 is bit 6 etc..
		int colourBit = x_pos % 8;
		colourBit -= 7;
		colourBit *= -1;

		// combine data 2 and data 1 to get the colour id for this pixel
		// in the tile
		int colourNum = bit_get_val(data2, colourBit);
		colourNum <<= 1;
		colourNum |= bit_get_val(data1, colourBit);

		// now we have the colour id get the actual
		// colour from palette 0xFF47
		COLOUR col = get_colour(colourNum, 0xFF47);
		int red = 0;
		int green = 0;
		int blue = 0;

		// setup the RGB values
		switch (col)
		{
		case WHITE: red = 255; green = 255; blue = 255; break;
		case LIGHT_GRAY:red = 0xCC; green = 0xCC; blue = 0xCC; break;
		case DARK_GRAY: red = 0x77; green = 0x77; blue = 0x77; break;
		}

		int y = read_memory(0xFF44);

		// safety check to make sure what im about
		// to set is int the 160x144 bounds
		if ((y < 0) || (y > 143) || (pixel < 0) || (pixel > 159))
		{
			continue;
		}

		m_screen_data[pixel][y][0] = red;
		m_screen_data[pixel][y][1] = green;
		m_screen_data[pixel][y][2] = blue;
	}
}

void Emulator::render_sprites() {
	BYTE lcd_control{ read_memory(0xFF40) };
	bool use8x16 = lcd_control & (1 << 2);

	for (int sprite = 0; sprite < 40; sprite++)
	{
		// sprite occupies 4 bytes in the sprite attributes table
		BYTE index = sprite * 4;
		BYTE y_pos = read_memory(0xFE00 + index) - 16;
		BYTE x_pos = read_memory(0xFE00 + index + 1) - 8;
		BYTE tile_location = read_memory(0xFE00 + index + 2);
		BYTE attributes = read_memory(0xFE00 + index + 3);

		bool y_flip = attributes & (1 << 6);
		bool x_flip = attributes & (1 << 5);

		int scanline = read_memory(0xFF44);

		int ysize = use8x16 ? 16 : 8;

		// does this sprite intercept with the scanline?
		if ((scanline >= y_pos) && (scanline < (y_pos + ysize)))
		{
			int line = scanline - y_pos;

			// read the sprite in backwards in the y axis
			if (y_flip)
			{
				line -= ysize;
				line *= -1;
			}

			line *= 2; // same as for tiles
			WORD dataAddress = (0x8000 + (tile_location * 16)) + line;
			BYTE data1 = read_memory(dataAddress);
			BYTE data2 = read_memory(dataAddress + 1);

			// its easier to read in from right to left as pixel 0 is
			// bit 7 in the colour data, pixel 1 is bit 6 etc...
			for (int tile_pixel = 7; tile_pixel >= 0; tile_pixel--)
			{
				int colourbit = tile_pixel;
				// read the sprite in backwards for the x axis
				if (x_flip)
				{
					colourbit -= 7;
					colourbit *= -1;
				}

				// the rest is the same as for tiles
				int colourNum = bit_get_val(data2, colourbit);
				colourNum <<= 1;
				colourNum |= bit_get_val(data1, colourbit);

				WORD colourAddress = attributes & (1 << 4) ? 0xFF49 : 0xFF48;
				COLOUR col = get_colour(colourNum, colourAddress);

				// white is transparent for sprites.
				if (col == WHITE)
					continue;

				int red = 0;
				int green = 0;
				int blue = 0;

				switch (col)
				{
				case WHITE: red = 255; green = 255; blue = 255; break;
				case LIGHT_GRAY: red = 0xCC; green = 0xCC; blue = 0xCC; break;
				case DARK_GRAY: red = 0x77; green = 0x77; blue = 0x77; break;
				}

				int x_pix = 7 - tile_pixel;

				int pixel = x_pos + x_pix;

				// sanity check
				if ((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
				{
					continue;
				}

				m_screen_data[pixel][scanline][0] = red;
				m_screen_data[pixel][scanline][1] = green;
				m_screen_data[pixel][scanline][2] = blue;
			}
		}
	}
}

Emulator::COLOUR Emulator::get_colour(BYTE colour_num, WORD address) const {
	COLOUR res = WHITE;
	BYTE palette = read_memory(address);
	int hi = 0;
	int lo = 0;

	// which bits of the colour palette does the colour id map to?
	switch (colour_num)
	{
	case 0: hi = 1; lo = 0; break;
	case 1: hi = 3; lo = 2; break;
	case 2: hi = 5; lo = 4; break;
	case 3: hi = 7; lo = 6; break;
	}

	// use the palette to get the colour
	int colour = 0;
	colour = bit_get_val(palette, hi) << 1;
	colour |= bit_get_val(palette, lo);

	// convert the game colour to emulator colour
	switch (colour)
	{
	case 0: res = WHITE; break;
	case 1: res = LIGHT_GRAY; break;
	case 2: res = DARK_GRAY; break;
	case 3: res = BLACK; break;
	}

	return res;
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
			status &= ~1u; // possible bug
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