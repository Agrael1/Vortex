#include <vortex/codec/ffmpeg/stream_manager.h>
#include <vortex/codec/ffmpeg/error.h>
#include <vortex/graphics.h>
#include <system_error>

void vortex::ffmpeg::StreamManager::AvLogCallbackThunk(void* ptr, int level, const char* fmt, va_list vargs)
{
    static auto log = vortex::GetLog(vortex::stream_log_name); // Static to avoid repeated lookups

    // Get only above info level
    if (level >= AV_LOG_INFO) {
        return;
    }

    char message[1024];
    vsnprintf(message, sizeof(message), fmt, vargs);

    // Remove trailing newlines
    size_t len = strlen(message);
    while (len > 0 && (message[len - 1] == '\n' || message[len - 1] == '\r')) {
        message[--len] = '\0';
    }

    switch (level) {
    case AV_LOG_QUIET:
        return; // No logging
    case AV_LOG_PANIC:
    case AV_LOG_FATAL:
        log.critical("[FFmpeg] {}", message);
        break;
    case AV_LOG_ERROR:
        log.error("[FFmpeg] {}", message);
        break;
    case AV_LOG_WARNING:
        log.warn("[FFmpeg] {}", message);
        break;
    case AV_LOG_INFO:
        log.info("[FFmpeg] {}", message);
        break;
    case AV_LOG_VERBOSE:
    case AV_LOG_DEBUG:
        log.debug("[FFmpeg] {}", message);
        break;
    default:
        log.info("[FFmpeg] {}", message);
        break;
    }
}

