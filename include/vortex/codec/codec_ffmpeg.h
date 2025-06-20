#pragma once
#include <filesystem>

namespace vortex::codec {
class CodecFFmpeg
{
public:
    static void LoadTexture(const std::filesystem::path& path);
};
} // namespace vortex::codec