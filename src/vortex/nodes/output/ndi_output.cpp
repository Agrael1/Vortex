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

    _audio_samples.resize(800 * 2); // Reserve space for 1 second of stereo float samples at 48kHz
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

    // Audio sink
    if (sinks[1] && sinks[1].source_node != sinks[0].source_node) {
        sinks[1].source_node->Evaluate(gfx, probe, &desc);
    }

    // Check the output audio buffer and send it via NDI if available
    //auto& audio_data = probe.audio_data;
    //if (audio_data.size() > 0) {
    //    // NDI expects planar float data interleaved as L, R, L, R, ...
    //    const float* audio_ptr = reinterpret_cast<const float*>(audio_data.data());
    //    std::size_t total_samples = audio_data.size() / sizeof(float);

    //    // Move the audio data to the internal audio buffer
    //    std::size_t samples_per_channel = total_samples / probe.audio_channels;
    //    std::span audio_span{ audio_data };
    //    for (uint32_t ch = 0; ch < probe.audio_channels; ++ch) {
    //        _audio_buffer.Write(audio_span.subspan(samples_per_channel * ch, samples_per_channel), ch);
    //    }
    //}

    // Write a sinusoidal test signal if no audio data is present
    static std::size_t it = 0;
    // if (audio_data.size() == 0) {
    float frequency = 440.0f; // A4 note
    float sample_rate = 48000.0f;
    std::size_t samples_per_channel = _audio_samples.size() / probe.audio_channels;
    for (std::size_t i = 0; i < samples_per_channel; ++i) {
        float sample = std::sin(2.0f * 3.14159265f * frequency * (i + it) / sample_rate) * 0.1f; // Reduced amplitude to avoid clipping
        for (uint32_t ch = 0; ch < probe.audio_channels; ++ch) {
            _audio_samples[i + ch * samples_per_channel] = sample;
        }
    }
    it += samples_per_channel;
    //}

    // Test write audio data to the internal audio buffer and send it via NDI
    std::size_t bytes_per_channel = _audio_samples.size() / probe.audio_channels * sizeof(float);
    std::span channel_span_read = std::as_bytes(std::span{ _audio_samples });

    for (uint32_t ch = 0; ch < probe.audio_channels; ++ch) {
        _audio_buffer.Write(channel_span_read.subspan(ch * bytes_per_channel, bytes_per_channel), ch);
    }

    std::vector<std::byte> empty_buffer(_audio_samples.size() * sizeof(float), std::byte{ 0 });
    std::span channel_span = std::as_writable_bytes(std::span{ empty_buffer });

    for (uint32_t ch = 0; ch < probe.audio_channels; ++ch) {
        std::span channel_subspan = channel_span.subspan(bytes_per_channel * ch, bytes_per_channel);
        _audio_buffer.Read(channel_subspan, ch, bytes_per_channel);
    }

    // assert(std::memcmp(empty_buffer.data(), _audio_samples.data(), empty_buffer.size()) == 0 && "Audio buffer read/write mismatch");

    _swapchain.SendAudio(_audio_samples);
    
}