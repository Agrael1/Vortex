#include <vortex/nodes/filter/blend.h>
#include <vortex/graphics.h>
#include <vortex/probe.h>

static constexpr wis::BlendAttachmentDesc blend_descs[]{
    // Normal - Standard alpha blending
    { .blend_enable = true,
     .src_color_blend = wis::BlendFactor::SrcAlpha,
     .dst_color_blend = wis::BlendFactor::InvSrcAlpha,
     .color_blend_op = wis::BlendOp::Add,
     .src_alpha_blend = wis::BlendFactor::SrcAlpha,
     .dst_alpha_blend = wis::BlendFactor::InvSrcAlpha,
     .alpha_blend_op = wis::BlendOp::Add,
     .color_write_mask = wis::ColorComponents::All },
    // Multiply - Multiply colors together
    { .blend_enable = true,
     .src_color_blend = wis::BlendFactor::DestColor,
     .dst_color_blend = wis::BlendFactor::Zero,
     .color_blend_op = wis::BlendOp::Add,
     .src_alpha_blend = wis::BlendFactor::Zero,
     .dst_alpha_blend = wis::BlendFactor::One,
     .alpha_blend_op = wis::BlendOp::Add,
     .color_write_mask = wis::ColorComponents::All },
    // Screen - Screen blend mode
    { .blend_enable = true,
     .src_color_blend = wis::BlendFactor::InvDestColor,
     .dst_color_blend = wis::BlendFactor::One,
     .color_blend_op = wis::BlendOp::Add,
     .src_alpha_blend = wis::BlendFactor::Zero,
     .dst_alpha_blend = wis::BlendFactor::One,
     .alpha_blend_op = wis::BlendOp::Add,
     .color_write_mask = wis::ColorComponents::All },
    // Add - Additive blending
    { .blend_enable = true,
     .src_color_blend = wis::BlendFactor::One,
     .dst_color_blend = wis::BlendFactor::One,
     .color_blend_op = wis::BlendOp::Add,
     .src_alpha_blend = wis::BlendFactor::Zero,
     .dst_alpha_blend = wis::BlendFactor::One,
     .alpha_blend_op = wis::BlendOp::Add,
     .color_write_mask = wis::ColorComponents::All },
    // Subtract - Subtractive blending (dst - src)
    { .blend_enable = true,
     .src_color_blend = wis::BlendFactor::One,
     .dst_color_blend = wis::BlendFactor::One,
     .color_blend_op = wis::BlendOp::RevSubtract,
     .src_alpha_blend = wis::BlendFactor::Zero,
     .dst_alpha_blend = wis::BlendFactor::One,
     .alpha_blend_op = wis::BlendOp::Add,
     .color_write_mask = wis::ColorComponents::All },
    // Darken - Select darker color using Min operation
    { .blend_enable = true,
     .src_color_blend = wis::BlendFactor::One,
     .dst_color_blend = wis::BlendFactor::One,
     .color_blend_op = wis::BlendOp::Min,
     .src_alpha_blend = wis::BlendFactor::Zero,
     .dst_alpha_blend = wis::BlendFactor::One,
     .alpha_blend_op = wis::BlendOp::Add,
     .color_write_mask = wis::ColorComponents::All },
    // Lighten - Select lighter color using Max operation
    { .blend_enable = true,
     .src_color_blend = wis::BlendFactor::One,
     .dst_color_blend = wis::BlendFactor::One,
     .color_blend_op = wis::BlendOp::Max,
     .src_alpha_blend = wis::BlendFactor::Zero,
     .dst_alpha_blend = wis::BlendFactor::One,
     .alpha_blend_op = wis::BlendOp::Add,
     .color_write_mask = wis::ColorComponents::All }
};
constexpr size_t blend_desc_count = std::size(blend_descs);
static_assert(blend_desc_count == vortex::BlendLazy::hw_blend_mode_count,
              "Blend desc count mismatch");

