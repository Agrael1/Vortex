#pragma once
#include <filesystem>
#include <expected>
#include <system_error>
#include <chrono>
#include <vortex/gfx/texture.h>
#include <vortex/codec/ffmpeg/types.h>
#include <vortex/util/rational.h>
#include <unordered_map>

namespace vortex {
class Graphics;
}

namespace vortex::codec {
struct StreamChannels {
    std::vector<AVStream*> video_channels;
    std::vector<AVStream*> audio_channels;
};

// Main codec class - keeping existing interface
class CodecFFmpeg
{
public:
    static std::expected<vortex::Texture2D, std::error_code>
    LoadTexture(const Graphics& gfx, const std::filesystem::path& path);

    static std::expected<ffmpeg::unique_context, std::error_code>
    ConnectToStream(std::string_view stream_url,
                    ffmpeg::unique_dictionary context_options = ffmpeg::unique_dictionary{},
                    std::chrono::microseconds timeout = std::chrono::microseconds{ 5000000 });

    static std::expected<vortex::codec::StreamChannels, std::error_code>
    GetStreams(const AVFormatContext* context);

    static std::expected<AVStream*, std::error_code>
    GetBestStream(AVFormatContext* context, AVMediaType stream_type);
};

} // namespace vortex::codec