#include <stdint.h>

#include <PPU.h>
#include <Console.h>
#include <Graphics.h>

PPU::PPU(Console *console) {
    this->console = console;
}

void PPU::boot() {
    // 0x2000: CTRL
    nameTableAddr = 0;              // 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    vRAMAddrIncr = false;           // 0=1 (across name table), 1=32 (down name table)
    sprPatternTableAddr = false;    // 0=0x0000, 1=0x1000, ignored if sprSize is 8x16
    bgPatternTableAddr = false;     // 0=0x0000, 1=0x1000
    sprSize = false;                // 0=8x8, 1=8x16
    // unused bit
    nmiOnVBlank = false;

    // 0x2001: MASK
    grayscale = false;              // mirrors palette to only shades of grey
    imageMask = false;              // show bg in left 8 columns of screen
    sprMask = false;                // show sprites in left 8 columns of screen
    showBg = false;
    showSpr = false;
    buffRed = false;                // dims non red colours
    buffGreen = false;              // dims non green colours
    buffBlue = false;               // dims non blue colours

    // 0x2002: STATUS
    // 5 unused bits
    sprOverflow = false;            // if > 8 sprites on any scanline in current frame
    spr0Hit = false;                // at first non-zero pixel overlap in both spr0 and bg
    isVBlank = true;                // unset when read

    // 0x2003: OAMADDR
    oamAddrBuffer = 0x00;

    // 0x2004: OAMDATA
    oamReadBuffer = 0x00;

    // 0x2005: SCROLL
    scrollX = 0x00;
    scrollY = 0x00;

    // 0x2006: ADDR
    addrBuffer = 0x0000;

    // 0x2007: DATA
    readBuffer = 0x00;

    // latch used for SCROLL, ADDR. Unset upon STATUS read
    latch = false;

    // palette for background and sprite tiles
    palette.init(console->getPalettePointer());

    // background metatiles
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 8; ++x) {
            for (int y = 0; y < 8; ++y) {
                // offset in nametable where attribute data is located
                uint16_t offset = 0x3C0 + (0x400 * nt) + (y * 8) + x;
                // init
                bgMetaTiles[nt][x][y].init(
                    console->getNameTablePointer() + offset);
            }
        }
    }
    // background tiles
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 32; ++x) {
            for (int y = 0; y < 30; ++y) {
                // offset in name table where data is located
                uint16_t offset = (0x400 * nt) + x + (y * 32);
                // position within enclosing metatile
                uint8_t quadrant = 0;
                if ((x % 4) >= 2) {
                    quadrant |= 0b01;
                }
                if ((y % 4) >= 2) {
                    quadrant |= 0b10;
                }
                // coordinates of parent metatile
                uint8_t mtX = x / 4;
                uint8_t mtY = y / 4;
                // init
                bgTiles[nt][x][y].init(
                    &bgMetaTiles[nt][mtX][mtY],
                    quadrant,
                    console->getNameTablePointer() + offset,
                    console->getPatternTablePointer(),
                    &(bgPatternTableAddr));
            }
        }
    }
    // background pixels
    for (int x = 0; x < 512; ++x) {
        for (int y = 0; y < 480; ++y) {
            // find tile to which pixel belongs
            int nt = 0;
            if (x >= 256) {
                nt += 0b01;
            }
            if (y >= 240) {
                nt += 0b10;
            }
            int tileX = (x % 256) / 8;
            int tileY = (y % 240) / 8;
            // init
            bgPixels[x][y].init(
                &bgTiles[nt][tileX][tileY],
                x % 8,
                y % 8);
        }
    }

    // sprite objects
    for (int i = 0; i < 64; i++) {
        sprites[i].init(
            console->getSprRamPointer() + (i * 4),
            console->getPatternTablePointer(),
            &(sprSize),
            &(sprPatternTableAddr));
    }

    clockCounter = VBLANK;
    oddFrame = false;
}

void PPU::setCTRL(uint8_t value) {
    nameTableAddr = value & 0x03;
    vRAMAddrIncr = !!(value & 0x04);
    sprPatternTableAddr = !!(value & 0x08);
    bgPatternTableAddr = !!(value & 0x10);
    sprSize = !!(value & 0x20);
    nmiOnVBlank = !!(value & 0x80);
}

void PPU::setMASK(uint8_t value) {
    grayscale = !!(value & 0x01);
    imageMask = !!(value & 0x02);
    sprMask = !!(value & 0x04);
    showBg = !!(value & 0x08);
    showSpr = !!(value & 0x10);
    buffRed = !!(value & 0x20);
    buffGreen = !!(value & 0x40);
    buffBlue = !!(value & 0x80);
}

uint8_t PPU::getSTATUS() {
    uint8_t result = 0x00;
    if (sprOverflow) {
        result |= 0x20;
    }
    if (spr0Hit) {
        result |= 0x40;
    }
    if (isVBlank) {
        result |= 0x80;
    }
    isVBlank = false;
    latch = false;
    return result;
}

void PPU::setOAMADDR(uint8_t value) {
    oamAddrBuffer = value;
}

void PPU::setOAMDATA(uint8_t value) {
    oamReadBuffer = value;
    console->oamWrite(oamAddrBuffer++, value);
}

uint8_t PPU::getOAMDATA() {
    // apparently unreliable in NES hardware, but some games use it
    return console->oamRead(oamAddrBuffer);
}

void PPU::setSCROLL(uint8_t value) {
    if (!latch) {
        scrollX = value;
    }
    else {
        scrollY = value;
    }
    latch = !latch;
}

