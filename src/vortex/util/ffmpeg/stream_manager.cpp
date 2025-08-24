#include <vortex/util/ffmpeg/stream_manager.h>
#include <vortex/util/ffmpeg/error.h>
#include <vortex/graphics.h>

vortex::ffmpeg::StreamManager::StreamManager(const vortex::Graphics& gfx)
    : _log(vortex::GetLog(vortex::stream_log_name))
    , _va_decode_context(ffmpeg::CreateDecodeContext(gfx.GetDevice()).value())
{
    // Start the IO loop thread
    _io_threads.emplace_back([this](std::stop_token stop) { IOLoop(stop); });
}
vortex::ffmpeg::StreamManager::~StreamManager()
{
    // Stop all IO threads
    for (auto& thread : _io_threads) {
        thread.request_stop();
    }
}

void vortex::ffmpeg::StreamManager::IOLoop(std::stop_token stop)
{
    _log.info("I/O thread started.");

    std::vector<std::shared_ptr<ManagedStream>> streams_to_read;
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
                        InitDecoder(*stream, update.stream_index);
                    } else {
                        stream->channels.erase(update.stream_index);
                    }
                }
            }

            work_done = IOProcessStream(*stream);
        }

        // If no stream had data, sleep briefly to prevent busy-waiting
        if (!work_done) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    _log.info("I/O thread stopped.");
}
void vortex::ffmpeg::StreamManager::IOFlushStream(vortex::ffmpeg::ManagedStream& stream)
{
    for (auto& [index, channel] : stream.channels) {
        if (!channel.decoder_ctx) {
            continue;
        }

        // Try to acquire the semaphore to avoid blocking if the decoder is busy
        if (!channel.decoder_sem.try_acquire()) {
            // Enqueue a flush request to be handled later
            channel.packets.emplace();
            continue; // Decoder is busy, skip flushing for now
        }

        // Send all the queued packets first
        while (!channel.packets.empty()) {
            avcodec_send_packet(channel.decoder_ctx.get(), channel.packets.front().address_of());
            channel.packets.pop();
        }

        // Send a null packet to flush the decoder
        if (avcodec_send_packet(channel.decoder_ctx.get(), nullptr) < 0) {
            _log.error("Error sending flush packet to decoder for stream index {}", index);
            channel.decoder_sem.release();
            continue;
        }
        channel.decoder_sem.release();
    }
}
bool vortex::ffmpeg::StreamManager::IOProcessStream(vortex::ffmpeg::ManagedStream& stream)
{
    // Check if there are any active channels
    if (stream.channels.empty()) {
        return false;
    }

    // Check if any decoder is overloaded with sent packets
    for (auto& [index, channel] : stream.channels) {
        if (channel.sent_packets.load(std::memory_order::relaxed) >= PacketStorage::max_sent_packets) {
            _log.warn("Decoder for stream index {} is overloaded ({} sent packets). Skipping read.", index, channel.sent_packets.load(std::memory_order::relaxed));
            return false; // Skip reading if any decoder is overloaded
        }
    }

    // This is the core of the non-blocking read. av_read_frame will return
    // after a timeout (set in StreamInput::initialize_stream) or when a packet arrives.
    ffmpeg::unique_packet packet;
    int ret = av_read_frame(stream.context.get(), packet.put<clear_strategy::none>());
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN)) {
            return false; // No data available right now
        }
        if (ret == AVERROR_EOF) {
            // End of stream, send flush packets to all decoders
            IOFlushStream(stream);
        }
        _log.error("Error reading frame from stream: {}", ffmpeg::ffmpeg_error_string(ret));
        return false;
    }

    // Successfully read a packet, now dispatch it to the appropriate channel
    auto it = stream.channels.find(packet->stream_index);
    if (it == stream.channels.end()) {
        // No active channel for this stream index, discard the packet
        return false;
    }

    // We have a packet for an active channel
    auto& channel = it->second;
    if (!channel.decoder_sem.try_acquire()) {
        channel.packets.emplace(std::move(packet));
        return true;
    }

    // We have the semaphore, send the packet
    if (!channel.packets.empty()) {
        channel.sent_packets.fetch_add(channel.packets.size(), std::memory_order::relaxed);
        // If there are queued packets, send them first
        while (!channel.packets.empty()) {
            auto& queued_packet = channel.packets.front();
            avcodec_send_packet(channel.decoder_ctx.get(), queued_packet.address_of());
            channel.packets.pop();
        }
    }

    // Now send the current packet
    avcodec_send_packet(channel.decoder_ctx.get(), packet.address_of());
    channel.sent_packets.fetch_add(1, std::memory_order::relaxed);
    channel.decoder_sem.release();
    return true;
}

