#include <vortex/nodes/filter/transform.h>
#include <vortex/graphics.h>
#include <vortex/probe.h>
#include <numbers>

// Push constant structures matching the shaders
struct TransformConstants {
    DirectX::XMFLOAT2 translation; // Translation in pixels (normalized)
    DirectX::XMFLOAT2 scale; // Scale factors
    DirectX::XMFLOAT2 pivot; // Pivot point (normalized 0-1)
    float rotation; // Rotation angle in radians
    float aspect_ratio; // Width / Height of the output
};

struct CropConstants {
    DirectX::XMFLOAT4 crop_rect; // x, y, width, height (normalized 0-1)
};

vortex::TransformLazy::TransformLazy(const vortex::Graphics& gfx)
{
    auto& device = gfx.GetDevice();
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    wis::Result result = wis::success;

    wis::DescriptorTableEntry entries_desc[] = {
        { .type = wis::DescriptorType::Texture, .bind_register = 0, .binding = 0, .count = 1 }
    };
    wis::DescriptorTableEntry entries_samp[] = {
        { .type = wis::DescriptorType::Sampler, .bind_register = 0, .binding = 0, .count = 1 },
    };
    wis::DescriptorTable tables[] = {
        { .type = wis::DescriptorHeapType::Descriptor,
         .entries = entries_desc,
         .entry_count = 1,
         .stage = wis::ShaderStages::Pixel },
        {    .type = wis::DescriptorHeapType::Sampler,
         .entries = entries_samp,
         .entry_count = 1,
         .stage = wis::ShaderStages::Pixel },
    };

    // Define push constants for vertex and pixel shaders
    wis::PushConstant push_constants[] = {
        { .stage = wis::ShaderStages::Vertex,
         .size_bytes = sizeof(TransformConstants),
         .bind_register = 0 },
        {  .stage = wis::ShaderStages::Pixel,
         .size_bytes = sizeof(CropConstants),
         .bind_register = 0 }
    };

    _root_signature = desc_ext.CreateRootSignature(result,
                                                   push_constants,
                                                   std::size(push_constants),
                                                   nullptr,
                                                   0,
                                                   tables,
                                                   std::size(tables));

    if (!vortex::success(result)) {
        vortex::error("Transform: Failed to create root signature: {}", result.error);
        return;
    }

    // Load shaders for the transform node
    auto vertex_shader = gfx.LoadShader("shaders/transform.vs");
    auto pixel_shader = gfx.LoadShader("shaders/transform.ps");

    // Create a pipeline state for the transform node
    wis::BlendStateDesc blend_desc{
        .attachment_count = 1,
    };
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
        .blend = &blend_desc,
    };
    _pipeline_state = device.CreateGraphicsPipeline(result, pipeline_desc);
    if (!vortex::success(result)) {
        vortex::error("Transform: Failed to create pipeline state: {}", result.error);
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

    _sampler = device.CreateSampler(result, sampler_desc);
    if (!vortex::success(result)) {
        vortex::error("Transform: Failed to create sampler: {}", result.error);
    }
}

bool vortex::Transform::Evaluate(const vortex::Graphics& gfx,
                                 RenderProbe& probe,
                                 const RenderPassForwardDesc* output_info)
{
    // First input is the base image
    auto& input_base = GetSinks()[0];
    if (!input_base) {
        return false;
    }

    auto view = probe.texture_pool.AcquireTexture(gfx);
    wis::RenderTargetDesc rtd{
        .format = wis::DataFormat::RGBA8Unorm,
    };

    auto rt = view.GetRTV();
    auto sr = view.GetSRV();
    auto tex = view.GetTexture();

    auto& cmd = *probe.command_list;
    wis::TextureBarrier before{
        .sync_before = wis::BarrierSync::Draw,
        .sync_after = wis::BarrierSync::RenderTarget,
        .access_before = wis::ResourceAccess::ShaderResource,
        .access_after = wis::ResourceAccess::RenderTarget,
        .state_before = wis::TextureState::ShaderResource,
        .state_after = wis::TextureState::RenderTarget,
    };
    cmd.TextureBarrier(before, tex);
    RenderPassForwardDesc info{
        .current_rt_view = rt,
        .output_size = output_info->output_size,
    };

    bool eval = input_base.source_node->Evaluate(gfx, probe, &info);

    wis::TextureBarrier after{
        .sync_before = wis::BarrierSync::RenderTarget,
        .sync_after = wis::BarrierSync::PixelShading,
        .access_before = wis::ResourceAccess::RenderTarget,
        .access_after = wis::ResourceAccess::ShaderResource,
        .state_before = wis::TextureState::RenderTarget,
        .state_after = wis::TextureState::ShaderResource,
    };
    cmd.TextureBarrier(after, tex);

    // If the input evaluation failed, skip rendering
    if (!eval) {
        return false;
    }

    auto root = _lazy_data.uget().GetRootSignature();
    auto pipeline = _lazy_data.uget().GetPipelineState();
    auto desc_table = probe.descriptor_buffer.SuballocateTable(1);
    auto samp_table = probe.sampler_buffer.SuballocateTable(1);

    // Prepare transform constants
    TransformConstants transform_constants{};

    // Convert translation from pixels to normalized coordinates
    float width = static_cast<float>(output_info->output_size.width);
    float height = static_cast<float>(output_info->output_size.height);
    auto translation = GetTranslation();
    transform_constants.translation = DirectX::XMFLOAT2{ translation.x / width,
                                                         translation.y / height };

    transform_constants.scale = GetScale();
    transform_constants.pivot = GetPivot();

    // Convert rotation from degrees to radians
    transform_constants.rotation = GetRotation() * std::numbers::pi_v<float> / 180.0f;
    transform_constants.aspect_ratio = width / height;

    // Prepare crop constants
    CropConstants crop_constants{};
    crop_constants.crop_rect = GetCropRect();

    // Now render with transformation
    wis::RenderPassRenderTargetDesc target_desc{
        .target = output_info->current_rt_view,
        .load_op = wis::LoadOperation::Clear,
        .store_op = wis::StoreOperation::Store,
        .clear_value = { 0.f, 0.f, 0.f, 0.f }
    };
    wis::RenderPassDesc pass_desc{
        .target_count = 1,
        .targets = &target_desc,
    };
    cmd.BeginRenderPass(pass_desc);
    cmd.SetPipelineState(pipeline);
    cmd.SetRootSignature(root);

    // Set push constants for vertex shader (transform)
    cmd.SetPushConstants(&transform_constants,
                         sizeof(TransformConstants) / 4,
                         0,
                         wis::ShaderStages::Vertex);

    // Set push constants for pixel shader (crop)
    cmd.SetPushConstants(&crop_constants, sizeof(CropConstants) / 4, 0, wis::ShaderStages::Pixel);

    desc_table.WriteTexture(0, sr);
    desc_table.BindOffset(gfx, cmd, root, 0);
    samp_table.WriteSampler(0, _lazy_data.uget().GetSampler());
    samp_table.BindOffset(gfx, cmd, root, 1);

    cmd.RSSetScissor(
            { 0, 0, int(output_info->output_size.width), int(output_info->output_size.height) });
    cmd.RSSetViewport({ 0.f,
                        0.f,
                        float(output_info->output_size.width),
                        float(output_info->output_size.height),
                        0.f,
                        1.f });
    cmd.IASetPrimitiveTopology(wis::PrimitiveTopology::TriangleList);
    cmd.DrawInstanced(3);
    cmd.EndRenderPass();
    return true;
}
