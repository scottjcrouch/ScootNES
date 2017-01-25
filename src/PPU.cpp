#include <stdint.h>

#include <PPU.h>
#include <CPU.h>
#include <Graphics.h>

void PPU::boot(Cart *cart, CPU *cpu) {
    this->cart = cart;
    this->cpu = cpu;

    // 0x2000: CTRL
    nameTableAddr = 0;              // 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    vRamAddrIncr = false;           // 0=1 (across name table), 1=32 (down name table)
    sprPatternTableSelector = false;// 0=0x0000, 1=0x1000, ignored if bigSprites is 8x16
    bgPatternTableSelector = false; // 0=0x0000, 1=0x1000
    bigSprites = false;             // 0=8x8, 1=8x16
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

    // 0x2005: SCROLL
    scrollX = 0x00;
    scrollY = 0x00;

    // 0x2006: ADDR
    addrBuffer = 0x0000;

    // 0x2007: DATA
    readBuffer = 0x00;

    // latch used for SCROLL, ADDR. Unset upon STATUS read
    latch = false;

    buildMetatileData();
    buildTileData();
    buildPixelData();
    buildSpriteData();

    clockCounter = VBLANK;
    frameCounter = 0;
    oddFrame = false;
    spr0Latch = false;
    spr0Reload = false;
}

void PPU::setCTRL(uint8_t value) {
    nameTableAddr = value & 0x03;
    vRamAddrIncr = !!(value & 0x04);
    sprPatternTableSelector = !!(value & 0x08);
    bgPatternTableSelector = !!(value & 0x10);
    bigSprites = !!(value & 0x20);
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
    // the third byte of every sprite entry is missing bits
    // in hardware, so zero them here before writing
    if (oamAddrBuffer % 4 == 2) {
        value &= 0xE3;
    }
    oam[oamAddrBuffer] = value;
    oamAddrBuffer++;
}

uint8_t PPU::getOAMDATA() {
    // apparently unreliable in NES hardware, but some games use it
    return oam[oamAddrBuffer];
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
	// set scroll and nametable selector values
	scrollX &= 0x07; // clear coarse x scroll bits
	scrollX |= (addrBuffer << 3) & 0xFF; // add new coarse x scroll bits
	scrollY = 0x0; // clear all y scroll bits
	scrollY |= (addrBuffer >> 2) & 0xF8; // add new coarse y scroll bits
	scrollY |= (addrBuffer >> 12) & 0x07; // add new fine y scroll bits
	nameTableAddr = (addrBuffer >> 10) & 0x03; // add new nametable select bits
    }
    latch = !latch;
}

void PPU::setDATA(uint8_t value) {
    write(addrBuffer, value);
    addrBuffer += ((vRamAddrIncr) ? 32 : 1);
}

uint8_t PPU::getDATA() {
    uint8_t returnVal;
    if (addrBuffer < 0x3F00) {
        // return what's in the read buffer from the previous
        // read, then load data at current address buffer
        // (before incrementing the address) into the read buffer
        returnVal = readBuffer;
        readBuffer = read(addrBuffer);
    }
    else {
        // when the address buffer points to the palette address range,
        // instead of reading from the read buffer, it returns the 
        // data at the immediate address, and stores the name table
        // data that would otherwise be mirrored "underneath" the 
        // palette address space in the read buffer
        returnVal = read(addrBuffer);
        readBuffer = read(addrBuffer - 0x1000);
    }
    addrBuffer += ((vRamAddrIncr) ? 32 : 1);
    return returnVal;
}

