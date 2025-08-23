#pragma once
#include <vortex/util/ffmpeg/hw_decoder.h>
#include <vortex/util/lib/SPSC-Queue.h>
#include <unordered_map>
#include <shared_mutex>
#include <semaphore>
#include <thread>

namespace vortex {
class Graphics;
}

namespace vortex::ffmpeg {
struct PacketStorage {
    dro::SPSCQueue<ffmpeg::unique_packet, 16> packets; // Queue for storing AVPackets
    vortex::ffmpeg::unique_codec_context decoder_ctx;
    std::binary_semaphore decoder_sem{ 1 }; // Semaphore to signal availability of packets for decoding
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
    StreamHandle RegisterStream(ffmpeg::unique_context context, std::span<int> active_channel_indices);
    void UnregisterStream(StreamHandle handle);

    void SetChannelActive(StreamHandle handle, int stream_index, bool active);
    void ActivateChannels(StreamHandle handle, std::span<int> active_channel_indices);
    void DeactivateChannels(StreamHandle handle, std::span<int> inactive_channel_indices);

    std::optional<ffmpeg::unique_packet>
    TryGetPacket(StreamHandle handle, int stream_index);

private:
    void IoLoop(std::stop_token stop);

    std::vector<std::jthread> _io_threads;

    // Control for unpdating streams
    std::atomic<bool> _update_pending{ true };
    std::shared_mutex _streams_mutex;
    std::unordered_map<StreamHandle, std::shared_ptr<ManagedStream>> _streams;

    ffmpeg::VADecodeContext _va_decode_context; // Shared hardware decode context for Wisdom VK/DX12
};
} // namespace vortex::ffmpeg