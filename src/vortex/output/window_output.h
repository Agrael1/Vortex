#pragma once
#include <vortex/ui/sdl.h>
#include <wisdom/wisdom.hpp>
#include <wisdom/wisdom_platform.hpp>
#include <vortex/node.h>
#include <vortex/gfx/swapchain.h>

namespace vortex {
struct WindowOutputProperties {
    std::string window_name = "Vortex 1";
    wis::Size2D size = { 1280, 720 };
    wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Default format for the swapchain
};
// Debug output is a window with a swapchain for rendering contents directly to the screen
class WindowOutput : public vortex::OutputImpl<WindowOutput, WindowOutputProperties>
{
public:
    WindowOutput(const vortex::Graphics& gfx, wis::Size2D size = { 1280, 720 }, wis::DataFormat format = wis::DataFormat::RGBA8Unorm, std::string_view name = "Vortex 1")
        : _window(name.data(), int(size.width), int(size.height), false)
    {
        wis::Result result = wis::success;
        auto [gfx_width, gfx_height] = _window.PixelSize();
        wis::SwapchainDesc desc{
            .size = { uint32_t(gfx_width), uint32_t(gfx_height) },
            .format = format, // RGBA8Unorm is the most common format for desktop applications TODO: make this configurable + HDR
            .buffer_count = 2, // double buffering, TODO: make this configurable + query for supported buffer counts
            .stereo = false,
            .vsync = true,
            .tearing = false,
            .scaling = wis::SwapchainScaling::Stretch
        };
        _swapchain = _window.CreateSwapchain(gfx, desc);

        _textures = _swapchain.GetBufferSpan();

        wis::RenderTargetDesc rt_desc{
            .format = format,
        };
        for (size_t i = 0; i < _textures.size(); ++i) {
            _render_targets[i] = gfx.GetDevice().CreateRenderTarget(result, _textures[i], rt_desc);
            if (!vortex::success(result)) {
                vortex::error("Failed to create render target for WindowOutput: {}", result.error);
                return;
            }
        }
    }

public:
    void Visit(RenderProbe& probe) override
    {
        // Start a render pass
    }
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

public:
    wis::SwapChain _swapchain;
    wis::RenderTarget _render_targets[2]; ///< Render target for the swapchain
    std::span<const wis::Texture> _textures; ///< Textures for the swapchain
    uint32_t _current_texture_index = 0; ///< Current texture index for rendering
};
} // namespace vortex