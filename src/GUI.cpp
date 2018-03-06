#include <GUI.h>
#include <SDL.h>

GUI::GUI() {
    initSDLVideo();
    
    createWindow();
    createRendererForWindow();
    createTextureForRenderer();
    
    setRendererDrawColor();
    setTextureScaling();
}

GUI::~GUI() {
    destroyTextureForRenderer();
    destroyRendererForWindow();
    destroyWindow();
    
    quitSDLVideo();
}

void GUI::clearRenderTarget() {
    SDL_RenderClear(renderer);
}

void GUI::renderAndDisplayFrame(uint32_t *frameBuffer) {
    // apply frame buffer data to texture
    SDL_UpdateTexture(frameTexture, NULL, frameBuffer, GUI_TEXTURE_WIDTH * sizeof(uint32_t));
    // render the texture
    SDL_RenderCopy(renderer, frameTexture, NULL, NULL);
    // finally present render target to the window
    SDL_RenderPresent(renderer);
}

void GUI::initSDLVideo() {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
	throw std::runtime_error(std::string("SDL_INIT_VIDEO subsystem init failed: ") + SDL_GetError());
    }
}

void GUI::quitSDLVideo() {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void GUI::setTextureScaling() {
    // nearest pixel sampling (i.e. sharp pixels, no aliasing)
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0")) {
	throw std::runtime_error(std::string("SDL_SetHint() failed: ") + SDL_GetError());
    }
}

void GUI::createWindow() {
    window = SDL_CreateWindow(
        GUI_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        GUI_WINDOW_WIDTH,
        GUI_WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (window == NULL) {
	throw std::runtime_error(std::string("SDL_CreateWindow() failed: ") + SDL_GetError());
    }
}

void GUI::createRendererForWindow() {
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED); // | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
	throw std::runtime_error(std::string("SDL_CreateRenderer() failed: ") + SDL_GetError());
    }
}

void GUI::setRendererDrawColor() {
    // set colour used for drawing operations
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

void GUI::createTextureForRenderer() {
    // create texture that we'll display and update each frame
    frameTexture = SDL_CreateTexture(
        renderer,
        SDL_GetWindowPixelFormat(window),
        SDL_TEXTUREACCESS_STREAMING,
        GUI_TEXTURE_WIDTH,
        GUI_TEXTURE_HEIGHT);
    if (frameTexture == NULL) {
	throw std::runtime_error(std::string("SDL_CreateTexture() failed: ") + SDL_GetError());
    }
}

void GUI::destroyWindow() {
    SDL_DestroyWindow(window);
}

void GUI::destroyRendererForWindow() {
    SDL_DestroyRenderer(renderer);
}

void GUI::destroyTextureForRenderer() {
    SDL_DestroyTexture(frameTexture);
}
