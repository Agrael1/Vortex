#include <vortex/nodes/output/ndi_output.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/graphics.h>
#include <vortex/probe.h>
#include <vortex/util/log.h>

vortex::NDIOutput::NDIOutput(const vortex::Graphics& gfx, SerializedProperties props)
    : ImplClass(props)
    , _swapchain(gfx, NDISwapchainDesc{ .width = window_size.x, .height = window_size.y, .format = format, .framerate = framerate, .name = name })
    , _audio_buffer(
              vortex::AudioFormat{
                      .data_format = vortex::Float32Planar,
                      .sample_rate = audio_sample_rate,
                      .channels = max_audio_channels },
              std::chrono::seconds(1)
      )
    , _desc_buffer(gfx, 64, 8)
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

    // Initialize sinks
    _sinks.sinks[1].type = graph::SinkType::Audio; // Audio sink
    std::size_t samples_per_frame = (framerate.denominator == 0 || framerate.numerator == 0) ? 0 : (48000 * framerate.denominator) / framerate.numerator;
    _audio_samples.resize(samples_per_frame * 2); // Reserve space for 1 second of stereo float samples at 48kHz
}

void vortex::NDIOutput::Update(const vortex::Graphics& gfx)
{
    if (_resized) {
        // Throttle(); // Wait for GPU to finish before resizing

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

bool vortex::NDIOutput::Evaluate(const vortex::Graphics& gfx)
{
    bool video = EvaluateVideo(gfx, _desc_buffer);
    bool audio = EvaluateAudio();
    
    // Return true if either video or audio was processed
    // The scheduler will mark this output as presented after this call
    return video || audio;
}

void vortex::NDIOutput::Throttle() const
{
    constexpr static uint64_t timeout_ns = 1'000'000'000; // 1 second timeout
    wis::Result res = _fence.Wait(_fence_value - 1, timeout_ns);
    if (res.status == wis::Status::Timeout) {
        vortex::warn("NDIOutput: Timeout while waiting for fence in Throttle(). The GPU may be unresponsive.");
    } else if (!vortex::success(res)) {
        vortex::error("NDIOutput: Failed to wait for fence in Throttle(): {}", res.error);
    }
}

bool vortex::NDIOutput::EvaluateAudio()
{
    auto sinks = _sinks.GetSinks();
    if (!sinks[1]) {
        return false; // No audio sink connected
    }

    // Not enough samples, need to read from the input
    AudioProbe audio_probe;
    audio_probe.audio_channels = 2; // Stereo
    audio_probe.audio_sample_rate = audio_sample_rate; // 48 kHz (Used to determine if input is needed)
    audio_probe.last_audio_pts = _last_audio_pts; // Start from the last PTS
    sinks[1].source_node->EvaluateAudio(audio_probe);

    if (!audio_probe.audio_data.empty()) {
        // Here is a good place to resample if needed in the future
        _audio_buffer.WritePlanar(std::span{ audio_probe.audio_data }); // Write the audio data to the buffer
        _last_audio_pts = audio_probe.last_audio_pts;
    }

    if (_audio_buffer.IsEmpty()) {
        return false; // No audio to send
    }

    std::size_t read_samples = _audio_buffer.ReadPlanar(std::span{ _audio_samples });
    if (read_samples < _audio_buffer.SamplesForFramerate(framerate)) {
        return false; // No samples to send
    }

    _swapchain.SendAudio(_audio_samples);
    return true;
}

bool vortex::NDIOutput::EvaluateVideo(const vortex::Graphics& gfx,
                                      vortex::DescriptorBuffer& desc_buffer)
{
    wis::Result result = wis::success;

    uint64_t frame_index = CurrentFrameIndex(); // Current frame index based on fence value
    uint64_t current_texture_index = _swapchain.GetCurrentIndex();

    auto textures = _swapchain.GetTextures();
    auto& current_texture = textures[current_texture_index];
    auto& current_render_target = _render_targets[current_texture_index];

    auto sink = _sinks.GetSinks()[0];
    if (!sink) {
        return false; // No video sink connected
    }

    RenderProbe probe{
        .descriptor_buffer = desc_buffer.DescBufferView(frame_index),
        .sampler_buffer = desc_buffer.SamplerBufferView(frame_index),
        .command_list = &_command_lists[frame_index],
        .output_framerate = GetFramerate() };

    // Pass to the sink nodes for post-order processing
    RenderPassForwardDesc desc{
        ._current_rt_view = current_render_target,
        ._current_rt_texture = &current_texture,
        ._output_size = { window_size.x, window_size.y }
    };

    // Barrier to ensure the render target is ready for rendering
    auto& cmd_list = *probe.command_list;
    std::ignore = cmd_list.Reset();
    desc_buffer.BindBuffers(gfx, cmd_list);
    cmd_list.TextureBarrier({
                                    .sync_before = wis::BarrierSync::Copy,
                                    .sync_after = wis::BarrierSync::RenderTarget,
                                    .access_before = wis::ResourceAccess::CopySource,
                                    .access_after = wis::ResourceAccess::RenderTarget,
                                    .state_before = wis::TextureState::CopySource,
                                    .state_after = wis::TextureState::RenderTarget,
                            },
                            current_texture);

    bool res = sink.source_node->Evaluate(gfx, probe, &desc);
    if (!res) {
        // Nothing to do, just return
        return false;
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

    // Copy the current texture to the staging buffer (it will be presented next time)
    _swapchain.CopyToStagingBuffer(_command_lists[current_texture_index]);

    // End the command list
    if (!cmd_list.Close()) {
        vortex::error("Failed to close command list for WindowOutput");
        return false;
    }

    auto& queue = gfx.GetMainQueue();
    wis::CommandListView views[]{ cmd_list };
    queue.ExecuteCommandLists(views, std::size(views));

    // Signal the fence for the current frame
    result = queue.SignalQueue(_fence, _fence_value);
    if (!vortex::success(result)) {
        vortex::error("Failed to signal fence for NDIOutput: {}", result.error);
        return false;
    }

    // We have strong ordering on the fence values, so this is safe
    // Wait for the previous frame to finish (this is where NDI blocks)
    result = _fence.Wait(_fence_value - 1);
    if (!vortex::success(result)) {
        vortex::error("Failed to wait for fence for NDIOutput: {}", result.error);
        return false;
    }

    // Present the swapchain (this will send the previous frame via NDI and may block)
    _swapchain.Present();

    ++_fence_value;
    
    // NDI output blocks during presentation, so return true to indicate successful presentation
    return true;
}
