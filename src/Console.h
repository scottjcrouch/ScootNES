#ifndef CONSOLE_H
#define CONSOLE_H

#include <Cart.h>
#include <Controller.h>
#include <CPU.h>
#include <PPU.h>
#include <APU.h>

class Console {
public:
  Console(Cart *cart, Controller *controller1);
  ~Console();
  void boot();
  void runForOneFrame();
  uint32_t *getFrameBuffer();

  void tick();
  
  uint8_t cpuRead(uint16_t addr);
  void cpuWrite(uint16_t addr, uint8_t data);

  Cart *cart;
  Controller *controller1;
  CPU *cpu;
  PPU *ppu;
  APU *apu;

  int cpuDivider;

//private:
  uint8_t cpuRAM[0x800];
  uint8_t sprRAM[0x100];
  uint8_t nameTables[0x1000];
  uint8_t paletteRAM[0x20];

  uint8_t openBus;
};

#endif
