#include <cstdint>

#include <APU.h>

static int null_dmc_reader(void*, uint16_t)
{
	return 0x55; // causes dmc sample to be flat
}

APU::APU()
{
	time = 0;
	frameLength = 29780;
	apu.dmc_reader(null_dmc_reader, NULL);
	apu.output(&buf);
	buf.clock_rate(1789773);
	buf.sample_rate(96000);
}

void APU::setDmcCallback(int (*f)(void* user_data, uint16_t), void* p)
{
	apu.dmc_reader(f, p);
}

void APU::writeRegister(uint16_t addr, uint8_t data)
{
	apu.write_register(clock(), addr, data);
}

int APU::getStatus()
{
	return apu.read_status(clock());
}

void APU::endFrame()
{
	time = 0;
	frameLength ^= 1;
	apu.end_frame(frameLength);
	buf.end_frame(frameLength);
}

long APU::samplesAvailable() const
{
	return buf.samples_avail();
}

long APU::readSamples(sample_t* buffer, long bufferSize)
{
	return buf.read_samples(buffer, bufferSize);
}
