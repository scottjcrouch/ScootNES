#ifndef SOUND_H
#define SOUND_H

#include <SoundQueue.h>

class Sound {
public:
	Sound();
	~Sound();

	void playSound(short *soundBuf, long SoundBufLength);
	
private:
	SoundQueue soundQueue;
	
	void initSDLAudio();
	void initSoundQueue();
	void quitSDLAudio();
};

#endif
