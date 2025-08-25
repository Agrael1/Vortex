#include "stream_input.h"
#include <vortex/graphics.h>
#include <vortex/util/ffmpeg/error.h>
#include <vortex/util/ffmpeg/hw_decoder.h>

static std::array<wis::DX12ShaderResource, 2>
DX12CreateSRVNV12(wis::Result& result, const wis::DX12Device& device, wis::DX12TextureView texture, std::span<const wis::ShaderResourceDesc> desc) noexcept
{
    std::array<wis::DX12ShaderResource, 2> out_resource;
    auto& internal_0 = out_resource[0].GetMutableInternal();
    auto& internal_1 = out_resource[1].GetMutableInternal();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc[2];
    for (size_t i = 0; i < 2; i++) {
        srv_desc[i] = {
            .Format = convert_dx(desc[i].format),
            .ViewDimension = convert_dx(desc[i].view_type),
            .Shader4ComponentMapping = UINT(D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
                    convert_dx(desc[i].component_mapping.r),
                    convert_dx(desc[i].component_mapping.g),
                    convert_dx(desc[i].component_mapping.b),
                    convert_dx(desc[i].component_mapping.a))),
        };
        switch (desc[i].view_type) {
        case wis::TextureViewType::Texture2D:
            srv_desc[i].Texture2D = {
                .MostDetailedMip = desc[i].subresource_range.base_mip_level,
                .MipLevels = desc[i].subresource_range.level_count,
                .PlaneSlice = UINT(i), // Plane 0 for Y, Plane 1 for UV
                .ResourceMinLODClamp = 0.0f,
            };
            break;
        default:
            result = wis::make_result<wis::Func<wis::FuncD()>(), "Unsupported view type for NV12 texture">(E_INVALIDARG);
            return {};
        }
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 1,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
    };

    auto& device_x = device.GetInternal().device;

    auto x = device_x->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device_x->CreateDescriptorHeap(&heap_desc, internal_0.heap.iid(), internal_0.heap.put_void());
    device_x->CreateDescriptorHeap(&heap_desc, internal_1.heap.iid(), internal_1.heap.put_void());
    device_x->CreateShaderResourceView(std::get<0>(texture), &srv_desc[0], internal_0.heap->GetCPUDescriptorHandleForHeapStart());
    device_x->CreateShaderResourceView(std::get<0>(texture), &srv_desc[1], internal_1.heap->GetCPUDescriptorHandleForHeapStart());
    return out_resource;
}

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
    av_dict_set(options.address_of(), "timeout", "5000000", 0); // 5 second timeout
    av_dict_set(options.address_of(), "max_delay", "500000", 0); // 0.5s max delay (reduced)
    av_dict_set(options.address_of(), "rtsp_transport", "tcp", 0);
    av_dict_set(options.address_of(), "buffer_size", "1048576", 0); // 1MB buffer (reduced from 4MB)
    av_dict_set(options.address_of(), "rtsp_flags", "prefer_tcp", 0);

    // Low latency and aggressive flushing
    av_dict_set(options.address_of(), "fflags", "+genpts+discardcorrupt+flush_packets+nobuffer", 0);
    av_dict_set(options.address_of(), "flags", "+low_delay", 0);
    
    // Critical: Add error concealment options for missing reference frames
    av_dict_set(options.address_of(), "ec", "guess_mvs+deblock", 0);    
    av_dict_set(options.address_of(), "err_detect", "ignore_err", 0);   
    av_dict_set(options.address_of(), "skip_frame", "noref", 0);        

    // NEW: Additional low-latency options
    av_dict_set(options.address_of(), "reorder_queue_size", "0", 0);    // Disable frame reordering
    av_dict_set(options.address_of(), "thread_queue_size", "4", 0);     // Small thread queue
    av_dict_set(options.address_of(), "avoid_negative_ts", "make_zero", 0); // Handle timing issues

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
    
    ExtractStreamTiming();
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

    // Decode video frames with better error handling
    video_channel.decoder_sem.acquire();
    while (true) {
        ffmpeg::unique_frame frame{ av_frame_alloc() };
        int ret = avcodec_receive_frame(video_channel.decoder_ctx.get(), frame.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            video_channel.sent_packets.store(0, std::memory_order::relaxed);
            frame.reset();
            break;
        } else if (ret < 0) {
            // Log specific error types for debugging
            if (ret == AVERROR_INVALIDDATA) {
                vortex::debug("StreamInput: Invalid video data received, continuing...");
            } else {
                vortex::debug("StreamInput: Video decode error: {}", ffmpeg::ffmpeg_error_string(ret));
            }
            video_channel.sent_packets.store(0, std::memory_order::relaxed);
            frame.reset();
            break;
        }

        // Validate frame before processing
        if (frame->width <= 0 || frame->height <= 0) {
            vortex::debug("StreamInput: Received invalid frame dimensions: {}x{}", frame->width, frame->height);
            continue;
        }

        // Track the first PTS to use as baseline
        if (!_stream_timing.video_initialized && frame->pts != AV_NOPTS_VALUE) {
            _stream_timing.first_video_pts = frame->pts;
            _stream_timing.start_time = std::chrono::steady_clock::now();
            _stream_timing.video_initialized = true;
            vortex::info("StreamInput: First video frame PTS: {}", _stream_timing.first_video_pts);
        }
        
        // Only store frames with valid PTS
        if (frame->pts != AV_NOPTS_VALUE) {
            _video_frames[frame->pts] = std::move(frame);
        }
    }
    video_channel.decoder_sem.release();

    // Decode audio frames
    audio_channel.decoder_sem.acquire();
    while (true) {
        ffmpeg::unique_frame frame{ av_frame_alloc() };
        int ret = avcodec_receive_frame(audio_channel.decoder_ctx.get(), frame.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            audio_channel.sent_packets.store(0, std::memory_order::relaxed);
            frame.reset();
            break;
        } else if (ret < 0) {
            if (ret != AVERROR_INVALIDDATA) { // Don't spam logs for common corruption
                vortex::debug("StreamInput: Audio decode error: {}", ffmpeg::ffmpeg_error_string(ret));
            }
            audio_channel.sent_packets.store(0, std::memory_order::relaxed);
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

        if (frame->pts != AV_NOPTS_VALUE) {
            _audio_frames[frame->pts] = std::move(frame);
        }
    }
    audio_channel.decoder_sem.release();
    TrimOldFrames();
}

