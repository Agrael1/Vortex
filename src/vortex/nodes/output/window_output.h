#pragma once
#include <vortex/ui/sdl.h>
#include <wisdom/wisdom.hpp>
#include <wisdom/wisdom_platform.hpp>
#include <vortex/graph/interfaces.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/probe.h>
#include <vortex/properties/props.hpp>

namespace vortex {
// Debug output is a window with a swapchain for rendering contents directly to the screen
class WindowOutput : public vortex::graph::OutputImpl<WindowOutput, WindowOutputProperties>
{
    std::chrono::time_point<std::chrono::steady_clock> _last_frame_time;
    static constexpr wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Default format for render targets
public:
    WindowOutput(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props), _window(name.data(), int(window_size.x), int(window_size.y), false)
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
        for (size_t i = 0; i < 2; ++i) {
            _command_lists[i] = gfx.GetDevice().CreateCommandList(result, wis::QueueType::Graphics);
            if (!vortex::success(result)) {
                vortex::error("Failed to create command list for WindowOutput: {}", result.error);
                return;
            }
        }
        _fence = gfx.GetDevice().CreateFence(result);
        _last_frame_time = std::chrono::steady_clock::now();
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
    void Throttle() noexcept
    {
        _fence.Wait(_fence_values[_frame_index] - 1);
    }
    void Present(const vortex::Graphics& gfx) noexcept
    {
        // Present the swapchain
        if (auto result = _swapchain.Present(); !vortex::success(result)) {
            vortex::error("Failed to present swapchain: {}", result.error);
            return;
        }

        auto& queue = gfx.GetMainQueue();
        auto result = queue.SignalQueue(_fence, _fence_values[_frame_index]);
        if (!vortex::success(result)) {
            vortex::error("Failed to signal queue for WindowOutput: {}", result.error);
            return;
        }

        _frame_index = (_frame_index + 1) % vortex::max_frames_in_flight;
        _fence_values[_frame_index] = ++_fence_value;
    }

    // Setters for properties
    void SetName(std::string_view name, bool notify = false)
    {
        WindowOutputProperties::SetName(name, true);
        if (IsInitialized()) {
            _window.SetTitle(name.data());
        }
    }
    void SetWindowSize(DirectX::XMUINT2 size, bool notify = false)
    {
        WindowOutputProperties::SetWindowSize(size, true);
        if (!IsInitialized()) {
            return; // Do not resize the window if it is not initialized
        }
        _window.SetSize(int(size.x), int(size.y));
        _resized = true; // Mark as resized to trigger swapchain resize
    }

public:
    virtual bool IsReady() const noexcept override
    {
        // Check if the swapchain is ready for rendering
        auto result = _fence.Wait(_fence_values[_frame_index] - 1, 0);
        if (result.status == wis::Status::Timeout) {
            // If the fence is not ready, return false
            return false;
        } else if (!vortex::success(result)) {
            vortex::error("Failed to wait for fence in WindowOutput: {}", result.error);
            return false;
        }

        return true;
    }
    void Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const RenderPassForwardDesc* output_info = nullptr) override
    {
        if (!IsReady()) {
            vortex::info("WindowOutput is not ready for rendering");
            return;
        }

        if (_resized) {
            // Resize the swapchain if the window has been resized
            auto [width, height] = _window.PixelSize();
            Throttle(); // Ensure the GPU is ready for the resize operation

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

        auto current_texture_index = _swapchain.GetCurrentIndex();
        probe._command_list = &_command_lists[_frame_index];

        // Pass to the sink nodes for post-order processing
        RenderPassForwardDesc desc{
            ._current_rt_view = _render_targets[_frame_index],
            ._current_rt_texture = &_textures[_frame_index],
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
                                _textures[current_texture_index]);

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
                                _textures[current_texture_index]);

        // End the command list
        if (!cmd_list.Close()) {
            vortex::error("Failed to close command list for WindowOutput");
            return;
        }
        gfx.ExecuteCommandLists({ cmd_list });

        Present(gfx); // Present the swapchain after rendering


        // Get the current time for frame
        auto now = std::chrono::steady_clock::now();
        auto frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _last_frame_time).count();

        // Change the window title to include the fps
        std::string title = std::format("{} - FPS: {:.2f}", name, 1000.0 / frame_duration);

        _last_frame_time = now;
        _window.SetTitle(title.c_str());
    }

private:
    vortex::ui::SDLWindow _window;

public:
    wis::CommandList _command_lists[2]; ///< Command list for rendering
    wis::SwapChain _swapchain;
    wis::RenderTarget _render_targets[2]; ///< Render target for the swapchain
    std::span<const wis::Texture> _textures; ///< Textures for the swapchain

    uint32_t _frame_index = 0; ///< Current frame index for synchronization

    wis::Fence _fence; ///< Fence for synchronization
    uint64_t _fence_value = 1; ///< Current fence value for synchronization
    uint64_t _fence_values[vortex::max_frames_in_flight] = { 1, 0 }; ///< Current fence value for synchronization
    bool _resized = false; ///< Flag to indicate if the window has been resized
};
} // namespace vortex