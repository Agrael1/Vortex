#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/probe.h>
#include <vortex/codec/ffmpeg/codec_ffmpeg.h>
#include <vortex/util/reflection.h>
#include <DirectXMath.h>

#include <vortex/properties/props.hpp>
#include <vortex/util/lazy.h>

namespace vortex {
// Will hold static data for the image input node
struct ImageInputLazy {
public:
    ImageInputLazy(const vortex::Graphics& gfx);

public:
    wis::Sampler _sampler; // Sampler for the texture
    wis::RootSignature _root_signature; // Root signature for the image input node
    wis::PipelineState _pipeline_state; // Pipeline state for rendering the image
};

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class ImageInput : public vortex::graph::NodeImpl<ImageInput, ImageInputProperties, 0, 1>
{
    
public:
    ImageInput(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props), _lazy_data(gfx)
    {
        // Create a root signature for the image input node
        if (!image_path.empty()) {
            // Load the texture from the image path
            path_changed = true; // Mark that the path has changed
        }
    }

public:
    void Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe) override;
    void Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info = nullptr) override;

    vortex::graph::NodeExecution Validate(const vortex::Graphics& gfx, const vortex::RenderProbe& probe)
    {
        // Validate that the texture was loaded successfully
        if (!_texture || image_size.x == 0 || image_size.y == 0) {
            return vortex::graph::NodeExecution::Skip; // Skip rendering if texture is not valid
        }

        return vortex::graph::NodeExecution::Render; // Proceed with rendering
    }

public:
    void SetImagePath(std::string_view path, bool notify = true)
    {
        if (GetImagePath() == path) {
            return; // No change in path, skip setting
        }
        if (std::filesystem::exists(path)) {
            ImageInputProperties::SetImagePath(path, notify);
        } else {
            vortex::error("ImageInput: Image path does not exist: {}", path);
            ImageInputProperties::SetImagePath("", notify);
        }
        path_changed = true; // Mark that the path has changed
    }

private:
    lazy_ptr<ImageInputLazy> _lazy_data; // Lazy data for static resources
    vortex::Texture2D _texture; // Texture loaded from the image file
    wis::ShaderResource _texture_resource; // Shader resource for the texture
    bool path_changed = false; // Flag to check if the node has been initialized
};
} // namespace vortex