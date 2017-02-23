#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <cstdint>

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
    uint8_t getValue();

    Tile *tile;
    uint8_t x;
    uint8_t y;
};

class Sprite {
public:
    uint8_t getValue(uint8_t x, uint8_t y, bool isBig);
    bool occludes(uint8_t x, uint8_t y);

    uint8_t oamIndex;
    uint8_t xPos;
    uint8_t yPos;
    int xBound;
    int yBound;
    uint8_t pattern[32];
    uint8_t paletteSelect;
    bool priority;
    bool flipHor;
    bool flipVert;
    bool visible;
};

#endif
