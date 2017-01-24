#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

class Metatile {
public:
    uint8_t getValue(uint8_t quadrant);

    uint16_t nameTableIndex;
    uint8_t attributeByte;
};

class Tile {
public:
    uint8_t getValue(uint8_t x, uint8_t y);

    Metatile *metaTile;
    uint8_t quadrant;
    uint16_t nameTableIndex;
    uint8_t pattern[16];
};

class Pixel {
public:
    void init(Tile *tile, uint8_t x, uint8_t y);
    uint8_t getValue();

    // tile it belongs to
    Tile *tile;
    // position within that tile
    uint8_t x;
    uint8_t y;
};

class Sprite {
public:
    void init(uint8_t *oamData, uint8_t *patternTable, bool *sprSize, bool *sprPatternTableAddr);
    void reload();
    uint8_t getValue(uint8_t x, uint8_t y);
    bool occludes(uint8_t x, uint8_t y);

    uint8_t *oamData;
    uint8_t *patternTable;
    bool *sprSize;
    bool *sprPatternTableAddr;
    uint8_t xPos;
    uint8_t yPos;
    int xBound;
    int yBound;
    uint8_t *pattern;
    uint8_t paletteSelect;
    bool priority;
    bool flipHor;
    bool flipVert;
    bool visible;
};

#endif
