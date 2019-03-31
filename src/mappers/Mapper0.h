#ifndef MAPPER_0_H
#define MAPPER_0_H

#include <CartMemory.h>
#include <Mapper.h>

class Mapper0: public Mapper
{
public:
    Mapper0(CartMemory mem) : Mapper(mem) { };
    uint8_t readPrg(uint16_t addr);
    uint8_t readChr(uint16_t addr);
    void writeChr(uint16_t addr, uint8_t value);
};

#endif
