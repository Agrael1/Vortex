#pragma once
#include <wisdom/wisdom.hpp>
#include <vector>
#include <vortex/util/rational.h>

struct SDL_AudioStream;

namespace vortex {
class Graphics;
class DescriptorBuffer;

struct RenderProbe
{
    vortex::DescriptorBuffer& descriptor_buffer;
    wis::CommandList* command_list = nullptr; // Command list for recording commands
    wis::Size2D output_size; // Useful for different render targets
    uint64_t frame_number = 0;

    vortex::ratio32_t output_framerate = { 60, 1 }; // Default 60 FPS

    // PTS timing information (90kHz timebase)
    uint64_t current_pts = 0;     // Current presentation timestamp
    uint64_t target_pts = 0;      // Target presentation timestamp for this frame
};

struct AudioProbe {
    constexpr inline static uint64_t invalid_pts = 0x8000000000000000ull;

    std::vector<float> audio_data; // Audio data for this frame
    uint32_t audio_sample_rate = 48000; // Default 48 kHz
    uint32_t audio_channels = 2; // Default stereo
    uint64_t first_audio_pts = invalid_pts; // First audio PTS for synchronization
    uint64_t last_audio_pts = invalid_pts; // Last audio PTS for synchronization
};

struct RenderPassForwardDesc {
    wis::RenderTargetView _current_rt_view;
    const wis::Texture* _current_rt_texture; // Must be in RT mode
    wis::Size2D _output_size; // Viewport
};
} // namespace vortex