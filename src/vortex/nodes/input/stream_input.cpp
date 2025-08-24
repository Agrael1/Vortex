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

        // Track the first PTS to use as baseline
        if (!_stream_timing.video_initialized && frame->pts != AV_NOPTS_VALUE) {
            _stream_timing.first_video_pts = frame->pts;
            _stream_timing.start_time = std::chrono::steady_clock::now();
            _stream_timing.video_initialized = true;
            vortex::info("StreamInput: First video frame PTS: {}", _stream_timing.first_video_pts);
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

        if (!_stream_timing.audio_initialized && frame->pts != AV_NOPTS_VALUE) {
            _stream_timing.first_audio_pts = frame->pts;
            if (!_stream_timing.video_initialized) {
                _stream_timing.start_time = std::chrono::steady_clock::now();
            }
            _stream_timing.audio_initialized = true;
            vortex::info("StreamInput: First audio frame PTS: {}", _stream_timing.first_audio_pts);
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
    // Check if the texture is valid before rendering
    if (_video_frames.empty()) {
        // vortex::info("ImageInput: Texture is not valid or has zero size.");
        return; // Skip rendering if texture is not valid
    }

    auto* selected_frame = SelectFrameForCurrentTime(probe);
    if (!selected_frame || !selected_frame->get()) {
        vortex::warn("StreamInput: No suitable frame found for current time.");
        return;
    }

    // Get D3D12 texture from the frame
    auto result = ffmpeg::GetTextureFromFrame(*selected_frame->get());
    if (!result) {
        vortex::warn("StreamInput: Failed to get texture from frame: {}", result.error().message());
        return;
    }
    
    auto& texture = result.value();
    // Create shader resource
    auto& device = gfx.GetDevice();
    wis::Result res = wis::success;
    wis::ShaderResourceDesc desc{
        
    };
    _shader_resources[probe.frame_number % vortex::max_frames_in_flight] = device.CreateShaderResource(res, texture, desc);

    // Bind the texture and sampler to the command list
    probe._descriptor_buffer.GetCurrentDescriptorBuffer().WriteTexture(0, 0, _shader_resources[probe.frame_number % vortex::max_frames_in_flight]);
    probe._descriptor_buffer.GetSamplerBuffer().WriteSampler(0, 0, _lazy_data.uget()._sampler);


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
    cmd_list.SetPipelineState(_lazy_data.uget()._pipeline_state);
    cmd_list.SetRootSignature(_lazy_data.uget()._root_signature);
    cmd_list.RSSetScissor({ 0, 0, int(output_info->_output_size.width), int(output_info->_output_size.height) });
    cmd_list.RSSetViewport({ 0.f, 0.f, float(output_info->_output_size.width), float(output_info->_output_size.height), 0.f, 1.f });
    cmd_list.IASetPrimitiveTopology(wis::PrimitiveTopology::TriangleList);
    std::array<DescriptorTableOffset, 2> offsets = {
        DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = false }, // Texture
        DescriptorTableOffset{ .descriptor_table_offset = 0, .is_sampler_table = true } // Sampler
    };
    probe._descriptor_buffer.BindOffsets(gfx, cmd_list, _lazy_data.uget()._root_signature, probe.frame_number % vortex::max_frames_in_flight, offsets);
    // Draw a quad that covers the viewport
    cmd_list.DrawInstanced(3, 1, 0, 0);

    // End the render pass
    cmd_list.EndRenderPass();
}

