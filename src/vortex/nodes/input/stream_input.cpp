#include "stream_input.h"
#include <vortex/graphics.h>
#include <vortex/util/ffmpeg/error.h>
#include <vortex/util/ffmpeg/hw_decoder.h>

vortex::StreamInputLazy::StreamInputLazy(const vortex::Graphics& gfx)
    : _manager(gfx)
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

void vortex::StreamInput::Update(const vortex::Graphics& gfx, vortex::RenderProbe& probe)
{
    // Check if the stream URL has changed
    if (url_changed && !stream_url.empty()) {
        InitializeStream();
        url_changed = false;
    }

    // Decode new frames from the stream
    DecodeStreamFrames(gfx);
}
void vortex::StreamInput::InitializeStream()
{
    if (stream_url.empty()) {
        return;
    }

    // Set a timeout to make av_read_frame non-blocking.
    // The value is in microseconds.
    ffmpeg::unique_dictionary options;
    av_dict_set(options.address_of(), "timeout", "5000000", 0); // 1ms timeout
    av_dict_set(options.address_of(), "max_delay", "5000000", 0);
    av_dict_set(options.address_of(), "rtsp_transport", "tcp", 0);
    av_dict_set(options.address_of(), "buffer_size", "2097152", 0); // 2MB buffer size
    av_dict_set(options.address_of(), "rtsp_flags", "prefer_tcp", 0);

    // Add these options for better error handling
    av_dict_set(options.address_of(), "fflags", "+genpts+discardcorrupt", 0);
    av_dict_set(options.address_of(), "flags", "+low_delay", 0);
    av_dict_set(options.address_of(), "analyzeduration", "0", 0);


    auto context_result = codec::CodecFFmpeg::ConnectToStream(stream_url, std::move(options));
    if (!context_result) {
        return;
    }

    auto& context = context_result.value();

    auto channels_result = codec::CodecFFmpeg::GetStreams(context.get());
    if (!channels_result) {
        return;
    }

    _stream_collection = std::move(channels_result.value());

    std::array<int, 1> active_indices = { -1 }; // -1 means activate all streams
    _stream_handle = MakeUniqueStream(std::move(context), active_indices);
}
void vortex::StreamInput::DecodeStreamFrames(const vortex::Graphics& gfx)
{
    if (!_stream_handle) {
        return;
    }

    // TODO: make this less ugly
    auto& stream = *std::bit_cast<ffmpeg::ManagedStream*>(_stream_handle.get());
    auto& video_channel = stream.channels.at(0);
    auto& audio_channel = stream.channels.at(1);

    // Decode video frames
    video_channel.decoder_sem.acquire();
    while (true) {
        ffmpeg::unique_frame frame{ av_frame_alloc() };
        int ret = avcodec_receive_frame(video_channel.decoder_ctx.get(), frame.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            video_channel.sent_packets.store(0, std::memory_order::relaxed); // Reset sent packets count on EAGAIN or EOF
            frame.reset();
            break;
        } else if (ret < 0) {
            vortex::error("Error receiving video frame: {}", ffmpeg::ffmpeg_error_string(ret));
            video_channel.sent_packets.store(0, std::memory_order::relaxed); // Reset sent packets count on EAGAIN or EOF
            frame.reset();
            break;
        }
        _video_frames[frame->pts] = std::move(frame);
    }
    video_channel.decoder_sem.release();

    // Decode audio frames
    audio_channel.decoder_sem.acquire();
    while (true) {
        ffmpeg::unique_frame frame{ av_frame_alloc() };
        int ret = avcodec_receive_frame(audio_channel.decoder_ctx.get(), frame.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            audio_channel.sent_packets.store(0, std::memory_order::relaxed); // Reset sent packets count on EAGAIN or EOF
            frame.reset();
            break;
        } else if (ret < 0) {
            vortex::error("Error receiving audio frame: {}", ffmpeg::ffmpeg_error_string(ret));
            audio_channel.sent_packets.store(0, std::memory_order::relaxed); // Reset sent packets count on EAGAIN or EOF
            frame.reset();
            break;
        }
        _audio_frames[frame->pts] = std::move(frame);
    }
    audio_channel.decoder_sem.release();
    TrimOldFrames();
}

void vortex::StreamInput::TrimOldFrames()
{
    static constexpr std::size_t max_cached_frames = 16; // Max number of frames to cache
    // Trim video frames
    while (_video_frames.size() > max_cached_frames) {
        _video_frames.erase(_video_frames.begin());
    }
    // Trim audio frames
    while (_audio_frames.size() > max_cached_frames) {
        _audio_frames.erase(_audio_frames.begin());
    }
}

void vortex::StreamInput::Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info)
{
}
