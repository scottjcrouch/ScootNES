#include <stdint.h>

#include <Graphics.h>

void Metatile::init(uint8_t *attribute) {
    this->attribute = attribute;
}

uint8_t Metatile::getValue(uint8_t quadrant) {
    return ((attributeByte) >> (quadrant * 2)) & 0b11;
}

void Metatile::reload() {
    attributeByte = *attribute;
}

void Tile::init(Metatile *metaTile, uint8_t quadrant, uint8_t *patternIndex, uint8_t *patternTable, bool *ptOffset) {
    this->metaTile = metaTile;
    this->quadrant = quadrant;
    this->patternIndex = patternIndex;
    this->patternTable = patternTable;
    this->ptOffset = ptOffset;
}

static uint8_t getBit(uint8_t byte, int index) {
    return (byte >> index) & 0x1;
}

uint8_t Tile::getValue(uint8_t x, uint8_t y) {
    uint8_t bit0 = getBit(pattern[y], 7 - x);
    uint8_t bit1 = getBit(pattern[y + 8], 7 - x);
    uint8_t index = bit0 | (bit1 << 1);
    uint8_t selector = metaTile->getValue(quadrant) << 2;
    return index | selector;
}

void Tile::reload() {
    int addr = *patternIndex * 16;
    if (*ptOffset) {
        addr += 0x1000;
    }
    pattern = patternTable + addr;
}

void Pixel::init(Tile *tile, uint8_t x, uint8_t y) {
    this->tile = tile;
    this->x = x;
    this->y = y;
}

uint8_t Pixel::getValue() {
    // get pixel's 4-bit index into its
    // 16 byte palette array
    return tile->getValue(x, y);
}

void Sprite::init(uint8_t *oamData, uint8_t *patternTable, bool *sprSize, bool *sprPatternTableAddr) {
    this->oamData = oamData;
    this->patternTable = patternTable;
    this->sprSize = sprSize;
    this->sprPatternTableAddr = sprPatternTableAddr;
}

void Sprite::reload() {
    xPos = oamData[3];
    yPos = oamData[0];
    visible = (xPos < 0xF9) && (yPos < 0xEF);
    yPos++;
    xBound = (int)xPos + 8;
    yBound = (int)yPos + (*sprSize ? 16 : 8);
        
    int ptOffset = 0;
    if (*sprSize) {
        // 8x16 sprite
        ptOffset = (oamData[1] & 0b11111110) * 16;
        if (oamData[1] & 0b00000001) {
            ptOffset += 0x1000;
        }
    }
    else {
        // 8x8 sprite
        ptOffset = oamData[1] * 16;
        if (*sprPatternTableAddr) {
            // stuff
            ptOffset += 0x1000;
        }
    }
    pattern = patternTable + ptOffset;
    
    paletteSelect = ((oamData[2] & 0b00000011) << 2) | 0x10;
    priority = !!(oamData[2] & 0b00100000);
    flipHor = !!(oamData[2] & 0b01000000);
    flipVert = !!(oamData[2] & 0b10000000);
}

uint8_t Sprite::getValue(uint8_t x, uint8_t y) {
    x -= xPos;
    y -= yPos;
    if (y >= 8) {
        y += 16;
    }
    uint8_t bitX = (flipHor ? x : 7 - x);
    uint8_t bitY = (flipVert ? 7 - y : y);
    uint8_t bit0 = getBit(pattern[bitY], bitX);
    uint8_t bit1 = getBit(pattern[bitY + 8], bitX);
    uint8_t index = bit0 | (bit1 << 1);
    return index | paletteSelect;
}

bool Sprite::occludes(uint8_t x, uint8_t y) {
    if (!visible ||
        x < xPos ||
        x >= xBound ||
        y < yPos ||
        y >= yBound) {
        return false;
    }
    return true;
}
