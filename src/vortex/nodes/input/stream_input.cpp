#include "stream_input.h"
#include <vortex/graphics.h>
#include <vortex/util/ffmpeg/error.h>

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
}
void vortex::StreamInput::InitializeStream()
{
    if (stream_url.empty()) {
        return;
    }

    // Set a timeout to make av_read_frame non-blocking.
    // The value is in microseconds.
    ffmpeg::unique_dictionary options;
    av_dict_set(options.address_of(), "timeout", "5000", 0); // 5ms timeout

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

    // For this example, let's just activate the first video stream.
    std::array<int, 1> active_indices = { -1 }; // -1 means activate all streams
    _stream_handle = MakeUniqueStream(std::move(context), active_indices);
}

void vortex::StreamInput::Evaluate(const vortex::Graphics& gfx, vortex::RenderProbe& probe, const vortex::RenderPassForwardDesc* output_info)
{
}

vortex::StreamManager::StreamManager()
{
    // Start the IO loop thread
    _io_threads.emplace_back([this](std::stop_token stop) { IoLoop(stop); });
}
vortex::StreamManager::~StreamManager()
{
    // Stop all IO threads
    for (auto& thread : _io_threads) {
        thread.request_stop();
    }
}

void vortex::StreamManager::IoLoop(std::stop_token stop)
{
    vortex::LogView log = vortex::GetLog(vortex::stream_log_name);
    log.info("I/O thread started.");

    std::vector<std::shared_ptr<ManagedStream>> streams_to_read;
    ffmpeg::unique_packet packet;

    while (!stop.stop_requested()) {
        // Copy the list of streams to read to minimize lock time
        if (_update_pending.exchange(false, std::memory_order::relaxed)) {
            std::shared_lock lock(_streams_mutex);
            streams_to_read.clear();
            for (const auto& pair : _streams) {
                streams_to_read.push_back(pair.second);
            }
        }

        if (streams_to_read.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        bool work_done = false;
        for (const auto& stream : streams_to_read) {
            if (stop.stop_requested()) {
                break;
            }

            // Handle pending updates (e.g., changing active sub-streams)
            if (stream->update_pending.exchange(false)) {
                std::unique_lock lock(_streams_mutex);
                for (const auto& update : stream->updates) {
                    if (update.active) {
                        stream->channels[update.stream_index];
                    } else {
                        stream->channels[update.stream_index];
                    }
                }
            }

            // This is the core of the non-blocking read. av_read_frame will return
            // after a timeout (set in StreamInput::initialize_stream) or when a packet arrives.
            int ret = av_read_frame(stream->context.get(), packet.put<clear_strategy::none>());
            if (ret >= 0) {
                work_done = true;
                if (auto it = stream->channels.find(packet->stream_index); it != stream->channels.end()) {
                    it->second.packets.force_push(packet.get());
                }
            } else if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                log.error("Failed to read frame from stream: {}", ffmpeg::ffmpeg_error_string(ret));
            }
        }

        // If no stream had data, sleep briefly to prevent busy-waiting
        if (!work_done) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    log.info("I/O thread stopped.");
}

vortex::StreamManager::StreamHandle
vortex::StreamManager::RegisterStream(ffmpeg::unique_context context, std::span<int> active_channel_indices)
{
    auto stream = std::make_shared<ManagedStream>();
    stream->context = std::move(context);
    bool activate_all = active_channel_indices.size() == 1 && active_channel_indices[0] == -1;
    if (activate_all) {
        stream->channels.reserve(stream->context->nb_streams);
        for (int i = 0; i < stream->context->nb_streams; i++) {
            stream->channels[i];
        }
    } else {
        stream->channels.reserve(active_channel_indices.size());
        for (auto i : active_channel_indices) {
            stream->channels[i];
        }
    }

    std::unique_lock lock(_streams_mutex);
    StreamHandle handle = std::bit_cast<StreamHandle>(stream.get());
    _streams[handle] = std::move(stream);
    _update_pending.store(true, std::memory_order::relaxed);
    return handle;
}
void vortex::StreamManager::UnregisterStream(StreamHandle handle)
{
    if (!handle) {
        return;
    }
    std::unique_lock lock(_streams_mutex);
    _streams.erase(handle);
}
void vortex::StreamManager::SetChannelActive(StreamHandle handle, int stream_index, bool active)
{
    if (!handle) {
        return;
    }
    std::shared_lock lock(_streams_mutex);
    if (auto it = _streams.find(handle); it != _streams.end()) {
        auto& stream = it->second;
        stream->updates.emplace_back(stream_index, active);
        stream->update_pending.store(true, std::memory_order::relaxed);
    }
}
void vortex::StreamManager::ActivateChannels(StreamHandle handle, std::span<int> active_stream_indices)
{
    if (!handle) {
        return;
    }
    std::shared_lock lock(_streams_mutex);
    if (auto it = _streams.find(handle); it != _streams.end()) {
        auto& stream = it->second;
        for (int index : active_stream_indices) {
            stream->updates.emplace_back(index, 1);
        }
        stream->update_pending.store(true, std::memory_order::relaxed);
    }
}
void vortex::StreamManager::DeactivateChannels(StreamHandle handle, std::span<int> inactive_stream_indices)
{
    if (!handle) {
        return;
    }
    std::shared_lock lock(_streams_mutex);
    if (auto it = _streams.find(handle); it != _streams.end()) {
        auto& stream = it->second;
        for (int index : inactive_stream_indices) {
            stream->updates.emplace_back(index, 0);
        }
        stream->update_pending.store(true, std::memory_order::relaxed);
    }
}

bool vortex::StreamManager::try_get_packet(StreamHandle handle, int stream_index, AVPacket& packet)
{
    if (!handle) {
        return false;
    }
    //// No lock needed here because we are only reading from the SPSC queue,
    //// and the map is safe as long as the consumer (StreamInput) holds the handle.
    //if (auto it = handle->stream_data.find(stream_index); it != handle->stream_data.end()) {
    //    return it->second.packets.try_pop(packet);
    //}
    //return false;
    return false;
}