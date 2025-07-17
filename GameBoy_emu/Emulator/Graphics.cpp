#include "Emulator.h"
#include "Misc/BitOps.h"

void Emulator::UpdateGraphics(int cycles)
{
    SetLCDStatus();

    if (IsLCDEnabled())
        m_ScanlineCounter -= cycles;
    else
        return;

    if (m_ScanlineCounter <= 0)
    {
        // time to move onto next scanline
        m_Rom[0xFF44]++;
        BYTE currentline = ReadMemory(0xFF44);

        m_ScanlineCounter = 456;

        // we have entered vertical blank period
        if (currentline == 144)
            RequestInterupt(0);

        // if gone past scanline 153 reset to 0
        else if (currentline > 153)
            m_Rom[0xFF44] = 0;

            // draw the current scanline
        else if (currentline < 144)
            DrawScanLine();
    }
}

void Emulator::DrawScanLine()
{
    BYTE control = ReadMemory(0xFF40);
    if (TestBit(control, 0))
        RenderTiles(control);

    if (TestBit(control, 1))
        RenderSprites(control);
}

void Emulator::RenderTiles(BYTE lcdControl)
{
    WORD tileData = 0;
    WORD backgroundMemory = 0;
    bool unsig = true;

    // where to draw the visual area and the window
    BYTE scrollY = ReadMemory(0xFF42);
    BYTE scrollX = ReadMemory(0xFF43);
    BYTE windowY = ReadMemory(0xFF4A);
    BYTE windowX = ReadMemory(0xFF4B) - 7;

    bool usingWindow = false;

    // is the window enabled?
    if (TestBit(lcdControl, 5))
    {
        // is the current scanline we're drawing
        // within the windows Y pos?,
        if (windowY <= ReadMemory(0xFF44))
            usingWindow = true;
    }

    // which tile data are we using?
    if (TestBit(lcdControl, 4))
    {
        tileData = 0x8000;
    }
    else
    {
        // IMPORTANT: This memory region uses signed
        // bytes as tile identifiers
        tileData = 0x8800;
        unsig = false;
    }

    // which background mem?
    if (false == usingWindow)
    {
        if (TestBit(lcdControl, 3))
            backgroundMemory = 0x9C00;
        else
            backgroundMemory = 0x9800;
    }
    else
    {
        // which window memory?
        if (TestBit(lcdControl, 6))
            backgroundMemory = 0x9C00;
        else
            backgroundMemory = 0x9800;
    }

    BYTE yPos = 0;

    // yPos is used to calculate which of 32 vertical tiles the
    // current scanline is drawing
    if (!usingWindow)
        yPos = scrollY + ReadMemory(0xFF44);
    else
        yPos = ReadMemory(0xFF44) - windowY;

    // which of the 8 vertical pixels of the current
    // tile is the scanline on?
    WORD tileRow = (((BYTE)(yPos / 8)) * 32);

    // time to start drawing the 160 horizontal pixels
    // for this scanline
    for (int pixel = 0; pixel < 160; pixel++)
    {
        BYTE xPos = pixel + scrollX;

        // translate the current x pos to window space if necessary
        if (usingWindow)
        {
            if (pixel >= windowX)
            {
                xPos = pixel - windowX;
            }
        }

        // which of the 32 horizontal tiles does this xPos fall within?
        WORD tileCol = (xPos / 8);
        SIGNED_WORD tileNum;

        // get the tile identity number. Remember it can be signed
        // or unsigned
        WORD tileAddrss = backgroundMemory + tileRow + tileCol;
        if (unsig)
            tileNum = (BYTE)ReadMemory(tileAddrss);
        else
            tileNum = (SIGNED_BYTE)ReadMemory(tileAddrss);

        // deduce where this tile identifier is in memory. Remember i
        // shown this algorithm earlier
        WORD tileLocation = tileData;

        if (unsig)
            tileLocation += (tileNum * 16);
        else
            tileLocation += ((tileNum + 128) * 16);

        // find the correct vertical line we're on of the
        // tile to get the tile data
        //from in memory
        BYTE line = yPos % 8;
        line *= 2; // each vertical line takes up two bytes of memory
        BYTE data1 = ReadMemory(tileLocation + line);
        BYTE data2 = ReadMemory(tileLocation + line + 1);

        // pixel 0 in the tile is it 7 of data 1 and data2.
        // Pixel 1 is bit 6 etc..
        int colourBit = xPos % 8;
        colourBit -= 7;
        colourBit *= -1;

        // combine data 2 and data 1 to get the colour id for this pixel
        // in the tile
        int colourNum = BitGetVal(data2, colourBit);
        colourNum <<= 1;
        colourNum |= BitGetVal(data1, colourBit);

        // now we have the colour id get the actual
        // colour from palette 0xFF47
        COLOUR col = GetColour(colourNum, 0xFF47);
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

        int finaly = ReadMemory(0xFF44);

        // safety check to make sure what im about
        // to set is int the 160x144 bounds
        if ((finaly < 0) || (finaly > 143) || (pixel < 0) || (pixel > 159))
        {
            continue;
        }

        m_ScreenData[pixel][finaly][0] = red;
        m_ScreenData[pixel][finaly][1] = green;
        m_ScreenData[pixel][finaly][2] = blue;
    }
}

