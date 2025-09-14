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
                      .sample_rate = 48000,
                      .channels = 2 },
              std::chrono::seconds(1))
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

    auto sinks = _sinks.GetSinks();

    // Pass to the sink nodes for post-order processing
    RenderPassForwardDesc desc{
        ._current_rt_view = current_render_target,
        ._current_rt_texture = &current_texture,
        ._output_size = { window_size.x, window_size.y }
    };

    // Barrier to ensure the render target is ready for rendering
    auto& cmd_list = *probe._command_list;

    // Video sink
    if (sinks[0]) {
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

        sinks[0].source_node->Evaluate(gfx, probe, &desc);

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

    EvaluateAudio();
}

#include <vortex/util/bench_clock.h>

void vortex::NDIOutput::EvaluateAudio()
{
    auto sinks = _sinks.GetSinks();
    if (!sinks[1]) {
        return; // No audio sink connected
    }

    static constexpr std::size_t sample_rate = 48000; // NDI expects 48 kHz audio
    // Check if we have enough samples to read for the current framerate
    //if (_audio_buffer.CanReadForFramerate(framerate)) {
    //    _audio_buffer.ReadPlanar(std::span{ _audio_samples });
    //    _swapchain.SendAudio(_audio_samples);
    //    return;
    //}

    // Not enough samples, need to read from the input
    AudioProbe audio_probe;
    audio_probe.audio_channels = 2; // Stereo
    audio_probe.audio_sample_rate = sample_rate; // 48 kHz (Used to determine if input is needed)
    audio_probe.last_audio_pts = _last_audio_pts; // Start from the last PTS
    sinks[1].source_node->EvaluateAudio(audio_probe);

    if (!audio_probe.audio_data.empty()) {
        // If audio data has PTS that is not continuous, reset the buffer
        //std::size_t tolerance = (framerate.denominator == 0 || framerate.numerator == 0)
        //        ? 0
        //        : (sample_rate * framerate.denominator) / framerate.numerator; // Tolerance of 1 frame
        //
        //if (_last_audio_pts != 0 && audio_probe.first_audio_pts > _last_audio_pts + tolerance) {
        //    vortex::warn("NDIOutput: Audio PTS discontinuity detected (last: {}, current: {}), resetting audio buffer", _last_audio_pts, audio_probe.first_audio_pts);
        //    _audio_buffer.Reset();
        //}

        // Here is a good place to resample if needed in the future
        _audio_buffer.WritePlanar(std::span{ audio_probe.audio_data }); // Write the audio data to the buffer
        _last_audio_pts = audio_probe.last_audio_pts;
    }


    std::size_t read_samples = _audio_buffer.ReadPlanar(std::span{ _audio_samples });
    if (read_samples < _audio_buffer.SamplesForFramerate(framerate)) {
        //vortex::warn("NDIOutput: Not enough audio samples to send to NDI (needed {}, got {})", _audio_buffer.SamplesForFramerate(framerate), read_samples);
        return; // No samples to send
    }

    _swapchain.SendAudio(_audio_samples);
}
