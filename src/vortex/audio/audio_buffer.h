#pragma once
#include <vortex/util/byte_ring.h>
#include <vortex/util/rational.h>
#include <vortex/util/log.h>

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
    template<typename T = std::byte, std::size_t Extent = std::dynamic_extent>
    std::size_t Read(std::span<T, Extent> buffer, uint16_t channel, size_t max_samples = std::numeric_limits<size_t>::max()) noexcept
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
        return _buffers[channel].read_as<T>(buffer.subspan(0, max_bytes));
    }

    // Read planar data into single buffer
    template<typename T = std::byte, std::size_t Extent = std::dynamic_extent>
    std::size_t ReadPlanar(std::span<T, Extent> output)
    {
        std::size_t samples_per_channel = output.size() / _format.channels;
        assert(samples_per_channel > 0);
        assert(output.size() % _format.channels == 0);

        std::size_t result = 0;
        for (uint16_t ch = 0; ch < _format.channels; ++ch) {
            std::size_t local = Read<T>(output.subspan(ch * samples_per_channel, samples_per_channel), ch);
            if (local < samples_per_channel) {
                vortex::warn("AudioBuffer: Channel {} underflow: requested {} samples, got {}", ch, samples_per_channel, local);
                // Zero out the rest of the buffer
                std::memset(output.data() + (ch * samples_per_channel + local), 0, (samples_per_channel - local) * sizeof(T));
            }
        }
        return result; // All requested samples read
    }
    template<typename T = std::byte, std::size_t Extent = std::dynamic_extent>
    void WritePlanar(std::span<T, Extent> input)
    {
        std::size_t samples_per_channel = input.size() / _format.channels;
        assert(samples_per_channel > 0);
        assert(input.size() % _format.channels == 0);
        for (uint16_t ch = 0; ch < _format.channels; ++ch) {
            // Write is really sensitive
            auto err = Write(std::as_bytes(input.subspan(ch * samples_per_channel, samples_per_channel)), ch);
            if (err != std::errc{}) {
                vortex::error("AudioBuffer: Failed to write to channel {}: {}", ch, reflect::enum_name(err));
            }
        }
    }

    std::size_t AvailableSamples(uint16_t channel) const noexcept
    {
        if (channel >= _format.channels) {
            return 0;
        }
        size_t bytes_per_sample = _format.data_format.bytes_per_sample;
        return _buffers[channel].size() / bytes_per_sample;
    }
    std::size_t GetFullSamples() const noexcept
    {
        // Get min samples across all the channels
        std::size_t total_samples = std::numeric_limits<std::size_t>::max();
        for (uint16_t ch = 0; ch < _format.channels; ++ch) {
            size_t samples = AvailableSamples(ch);
            if (samples < total_samples) {
                total_samples = samples;
            }
        }
        return total_samples;
    }
    std::size_t SamplesForFramerate(vortex::ratio32_t framerate) const noexcept
    {
        if (framerate.denominator == 0 || framerate.numerator == 0) {
            return 0;
        }
        return (uint32_t(_format.sample_rate) * framerate.denominator) / framerate.numerator;
    }
    bool CanReadForFramerate(vortex::ratio32_t framerate) const noexcept
    {
        auto needed = SamplesForFramerate(framerate);
        return GetFullSamples() >= needed;
    }
    void Reset() noexcept
    {
        for (uint16_t ch = 0; ch < _format.channels; ++ch) {
            _buffers[ch].clear();
        }
    }

public:
    std::unique_ptr<vortex::byte_ring[]> _buffers;
    AudioFormat _format;
};
} // namespace vortex