#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <string>

#include <SDL.h>

#include <Console.h>
#include <Cart.h>
#include <Controller.h>

const unsigned int WINDOW_WIDTH = 256*2;
const unsigned int WINDOW_HEIGHT = 240*2;
const unsigned int GAME_FPS = 60;
const unsigned int TICKS_PER_FRAME = 1000 / GAME_FPS;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *frameTexture = NULL;

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
        "BasicNES",
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

void freeVideo() {
    SDL_DestroyTexture(frameTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = NULL;
    renderer = NULL;
    frameTexture = NULL;
    SDL_Quit();
}

bool getButton(Controller::Button &b, SDL_Keycode sym) {
    switch (sym) {
    case SDLK_UP:
        b = Controller::BUTTON_UP;
        return true;
    case SDLK_DOWN:
        b = Controller::BUTTON_DOWN;
        return true;
    case SDLK_LEFT:
        b = Controller::BUTTON_LEFT;
        return true;
    case SDLK_RIGHT:
        b = Controller::BUTTON_RIGHT;
        return true;
    case SDLK_z:
        b = Controller::BUTTON_A;
        return true;
    case SDLK_x:
        b = Controller::BUTTON_B;
        return true;
    case SDLK_SPACE:
        b = Controller::BUTTON_SELECT;
        return true;
    case SDLK_RETURN:
        b = Controller::BUTTON_START;
        return true;
    default:
        return false;
    }
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

    std::string romFileName(args[1]);
    
    //romFileName = "roms/cpu/nestest/nestest.nes";
    //romFileName = "roms/cpu/instr_test-v3/official_only.nes";
    //romFileName = "roms/cpu/cpu_interrupts_v2/rom_singles/1-cli_latency.nes";
    //romFileName = "roms/cpu/cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes";
    //romFileName = "roms/cpu/cpu_interrupts_v2/rom_singles/3-nmi_and_irq.nes";
    //romFileName = "roms/cpu/cpu_interrupts_v2/rom_singles/4-irq_and_dma.nes";
    //romFileName = "roms/cpu/cpu_interrupts_v2/rom_singles/5-branch_delays_irq.nes";
    //romFileName = "roms/ppu/palette.nes";
    //romFileName = "roms/ppu/nestress.nes";

    Cart *cart = new Cart();
    if (!cart->readFile(romFileName)) {
        printf("cart failed to read file\n");
        return 1;
    }
    Controller *contr1 = new Controller();
    Console *console = new Console(cart, contr1);
    console->boot();

    // main loop
    bool quit = false;
    bool pause = false;
    SDL_Event e;
    while (!quit) {
        // track time elapsed during frame/loop
        unsigned int frameTimer = SDL_GetTicks();

        // handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                Controller::Button b;
                if (getButton(b, e.key.keysym.sym)) {
                    contr1->press(b);
                }
                else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
                else if (e.key.keysym.sym == SDLK_p) {
                    pause = !pause;
                }
            }
            else if (e.type == SDL_KEYUP) {
                Controller::Button b;
                if (getButton(b, e.key.keysym.sym)) {
                    contr1->release(b);
                }
            }
        }

        // clear screen to black
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        // spin cpu for 1 frame
        if (!pause) {
            console->runFrame();
        }
        
        // get frame array
        uint32_t *frameArray = console->getFrame();

        // apply frame data to texture
        SDL_UpdateTexture(frameTexture, NULL, frameArray, 256 * sizeof(uint32_t));

        // render the texture
        SDL_RenderCopy(renderer, frameTexture, NULL, NULL);

        // finally present render target to the window
        SDL_RenderPresent(renderer);

        // delay frame to enforce framerate
        frameTimer = SDL_GetTicks() - frameTimer;
        if (frameTimer < TICKS_PER_FRAME) {
            SDL_Delay(TICKS_PER_FRAME - frameTimer);
        }
    }

    // cleanup
    freeVideo();
    delete cart, contr1, console;

    return 0;
}
