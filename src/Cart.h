#ifndef CART_H
#define CART_H

#include <cstdint>
#include <fstream>
#include <string>
#include <memory>

#include <Mapper.h>

static const int PRG_BANK_SIZE = 16 * 1024;
static const int CHR_BANK_SIZE = 8 * 1024;
static const int RAM_BANK_SIZE = 8 * 1024;

class Cart {
public:
    bool loadFile(std::string romFileName);
    uint8_t readPrg(uint16_t addr);
    void writePrg(uint16_t addr, uint8_t value);
    uint8_t readChr(uint16_t addr);
    void writeChr(uint16_t addr, uint8_t value);

    enum Mirroring {
	MIRROR_VERT,
	MIRROR_HOR,
	MIRROR_ALL,
	MIRROR_NONE,
    };

    Mirroring mirroring;
  
private:
    bool verifyINesHeaderSignature(char* header);
    void loadINesHeaderData(char* header);
    void allocateCartMemory();
    void loadCartMemoryFromFile(std::ifstream& romFileStream);
    bool initializeMapper();
    
    int prgSize;
    int chrSize;
    bool chrIsSingleRamBank;
    int ramSize;
    bool isRamBattery;
    bool isTrainer;
    int mapperNum;
    uint8_t trainer[512] = {0};
    std::unique_ptr<uint8_t[]> prg;
    std::unique_ptr<uint8_t[]> chr;
    std::unique_ptr<uint8_t[]> ram;

    std::unique_ptr<Mapper> mapper;
};

#endif
