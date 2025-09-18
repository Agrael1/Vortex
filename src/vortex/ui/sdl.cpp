#include <vortex/ui/sdl.h>
#include <vortex/platform.h>
#include <vortex/util/log.h>
#include <vortex/graphics.h>
#include <vortex/util/common.h>
#include <wisdom/wisdom_platform.hpp>

typedef struct HWND__* HWND;

wis::SwapChain vortex::ui::SDLWindow::CreateSwapchain(const vortex::Graphics& gfx, const wis::SwapchainDesc& desc) const noexcept
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
            swapchain = static_cast<const wis::platform::X11Extension*>(platform.GetExtension())
                                ->CreateSwapchain(result, device, main_queue, desc, xdisplay, xwindow);
        }
    } break;
    case Wayland: {
        struct wl_display* display = (struct wl_display*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        struct wl_surface* surface = (struct wl_surface*)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        if (display && surface) {
            swapchain = static_cast<const wis::platform::WaylandExtension*>(platform.GetExtension())
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
