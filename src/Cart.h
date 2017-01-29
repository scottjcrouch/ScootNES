#ifndef CART_H
#define CART_H

#include <cstdint>
#include <string>
#include <memory>

static const int PRG_BANK_SIZE = 16 * 1024;
static const int CHR_BANK_SIZE = 8 * 1024;
static const int RAM_BANK_SIZE = 8 * 1024;

class Cart {
public:
    bool loadFile(std::string romFileName);
    uint8_t readPrg(int index);
    void writePrg(int index, uint8_t value);
    uint8_t readChr(int index);
    void writeChr(int index, uint8_t value);
    uint8_t readRam(int index);
    void writeRam(int index, uint8_t value);

    enum Mirroring {
	MIRROR_VERT,
	MIRROR_HOR,
	MIRROR_ALL,
	MIRROR_NONE,
    };

    Mirroring mirroring;
  
private:
    int prgLen;
    int chrLen;
    int ramLen;
    bool isRamBattery;
    bool isTrainer;
    int mapperNum;

    uint8_t trainer[512] = {0};
    std::unique_ptr<uint8_t[]> prg;
    std::unique_ptr<uint8_t[]> chr;
    std::unique_ptr<uint8_t[]> ram;
};

#endif
