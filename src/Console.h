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

class Console
{
public:
    Console();
    void reset();
    void loadINesFile(std::string fileName);
    uint32_t *getFrameBuffer();
    std::vector<short> getAvailableSamples();
    void runForOneFrame();

    Controller controller1;
    APU apu;

private:
    void tick();
    uint8_t cpuRead(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);

    std::array<uint8_t, 0x800> cpuRam{0};
    uint8_t cpuBusMDR;

    Divider cpuDivider;
    Cart cart;
    CPU cpu;
    PPU ppu;
};

#endif
