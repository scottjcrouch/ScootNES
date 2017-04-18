#include <cstdio>
#include <fstream>
#include <string>
#include <map>

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

SoundQueue *soundQueue = NULL;

Console console;

std::map<SDL_Keycode,Controller::Button> controllerKeyBinds;

bool initVideo() {
    // init SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init() failed: %s\n", SDL_GetError());
        return false;
    }

    // set texture scaling to nearest pixel sampling (sharp pixels)
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0")) {
        printf("SDL_SetHint() failed\n");
        return false;
    }

    // create window
    window = SDL_CreateWindow(
        "ScootNES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("SDL_CreateWindow() failed: %s\n", SDL_GetError());
        return false;
    }

    // create renderer for window
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED); // | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("SDL_CreateRederer() failed: %s\n", SDL_GetError());
        return false;
    }

    // set colour used for drawing operations e.g. SDL_RenderClear
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    // create texture that we'll display and update each frame
    frameTexture = SDL_CreateTexture(
        renderer,
        SDL_GetWindowPixelFormat(window),
        SDL_TEXTUREACCESS_STREAMING,
        256,
        240);
    if (frameTexture == NULL) {
        printf("SDL_CreateTexture() failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool initSound() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
	printf("SDL_Init() failed: %s\n", SDL_GetError());
        return false;
    }
    
    soundQueue = new SoundQueue;
    if (!soundQueue || soundQueue->init(44100)) {
	printf("Sound queue init failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void freeAndQuitSDL() {
    SDL_DestroyTexture(frameTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = NULL;
    renderer = NULL;
    frameTexture = NULL;
    delete soundQueue;
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

int main(int argc, char *args[]) {
    if (argc < 2) {
	printf("Rom path not provided\n");
	return 1;
    }
    if (!initVideo()) {
        printf("initVideo() failed\n");
        return 1;
    }
    if (!initSound()) {
        printf("initSound() failed\n");
        return 1;
    }

    std::string romFileName(args[1]);
    
    if (!console.loadINesFile(romFileName)) {
        printf("failed to read file\n");
        return 1;
    }

    console.boot();

    addControllerKeyBinds();

    // main loop
    bool isQuitting = false;
    bool isPaused = false;
    SDL_Event event;
    while (!isQuitting) {
        // track time elapsed during frame/loop
        unsigned int frameTimer = SDL_GetTicks();

        // handle events
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

        // clear screen to black
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        // spin cpu for 1 frame
        if (!isPaused) {
            console.runForOneFrame();
        }
        
        // apply frame buffer data to texture
        SDL_UpdateTexture(frameTexture, NULL, console.getFrameBuffer(), 256 * sizeof(uint32_t));

        // render the texture
        SDL_RenderCopy(renderer, frameTexture, NULL, NULL);

        // finally present render target to the window
        SDL_RenderPresent(renderer);

	// retrieve buffered sound samples
	APU::sample_t soundBuf[2048];
	long soundBufLength = console.apu.readSamples(soundBuf, 2048);
	
	// play samples in the buffer
	soundQueue->write(soundBuf, soundBufLength);

        // delay frame to enforce framerate
        frameTimer = SDL_GetTicks() - frameTimer;
        if (frameTimer < TICKS_PER_FRAME) {
            SDL_Delay(TICKS_PER_FRAME - frameTimer);
        }
    }

    // cleanup
    freeAndQuitSDL();

    return 0;
}
