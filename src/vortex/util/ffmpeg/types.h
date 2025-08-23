#pragma once
#include <vortex/util/unique_any.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace vortex::ffmpeg {
// RAII wrappers for the most common FFmpeg types
using unique_context = vortex::unique_any<AVFormatContext*, avformat_close_input>;
using unique_codec_context = vortex::unique_any<AVCodecContext*, avcodec_free_context>;
using unique_frame = vortex::unique_any<AVFrame*, av_frame_free>;
using unique_swscontext = vortex::unique_any<SwsContext*, sws_freeContext>;
using unique_dictionary = vortex::unique_any<AVDictionary*, av_dict_free>;

using unique_buffer = vortex::unique_any<AVBufferRef*, av_buffer_unref>;

using unique_packet = vortex::unique_any<AVPacket, av_packet_unref>;
} // namespace vortex::ffmpeg

namespace std {
// Specializtion for std::formatter to handle AVStream
template<>
struct formatter<AVCodecParameters> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    // Format function that outputs CodecParameters fields
    template<typename FormatContext>
    auto format(const AVCodecParameters& codecpar, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "AVCodecParameters(codec_type={}, codec_id={}, width={}, height={})",
                              reflect::enum_name(codecpar.codec_type),
                              reflect::enum_name(codecpar.codec_id),
                              codecpar.width,
                              codecpar.height);
    }
};

template<>
struct formatter<AVStream> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    // Format function that outputs TextureDesc fields
    template<typename FormatContext>
    auto format(const AVStream& stream, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(),
                              "AVStream(index={}, codecpar = {})",
                              stream.index, *stream.codecpar);
    }
};
} // namespace std