#pragma once
#include <wisdom/wisdom.hpp>
#include <vector>

struct SDL_AudioStream;

namespace vortex {
class Graphics;
class DescriptorBuffer;

struct RenderProbe
{
    vortex::DescriptorBuffer& _descriptor_buffer;
    wis::CommandList* _command_list = nullptr; // Command list for recording commands

    wis::RenderTargetView _current_rt_view;
    const wis::Texture* _current_rt_texture;
    wis::Size2D _output_size; // Useful for different render targets
    uint64_t frame_number = 0;

    vortex::ratio32_t output_framerate = { 60, 1 }; // Default 60 FPS

    // Audio playback
    template<typename T>
    void AssignAudio(std::span<const T> data)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Audio data must be trivially copyable");
        audio_data.resize(data.size_bytes());
        std::memcpy(audio_data.data(), data.data(), data.size_bytes());
    }

    std::vector<std::byte> audio_data; // Audio data for this frame
    uint32_t audio_sample_rate = 48000; // Default 48 kHz
    uint32_t audio_channels = 2; // Default stereo
};

struct RenderPassForwardDesc {
    wis::RenderTargetView _current_rt_view;
    const wis::Texture* _current_rt_texture; // Must be in RT mode
    wis::Size2D _output_size; // Viewport
};
} // namespace vortex