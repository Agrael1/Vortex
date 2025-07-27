#pragma once
#include <vortex/ui/sdl.h>
#include <wisdom/wisdom.hpp>
#include <wisdom/wisdom_platform.hpp>
#include <vortex/graph/interfaces.h>
#include <vortex/gfx/swapchain.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/probe.h>
#include <vortex/properties/props.hpp>

namespace vortex {
// Debug output is a window with a swapchain for rendering contents directly to the screen
class WindowOutput : public vortex::graph::OutputImpl<WindowOutput, WindowOutputProperties>
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

        size = desc.size; // Update size to match the swapchain size

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
        for (size_t i = 0; i < 2; ++i) {
            _command_lists[i] = gfx.GetDevice().CreateCommandList(result, wis::QueueType::Graphics);
            if (!vortex::success(result)) {
                vortex::error("Failed to create command list for WindowOutput: {}", result.error);
                return;
            }
        }
        _fence = gfx.GetDevice().CreateFence(result);
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
    void Throttle(const vortex::Graphics& gfx)
    {
        auto& q = gfx.GetMainQueue();
        q.SignalQueue(_fence, ++_fence_value);
        _fence.Wait(_fence_value);
    }

    // Setters for properties
    void SetName(std::string_view name, bool notify = false)
    {
        WindowOutputProperties::SetName(name, true);
        _window.SetTitle(name.data());
    }
    void SetWindowSize(DirectX::XMUINT2 size, bool notify = false)
    {
        WindowOutputProperties::SetWindowSize(size, true);
        _window.SetSize(int(size.x), int(size.y));
        _resized = true; // Mark as resized to trigger swapchain resize
    }

    void Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr) override
    {
        if (_resized) {
            // Resize the swapchain if the window has been resized
            auto [width, height] = _window.PixelSize();
            Throttle(gfx); // Ensure the GPU is ready for the resize operation
            _swapchain.Resize(width, height);
            // Recreate the render targets after resizing the swapchain
            wis::Result result = wis::success;
            for (size_t i = 0; i < _textures.size(); ++i) {
                _render_targets[i] = gfx.GetDevice().CreateRenderTarget(result, _textures[i], wis::RenderTargetDesc{ .format = wis::DataFormat::RGBA8Unorm });
                if (!vortex::success(result)) {
                    vortex::error("Failed to recreate render target after swapchain resize: {}", result.error);
                    return;
                }
            }
            _textures = _swapchain.GetBufferSpan(); // Update textures to match the new swapchain buffers
            _resized = false; // Reset the resized flag
        }

        _current_texture_index = _swapchain.GetCurrentIndex();
        probe._command_list = &_command_lists[_current_texture_index];

        // Pass to the sink nodes for post-order processing
        RenderPassForwardDesc desc{
            ._current_rt_view = _render_targets[_current_texture_index],
            ._current_rt_texture = &_textures[_current_texture_index],
            ._output_size = { window_size.x, window_size.y }
        };

        // Barrier to ensure the render target is ready for rendering
        auto& cmd_list = *probe._command_list;
        cmd_list.Reset();
        probe._descriptor_buffer.BindBuffers(gfx, cmd_list);

        cmd_list.TextureBarrier({
                                        .sync_before = wis::BarrierSync::None,
                                        .sync_after = wis::BarrierSync::RenderTarget,
                                        .access_before = wis::ResourceAccess::NoAccess,
                                        .access_after = wis::ResourceAccess::RenderTarget,
                                        .state_before = wis::TextureState::Present,
                                        .state_after = wis::TextureState::RenderTarget,
                                },
                                _textures[_current_texture_index]);

        // Pass to the next nodes in the graph
        for (auto& sink : _sinks.GetSinks() | std::views::filter([](auto& sink) { return sink; })) {
            sink.source_node->Evaluate(gfx, probe, &desc);
        }

        // Close the render target
        cmd_list.TextureBarrier({
                                        .sync_before = wis::BarrierSync::RenderTarget,
                                        .sync_after = wis::BarrierSync::RenderTarget,
                                        .access_before = wis::ResourceAccess::RenderTarget,
                                        .access_after = wis::ResourceAccess::Common,
                                        .state_before = wis::TextureState::RenderTarget,
                                        .state_after = wis::TextureState::Present,
                                },
                                _textures[_current_texture_index]);

        // End the command list
        if (!cmd_list.Close()) {
            vortex::error("Failed to close command list for WindowOutput");
            return;
        }

        wis::CommandListView views[]{ cmd_list };
        gfx.GetMainQueue().ExecuteCommandLists(views, 1);

        // Present the swapchain
        _swapchain.Present();
    }

private:
    vortex::ui::SDLWindow _window;

public:
    wis::CommandList _command_lists[2]; ///< Command list for rendering
    wis::SwapChain _swapchain;
    wis::RenderTarget _render_targets[2]; ///< Render target for the swapchain
    std::span<const wis::Texture> _textures; ///< Textures for the swapchain
    uint32_t _current_texture_index = 0; ///< Current texture index for rendering

    wis::Fence _fence; ///< Fence for synchronization
    uint64_t _fence_value = 0; ///< Current fence value for synchronization
    bool _resized = false; ///< Flag to indicate if the window has been resized
};
} // namespace vortex