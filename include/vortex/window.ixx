module;
#include <SDL3/SDL.h>
#include <memory>
#include <stdexcept>

#ifdef __INTELLISENSE__
#include <wisdom/wisdom.hpp>
#include <wisdom/wisdom_platform.hpp>
#endif
export module vortex.window;

import wisdom;
import wisdom.platform;

namespace vortex {
export class SDLInstance{
public:
    SDLInstance()
    {
        SDL_Init(SDL_INIT_VIDEO);
    }
    ~SDLInstance()
    {
        SDL_Quit();
    }
};

export class PlatformExtension
{
public:
    enum class Selector {
        None,
        Windows,
        X11,
        Wayland
    };

public:
    PlatformExtension() {
        current = Selector::None;
        platform = {};
        const char* platform_name = SDL_GetCurrentVideoDriver();
#if defined(SDL_PLATFORM_WIN32)
        platform = std::make_unique<wis::platform::WindowsExtension>();
        current = Selector::Windows;
#elif defined(SDL_PLATFORM_LINUX)
        if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
            platform = std::make_unique<wis::platform::X11Extension>();
            current = Selector::X11;
        } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
            platform = std::make_unique<wis::platform::WaylandExtension>();
            current = Selector::Wayland;
        }
#endif
    }

public:
    wis::FactoryExtension* get() noexcept
    {
        return platform.get();
    }

public:
    Selector current = Selector::None;
    std::unique_ptr<wis::FactoryExtension> platform;
};

export class Window
{
public:
    Window(const char* title, int width, int height, bool fullscreen)
    {
        window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    }
    ~Window()
    {
        SDL_DestroyWindow(window);
    }
    SDL_Window* GetWindow() const
    {
        return window;
    }

    int PollEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                return 1;
            }
        }
    }

public:
    wis::FactoryExtension* GetPlatformExtension()
    {
        return _platform.get();
    }
    wis::SwapChain CreateSwapchain(wis::Result& result, const wis::Device& device, const wis::CommandQueue& main_queue, const wis::SwapchainDesc& desc)
    {
        using enum PlatformExtension::Selector;
        if (_platform.current == None) {
            throw std::runtime_error("Platform is not selected");
        }

        auto [width, height] = PixelSize();

        switch (_platform.current) {
#if defined(SDL_PLATFORM_WIN32)
        case Windows: {
            HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
            if (hwnd) {
                return static_cast<wis::platform::WindowsExtension*>(_platform.get())
                        ->CreateSwapchain(result, device, main_queue, desc, hwnd);
            }
        } break;
#elif defined(SDL_PLATFORM_LINUX)
        case X11: {
            Display* xdisplay = (Display*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
            ::Window xwindow = (::Window)SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
            if (xdisplay && xwindow) {
                return static_cast<wis::platform::X11Extension*>(_platform.get())
                        ->CreateSwapchain(result, device, main_queue, desc, xdisplay, xwindow);
            }
        } break;
        case Wayland: {
            struct wl_display* display = (struct wl_display*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
            struct wl_surface* surface = (struct wl_surface*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
            if (display && surface) {
                return static_cast<wis::platform::WaylandExtension*>(_platform.get())
                        ->CreateSwapchain(result, device, main_queue, desc, display, surface);
            }
        } break;
#endif
        }
        throw std::runtime_error("Failed to create swapchain");
    }

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
    PlatformExtension _platform;
};

} // namespace vortex