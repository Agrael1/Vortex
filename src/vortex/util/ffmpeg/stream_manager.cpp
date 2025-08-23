#include <vortex/util/ffmpeg/stream_manager.h>
#include <vortex/util/ffmpeg/error.h>
#include <vortex/graphics.h>

vortex::ffmpeg::StreamManager::StreamManager(const vortex::Graphics& gfx)
    : _va_decode_context(ffmpeg::CreateDecodeContext(gfx.GetDevice()).value())
{
    // Start the IO loop thread
    _io_threads.emplace_back([this](std::stop_token stop) { IoLoop(stop); });
}
vortex::ffmpeg::StreamManager::~StreamManager()
{
    // Stop all IO threads
    for (auto& thread : _io_threads) {
        thread.request_stop();
    }
}

void vortex::ffmpeg::StreamManager::IoLoop(std::stop_token stop)
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
                        stream->channels.erase(update.stream_index);
                    }
                }
            }

            // This is the core of the non-blocking read. av_read_frame will return
            // after a timeout (set in StreamInput::initialize_stream) or when a packet arrives.
            int ret = av_read_frame(stream->context.get(), packet.put<clear_strategy::none>());
            if (ret >= 0) {
                work_done = true;
                if (auto it = stream->channels.find(packet->stream_index); it != stream->channels.end()) {
                    it->second.packets.force_push(packet.release());
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

vortex::ffmpeg::StreamManager::StreamHandle
vortex::ffmpeg::StreamManager::RegisterStream(ffmpeg::unique_context context, std::span<int> active_channel_indices)
{
    auto stream = std::make_shared<ManagedStream>();
    stream->context = std::move(context);
    bool activate_all = active_channel_indices.size() == 1 && active_channel_indices[0] == -1;
    if (activate_all) {
        stream->channels.reserve(stream->context->nb_streams);
        for (int i = 0; i < stream->context->nb_streams; i++) {
            // If it's a video stream, set the decode context
            if (stream->context->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
                stream->channels[i];
                continue;
            }
            AVCodecParameters* codec_params = stream->context->streams[i]->codecpar;
            const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
            if (!codec) {
                continue;
            }

            stream->channels[i];
            stream->channels[i].decoder_ctx.reset(avcodec_alloc_context3(codec));
            if (!stream->channels[i].decoder_ctx) {
                stream->channels.erase(i);
                continue;
            }
            if (avcodec_parameters_to_context(stream->channels[i].decoder_ctx.get(), stream->context->streams[i]->codecpar) < 0) {
                stream->channels.erase(i);
                continue;
            }
            stream->channels[i].decoder_ctx->hw_device_ctx = av_buffer_ref(_va_decode_context.GetHWDeviceContext());
            if (avcodec_open2(stream->channels[i].decoder_ctx.get(), codec, nullptr) < 0) {
                stream->channels.erase(i);
                continue;
            }
        }
    } else {
        stream->channels.reserve(active_channel_indices.size());
        for (auto i : active_channel_indices) {
            // If it's a video stream, set the decode context
            if (stream->context->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
                stream->channels[i];
                continue;
            }
            AVCodecParameters* codec_params = stream->context->streams[i]->codecpar;
            const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
            if (!codec) {
                continue;
            }

            stream->channels[i];
            stream->channels[i].decoder_ctx.reset(avcodec_alloc_context3(codec));
            if (!stream->channels[i].decoder_ctx) {
                stream->channels.erase(i);
                continue;
            }
            if (avcodec_parameters_to_context(stream->channels[i].decoder_ctx.get(), stream->context->streams[i]->codecpar) < 0) {
                stream->channels.erase(i);
                continue;
            }
            stream->channels[i].decoder_ctx->hw_device_ctx = av_buffer_ref(_va_decode_context.GetHWDeviceContext());
            if (avcodec_open2(stream->channels[i].decoder_ctx.get(), codec, nullptr) < 0) {
                stream->channels.erase(i);
                continue;
            }
        }
    }

    std::unique_lock lock(_streams_mutex);
    StreamHandle handle = std::bit_cast<StreamHandle>(stream.get());
    _streams[handle] = std::move(stream);
    _update_pending.store(true, std::memory_order::relaxed);
    return handle;
}
void vortex::ffmpeg::StreamManager::UnregisterStream(StreamHandle handle)
{
    if (!handle) {
        return;
    }
    std::unique_lock lock(_streams_mutex);
    _streams.erase(handle);
}
void vortex::ffmpeg::StreamManager::SetChannelActive(StreamHandle handle, int stream_index, bool active)
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
void vortex::ffmpeg::StreamManager::ActivateChannels(StreamHandle handle, std::span<int> active_stream_indices)
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
void vortex::ffmpeg::StreamManager::DeactivateChannels(StreamHandle handle, std::span<int> inactive_stream_indices)
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

std::optional<vortex::ffmpeg::unique_packet>
vortex::ffmpeg::StreamManager::TryGetPacket(StreamHandle handle, int stream_index)
{
    if (!handle) {
        return {};
    }

    std::optional<vortex::ffmpeg::unique_packet> packet;

    // Convert handle to pointer
    auto& stream = *std::bit_cast<ManagedStream*>(handle);
    if (auto channel_it = stream.channels.find(stream_index); channel_it != stream.channels.end()) {
        // SPSCQueue::try_pop is thread-safe for one producer/one consumer
        bool success = channel_it->second.packets.try_pop(packet.emplace());
        if (!success) {
            packet.reset();
        }
    }

    return packet;
}