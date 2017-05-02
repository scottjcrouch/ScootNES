#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include <vector>

#include <Cart.h>
#include <Graphics.h>

class CPU;

static const unsigned int FRAME_WIDTH = 256;
static const unsigned int FRAME_HEIGHT = 240;

static const int SCANL_PER_FRAME  = 262; // 240 rendered + 22 non-rendered
static const int CYC_PER_SCANL    = 341;
static const int HBLANK           = 256;
static const int CYC_PER_FRAME    = CYC_PER_SCANL * SCANL_PER_FRAME;
static const int POST_REND        = CYC_PER_SCANL * 240;
static const int VBLANK           = CYC_PER_SCANL * 241;
static const int PRE_REND         = CYC_PER_SCANL * 261;

static const uint32_t universalPalette[64] = {
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
    void reloadGraphicsData();
    void renderFrame();
    void renderPixel(int x, int y);
    void renderScanline(int scanlNum);
    uint8_t readPalette(uint16_t index);
    void writePalette(uint16_t index, uint8_t value);
    uint8_t readNameTables(uint16_t addr);
    void writeNameTables(uint16_t addr, uint8_t value);
    uint8_t readPatternTables(uint16_t addr);
    void writePatternTables(uint16_t addr, uint8_t value);
    uint16_t getCiRamIndexFromNameTableIndex(uint16_t index);
    void buildMetatileData();
    void reloadMetatileData();
    void buildTileData();
    void reloadTileData();
    void buildPixelData();
    void buildSpriteData();
    void reloadSpriteData();

    Cart *cart;
    CPU *cpu;

    // memory
    uint8_t oam[0x100] = {0};
    uint8_t ciRam[0x800] = {0};
    uint8_t paletteRam[0x20] = {
	0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D,
	0x08, 0x10, 0x08, 0x24, 0x00, 0x00, 0x04, 0x2C,
	0x09, 0x01, 0x34, 0x03, 0x00, 0x04, 0x00, 0x14,
	0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08};

    // REGISTERS

    // latch used for SCROLL, ADDR. Unset upon STATUS read
    bool latch;
    // 0x2000: CTRL
    int nameTableAddr;          // 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    bool vRamAddrIncr;          // 0=1 (across name table), 1=32 (down name table)
    bool sprPatternTableSelector;// 0=0x0000, 1=0x1000, ignored if sprSize is 8x16
    bool bgPatternTableSelector;// 0=0x0000, 1=0x1000
    bool bigSprites;            // 0=8x8, 1=8x16
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

    int clockCounter;
    int frameCounter;
    bool oddFrame;
    bool spr0Latch;
    bool spr0Reload;

    uint32_t frameBuffer[FRAME_WIDTH * FRAME_HEIGHT];
    Pixel bgPixels[512][480];
    Tile bgTiles[4][32][30];
    Metatile bgMetaTiles[4][8][8];
    Sprite sprites[64];
    std::vector<Sprite> spriteBuffer;
};

#endif
