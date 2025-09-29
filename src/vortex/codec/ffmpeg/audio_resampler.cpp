#include <vortex/codec/ffmpeg/audio_resampler.h>

std::error_code
vortex::ffmpeg::AudioResampler::Reset(ResamplerDesc desc) noexcept
{
    int result = swr_alloc_set_opts2(_swr_ctx.address_of(), // Context to allocate/reallocate
                                     desc.dst_layout.address_of(), // Output channel layout
                                     desc.dst_format, // Output sample format
                                     desc.dst_rate, // Output sample rate
                                     desc.src_layout.address_of(), // Input channel layout
                                     desc.src_format, // Input sample format
                                     desc.src_rate, // Input sample rate
                                     0, // Log offset (unused)
                                     nullptr); // Log context (unused)
    if (result < 0) {
        return ffmpeg::make_ffmpeg_error(result);
    }
    result = swr_init(_swr_ctx.get());
    if (result < 0) {
        _swr_ctx.reset(); // Free the context on failure
        return ffmpeg::make_ffmpeg_error(result);
    }
    this->desc = std::move(desc); // Store the description
    return {}; // Success
}

std::error_code vortex::ffmpeg::AudioResampler::Resample(std::span<const std::byte> samples, std::span<std::byte> output) noexcept
{
    std::size_t output_size = GetOutputBufferSize(samples.size() / (av_get_bytes_per_sample(desc.src_format) * desc.src_layout->nb_channels));
    if (output_size == 0) {
        return {}; // Nothing to do
    }
    if (output.size() < output_size) {
        return ffmpeg::make_ffmpeg_error(AVERROR(EINVAL)); // Output buffer too small
    }
    const uint8_t* in_data[1] = { reinterpret_cast<const uint8_t*>(samples.data()) };
    int in_samples = int(samples.size() / (av_get_bytes_per_sample(desc.src_format) * desc.src_layout->nb_channels));
    uint8_t* out_data[1] = { reinterpret_cast<uint8_t*>(output.data()) };
    int out_samples = int(output.size() / (av_get_bytes_per_sample(desc.dst_format) * desc.dst_layout->nb_channels));
    int result = swr_convert(_swr_ctx.get(), out_data, out_samples, in_data, in_samples);
    if (result < 0) {
        return ffmpeg::make_ffmpeg_error(result); // Conversion error
    }
    return {}; // Success
}

std::error_code vortex::ffmpeg::AudioResampler::ResamplePlanar(std::span<const std::byte> samples, std::span<std::span<std::byte>> output) noexcept
{
    return ffmpeg::make_ffmpeg_error(AVERROR(ENOSYS)); // Not implemented
}


std::expected<vortex::ffmpeg::AudioResampler, std::error_code> 
vortex::ffmpeg::CreateAudioResampler(ResamplerDesc desc) noexcept
{
    AudioResampler resampler;
    auto err = resampler.Reset(std::move(desc));
    if (err.value() != int(ffmpeg_errc::success)) {
        return std::unexpected(err);
    }
    return std::expected<vortex::ffmpeg::AudioResampler, std::error_code>(std::move(resampler));
}
