#pragma once
#include <vortex/ui/sdl.h>
#include <wisdom/wisdom.hpp>
#include <wisdom/wisdom_platform.hpp>
#include <vortex/node.h>
#include <vortex/swapchain.h>

typedef struct HWND__* HWND;

namespace vortex {
// Debug output is a window with a swapchain for rendering contents directly to the screen
class WindowOutput : public vortex::NodeImpl<WindowOutput, IOutput>
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
    vortex::ui::SDLWindow _window;
    std::optional<vortex::Swapchain> _swapchain;
};
} // namespace vortex