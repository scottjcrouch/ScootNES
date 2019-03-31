#ifndef APU_H
#define APU_H

#include <cstdint>
#include <vector>

#include "nes_apu/Nes_Apu.h"
#include "nes_apu/Blip_Buffer.h"

class APU
{
public:
    APU();
    void setDmcCallback(int (*callback)(void* user_data, unsigned int), void* user_data = NULL);
    void writeRegister(uint16_t, uint8_t data);
    int getStatus();
    // End a 1/60 sound frame
    void endFrame();
    // Number of samples in buffer
    long samplesAvailable() const;
    // Read at most 'count' samples and return number of samples actually read
    typedef blip_sample_t sample_t;
    std::vector<short> getAvailableSamples();

private:
    Nes_Apu apu;
    Blip_Buffer buf;
    blip_time_t time;
    blip_time_t frameLength;
    blip_time_t clock() { return time += 4; }
};

#endif
