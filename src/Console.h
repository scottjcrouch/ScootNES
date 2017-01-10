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

  int cpuDivider;
  bool nmiSignal;
  bool irqSignal;

//private:
  uint8_t cpuRAM[0x800];
  uint8_t sprRAM[0x100];
  uint8_t nameTables[0x1000];
  uint8_t paletteRAM[0x20];

  uint8_t openBus;

  void oamDMA(uint8_t offset);
};

#endif
