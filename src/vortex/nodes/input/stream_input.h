#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/probe.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/util/reflection.h>
#include <DirectXMath.h>

#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>
#include <vortex/util/ffmpeg/stream_manager.h>

namespace vortex {
// Will hold static data for the image input node
struct StreamInputLazy {
public:
    StreamInputLazy(const vortex::Graphics& gfx);

public:
    wis::Sampler _sampler; // Sampler for the texture
    wis::RootSignature _root_signature; // Root signature for the image input node
    wis::PipelineState _pipeline_state; // Pipeline state for rendering the image
    ffmpeg::StreamManager _manager; // Stream manager for handling streams
};

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class StreamInput : public vortex::graph::NodeImpl<StreamInput, StreamInputProperties, 0, 1>
{
private:
    static void UnregisterStream(ffmpeg::StreamManager::StreamHandle handle) noexcept
    {
        lazy_ptr<StreamInputLazy>::uget()._manager.UnregisterStream(handle);
    }
    using unique_stream = vortex::unique_any<ffmpeg::StreamManager::StreamHandle, UnregisterStream>;
    unique_stream MakeUniqueStream(ffmpeg::unique_context context, std::span<int> active_channel_indices) noexcept
    {
        return unique_stream{ lazy_ptr<StreamInputLazy>::uget()._manager.RegisterStream(std::move(context), active_channel_indices) };
    }

public:
    StreamInput(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props), _lazy_data(gfx)
    {
    }

public:
    void Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe) override;
    void Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info = nullptr) override;

    vortex::graph::NodeExecution Validate(const vortex::Graphics& gfx, const vortex::RenderProbe& probe)
    {
        return vortex::graph::NodeExecution::Render; // Proceed with rendering
    }

public:
    void SetStreamUrl(std::string_view value, bool notify = false)
    {
        StreamInputProperties::SetStreamUrl(value, notify);
        url_changed = true;
    }
    std::vector<uint8_t> GetAudioForPlayback(const vortex::RenderProbe& probe);

private:
    void InitializeStream();
    void DecodeStreamFrames(const vortex::Graphics& gfx);
    void TrimOldFrames();

    ffmpeg::unique_frame* SelectFrameForCurrentTime(const vortex::RenderProbe& probe);
    int64_t CalculateTargetVideoPTS(const vortex::RenderProbe& probe) const;
    int64_t CalculateTargetAudioPTS(const vortex::RenderProbe& probe) const;
    void ExtractStreamTiming();

private:
    [[no_unique_address]] lazy_ptr<StreamInputLazy> _lazy_data; // Lazy data for static resources
    std::array<wis::ShaderResource, 2> _shader_resources[vortex::max_frames_in_flight]; // Shader resource for the texture
    wis::Texture _textures[vortex::max_frames_in_flight]; // Textures for each frame in flight
    wis::Fence _fences[vortex::max_frames_in_flight]; // Fences for each frame in flight

    // Stream related data
    codec::StreamChannels _stream_collection; // Collection of streams

    std::map<int64_t, ffmpeg::unique_frame> _video_frames; // Map of video frames by pts
    std::map<int64_t, ffmpeg::unique_frame> _audio_frames; // Map of audio frames by pts
    std::array<int64_t, 2> _stream_indices{}; // Last rendered video PTS for each frame in flight

    unique_stream _stream_handle; // Handle to the stream managed by StreamManager

    std::chrono::steady_clock::time_point _start_time;

    // Stream timing information extracted from actual stream data
    struct StreamTimingInfo {
        // Video timing
        AVRational video_timebase = { 0, 0 }; // Video stream timebase
        AVRational video_framerate = { 0, 0 }; // Video stream framerate
        int64_t first_video_pts = AV_NOPTS_VALUE;

        // Audio timing
        AVRational audio_timebase = { 0, 0 }; // Audio stream timebase
        int audio_sample_rate = 0; // Audio sample rate (Hz)
        int64_t first_audio_pts = AV_NOPTS_VALUE;

        std::chrono::steady_clock::time_point start_time;
        bool video_initialized = false;
        bool audio_initialized = false;
    } _stream_timing;

    ffmpeg::unique_swscontext _sws_context;
    ffmpeg::unique_swrcontext _swr_context;
    bool url_changed = true; // Flag to check if the node has been initialized
};
} // namespace vortex