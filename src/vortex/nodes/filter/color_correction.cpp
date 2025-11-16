#include <vortex/nodes/filter/color_correction.h>
#include <vortex/graphics.h>
#include <vortex/probe.h>

struct ColorCorrectionConstants {
    vortex::LUTInterp interp_mode;
    float brightness;
    float contrast;
    float saturation;
};

vortex::ColorCorrectionLazy::ColorCorrectionLazy(const vortex::Graphics& gfx)
{
    auto& device = gfx.GetDevice();
    auto& desc_ext = gfx.GetDescriptorBufferExtension();
    wis::Result result = wis::success;

    // Descriptor tables: 2 textures (LUT + input), 2 samplers
    wis::DescriptorTableEntry entries_desc[] = {
        { .type = wis::DescriptorType::Texture, .bind_register = 0, .binding = 0, .count = 1 },
        { .type = wis::DescriptorType::Texture, .bind_register = 1, .binding = 1, .count = 1 }
    };
    wis::DescriptorTableEntry entries_samp[] = {
        { .type = wis::DescriptorType::Sampler, .bind_register = 0, .binding = 0, .count = 1 },
        { .type = wis::DescriptorType::Sampler, .bind_register = 1, .binding = 1, .count = 1 }
    };
    wis::DescriptorTable tables[] = {
        { .type = wis::DescriptorHeapType::Descriptor,
         .entries = entries_desc,
         .entry_count = std::size(entries_desc),
         .stage = wis::ShaderStages::Pixel },
        {    .type = wis::DescriptorHeapType::Sampler,
         .entries = entries_samp,
         .entry_count = std::size(entries_samp),
         .stage = wis::ShaderStages::Pixel }
    };
    wis::PushConstant push_constants[] = {
        { .stage = wis::ShaderStages::Pixel,
         .size_bytes = sizeof(ColorCorrectionConstants),
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
        vortex::error("ColorCorrection: Failed to create root signature: {}", result.error);
        return;
    }

    // Load shaders
    auto vertex_shader = gfx.LoadShader("shaders/basic.vs");
    auto pixel_shader = gfx.LoadShader("shaders/color_correction.ps");

    // Create pipeline
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
            .attachment_formats = { wis::DataFormat::RGBA8Unorm },
            .attachments_count = 1,
            .depth_attachment = wis::DataFormat::Unknown,
        },
        .blend = &blend_desc,
    };
    _pipeline_state = device.CreateGraphicsPipeline(result, pipeline_desc);
    if (!vortex::success(result)) {
        vortex::error("ColorCorrection: Failed to create pipeline state: {}", result.error);
        return;
    }

    // Create linear sampler for input texture and 3D LUT
    wis::SamplerDesc sampler_linear_desc{
        .min_filter = wis::Filter::Linear,
        .mag_filter = wis::Filter::Linear,
        .mip_filter = wis::Filter::Linear,
        .anisotropic = false,
        .max_anisotropy = 1,
        .address_u = wis::AddressMode::ClampToEdge,
        .address_v = wis::AddressMode::ClampToEdge,
        .address_w = wis::AddressMode::ClampToEdge,
        .min_lod = 0.f,
        .max_lod = 1.f,
        .mip_lod_bias = 0.f,
        .comparison_op = wis::Compare::None,
    };
    _sampler_linear = device.CreateSampler(result, sampler_linear_desc);
    if (!vortex::success(result)) {
        vortex::error("ColorCorrection: Failed to create linear sampler: {}", result.error);
        return;
    }

    // Create point sampler for 1D LUT (no interpolation, we do it manually)
    wis::SamplerDesc sampler_point_desc{
        .min_filter = wis::Filter::Point,
        .mag_filter = wis::Filter::Point,
        .mip_filter = wis::Filter::Point,
        .anisotropic = false,
        .max_anisotropy = 1,
        .address_u = wis::AddressMode::ClampToEdge,
        .address_v = wis::AddressMode::ClampToEdge,
        .address_w = wis::AddressMode::ClampToEdge,
        .min_lod = 0.f,
        .max_lod = 1.f,
        .mip_lod_bias = 0.f,
        .comparison_op = wis::Compare::None,
    };
    _sampler_point = device.CreateSampler(result, sampler_point_desc);
    if (!vortex::success(result)) {
        vortex::error("ColorCorrection: Failed to create point sampler: {}", result.error);
    }
}

vortex::ColorCorrection::ColorCorrection(const vortex::Graphics& gfx, SerializedProperties props)
    : ImplClass(props)
    , _lazy_data(gfx)
{
    if (!lut.empty()) {
        _lut_changed = true;
    }
}

void vortex::ColorCorrection::SetLut(std::string_view path, bool notify)
{
    if (GetLut() == path) {
        return; // No change in path, skip setting
    }
    if (std::filesystem::exists(path)) {
        ColorCorrectionProperties::SetLut(path, notify);
    } else {
        vortex::error("ImageInput: Image path does not exist: {}", path);
        ColorCorrectionProperties::SetLut("", notify);
    }
    _lut_changed = true; // Mark that the path has changed
}

void vortex::ColorCorrection::Update(const vortex::Graphics& gfx)
{
    if (_lut_changed) {
        auto lut_path = GetLut();
        LoadLUT(gfx, lut_path);
        _lut_changed = false;
    }
}

