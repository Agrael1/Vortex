#pragma once
#include <filesystem>
#include <expected>
#include <system_error>
#include <chrono>
#include <vortex/gfx/texture.h>
#include <vortex/util/ffmpeg/types.h>
#include <vortex/util/rational.h>

namespace vortex {
class Graphics;
}

namespace vortex::codec {
struct StreamCollection {
    std::vector<AVStream*> video_streams;
    std::vector<AVStream*> audio_streams;
};

// Main codec class - keeping existing interface
class CodecFFmpeg
{
public:
    static std::expected<vortex::Texture2D, std::error_code>
    LoadTexture(const Graphics& gfx, const std::filesystem::path& path);

    static std::expected<ffmpeg::unique_context, std::error_code>
    ConnectToStream(std::string_view stream_url, std::chrono::microseconds timeout = std::chrono::microseconds{ 5000000 });

    static std::expected<vortex::codec::StreamCollection, std::error_code>
    GetStreams(const AVFormatContext* context);

    static std::expected<AVStream*, std::error_code>
    GetBestStream(AVFormatContext* context, AVMediaType stream_type);
};

} // namespace vortex::codec