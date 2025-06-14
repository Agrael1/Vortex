#pragma once
#include <wisdom/wisdom.hpp>
#include <filesystem>

typedef png_struct* png_structp;
typedef const char* png_const_charp;


namespace vortex {
class Graphics;
} // namespace vortex

namespace vortex::codec {
class PNGCodec
{
public:
    static void ErrorHandler(png_structp png_ptr, png_const_charp error_msg);
    static void WarningHandler(png_structp png_ptr, png_const_charp warning_msg);

public:
    static wis::Texture LoadTexture(const vortex::Graphics& gfx, const std::filesystem::path& file_path);
};
} // namespace vortex::codec