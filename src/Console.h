#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>

#include <Cart.h>
#include <Controller.h>
#include <CPU.h>
#include <PPU.h>
#include <APU.h>
#include <Divider.h>

class Console {
public:
    Console() : cpuDivider(3) { }
    void boot();
    bool loadINesFile(std::string fileName);
    uint32_t *getFrameBuffer();
    std::vector<short> getAvailableSamples();
    void runForOneFrame();

    Controller controller1;
    APU apu;

private:
    void tick();
    uint8_t cpuRead(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);

    Cart cart;
    PPU ppu;
    CPU cpu;
    Divider cpuDivider;

    uint8_t cpuRam[0x800] = {0};
    uint8_t cpuBusMDR;
};

#endif
