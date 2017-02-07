#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <vector>

#include <Mirroring.h>

class Mapper {
public:
    virtual uint8_t readPrg(uint16_t addr) { };
    virtual void writePrg(uint16_t addr, uint8_t value) { };
    virtual uint8_t readChr(uint16_t addr) { };
    virtual void writeChr(uint16_t addr, uint8_t value) { };

    Mirroring mirroring;
    bool chrIsRam;
    std::vector<uint8_t> trainer;
    std::vector<uint8_t> prg;
    std::vector<uint8_t> chr;
    std::vector<uint8_t> ram;
};

#endif
