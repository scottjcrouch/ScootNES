#include <cstdio>
#include <fstream>
#include <string>
#include <map>
#include <memory>

#include <SDL.h>

#include <Console.h>
#include <Cart.h>
#include <Controller.h>
#include <APU.h>
#include <GUI.h>
#include <Sound.h>

const unsigned int GAME_FPS = 60;
const unsigned int TICKS_PER_FRAME = 1000 / GAME_FPS;

GUI gui;
Sound sound;

Console console;

std::map<SDL_Keycode,Controller::Button> controllerKeyBinds;

bool isQuitting = false, isPaused = false;

void addControllerKeyBinds() {
    controllerKeyBinds[SDLK_UP]      = Controller::BUTTON_UP;
    controllerKeyBinds[SDLK_DOWN]    = Controller::BUTTON_DOWN;
    controllerKeyBinds[SDLK_LEFT]    = Controller::BUTTON_LEFT;
    controllerKeyBinds[SDLK_RIGHT]   = Controller::BUTTON_RIGHT;
    controllerKeyBinds[SDLK_z]       = Controller::BUTTON_A;
    controllerKeyBinds[SDLK_x]       = Controller::BUTTON_B;
    controllerKeyBinds[SDLK_SPACE]   = Controller::BUTTON_SELECT;
    controllerKeyBinds[SDLK_RETURN]  = Controller::BUTTON_START;
}

void playSound() {
    short soundBuf[2048];
    long soundBufLength = console.apu.readSamples(soundBuf, 2048);
    sound.playSound(soundBuf, soundBufLength);
}

void clearNextFrame() {
    gui.clearNextFrame();
}

void updateScreen() {
    uint32_t *frameBuffer = console.getFrameBuffer();
    gui.updateScreen(frameBuffer);
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
    while (!isQuitting) {
        // track time elapsed during frame/loop
        unsigned int frameTimer = SDL_GetTicks();

	handleEvents();

	clearNextFrame();

        if (!isPaused) {
            console.runForOneFrame();
        }

	updateScreen();

	playSound();

        // delay frame to enforce framerate
        frameTimer = SDL_GetTicks() - frameTimer;
        if (frameTimer < TICKS_PER_FRAME) {
            SDL_Delay(TICKS_PER_FRAME - frameTimer);
        }
    }
}

int main(int argc, char *args[]) {
    if (argc < 2) {
	printf("Rom path not provided\n");
	return 1;
    }

    std::string romFileName(args[1]);

    if (!console.loadINesFile(romFileName)) {
        printf("failed to read file\n");
        return 1;
    }

    console.boot();

    addControllerKeyBinds();

    runEmulation();

    return 0;
}