void PPU::reloadGraphicsData() {
    reloadTileData();
    reloadMetatileData();
    reloadSpriteData();
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

    if (sprites[0].occludes(x, y) &&
	(sprites[0].getValue(x, y) % 4 != 0) &&
	(bgPixels[realX][realY].getValue() % 4 != 0) &&
	((x > 7) || (imageMask && sprMask)) &&
	x != 255 &&
	!spr0Latch &&
	showBg &&
	showSpr) {
	spr0Hit = true;
	spr0Latch = true;
	spr0Reload = true;
    }
    
    // render first occluding sprite that isn't transparent
    bool isBgSpr = false;
    if (showSpr && (x > 7 || sprMask)) {
	for (int i = 0; i < 64; ++i) {
	    if (sprites[i].occludes(x, y)) {
		uint8_t paletteIndex = sprites[i].getValue(x, y);
		if (paletteIndex % 4 != 0) {
		    uint8_t paletteVal = readPalette(paletteIndex);
		    uint32_t pixelColour = universalPalette[paletteVal];
		    frameBuffer[x + (y * FRAME_WIDTH)] = pixelColour;
		    isBgSpr = sprites[i].priority;
		    if (!isBgSpr) {
			return;
		    }
		    else {
			break;
		    }
		}
	    }
	}
    }

    // otherwise render background if not transparent
    if (showBg && (x > 7 || imageMask)) {
	uint8_t paletteIndex = bgPixels[realX][realY].getValue();
	if (paletteIndex % 4 != 0) {
	    uint8_t paletteVal = readPalette(paletteIndex);
	    uint32_t pixelColour = universalPalette[paletteVal];
	    frameBuffer[x + (y * FRAME_WIDTH)] = pixelColour;
	    return;
	}
    }

    // otherwise render universal background colour
    if (!isBgSpr) {
	frameBuffer[x + (y * FRAME_WIDTH)] = universalPalette[readPalette(0)];
    }
}

void PPU::renderFrame() {
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            renderPixel(x, y);
        }
    }
}

void PPU::renderScanline(int scanlNum) {
    for (int x = 0; x < FRAME_WIDTH; x++) {
	renderPixel(x, scanlNum);
    }
}

uint32_t *PPU::getFrameBuffer() {
    return frameBuffer;
}

void PPU::tick() {
    ++clockCounter;
    if (clockCounter >= POST_REND) {
	switch (clockCounter) {
	case CYC_PER_FRAME:
	    isVBlank = false;
	    clockCounter = 0;
	    reloadGraphicsData();
	    if (oddFrame && (showBg || showSpr)) {
		// skip idle cycle 0
		++clockCounter;
		renderScanline(0);
	    }
	    break;
	case PRE_REND:
	    isVBlank = false;
	    spr0Hit = false;
	    spr0Latch = false;
	    spr0Reload = false;
	    break;
	case VBLANK:
	    isVBlank = true;
	    if (nmiOnVBlank) {
		cpu->signalNMI();
	    }
	    ++frameCounter;
	    oddFrame ^= 1;
	    break;
	}
    }
    else if (!(clockCounter % CYC_PER_SCANL)) {
	if (spr0Reload) {
	    reloadGraphicsData();
	    spr0Reload = false;
	}
	oamAddrBuffer = 0;
	renderScanline(clockCounter / CYC_PER_SCANL);
    }
}

bool PPU::endOfFrame() {
    return clockCounter == VBLANK;
}

uint8_t PPU::read(uint16_t addr) {
    addr %= 0x4000;
    if (addr >= 0x3F00) {
        return readPalette(addr % 0x20);
    }
    else if (addr >= 0x2000) {
	return readNameTables(addr % 0x1000);
    }
    else {
        return readPatternTables(addr);
    }
}

void PPU::write(uint16_t addr, uint8_t value) {
    addr %= 0x4000;
    if (addr >= 0x3F00) {
	writePalette(addr % 0x20, value);
    }
    else if (addr >= 0x2000) {
	writeNameTables(addr % 0x1000, value);
    }
    else {
        writePatternTables(addr, value);
    }
}

uint8_t PPU::readPalette(uint16_t index) {
    if (index % 4 == 0) {
	return paletteRam[0];
    }
    return paletteRam[index];
}

void PPU::writePalette(uint16_t index, uint8_t value) {
    paletteRam[index] = value;
    if (index % 4 == 0) {
	paletteRam[index ^ 0x10] = value;
    }
}

uint8_t PPU::readNameTables(uint16_t index) {
    switch (cart->mirroring) {
    case Cart::MIRROR_VERT:
	index %= 0x800;
	break;
    case Cart::MIRROR_HOR:
	if (index & 0x800) {
	    index |= 0x400;
	}
	index %= 0x800;
	break;
    case Cart::MIRROR_ALL:
	printf("Single screen nametable not yet supported\n");
	break;
    case Cart::MIRROR_NONE:
	printf("4-screen nametables not yet supported\n");
	break;
    }
    return ciRam[index];
}

void PPU::writeNameTables(uint16_t index, uint8_t value) {
    switch (cart->mirroring) {
    case Cart::MIRROR_VERT:
	index %= 0x800;
	break;
    case Cart::MIRROR_HOR:
	if (index & 0x800) {
	    index |= 0x400;
	}
	index %= 0x800;
	break;
    case Cart::MIRROR_ALL:
	printf("Single screen nametable not yet supported\n");
	break;
    case Cart::MIRROR_NONE:
	printf("4-screen nametables not yet supported\n");
	break;
    }
    ciRam[index] = value;
}

