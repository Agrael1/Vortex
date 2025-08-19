#pragma once
#include <filesystem>
#include <expected>
#include <system_error>
#include <vortex/gfx/texture.h>
#include <vortex/util/ffmpeg/types.h>
#include <vortex/util/rational.h>

namespace vortex {
class Graphics;
}

namespace vortex::codec {
struct StreamInfo {
    int stream_index = -1;
    std::string codec_name;
    std::string media_type; // "video", "audio", "subtitle", etc.

    // Video specific
    int width = 0;
    int height = 0;
    vortex::ratio64_t frame_rate{ 0, 1 };
    vortex::ratio64_t time_base{ 0, 1 };

    // Audio specific
    int sample_rate = 0;
    int channels = 0;

    // Timing (your broken fps issue)
    vortex::ratio64_t avg_frame_rate{ 0, 1 }; // Average framerate
    vortex::ratio64_t r_frame_rate{ 0, 1 }; // Real base framerate (tbr)
    int64_t start_time = 0;
    int64_t duration = 0;
};

// Main codec class - keeping existing interface
class CodecFFmpeg
{
public:
    static std::expected<vortex::Texture2D, std::error_code>
    LoadTexture(const Graphics& gfx, const std::filesystem::path& path);

    // NEW: Just stream connection
    static std::expected<ffmpeg::unique_context, std::error_code>
    ConnectToStream(std::string_view stream_url);

    static std::expected<std::vector<StreamInfo>, std::error_code>
    GetStreamInfo(const vortex::ffmpeg::unique_context& context);

    static std::expected<StreamInfo, std::error_code>
    FindBestVideoStream(const vortex::ffmpeg::unique_context& context);
};

} // namespace vortex::codec