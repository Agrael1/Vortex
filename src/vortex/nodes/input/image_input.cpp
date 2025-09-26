#include <vortex/nodes/input/image_input.h>
#include <vortex/gfx/descriptor_buffer.h>

#include <vortex/graphics.h>

vortex::ImageInputLazy::ImageInputLazy(const vortex::Graphics& gfx)
{
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
    auto vertex_shader = gfx.LoadShader("shaders/basic.vs");
    auto pixel_shader = gfx.LoadShader("shaders/image.ps");

    // Create a pipeline state for the image input node
    wis::GraphicsPipelineDesc pipeline_desc{
        .root_signature = _root_signature,
        .shaders = {
                .vertex = vertex_shader,
                .pixel = pixel_shader,
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
    wis::SamplerDesc sampler_desc{
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
    };

    _sampler = gfx.GetDevice().CreateSampler(result, sampler_desc);
}

void vortex::ImageInput::Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe)
{
    // Load the texture from the image path if it has changed
    if (path_changed && !image_path.empty()) {
        auto result = codec::CodecFFmpeg::LoadTexture(gfx, image_path);
        if (!result) {
            vortex::error("ImageInput: Failed to load texture from path: {}. Error: {}", image_path, result.error().message());
            image_path = ""; // Clear the path if loading failed
            path_changed = false; // Reset the path changed flag
            return; // Skip rendering if texture loading failed
        }

        _texture = std::move(result.value()); // Store the loaded texture
        _texture_resource = _texture.CreateShaderResource(gfx);

        // Bind the texture and sampler to the command list
        probe.descriptor_buffer.GetCurrentDescriptorBuffer().WriteTexture(0, 0, _texture_resource);
        probe.descriptor_buffer.GetSamplerBuffer().WriteSampler(0, 0, _lazy_data.uget()._sampler);

        auto& cmd_list = *probe.command_list;
        // Update state to shader resource
        std::ignore = cmd_list.Reset();
        cmd_list.TextureBarrier({
                                        .sync_before = wis::BarrierSync::None,
                                        .sync_after = wis::BarrierSync::None,
                                        .access_before = wis::ResourceAccess::NoAccess,
                                        .access_after = wis::ResourceAccess::NoAccess,
                                        .state_before = wis::TextureState::Undefined,
                                        .state_after = wis::TextureState::ShaderResource,
                                },
                                _texture.Get());

        cmd_list.Close();
        wis::CommandListView views[]{ cmd_list };
        gfx.GetMainQueue().ExecuteCommandLists(views, 1);
        gfx.WaitForGPU(); // Ensure the texture is ready for rendering

        path_changed = false; // Reset the path changed flag after loading
    }
}

bool vortex::ImageInput::Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info)
{
    // Check if the texture is valid before rendering
    if (!_texture || _texture.GetSize().width == 0 || _texture.GetSize().height == 0) {
        //vortex::info("ImageInput: Texture is not valid or has zero size.");
        return false; // Skip rendering if texture is not valid
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

    auto& cmd_list = *probe.command_list;

    // Begin the render pass
    cmd_list.BeginRenderPass(pass_desc);
    cmd_list.SetPipelineState(_lazy_data.uget()._pipeline_state);
    cmd_list.SetRootSignature(_lazy_data.uget()._root_signature);
    cmd_list.RSSetScissor({ 0, 0, int(output_info->_output_size.width), int(output_info->_output_size.height) });
    cmd_list.RSSetViewport({ 0.f, 0.f, float(output_info->_output_size.width), float(output_info->_output_size.height), 0.f, 1.f });
    cmd_list.IASetPrimitiveTopology(wis::PrimitiveTopology::TriangleList);
    std::array<DescriptorTableOffset, 2> offsets = {
        DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = false }, // Texture
        DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = true } // Sampler
    };
    probe.descriptor_buffer.BindOffsets(gfx, cmd_list, _lazy_data.uget()._root_signature, probe.frame_number % vortex::max_frames_in_flight, offsets);
    // Draw a quad that covers the viewport
    cmd_list.DrawInstanced(3, 1, 0, 0);

    // End the render pass
    cmd_list.EndRenderPass();
    return true;
}
