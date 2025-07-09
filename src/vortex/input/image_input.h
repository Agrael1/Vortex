#pragma once
#include <vortex/node.h>
#include <vortex/probe.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/util/reflection.h>
#include <DirectXMath.h>

namespace vortex {

struct ImageInputProperties {
    std::string image_path;
    DirectX::XMFLOAT4A crop_rect = { 0.f, 0.f, 0.f, 0.f }; // Normalized crop rectangle
    DirectX::XMINT2 size = { 100, 100 }; // Size of the image in pixels
    DirectX::XMINT2 origin = { 0, 0 }; // Offset in pixels
    DirectX::XMFLOAT2 rotation = { 0.f, 0.f }; // Rotation in radians

    //-- Getters for the properties --//

    // Get image_path as a string_view
    template<typename Self>
    std::string_view GetImagePath(const Self&& self)
    {
        return self.image_path;
    }
    // Get crop_rect as a DirectX::XMFLOAT4A
    template<typename Self>
    DirectX::XMFLOAT4A GetCropRect(const Self&& self)
    {
        return self.crop_rect;
    }
    // Get size as a DirectX::XMUINT2
    template<typename Self>
    DirectX::XMINT2 GetSize(const Self&& self)
    {
        return self.size;
    }
    // Get origin as a DirectX::XMINT2
    template<typename Self>
    DirectX::XMINT2 GetOrigin(const Self&& self)
    {
        return self.origin;
    }
    // Get rotation as a DirectX::XMFLOAT2
    template<typename Self>
    DirectX::XMFLOAT2 GetRotation(const Self&& self)
    {
        return self.rotation;
    }

    //-- Setters for the properties --//

    // Set image_path from a string
    template<typename Self>
    void SetImagePath(Self& self, std::string_view path, bool notify = true)
    {
        self.image_path = path;
    }

    // Set crop_rect from a DirectX::XMFLOAT4A
    template<typename Self>
    void SetCropRect(Self& self, DirectX::XMFLOAT4A rect, bool notify = true)
    {
        self.crop_rect = rect;
    }

    // Set size from a DirectX::XMUINT2
    template<typename Self>
    void SetSize(Self& self, DirectX::XMINT2 size, bool notify = true)
    {
        self.size = size;
    }
    // Set origin from a DirectX::XMINT2
    template<typename Self>
    void SetOrigin(Self& self, DirectX::XMINT2 origin, bool notify = true)
    {
        self.origin = origin;
    }
    // Set rotation from a DirectX::XMFLOAT2
    template<typename Self>
    void SetRotation(Self& self, DirectX::XMFLOAT2 rotation, bool notify = true)
    {
        self.rotation = rotation;
    }

    // Comes from javascript
    template<typename Self>
    void SetProperty(Self& self, uint32_t index, std::string_view value, bool notify = false)
    {
        switch (index) {
        case 0:
            if (std::string_view path; vortex::reflection_traits<std::string_view>::deserialize(&path, value)) {
                self.SetImagePath<Self>(path, notify);
            }
            break; // image_path
        case 1: 
            if (DirectX::XMFLOAT4A rect; vortex::reflection_traits<DirectX::XMFLOAT4A>::deserialize(&rect, value)) {
                self.SetCropRect<Self>(rect, notify);
            }
            break; // crop_rect
        case 2:
            if (DirectX::XMINT2 size; vortex::reflection_traits<DirectX::XMINT2>::deserialize(&size, value)) {
                self.SetSize<Self>(size, notify);
            }
            break; // size
        case 3:
            if (DirectX::XMINT2 origin; vortex::reflection_traits<DirectX::XMINT2>::deserialize(&origin, value)) {
                self.SetOrigin<Self>(origin, notify);
            }
            break; // origin
        case 4:
            if (DirectX::XMFLOAT2 rotation; vortex::reflection_traits<DirectX::XMFLOAT2>::deserialize(&rotation, value)) {
                self.SetRotation<Self>(rotation, notify);
            }
            break; // rotation
        default:
            vortex::error("ImageInputProperties: Invalid property index: {}", index);
            break;
        }
    }

    template<typename Self>
    void NotifyPropertyChange(Self& self, uint32_t index)
    {
        switch (index) {
        case 0: // image_path
            notifier(0, vortex::reflection_traits<std::string_view>::serialize(self.image_path));
            break;
        case 1: // crop_rect
            notifier(1, vortex::reflection_traits<DirectX::XMFLOAT4A>::serialize(self.crop_rect));
            break;
        case 2: // size
            notifier(2, vortex::reflection_traits<DirectX::XMINT2>::serialize(self.size));
            break;
        case 3: // origin
            notifier(3, vortex::reflection_traits<DirectX::XMINT2>::serialize(self.origin));
            break;
        case 4: // rotation
            notifier(4, vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.rotation));
            break;
        default:
            vortex::error("ImageInputProperties: Invalid property index for notification: {}", index);
            break;
        }
    }

