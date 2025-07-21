#pragma once
#include <vortex/graph/interfaces.h>
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
    std::string_view GetImagePath(this Self&& self)
    {
        return self.image_path;
    }
    // Get crop_rect as a DirectX::XMFLOAT4A
    template<typename Self>
    DirectX::XMFLOAT4A GetCropRect(this Self&& self)
    {
        return self.crop_rect;
    }
    // Get size as a DirectX::XMUINT2
    template<typename Self>
    DirectX::XMINT2 GetSize(this Self&& self)
    {
        return self.size;
    }
    // Get origin as a DirectX::XMINT2
    template<typename Self>
    DirectX::XMINT2 GetOrigin(this Self&& self)
    {
        return self.origin;
    }
    // Get rotation as a DirectX::XMFLOAT2
    template<typename Self>
    DirectX::XMFLOAT2 GetRotation(this Self&& self)
    {
        return self.rotation;
    }

    //-- Setters for the properties --//

    // Set image_path from a string
    template<typename Self>
    void SetImagePath(this Self& self, std::string_view path, bool notify = true)
    {
        self.image_path = path;
        if (notify) {
            self.NotifyPropertyChange(0); // Notify that image_path has changed
        }
    }

    // Set crop_rect from a DirectX::XMFLOAT4A
    template<typename Self>
    void SetCropRect(this Self& self, DirectX::XMFLOAT4A rect, bool notify = true)
    {
        self.crop_rect = rect;
        if (notify) {
            self.NotifyPropertyChange(1); // Notify that crop_rect has changed
        }
    }

    // Set size from a DirectX::XMUINT2
    template<typename Self>
    void SetSize(this Self& self, DirectX::XMINT2 size, bool notify = true)
    {
        self.size = size;
        if (notify) {
            self.NotifyPropertyChange(2); // Notify that size has changed
        }
    }
    // Set origin from a DirectX::XMINT2
    template<typename Self>
    void SetOrigin(this Self& self, DirectX::XMINT2 origin, bool notify = true)
    {
        self.origin = origin;
        if (notify) {
            self.NotifyPropertyChange(3); // Notify that origin has changed
        }
    }
    // Set rotation from a DirectX::XMFLOAT2
    template<typename Self>
    void SetRotation(this Self& self, DirectX::XMFLOAT2 rotation, bool notify = true)
    {
        self.rotation = rotation;
        if (notify) {
            self.NotifyPropertyChange(4); // Notify that rotation has changed
        }
    }

    // Comes from javascript
    template<typename Self>
    void SetPropertyStub(this Self& self, uint32_t index, std::string_view value, bool notify = false)
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
    void NotifyPropertyChange(this Self& self, uint32_t index)
    {
        if (!self.notifier) {
            vortex::error("ImageInputProperties: Notifier callback is not set.");
            return; // No notifier set, cannot notify
        }

        switch (index) {
        case 0: // image_path
            self.notifier(0, vortex::reflection_traits<std::string_view>::serialize(self.image_path));
            break;
        case 1: // crop_rect
            self.notifier(1, vortex::reflection_traits<DirectX::XMFLOAT4A>::serialize(self.crop_rect));
            break;
        case 2: // size
            self.notifier(2, vortex::reflection_traits<DirectX::XMINT2>::serialize(self.size));
            break;
        case 3: // origin
            self.notifier(3, vortex::reflection_traits<DirectX::XMINT2>::serialize(self.origin));
            break;
        case 4: // rotation
            self.notifier(4, vortex::reflection_traits<DirectX::XMFLOAT2>::serialize(self.rotation));
            break;
        default:
            vortex::error("ImageInputProperties: Invalid property index for notification: {}", index);
            break;
        }
    }

    notifier_callback notifier; // Callback for property change notifications
};

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class ImageInput : public vortex::graph::NodeImpl<ImageInput, ImageInputProperties, 0, 1>
{
public:
    ImageInput() = default;
    ImageInput(const vortex::Graphics& gfx)
    {
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

        // clang-format off
        _sampler = gfx.GetDevice().CreateSampler(result, wis::SamplerDesc{
                                                                 .min_filter = wis::Filter::Linear, 
                                                                 .mag_filter = wis::Filter::Linear, 
                                                                 .mip_filter = wis::Filter::Linear, 
                                                                 .anisotropic = false, 
                                                                 .max_anisotropy = 1, 
                                                                 .address_u = wis::AddressMode::ClampToBorder, 
                                                                 .address_v = wis::AddressMode::ClampToBorder, 
                                                                 .address_w = wis::AddressMode::ClampToBorder, 
                                                                 .min_lod = 0.f, 
                                                                 .max_lod = 1.f, 
                                                                 .mip_lod_bias = 0.f, 
                                                                 .comparison_op = wis::Compare::None, 
                                                                 .border_color = { 0.f, 0.f, 0.f, 0.f }, // Transparent border color
                                                         });
        // clang-format on
    }

public:
    void Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe) override
    {
        // Load the texture from the image path if it has changed
        if (path_changed && !image_path.empty()) {
            _texture = codec::CodecFFmpeg::LoadTexture(gfx, image_path);
            _texture_resource = _texture.CreateShaderResource(gfx);
            path_changed = false; // Reset the path changed flag after loading
        }
    }
    vortex::graph::NodeExecution Validate(const vortex::Graphics& gfx, const vortex::RenderProbe& probe)
    {
        // Validate that the texture was loaded successfully
        if (!_texture || size.x == 0 || size.y == 0) {
            return vortex::graph::NodeExecution::Skip; // Skip rendering if texture is not valid
        }

        return vortex::graph::NodeExecution::Render; // Proceed with rendering
    }

    // wis::Result Render(const vortex::RenderProbe& probe)
    //{
    //     auto& cmd_list = probe._command_list;

    //    auto [vwidth, vheight] = probe._output_size;

    //    if (!initialized) {
    //        probe._descriptor_buffer.GetCurrentDescriptorBuffer().WriteTexture(0, 0, _texture_resource);
    //        probe._descriptor_buffer.GetSamplerBuffer().WriteSampler(0, 0, _sampler);
    //        initialized = true; // Mark the node as initialized
    //    }

    //    cmd_list.SetPipelineState(_pipeline_state);
    //    cmd_list.SetRootSignature(_root_signature);
    //    cmd_list.RSSetScissor({ 0, 0, int(vwidth), int(vheight) });
    //    cmd_list.RSSetViewport({ 0.f, 0.f, float(vwidth), float(vheight), 0.f, 1.f });
    //    cmd_list.IASetPrimitiveTopology(wis::PrimitiveTopology::TriangleList);

    //    std::array<DescriptorTableOffset, 2> offsets = {
    //        DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = false }, // Texture
    //        DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = true } // Sampler
    //    };

    //    probe._descriptor_buffer.BindOffsets(probe._gfx, cmd_list, _root_signature, probe.frame, offsets);

    //    // Draw a quad that covers the viewport
    //    cmd_list.DrawInstanced(3, 1, 0, 0);
    //    return wis::success;
    //}

public:
    template<typename Self>
    void SetImagePath(this Self& self, std::string_view path, bool notify = true)
    {
        if (self.GetImagePath() == path) {
            return; // No change in path, skip setting
        }
        if (std::filesystem::exists(path)) {
            self.ImageInputProperties::SetImagePath(path, notify);
        } else {
            vortex::error("ImageInput: Image path does not exist: {}", path);
            self.ImageInputProperties::SetImagePath("", notify);
        }
        self.path_changed = true; // Mark that the path has changed
    }

private:
    vortex::Texture2D _texture; // Texture loaded from the image file
    wis::ShaderResource _texture_resource; // Shader resource for the texture
    wis::Sampler _sampler; // Sampler for the texture
    wis::RootSignature _root_signature; // Root signature for the image input node
    wis::PipelineState _pipeline_state; // Pipeline state for rendering the image
    wis::Shader _vertex_shader; // Vertex shader for the image input node
    wis::Shader _pixel_shader; // Pixel shader for the image input node

    bool path_changed = false; // Flag to check if the node has been initialized
};
} // namespace vortex