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

    static AVPixelFormat GetHWFormat(AVCodecContext* ctx, const AVPixelFormat* pix_fmts);

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