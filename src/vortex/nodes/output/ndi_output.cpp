#include <vortex/nodes/output/ndi_output.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/graphics.h>
#include <vortex/probe.h>
#include <vortex/util/log.h>

vortex::NDIOutput::NDIOutput(const vortex::Graphics& gfx, SerializedProperties props)
    : ImplClass(props)
    , _swapchain(gfx, NDISwapchainDesc{ .width = window_size.x, .height = window_size.y, .format = format, .framerate = framerate, .name = name })
{
    wis::Result result = wis::success;

    // Create the render target for the NDI output
    auto textures = _swapchain.GetTextures();

    wis::RenderTargetDesc rt_desc{
        .format = format,
    };
    for (size_t i = 0; i < textures.size(); ++i) {
        _render_targets[i] = gfx.GetDevice().CreateRenderTarget(result, textures[i], rt_desc);
        if (!vortex::success(result)) {
            vortex::error("Failed to create render target for NDIOutput: {}", result.error);
            return;
        }
    }

    // Initialize command lists for each swapchain image
    for (size_t i = 0; i < NDISwapchain::max_swapchain_images; ++i) {
        _command_lists[i] = gfx.GetDevice().CreateCommandList(result, wis::QueueType::Graphics);
        if (!vortex::success(result)) {
            vortex::error("Failed to create command list for NDIOutput: {}", result.error);
            return;
        }
    }

    // Create the fence for synchronization
    _fence = gfx.GetDevice().CreateFence(result);
    if (!vortex::success(result)) {
        vortex::error("Failed to create fence for NDIOutput: {}", result.error);
        return;
    }
}
vortex::NDIOutput::~NDIOutput()
{
    Throttle();
}

void vortex::NDIOutput::Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe)
{
    if (_resized) {
        Throttle(); // Wait for GPU to finish before resizing

        // Resize the swapchain and render target
        if (_swapchain.Resize(gfx, window_size.x, window_size.y)) {
            auto textures = _swapchain.GetTextures();
            for (size_t i = 0; i < textures.size(); ++i) {
                wis::Result result = wis::success;
                _render_targets[i] = gfx.GetDevice().CreateRenderTarget(result, textures[i], wis::RenderTargetDesc{ .format = format });
                if (!vortex::success(result)) {
                    vortex::error("Failed to recreate render target after swapchain resize: {}", result.error);
                    return;
                }
            }
        }
        _resized = false;
    }
}
void vortex::NDIOutput::Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const RenderPassForwardDesc* output_info)
{
    uint64_t frame_index = (_fence_value - 1) % vortex::max_frames_in_flight; // Current frame index based on fence value
    uint64_t current_texture_index = _swapchain.GetCurrentIndex();

    auto textures = _swapchain.GetTextures();
    auto& current_texture = textures[current_texture_index];
    auto& current_render_target = _render_targets[current_texture_index];

    probe._command_list = &_command_lists[frame_index];
    probe.output_framerate = GetFramerate();

    // Pass to the sink nodes for post-order processing
    RenderPassForwardDesc desc{
        ._current_rt_view = current_render_target,
        ._current_rt_texture = &current_texture,
        ._output_size = { window_size.x, window_size.y }
    };

    // Barrier to ensure the render target is ready for rendering
    auto& cmd_list = *probe._command_list;
    std::ignore = cmd_list.Reset();
    probe._descriptor_buffer.BindBuffers(gfx, cmd_list);

    cmd_list.TextureBarrier({
                                    .sync_before = wis::BarrierSync::Copy,
                                    .sync_after = wis::BarrierSync::RenderTarget,
                                    .access_before = wis::ResourceAccess::CopySource,
                                    .access_after = wis::ResourceAccess::RenderTarget,
                                    .state_before = wis::TextureState::CopySource,
                                    .state_after = wis::TextureState::RenderTarget,
                            },
                            current_texture);

    // Pass to the next nodes in the graph
    for (auto& sink : _sinks.GetSinks() | std::views::filter([](auto& sink) { return sink; })) {
        sink.source_node->Evaluate(gfx, probe, &desc);
    }

    // Close the render target
    cmd_list.TextureBarrier({
                                    .sync_before = wis::BarrierSync::RenderTarget,
                                    .sync_after = wis::BarrierSync::Copy,
                                    .access_before = wis::ResourceAccess::RenderTarget,
                                    .access_after = wis::ResourceAccess::CopySource,
                                    .state_before = wis::TextureState::RenderTarget,
                                    .state_after = wis::TextureState::CopySource,
                            },
                            current_texture);

    _swapchain.Present(_command_lists[current_texture_index]);

    // End the command list
    if (!cmd_list.Close()) {
        vortex::error("Failed to close command list for WindowOutput");
        return;
    }

    auto& queue = gfx.GetMainQueue();
    wis::CommandListView views[]{ cmd_list };
    queue.ExecuteCommandLists(views, std::size(views));
    std::ignore = queue.SignalQueue(_fence, _fence_values[frame_index]);

    ++_fence_value;
    _fence_values[_fence_value % vortex::max_frames_in_flight] = _fence_value;
}
