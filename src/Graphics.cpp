#include <stdint.h>

#include <Graphics.h>

uint8_t Metatile::getValue(uint8_t quadrant) {
    return ((attributeByte) >> (quadrant * 2)) & 0b11;
}

static uint8_t getBit(uint8_t byte, int index) {
    return (byte >> index) & 0x1;
}

uint8_t Tile::getValue(uint8_t x, uint8_t y) {
    uint8_t bit0 = getBit(pattern[y], 7 - x);
    uint8_t bit1 = getBit(pattern[y + 8], 7 - x);
    uint8_t index = bit0 | (bit1 << 1);
    uint8_t selector = metaTile->getValue(quadrant) << 2;
    return selector | index;
}

uint8_t Pixel::getValue() {
    // get pixel's 4-bit index into its
    // 16 byte palette array
    return tile->getValue(x, y);
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
    return paletteSelect | index;
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
