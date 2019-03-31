#ifndef SOUND_H
#define SOUND_H

#include <SoundQueue.h>
#include <vector>

class Sound
{
public:
	Sound();
	~Sound();

	void playSound(std::vector<short> buffer);

private:
	SoundQueue soundQueue;

	void initSDLAudio();
	void initSoundQueue();
	void quitSDLAudio();
};

#endif
