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

    using unique_packet = vortex::unique_any<AVPacket, av_packet_unref>;
}
