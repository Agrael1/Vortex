#include "stream_input.h"
#include <vortex/graphics.h>
#include <vortex/codec/ffmpeg/error.h>
#include <vortex/gfx/descriptor_buffer.h>

uint64_t TimeToPts(AVRational timebase, uint64_t time_ms)
{
    return av_rescale_q(time_ms, { 1, 1000 }, timebase);
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
        { .type = wis::DescriptorHeapType::Descriptor,
         .entries = entries,
         .entry_count = 2,
         .stage = wis::ShaderStages::Pixel },
        {    .type = wis::DescriptorHeapType::Sampler,
         .entries = entries + 2,
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

void vortex::StreamInput::Update(const vortex::Graphics& gfx)
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

    // Make bigger RTP buffer
    av_dict_set(options.address_of(), "buffer_size", "1048576", 0); // 1MB buffer
    av_dict_set(options.address_of(), "rtbufsize", "1048576", 0); // 1MB buffer
    av_dict_set(options.address_of(), "max_delay", "500000", 0); // 0.5 second max delay

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

bool vortex::StreamInput::Evaluate(const vortex::Graphics& gfx,
                                   vortex::RenderProbe& probe,
                                   const vortex::RenderPassForwardDesc* output_info)
{
    // Check if the texture is valid before rendering
    if (_video_frames.empty()) {
        // vortex::info("ImageInput: Texture is not valid or has zero size.");
        return false; // Skip rendering if texture is not valid
    }

    auto now = std::chrono::steady_clock::now();
    if (_first_video_pts == AV_NOPTS_VALUE) {
        return false;
    }

    static std::streamsize frames_available = 0;
    uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                                _start_time_video)
                                  .count();
    uint64_t current_video_pts = _first_video_pts +
            TimeToPts(_stream_collection.video_channels[0]->time_base, elapsed_ms);
    // adjust for some latency
    current_video_pts = std::max<int64_t>(_first_video_pts, current_video_pts - 2000);
    auto it = _video_frames.lower_bound(current_video_pts);
    if (it == _video_frames.end()) {
        return false; // No frames ready to be played
    }

    AVFrame* frame = it->second.get();

    // Get D3D12 texture from the frame
    auto result_texture = ffmpeg::GetTextureFromFrame(*frame);
    if (!result_texture) {
        vortex::warn("StreamInput: Failed to get texture from frame: {}",
                     result_texture.error().message());
        return false;
    }
    auto& texture = _textures[probe.frame_number % vortex::max_frames_in_flight] = std::move(
            result_texture.value());

    auto result_fence = ffmpeg::GetFenceFromFrame(*frame);
    if (!result_fence) {
        vortex::warn("StreamInput: Failed to get fence from frame: {}",
                     result_fence.error().message());
        return false;
    }
    auto& fence = _fences[probe.frame_number % vortex::max_frames_in_flight] = std::move(
            result_fence.value());

    auto fence_value = ffmpeg::GetFenceValueFromFrame(*frame);
    if (!fence_value) {
        vortex::warn("StreamInput: Failed to get fence value from frame: {}",
                     fence_value.error().message());
        return false;
    }
    uint64_t value = fence_value.value();

    // Suballocate a table
    auto desc_table = probe.descriptor_buffer.SuballocateTable(2);
    auto sampler_table = probe.sampler_buffer.SuballocateTable(1);

    // Create shader resource
    auto& device = gfx.GetDevice();
    wis::Result res = wis::success;
    wis::ShaderResourceDesc descs[2]{
        {                             .format = wis::DataFormat::R8Unorm, // Y plane
 .view_type = wis::TextureViewType::Texture2D,
         .subresource_range = { 0, 1, 0, 1 } },
        { .format = wis::DataFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM), // UV plane
 .view_type = wis::TextureViewType::Texture2D,
         .subresource_range = { 0, 1, 0, 1 } }
    };

    _shader_resources[probe.frame_number %
                      vortex::max_frames_in_flight] = vortex::ffmpeg::DX12CreateSRVNV12(res,
                                                                                        device,
                                                                                        texture,
                                                                                        descs);

    std::ignore = gfx.GetMainQueue().WaitQueue(fence, value); // Wait for the frame to be ready

    // Bind the texture and sampler to the command list
    desc_table.WriteTexture(
            0,
            _shader_resources[probe.frame_number % vortex::max_frames_in_flight][0]); // Y plane
    desc_table.WriteTexture(
            1,
            _shader_resources[probe.frame_number % vortex::max_frames_in_flight][1]); // UV plane
    sampler_table.WriteSampler(0, _lazy_data.uget()._sampler);

    wis::RenderPassRenderTargetDesc target_desc{
        .target = output_info->current_rt_view,
        .load_op = wis::LoadOperation::Clear,
        .store_op = wis::StoreOperation::Store,
        .clear_value = { 0.f, 0.f, 0.f, 1.f }  // Clear to transparent black
    };
    wis::RenderPassDesc pass_desc{
        .target_count = 1,
        .targets = &target_desc,
    };

    auto& cmd_list = *probe.command_list;
    auto& root = _lazy_data.uget()._root_signature;

    // Begin the render pass
    cmd_list.BeginRenderPass(pass_desc);
    cmd_list.SetPipelineState(_lazy_data.uget()._pipeline_state);
    cmd_list.SetRootSignature(root);
    cmd_list.RSSetScissor(
            { 0, 0, int(output_info->output_size.width), int(output_info->output_size.height) });
    cmd_list.RSSetViewport({ 0.f,
                             0.f,
                             float(output_info->output_size.width),
                             float(output_info->output_size.height),
                             0.f,
                             1.f });
    cmd_list.IASetPrimitiveTopology(wis::PrimitiveTopology::TriangleList);
    desc_table.BindOffset(gfx, cmd_list, root, 0);
    sampler_table.BindOffset(gfx, cmd_list, root, 1);
    // Draw a quad that covers the viewport
    cmd_list.DrawInstanced(3, 1, 0, 0);

    // End the render pass
    cmd_list.EndRenderPass();
    return true;
}