    notifier_callback notifier; // Callback for property change notifications
};

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class ImageInput : public NodeImplWithP<ImageInput, ImageInputProperties>
{
public:
    using Parameters = ImageInputProperties;
    using ImageParams = NodeDescT<Parameters>;

public:
    ImageInput() = default;
    ImageInput(const vortex::Graphics& gfx, const ImageParams& params)
        : _texture(codec::CodecFFmpeg::LoadTexture(gfx, params.data.image_path))
    {
        if (!_texture) {
            vortex::warn("ImageInput: Failed to load texture from path: {}", params.data.image_path);
        }
        // Create a root signature for the image input node
        wis::Result result = wis::success;

        wis::DescriptorTableEntry entries[] = {
            { .type = wis::DescriptorType::Texture, .bind_register = 0, .binding = 0, .count = 1 },
            { .type = wis::DescriptorType::Sampler, .bind_register = 0, .binding = 1, .count = 1 },
        };
        wis::DescriptorTable tables[] = {
            { .type = wis::DescriptorHeapType::Descriptor, .entries = entries, .entry_count = 1, .stage = wis::ShaderStages::Pixel },
            { .type = wis::DescriptorHeapType::Sampler, .entries = entries + 1, .entry_count = 1, .stage = wis::ShaderStages::Pixel },
        };
        _root_signature = gfx.GetDescriptorBufferExtension().CreateRootSignature(result, nullptr, 0, nullptr, 0, tables, std::size(tables));
        if (!vortex::success(result)) {
            vortex::error("ImageInput: Failed to create root signature: {}", result.error);
            return;
        }

        // Load shaders for the image input node
        _vertex_shader = gfx.LoadShader("shaders/basic.vs");
        _pixel_shader = gfx.LoadShader("shaders/image.ps");

        // Create a pipeline state for the image input node
        wis::GraphicsPipelineDesc pipeline_desc{
            .root_signature = _root_signature,
            .shaders = {
                    .vertex = _vertex_shader,
                    .pixel = _pixel_shader,
            },
            .attachments = {
                    .attachment_formats = { wis::DataFormat::RGBA8Unorm }, .attachments_count = 1,
                    .depth_attachment = wis::DataFormat::Unknown, // No depth attachment
            },
            .flags = wis::PipelineFlags::DescriptorBuffer,
        };

        _pipeline_state = gfx.GetDevice().CreateGraphicsPipeline(result, pipeline_desc);
        if (!vortex::success(result)) {
            vortex::error("ImageInput: Failed to create graphics pipeline: {}", result.error);
            return;
        }
    }
    ImageInput(const vortex::Graphics& gfx, vortex::NodeDesc* initializers)
        : ImageInput(gfx, *static_cast<ImageParams*>(initializers))
    {
    }

    NodeExecution Validate(const vortex::Graphics& gfx, const vortex::RenderProbe& probe)
    {
        // Validate that the texture was loaded successfully
        if (!_texture || size.x == 0 || size.y == 0) {
            return NodeExecution::Skip; // Skip rendering if texture is not valid
        }

        return NodeExecution::Render; // Proceed with rendering
    }

    wis::Result Render(const vortex::Graphics& gfx, const vortex::RenderProbe& probe)
    {
        auto& cmd_list = probe._command_list;

        auto [vwidth, vheight] = probe._output_size;

        wis::RenderPassRenderTargetDesc rprtd{
            .target = probe._current_rt_view,
            .load_op = wis::LoadOperation::Clear,
            .store_op = wis::StoreOperation::Store,
            .clear_value = { 0.f, 0.f, 0.f, 0.f }, // Clear to transparent black
        };
        wis::RenderPassDesc rpdesc{
            .target_count = 1,
            .targets = &rprtd,
        };
        cmd_list.BeginRenderPass(rpdesc);
        cmd_list.SetPipelineState(_pipeline_state);
        cmd_list.SetRootSignature(_root_signature);
        cmd_list.RSSetScissor({ 0, 0, int(vwidth), int(vheight) });
        cmd_list.RSSetViewport({ 0.f, 0.f, float(vwidth), float(vheight), 0.f, 1.f });
        cmd_list.IASetPrimitiveTopology(wis::PrimitiveTopology::TriangleList);

        std::array<DescriptorTableOffset, 2> offsets = {
            DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = false }, // Texture
            DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = true } // Sampler
        };

        probe._descriptor_buffer.BindOffsets(gfx, cmd_list, _root_signature, probe.frame, offsets);

        // Draw a quad that covers the viewport
        cmd_list.DrawInstanced(3, 1, 0, 0);
        cmd_list.EndRenderPass();

        return wis::success;
    }

private:
    vortex::Texture2D _texture; // Texture loaded from the image file
    wis::RootSignature _root_signature; // Root signature for the image input node
    wis::PipelineState _pipeline_state; // Pipeline state for rendering the image
    wis::Shader _vertex_shader; // Vertex shader for the image input node
    wis::Shader _pixel_shader; // Pixel shader for the image input node
};

using ImageParams = ImageInput::ImageParams;

} // namespace vortex