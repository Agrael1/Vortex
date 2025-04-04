module;
#include <SDL3/SDL.h>
#include <memory>
#include <format>
#include <optional>
//
//#ifdef __INTELLISENSE__
//#include <wisdom/wisdom.hpp>
//#include <wisdom/wisdom_platform.hpp>
//#endif
export module vortex.sdl;

import wisdom;
import wisdom.platform;
import vortex.platform;
import vortex.graphics;
import vortex.swapchain;
import vortex.log;

export import vortex.node;

typedef struct HWND__* HWND;

namespace vortex {
export class SDLLibrary
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

export class SDLWindow
{
public:
    SDLWindow(const char* title, int width, int height, bool fullscreen)
    {
        window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    }
    ~SDLWindow()
    {
        SDL_DestroyWindow(window);
    }
    SDL_Window* GetWindow() const
    {
        return window;
    }

public:
    wis::SwapChain CreateSwapchain(const vortex::Graphics& gfx, const wis::SwapchainDesc& desc) const noexcept
    {
        using enum vortex::PlatformExtension::Selector;

        wis::SwapChain swapchain;
        wis::Result result = wis::success;
        const auto& platform = gfx.GetPlatform();
        const auto& device = gfx.GetDevice();
        const auto& main_queue = gfx.GetMainQueue();
        if (platform.current == None) {
            vortex::error("No platform extension found");
            return swapchain;
        }

        auto [width, height] = PixelSize();

        switch (platform.current) {
#if defined(SDL_PLATFORM_WIN32)
        case Windows: {
            HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
            if (hwnd) {
                swapchain = static_cast<const wis::platform::WindowsExtension*>(platform.GetExtension())
                                    ->CreateSwapchain(result, device, main_queue, desc, hwnd);
            }
        } break;
#elif defined(SDL_PLATFORM_LINUX)
        case X11: {
            Display* xdisplay = (Display*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
            ::Window xwindow = (::Window)SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
            if (xdisplay && xwindow) {
                swapchain = static_cast<wis::platform::X11Extension*>(platform.GetExtension())
                                    ->CreateSwapchain(result, device, main_queue, desc, xdisplay, xwindow);
            }
        } break;
        case Wayland: {
            struct wl_display* display = (struct wl_display*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
            struct wl_surface* surface = (struct wl_surface*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
            if (display && surface) {
                swapchain = static_cast<wis::platform::WaylandExtension*>(platform.GetExtension())
                                    ->CreateSwapchain(result, device, main_queue, desc, display, surface);
            }
        } break;
#endif
        default:
            vortex::error("Platform not supported");
            break;
        }
        if (!vortex::success(result)) {
            vortex::error("Failed to create swapchain: {}", result.error);
        }
        return swapchain;
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
};

// Debug output is a window with a swapchain for rendering contents directly to the screen
export class WindowOutput : public vortex::NodeImpl<WindowOutput, IOutput>
{
public:
    WindowOutput(const vortex::Graphics& gfx, const OutputDesc& out_desc)
        : _window(out_desc.name, int(out_desc.size.width), int(out_desc.size.height), false)
    {
        if (out_desc.input != NodeInput::OUTPUT_DESC) {
            throw std::runtime_error("Invalid input type for WindowOutput");
        }

        auto [gfx_width, gfx_height] = _window.PixelSize();
        wis::SwapchainDesc desc{
            .size = { uint32_t(gfx_width), uint32_t(gfx_height) },
            .format = out_desc.format, // RGBA8Unorm is the most common format for desktop applications TODO: make this configurable + HDR
            .buffer_count = 2, // double buffering, TODO: make this configurable + query for supported buffer counts
            .stereo = false,
            .vsync = true,
            .tearing = false,
            .scaling = wis::SwapchainScaling::Stretch
        };
        _swapchain.emplace(gfx, _window.CreateSwapchain(gfx, desc), desc);
    }
    WindowOutput(const vortex::Graphics& gfx, vortex::NodeDesc* initializers)
        : WindowOutput(gfx, *reinterpret_cast<OutputDesc*>(initializers))
    {
    }

public:
    int ProcessEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                return 1;
            }
        }
        return 0;
    }

private:
    vortex::SDLWindow _window;
    std::optional<vortex::Swapchain> _swapchain;
};
} // namespace vortex