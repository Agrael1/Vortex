#pragma once
#include <filesystem>

namespace vortex {
class Graphics;
}

namespace vortex::codec {
class CodecFFmpeg
{
public:
    static wis::Texture LoadTexture(const Graphics& gfx, const std::filesystem::path& path);
};
} // namespace vortex::codec