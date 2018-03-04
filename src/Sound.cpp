#include <Sound.h>
#include <SoundQueue.h>
#include <SDL.h>
#include <stdexcept>

Sound::Sound()
{
    initSDLAudio();
    initSoundQueue();
}

Sound::~Sound()
{
    quitSDLAudio();
}

void Sound::initSDLAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
	throw std::runtime_error(std::string("SDL_Init() failed: ") + SDL_GetError());
    }
}

void Sound::initSoundQueue() {
    const char *errorMessage = soundQueue.init(44100);
    if (errorMessage) {
	throw std::runtime_error(std::string("SoundQueue init failed: ") + errorMessage);
    }
}

void Sound::quitSDLAudio() {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void Sound::playSound(short *soundBuf, long soundBufLength) {
    soundQueue.write(soundBuf, soundBufLength);
}
