#include <cstdint>

#include <Graphics.h>

uint8_t Metatile::getValue(uint8_t quadrant)
{
    return ((attributeByte) >> (quadrant * 2)) & 0b11;
}

static uint8_t getBit(uint8_t byte, int index)
{
    return (byte >> index) & 0x1;
}

uint8_t Tile::getValue(uint8_t x, uint8_t y)
{
    uint8_t bit0 = getBit(pattern[y], 7 - x);
    uint8_t bit1 = getBit(pattern[y + 8], 7 - x);
    uint8_t index = bit0 | (bit1 << 1);
    uint8_t selector = metaTile->getValue(quadrant) << 2;
    return selector | index;
}

uint8_t Pixel::getValue()
{
    // get pixel's 4-bit index into its
    // 16 byte palette array
    return tile->getValue(x, y);
}

uint8_t Sprite::getValue(uint8_t x, uint8_t y, bool isBig)
{
    x -= xPos;
    y -= yPos;
    uint8_t bitColumn = (flipHor ? x : 7 - x);
    uint8_t bitRow = (flipVert ? (isBig ? 15 : 7) - y : y);
    if (bitRow > 7) {
	bitRow += 8;
    }
    uint8_t bit0 = getBit(pattern[bitRow], bitColumn);
    uint8_t bit1 = getBit(pattern[bitRow + 8], bitColumn);
    uint8_t index = bit0 | (bit1 << 1);
    return paletteSelect | index;
}

bool Sprite::occludes(uint8_t x, uint8_t y)
{
    if (!visible ||
        x < xPos ||
        x >= xBound ||
        y < yPos ||
        y >= yBound) {
        return false;
    }
    return true;
}
