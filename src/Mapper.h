#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <vector>

#include <CartMemory.h>
#include <Mirroring.h>

class Mapper {
public:
    Mapper(CartMemory mem) : cartMemory(mem) { };
    Mirroring getMirroring() { return cartMemory.mirroring; };
    virtual uint8_t readPrg(uint16_t addr) { return 0; };
    virtual void writePrg(uint16_t addr, uint8_t value) { };
    virtual uint8_t readChr(uint16_t addr) { return 0; };
    virtual void writeChr(uint16_t addr, uint8_t value) { };

protected:
    CartMemory cartMemory;
};

#endif
