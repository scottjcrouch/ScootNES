#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

class Console; // forward declaration

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

class Renderer {
public:
  Renderer(Console *console);
  uint32_t *getFrame();
  void load();
  void renderFrame();

private:
  Console *console;
  static const unsigned int FRAME_WIDTH = 256;
  static const unsigned int FRAME_HEIGHT = 240;
  uint32_t frameBuffer[FRAME_WIDTH * FRAME_HEIGHT];

  static const uint32_t universalPalette[64];

  // background objects
  Pixel bgPixels[512][480];
  Tile bgTiles[4][32][30];
  Metatile bgMetaTiles[4][8][8];

  // sprites
  Sprite sprites[64];

  // palette
  Palette palette;

  void renderPixel(int x, int y);
};

#endif