vortex::ffmpeg::StreamManager::StreamManager(const vortex::Graphics& gfx)
    : _log(vortex::GetLog(vortex::stream_log_name))
{
    // Set FFmpeg log callback
    av_log_set_callback(AvLogCallbackThunk);

    _va_decode_context = ffmpeg::CreateDecodeContext(gfx.GetDevice()).value();

    // Start a single packet reading thread
    _io_threads.emplace_back([this](std::stop_token stop) { PacketLoop(stop); });

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

void vortex::ffmpeg::StreamManager::PacketLoop(std::stop_token stop)
{
    _log.info("Packet thread started.");
    std::vector<std::shared_ptr<ManagedStream>> streams_to_read;
    uint64_t last_update_generation = 0;

    while (!stop.stop_requested()) {
        // In order to stabilize the input stream, we read packets from all streams in a round-robin fashion.
        uint64_t current_update_generation = _update_generation.load(std::memory_order::acquire);
        if (current_update_generation != last_update_generation) {
            std::shared_lock lock(_streams_mutex);
            streams_to_read.clear();
            for (const auto& pair : _streams) {
                streams_to_read.push_back(pair.second);
            }
            last_update_generation = current_update_generation;
        }

        if (streams_to_read.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        for (const auto& stream : streams_to_read) {
            ReadStreamPackets(stop, *stream);
        }
    }
    _log.info("Packet thread stopped.");
}
bool vortex::ffmpeg::StreamManager::ReadStreamPackets(
        std::stop_token stop,
        vortex::ffmpeg::ManagedStream& stream)
{
    ffmpeg::unique_packet packet{ av_packet_alloc() };
    int ret = av_read_frame(stream.context.get(), packet.get());
    if (ret == AVERROR(EAGAIN)) {
        // No packet available right now, try again later
        return false;
    }
    if (ret >= 0) {
        if (stream.read_queue.size() == 64) {
            // Read queue is full, drop the packet
            _log.warn("Read queue full, force pushing for stream index {}", packet->stream_index);
        }

        // Successfully read a packet, now dispatch it to the appropriate channel
        stream.read_queue.force_emplace(std::move(packet)); // Non-blocking enqueue
        return true;
    }

    if (ret == AVERROR_EOF) {
        // End of stream, send flush packets to all decoders
        IOFlushStream(stream);
        return false;
    }
    _log.error("Error reading frame from stream: {}", ffmpeg::ffmpeg_error_string(ret));
    return false;
}

void vortex::ffmpeg::StreamManager::IOLoop(std::stop_token stop)
{
    _log.info("I/O thread started.");
    std::vector<std::shared_ptr<ManagedStream>> streams_to_read;
    uint64_t last_update_generation = 0;

    while (!stop.stop_requested()) {
        // Copy the list of streams to read to minimize lock time
        uint64_t current_update_generation = _update_generation.load(std::memory_order::acquire);
        if (current_update_generation != last_update_generation) {
            std::shared_lock lock(_streams_mutex);
            streams_to_read.clear();
            for (const auto& pair : _streams) {
                streams_to_read.push_back(pair.second);
            }
            last_update_generation = current_update_generation;
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
        if (!channel.IsValid()) {
            continue;
        }

        // Enqueue a flush packet for the decoder
        channel.SendPacket(nullptr, _log);
    }
}
bool vortex::ffmpeg::StreamManager::IOProcessStream(vortex::ffmpeg::ManagedStream& stream)
{
    // Check if there are any active channels
    if (stream.channels.empty()) {
        return false;
    }

    // Check if any decoder is overloaded with sent packets
    while (true) {
        for (auto& [index, channel] : stream.channels) {
            // Try to send queued packets to free up space
            bool ok = channel.SendQueuedPackets(_log);
            if (!ok && channel.IsOverflown()) {
                _log.warn("Decoder for stream {} is overloaded and cannot send queued packets.", index);
                return false; // If sending queued packets failed, skip reading new packets
            }
        }

        // This is the core of the non-blocking read. av_read_frame will return
        // after a timeout (set in StreamInput::initialize_stream) or when a packet arrives.

        ffmpeg::unique_packet packet;
        bool got_packet = stream.read_queue.try_pop(packet);
        if (!got_packet) {
            return false; // No more packets to process right now
        }

        // Successfully read a packet, now dispatch it to the appropriate channel
        auto it = stream.channels.find(packet->stream_index);
        if (it == stream.channels.end()) {
            // No active channel for this stream index, discard the packet
            continue;
        }
        auto& channel = it->second;

        bool ok = channel.SendPacket(std::move(packet), _log);
    }
    return true;
}

bool vortex::ffmpeg::StreamManager::InitVideoDecoder(vortex::ffmpeg::ManagedStream& stream, int channel)
{
    AVCodecParameters* codec_params = stream.context->streams[channel]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        _log.error("Failed to find decoder for stream {}: {}", channel, *codec_params);
        return false;
    }

    ffmpeg::unique_codec_context decoder_ctx{ avcodec_alloc_context3(codec) };
    if (!decoder_ctx) {
        _log.error("Failed to allocate decoder context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    if (avcodec_parameters_to_context(decoder_ctx.get(), codec_params) < 0) {
        _log.error("Failed to copy codec parameters to context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }

    // Setup hardware acceleration context
    decoder_ctx->hw_device_ctx = av_buffer_ref(_va_decode_context.GetHWDeviceContext());
    auto frames_ctx_result = _va_decode_context.CreateHWFramesContext(
            decoder_ctx->width,
            decoder_ctx->height,
            AV_PIX_FMT_NV12);

    if (!frames_ctx_result) {
        _log.error("Failed to create HW frames context for stream {}: {}", channel, frames_ctx_result.error().message());
        stream.channels.erase(channel);
        return false;
    }
    decoder_ctx->hw_frames_ctx = frames_ctx_result.value().release();
    decoder_ctx->thread_count = 1; // Use single-threaded decoding for hardware acceleration
    decoder_ctx->thread_type = FF_THREAD_FRAME;

    // Configure decoder for better hardware acceleration compatibility
    decoder_ctx->flags |= AV_CODEC_FLAG_OUTPUT_CORRUPT; // Handle corrupted frames gracefully
    decoder_ctx->flags2 |= AV_CODEC_FLAG2_FAST; // Prioritize speed over quality

    decoder_ctx->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    decoder_ctx->err_recognition = AV_EF_CAREFUL | AV_EF_COMPLIANT | AV_EF_AGGRESSIVE;

    unique_dictionary opts;
    av_dict_set(opts.address_of(), "extra_hw_frames", "16", 0); // Extra surfaces for reference frames
    av_dict_set(opts.address_of(), "async_depth", "8", 0); // Async decode depth

    if (avcodec_open2(decoder_ctx.get(), codec, opts.address_of()) < 0) {
        _log.error("Failed to open codec for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    _log.info("Initialized decoder for stream {}: {}", channel, *codec_params);
    stream.channels.emplace(channel, std::move(decoder_ctx));
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

    ffmpeg::unique_codec_context decoder_ctx{ avcodec_alloc_context3(codec) };
    if (!decoder_ctx) {
        _log.error("Failed to allocate decoder context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    if (avcodec_parameters_to_context(decoder_ctx.get(), codec_params) < 0) {
        _log.error("Failed to copy codec parameters to context for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    if (avcodec_open2(decoder_ctx.get(), codec, nullptr) < 0) {
        _log.error("Failed to open codec for stream {}: {}", channel, *codec_params);
        stream.channels.erase(channel);
        return false;
    }
    _log.info("Initialized decoder for stream {}: {}", channel, *codec_params);
    stream.channels.emplace(channel, std::move(decoder_ctx)); // Ensure the channel entry exists
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
    _update_generation.fetch_add(1, std::memory_order::relaxed);
    return handle;
}
void vortex::ffmpeg::StreamManager::UnregisterStream(StreamHandle handle)
{
    if (!handle) {
        return;
    }
    std::unique_lock lock(_streams_mutex);
    _streams.erase(handle);
    _update_generation.fetch_add(1, std::memory_order::relaxed);
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

auto vortex::ffmpeg::ChannelStorage::Decode() noexcept -> std::expected<vortex::ffmpeg::unique_frame, vortex::ffmpeg::ffmpeg_errc>
{
    ffmpeg::unique_frame frame{ av_frame_alloc() };
    AVCodecContext* ctx = _decoder_ctx.get();
    AVFrame* raw_frame = frame.get();
    int ret = 0;

    ret = avcodec_receive_frame(ctx, raw_frame);
    if (ret == AVERROR(EAGAIN)) {
        return std::unexpected{ ffmpeg_errc::not_enough_data };
    }
    if (ret == AVERROR_EOF) {
        return std::unexpected{ ffmpeg_errc::end_of_file };
    }
    if (ret < 0) {
        vortex::error("Error during decoding video frame: {}", ffmpeg::ffmpeg_error_string(ret));
        return std::unexpected{ ffmpeg::ffmpeg_errc(ret) };
    }
    return std::expected<ffmpeg::unique_frame, ffmpeg::ffmpeg_errc>(std::move(frame)); // Successfully received a frame
}
auto vortex::ffmpeg::ChannelStorage::SendQueuedPackets(vortex::LogView log) noexcept -> bool
{
    // We have the semaphore, send the packet
    if (_packets.empty()) {
        return true; // Nothing to send
    }

    std::size_t sent_count = 0;
    // If there are queued packets, send them first
    while (!_packets.empty()) {
        auto& queued_packet = _packets.front();
        int result = avcodec_send_packet(_decoder_ctx.get(), queued_packet.get());
        switch (result) {
        case 0:
            break; // Successfully sent
        case AVERROR(EAGAIN):
            if (IsFrameQueueFull()) {
                return false; // Frame queue is full, cannot send more packets
            }

            while (TryDecodeFrame()) {
                // Keep decoding frames until we can't anymore
            }
            continue; // Resend the same packet
        case AVERROR_EOF:
            _packets = {}; // Clear all packets
            log.info("Decoder flushed, clearing queued packets.");
            return false; // Decoder flushed
        default:
            log.error("Error sending queued packet to decoder: {}", ffmpeg::ffmpeg_error_string(result));
            return false; // Problem sending packet. Stop processing.
        }
        _packets.pop();
    }
}
auto vortex::ffmpeg::ChannelStorage::SendPacket(ffmpeg::unique_packet packet, vortex::LogView log) noexcept -> bool
{
    // We have the semaphore, send the packet
    int result = avcodec_send_packet(_decoder_ctx.get(), packet.get());
    auto pts = packet->pts;
    switch (result) {
    case 0:
        return true; // Successfully sent
    case AVERROR(EAGAIN):
        _packets.emplace(std::move(packet));
        return true; // Decoder not ready, try again later
    case AVERROR_EOF:
        _packets = {};
        log.info("Decoder flushed, clearing queued packets.");
        return false; // Decoder flushed
    default:
        log.error("Error sending packet to decoder: {}", ffmpeg::ffmpeg_error_string(result));
        return false; // Problem sending packet. Stop processing.
    }
}
auto vortex::ffmpeg::ChannelStorage::GetDecodedFrame() noexcept -> std::optional<ffmpeg::unique_frame>
{
    ffmpeg::unique_frame frame;
    auto sz = _frames.size();

    if (_frames.try_pop(frame)) {
        return std::optional<ffmpeg::unique_frame>{ std::move(frame) };
    }
    return std::nullopt;
}
auto vortex::ffmpeg::ChannelStorage::TryDecodeFrame() noexcept -> bool
{
    if (IsFrameQueueFull()) {
        return false; // Frame queue is full, cannot decode more frames
    }

    auto decode_result = Decode();
    if (!decode_result) {
        auto err = decode_result.error();
        if (err == ffmpeg_errc::end_of_file) {
            _packets = {}; // Clear all packets
        }
        return false; // Decoder not ready, try again later
    }
    return _frames.try_push(std::move(decode_result.value()));
}
