#pragma once
#include <vortex/codec/ffmpeg/hw_decoder.h>
#include <vortex/util/lib/SPSC-Queue.h>

#include <unordered_map>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <queue>

namespace vortex {
class Graphics;
}

namespace vortex::ffmpeg {
struct ChannelStorage {
    static constexpr std::size_t max_packets = 32; // Max packets sent without receiving frames
    static constexpr std::size_t max_frames = 16; // Max packets to queue for decoding

public:
    ChannelStorage(vortex::ffmpeg::unique_codec_context decoder_ctx) noexcept
        : _decoder_ctx(std::move(decoder_ctx))
    {
    }

public:
    /// @brief Attempts to decode a frame and returns the result or an error code.
    /// @return A std::expected containing a unique decoded frame on success, or an ffmpeg error code on failure.
    auto Decode() noexcept -> std::expected<ffmpeg::unique_frame, ffmpeg::ffmpeg_errc>;

    /// @brief Attempts to send all packets currently queued for transmission.
    /// @param log A LogView object used to record logging information during the operation.
    /// @return true if all queued packets were sent successfully; false otherwise.
    bool SendQueuedPackets(vortex::LogView log) noexcept;

    /// @brief Sends or queues a packet using FFmpeg.
    /// @param packet A unique FFmpeg packet to be sent.
    /// @param log A logging view used to record information or errors during the send operation.
    /// @return True if the packet was sent successfully; otherwise, false.
    bool SendPacket(ffmpeg::unique_packet packet, vortex::LogView log) noexcept;

    /// @brief Retrieves a decoded video frame, if available.
    /// @return An optional containing a unique decoded frame if one is available; otherwise, an empty optional.
    auto GetDecodedFrame() noexcept -> std::optional<ffmpeg::unique_frame>;

    /// @brief Attempts to decode a frame and add it to the frame queue if possible.
    /// @return true if a frame was successfully decoded and added to the queue; false otherwise (e.g., if the queue is full or decoding failed).
    bool TryDecodeFrame() noexcept;

    /// @brief Checks if the decoder context is valid and properly initialized.
    /// @return true if the decoder context exists and has a valid codec; false otherwise.
    auto IsValid() const noexcept -> bool
    {
        return _decoder_ctx && _decoder_ctx->codec;
    }

    /// @brief Checks if the packet queue has reached its maximum capacity.
    /// @return true if the packet queue is full; false otherwise.
    bool IsOverflown() const noexcept
    {
        return _packets.size() == max_packets;
    }

    /// @brief Checks if the frame queue has reached its maximum capacity.
    /// @return true if the frame queue is full; false otherwise.
    bool IsFrameQueueFull() const noexcept
    {
        return _frames.size() == max_frames;
    }

private:
    std::queue<ffmpeg::unique_packet> _packets; // This does not need to be thread-safe, only accessed from I/O thread
    dro::SPSCQueue<ffmpeg::unique_frame, max_frames> _frames; // Frames decoded and ready for consumption
    vortex::ffmpeg::unique_codec_context _decoder_ctx;
};

// Represents a stream being read by the StreamManager
struct ManagedStream {
    struct UpdateRequest {
        uint32_t stream_index : 31;
        uint32_t active : 1; // 1 for activate, 0 for deactivate
    };

    // Only accessed from the I/O thread
    ffmpeg::unique_context context;
    std::unordered_map<int, ChannelStorage> channels;
    dro::SPSCQueue<ffmpeg::unique_packet, 64> read_queue; // Packets read from the stream, to be sent to decoders

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
    void PacketLoop(std::stop_token stop);
    bool ReadStreamPackets(std::stop_token stop, vortex::ffmpeg::ManagedStream& stream);


    void VideoDecodeLoop(std::stop_token stop);
    void AudioDecodeLoop(std::stop_token stop);

    void IOLoop(std::stop_token stop);
    void IOFlushStream(vortex::ffmpeg::ManagedStream& stream);
    bool IOProcessStream(vortex::ffmpeg::ManagedStream& stream);

    static void AvLogCallbackThunk(void* ptr, int level, const char* fmt, va_list vargs);

    vortex::LogView _log;
    std::vector<std::jthread> _io_threads;

    // Control for unpdating streams
    std::atomic<uint64_t> _update_generation{ 0 };
    std::shared_mutex _streams_mutex;
    std::unordered_map<StreamHandle, std::shared_ptr<ManagedStream>> _streams;

    ffmpeg::VADecodeContext _va_decode_context; // Shared hardware decode context for Wisdom VK/DX12
};
} // namespace vortex::ffmpeg