void PPU::setADDR(uint8_t value) {
    if (!latch) {
        // set high byte
        addrBuffer &= 0x00FF;
        addrBuffer |= ((uint16_t)value << 8);
    }
    else {
        // set low byte
        addrBuffer &= 0xFF00;
        addrBuffer |= value;
    }
    latch = !latch;
}

void PPU::setDATA(uint8_t value) {
    console->ppuWrite(addrBuffer, value);
    addrBuffer += ((vRAMAddrIncr) ? 32 : 1);
}

uint8_t PPU::getDATA() {
    uint8_t returnVal;
    if (addrBuffer < 0x3F00) {
        // return what's in the read buffer from the previous
        // read, then load data at current address buffer
        // (before incrementing the address) into the read buffer
        returnVal = readBuffer;
        readBuffer = console->ppuRead(addrBuffer);
    }
    else {
        // when the address buffer points to the palette address range,
        // instead of reading from the read buffer, it returns the 
        // data at the immediate address, and stores the name table
        // data that would otherwise be mirrored "underneath" the 
        // palette address space in the read buffer
        returnVal = console->ppuRead(addrBuffer);
        readBuffer = console->ppuRead(addrBuffer - 0x1000);
    }
    addrBuffer += ((vRAMAddrIncr) ? 32 : 1);
    return returnVal;
}

void PPU::load() {
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 32; ++x) {
            for (int y = 0; y < 30; ++y) {
                bgTiles[nt][x][y].reload();
            }
        }
    }
    for (int i = 0; i < 64; ++i) {
        sprites[i].reload();
    }
    // TODO reload ppu data here such as scrollX and scrollY
    // reload sprites, due to oamaddr changing
}

void PPU::renderPixel(int x, int y) {
    int realX = x;
    if (nameTableAddr & 0b01) {
        realX += 256;
    }
    realX += scrollX;
    realX %= 512;

    int realY = y;
    if (nameTableAddr & 0b10) {
        realY += 240;
    }
    realY += scrollY;
    realY %= 480;

    // render first occluding sprite that isn't transparent
    for (int i = 0; i < 64; ++i) {
        if (sprites[i].occludes(x, y)) {
            uint8_t paletteIndex = sprites[i].getValue(x, y);
            if (paletteIndex % 4 != 0) {
                uint8_t paletteVal = palette.getValue(paletteIndex);
                uint32_t pixelColour = universalPalette[paletteVal];
                frameBuffer[x + (y * FRAME_WIDTH)] = pixelColour;
                return;
            }
        }
    }

    // otherwise render background if not transparent
    uint8_t paletteIndex = bgPixels[realX][realY].getValue();
    if (paletteIndex % 4 != 0) {
        uint8_t paletteVal = palette.getValue(paletteIndex);
        uint32_t pixelColour = universalPalette[paletteVal];
        frameBuffer[x + (y * FRAME_WIDTH)] = pixelColour;
        return;
    }

    // otherwise render universal background colour
    frameBuffer[x + (y * FRAME_WIDTH)] = universalPalette[palette.getValue(0)];
}

void PPU::renderFrame() {
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            renderPixel(x, y);
            //if(!(x%8) || !(y%8))
            //    frameBuffer[x + (y * FRAME_WIDTH)] = 0xFF0000;
        }
    }
}

void PPU::renderScanline(int scanlNum) {
    for (int x = 0; x < FRAME_WIDTH; x++) {
	renderPixel(x, scanlNum);
    }
}

uint32_t *PPU::getFrame() {
    return frameBuffer;
}

const uint32_t PPU::universalPalette[64] = {
    0x757575, 0x271B8F, 0x0000AB, 0x47009F, 0x8F0077, 0xAB0013,
    0xA70000, 0x7F0B00, 0x432F00, 0x004700, 0x005100, 0x003F17,
    0x1B3F5F, 0x000000, 0x000000, 0x000000, 0xBCBCBC, 0x0073EF,
    0x233BEF, 0x8300F3, 0xBF00BF, 0xE7005B, 0xDB2B00, 0xCB4F0F,
    0x8B7300, 0x009700, 0x00AB00, 0x00933B, 0x00838B, 0x000000,
    0x000000, 0x000000, 0xFFFFFF, 0x3FBFFF, 0x5F97FF, 0xA78BFD,
    0xF77BFF, 0xFF77B7, 0xFF7763, 0xFF9B3B, 0xF3BF3F, 0x83D313,
    0x4FDF4B, 0x58F898, 0x00EBDB, 0x444444, 0x000000, 0x000000,
    0xFFFFFF, 0xABE7FF, 0xC7D7FF, 0xD7CBFF, 0xFFC7FF, 0xFFC7DB,
    0xFFBFB3, 0xFFDBAB, 0xFFE7A3, 0xE3FFA3, 0xABF3BF, 0xB3FFCF,
    0x9FFFF3, 0xAAAAAA, 0x000000, 0x000000};

void PPU::tick() {
    ++clockCounter;
    if (clockCounter >= POST_REND) {
	if (clockCounter == CYC_PER_FRAME) {
	    isVBlank = false;
	    clockCounter = 0;
	    load();
	    if (oddFrame && (showBg || showSpr)) {
		// skip idle cycle 0
		++clockCounter;
		renderScanline(0);
	    }
	}
	else if (clockCounter == PRE_REND) {
	    isVBlank = false;
	    spr0Hit = false;
	}
	else if (clockCounter == VBLANK) {
	    isVBlank = true;
	    if (nmiOnVBlank) {
		console->cpu->signalNMI();
	    }
	    oddFrame ^= 1;
	}
    }
    else if (!(clockCounter % CYC_PER_SCANL)) {
	renderScanline(clockCounter / CYC_PER_SCANL);
    }
}

bool PPU::endOfFrame() {
    return clockCounter == VBLANK;
}
