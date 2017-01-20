#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#include <Cart.h>
#include <Graphics.h>

class CPU;

class PPU {
public:
    void boot(Cart *cart, CPU *cpu);
    void setCTRL(uint8_t value);
    void setMASK(uint8_t value);
    uint8_t getSTATUS();
    void setOAMADDR(uint8_t value);
    void setOAMDATA(uint8_t value);
    uint8_t getOAMDATA();
    void setSCROLL(uint8_t value);
    void setADDR(uint8_t value);
    void setDATA(uint8_t value);
    uint8_t getDATA();
    void tick();
    uint32_t *getFrameBuffer();
    bool endOfFrame();

private:
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);
    void load();
    void renderFrame();
    void renderPixel(int x, int y);
    void renderScanline(int scanlNum);
    uint8_t readPalette(uint16_t addr);
    void writePalette(uint16_t addr, uint8_t value);
    uint8_t *getPalettePointer();
    uint8_t *getNameTablePointer();
    uint8_t *getPatternTablePointer();
    uint8_t *getSprRamPointer();

    Cart *cart;
    CPU *cpu;

    // memory
    uint8_t sprRAM[0x100] = {0};
    uint8_t nameTables[0x1000] = {0};
    uint8_t paletteRAM[0x20] = {
	0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D,
	0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
	0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14,
	0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08};

    // registers

    // 0x2000: CTRL
    int nameTableAddr;          // 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    bool vRAMAddrIncr;          // 0=1 (across name table), 1=32 (down name table)
    bool sprPatternTableAddr;   // 0=0x0000, 1=0x1000, ignored if sprSize is 8x16
    bool bgPatternTableAddr;    // 0=0x0000, 1=0x1000
    bool sprSize;               // 0=8x8, 1=8x16
    // unused bit
    bool nmiOnVBlank;

    // 0x2001: MASK
    bool grayscale;             // mirrors palette to only shades of grey
    bool imageMask;             // show bg in left 8 columns of screen
    bool sprMask;               // show sprites in left 8 columns of screen
    bool showBg;
    bool showSpr;
    bool buffRed;               // dims non red colours
    bool buffGreen;             // dims non green colours
    bool buffBlue;              // dims non blue colours

    // 0x2002: STATUS
    // 5 unused bits
    bool sprOverflow;           // if > 8 sprites on any scanline in current frame
    bool spr0Hit;               // at first non-zero pixel overlap in both spr0 and bg
    bool isVBlank;              // set during VBlank

    // 0x2003: OAMADDR
    uint8_t oamAddrBuffer;

    // 0x2005: SCROLL
    uint8_t scrollX;
    uint8_t scrollY;

    // 0x2006: ADDR
    uint16_t addrBuffer;

    // 0x2007: DATA
    uint8_t readBuffer;

    // latch used for SCROLL, ADDR. Unset upon STATUS read
    bool latch;

    // rendering
    static const unsigned int FRAME_WIDTH = 256;
    static const unsigned int FRAME_HEIGHT = 240;
    uint32_t frameBuffer[FRAME_WIDTH * FRAME_HEIGHT];

    static const int SCANL_PER_FRAME  = 262; // 240 rendered + 22 non-rendered
    static const int CYC_PER_SCANL    = 341;
    static const int HBLANK           = 256;
    static const int CYC_PER_FRAME    = CYC_PER_SCANL * SCANL_PER_FRAME;
    static const int POST_REND        = CYC_PER_SCANL * 240;
    static const int VBLANK           = CYC_PER_SCANL * 241;
    static const int PRE_REND         = CYC_PER_SCANL * 261;

    int clockCounter;
    int frameCounter;
    bool oddFrame;
    bool spr0Latch;
    bool spr0Reload;

    // background objects
    Pixel bgPixels[512][480];
    Tile bgTiles[4][32][30];
    Metatile bgMetaTiles[4][8][8];

    // sprites
    Sprite sprites[64];

    // palette
    Palette palette;
    static const uint32_t universalPalette[64];
};

#endif