bool vortex::ColorCorrection::Evaluate(const vortex::Graphics& gfx,
                                       RenderProbe& probe,
                                       const RenderPassForwardDesc* output_info)
{
    // Get input
    auto& input_base = GetSinks()[0];
    if (!input_base) {
        return false;
    }

    // Check if we need to do any processing at all
    bool has_lut = (_lut_type != LutType::Undefined);
    bool has_adjustments = (GetBrightness() != 0.0f || 
                           GetContrast() != 1.0f || 
                           GetSaturation() != 1.0f);
    
    // If no LUT and no adjustments, just pass through
    if (!has_lut && !has_adjustments) {
        return input_base.source_node->Evaluate(gfx, probe, output_info);
    }

    // Render input to intermediate texture
    auto view = probe.texture_pool.AcquireTexture(gfx);
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
    if (!eval) {
        return false;
    }

    wis::TextureBarrier after{
        .sync_before = wis::BarrierSync::RenderTarget,
        .sync_after = wis::BarrierSync::PixelShading,
        .access_before = wis::ResourceAccess::RenderTarget,
        .access_after = wis::ResourceAccess::ShaderResource,
        .state_before = wis::TextureState::RenderTarget,
        .state_after = wis::TextureState::ShaderResource,
    };
    cmd.TextureBarrier(after, tex);

    // Apply color correction
    auto root = _lazy_data.uget().GetRootSignature();
    auto pipeline = _lazy_data.uget().GetPipelineState();
    auto desc_table = probe.descriptor_buffer.SuballocateTable(2);
    auto samp_table = probe.sampler_buffer.SuballocateTable(2);

    // Render with color correction
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

    // Bind descriptors
    // If no LUT, bind the input texture to both slots (shader will detect 1D vs 3D)
    desc_table.WriteTexture(0, has_lut ? _lut_srv : sr); // LUT texture (or dummy)
    desc_table.WriteTexture(1, sr); // Input texture
    desc_table.BindOffset(gfx, cmd, root, 0);

    // Choose sampler based on LUT type and interpolation mode
    auto interp = GetLutInterp();

    // Bind samplers
    samp_table.WriteSampler(0, _lazy_data.uget().GetSampler(interp));
    samp_table.WriteSampler(1, _lazy_data.uget().GetSampler(LUTInterp::Trilinear));
    samp_table.BindOffset(gfx, cmd, root, 1);

    // Push constants
    ColorCorrectionConstants constants{
        .interp_mode = interp,
        .brightness = GetBrightness(),
        .contrast = GetContrast(),
        .saturation = GetSaturation(),
    };
    cmd.SetPushConstants(&constants, sizeof(constants) / 4, 0, wis::ShaderStages::Pixel);

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

void vortex::ColorCorrection::LoadLUT(const vortex::Graphics& gfx,
                                      const std::filesystem::path& path)
{
    if (path.empty()) {
        _lut_type = LutType::Undefined;
        return;
    }

    try {
        auto lut_data = LutLoader::LoadLut(path);
        if (lut_data.type == LutType::Undefined) {
            vortex::error("ColorCorrection: Failed to load LUT from: {}", path.string());
            _lut_type = LutType::Undefined;
            return;
        }
        _lut_type = lut_data.type;

        // Make a texture for the LUT
        CreateLUTTexture(gfx, lut_data);
        vortex::info("ColorCorrection: Loaded {} LUT (size: {}) from: {}",
                     _lut_type == LutType::Lut1D ? "1D" : "3D",
                     lut_data.stride,
                     path.string());

    } catch (const std::exception& e) {
        vortex::error("ColorCorrection: Exception loading LUT: {}", e.what());
        _lut_type = LutType::Undefined;
    }
}

void vortex::ColorCorrection::CreateLUTTexture(const vortex::Graphics& gfx, const LutData& lut_data)
{
    auto& device = gfx.GetDevice();
    auto& upload_ext = gfx.GetExtendedAllocation();
    auto& allocator = gfx.GetAllocator();

    wis::Result result = wis::success;
    wis::TextureDesc tex_desc{
        .format = wis::DataFormat::RGBA32Float,
        .size = { .width = lut_data.stride,
                 .height = lut_data.type == LutType::Lut1D ? 1 : lut_data.stride,
                 .depth_or_layers = lut_data.type == LutType::Lut1D ? 1 : lut_data.stride },
        .mip_levels = 1,
        .layout = wis::TextureLayout::Texture3D,
        .sample_count = wis::SampleRate::S1,
        .usage = wis::TextureUsage::ShaderResource | wis::TextureUsage::CopyDst |
                wis::TextureUsage::HostCopy,
    };
    _lut_texture = upload_ext.CreateGPUUploadTexture(result, allocator, tex_desc);
    if (!vortex::success(result)) {
        vortex::error("ColorCorrection: Failed to create LUT texture: {}", result.error);
        return;
    }
    // Upload data
    wis::TextureRegion region{
        .size = { tex_desc.size.width, tex_desc.size.height, tex_desc.size.depth_or_layers },
        .format = tex_desc.format
    };
    result = upload_ext.WriteMemoryToSubresourceDirect(lut_data.data.get(),
                                                       _lut_texture,
                                                       wis::TextureState::Common,
                                                       region);
    if (!vortex::success(result)) {
        vortex::error("ColorCorrection: Failed to upload LUT data: {}", result.error);
    }

    // Create SRV
    wis::ShaderResourceDesc srv_desc{
        .format = tex_desc.format,
        .view_type = wis::TextureViewType::Texture3D,
        .subresource_range = {
            .base_mip_level = 0,
            .level_count = tex_desc.mip_levels,
            .base_array_layer = 0,
            .layer_count = 1,
        },
    };
    _lut_srv = device.CreateShaderResource(result, _lut_texture, srv_desc);
    if (!vortex::success(result)) {
        vortex::error("ColorCorrection: Failed to create LUT SRV: {}", result.error);
    }
}
