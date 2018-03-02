#include <cstdio>
#include <fstream>
#include <string>
#include <map>
#include <memory>
#include <exception>
#include <stdexcept>

#include <SDL.h>

#include <Console.h>
#include <Cart.h>
#include <Controller.h>
#include <APU.h>
#include <SoundQueue.h>

const unsigned int WINDOW_WIDTH = 256*2;
const unsigned int WINDOW_HEIGHT = 240*2;
const unsigned int GAME_FPS = 60;
const unsigned int TICKS_PER_FRAME = 1000 / GAME_FPS;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *frameTexture = NULL;

std::unique_ptr<SoundQueue> soundQueue;

Console console;

std::map<SDL_Keycode,Controller::Button> controllerKeyBinds;

bool isQuitting, isPaused;

void initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	throw std::runtime_error(std::string("SDL_Init() failed: ") + SDL_GetError());
    }
}

void setTextureScaling() {
    // nearest pixel sampling (i.e. sharp pixels, no aliasing)
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0")) {
	throw std::runtime_error(std::string("SDL_SetHint() failed: ") + SDL_GetError());
    }
}

void createWindow() {
    window = SDL_CreateWindow(
        "ScootNES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (window == NULL) {
	throw std::runtime_error(std::string("SDL_CreateWindow() failed: ") + SDL_GetError());
    }
}

void createRendererForWindow() {
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED); // | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
	throw std::runtime_error(std::string("SDL_CreateRenderer() failed: ") + SDL_GetError());
    }
}

void setRendererDrawColor() {
    // set colour used for drawing operations e.g. SDL_RenderClear
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

void createTextureForRenderer() {
    // create texture that we'll display and update each frame
    frameTexture = SDL_CreateTexture(
        renderer,
        SDL_GetWindowPixelFormat(window),
        SDL_TEXTUREACCESS_STREAMING,
        256,
        240);
    if (frameTexture == NULL) {
	throw std::runtime_error(std::string("SDL_CreateTexture() failed: ") + SDL_GetError());
    }
}

void initVideo() {
    initSDL();
    createWindow();
    createRendererForWindow();
    setRendererDrawColor();
    createTextureForRenderer();
    setTextureScaling();
}

void initSDLAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
	throw std::runtime_error(std::string("SDL_Init() failed: ") + SDL_GetError());
    }
}

void initSoundQueue() {
    soundQueue = std::unique_ptr<SoundQueue>(new SoundQueue);
    if (!soundQueue || soundQueue->init(44100)) {
	throw std::runtime_error(std::string("Sound queue init failed: ") + SDL_GetError());
    }
}

bool initSound() {
    initSDLAudio();
    initSoundQueue();
}

void freeAndQuitSDL() {
    SDL_DestroyTexture(frameTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = NULL;
    renderer = NULL;
    frameTexture = NULL;
    SDL_Quit();
}

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

void clearScreen() {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);
}

void updateScreen() {
    // apply frame buffer data to texture
    SDL_UpdateTexture(frameTexture, NULL, console.getFrameBuffer(), 256 * sizeof(uint32_t));
    // render the texture
    SDL_RenderCopy(renderer, frameTexture, NULL, NULL);
    // finally present render target to the window
    SDL_RenderPresent(renderer);
}

void playSound() {
    // retrieve buffered sound samples
    APU::sample_t soundBuf[2048];
    long soundBufLength = console.apu.readSamples(soundBuf, 2048);
    // play samples in the buffer
    soundQueue->write(soundBuf, soundBufLength);
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
    isQuitting = false;
    isPaused = false;
    while (!isQuitting) {
        // track time elapsed during frame/loop
        unsigned int frameTimer = SDL_GetTicks();

	handleEvents();

	clearScreen();

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

    try {
	initVideo();
	initSound();
    } catch (std::exception const& e) {
	printf(e.what());
	SDL_Quit();
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

    freeAndQuitSDL();

    return 0;
}
