#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <vector>

#include <Mirroring.h>

class Mapper {
public:
    virtual void doStuff() { };

    Mirroring mirroring;
    bool chrIsSingleRamBank;
    std::vector<uint8_t> trainer;
    std::vector<uint8_t> prg;
    std::vector<uint8_t> chr;
    std::vector<uint8_t> ram;
};

#endif
