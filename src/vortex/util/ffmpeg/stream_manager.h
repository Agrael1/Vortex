#pragma once
#include <vortex/util/ffmpeg/hw_decoder.h>
#include <unordered_map>
#include <shared_mutex>
#include <semaphore>
#include <thread>
#include <chrono>

#include <map>
#include <vortex/util/lib/SPSC-Queue.h>

namespace vortex {
class Graphics;
}

namespace vortex::ffmpeg {
struct PacketStorage {
    static constexpr std::size_t max_sent_packets = 512; // Max packets sent without receiving frames
    static constexpr std::size_t max_frames = 16; // Max packets to queue for decoding

    std::map<int64_t, ffmpeg::unique_packet> packets; // This does not need to be thread-safe, only accessed from I/O thread
    dro::SPSCQueue<ffmpeg::unique_frame, max_frames> frames; // Frames decoded and ready for consumption

    // Accessed from both I/O and main thread
    vortex::ffmpeg::unique_codec_context decoder_ctx;
    std::binary_semaphore decoder_sem{ 1 }; // Semaphore to signal availability of packets for decoding
    std::atomic<std::size_t> sent_packets{ 0 }; // Number of packets sent to the decoder

    auto Decode() -> std::expected<ffmpeg::unique_frame, ffmpeg::ffmpeg_errc>;
    auto DecodeSync() -> std::expected<ffmpeg::unique_frame, ffmpeg::ffmpeg_errc>;
    auto SendQueuedPackets(vortex::LogView log) -> bool;
    auto SendPacket(ffmpeg::unique_packet packet, vortex::LogView log) -> bool;
};

// Represents a stream being read by the StreamManager
struct ManagedStream {
    struct UpdateRequest {
        uint32_t stream_index : 31;
        uint32_t active : 1; // 1 for activate, 0 for deactivate
    };

    // Only accessed from the I/O thread
    ffmpeg::unique_context context;
    std::unordered_map<int, PacketStorage> channels;

    // Modifiable from outside the I/O thread
    std::atomic<bool> update_pending{ false };
    std::vector<UpdateRequest> updates;
};

// Manages all stream I/O in a dedicated thread pool.
class StreamManager
{
public:
    using StreamHandle = uintptr_t;

public:
    StreamManager(const vortex::Graphics& gfx);
    ~StreamManager();

public:
    bool InitVideoDecoder(vortex::ffmpeg::ManagedStream& stream, int channel);
    bool InitAudioDecoder(vortex::ffmpeg::ManagedStream& stream, int channel);
    void InitDecoder(vortex::ffmpeg::ManagedStream& stream, int i);
    StreamHandle RegisterStream(ffmpeg::unique_context context, std::span<int> active_channel_indices);
    void UnregisterStream(StreamHandle handle);

    void SetChannelActive(StreamHandle handle, int stream_index, bool active);
    void ActivateChannels(StreamHandle handle, std::span<int> active_channel_indices);
    void DeactivateChannels(StreamHandle handle, std::span<int> inactive_channel_indices);

private:
    void IOLoop(std::stop_token stop);
    void IOFlushStream(vortex::ffmpeg::ManagedStream& stream);
    bool IOProcessStream(vortex::ffmpeg::ManagedStream& stream);

    static void AvLogCallbackThunk(void* ptr, int level, const char* fmt, va_list vargs);

    vortex::LogView _log;
    std::vector<std::jthread> _io_threads;

    // Control for unpdating streams
    std::atomic<bool> _update_pending{ true };
    std::shared_mutex _streams_mutex;
    std::unordered_map<StreamHandle, std::shared_ptr<ManagedStream>> _streams;

    ffmpeg::VADecodeContext _va_decode_context; // Shared hardware decode context for Wisdom VK/DX12
};
} // namespace vortex::ffmpeg