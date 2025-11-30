#include <vortex/nodes/output/window_output.h>
#include <vortex/graphics.h>

vortex::WindowOutput::WindowOutput(const vortex::Graphics& gfx, SerializedProperties props)
    : ImplClass(props)
    , _window(name.data(), int(window_size.x), int(window_size.y), false)
    , _desc_buffer(gfx, 256, 32)
    , _texture_pool(gfx,
                    {
                            .format = format,
                            .size = { window_size.x, window_size.y }
})
{
    wis::Result result = wis::success;
    auto& device = gfx.GetDevice();

    auto [gfx_width, gfx_height] = _window.PixelSize();
    wis::SwapchainDesc desc{
        .size = { uint32_t(gfx_width), uint32_t(gfx_height) },
        .format = format, // RGBA8Unorm is the most common format for desktop applications TODO:
        // make this configurable + HDR
        .buffer_count = max_swapchain_images, // double buffering, TODO: make this configurable +
        // query for supported buffer counts
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
        _render_targets[i] = device.CreateRenderTarget(result, _textures[i], rt_desc);
        if (!vortex::success(result)) {
            vortex::error("Failed to create render target for WindowOutput: {}", result.error);
            return;
        }
    }
    for (size_t i = 0; i < 2; ++i) {
        _command_lists[i] = device.CreateCommandList(result, wis::QueueType::Graphics);
        if (!vortex::success(result)) {
            vortex::error("Failed to create command list for WindowOutput: {}", result.error);
            return;
        }
    }
    _fence = device.CreateFence(result);
    if (!vortex::success(result)) {
        vortex::error("Failed to create fence for WindowOutput: {}", result.error);
        return;
    }
}

void vortex::WindowOutput::Throttle() noexcept
{
    constexpr static uint64_t timeout_ns = 1'000'000'000; // 1 second timeout
    wis::Result res = _fence.Wait(_fence_values[_frame_index] - 1, timeout_ns);
    if (res.status == wis::Status::Timeout) {
        vortex::warn("WindowOutput: Timeout while waiting for fence in Throttle(). The GPU may be "
                     "unresponsive.");
    } else if (!vortex::success(res)) {
        vortex::error("WindowOutput: Failed to wait for fence in Throttle(): {}", res.error);
    }
}

void vortex::WindowOutput::Present(const vortex::Graphics& gfx) noexcept
{
    // Present the swapchain (non-blocking for window output)
    if (auto result = _swapchain.Present(); !vortex::success(result)) {
        vortex::error("Failed to present swapchain: {}", result.error);
        return;
    }

    auto& queue = gfx.GetMainQueue();
    auto result = queue.SignalQueue(_fence, _fence_value);
    if (!vortex::success(result)) {
        vortex::error("Failed to signal queue for WindowOutput: {}", result.error);
        return;
    }

    // Non-blocking fence check for window output
    _frame_index = _swapchain.GetCurrentIndex() % vortex::max_frames_in_flight;
    result = _fence.Wait(_fence_values[_frame_index]);
    if (!vortex::success(result)) {
        vortex::error("Failed to wait for fence in WindowOutput: {}", result.error);
        return;
    }

    _fence_values[_frame_index] = ++_fence_value;
}

void vortex::WindowOutput::Update(const vortex::Graphics& gfx)
{
    if (_resized) {
        // Resize the swapchain if the window has been resized
        auto [width, height] = _window.PixelSize();
        Throttle(); // Ensure the GPU is ready for the resize operation

        std::ignore = _swapchain.Resize(width, height);
        // Recreate the render targets after resizing the swapchain
        wis::Result result = wis::success;
        for (size_t i = 0; i < _textures.size(); ++i) {
            _render_targets[i] = gfx.GetDevice().CreateRenderTarget(
                    result,
                    _textures[i],
                    wis::RenderTargetDesc{ .format = wis::DataFormat::RGBA8Unorm });
            if (!vortex::success(result)) {
                vortex::error("Failed to recreate render target after swapchain resize: {}",
                              result.error);
                return;
            }
        }
        _textures = _swapchain.GetBufferSpan(); // Update textures to match the new swapchain
                                                // buffers
        _resized = false; // Reset the resized flag
    }
}

bool vortex::WindowOutput::Evaluate(const vortex::Graphics& gfx, int64_t pts)
{
    auto sink = _sinks.sinks[0];

    if (!sink) {
        return false; // No source connected, nothing to render
    }

    // Pass to the sink nodes for post-order processing
    RenderPassForwardDesc desc{
        .current_rt_view = _render_targets[_frame_index],
        .output_size = { window_size.x, window_size.y },
        .rt_generation = invalid_generation,
        .depth = 1, // send +1
    };
    vortex::RenderProbe probe{
        .descriptor_buffer = _desc_buffer.DescBufferView(_frame_index),
        .sampler_buffer = _desc_buffer.SamplerBufferView(_frame_index),
        .texture_pool = _texture_pool,
        .command_list = &_command_lists[_frame_index],
        .frame_number = _frame_index,
        .output_framerate = GetFramerate(),

        // PTS timing information (90kHz timebase)
        .current_pts = pts,
        .output_base_pts = GetBasePTS(),
    };

    // Barrier to ensure the render target is ready for rendering
    auto& cmd_list = *probe.command_list;
    std::ignore = cmd_list.Reset();
    _desc_buffer.BindBuffers(gfx, cmd_list);

    cmd_list.TextureBarrier(
            {
                    .sync_before = wis::BarrierSync::None,
                    .sync_after = wis::BarrierSync::RenderTarget,
                    .access_before = wis::ResourceAccess::NoAccess,
                    .access_after = wis::ResourceAccess::RenderTarget,
                    .state_before = wis::TextureState::Present,
                    .state_after = wis::TextureState::RenderTarget,
            },
            _textures[_frame_index]);

    // Pass to the next nodes in the graph
    bool rendered = sink.source_node->Evaluate(gfx, probe, &desc);
    if (!rendered) {
        return false; // Rendering failed
    }

    // Close the render target
    cmd_list.TextureBarrier(
            {
                    .sync_before = wis::BarrierSync::RenderTarget,
                    .sync_after = wis::BarrierSync::RenderTarget,
                    .access_before = wis::ResourceAccess::RenderTarget,
                    .access_after = wis::ResourceAccess::Common,
                    .state_before = wis::TextureState::RenderTarget,
                    .state_after = wis::TextureState::Present,
            },
            _textures[_frame_index]);

    // End the command list
    if (!cmd_list.Close()) {
        vortex::error("Failed to close command list for WindowOutput");
        return false;
    }
    gfx.ExecuteCommandLists({ cmd_list });

    // Present the swapchain after rendering (non-blocking for window)
    Present(gfx);
    _texture_pool.SwapFrame(); // Swap the texture pool frame

    // Window output doesn't block, so return true immediately
    return true;
}
