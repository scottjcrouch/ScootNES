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
};

#endif
