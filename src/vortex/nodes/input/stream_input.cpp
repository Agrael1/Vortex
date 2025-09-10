#include "stream_input.h"
#include <vortex/graphics.h>
#include <vortex/util/ffmpeg/error.h>

vortex::StreamInputLazy::StreamInputLazy(const vortex::Graphics& gfx)
    : _manager(gfx)
{
    wis::Result result = wis::success;

    wis::DescriptorTableEntry entries[] = {
        { .type = wis::DescriptorType::Texture, .bind_register = 0, .binding = 0, .count = 1 },
        { .type = wis::DescriptorType::Texture, .bind_register = 1, .binding = 0, .count = 1 },
        { .type = wis::DescriptorType::Sampler, .bind_register = 0, .binding = 0, .count = 1 },
    };
    wis::DescriptorTable tables[] = {
        { .type = wis::DescriptorHeapType::Descriptor, .entries = entries, .entry_count = 2, .stage = wis::ShaderStages::Pixel },
        { .type = wis::DescriptorHeapType::Sampler, .entries = entries + 2, .entry_count = 1, .stage = wis::ShaderStages::Pixel },
    };
    _root_signature = gfx.GetDescriptorBufferExtension().CreateRootSignature(result, nullptr, 0, nullptr, 0, tables, std::size(tables));
    if (!vortex::success(result)) {
        vortex::error("ImageInput: Failed to create root signature: {}", result.error);
        return;
    }

    // Load shaders for the image input node
    auto vertex_shader = gfx.LoadShader("shaders/basic.vs");
    auto pixel_shader = gfx.LoadShader("shaders/video.ps");

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

    // Optimized settings for low latency and reduced buffering
    ffmpeg::unique_dictionary options;
    av_dict_set(options.address_of(), "timeout", "10000000", 0); // 5 second timeout
    av_dict_set(options.address_of(), "analyzeduration", "10000000", 0); // 0.5 second timeout
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
    _stream_indices[0] = _stream_collection.video_channels[0]->index;
    _stream_indices[1] = _stream_collection.audio_channels[0]->index;

    std::array<int, 1> active_indices = { -1 };
    _stream_handle = MakeUniqueStream(std::move(context), active_indices);
}
void vortex::StreamInput::DecodeStreamFrames(const vortex::Graphics& gfx)
{
    if (!_stream_handle) {
        return;
    }

    // TODO: make this less ugly
    auto& stream = *std::bit_cast<ffmpeg::ManagedStream*>(_stream_handle.get());
    auto& video_channel = stream.channels.at(_stream_indices[0]);
    auto& audio_channel = stream.channels.at(_stream_indices[1]);

    DecodeVideoFrames(video_channel);
    DecodeAudioFrames(audio_channel);

    // Remove old video frames (keep only the latest 100 frames)
    while (_video_frames.size() > 100) {
        _video_frames.erase(_video_frames.begin());
    }
    // Remove old audio frames (keep only the latest 100 frames)
    while (_audio_frames.size() > 100) {
        _audio_frames.erase(_audio_frames.begin());
    }
}

void vortex::StreamInput::Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info)
{
    // Check if the texture is valid before rendering
    if (_video_frames.empty()) {
        // vortex::info("ImageInput: Texture is not valid or has zero size.");
        return; // Skip rendering if texture is not valid
    }
}

uint64_t TimeToPts(AVRational timebase, uint64_t time_ms)
{
    return av_rescale_q(time_ms, { 1, 1000 }, timebase);
}

