#pragma once
#include <vortex/graph/interfaces.h>
#include <vortex/probe.h>
#include <vortex/gfx/descriptor_buffer.h>
#include <vortex/codec/codec_ffmpeg.h>
#include <vortex/util/reflection.h>
#include <DirectXMath.h>

#include <vortex/properties/props.hpp>

namespace vortex {

// Rendering a texture from an image input node onto a 2D plane in the scene graph.
class ImageInput : public vortex::graph::NodeImpl<ImageInput, ImageInputProperties, 0, 1>
{
public:
    ImageInput() = default;
    ImageInput(const vortex::Graphics& gfx, SerializedProperties props)
        : ImplClass(props)
    {
        // Create a root signature for the image input node
        wis::Result result = wis::success;

        wis::DescriptorTableEntry entries[] = {
            { .type = wis::DescriptorType::Texture, .bind_register = 0, .binding = 0, .count = 1 },
            { .type = wis::DescriptorType::Sampler, .bind_register = 0, .binding = 0, .count = 1 },
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

        if (!image_path.empty()) {
            // Load the texture from the image path
            path_changed = true; // Mark that the path has changed
        }
    }

public:
    void Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe) override
    {
        // Load the texture from the image path if it has changed
        if (path_changed && !image_path.empty()) {
            _texture = codec::CodecFFmpeg::LoadTexture(gfx, image_path);
            _texture_resource = _texture.CreateShaderResource(gfx);

            // Bind the texture and sampler to the command list
            probe._descriptor_buffer.GetCurrentDescriptorBuffer().WriteTexture(0, 0, _texture_resource);
            probe._descriptor_buffer.GetSamplerBuffer().WriteSampler(0, 0, _sampler);

            auto& cmd_list = *probe._command_list;
            // Update state to shader resource
            std::ignore = cmd_list.Reset();
            cmd_list.TextureBarrier({
                                            .sync_before = wis::BarrierSync::None,
                                            .sync_after = wis::BarrierSync::None,
                                            .access_before = wis::ResourceAccess::NoAccess,
                                            .access_after = wis::ResourceAccess::NoAccess,
                                            .state_before = wis::TextureState::Undefined,
                                            .state_after = wis::TextureState::ShaderResource,
                }, _texture.Get());

            cmd_list.Close();
            wis::CommandListView views[]{ cmd_list };
            gfx.GetMainQueue().ExecuteCommandLists(views, 1);
            gfx.WaitForGPU(); // Ensure the texture is ready for rendering

            path_changed = false; // Reset the path changed flag after loading
        }
    }
    void Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info = nullptr) override
    {
        // Check if the texture is valid before rendering
        if (!_texture || _texture.GetSize().width == 0 || _texture.GetSize().height == 0) {
            vortex::info("ImageInput: Texture is not valid or has zero size.");
            return; // Skip rendering if texture is not valid
        }

        wis::RenderPassRenderTargetDesc target_desc{
            .target = output_info->_current_rt_view,
            .load_op = wis::LoadOperation::Clear,
            .store_op = wis::StoreOperation::Store,
            .clear_value = { 0.f, 0.f, 0.f, 1.f } // Clear to transparent black
        };
        wis::RenderPassDesc pass_desc{
            .target_count = 1,
            .targets = &target_desc,
        };

        auto& cmd_list = *probe._command_list;

        // Begin the render pass
        cmd_list.BeginRenderPass(pass_desc);
        cmd_list.SetPipelineState(_pipeline_state);
        cmd_list.SetRootSignature(_root_signature);
        cmd_list.RSSetScissor({ 0, 0, int(output_info->_output_size.width), int(output_info->_output_size.height) });
        cmd_list.RSSetViewport({ 0.f, 0.f, float(output_info->_output_size.width), float(output_info->_output_size.height), 0.f, 1.f });
        cmd_list.IASetPrimitiveTopology(wis::PrimitiveTopology::TriangleList);
        std::array<DescriptorTableOffset, 2> offsets = {
            DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = false }, // Texture
            DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = true } // Sampler
        };
        probe._descriptor_buffer.BindOffsets(gfx, cmd_list, _root_signature, probe.frame, offsets);
        // Draw a quad that covers the viewport
        cmd_list.DrawInstanced(3, 1, 0, 0);

        // End the render pass
        cmd_list.EndRenderPass();
    }

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