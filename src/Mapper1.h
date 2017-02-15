#ifndef MAPPER_1_H
#define MAPPER_1_H

#include <Mapper.h>

enum PrgMode {
    PRG_32KB,
    PRG_FIX_FIRST_16KB,
    PRG_FIX_LAST_16KB,
};

enum ChrMode {
    CHR_8KB,
    CHR_4KB,
};

class Mapper1: public Mapper {
public:
    using Mapper::Mapper;
    void init();
    uint8_t readPrg(uint16_t addr);
    void writePrg(uint16_t addr, uint8_t value);
    uint8_t readChr(uint16_t addr);
    void writeChr(uint16_t addr, uint8_t value);

private:
    void loadRegister(uint16_t addr, uint8_t value);
    void updateBankAddresses();
    int decodePrgRomAddress(uint16_t addr);
    int decodeChrRomAddress(uint16_t addr);

    int shiftRegister = 0x10;

    int prgRomBank = 0;
    bool prgRamDisable = false;
    int chrRomBank0 = 0;
    int chrRomBank1 = 0;

    PrgMode prgMode = PrgMode::PRG_FIX_LAST_16KB;
    ChrMode chrMode = ChrMode::CHR_8KB;

    int prg16kBankAddresses[2];
    int chr4kBankAddresses[2];
};

#endif
