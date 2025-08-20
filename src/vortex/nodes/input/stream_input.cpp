#include "stream_input.h"
#include <vortex/graphics.h>

vortex::StreamInputLazy::StreamInputLazy(const vortex::Graphics& gfx)
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
    auto root_signature = gfx.GetDescriptorBufferExtension().CreateRootSignature(result, nullptr, 0, nullptr, 0, tables, std::size(tables));
    if (!vortex::success(result)) {
        vortex::error("ImageInput: Failed to create root signature: {}", result.error);
        return;
    }

    // Load shaders for the image input node
    auto _vertex_shader = gfx.LoadShader("shaders/basic.vs");
    auto _pixel_shader = gfx.LoadShader("shaders/image.ps");

    // Create a pipeline state for the image input node
    wis::GraphicsPipelineDesc pipeline_desc{
        .root_signature = root_signature,
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

    auto pipeline_state = gfx.GetDevice().CreateGraphicsPipeline(result, pipeline_desc);
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

    _data.emplace(gfx.GetDevice().CreateSampler(result, sampler_desc), std::move(root_signature), std::move(pipeline_state));
}

void vortex::StreamInput::Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe)
{
    // Check if the stream URL has changed
    if (url_changed && !stream_url.empty()) {
        auto result = codec::CodecFFmpeg::ConnectToStream(stream_url);
        if (!result) {
            vortex::error("StreamInput: Failed to connect to stream: {}. Error: {}", stream_url, result.error().message());
            stream_url = ""; // Clear the URL if connection failed
            url_changed = false; // Reset the URL changed flag
            return; // Skip rendering if connection failed
        }
        auto stream_context = std::move(result.value());
        auto best_stream = codec::CodecFFmpeg::GetBestStream(stream_context.get(), AVMEDIA_TYPE_VIDEO);
        auto stream = best_stream.value();

        vortex::info("StreamInput: Connected to stream: {} stream: {}", stream_url, *stream);

        url_changed = false; // Reset the URL changed flag after connecting
    }
}

void vortex::StreamInput::Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info)
{

}
