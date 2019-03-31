#ifndef CART_H
#define CART_H

#include <cstdint>
#include <fstream>
#include <string>
#include <memory>

#include <CartMemory.h>
#include <Mapper.h>
#include <Mirroring.h>

static const int PRG_BANK_SIZE = 16 * 1024;
static const int CHR_BANK_SIZE = 8 * 1024;
static const int RAM_BANK_SIZE = 8 * 1024;
static const int TRAINER_SIZE = 512;

class Cart
{
public:
    void loadFile(std::string romFileName);
    uint8_t readPrg(uint16_t addr);
    void writePrg(uint16_t addr, uint8_t value);
    uint8_t readChr(uint16_t addr);
    void writeChr(uint16_t addr, uint8_t value);
    Mirroring getMirroring();

private:
    std::vector<char> getINesHeaderFromFile(std::ifstream& romFileStream);
    bool verifyINesHeaderSignature(std::vector<char> iNesHeader);
    CartMemory getCartMemoryFromFile(std::vector<char> iNesHeader, std::ifstream& romFileStream);
    int getMapperNumberFromHeader(std::vector<char> iNesHeader);
    bool initializeMapper(int mapperNum, CartMemory mem);

    std::unique_ptr<Mapper> mapper;
};

#endif
