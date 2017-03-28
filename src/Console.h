#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>

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
    void runForOneFrame();

    Controller controller1;
    APU apu;

private:
    void tick();

    Cart cart;
    PPU ppu;
    CPU cpu;
    Divider cpuDivider;
};

#endif