void vortex::StreamInput::EvaluateAudio(vortex::AudioProbe& probe)
{
    auto now = std::chrono::steady_clock::now();
    if (_first_audio_pts == AV_NOPTS_VALUE) {
        return;
    }

    static std::streamsize samples_available = 0;

    uint64_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                                _start_time_audio)
                                  .count();
    uint64_t current_audio_pts = _first_audio_pts +
            TimeToPts(_stream_collection.audio_channels[0]->time_base, elapsed_ms);

    // Get all audio frames that are ready to be played
    auto it = _audio_frames.lower_bound(current_audio_pts);
    if (it == _audio_frames.end()) {
        return; // No frames ready to be played
    }

    std::size_t frames = std::distance(it, _audio_frames.end());
    if (frames > 3) {
        frames = 3; // Limit to 3 frames to avoid excessive latency
    }

    AVFrame* frame = it->second.get();
    probe.first_audio_pts = it->first;

    auto& data = probe.audio_data;
    data.resize(frame->ch_layout.nb_channels * frame->nb_samples * frames);
    auto delta = frame->nb_samples * frames;

    for (std::size_t i = 0; i < frames; ++i) {
        AVFrame* frame = it->second.get();
        // MOCK: assume the audio is already in float format and has stereo planar layout
        if (frame->format == AV_SAMPLE_FMT_FLTP && frame->ch_layout.nb_channels == 2) {
            std::memcpy(data.data() + frame->nb_samples * i,
                        frame->data[0],
                        frame->nb_samples * sizeof(float)); // Left channel
            std::memcpy(data.data() + delta + frame->nb_samples * i,
                        frame->data[1],
                        frame->nb_samples * sizeof(float)); // Right channel
        } else {
            vortex::warn("StreamInput: Unsupported audio format or channel count. Expected float "
                         "planar stereo.");
        }
        // erase the frame after consuming
        auto next_it = std::next(it);
        _audio_frames.erase(it);
        it = next_it;
    }

    // probe.last_audio_pts = it->first + frame->duration;
}

void vortex::StreamInput::DecodeVideoFrames(vortex::ffmpeg::ChannelStorage& video_channel)
{
    static int64_t last_pts = AV_NOPTS_VALUE;

    // try read frames from atomic queue
    while (auto frame = video_channel.GetDecodedFrame()) {
        // if first audio frame, set the first audio pts
        if (_first_video_pts == AV_NOPTS_VALUE) {
            _start_time_video = std::chrono::steady_clock::now();
            _first_video_pts = _first_video_pts == AV_NOPTS_VALUE ? frame->get()->pts
                                                                  : _first_video_pts;
        }

        AVFrame* raw_frame = frame->get();
        // vortex::info("Drained audio frame with PTS: {}, nb_samples: {}, dpts: {}",
        // raw_frame->pts, raw_frame->nb_samples, raw_frame->pts - last_pts);
        last_pts = raw_frame->pts;

        _video_frames[raw_frame->pts] = std::move(frame.value());
    }
}
void vortex::StreamInput::DecodeAudioFrames(vortex::ffmpeg::ChannelStorage& audio_channel)
{
    static int64_t last_pts = AV_NOPTS_VALUE;

    // try read frames from atomic queue
    while (auto frame = audio_channel.GetDecodedFrame()) {
        // if first audio frame, set the first audio pts
        if (_first_audio_pts == AV_NOPTS_VALUE) {
            _start_time_audio = std::chrono::steady_clock::now();
            _first_audio_pts = _first_audio_pts == AV_NOPTS_VALUE ? frame->get()->pts
                                                                  : _first_audio_pts;
        }

        AVFrame* raw_frame = frame->get();
        // vortex::info("Drained audio frame with PTS: {}, nb_samples: {}, dpts: {}",
        // raw_frame->pts, raw_frame->nb_samples, raw_frame->pts - last_pts);
        last_pts = raw_frame->pts;

        _audio_frames[raw_frame->pts] = std::move(frame.value());
    }
}