bool vortex::ffmpeg::StreamManager::InitVideoDecoder(vortex::ffmpeg::ManagedStream& stream, int channel)
{
    av_log_set_level(AV_LOG_WARNING);
    AVCodecParameters* codec_params = stream.context->streams[channel]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        _log.error("Failed to find decoder for stream {}: {}", channel, *codec_params);
        return false;
    }

    stream.channels[channel]; // Ensure the channel entry exists
    stream.channels[channel].decoder_ctx.reset(avcodec_alloc_context3(codec));
    if (!stream.channels[channel].decoder_ctx) {
        _log.error("Failed to allocate decoder context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    auto& decoder = stream.channels[channel].decoder_ctx;

    if (avcodec_parameters_to_context(decoder.get(), stream.context->streams[channel]->codecpar) < 0) {
        _log.error("Failed to copy codec parameters to context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    decoder->hw_device_ctx = av_buffer_ref(_va_decode_context.GetHWDeviceContext());
    auto frames_ctx_result = _va_decode_context.CreateHWFramesContext(
            decoder->width,
            decoder->height,
            AV_PIX_FMT_NV12);

    if (!frames_ctx_result) {
        _log.error("Failed to create HW frames context for stream {}: {}", channel, frames_ctx_result.error().message());
        stream.channels.erase(channel);
        return false;
    }
    decoder->hw_frames_ctx = frames_ctx_result.value().release();
    decoder->thread_count = 1; // Use single-threaded decoding for hardware acceleration
    decoder->thread_type = FF_THREAD_FRAME;

    // Configure decoder for better hardware acceleration compatibility
    decoder->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT; // Handle corrupted frames gracefully
    decoder->flags2 |= AV_CODEC_FLAG2_FAST; // Prioritize speed over quality

    decoder->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    decoder->err_recognition = AV_EF_CRCCHECK | AV_EF_BITSTREAM;
    decoder->skip_frame = AVDISCARD_DEFAULT;
    decoder->skip_idct = AVDISCARD_DEFAULT;
    decoder->skip_loop_filter = AVDISCARD_DEFAULT;

    unique_dictionary opts;
    av_dict_set(opts.address_of(), "extra_hw_frames", "16", 0); // Extra surfaces for reference frames
    av_dict_set(opts.address_of(), "async_depth", "8", 0); // Async decode depth

    if (avcodec_open2(decoder.get(), codec, opts.address_of()) < 0) {
        _log.error("Failed to open codec for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    _log.info("Initialized decoder for stream {}: {}", channel, *codec_params);
    return true;
}
bool vortex::ffmpeg::StreamManager::InitAudioDecoder(vortex::ffmpeg::ManagedStream& stream, int channel)
{
    AVCodecParameters* codec_params = stream.context->streams[channel]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        _log.error("Failed to find decoder for stream {}: {}", channel, *codec_params);
        return false;
    }
    stream.channels[channel]; // Ensure the channel entry exists
    stream.channels[channel].decoder_ctx.reset(avcodec_alloc_context3(codec));
    if (!stream.channels[channel].decoder_ctx) {
        _log.error("Failed to allocate decoder context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    if (avcodec_parameters_to_context(stream.channels[channel].decoder_ctx.get(), stream.context->streams[channel]->codecpar) < 0) {
        _log.error("Failed to copy codec parameters to context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    if (avcodec_open2(stream.channels[channel].decoder_ctx.get(), codec, nullptr) < 0) {
        _log.error("Failed to open codec for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    _log.info("Initialized decoder for stream {}: {}", channel, *codec_params);
    return true;
}
void vortex::ffmpeg::StreamManager::InitDecoder(vortex::ffmpeg::ManagedStream& stream, int channel)
{
    auto codecpar = stream.context->streams[channel]->codecpar;
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        InitVideoDecoder(stream, channel);
        return;
    case AVMEDIA_TYPE_AUDIO:
        InitAudioDecoder(stream, channel);
        return;
    default: {
        _log.warn("Unsupported codec type for stream {}: {}", channel, *codecpar);
        return;
    };
    }
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
            InitDecoder(*stream, i);
        }
    } else {
        stream->channels.reserve(active_channel_indices.size());
        for (auto i : active_channel_indices) {
            InitDecoder(*stream, i);
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
