#ifndef CART_H
#define CART_H

#include <cstdint>
#include <fstream>
#include <string>
#include <memory>
#include <vector>

#include <Mapper.h>
#include <Mirroring.h>

static const int PRG_BANK_SIZE = 16 * 1024;
static const int CHR_BANK_SIZE = 8 * 1024;
static const int RAM_BANK_SIZE = 8 * 1024;
static const int TRAINER_SIZE = 512;

class Cart {
public:
    bool loadFile(std::string romFileName);
    uint8_t readPrg(uint16_t addr);
    void writePrg(uint16_t addr, uint8_t value);
    uint8_t readChr(uint16_t addr);
    void writeChr(uint16_t addr, uint8_t value);
    Mirroring getMirroring();

private:
    bool verifyINesHeaderSignature(char* header);
    void loadINesHeaderData(char* header);
    void loadCartMemory(std::ifstream& romFileStream);
    bool initializeMapper(int mapperNum);

    Mirroring mirroring;
    int prgSize;
    int chrSize;
    bool chrIsSingleRamBank;
    int ramSize;
    bool isRamBattery;
    bool isTrainer;
    std::vector<uint8_t> trainer;
    std::vector<uint8_t> prg;
    std::vector<uint8_t> chr;
    std::vector<uint8_t> ram;

    std::unique_ptr<Mapper> mapper;
};

#endif
