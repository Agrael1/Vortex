#pragma once
#include <wisdom/wisdom.hpp>
#include <vector>
#include <vortex/util/rational.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/gfx/texture_pool.h>

struct SDL_AudioStream;

namespace vortex {
struct RenderProbe
{
    vortex::DescriptorBufferView descriptor_buffer;
    vortex::DescriptorBufferView sampler_buffer;
    vortex::TexturePool& texture_pool; // Texture for rendering

    wis::CommandList* command_list = nullptr; // Command list for recording commands
    uint64_t frame_number = 0;

    // PTS timing information (90kHz timebase)
    vortex::ratio32_t output_framerate = { 60, 1 }; // Default 60 FPS
    int64_t current_pts = invalid_pts;     // Current presentation timestamp
    int64_t output_base_pts = invalid_pts; // Target presentation timestamp for this frame
};

struct AudioProbe {
    std::vector<float> audio_data; // Audio data for this frame
    uint32_t audio_sample_rate = 48000; // Default 48 kHz
    uint32_t audio_channels = 2; // Default stereo
    int64_t first_audio_pts = invalid_pts; // First audio PTS for synchronization
    int64_t last_audio_pts = invalid_pts; // Last audio PTS for synchronization
};

struct RenderPassForwardDesc {
    wis::RenderTargetView current_rt_view;
    wis::Size2D output_size; // Viewport
    wis::DataFormat format = wis::DataFormat::RGBA8Unorm; // Format of the render target
};
} // namespace vortex