vortex::BlendLazy::BlendLazy(const vortex::Graphics& gfx)
{
    auto& device = gfx.GetDevice();
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
    _root_signature = gfx.GetDescriptorBufferExtension().CreateRootSignature(result,
                                                                             nullptr,
                                                                             0,
                                                                             nullptr,
                                                                             0,
                                                                             tables,
                                                                             std::size(tables));

    if (!vortex::success(result)) {
        vortex::error("Blend: Failed to create root signature: {}", result.error);
        return;
    }

    // Load shaders for the blend node
    auto vertex_shader = gfx.LoadShader("shaders/basic.vs");
    auto pixel_shader = gfx.LoadShader("shaders/basic.ps");

    // Create a pipeline state for the blend node
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
        .flags = wis::PipelineFlags::DescriptorBuffer,
    };

    for (size_t i = 0; i < blend_desc_count; i++) {
        blend_desc.attachments[0] = blend_descs[i];
        _pipeline_states[i] = device.CreateGraphicsPipeline(result, pipeline_desc);
        if (!vortex::success(result)) {
            vortex::error("Blend: Failed to create pipeline state: {}", result.error);
            return;
        }
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
        vortex::error("Blend: Failed to create sampler: {}", result.error);
    }
}

vortex::Blend::Blend(const vortex::Graphics& gfx, SerializedProperties props)
    : ImplClass(props)
    , _lazy_data(gfx)
{
}

bool vortex::Blend::Evaluate(const vortex::Graphics& gfx,
                             RenderProbe& probe,
                             const RenderPassForwardDesc* output_info)
{
    // First input is the base image, send it through
    auto& input_base = GetSinks()[0];
    bool source_valid = false;

    if (input_base) {
        input_base.source_node->Evaluate(gfx, probe, output_info);
        // Block the texture used by the base input to avoid overwriting
        probe.texture_pool.BlockTexture(output_info->rt_index);
        source_valid = true;
    } // If there is no image, we will just render nothing (black)

    // Second input is the overlay image to blend
    auto& input_overlay = GetSinks()[1];
    if (!input_overlay) {
        return source_valid; // No overlay image, nothing to blend
    }

    auto view = probe.texture_pool.AcquireTexture(gfx,
                                                  output_info->depth,
                                                  output_info->rt_generation);

    auto rt = view.GetRTV();
    auto sr = view.GetSRV();
    auto tex = view.GetTexture();

    auto& cmd = *probe.command_list;
    RenderPassForwardDesc info{
        .current_rt_view = rt,
        .output_size = output_info->output_size,
        .rt_index = view.GetIndex(),
        .rt_generation = output_info->depth,
        .depth = output_info->depth + 1,
    };

    input_overlay.source_node->Evaluate(gfx, probe, &info);

    auto root = _lazy_data.uget().GetRootSignature();
    auto pipeline = _lazy_data.uget().GetPipelineState(GetBlendMode());
    auto desc_table = probe.descriptor_buffer.SuballocateTable(1);
    auto samp_table = probe.sampler_buffer.SuballocateTable(1);

    wis::TextureBarrier before{
        .sync_before = wis::BarrierSync::RenderTarget,
        .sync_after = wis::BarrierSync::PixelShading,
        .access_before = wis::ResourceAccess::RenderTarget,
        .access_after = wis::ResourceAccess::ShaderResource,
        .state_before = wis::TextureState::RenderTarget,
        .state_after = wis::TextureState::ShaderResource,
    };
    cmd.TextureBarrier(before, tex);

    // Now blend the two images together
    wis::RenderPassRenderTargetDesc target_desc{
        .target = output_info->current_rt_view,
        .load_op = wis::LoadOperation::Load,
        .store_op = wis::StoreOperation::Store,
    };
    wis::RenderPassDesc pass_desc{
        .target_count = 1,
        .targets = &target_desc,
    };
    cmd.BeginRenderPass(pass_desc);
    cmd.SetPipelineState(pipeline);
    cmd.SetRootSignature(root);
    desc_table.WriteTexture(0, sr);
    desc_table.BindOffset(gfx, cmd, _lazy_data.uget().GetRootSignature(), 0);
    samp_table.WriteSampler(0, _lazy_data.uget().GetSampler());
    samp_table.BindOffset(gfx, cmd, _lazy_data.uget().GetRootSignature(), 1);
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

    wis::TextureBarrier after{
        .sync_before = wis::BarrierSync::Draw,
        .sync_after = wis::BarrierSync::RenderTarget,
        .access_before = wis::ResourceAccess::ShaderResource,
        .access_after = wis::ResourceAccess::RenderTarget,
        .state_before = wis::TextureState::ShaderResource,
        .state_after = wis::TextureState::RenderTarget,
    };
    cmd.TextureBarrier(after, tex);

    return true;
}