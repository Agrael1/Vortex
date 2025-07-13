#pragma once
#include <vortex/ui/sdl.h>
#include <wisdom/wisdom.hpp>
#include <wisdom/wisdom_platform.hpp>
#include <vortex/node.h>
#include <vortex/gfx/swapchain.h>

namespace vortex {
// Debug output is a window with a swapchain for rendering contents directly to the screen
class WindowOutput : public vortex::NodeImpl<WindowOutput, IOutput>
{
public:
    WindowOutput(const vortex::Graphics& gfx, const OutputDesc& out_desc)
        : _window(out_desc.name, int(out_desc.size.width), int(out_desc.size.height), false)
    {
        wis::Result result = wis::success;
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
        _swapchain = _window.CreateSwapchain(gfx, desc);

        _textures = _swapchain.GetBufferSpan();

        wis::RenderTargetDesc rt_desc{
            .format = out_desc.format,
        };
        for (size_t i = 0; i < _textures.size(); ++i) {
            _render_targets[i] = gfx.GetDevice().CreateRenderTarget(result, _textures[i], rt_desc);
            if (!vortex::success(result)) {
                vortex::error("Failed to create render target for WindowOutput: {}", result.error);
                return;
            }
        }
    }
    WindowOutput(const vortex::Graphics& gfx, vortex::NodeDesc* initializers)
        : WindowOutput(gfx, *reinterpret_cast<OutputDesc*>(initializers))
    {
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