void vortex::StreamInput::EvaluateAudio(vortex::AudioProbe& probe)
{
    auto now = std::chrono::steady_clock::now();
    if (_first_audio_pts == AV_NOPTS_VALUE) {
        return;
    }

    static std::streamsize samples_available = 0;

    uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _start_time_audio).count();
    uint64_t current_audio_pts = _first_audio_pts + TimeToPts(_stream_collection.audio_channels[0]->time_base, elapsed_ms);
    // Slightly adjust current_audio_pts to account for processing delays
    current_audio_pts = current_audio_pts > 0 ? current_audio_pts - 6000 : current_audio_pts;

    if (current_audio_pts < _first_audio_pts) {
        return; // Not enough time has passed to play audio
    }


    std::size_t samples = 0;
    std::size_t frames_used = 0;
    for (const auto& [pts, frame] : _audio_frames) {
        if (pts < current_audio_pts) {
            continue; // Skip frames that are before the current audio PTS
        }
        samples += frame->nb_samples;
        frames_used++;
    }
    samples_available += samples;

    samples_available -= 800; // 60 frames at 48kHz
    vortex::info("Available audio frames: {}, that have {} samples. Remaining samples {}", frames_used, samples, samples_available);
    _audio_frames.clear();
}

void vortex::StreamInput::DecodeVideoFrames(vortex::ffmpeg::PacketStorage& video_channel)
{
    ffmpeg::ffmpeg_errc last_error = ffmpeg::ffmpeg_errc::success;
    video_channel.decoder_sem.acquire();
    do {
        auto frame = video_channel.Decode();
        if (frame) {
            // if first video frame, set the first video pts
            if (_first_video_pts == AV_NOPTS_VALUE) {
                _start_time_video = std::chrono::steady_clock::now();
                _first_video_pts = _first_video_pts == AV_NOPTS_VALUE ? frame->get()->pts : _first_video_pts;
            }

            AVFrame* raw_frame = frame->get();
            _video_frames[raw_frame->pts] = std::move(frame.value());
        } else {
            last_error = frame.error();
            if (last_error == ffmpeg::ffmpeg_errc::not_enough_data || last_error == ffmpeg::ffmpeg_errc::end_of_file) {
                break; // No more frames available right now
            } else if (last_error != ffmpeg::ffmpeg_errc::success) {
                vortex::error("Error during decoding video frame: {}", ffmpeg::ffmpeg_error_string(static_cast<int>(last_error)));
                break;
            }
        }
    } while (last_error == ffmpeg::ffmpeg_errc::success);
    video_channel.sent_packets.store(0, std::memory_order_release); // Reset sent packets count after decoding
    video_channel.decoder_sem.release();
}
void vortex::StreamInput::DecodeAudioFrames(vortex::ffmpeg::PacketStorage& audio_channel)
{
    ffmpeg::ffmpeg_errc last_error = ffmpeg::ffmpeg_errc::success;
    uint32_t frames_decoded = 0;

    audio_channel.decoder_sem.acquire();
    do {
        auto frame = audio_channel.Decode();
        if (frame) {
            // if first audio frame, set the first audio pts
            if (_first_audio_pts == AV_NOPTS_VALUE) {
                _start_time_audio = std::chrono::steady_clock::now();
                _first_audio_pts = _first_audio_pts == AV_NOPTS_VALUE ? frame->get()->pts : _first_audio_pts;
            }

            AVFrame* raw_frame = frame->get();
            _audio_frames[raw_frame->pts] = std::move(frame.value());
            frames_decoded++;
        } else {
            last_error = frame.error();
            if (last_error == ffmpeg::ffmpeg_errc::not_enough_data || last_error == ffmpeg::ffmpeg_errc::end_of_file) {
                break; // No more frames available right now
            } else if (last_error != ffmpeg::ffmpeg_errc::success) {
                vortex::error("Error during decoding video frame: {}", ffmpeg::ffmpeg_error_string(static_cast<int>(last_error)));
                break;
            }
        }
    } while (last_error == ffmpeg::ffmpeg_errc::success);
    audio_channel.sent_packets.store(0, std::memory_order_release); // Reset sent packets count after decoding
    audio_channel.decoder_sem.release();

    //if (frames_decoded > 0) {
    //    vortex::info("Decoded {} audio frames", frames_decoded);
    //}
}
