#ifndef MAPPER_0_H
#define MAPPER_0_H

#include <Mapper.h>

class Mapper0: public Mapper {
public:
    using Mapper::Mapper;
    uint8_t readPrg(uint16_t addr);
    void writePrg(uint16_t addr, uint8_t value);
    uint8_t readChr(uint16_t addr);
    void writeChr(uint16_t addr, uint8_t value);
};

#endif