void Emulator::RenderSprites(BYTE lcdControl)
{
    bool use8x16 = false;
    if (TestBit(lcdControl, 2))
        use8x16 = true;

    for (int sprite = 0; sprite < 40; sprite++)
    {
        // sprite occupies 4 bytes in the sprite attributes table
        BYTE index = sprite * 4;
        BYTE yPos = ReadMemory(0xFE00 + index) - 16;
        BYTE xPos = ReadMemory(0xFE00 + index + 1) - 8;
        BYTE tileLocation = ReadMemory(0xFE00 + index + 2);
        BYTE attributes = ReadMemory(0xFE00 + index + 3);

        bool yFlip = TestBit(attributes, 6);
        bool xFlip = TestBit(attributes, 5);

        int scanline = ReadMemory(0xFF44);

        int ysize = 8;
        if (use8x16)
            ysize = 16;

        // does this sprite intercept with the scanline?
        if ((scanline >= yPos) && (scanline < (yPos + ysize)))
        {
            int line = scanline - yPos;

            // read the sprite in backwards in the y axis
            if (yFlip)
            {
                line -= ysize;
                line *= -1;
            }

            line *= 2; // same as for tiles
            WORD dataAddress = (0x8000 + (tileLocation * 16)) + line;
            BYTE data1 = ReadMemory(dataAddress);
            BYTE data2 = ReadMemory(dataAddress + 1);

            // its easier to read in from right to left as pixel 0 is
            // bit 7 in the colour data, pixel 1 is bit 6 etc...
            for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
            {
                int colourbit = tilePixel;
                // read the sprite in backwards for the x axis
                if (xFlip)
                {
                    colourbit -= 7;
                    colourbit *= -1;
                }

                // the rest is the same as for tiles
                int colourNum = BitGetVal(data2, colourbit);
                colourNum <<= 1;
                colourNum |= BitGetVal(data1, colourbit);

                WORD colourAddress = TestBit(attributes, 4) ? 0xFF49 : 0xFF48;
                COLOUR col = GetColour(colourNum, colourAddress);

                // white is transparent for sprites.
                if (col == WHITE)
                    continue;

                int red = 0;
                int green = 0;
                int blue = 0;

                switch (col)
                {
                case WHITE: red = 255; green = 255; blue = 255; break;
                case LIGHT_GRAY:red = 0xCC; green = 0xCC; blue = 0xCC; break;
                case DARK_GRAY:red = 0x77; green = 0x77; blue = 0x77; break;
                }

                int xPix = 0 - tilePixel;
                xPix += 7;

                int pixel = xPos + xPix;

                // sanity check
                if ((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
                {
                    continue;
                }

                m_ScreenData[pixel][scanline][0] = red;
                m_ScreenData[pixel][scanline][1] = green;
                m_ScreenData[pixel][scanline][2] = blue;
            }
        }
    }
}

Emulator::COLOUR Emulator::GetColour(BYTE colourNum, WORD address) const
{
    COLOUR res = WHITE;
    BYTE palette = ReadMemory(address);
    int hi = 0;
    int lo = 0;

    // which bits of the colour palette does the colour id map to?
    switch (colourNum)
    {
    case 0: hi = 1; lo = 0; break;
    case 1: hi = 3; lo = 2; break;
    case 2: hi = 5; lo = 4; break;
    case 3: hi = 7; lo = 6; break;
    }

    // use the palette to get the colour
    int colour = 0;
    colour = BitGetVal(palette, hi) << 1;
    colour |= BitGetVal(palette, lo);

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