#ifndef CONSOLE_H
#define CONSOLE_H

#include <Cart.h>
#include <Controller.h>
#include <CPU.h>
#include <PPU.h>
#include <APU.h>

class Console {
public:
  Console(Cart *cart, Controller *contr1);
  ~Console();
  void boot();
  void runFrame();
  uint32_t *getFrame();

  void tick();
  
  uint8_t cpuRead(uint16_t addr);
  void cpuWrite(uint16_t addr, uint8_t data);
  uint8_t ppuRead(uint16_t addr);
  void ppuWrite(uint16_t addr, uint8_t data);
  uint8_t oamRead(uint8_t addr);
  void oamWrite(uint8_t addr, uint8_t data);
  uint8_t *getPalettePointer();
  uint8_t *getNameTablePointer();
  uint8_t *getPatternTablePointer();
  uint8_t *getSprRamPointer();

  Cart *cart;
  Controller *contr1;
  CPU *cpu;
  PPU *ppu;
  APU *apu;

  int masterClock;
  int cpuDivider;
  int instrCount;
  int frameCount;
  bool nmiSignal;
  bool irqSignal;
  bool vbLatch;

//private:
  static const int SCANL_PER_FRAME  = 262; // 240 rendered + 22 non-rendered
  static const int CYC_PER_SCANL    = 341;
  static const int HBLANK           = 256;
  static const int CYC_PER_FRAME    = CYC_PER_SCANL * SCANL_PER_FRAME;
  static const int POST_REND        = CYC_PER_SCANL * 240;
  static const int VBLANK           = CYC_PER_SCANL * 241;
  static const int PRE_REND         = CYC_PER_SCANL * 261;

  uint8_t cpuRAM[0x800];
  uint8_t sprRAM[0x100];
  uint8_t nameTables[0x1000];
  uint8_t paletteRAM[0x20];

  uint8_t openBus;

  void oamDMA(uint8_t offset);
};

#endif
