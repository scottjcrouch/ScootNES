#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include <Cart.h>
#include <PPU.h>
#include <APU.h>
#include <Controller.h>

class CPU {
public:
  CPU(PPU* ppu, Cart* cart, APU* apu, Controller* controller1);
  void executeNextOp();
  void addCycles(int);
  void tick();
  void signalNMI();
  void signalIRQ();

//private:
  Cart* cart;
  PPU* ppu;
  APU* apu;
  Controller* controller1;

  // interrupt vectors
  static const uint16_t NMI_VECTOR = 0xFFFA;
  static const uint16_t RESET_VECTOR = 0xFFFC;
  static const uint16_t IRQ_VECTOR = 0xFFFE;
  // registers
  uint16_t pc;
  uint8_t sp;
  uint8_t acc;
  uint8_t x;
  uint8_t y;
  // status flags
  int carry;
  int zero;
  int intdisable;
  int decmode;
  int brk;
  int overflow;
  int negative;
  // address mode parameters
  uint16_t targetAddr;
  uint16_t operand16;
  uint8_t operand8;
  int useAcc;
  // interrupt signals
  int nmiSignal;
  int irqSignal;
  // misc
  int cyclesLeft;

  // memory
  uint8_t cpuRAM[0x800] = {0};
  uint8_t openBus;

  void runInstr(uint8_t opCode);
  void push8(uint8_t value);
  void push16(uint16_t value);
  uint8_t pop8();
  uint16_t pop16();
  uint16_t loadAddr(uint16_t addr);
  uint8_t getStatus();
  void setStatus(uint8_t value);
  void handleInterrupts();
  void branch();
  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);

  // instructions
  void OpADC();
  void OpAND();
  void OpASL();
  void OpBRK();
  void OpBCC();
  void OpBCS();
  void OpBEQ();
  void OpBIT();
  void OpBMI();
  void OpBNE();
  void OpBPL();
  void OpBVC();
  void OpBVS();
  void OpCLC();
  void OpCLD();
  void OpCLI();
  void OpCLV();
  void OpCMP();
  void OpCPX();
  void OpCPY();
  void OpDEC();
  void OpDEX();
  void OpDEY();
  void OpEOR();
  void OpINC();
  void OpINX();
  void OpINY();
  void OpJMP();
  void OpJSR();
  void OpLDA();
  void OpLDX();
  void OpLDY();
  void OpLSR();
  void OpNOP();
  void OpORA();
  void OpPHA();
  void OpPHP();
  void OpPLA();
  void OpPLP();
  void OpROL();
  void OpROR();
  void OpRTI();
  void OpRTS();
  void OpSBC();
  void OpSEC();
  void OpSED();
  void OpSEI();
  void OpSTA();
  void OpSTX();
  void OpSTY();
  void OpTAX();
  void OpTAY();
  void OpTSX();
  void OpTXA();
  void OpTXS();
  void OpTYA();
  // undocumented ops
  void OpJAM();
  void OpSLO();
  void OpDOP();
  void OpANC();
  void OpTOP();
  void OpRLA();
  void OpSRE();
  void OpALR();
  void OpRRA();
  void OpARR();
  void OpSAX();
  void OpXAA();
  void OpAHX();
  void OpXAS();
  void OpSHY();
  void OpSHX();
  void OpLAX();
  void OpLAR();
  void OpDCP();
  void OpAXS();
  void OpISC();
  // address modes
  void AmABS();
  void AmABX();
  void AmABX_C();
  void AmABY();
  void AmABY_C();
  void AmACC();
  void AmIMM();
  void AmIMP();
  void AmIND();
  void AmINX();
  void AmINY();
  void AmINY_C();
  void AmZPG();
  void AmZPX();
  void AmZPY();
};

#endif