void vortex::StreamInput::ExtractStreamTiming()
{
    // Reset timing info
    _stream_timing = StreamTimingInfo{};

    // Extract video timing from first video stream
    if (!_stream_collection.video_channels.empty()) {
        AVStream* video_stream = _stream_collection.video_channels[0];

        _stream_timing.video_timebase = video_stream->time_base;

        // Extract framerate - prefer avg_frame_rate, fallback to r_frame_rate
        if (video_stream->avg_frame_rate.num > 0 && video_stream->avg_frame_rate.den > 0) {
            _stream_timing.video_framerate = video_stream->avg_frame_rate;
        } else if (video_stream->r_frame_rate.num > 0 && video_stream->r_frame_rate.den > 0) {
            _stream_timing.video_framerate = video_stream->r_frame_rate;
        } else {
            // Try to get framerate from codec parameters
            if (video_stream->codecpar->framerate.num > 0 && video_stream->codecpar->framerate.den > 0) {
                _stream_timing.video_framerate = video_stream->codecpar->framerate;
            } else {
                _stream_timing.video_framerate = { 30, 1 }; // 30 FPS fallback
                vortex::warn("StreamInput: No video framerate found, using 30 FPS default");
            }
        }

        vortex::info("StreamInput: Video - timebase: {}/{}, framerate: {}/{}",
                     _stream_timing.video_timebase.num, _stream_timing.video_timebase.den,
                     _stream_timing.video_framerate.num, _stream_timing.video_framerate.den);
    }

    // Extract audio timing from first audio stream
    if (!_stream_collection.audio_channels.empty()) {
        AVStream* audio_stream = _stream_collection.audio_channels[0];

        _stream_timing.audio_timebase = audio_stream->time_base;
        _stream_timing.audio_sample_rate = audio_stream->codecpar->sample_rate;

        vortex::info("StreamInput: Audio - timebase: {}/{}, sample_rate: {} Hz",
                     _stream_timing.audio_timebase.num, _stream_timing.audio_timebase.den,
                     _stream_timing.audio_sample_rate);
    }
}

int64_t vortex::StreamInput::CalculateTargetVideoPTS(const vortex::RenderProbe& probe) const
{
    if (!_stream_timing.video_initialized || _stream_timing.first_video_pts == AV_NOPTS_VALUE) {
        return _stream_timing.first_video_pts;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - _stream_timing.start_time);

    // Calculate target frame based on output timing
    double output_frame_duration_ms = 1000.0 * probe.output_framerate.denominator / probe.output_framerate.numerator;
    double video_frame_duration_ms = 1000.0 * _stream_timing.video_framerate.den / _stream_timing.video_framerate.num;

    // Frame selection ratio (handles framerate mismatch between stream and output)
    double frame_ratio = video_frame_duration_ms / output_frame_duration_ms;
    double output_frames_elapsed = elapsed.count() / 1000.0 / output_frame_duration_ms;
    double target_video_frame = output_frames_elapsed * frame_ratio;

    // Convert frame index to PTS using actual video timebase
    int64_t frame_duration_pts = av_rescale_q(1,
                                              av_inv_q(_stream_timing.video_framerate),
                                              _stream_timing.video_timebase);

    int64_t target_pts = _stream_timing.first_video_pts +
            static_cast<int64_t>(target_video_frame * frame_duration_pts);

    return target_pts;
}

int64_t vortex::StreamInput::CalculateTargetAudioPTS(const vortex::RenderProbe& probe) const
{
    if (!_stream_timing.audio_initialized || _stream_timing.first_audio_pts == AV_NOPTS_VALUE) {
        return _stream_timing.first_audio_pts;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - _stream_timing.start_time);

    // Convert elapsed time to audio samples
    int64_t samples_elapsed = (elapsed.count() * _stream_timing.audio_sample_rate) / 1000000;

    // Convert samples to PTS using audio timebase
    int64_t pts_offset = av_rescale_q(samples_elapsed,
                                      { 1, _stream_timing.audio_sample_rate },
                                      _stream_timing.audio_timebase);

    return _stream_timing.first_audio_pts + pts_offset;
}

vortex::ffmpeg::unique_frame* vortex::StreamInput::SelectFrameForCurrentTime(const vortex::RenderProbe& probe)
{
    if (_video_frames.empty() || !_stream_timing.video_initialized) {
        return _video_frames.empty() ? nullptr : &_video_frames.begin()->second;
    }

    int64_t target_pts = CalculateTargetVideoPTS(probe);

    // Find the frame closest to target PTS
    auto it = _video_frames.lower_bound(target_pts);

    if (it == _video_frames.end()) {
        return &_video_frames.rbegin()->second;
    }

    if (it == _video_frames.begin()) {
        return &it->second;
    }

    // Check if previous frame is closer to target
    auto prev_it = std::prev(it);
    int64_t curr_diff = std::abs(target_pts - it->first);
    int64_t prev_diff = std::abs(target_pts - prev_it->first);

    return (prev_diff <= curr_diff) ? &prev_it->second : &it->second;
}