void vortex::StreamInput::TrimOldFrames()
{
    static constexpr std::size_t max_cached_frames = 32; // Max number of frames to cache
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
    auto result_texture = ffmpeg::GetTextureFromFrame(*selected_frame->get());
    if (!result_texture) {
        vortex::warn("StreamInput: Failed to get texture from frame: {}", result_texture.error().message());
        return;
    }
    auto& texture = _textures[probe.frame_number % vortex::max_frames_in_flight] = std::move(result_texture.value());

    auto result_fence = ffmpeg::GetFenceFromFrame(*selected_frame->get());
    if (!result_fence) {
        vortex::warn("StreamInput: Failed to get fence from frame: {}", result_fence.error().message());
        return;
    }
    auto& fence = _fences[probe.frame_number % vortex::max_frames_in_flight] = std::move(result_fence.value());

    auto fence_value = ffmpeg::GetFenceValueFromFrame(*selected_frame->get());
    if (!fence_value) {
        vortex::warn("StreamInput: Failed to get fence value from frame: {}", fence_value.error().message());
        return;
    }
    uint64_t value = fence_value.value();

    auto desc_hw = texture.GetMutableInternal().resource->GetDesc();

    // Create shader resource
    auto& device = gfx.GetDevice();
    wis::Result res = wis::success;
    wis::ShaderResourceDesc descs[2]{ { .format = wis::DataFormat::R8Unorm, // Y plane
                                        .view_type = wis::TextureViewType::Texture2D,
                                        .subresource_range = { 0, 1, 0, 1 } },
                                      { .format = wis::DataFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM), // UV plane
                                        .view_type = wis::TextureViewType::Texture2D,
                                        .subresource_range = { 0, 1, 0, 1 } } };

    _shader_resources[probe.frame_number % vortex::max_frames_in_flight] =
            DX12CreateSRVNV12(res, device, texture, descs);

    gfx.GetMainQueue().WaitQueue(fence, value); // Wait for the frame to be ready

    // Bind the texture and sampler to the command list
    probe._descriptor_buffer.WriteTexture(probe.frame_number % vortex::max_frames_in_flight, 0, 0, _shader_resources[probe.frame_number % vortex::max_frames_in_flight][0]);
    probe._descriptor_buffer.WriteTexture(probe.frame_number % vortex::max_frames_in_flight, 0, 1, _shader_resources[probe.frame_number % vortex::max_frames_in_flight][1]);
    probe._descriptor_buffer.WriteSampler(probe.frame_number % vortex::max_frames_in_flight, 0, 0, _lazy_data.uget()._sampler);

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