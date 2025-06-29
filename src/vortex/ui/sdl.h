#pragma once
#include <SDL3/SDL.h>

namespace wis {
struct SwapchainDesc;
}
namespace vortex {
class Graphics;
} // namespace vortex

namespace vortex::ui {
class SDLLibrary
{
public:
    SDLLibrary()
    {
        SDL_Init(SDL_INIT_VIDEO);
    }
    ~SDLLibrary()
    {
        SDL_Quit();
    }
};

class SDLWindow
{
public:
    SDLWindow(const char* title, int width, int height, bool fullscreen)
    {
        window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
        id = SDL_GetWindowID(window);
    }
    ~SDLWindow()
    {
        SDL_DestroyWindow(window);
    }
    SDL_Window* GetWindow() const
    {
        return window;
    }
    SDL_WindowID GetID()
    {
        return id;
    }

public:
    wis::SwapChain CreateSwapchain(const vortex::Graphics& gfx, const wis::SwapchainDesc& desc) const noexcept;

    void PostQuit()
    {
        SDL_Event event;
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    }
    std::pair<int, int> PixelSize() const noexcept
    {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        return { w, h };
    }

private:
    SDL_Window* window;
    SDL_WindowID id;
};
} // namespace vortex