uint8_t PPU::readPatternTables(uint16_t index) {
    return cart->readChr(index);
}

void PPU::writePatternTables(uint16_t index, uint8_t value) {
    cart->writeChr(index, value);
}

void PPU::buildMetatileData() {
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 8; ++x) {
            for (int y = 0; y < 8; ++y) {
                // index in nametable where attribute data is located
                uint16_t nameTableIndex = 0x3C0 + (0x400 * nt) + (y * 8) + x;
                bgMetaTiles[nt][x][y].nameTableIndex = nameTableIndex;
            }
        }
    }
}

void PPU::reloadMetatileData() {
    for (int nt = 0; nt < 4; ++nt) {
	for (int x = 0; x < 8; ++x) {
            for (int y = 0; y < 8; ++y) {
                bgMetaTiles[nt][x][y].attributeByte =
		    readNameTables(bgMetaTiles[nt][x][y].nameTableIndex);
            }
        }
    }
}

void PPU::buildTileData() {
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 32; ++x) {
            for (int y = 0; y < 30; ++y) {
                // offset in name table where data is located
                uint16_t nameTableIndex = (0x400 * nt) + x + (y * 32);
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
		bgTiles[nt][x][y].metaTile = &bgMetaTiles[nt][mtX][mtY];
		bgTiles[nt][x][y].quadrant = quadrant;
		bgTiles[nt][x][y].nameTableIndex = nameTableIndex;
            }
        }
    }
}

void PPU::reloadTileData() {
    for (int nt = 0; nt < 4; ++nt) {
        for (int x = 0; x < 32; ++x) {
            for (int y = 0; y < 30; ++y) {
		int patternTableIndex =
		    readNameTables(bgTiles[nt][x][y].nameTableIndex) * 16;
		if (bgPatternTableSelector) {
		    patternTableIndex += 0x1000;
		}
		for (int i = 0; i < 16; ++i) {
		    bgTiles[nt][x][y].pattern[i] =
			readPatternTables(patternTableIndex + i);
		}
            }
        }
    }
}

void PPU::buildPixelData() {
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
	    bgPixels[x][y].tile = &bgTiles[nt][tileX][tileY];
	    bgPixels[x][y].x = x % 8;
	    bgPixels[x][y].y = y % 8;
        }
    }
}

void PPU::buildSpriteData() {
    for (int i = 0; i < 64; i++) {
	sprites[i].oamIndex = i * 4;
    }
}

void PPU::reloadSpriteData() {
    for (int i = 0; i < 64; ++i) {
	sprites[i].xPos = oam[sprites[i].oamIndex + 3];
	sprites[i].yPos = oam[sprites[i].oamIndex];
	sprites[i].visible =
	    (sprites[i].xPos < 0xF9) && (sprites[i].yPos < 0xEF);

	sprites[i].yPos++;
	sprites[i].xBound = (int)(sprites[i].xPos) + 8;
	sprites[i].yBound = (int)(sprites[i].yPos) + (bigSprites ? 16 : 8);

	if (!sprites[i].visible) {
	    continue;
	}
        
	int patternTableIndex = 0;
	if (bigSprites) {
	    // 8x16 sprite
	    patternTableIndex = (oam[sprites[i].oamIndex + 1] & 0b11111110) * 16;
	    if (oam[sprites[i].oamIndex + 1] & 0b00000001) {
		patternTableIndex += 0x1000;
	    }
	}
	else {
	    // 8x8 sprite
	    patternTableIndex = oam[sprites[i].oamIndex + 1] * 16;
	    if (sprPatternTableSelector) {
		// stuff
		patternTableIndex += 0x1000;
	    }
	}
	for (int patternOffset = 0; patternOffset < (bigSprites ? 32 : 16); ++patternOffset) {
	    sprites[i].pattern[patternOffset] =
		readPatternTables(patternTableIndex + patternOffset);
	}
	
	sprites[i].paletteSelect =
	    ((oam[sprites[i].oamIndex + 2] & 0b00000011) << 2) | 0x10;
	sprites[i].priority = !!(oam[sprites[i].oamIndex + 2] & 0b00100000);
	sprites[i].flipHor  = !!(oam[sprites[i].oamIndex + 2] & 0b01000000);
	sprites[i].flipVert = !!(oam[sprites[i].oamIndex + 2] & 0b10000000);
    }
}
