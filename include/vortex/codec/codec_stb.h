#pragma once
#include <wisdom/wisdom.hpp>
#include <filesystem>

namespace vortex {
class Graphics;
} // namespace vortex

namespace vortex::codec {
class STBCodec
{
public:
    static wis::Texture LoadTexture(const vortex::Graphics& gfx, const std::filesystem::path& file_path);
};
} // namespace vortex::codec