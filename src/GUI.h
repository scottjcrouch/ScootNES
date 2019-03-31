#ifndef GUI_H
#define GUI_H

#include <SDL.h>
#include <memory>

const char * const GUI_WINDOW_TITLE = "ScootNES";
const unsigned int GUI_TEXTURE_WIDTH = 256;
const unsigned int GUI_TEXTURE_HEIGHT = 240;
const unsigned int GUI_WINDOW_WIDTH = 256*2;
const unsigned int GUI_WINDOW_HEIGHT = 240*2;

class GUI
{
public:
    GUI();
    ~GUI();

    void clearRenderTarget();
    void renderAndDisplayFrame(uint32_t *frameBuffer);

private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *frameTexture;

    void initSDLVideo();
    void quitSDLVideo();
    void setTextureScaling();
    void createWindow();
    void createRendererForWindow();
    void setRendererDrawColor();
    void createTextureForRenderer();
    void destroyWindow();
    void destroyRendererForWindow();
    void destroyTextureForRenderer();
};

#endif
