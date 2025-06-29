#pragma once
#include <vortex/node.h>
#include <vortex/probe.h>
#include <vortex/codec/codec_ffmpeg.h>
#include <DirectXMath.h>

namespace vortex {

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class ImageInput : public NodeImpl<ImageInput>
{
public:
    struct Parameters {
        std::string image_path;
        DirectX::XMFLOAT4A crop_rect = { 0.f, 0.f, 0.f, 0.f }; // Normalized crop rectangle
        DirectX::XMUINT2 size = { 100, 100 }; // Size of the image in pixels
        DirectX::XMINT2 origin = { 0, 0 }; // Offset in pixels
        DirectX::XMFLOAT2 rotation = { 0.f, 0.f }; // Rotation in radians
    };
    using ImageParams = NodeDescT<ImageInput::Parameters>;

public:
    ImageInput() = default;
    ImageInput(const vortex::Graphics& gfx, const ImageParams& params)
        : _texture(codec::CodecFFmpeg::LoadTexture(gfx, params.data.image_path))
        , _params(params.data)
    {
        if (!_texture) {
            vortex::warn("ImageInput: Failed to load texture from path: {}", params.data.image_path);
        }
    }
    ImageInput(const vortex::Graphics& gfx, vortex::NodeDesc* initializers)
        : ImageInput(gfx, *static_cast<ImageParams*>(initializers))
    {
    }

    NodeExecution Validate(const vortex::Graphics& gfx, const vortex::RenderProbe& probe)
    {
        // Validate that the texture was loaded successfully
        if (!_texture || _params.size.x == 0 || _params.size.y == 0) {
            return NodeExecution::Skip; // Skip rendering if texture is not valid
        }

        return NodeExecution::Render; // Proceed with rendering
    }

    wis::Result Render(const vortex::Graphics& gfx, const vortex::RenderProbe& probe)
    {
        auto& cmd_list = probe._command_list;

        auto [vwidth, vheight] = probe._output_size;

        // Draw a quad that covers the viewport
        cmd_list.DrawInstanced(4, 1, 0, 0);

        return wis::success;
    }

private:
    vortex::Texture2D _texture; // Texture loaded from the image file
    Parameters _params; // Parameters for the image input node
};

using ImageParams = ImageInput::ImageParams;

} // namespace vortex