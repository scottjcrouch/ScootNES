#include <string>
#include <map>
#include <exception>

#include <SDL.h>

#include <Console.h>
#include <Controller.h>

#include <GUI.h>
#include <Sound.h>
#include <FrameDelayTimer.h>

const unsigned int GAME_FPS = 60;
const unsigned int TICKS_PER_FRAME = 1000 / GAME_FPS;

GUI gui;
Sound sound;

Console console;

bool isQuitting = false, isPaused = false;

std::map<SDL_Keycode,Controller::Button> controllerKeyBinds = {
    {SDLK_UP,     Controller::BUTTON_UP},
    {SDLK_DOWN,   Controller::BUTTON_DOWN},
    {SDLK_LEFT,   Controller::BUTTON_LEFT},
    {SDLK_RIGHT,  Controller::BUTTON_RIGHT},
    {SDLK_z,      Controller::BUTTON_A},
    {SDLK_x,      Controller::BUTTON_B},
    {SDLK_SPACE,  Controller::BUTTON_SELECT},
    {SDLK_RETURN, Controller::BUTTON_START}};

void playSound() {
    sound.playSound(console.getAvailableSamples());
}

void clearNextFrame() {
    gui.clearRenderTarget();
}

void displayFrameBuffer() {
    uint32_t *frameBuffer = console.getFrameBuffer();
    gui.renderAndDisplayFrame(frameBuffer);
}

void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
	if (event.type == SDL_QUIT) {
	    isQuitting = true;
	}
	else if (event.type == SDL_KEYDOWN) {
	    SDL_Keycode sym = event.key.keysym.sym;
	    if (controllerKeyBinds.count(sym) > 0) {
		console.controller1.press(controllerKeyBinds[sym]);
	    }
	    else if (sym == SDLK_ESCAPE) {
		isQuitting = true;
	    }
	    else if (sym == SDLK_p) {
		isPaused ^= 1;
	    }
	}
	else if (event.type == SDL_KEYUP) {
	    SDL_Keycode sym = event.key.keysym.sym;
	    if (controllerKeyBinds.count(sym) > 0) {
		console.controller1.release(controllerKeyBinds[sym]);
	    }
	}
    }
}

void runEmulation() {
    FrameDelayTimer timer(TICKS_PER_FRAME);
    while (!isQuitting) {
	handleEvents();
	clearNextFrame();
        if (!isPaused) {
            console.runForOneFrame();
        }
	displayFrameBuffer();
	playSound();
	timer.delay();
    }
}

int main(int argc, char *args[]) {
    if (argc < 2) {
	printf("Rom path not provided\n");
	return 1;
    }
    std::string romFileName(args[1]);
    try {
        console.loadINesFile(romFileName);
    } catch (const std::exception& e) {
        printf("Loading rom file failed: ");
        printf(e.what());
        return 1;
    }
    
    console.boot();
    
    runEmulation();
    
    return 0;
}
