#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

class Palette {
public:
  void init(uint8_t *palettePointer);
  uint8_t getValue(int index);

private:
  uint8_t *palettePointer;
};

class Metatile {
public:
  void init(uint8_t *attribute);
  uint8_t getValue(uint8_t quadrant);

private:
  uint8_t *attribute;
};

class Tile {
public:
  void init(Metatile *metaTile, uint8_t quadrant, uint8_t *patternIndex, uint8_t *patternTable, bool *ptOffset);
  uint8_t getValue(uint8_t x, uint8_t y);
  void reload();

private:
  // metatile it belongs to
  Metatile *metaTile;
  // position in metatile
  uint8_t quadrant;
  uint8_t *patternIndex;
  uint8_t *patternTable;
  bool *ptOffset;

  uint8_t *pattern;
};

class Pixel {
public:
  void init(Tile *tile, uint8_t x, uint8_t y);
  uint8_t getValue();

private:
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

private:
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
