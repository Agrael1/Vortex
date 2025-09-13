#pragma once
#include <vortex/util/ffmpeg/types.h>
#include <vortex/util/ffmpeg/error.h>
#include <expected>

namespace vortex::ffmpeg {
struct ResamplerDesc {
    static constexpr AVChannelLayout stereo_layout{
        .order = AV_CHANNEL_ORDER_NATIVE,
        .nb_channels = 2,
        .u = { .mask = AV_CH_LAYOUT_STEREO },
        .opaque = nullptr
    };

    ffmpeg::unique_channel_layout src_layout{ stereo_layout }; // Source channel layout (e.g., AV_CH_LAYOUT_STEREO)
    ffmpeg::unique_channel_layout dst_layout{ stereo_layout }; // Destination channel layout

    AVSampleFormat src_format; // Source sample format (e.g., AV_SAMPLE_FMT_FLTP)
    AVSampleFormat dst_format = AV_SAMPLE_FMT_FLTP; // Destination sample format

    int src_rate = 48000; // Source sample rate (e.g., 44100)
    int dst_rate = 48000; // Destination sample rate
};

class AudioResampler
{
public:
    AudioResampler() = default;

public:
    std::error_code Reset(ResamplerDesc desc) noexcept;
    void Reset()
    {
        _swr_ctx.reset();
    }

    std::ptrdiff_t GetOutputBufferSize(size_t in_samples) const noexcept
    {
        return GetOutputSampleCount(in_samples * av_get_bytes_per_sample(desc.dst_format));
    }
    std::ptrdiff_t GetOutputSampleCount(size_t in_samples) const noexcept
    {
        return swr_get_out_samples(_swr_ctx.get(), int(in_samples));
    }

    std::error_code Resample(std::span<const std::byte> samples, std::span<std::byte> output) noexcept;
    std::error_code ResamplePlanar(std::span<const std::byte> samples, std::span<std::span<std::byte>> output) noexcept;

private:


private:
    vortex::ffmpeg::unique_swrcontext _swr_ctx;
    ResamplerDesc desc;
};

std::expected<AudioResampler, std::error_code> CreateAudioResampler(ResamplerDesc desc) noexcept;
} // namespace vortex::ffmpeg