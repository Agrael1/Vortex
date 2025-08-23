#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/probe.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/util/reflection.h>
#include <DirectXMath.h>

#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>
#include <vortex/util/lib/SPSC-Queue.h>
#include <condition_variable>
#include <shared_mutex>
#include <memory>

namespace vortex {
struct PacketStorage {
    dro::SPSCQueue<AVPacket, 16> packets; // Queue for storing AVPackets
};

// Represents a stream being read by the StreamManager
struct ManagedStream {
    struct UpdateRequest {
        uint32_t stream_index : 31;
        uint32_t active : 1; // 1 for activate, 0 for deactivate
    };

    // Only accessed from the I/O thread
    ffmpeg::unique_context context;
    std::unordered_map<int, PacketStorage> channels;

    // Modifiable from outside the I/O thread
    std::atomic<bool> update_pending{ false };
    std::vector<UpdateRequest> updates;
};

// Manages all stream I/O in a dedicated thread pool.
class StreamManager
{
public:
    using StreamHandle = uintptr_t;

public:
    StreamManager();
    ~StreamManager();

public:
    StreamHandle RegisterStream(ffmpeg::unique_context context, std::span<int> active_channel_indices);
    void UnregisterStream(StreamHandle handle);

    void SetChannelActive(StreamHandle handle, int stream_index, bool active);
    void ActivateChannels(StreamHandle handle, std::span<int> active_channel_indices);
    void DeactivateChannels(StreamHandle handle, std::span<int> inactive_channel_indices);

    bool try_get_packet(StreamHandle handle, int stream_index, AVPacket& packet);

private:
    void IoLoop(std::stop_token stop);

    std::vector<std::jthread> _io_threads;

    // Control for unpdating streams
    std::atomic<bool> _update_pending{ true };
    std::shared_mutex _streams_mutex;
    std::unordered_map<StreamHandle, std::shared_ptr<ManagedStream>> _streams;
};

// Will hold static data for the image input node
struct StreamInputLazy {
public:
    StreamInputLazy(const vortex::Graphics& gfx);

public:
    wis::Sampler _sampler; // Sampler for the texture
    wis::RootSignature _root_signature; // Root signature for the image input node
    wis::PipelineState _pipeline_state; // Pipeline state for rendering the image
    StreamManager _manager; // Stream manager for handling streams
};

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class StreamInput : public vortex::graph::NodeImpl<StreamInput, StreamInputProperties, 0, 1>
{
private:
    static void UnregisterStream(StreamManager::StreamHandle handle) noexcept
    {
        lazy_ptr<StreamInputLazy>::uget()._manager.UnregisterStream(handle);
    }
    using unique_stream = vortex::unique_any<StreamManager::StreamHandle, UnregisterStream>;
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
        // Validate that the texture was loaded successfully
        if (!_texture || stream_size.x == 0 || stream_size.y == 0) {
            return vortex::graph::NodeExecution::Skip; // Skip rendering if texture is not valid
        }

        return vortex::graph::NodeExecution::Render; // Proceed with rendering
    }

public:
    void SetStreamUrl(std::string_view value, bool notify = false)
    {
        StreamInputProperties::SetStreamUrl(value, notify);
        url_changed = true;
    }

private:
    void InitializeStream();

private:
    [[no_unique_address]] lazy_ptr<StreamInputLazy> _lazy_data; // Lazy data for static resources
    vortex::Texture2D _texture; // Texture loaded from the image file
    wis::ShaderResource _texture_resource; // Shader resource for the texture

    // Stream related data
    codec::StreamChannels _stream_collection; // Collection of streams
    unique_stream _stream_handle; // Handle to the stream managed by StreamManager

    bool url_changed = true; // Flag to check if the node has been initialized
};
} // namespace vortex