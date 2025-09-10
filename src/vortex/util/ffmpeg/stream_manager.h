#pragma once
#include <vortex/util/ffmpeg/hw_decoder.h>
#include <unordered_map>
#include <shared_mutex>
#include <semaphore>
#include <thread>
#include <queue>
#include <chrono>

namespace vortex {
class Graphics;
}

namespace vortex::ffmpeg {
struct PacketStorage {
    static constexpr std::size_t max_sent_packets = 512; // Max packets sent without receiving frames

    std::queue<ffmpeg::unique_packet> packets; // This does not need to be thread-safe, only accessed from I/O thread

    // Accessed from both I/O and main thread
    vortex::ffmpeg::unique_codec_context decoder_ctx;
    std::binary_semaphore decoder_sem{ 1 }; // Semaphore to signal availability of packets for decoding
    std::atomic<std::size_t> sent_packets{ 0 }; // Number of packets sent to the decoder

    std::expected<ffmpeg::unique_frame, ffmpeg::ffmpeg_errc> Decode()
    {
        ffmpeg::unique_frame frame{ av_frame_alloc() };
        AVCodecContext* ctx = decoder_ctx.get();
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
    std::expected<ffmpeg::unique_frame, ffmpeg::ffmpeg_errc> DecodeSync()
    {
        decoder_sem.acquire();
        auto frame = Decode();
        decoder_sem.release();
        return frame; // Successfully received a frame
    }
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