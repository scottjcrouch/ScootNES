#ifndef CART_MEMORY_H
#define CART_MEMORY_H

#include <cstdint>
#include <vector>

#include <Mirroring.h>

struct CartMemory {
    Mirroring mirroring;
    bool chrIsRam;
    std::vector<uint8_t> trainer;
    std::vector<uint8_t> prg;
    std::vector<uint8_t> chr;
    std::vector<uint8_t> ram;
};

#endif
