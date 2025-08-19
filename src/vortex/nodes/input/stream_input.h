#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/probe.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/util/reflection.h>
#include <DirectXMath.h>

#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>

namespace vortex {
// Will hold static data for the image input node
struct StreamInputLazy : public Lazy<StreamInputLazy> {
    struct Data {
        wis::Sampler _sampler; // Sampler for the texture
        wis::RootSignature _root_signature; // Root signature for the image input node
        wis::PipelineState _pipeline_state; // Pipeline state for rendering the image
    };
    static StreamInputLazy& Create(const vortex::Graphics& gfx)
    {
        static StreamInputLazy instance(gfx);
        return instance;
    }
    void Destroy() noexcept
    {
        _data.reset(); // Reset the data to release resources
    }

private:
    StreamInputLazy(const vortex::Graphics& gfx);

public:
    std::optional<Data> _data; // Optional data for the image input node
};

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class StreamInput : public vortex::graph::NodeImpl<StreamInput, StreamInputProperties, 0, 1>
{

public:
    StreamInput(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props)
    {
        if (!_lazy_data) {
            _lazy_data = &StreamInputLazy::Create(gfx); // Create the lazy data for static resources
        }

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
        url_changed = true; // Mark that the path has changed
    }

private:
    static inline const StreamInputLazy* _lazy_data = nullptr; // Lazy data for static resources
    vortex::Texture2D _texture; // Texture loaded from the image file
    wis::ShaderResource _texture_resource; // Shader resource for the texture
    bool url_changed = true; // Flag to check if the node has been initialized
};
} // namespace vortex