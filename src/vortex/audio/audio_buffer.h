#pragma once
#include <vortex/util/byte_ring.h>

namespace vortex {
enum AudioDataRate : uint16_t {
    Unknown = 0,

    Int,
    Float,
};

struct AudioDataFormat {
    uint16_t format : 16; // Lower 15 bits for format
    uint16_t bytes_per_sample : 8; // Bits per sample (e.g., 1, 2, 4)
    uint16_t planar : 1; // 1 if planar, 0 if interleaved
};
constexpr AudioDataFormat Float32Planar{ .format = AudioDataRate::Float, .bytes_per_sample = 4, .planar = 1 };
constexpr AudioDataFormat Int16Interleaved{ .format = AudioDataRate::Int, .bytes_per_sample = 2, .planar = 0 };

struct AudioFormat {
    AudioDataFormat data_format = Float32Planar;
    uint32_t sample_rate = 48000; // Samples per second
    uint16_t channels = 2; // Number of audio channels (e.g., 2 for stereo)
};

class AudioBuffer
{
public:
    AudioBuffer(const AudioFormat& format, std::chrono::milliseconds buffer_length) noexcept
        : AudioBuffer(format, (buffer_length.count() * format.sample_rate) / 1000)
    {
    }
    AudioBuffer(const AudioFormat& format, std::size_t buffer_length_samples) noexcept
        : _format(format)
    {
        // for planar format allocate buffer for each channel
        _buffers = std::make_unique_for_overwrite<vortex::byte_ring[]>(format.channels);
        for (uint16_t ch = 0; ch < format.channels; ++ch) {
            auto err = _buffers[ch].reserve(buffer_length_samples); // 1 second buffer
            if (err != std::errc{}) {
                vortex::error("AudioBuffer: Failed to reserve buffer for channel {}: {}", ch, static_cast<int>(err));
            }
        }
    }

public:
    std::errc Write(std::span<const std::byte> data, uint16_t channel) noexcept
    {
        if (channel >= _format.channels) {
            return std::errc::invalid_argument;
        }
        if (data.empty()) {
            return {};
        }
        // Calculate bytes per sample
        size_t bytes_per_sample = _format.data_format.bytes_per_sample;
        // Ensure data size is a multiple of bytes per sample
        if (data.size() % bytes_per_sample != 0) {
            return std::errc::invalid_argument;
        }
        return _buffers[channel].write(data);
    }
    std::size_t Read(std::span<std::byte> buffer, uint16_t channel, size_t max_samples = std::numeric_limits<size_t>::max()) noexcept
    {
        if (channel >= _format.channels) {
            return 0;
        }
        if (buffer.empty()) {
            return 0;
        }
        // Calculate bytes per sample
        size_t bytes_per_sample = _format.data_format.bytes_per_sample;
        size_t max_bytes = max_samples * bytes_per_sample;
        if (max_bytes > buffer.size()) {
            max_bytes = buffer.size() - (buffer.size() % bytes_per_sample); // Align to sample size
        }
        return _buffers[channel].read(buffer.subspan(0, max_bytes));
    }

public:
    std::unique_ptr<vortex::byte_ring[]> _buffers;
    AudioFormat _format;
};
} // namespace vortex