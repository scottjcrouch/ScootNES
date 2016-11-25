#ifndef PPU_H
#define PPU_H

#include <stdint.h>

class Console; // forward declaration

class PPU {
public:
  PPU(Console *console);
  void boot();
  void enterVBlank();
  void exitVBlank();
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

//private:
  Console *console;

  static const int WINDOW_WIDTH = 256;
  static const int WINDOW_HEIGHT = 240;

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

  // 0x2004: OAMDATA
  uint8_t oamReadBuffer;

  // 0x2005: SCROLL
  uint8_t scrollX;
  uint8_t scrollY;

  // 0x2006: ADDR
  uint16_t addrBuffer;

  // 0x2007: DATA
  uint8_t readBuffer;

  // latch used for SCROLL, ADDR. Unset upon STATUS read
  bool latch;
};

#endif