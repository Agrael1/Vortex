#pragma once
#include <filesystem>
#include <vortex/gfx/texture.h>

namespace vortex {
class Graphics;
}

namespace vortex::codec {
class CodecFFmpeg
{
public:
    static vortex::Texture2D LoadTexture(const Graphics& gfx, const std::filesystem::path& path);
};
} // namespace vortex::codec