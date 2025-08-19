#pragma once
#include <filesystem>
#include <expected>
#include <system_error>
#include <vortex/gfx/texture.h>
#include <vortex/util/ffmpeg/types.h>

namespace vortex {
class Graphics;
}

namespace vortex::codec {

// Main codec class - keeping existing interface
class CodecFFmpeg
{
public:
    static std::expected<vortex::Texture2D, std::error_code>
    LoadTexture(const Graphics& gfx, const std::filesystem::path& path);

    // NEW: Just stream connection
    static std::expected<ffmpeg::unique_context, std::error_code>
    ConnectToStream(std::string_view stream_url);
};

} // namespace vortex::codec